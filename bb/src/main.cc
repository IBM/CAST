/*******************************************************************************
 |    main.cc
 |
 |  © Copyright IBM Corporation 2015,2016. All Rights Reserved
 |
 |    This program is licensed under the terms of the Eclipse Public License
 |    v1.0 as published by the Eclipse Foundation and available at
 |    http://www.eclipse.org/legal/epl-v10.html
 |
 |    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
 |    restricted by GSA ADP Schedule Contract with IBM Corp.
 *******************************************************************************/

#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <execinfo.h>
#include <iostream>
#include <ucontext.h>
#include <boost/program_options.hpp>
#include <sys/syscall.h>
#define NORASEXTERN
#include "bbras.h"

#include "connections.h"

using namespace std;

#include "bbinternal.h"
#include "nodecontroller.h"
#include "logging.h"

#if BBSERVER
#include "bbserver_flightlog.h"
#define NAME "bbServer"
#define WHOAMI DEFAULT_SERVER_NAME
#endif

#if BBPROXY
#include "bbproxy_flightlog.h"
#define NAME "bbProxy"
#define WHOAMI DEFAULT_PROXY_NAME
#endif

#ifndef WHOAMI
#define WHOAMI "unknown"
#endif

#ifdef PROF_TIMING
#include <ctime>
#include <chrono>
#endif

namespace po = boost::program_options;

static string who;

//  NOTE: Definition for bbServer and bbProxy.
//        Set for bbProxy, empty string for bbServer.
string ProcessId = "";

sem_t block_until_exit_request;

int bb_main(std::string who);
int set_bb_nvmecliPath(const string& executable);
int set_bb_nvmfConnectPath(const string& executable);

extern int bb_exit(std::string who);
int registerHandlers(); //bb messages handlers

int    main_argc;
char** main_argv;
int signalPipe[2];

#define SETSIGNAL(sig) if(sigaction(sig, &sact, NULL)) { cerr << "Error initializing signal handler " << #sig << " errno=" << errno << "(" << strerror(errno) << ")" << endl; }

#define CONFIGURESIGNALS() \
    SETSIGNAL(SIGINT);     \
    SETSIGNAL(SIGHUP);     \
    SETSIGNAL(SIGTERM);    \
    SETSIGNAL(SIGSEGV);    \
    SETSIGNAL(SIGBUS);     \
    SETSIGNAL(SIGFPE);     \
    SETSIGNAL(SIGILL);     \
    SETSIGNAL(SIGABRT);

void flightlog_Backtrace(uint64_t key)
{
    int x;
    void*  traceBuf[256];
    int traceSize;
    pid_t tid = syscall(SYS_gettid);
    traceSize = backtrace(traceBuf, sizeof(traceBuf)/sizeof(void*));

    if(FLError)
    {
        for(x=0; x<traceSize; x++)
        {
            FL_Write(FLError, Backtrace, "BACKTRACE(key=%ld  tid=%ld  frame=%ld): %lx", key, tid, x, (uint64_t)traceBuf[x]);
        }
    }
}

void exitSignalHandler(int sig, siginfo_t* si, void* ptr)
{
    void*  traceBuf[256];
    int traceSize;
    char buffer[1024];
#if __powerpc64__
    ucontext_t* uc = (ucontext_t*)ptr;
#endif

    /* Note:  Since the pipe isn't drained until the semaphore is posted, the size of signal
       data cannot exceed size of pipe queue...  which is allegedly 16 pages of
       64 KiB on POWER (or 1 MiB). */

    if(FLMain)
    {
        FL_Write6(FLMain, SignalReceived, "Signal Received: %ld.  IP=%p  LR=%p  Data address=%p.  Code=%ld  Errno=%ld", sig,
#if __powerpc64__
        uc->uc_mcontext.gp_regs[PT_NIP],
        uc->uc_mcontext.gp_regs[PT_LNK],
#else
        0,0,
#endif
        (uint64_t)si->si_addr,si->si_code,si->si_errno);
    }
    snprintf(buffer, sizeof(buffer), "Signal Received: %d (%s)\n", sig, strsignal(sig));
    write(signalPipe[1], buffer, strlen(buffer));
    snprintf(buffer, sizeof(buffer), "Signal Received from pid=%d \n", si->si_pid);
    write(signalPipe[1], buffer, strlen(buffer));
    if(sig != SIGINT)
    {
        snprintf(buffer, sizeof(buffer), "*** BACKTRACE ***\n");
        write(signalPipe[1], buffer, strlen(buffer));
        traceSize = backtrace(traceBuf, sizeof(traceBuf)/sizeof(void*));
        backtrace_symbols_fd(traceBuf, traceSize, signalPipe[1]);
        snprintf(buffer, sizeof(buffer), "*****************\n");
        write(signalPipe[1], buffer, strlen(buffer));
    }
    if(FLMain)
    {
        FL_Write(FLMain, SignalPost, "Posting signal done",0,0,0,0);
    }
    sem_post(&block_until_exit_request);
    sleep(30);  // Give the main() thread time to exit properly
}

