/*******************************************************************************
 |    MonitorHandler.cc
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


//! \file
//! \brief
// Includes
#include <vector>

#include <boost/lexical_cast.hpp>
#include <sys/types.h>
#include <sys/wait.h>

#include "Attribute.h"
#include "CnxSock.h"
#include "fshipmond_flightlog.h"
#include "logging.h"
#include "MonitorHandler.h"
#include "util.h"


/*
 * Static methods
 */

void MonitorHandler::signalHandler(int pSignal, siginfo_t* pSignalInfo, void* pContext) {
    int l_Status = 0;
    pid_t l_PID;

    l_PID = waitpid(-1, &l_Status, WNOHANG);
    if (l_PID == pSignalInfo->si_pid) {
        LOG(fshipmond,info) << "PID " << pSignalInfo->si_pid << " received signal number " << pSignalInfo->si_status << " (" << strsignal(pSignalInfo->si_status) << ")";
    } else {
        // Slightly strange...
        LOG(fshipmond,error) << "signalHandler invoked for PID " << pSignalInfo->si_pid << ", but waitpid() returned PID " << l_PID << " which received signal number " << l_Status << " (" << strsignal(l_Status) << ")";
    }

    return;
}


/*
 * Non-static methods
 */

int MonitorHandler::readInMessage(txp::ConnexPtr pCnxptr) {
    int l_RC = 0;
    txp::Msg * l_MsgPtr = 0;

    //  Read a message from an instance of fshipcld...
    l_RC = pCnxptr->read(l_MsgPtr);
    if (!l_RC) {

        //  Determine the message id and process the message...
        txp::Id l_Id = l_MsgPtr->getMsgId();
        switch (l_Id) {
            case txp::CORAL_HELLO_MONITOR:
            {
                l_RC = Hello(l_MsgPtr);
                break;
            }

            default:
            {
                LOG(fshipmond,error) << "Invalid message id of " << l_Id << " received.";
                l_RC = -1;
                break;
            }
        }
    }

    //  Cleanup for this message...
    if (l_MsgPtr) {
        txp::Msg * l_Temp = l_MsgPtr;
        l_MsgPtr = 0;
        delete l_Temp;
    }

    return l_RC;
}

void * MonitorHandler::run() {
    int l_RC = 0;

    /**  \TODO Finish signal handlers
     **/

    // Ignore SIGHUP
    signal(SIGHUP, SIG_IGN);

    // Set up the signal handler for SIGCHLD
    struct sigaction l_SignalHandler;
    memset(&l_SignalHandler, 0, sizeof(l_SignalHandler));
    l_SignalHandler.sa_sigaction = &MonitorHandler::signalHandler;
    l_SignalHandler.sa_flags = SA_SIGINFO;
    sigaction(SIGCHLD, &l_SignalHandler, NULL);

    txp::Connex* l_NewSock = 0;
    while (!l_RC) {
        l_RC = (_connectPtr)->listen4remote();
        if (!l_RC) {
            bool l_Continue = true;
            while (!l_RC && l_Continue) {
                l_RC = _connectPtr->accept(l_NewSock);
                if ((l_RC >= 0) && l_NewSock) {
                    l_RC = 0;
                    txp::ConnexPtr l_Cnxptr = l_NewSock;
                    if (l_Cnxptr->poll4DataIn()==1) {
                        l_RC = readInMessage(l_Cnxptr);
                        if (l_RC) {
                            if (errno != ENOTCONN) {
                                LOG(fshipmond,error) << "Error occurred during the processing of the last message, l_RC = " << l_RC << ", errno = " << errno << ".";
                            }
                        }
                    }
                    l_Continue = false;

                } else {
                    // Accept failed...
                    if (errno == EINTR) {
                        l_RC = 0;
                        continue;
                    }
                }

                if (l_NewSock) {
                    
                    delete l_NewSock;
                }
            }
            l_RC = 0;
        }
    }

    if (!l_RC) {
        LOG(fshipmond,info)<<"MonitorHandler::run(): Ending normally";
    } else {
        // \todo - Need to work on the error codes...  A errno string is created, but may not always apply...
        LOG(fshipmond,error)<<"MonitorHandler::run(): Ending with rc=" << l_RC << ", " << strerror(std::abs(l_RC));
    }

    signal(SIGCHLD, SIG_DFL);
    signal(SIGHUP, SIG_DFL);

    return NULL;
}

int MonitorHandler::Hello(txp::Msg* pMsg) {
    int l_RC = 0;

    pid_t l_PID = fork();
    switch (l_PID) {
        case 0:
        {
            // Child - exec fshipd
            LOG(fshipmond,info) << "Child process started for new instance of fshipd...";   

            std::vector<std::string> l_Args;
            l_Args.push_back("-r");
            std::string l_RmtAddress = (char*)((pMsg->retrieveAttr(txp::data))->getDataPtr());
            l_Args.push_back(l_RmtAddress);
            LOG(fshipmond,info) << "Return address for fshipcld: " << l_RmtAddress;

            l_Args.push_back("-c");
            txp::Attribute* l_configPathAttr = pMsg->retrieveAttr(txp::configPath); 
            char * configPath = NULL;
            if (l_configPathAttr) {
              configPath = (char*)(l_configPathAttr->getDataPtr());
              l_Args.push_back(configPath);
            }
            else l_Args.push_back(curConfig.getPath());
            LOG(fshipmond,info) << "Configuration file: " << curConfig.getPath();
         
            l_Args.push_back("-m");
            std::string mode = (char*)((pMsg->retrieveAttr(txp::mode))->getDataPtr());
            LOG(fshipmond,info) << "fshipcld mode=" << mode;
            l_Args.push_back(mode);

            size_t l_NumArgs = l_Args.size();
            std::string fshipdexec = config.get("fship.server.bin", "./work/bin/fshipd");
            const char* l_Pgm = fshipdexec.c_str();
            const char** l_Argv = new const char*[l_NumArgs+2];
            l_Argv[0] = l_Pgm;
            LOG(fshipmond,info) << "fshipd executable=" << fshipdexec;
            for (size_t i=0; i<l_NumArgs; ++i)
            {
                LOG(fshipmond,info) << "fshipd argv[" << i << "]=" << l_Args[i];
                l_Argv[i+1] = l_Args[i].c_str();
            }
            l_Argv[l_NumArgs+1] = NULL;

            l_RC = execv(l_Pgm, (char**)l_Argv);
            if (l_RC >= 0) {
                LOG(fshipmond,info) << "Instance of fshipd started...";
            } else {
                LOG(fshipmond,error) << "execv of fshipd failed, l_RC = " << l_RC;
                l_RC = -1;
            }

            break;
        }

        case -1:
        {
            // Unsuccessful fork
            // \todo - What to do?  @DLH
            l_RC = -1;
            LOGERRNO(fshipmond,error,errno);

            break;
        }

        default:
        {
            // Parent - Just return...
            // NOTE:  The response message will be built/sent by the instance of fshipd
            //        that was fork and execvp'ed above...

            break;
        }
    }

    return l_RC;
};