void* mainThread(void* ptr)
{
    int rc = -99;
#ifdef PROF_TIMING
    chrono::high_resolution_clock::time_point start = chrono::high_resolution_clock::now();
#endif
    try
    {
        #define SETRASIDS 1 
        #include "bbras.h"
        #undef SETRASIDS
        
        if (getuid())
        {

            throw runtime_error(string("Must be run under root authority"));
        }

        po::variables_map vm;
        po::options_description desc(NAME " allowed options");
        desc.add_options()
            ("help", "Display this help message")
            ("whoami", po::value<string>()->default_value(WHOAMI))
            ("instance", po::value<string>()->default_value("0"))
            ("config", po::value<string>()->default_value(DEFAULT_CONFIGFILE), "Path to configuration file")
            //  ("withoutssl", po::bool_switch()->default_value(false), "using clear channel instead of TLS encryption")
            ;

        po::store(po::parse_command_line(main_argc, main_argv, desc), vm);
        po::notify(vm);

        if (vm.count("help"))
        {
            cout << desc << endl;
            throw ExceptionBailout();
        }

        if(config.size() == 0)
        {
            if (!curConfig.load(vm["config"].as<string>()))
            {
                config = curConfig.getTree();
            }
            else
            {
                throw runtime_error(string("Error loading configurations"));
            }
        }

        config = curConfig.getTree();

        who = vm["whoami"].as<string>();
        ProcessId = who;
        string instance = vm["instance"].as<string>();
        initializeLogging(who + ".log", config);
        LOG(bb,always) << "Process Name      : " NAME;
        LOG(bb,always) << "Configuration file: " << curConfig.getPath();
        LOG(bb,always) << "Process identifier: " << who << " pid:"<<getpid();
        LOG(bb,always) << "Process instance  : " << instance;
        LOG(bb,always) << "Compiled :  "<<__TIME__<<" "<<__DATE__;
#ifdef BBSERVER
        LOG(bb,always) << "Metadata location : " << config.get("bb.bbserverMetadataPath", DEFAULT_BBSERVER_METADATAPATH);
#endif
        LOG(bb,always) << "Flightlog location: " << config.get(who + ".flightlog", NO_CONFIG_VALUE);

        rc = FL_CreateAll(config.get(who + ".flightlog", NO_CONFIG_VALUE).c_str());
        if(rc)
        {
            LOG(bb,error) << "Unable to initialize flightlog (" << config.get(who + ".flightlog", NO_CONFIG_VALUE) << ") rc=" << rc;
            throw runtime_error(string("Unable to initialize flightlog"));
        }

        rc=set_bb_nvmecliPath(config.get("bb.nvmecliPath", "/usr/sbin/nvme"));
#ifdef BBSERVER
        rc=set_bb_nvmfConnectPath(config.get("bb.nvmfConnectPath", "/opt/ibm/bb/scripts/nvmfConnect.sh"));
        extern void findSerials(void);
        findSerials();
#endif
        rc = setupNodeController(who);
        if(!rc)
        {
            char l_HostName[64] = {'\0'};
            ::gethostname(l_HostName, sizeof(l_HostName));
            string l_HostName2;
            activecontroller->gethostname(l_HostName2);
            LOG(bb,info) << "mainThread(): whoami=" << who << ", hostname=" << l_HostName << ", active controller hostname=" << l_HostName2;
        }
        else
        {
            LOG(bb,error) << "Unable to setup nodeControllers.  rc=" << rc;
            throw runtime_error(string("Unable to setup nodeControllers"));
        }

        rc = registerHandlers();
        if(rc)
        {
            LOG(bb,error) << "Unable to register handlers.  rc=" << rc;
            throw runtime_error(string("Unable to register handlers"));
        }

        // From here forward, we cannot return inline...  Must fall through to run bb_exit()...
        rc = setupConnections(who, instance);
        if(rc)
        {
            LOG(bb,error) << "Unable to setup connections.  rc=" << rc;
            throw runtime_error(string("Unable to setup connections"));
        }

        rc = bb_main(who);
        if(rc)
        {
            LOG(bb,error) << "Failure in bb_main.  rc=" << rc;
            throw runtime_error(string("Unable to run bb_main"));
        }
#ifdef PROF_TIMING

        chrono::high_resolution_clock::time_point stop = chrono::high_resolution_clock::now();
        std::chrono::duration<double,std::micro> elapsed_microseconds = stop - start;
        LOG(bb,trace) << "Initialization completed after: " << elapsed_microseconds.count() << " microseconds";
        //cout << chrono::duration_cast<chrono::microseconds>(stop - start).count() << "\n";
#endif

        FL_Write(FLMain, BBInited, "Burst Buffer initialization success",0,0,0,0);
    }
    catch (exception& e)
    {
        if(FLMain)
        {
            FL_Write(FLMain, BBInitFail, "Burst Buffer initialization failed.  rc=%ld",rc,0,0,0);
        }
        std::string errmsg = string("Error: ") + e.what() + string("\n");
        write(signalPipe[1], errmsg.c_str(), errmsg.size());
        sem_post(&block_until_exit_request);

        return NULL;
    }
    catch(ExceptionBailout& e)
    {
        sem_post(&block_until_exit_request);
        return NULL;
    }

    LOG(bb,always) << "BB initialization complete";
    return NULL;
}

int main(int argc, char** argv)
{
    int              rc = 0;
    ssize_t          signalBufferReadLen;
    struct sigaction sact;
    int              fcntl_rc;
    size_t           signalBufferSize;
    char*            signalBuffer;

    // Save argc/argv in globals for mainThread
    main_argc = argc;
    main_argv = argv;

    // Configure exit signal handlers
    pipe2(signalPipe, O_CLOEXEC | O_NONBLOCK);
    fcntl_rc = fcntl(signalPipe[0], F_GETPIPE_SZ);
    assert(fcntl_rc > 0);
    signalBufferSize = (size_t)fcntl_rc;
    signalBuffer = (char*)malloc(signalBufferSize+1);
    memset(signalBuffer,0,signalBufferSize+1);

    sigemptyset(&sact.sa_mask);
    sact.sa_sigaction = exitSignalHandler;
    sact.sa_flags = SA_SIGINFO | SA_RESETHAND;
    CONFIGURESIGNALS();

    sigemptyset(&sact.sa_mask);
    sact.sa_handler = SIG_IGN;
    sact.sa_flags = 0;
    SETSIGNAL(SIGPIPE);
    SETSIGNAL(SIGHUP);

    // Set signal handler parameters back to default.  (When exit_request is performed
    // again, CONFIGURESIGNALS will switch back to defaults)
    sigemptyset(&sact.sa_mask);
    sact.sa_handler = SIG_DFL;
    sact.sa_flags = 0;

    // Initialize exit semaphore
    sem_init(&block_until_exit_request,0,0);

    sigset_t   sigmaskpipe;
    sigemptyset(&sigmaskpipe);
    sigaddset(&sigmaskpipe, SIGPIPE);
    pthread_sigmask(SIG_BLOCK, &sigmaskpipe, NULL);

    // Spawn thread to handle main initialization.  Keep this thread for signal/error processing
    pthread_t maintid;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&maintid, &attr, mainThread, NULL);

    sigset_t   sigmask;
    sigemptyset(&sigmask);
    sigaddset(&sigmask, SIGINT);
    sigaddset(&sigmask, SIGHUP);
    sigaddset(&sigmask, SIGTERM);
    sigaddset(&sigmask, SIGSEGV);
    sigaddset(&sigmask, SIGBUS);
    sigaddset(&sigmask, SIGFPE);
    sigaddset(&sigmask, SIGILL);
    sigaddset(&sigmask, SIGABRT);
    pthread_sigmask(SIG_BLOCK, &sigmask, NULL);

    // Keep main thread from exiting and tearing down the process
    sem_wait(&block_until_exit_request);
    if(FLMain)
    {
        FL_Write(FLMain, ExitRequested, "Process exit has been requested",0,0,0,0);
    }
    CONFIGURESIGNALS();
    pthread_sigmask(SIG_UNBLOCK, &sigmask, NULL);

    // Read data from signal pipe.  Print each line to log.
    while((signalBufferReadLen = read(signalPipe[0], signalBuffer, signalBufferSize)) > 0)
    {
        signalBuffer[signalBufferReadLen-1] = 0; // ensure null terminated
        for(auto line : buildTokens(string(signalBuffer), "\n"))
        {
            LOG(bb,error) << line;
        }
    }

    // Free the signal buffer
    free(signalBuffer);

    // Call BB process-specific exit routine
    bb_exit(who);

    // SSL cleanup
    ERR_free_strings();
    EVP_cleanup();

    return rc;
}
