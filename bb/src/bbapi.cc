/*******************************************************************************
 |    bbapi.cc
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

#include <stdio.h>
#include <unistd.h>
#include <iostream>
#include <string>
#include <execinfo.h>

#include <boost/program_options.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/filesystem.hpp>
namespace bfs = boost::filesystem;

#define NORASEXTERN
#include "bbras.h"

// NOTE:  Do not put "connections.h" *AFTER*
//        "using namespace std".
//        Logging failures will occur if you do...
#include "connections.h"

using namespace std;
#ifdef PROF_TIMING
#include <ctime>
#include <chrono>
#endif


#include "bbapi_flightlog.h"
#include "bbapi.h"
#include "bbapiAdmin.h"
#include "bbapi2.h"
#include "bbinternal.h"
#include "BBTransferDef.h"
#include "logging.h"

#define NAME "bbAPI"

// External data
string ProcessId = DEFAULT_PROXY_NAME;

#ifdef TXP_DEVELOPMENT
txp::Log bbapi_log(txp::Log::DEFAULT_LOG_DESTINATION, txp::Log::DEFAULT_OPEN_LOGSTATE, txp::Log::DEBUG_LOGLEVEL);
#endif

typedef BBTransferDef_t* BBTransferDef_tPTR;
typedef BBTransferDef * BBTransferDefPTR;
typedef std::pair<BBTransferDef_tPTR,BBTransferDefPTR> BBTransferDefPair_t;
typedef std::map<BBTransferDef_tPTR,BBTransferDefPTR> mapBBTransferDef_t;
typedef mapBBTransferDef_t::iterator mapBBTransferDefIterator_t;
typedef const mapBBTransferDef_t::iterator mapBBTransferDefIteratorConst_t;
static mapBBTransferDef_t mapBBtransferDef;

static volatile bool CRuntimeInitialized = false;

BBTransferDefPTR getBBTransferDefPTR(BBTransferDef_tPTR pBBTransferDef_tPTR)
{
//need to lock on the map
    BBTransferDefPTR returnValue=NULL;
    mapBBTransferDefIteratorConst_t it = mapBBtransferDef.find(pBBTransferDef_tPTR);
    if (it != mapBBtransferDef.end()){
       returnValue=it->second;
    }
//need to unlock a lock on the map
    return returnValue;
}

BBTransferDefPTR removeBBTransferDefPTR(BBTransferDef_tPTR pBBTransferDef_tPTR)
{
//need to lock on the map
    BBTransferDefPTR returnValue=NULL;
    mapBBTransferDefIteratorConst_t it = mapBBtransferDef.find(pBBTransferDef_tPTR);
    if (it != mapBBtransferDef.end()){
       returnValue=it->second;
       mapBBtransferDef.erase(it);
    }
//need to unlock a lock on the map
    return returnValue;
}

BBTransferDef_tPTR addBBTransferDefPTR(BBTransferDefPTR pBBTransferDefPTR)
{
    BBTransferDef_tPTR returnValue=(BBTransferDef_tPTR)pBBTransferDefPTR;
    if (!returnValue) return returnValue;
    //need to lock on the map
    mapBBtransferDef.insert( BBTransferDefPair_t(returnValue,pBBTransferDefPTR) );
    //need to unlock a lock on the map
    return returnValue;
}


/*******************************************************************************
 | External methods
 *******************************************************************************/

// NOTE: If this method is invoked with an already allocated buffer (i.e., the pointer
//       to the pointer is not NULL), this method will call delete on that allocated
//       memory.  This may or may not be desirable...  User beware...
void getLastErrorDetails(BBERRORFORMAT pFormat, char** pBuffer)
{
    int rc;
    size_t l_NumBytesAvailable;
    if(pBuffer)
    {
        rc = BB_GetLastErrorDetails(pFormat, &l_NumBytesAvailable, 0, NULL);
        if(rc == 0)
        {
            *pBuffer = (char*)malloc(l_NumBytesAvailable+1);
            BB_GetLastErrorDetails(pFormat, NULL, l_NumBytesAvailable, *pBuffer);
        }
        else
        {
            *pBuffer = NULL;
        }
    }
}

void verifyInit(const bool pExpectedValue)
{
    int rc;
    stringstream errorText;


    if (connectionExists(ProcessId))
    {
        if (!pExpectedValue)
        {
            rc = EBUSY;     //exists--already initialized
            errorText << "BB_InitLibrary() must not be called multiple times";
            LOG_ERROR_TEXT_ERRNO_AND_BAIL(errorText, rc);
        }
    }
    else
    {
        if (pExpectedValue)
        {
            rc = ENODEV;
            errorText << "BB_InitLibrary() must be called before any other burst buffer APIs";
            LOG_ERROR_TEXT_ERRNO_AND_BAIL(errorText, rc);
        }
    }

    return;
}

// NOTE:  Not currently invoked...
int validateContribId(const uint32_t pContribId)
{
    int rc = 0;

    switch (pContribId)
    {
        case UNDEFINED_CONTRIBID:
            rc = EINVAL;
            break;
        default:
            break;
    }

    return rc;
}


/*******************************************************************************
 | APIs for Library Initialization and Termination
 *******************************************************************************/

volatile static int initLibraryDone=0;
static pthread_mutex_t  lockLibraryDone = PTHREAD_MUTEX_INITIALIZER;


int BB_InitLibrary(uint32_t contribId, const char* clientVersion)
{
    int rc = 0;
    bool l_PerformCleanup = true;
    stringstream errorText;

    if(CRuntimeInitialized == false)
    {
        bool   earlybailout = false;
        void*  traceBuf[256];
        int    traceSize;
        traceSize = backtrace(traceBuf, sizeof(traceBuf)/sizeof(void*));
        char** syms = backtrace_symbols(traceBuf, traceSize);
        if(strncmp(syms[traceSize-1], "/lib64/ld64", 11) == 0)
            earlybailout = true;

        free(syms);
        if(earlybailout)   // glibc has not fully executed constructor list.  C++ runtime may not be fully functional.
            return -1;

        CRuntimeInitialized = true;
    }

    pthread_mutex_lock(&lockLibraryDone);
    if (!initLibraryDone)
    {
      try
      {
        // Verify initialization has not yet been invoked
        rc = EBUSY; //exists if initialization has already been invoked
        verifyInit(false); //throws bailout if an access error
        rc=0;

        if(!curConfig.isLoaded())
        {
            bberror << err("env.configfile", DEFAULT_CONFIGFILE);
            if (!curConfig.load(DEFAULT_CONFIGFILE))
            {
                config = curConfig.getTree();
            }
            else
            {
                rc = ENOENT;
                errorText << "Error loading configuration from " << DEFAULT_CONFIGFILE;
                cerr << errorText << endl;
                LOG_ERROR_TEXT_ERRNO_AND_BAIL(errorText, rc);
            }
        }

        if (!clientVersion || !clientVersion[0])
        {
            rc = EINVAL;
            errorText << "Parameter clientVersion is NULL or NULL string";
            LOG_ERROR_TEXT_ERRNO_AND_BAIL(errorText, rc);
        }

        rc = BB_InitLibrary2(contribId, clientVersion, NULL, l_PerformCleanup);

        if (rc) bberror << bailout;
        initLibraryDone=1;
      }
      catch(ExceptionBailout& e) { l_PerformCleanup = true; }
      catch(exception& e)
      {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
        l_PerformCleanup = true;
      }
    }
    else rc=EALREADY;

    if (l_PerformCleanup)
    {
        cleanupInit();
    }
    pthread_mutex_unlock(&lockLibraryDone);

    return rc;
}

int BB_TerminateLibrary()
{
    /// \todo graceful connection close?
    int rc = 0;
    if(CRuntimeInitialized == false)
    {
        return -1;
    }

    pthread_mutex_lock(&lockLibraryDone);
    if (initLibraryDone)
    {
      try
      {
  	// Verify initialization
	rc = ENODEV;
	verifyInit(true);
        cleanupAllConnections();
	rc = 0;
        initLibraryDone=0;
      }
      catch(ExceptionBailout& e) { }
      catch(exception& e)
      {
	rc = -1;
	LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
      }
    }
    else rc=EALREADY;

    pthread_mutex_unlock(&lockLibraryDone);
    return rc;
}

int BB_GetLastErrorDetails(BBERRORFORMAT format, size_t* numBytesAvail, size_t buffersize, char* bufferForErrorDetails)
{
#ifdef PROF_TIMING
    chrono::high_resolution_clock::time_point time_start = chrono::high_resolution_clock::now();
#endif
    int rc = 0;

    try
    {
        std::string errtext(" ");
        if(format == BBERRORJSON)
        {
            errtext = bberror.get("json");
        }
        else if(format == BBERRORXML)
        {
            errtext = bberror.get("xml");
        }
        else //(format == BBERRORFLAT)
        {
            errtext = bberror.get("pretty");
        }

        if(numBytesAvail)
        {
            *numBytesAvail = errtext.size()+1;
        }

        if (buffersize && bufferForErrorDetails)
        {
            strCpy(bufferForErrorDetails, errtext.c_str(), buffersize);
        }
    }
    catch(ExceptionBailout& e) { }
    catch(exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

#ifdef PROF_TIMING
    chrono::high_resolution_clock::time_point time_stop = chrono::high_resolution_clock::now();
    std::chrono::duration<double,std::micro> elapsed_microseconds = time_stop - time_start;
    LOG(bb,trace) << __FILE__ <<":"<< __LINE__ << " " << __FUNCTION__ << "() completed after: " << elapsed_microseconds.count() << " microseconds";
#endif
    return rc;
}

int BB_GetVersion(size_t size, char* APIVersion)
{
    int rc = 0;
    stringstream errorText;

    // NOTE: Getting the version is valid whether init has,
    //       or has not, been invoked...
    try
    {
        if (!APIVersion)
        {
            rc = EINVAL;
            errorText << "Null parameter APIVersion";
            LOG_ERROR_TEXT_ERRNO_AND_BAIL(errorText, rc);
        }

        size_t l_versionStringLength = strlen(BBAPI_CLIENTVERSIONSTR);
        if (size < l_versionStringLength + 1)
        {
            rc = ENOSPC;
            errorText << "APIVersion needs more space";
            bberror << err("error.size_needed", l_versionStringLength+1) << err("API.version", BBAPI_CLIENTVERSIONSTR);
            LOG_ERROR_TEXT_ERRNO_AND_BAIL(errorText, rc);
        }

        int retvalue = snprintf(APIVersion, size, BBAPI_CLIENTVERSIONSTR);
        if (retvalue < 0)
        {
          rc = EBFONT;
          errorText << "snprintf output error";
          bberror << err("error.retvalue", sizeof(BBAPI_CLIENTVERSIONSTR)-1);
          LOG_ERROR_TEXT_ERRNO_AND_BAIL(errorText, rc);
        }
    }
    catch(ExceptionBailout& e) { }
    catch(exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }
    return rc;
}


/*******************************************************************************
 | APIs for Transfer Definition Manipulation
 *******************************************************************************/

int BB_CreateTransferDef(BBTransferDef_t** transfer)
{
#ifdef PROF_TIMING
    chrono::high_resolution_clock::time_point time_start = chrono::high_resolution_clock::now();
#endif
    int rc = 0;
    stringstream errorText;

    bberror.clear();

    try
    {
        // Verify initialization
        rc = ENODEV;
        verifyInit(true);
        rc = 0;

        if (!transfer)
        {
            rc = EINVAL;
            errorText << "Null parameter transfer";
            LOG_ERROR_TEXT_ERRNO_AND_BAIL(errorText, rc);
        }

        BBTransferDefPTR  l_BBTransferDefPTR = getBBTransferDefPTR(*transfer);
        if (l_BBTransferDefPTR )
        {
            rc = EADDRINUSE;
            errorText << "Specified TransferDef pointer has already been created and was not freed";
            LOG_ERROR_TEXT_ERRNO_AND_BAIL(errorText, rc);
        }

        l_BBTransferDefPTR = new BBTransferDef;
        if(!l_BBTransferDefPTR)
        {
            rc = ENOMEM;
            *transfer=NULL;
            errorText << "Unable to allocate memory";
            LOG_ERROR_TEXT_ERRNO_AND_BAIL(errorText, rc);
        }
        else
        {
            *transfer = addBBTransferDefPTR(l_BBTransferDefPTR );
            assert(*transfer!=NULL);
        }
    }
    catch(ExceptionBailout& e) { }
    catch(exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

#ifdef PROF_TIMING
    chrono::high_resolution_clock::time_point time_stop = chrono::high_resolution_clock::now();
    std::chrono::duration<double,std::micro> elapsed_microseconds = time_stop - time_start;
    LOG(bb,trace) << __FILE__ <<":"<< __LINE__ << " " << __FUNCTION__ << "() completed after: " << elapsed_microseconds.count() << " microseconds";
#endif
    return rc;
}

int BB_FreeTransferDef(BBTransferDef_t* transfer)
{
#ifdef PROF_TIMING
    chrono::high_resolution_clock::time_point time_start = chrono::high_resolution_clock::now();
#endif
    int rc = 0;
    stringstream errorText;

    bberror.clear();

    try
    {
        // Verify initialization
        rc = ENODEV;
        verifyInit(true);
        rc = 0;

       if (!transfer)
       {
            rc = EINVAL;
            errorText << "Invalid null parameter transfer";
//            cerr << errorText << endl;
            LOG_ERROR_TEXT_ERRNO_AND_BAIL(errorText, rc);
        }

        BBTransferDefPTR  ptr = removeBBTransferDefPTR(transfer);
        if (!ptr)
        {
            rc = EADDRNOTAVAIL;
            errorText << "Invalid parameter transfer";
//            cerr << errorText << endl;
            LOG_ERROR_TEXT_ERRNO_AND_BAIL(errorText, rc);
        }
        delete ptr;
    }
    catch(ExceptionBailout& e) { }
    catch(exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

#ifdef PROF_TIMING
    chrono::high_resolution_clock::time_point time_stop = chrono::high_resolution_clock::now();
    std::chrono::duration<double,std::micro> elapsed_microseconds = time_stop - time_start;
    LOG(bb,trace) << __FILE__ <<":"<< __LINE__ << " " << __FUNCTION__ << "() completed after: " << elapsed_microseconds.count() << " microseconds";
#endif
    return rc;
}

int BB_AddFiles(BBTransferDef_t* transfer, const char* source, const char* target, BBFILEFLAGS flags)
{
#ifdef PROF_TIMING
    chrono::high_resolution_clock::time_point time_start = chrono::high_resolution_clock::now();
#endif
    int rc = 0;
    stringstream errorText;

    bberror.clear();

    Extent newextent;

    try
    {
        // Verify initialization
        rc = ENODEV;
        verifyInit(true);
        rc = 0;

        if (!transfer)
        {
            rc = EINVAL;
            errorText << "Invalid null parameter transfer";
//            cerr << errorText << endl;
            LOG_ERROR_TEXT_ERRNO_AND_BAIL(errorText, rc);
        }

        if (!source)
        {
            rc = EINVAL;
            errorText << "Invalid null parameter source";
//            cerr << errorText << endl;
            LOG_ERROR_TEXT_ERRNO_AND_BAIL(errorText, rc);
        }
        else if (!source[0])
        {
            rc = EINVAL;
            errorText << "Parameter source is NULL string";
//            cerr << errorText << endl;
            LOG_ERROR_TEXT_ERRNO_AND_BAIL(errorText, rc);
        }

        if (!target)
        {
            rc = EINVAL;
            errorText << "Invalid null parameter target";
//            cerr << errorText << endl;
            LOG_ERROR_TEXT_ERRNO_AND_BAIL(errorText, rc);
        }
        else if (!target[0])
        {
            rc = EINVAL;
            errorText << "Parameter target is NULL string";
//            cerr << errorText << endl;
            LOG_ERROR_TEXT_ERRNO_AND_BAIL(errorText, rc);
        }
        uint64_t l_Flags = (uint64_t)flags;
        if (l_Flags & 0xFFFFFFFFFFFF0022)
        {
            rc = -1;
            errorText << "Invalid parameter value 0x" << hex << uppercase << setfill('0') << l_Flags << setfill(' ') << nouppercase << dec \
                      << " for source file " << source << ", target file " << target;
//            cerr << errorText << endl;
            LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
        }

        if (BBTransferTypeFromFlags(l_Flags) == BBTransferTypeRegular)
        {
            // 'Regular file'...
            // Ensure that the bundle id and the associated 'BSCFS' bits are zero...
            // NOTE: The two high order bits in the low-order nibble are used in test
            //       to force 'stage-in' or 'stage-out' if both files are local...
            if (l_Flags & 0xFFFFFFFFFFFFFFF2)
            {
                rc = -1;
                errorText << "Invalid parameter value 0x" << hex << uppercase << setfill('0') << l_Flags << setfill(' ') << nouppercase << dec \
                          << " for normal source file " << source << ", target file " << target;
//                cerr << errorText << endl;
                LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
            }
        }

        bfs::path spath = bfs::system_complete(bfs::path(source));
        bfs::path tpath = bfs::system_complete(bfs::path(target));

        if(bfs::is_regular_file(spath) && bfs::is_directory(tpath))
        {
            tpath /= spath.filename();
        }

        if(!bfs::exists(tpath.parent_path()))
        {
            rc = ENOENT;
            errorText << "Target directory '" << tpath.parent_path() << "' does not exist";
            LOG_ERROR_TEXT_ERRNO_AND_BAIL(errorText, rc);
        }

        BBTransferDefPTR  ptr = getBBTransferDefPTR(transfer);
        if (!ptr)
        {
            rc = EADDRNOTAVAIL;
            errorText << "Invalid parameter transfer";
//            cerr << errorText << endl;
            LOG_ERROR_TEXT_ERRNO_AND_BAIL(errorText, rc);
        }
        ptr->lock();

        if(((flags & BBRecursive) != 0) && (bfs::is_directory(spath)))
        {
            for(auto& d : bfs::recursive_directory_iterator(spath))
            {
                if(bfs::is_regular_file(d))
                {
                    string src = d.path().string();
                    string dst = tpath.string() + "/" + d.path().string().substr(spath.parent_path().string().length());
                    if (ptr->checkOneCPSourceToDest(src, dst) ){
                        rc=EALREADY;
                        errorText << "Another source file is already destined for copying to target";
                        bberror << err("error.source", src)<<err("error.target",dst);
                        LOG_ERROR_TEXT_ERRNO_AND_BAIL(errorText, rc);
                    }
                    ptr->files.push_back(src);
                    newextent.sourceindex = ptr->files.size()-1;
                    ptr->files.push_back(dst);
                    newextent.targetindex = ptr->files.size()-1;
                    newextent.flags = l_Flags;
                    ptr->extents.push_back(newextent);
                }
            }
        }
        else
        {
            // Source must be a file
            if((!bfs::is_regular_file(spath)) && (spath != "/dev/zero"))
            {
                rc = EINVAL;
                errorText << "Value passed for source is not a file";
                bberror << err("error.value", source);
                LOG_ERROR_TEXT_ERRNO_AND_BAIL(errorText, rc);
            }
            if (ptr->checkOneCPSourceToDest(spath.string(), tpath.string()) ){
                rc=EALREADY;
                errorText << "Another source file is already destined for copying to target";
                bberror << err("error.source", spath.string())<<err("error.target",tpath.string());
                LOG_ERROR_TEXT_ERRNO_AND_BAIL(errorText, rc);
            }
            ptr->files.push_back(spath.string());
            newextent.sourceindex = ptr->files.size()-1;
            ptr->files.push_back(tpath.string());
            newextent.targetindex = ptr->files.size()-1;
            newextent.flags = l_Flags;
            ptr->extents.push_back(newextent);
        }
        ptr->unlock();
    }

    catch(ExceptionBailout& e) { }
    catch(exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

#ifdef PROF_TIMING
    chrono::high_resolution_clock::time_point time_stop = chrono::high_resolution_clock::now();
    std::chrono::duration<double,std::micro> elapsed_microseconds = time_stop - time_start;
    LOG(bb,trace) << __FILE__ <<":"<< __LINE__ << " " << __FUNCTION__ << "() completed after: " << elapsed_microseconds.count() << " microseconds";
#endif
    return rc;
}

int BB_AddKeys(BBTransferDef_t* transfer, const char* key, const char* value)
{
#ifdef PROF_TIMING
    chrono::high_resolution_clock::time_point time_start = chrono::high_resolution_clock::now();
#endif
    int rc = 0;
    stringstream errorText;

    bberror.clear();

    try
    {
        // Verify initialization
        rc = ENODEV;
        verifyInit(true);
        rc = 0;

        if (!transfer)
        {
            rc = EINVAL;
            errorText << "Invalid null parameter transfer";
//            cerr << errorText << endl;
            LOG_ERROR_TEXT_ERRNO_AND_BAIL(errorText, rc);
        }


        if (!key || !key[0])
        {
            rc = EINVAL;
            errorText << "Invalid null parameter key";
//            cerr << errorText << endl;
            LOG_ERROR_TEXT_ERRNO_AND_BAIL(errorText, rc);
        }

        if (!value || !value[0])
        {
            rc = EINVAL;
            errorText << "Invalid null parameter value";
//            cerr << errorText << endl;
            LOG_ERROR_TEXT_ERRNO_AND_BAIL(errorText, rc);
        }

        BBTransferDefPTR  ptr = getBBTransferDefPTR(transfer);
        if (!ptr)
        {
            rc = EADDRNOTAVAIL;
            errorText << "Transfer parameter is invalid";
//            cerr << errorText << endl;
            LOG_ERROR_TEXT_ERRNO_AND_BAIL(errorText, rc);
        }

        ptr->lock();

        {
            ptr->keyvalues[key] = value;
        }

        ptr->unlock();

        LOG(bb,debug) << "addkeys: " << key << " = " << value;
    }
    catch(ExceptionBailout& e) { }
    catch(exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

#ifdef PROF_TIMING
    chrono::high_resolution_clock::time_point time_stop = chrono::high_resolution_clock::now();
    std::chrono::duration<double,std::micro> elapsed_microseconds = time_stop - time_start;
    LOG(bb,trace) << __FILE__ <<":"<< __LINE__ << " " << __FUNCTION__ << "() completed after: " << elapsed_microseconds.count() << " microseconds";
#endif
    return rc;
}


/*******************************************************************************
 | Operations for bbServer
 *******************************************************************************/

int BB_CancelTransfer(BBTransferHandle_t pHandle, BBCANCELSCOPE pScope)
{
#ifdef PROF_TIMING
    chrono::high_resolution_clock::time_point time_start = chrono::high_resolution_clock::now();
#endif
    int rc = 0;
    stringstream errorText;

    bberror.clear();

    ResponseDescriptor reply;
    txp::Msg* msg = 0;

    try
    {
        // Verify initialization
        rc = ENODEV;
        verifyInit(true);
        rc = 0;
        stringstream errorText;

        if (!pHandle)
        {
            rc = EINVAL;
            errorText << "Invalid null parameter handle";
//            cerr << errorText << endl;
            LOG_ERROR_TEXT_ERRNO_AND_BAIL(errorText, rc);
        }

        if (check_cancel_scope(pScope))
        {
            rc = EINVAL;
            errorText << "Cancel scope option 0x" << hex << uppercase << setfill('0') << pScope << setfill(' ') << nouppercase << dec << " is invalid";
//            cerr << errorText << endl;
            LOG_ERROR_TEXT_ERRNO_AND_BAIL(errorText, rc);
        }

        txp::Msg::buildMsg(txp::BB_CANCELTRANSFER, msg);
        msg->addAttribute(txp::handle, (uint64_t)pHandle);
        msg->addAttribute(txp::flags, (uint64_t)pScope);

        rc = sendMessage(ProcessId, msg, reply);
        delete msg;
        if (rc) LOG_RC_AND_BAIL(rc);

        rc = waitReply(reply, msg);
        if (rc) LOG_RC_AND_BAIL(rc);

        rc = bberror.merge(msg);

        delete msg;
    }
    catch(ExceptionBailout& e) { }
    catch(exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

#ifdef PROF_TIMING
    chrono::high_resolution_clock::time_point time_stop = chrono::high_resolution_clock::now();
    std::chrono::duration<double,std::micro> elapsed_microseconds = time_stop - time_start;
    LOG(bb,trace) << __FILE__ <<":"<< __LINE__ << " " << __FUNCTION__ << "() completed after: " << elapsed_microseconds.count() << " microseconds";
#endif
    return rc;
}

int BB_GetThrottleRate(const char* mountpoint, uint64_t* rate)
{
#ifdef PROF_TIMING
    chrono::high_resolution_clock::time_point time_start = chrono::high_resolution_clock::now();
#endif

    int rc = 0;
    stringstream errorText;

    bberror.clear();

    ResponseDescriptor reply;
    txp::Msg* msg = 0;

    try
    {
        // Verify initialization
        rc = ENODEV;
        verifyInit(true);
        rc = 0;

        if (!rate)
        {
            rc = EINVAL;
            errorText << "Invalid parameter rate";
//            cerr << errorText << endl;
            LOG_ERROR_TEXT_ERRNO_AND_BAIL(errorText, rc);
        }

        if ( (!mountpoint)|| (!mountpoint[0]) )
        {
            rc = EINVAL;
            errorText << "Parameter mountpoint is NULL pointer or NULL string";
//            cerr << errorText << endl;
            LOG_ERROR_TEXT_ERRNO_AND_BAIL(errorText, rc);
        }

        txp::Msg::buildMsg(txp::BB_GETTHROTTLERATE, msg);
        msg->addAttribute(txp::mountpoint, mountpoint, strlen(mountpoint)+1);

        rc = sendMessage(ProcessId, msg, reply);
        delete msg;
        if (rc) LOG_RC_AND_BAIL(rc);

        rc = waitReply(reply, msg);
        if (rc) LOG_RC_AND_BAIL(rc);

        rc = bberror.merge(msg);
        if (!rc)
        {
            // NOTE:  Only return a rate if the rc from bbServer is zero...
            *rate = ((txp::Attr_uint64*)msg->retrieveAttrs()->at(txp::rate))->getData();
        }
        delete msg;
    }
    catch(ExceptionBailout& e) { }
    catch(exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

#ifdef PROF_TIMING
    chrono::high_resolution_clock::time_point time_stop = chrono::high_resolution_clock::now();
    std::chrono::duration<double,std::micro> elapsed_microseconds = time_stop - time_start;
    LOG(bb,trace) << __FILE__ <<":"<< __LINE__ << " " << __FUNCTION__ << "() completed after: " << elapsed_microseconds.count() << " microseconds";
#endif
    return rc;
}

int BB_GetTransferHandle(BBTAG pTag, uint64_t pNumContrib, uint32_t pContrib[], BBTransferHandle_t* pHandle)
{
#ifdef PROF_TIMING
    chrono::high_resolution_clock::time_point time_start = chrono::high_resolution_clock::now();
#endif

    int rc = 0;
    stringstream errorText;

    bberror.clear();

    ResponseDescriptor reply;
    txp::Msg* msg = 0;

    try
    {
        if (!pHandle)
        {
            rc = EINVAL;
            errorText << "Invalid null parameter handle";
//            cerr << errorText << endl;
            LOG_ERROR_TEXT_ERRNO_AND_BAIL(errorText, rc);
        }
        // Initialize return handle
        *pHandle = 0;

        // Verify initialization
        rc = ENODEV;
        verifyInit(true);
        rc = 0;

        // Build the message to be sent to bbproxy...
        txp::Msg::buildMsg(txp::BB_GETTRANSFERHANDLE, msg);

        msg->addAttribute(txp::tag, pTag);

        uint64_t l_NumContrib = pNumContrib;
        if ((!l_NumContrib) || pContrib == NULL)
        {
            l_NumContrib = 0;
        }
        msg->addAttribute(txp::numcontrib, l_NumContrib);
        if (l_NumContrib && pContrib)
        {
            msg->addAttribute(txp::contrib, (const char*)pContrib, sizeof(uint32_t) * pNumContrib);
        }

        rc = sendMessage(ProcessId, msg, reply);
        delete msg;
        if (rc) LOG_RC_AND_BAIL(rc);

        rc = waitReply(reply, msg);
        if (rc) LOG_RC_AND_BAIL(rc);

        rc = bberror.merge(msg);
        if (!rc)
        {
            // NOTE:  Only return a handle if the rc from bbServer is zero...
            *pHandle = ((txp::Attr_uint64*)msg->retrieveAttrs()->at(txp::transferHandle))->getData();
        }
        delete msg;
    }
    catch(ExceptionBailout& e) { }
    catch(exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

#ifdef PROF_TIMING
    chrono::high_resolution_clock::time_point time_stop = chrono::high_resolution_clock::now();
    std::chrono::duration<double,std::micro> elapsed_microseconds = time_stop - time_start;
    LOG(bb,trace) << __FILE__ <<":"<< __LINE__ << " " << __FUNCTION__ << "() completed after: " << elapsed_microseconds.count() << " microseconds";
#endif
    return rc;
}

int BB_GetTransferKeys(BBTransferHandle_t handle, size_t buffersize, char* bufferForKeyData)
{
#ifdef PROF_TIMING
    chrono::high_resolution_clock::time_point time_start = chrono::high_resolution_clock::now();
#endif

    int rc = 0;
    stringstream errorText;

    bberror.clear();

    ResponseDescriptor reply;
    txp::Msg* msg = 0;

    try
    {
        // Verify initialization
        rc = ENODEV;
        verifyInit(true);
        rc = 0;

        if (!handle)
        {
            rc = EINVAL;
            errorText << "Invalid null parameter handle";
//            cerr << errorText << endl;
            LOG_ERROR_TEXT_ERRNO_AND_BAIL(errorText, rc);
        }

        if (!bufferForKeyData)
        {
            rc = EINVAL;
            errorText << "Invalid null parameter buffer for key data";
//            cerr << errorText << endl;
            LOG_ERROR_TEXT_ERRNO_AND_BAIL(errorText, rc);
        }

        txp::Msg::buildMsg(txp::BB_GETTRANSFERKEYS, msg);
        msg->addAttribute(txp::handle, (uint64_t)handle);
        msg->addAttribute(txp::buffersize, (uint64_t)buffersize);

        rc = sendMessage(ProcessId, msg, reply);
        delete msg;
        if (rc) LOG_RC_AND_BAIL(rc);

        rc = waitReply(reply, msg);
        if (rc) LOG_RC_AND_BAIL(rc);

        rc = bberror.merge(msg);
        if (!rc)
        {
            // NOTE:  Only return information if the rc from bbServer is zero...
            uint64_t l_ActualSize = ((txp::Attr_uint64*)msg->retrieveAttrs()->at(txp::buffersize))->getData();
            if (l_ActualSize)
            {
                // Key data exists
                if (buffersize >= l_ActualSize+1)
                {
                    // Copy the key data
                    strCpy(bufferForKeyData, (char*)msg->retrieveAttrs()->at(txp::buffer)->getDataPtr(), l_ActualSize);
                }
                else
                {
                    // Not enough room in the buffer
                    rc = -2;
                }
            }
            else
            {
                // No key data exists.  Zero the first byte in the buffer...
                memset(bufferForKeyData, 0, 1);
            }
        }
        delete msg;
    }
    catch(ExceptionBailout& e) { }
    catch(exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

#ifdef PROF_TIMING
    chrono::high_resolution_clock::time_point time_stop = chrono::high_resolution_clock::now();
    std::chrono::duration<double,std::micro> elapsed_microseconds = time_stop - time_start;
    LOG(bb,trace) << __FILE__ <<":"<< __LINE__ << " " << __FUNCTION__ << "() completed after: " << elapsed_microseconds.count() << " microseconds";
#endif
    return rc;
}

int BB_SetThrottleRate(const char* mountpoint, uint64_t rate)
{
#ifdef PROF_TIMING
    chrono::high_resolution_clock::time_point time_start = chrono::high_resolution_clock::now();
#endif

    int rc = 0;
    stringstream errorText;

    bberror.clear();

    ResponseDescriptor reply;
    txp::Msg* msg = 0;

    try
    {
        // Verify initialization
        rc = ENODEV;
        verifyInit(true);
        rc = 0;

        if ( (!mountpoint)|| (!mountpoint[0]) )
        {
            rc = EINVAL;
            errorText << "Invalid null parameter mountpoint";
//            cerr << errorText << endl;
            LOG_ERROR_TEXT_ERRNO_AND_BAIL(errorText, rc);
        }

        txp::Msg::buildMsg(txp::BB_SETTHROTTLERATE, msg);
        msg->addAttribute(txp::mountpoint, mountpoint, strlen(mountpoint)+1);
        msg->addAttribute(txp::rate, rate);

        rc = sendMessage(ProcessId, msg, reply);
        delete msg;
        if (rc) LOG_RC_AND_BAIL(rc);

        rc = waitReply(reply, msg);
        if (rc) LOG_RC_AND_BAIL(rc);

        rc = bberror.merge(msg);
        delete msg;
    }
    catch(ExceptionBailout& e) { }
    catch(exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

#ifdef PROF_TIMING
    chrono::high_resolution_clock::time_point time_stop = chrono::high_resolution_clock::now();
    std::chrono::duration<double,std::micro> elapsed_microseconds = time_stop - time_start;
    LOG(bb,trace) << __FILE__ <<":"<< __LINE__ << " " << __FUNCTION__ << "() completed after: " << elapsed_microseconds.count() << " microseconds";
#endif
    return rc;
}

int BB_StartTransfer(BBTransferDef_t* pTransfer, BBTransferHandle_t pHandle)
{
#ifdef PROF_TIMING
    chrono::high_resolution_clock::time_point time_start = chrono::high_resolution_clock::now();
#endif

    int rc = 0;
    stringstream errorText;

    bberror.clear();

    ResponseDescriptor reply;
    txp::Msg* msg = 0;

    vector<txp::CharArray> freelist;  ///< Deallocates any marshalled CharArrays when function exits

    try
    {
        // Verify initialization
        rc = ENODEV;
        verifyInit(true);
        rc = 0;

        if (!pTransfer)
        {
            rc = EINVAL;
            errorText << "Invalid null parameter transfer";
//            cerr << errorText << endl;
            LOG_ERROR_TEXT_ERRNO_AND_BAIL(errorText, rc);
        }

        if (!pHandle){
            rc = EINVAL;
            errorText << "Invalid null parameter handle";
//            cerr << errorText << endl;
            LOG_ERROR_TEXT_ERRNO_AND_BAIL(errorText, rc);
        }

        BBTransferDefPTR  l_Transfer = getBBTransferDefPTR(pTransfer);
        if (!l_Transfer)
        {
            rc = EADDRNOTAVAIL;
            errorText << "Transfer parameter is invalid";
//            cerr << errorText << endl;
            LOG_ERROR_TEXT_ERRNO_AND_BAIL(errorText, rc);
        }

        txp::Msg::buildMsg(txp::BB_STARTTRANSFER, msg);

        msg->addAttribute(txp::handle, *((uint64_t*)&pHandle));

        // NOTE: jobid, jobstepid, and tag are NOT filled in
        //       for the transfer definition sent to bbproxy.
        //       They are both set to zero during construction.
        BBTransferDef::marshallTransferDef(msg, l_Transfer, freelist);

        rc = sendMessage(ProcessId, msg, reply);
        delete msg;
        if (rc) LOG_RC_AND_BAIL(rc);

        rc = waitReply(reply, msg);
        if (rc) LOG_RC_AND_BAIL(rc);

        rc = bberror.merge(msg);

        delete msg;
    }
    catch(ExceptionBailout& e) { }
    catch(exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

#ifdef PROF_TIMING
    chrono::high_resolution_clock::time_point time_stop = chrono::high_resolution_clock::now();
    std::chrono::duration<double,std::micro> elapsed_microseconds = time_stop - time_start;
    LOG(bb,trace) << __FILE__ <<":"<< __LINE__ << " " << __FUNCTION__ << "() completed after: " << elapsed_microseconds.count() << " microseconds";
#endif
    return rc;
}

int BB_GetTransferList(BBSTATUS pMatchStatus, uint64_t* pNumHandles, BBTransferHandle_t pHandles[], uint64_t* pNumAvailHandles)
{
#ifdef PROF_TIMING
    chrono::high_resolution_clock::time_point time_start = chrono::high_resolution_clock::now();
#endif

    int rc = 0;
    stringstream errorText;

    bberror.clear();

    ResponseDescriptor reply;
    txp::Msg* msg = 0;
    vector<txp::CharArray> freelist;  ///< Deallocates any marshalled CharArrays when function exits

    try
    {
        // Initialize the return variables
        *pNumAvailHandles = 0;

        // Verify initialization
        rc = ENODEV;
        verifyInit(true);
        rc = 0;

        if (!pHandles)
        {
            rc = EINVAL;
            errorText << "Invalid null parameter handle";
//            cerr << errorText << endl;
            LOG_ERROR_TEXT_ERRNO_AND_BAIL(errorText, rc);
        }

        if (!pNumHandles)
        {
            rc = EINVAL;
            errorText << "Invalid null parameter number of handles";
//            cerr << errorText << endl;
            LOG_ERROR_TEXT_ERRNO_AND_BAIL(errorText, rc);
        }

        if (!pNumAvailHandles)
        {
            rc = EINVAL;
            errorText << "Invalid null parameter number of available handles";
//            cerr << errorText << endl;
            LOG_ERROR_TEXT_ERRNO_AND_BAIL(errorText, rc);
        }

        // Build the message to be sent to bbproxy
        txp::Msg::buildMsg(txp::BB_GETTRANSFERLIST, msg);
        // NOTE:  The matchstatus value was not independently verified by bbapi...
        msg->addAttribute(txp::matchstatus, (uint64_t)pMatchStatus);
        msg->addAttribute(txp::numhandles, *pNumHandles);

        rc = sendMessage(ProcessId, msg, reply);
        delete msg;
        if (rc) LOG_RC_AND_BAIL(rc);

        rc = waitReply(reply, msg);
        if (rc) LOG_RC_AND_BAIL(rc);

        rc = bberror.merge(msg);
        if (!rc)
        {
            // NOTE:  Only return information if the rc from bbServer is zero...
            *pNumHandles = ((txp::Attr_uint64*)msg->retrieveAttrs()->at(txp::numhandles))->getData();
            *pNumAvailHandles = ((txp::Attr_uint64*)msg->retrieveAttrs()->at(txp::numavailhandles))->getData();
            uint64_t* l_Handles = (uint64_t*)msg->retrieveAttrs()->at(txp::handles)->getDataPtr();
            uint64_t* l_HandlesPtr = l_Handles;
            for(uint64_t x=0; x<*pNumHandles; x++)
            {
                // NOTE: Inverse the display order of the handles so they are
                //       in chronological ascending order
                pHandles[*pNumHandles-x-1] = (BBTransferHandle_t)(*l_HandlesPtr);
                ++l_HandlesPtr;
            }
        }
        delete msg;
    }
    catch(ExceptionBailout& e) { }
    catch(exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

#ifdef PROF_TIMING
    chrono::high_resolution_clock::time_point time_stop = chrono::high_resolution_clock::now();
    std::chrono::duration<double,std::micro> elapsed_microseconds = time_stop - time_start;
    LOG(bb,trace) << __FILE__ <<":"<< __LINE__ << " " << __FUNCTION__ << "() completed after: " << elapsed_microseconds.count() << " microseconds";
#endif
    return rc;
}

int BB_GetTransferInfo(BBTransferHandle_t pHandle, BBTransferInfo_t* pInfo)
{
#ifdef PROF_TIMING
    chrono::high_resolution_clock::time_point time_start = chrono::high_resolution_clock::now();
#endif

    int rc = 0;
    stringstream errorText;

    bberror.clear();

    ResponseDescriptor reply;
    txp::Msg* msg = 0;
    vector<txp::CharArray> freelist;  ///< Deallocates any marshalled CharArrays when function exits

    try
    {

        if (!pHandle)
        {
            rc = EINVAL;
            errorText << "Invalid null parameter handle";
//            cerr << errorText << endl;
            LOG_ERROR_TEXT_ERRNO_AND_BAIL(errorText, rc);
        }

        if (!pInfo)
        {
            rc = EINVAL;
            errorText << "Invalid null parameter for return information";
//            cerr << errorText << endl;
            LOG_ERROR_TEXT_ERRNO_AND_BAIL(errorText, rc);
        }

        // Initialize the return structure
        memset((void*)pInfo, 0, sizeof(BBTransferInfo_t));

        // Verify initialization
        rc = ENODEV;
        verifyInit(true);
        rc = 0;

        // Build the message to be sent to bbproxy
        txp::Msg::buildMsg(txp::BB_GETTRANSFERINFO, msg);

        msg->addAttribute(txp::handle, (uint64_t)pHandle);

        rc = sendMessage(ProcessId, msg, reply);
        delete msg;
        if (rc) LOG_RC_AND_BAIL(rc);

        rc = waitReply(reply, msg);
        if (rc) LOG_RC_AND_BAIL(rc);

        rc = bberror.merge(msg);
        if (!rc)
        {
            // NOTE:  Only return information if the rc from bbServer is zero...
            pInfo->handle = pHandle;
            pInfo->contribid = ((txp::Attr_uint32*)msg->retrieveAttrs()->at(txp::contribid))->getData();
            pInfo->jobid = ((txp::Attr_uint64*)msg->retrieveAttrs()->at(txp::jobid))->getData();
            pInfo->jobstepid = ((txp::Attr_uint64*)msg->retrieveAttrs()->at(txp::jobstepid))->getData();
            pInfo->tag = (BBTAG)((txp::Attr_uint64*)msg->retrieveAttrs()->at(txp::tag))->getData();
            pInfo->numcontrib = ((txp::Attr_uint64*)msg->retrieveAttrs()->at(txp::numcontrib))->getData();
            pInfo->status = (BBSTATUS)((txp::Attr_uint64*)msg->retrieveAttrs()->at(txp::status))->getData();
            pInfo->numreportingcontribs = ((txp::Attr_uint64*)msg->retrieveAttrs()->at(txp::numreportingcontribs))->getData();
            pInfo->totalTransferKeyLength = (size_t)((txp::Attr_uint64*)msg->retrieveAttrs()->at(txp::totalTransferKeyLength))->getData();
            pInfo->totalTransferSize = (size_t)((txp::Attr_uint64*)msg->retrieveAttrs()->at(txp::totalTransferSize))->getData();
            pInfo->localstatus = (BBSTATUS)((txp::Attr_uint64*)msg->retrieveAttrs()->at(txp::localstatus))->getData();
            pInfo->localTransferSize = (size_t)((txp::Attr_uint64*)msg->retrieveAttrs()->at(txp::localTransferSize))->getData();
        }
        delete msg;
    }
    catch(ExceptionBailout& e) { }
    catch(exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

#ifdef PROF_TIMING
    chrono::high_resolution_clock::time_point time_stop = chrono::high_resolution_clock::now();
    std::chrono::duration<double,std::micro> elapsed_microseconds = time_stop - time_start;
    LOG(bb,trace) << __FILE__ <<":"<< __LINE__ << " " << __FUNCTION__ << "() completed after: " << elapsed_microseconds.count() << " microseconds";
#endif
    return rc;
}


/*******************************************************************************
 | Operations for SSD setup
 *******************************************************************************/

int BB_CreateDirectory(const char* newpathname)
{
#ifdef PROF_TIMING
    chrono::high_resolution_clock::time_point time_start = chrono::high_resolution_clock::now();
#endif

    int rc = 0;
    stringstream errorText;

    bberror.clear();

    ResponseDescriptor reply;
    txp::Msg* msg = 0;

    try
    {
        // Verify initialization
        rc = ENODEV;
        verifyInit(true);
        rc = 0;

        if ( (!newpathname) || (!newpathname[0]) )
        {
            rc = EINVAL;
            errorText << "Invalid null parameter new path name";
//            cerr << errorText << endl;
            LOG_ERROR_TEXT_ERRNO_AND_BAIL(errorText, rc);
        }

        txp::Msg::buildMsg(txp::BB_CREATEDIR, msg);
        msg->addAttribute(txp::newpathname, newpathname, strlen(newpathname)+1);

        rc = sendMessage(ProcessId, msg, reply);
        delete msg;
        if (rc) LOG_RC_AND_BAIL(rc);

        rc = waitReply(reply, msg);
        if (rc) LOG_RC_AND_BAIL(rc);

        rc = bberror.merge(msg);
        delete msg;
    }
    catch(ExceptionBailout& e) { }
    catch(exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

#ifdef PROF_TIMING
    chrono::high_resolution_clock::time_point time_stop = chrono::high_resolution_clock::now();
    std::chrono::duration<double,std::micro> elapsed_microseconds = time_stop - time_start;
    LOG(bb,trace) << __FILE__ <<":"<< __LINE__ << " " << __FUNCTION__ << "() completed after: " << elapsed_microseconds.count() << " microseconds";
#endif
    return rc;
}

int BB_RemoveDirectory(const char* pathname)
{
#ifdef PROF_TIMING
    chrono::high_resolution_clock::time_point time_start = chrono::high_resolution_clock::now();
#endif

    int rc = 0;
    stringstream errorText;

    bberror.clear();

    ResponseDescriptor reply;
    txp::Msg* msg = 0;

    try
    {
        // Verify initialization
        rc = ENODEV;
        verifyInit(true);
        rc  =0;

        if ( (!pathname) || (!pathname[0]) )
        {
            rc = EINVAL;
            errorText << "Invalid null parameter path name";
//            cerr << errorText << endl;
            LOG_ERROR_TEXT_ERRNO_AND_BAIL(errorText, rc);
        }

        txp::Msg::buildMsg(txp::BB_REMOVEDIR, msg);
        msg->addAttribute(txp::pathname, pathname, strlen(pathname)+1);

        rc = sendMessage(ProcessId, msg, reply);
        delete msg;
        if (rc) LOG_RC_AND_BAIL(rc);

        rc = waitReply(reply, msg);
        if (rc) LOG_RC_AND_BAIL(rc);

        rc = bberror.merge(msg);
        delete msg;
    }
    catch(ExceptionBailout& e) { }
    catch(exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

#ifdef PROF_TIMING
    chrono::high_resolution_clock::time_point time_stop = chrono::high_resolution_clock::now();
    std::chrono::duration<double,std::micro> elapsed_microseconds = time_stop - time_start;
    LOG(bb,trace) << __FILE__ <<":"<< __LINE__ << " " << __FUNCTION__ << "() completed after: " << elapsed_microseconds.count() << " microseconds";
#endif
    return rc;
}

int BB_ChangeOwner(const char* pathname, const char* owner, const char* group)
{
#ifdef PROF_TIMING
    chrono::high_resolution_clock::time_point time_start = chrono::high_resolution_clock::now();
#endif

    int rc = 0;
    stringstream errorText;

    bberror.clear();

    ResponseDescriptor reply;
    txp::Msg* msg = 0;

    try
    {
        // Verify initialization
        rc = ENODEV;
        verifyInit(true);
        rc = 0;

        if ( (!pathname) || (!pathname[0]) )
        {
            rc = EINVAL;
            errorText << "Invalid null parameter path name";
//            cerr << errorText << endl;
            LOG_ERROR_TEXT_ERRNO_AND_BAIL(errorText, rc);
        }

        if ( (!owner)||(!owner[0]) )
        {
            rc = EINVAL;
            errorText << "Invalid null parameter owner";
//            cerr << errorText << endl;
            LOG_ERROR_TEXT_ERRNO_AND_BAIL(errorText, rc);
        }

        if ( (!group) || (!group[0]) )
        {
            rc = EINVAL;
            errorText << "Invalid null parameter group";
//            cerr << errorText << endl;
            LOG_ERROR_TEXT_ERRNO_AND_BAIL(errorText, rc);
        }


        txp::Msg::buildMsg(txp::BB_CHOWN, msg);
        msg->addAttribute(txp::pathname, pathname, strlen(pathname)+1);
        msg->addAttribute(txp::newowner, owner, strlen(owner)+1);
        msg->addAttribute(txp::newgroup, group, strlen(owner)+1);

        rc = sendMessage(ProcessId, msg, reply);
        delete msg;
        if (rc) LOG_RC_AND_BAIL(rc);

        rc = waitReply(reply, msg);
        if (rc) LOG_RC_AND_BAIL(rc);

        rc = bberror.merge(msg);
        delete msg;
    }
    catch(ExceptionBailout& e) { }
    catch(exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

#ifdef PROF_TIMING
    chrono::high_resolution_clock::time_point time_stop = chrono::high_resolution_clock::now();
    std::chrono::duration<double,std::micro> elapsed_microseconds = time_stop - time_start;
    LOG(bb,trace) << __FILE__ <<":"<< __LINE__ << " " << __FUNCTION__ << "() completed after: " << elapsed_microseconds.count() << " microseconds";
#endif
    return rc;
}

int BB_ChangeMode(const char* pathname, mode_t mode)
{
#ifdef PROF_TIMING
    chrono::high_resolution_clock::time_point time_start = chrono::high_resolution_clock::now();
#endif

    int rc = 0;
    stringstream errorText;

    bberror.clear();

    ResponseDescriptor reply;
    txp::Msg* msg = 0;

    try
    {
        // Verify initialization
        rc = ENODEV;
        verifyInit(true);
        rc = 0;

        if ( (!pathname) || (!pathname[0]) )
        {
            rc = EINVAL;
            errorText << "Invalid null parameter path name";
//            cerr << errorText << endl;
            LOG_ERROR_TEXT_ERRNO_AND_BAIL(errorText, rc);
        }

        txp::Msg::buildMsg(txp::BB_CHMOD, msg);
        msg->addAttribute(txp::pathname, pathname, strlen(pathname)+1);
        msg->addAttribute(txp::newmode, mode);

        rc = sendMessage(ProcessId, msg, reply);
        delete msg;
        if (rc) LOG_RC_AND_BAIL(rc);

        rc = waitReply(reply, msg);
        if (rc) LOG_RC_AND_BAIL(rc);

        rc = bberror.merge(msg);
        delete msg;
    }
    catch(ExceptionBailout& e) { }
    catch(exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

#ifdef PROF_TIMING
    chrono::high_resolution_clock::time_point time_stop = chrono::high_resolution_clock::now();
    std::chrono::duration<double,std::micro> elapsed_microseconds = time_stop - time_start;
    LOG(bb,trace) << __FILE__ <<":"<< __LINE__ << " " << __FUNCTION__ << "() completed after: " << elapsed_microseconds.count() << " microseconds";
#endif
    return rc;
}

int BB_CreateLogicalVolume(const char* mountpoint, const char* size, BBCREATEFLAGS flags=BBXFS)
{
#ifdef PROF_TIMING
    chrono::high_resolution_clock::time_point time_start = chrono::high_resolution_clock::now();
#endif

    int rc = 0;
    stringstream errorText;

    bberror.clear();

    ResponseDescriptor reply;
    txp::Msg* msg = 0;

    try
    {
        // Verify initialization
        rc = ENODEV;
        verifyInit(true);
        rc = 0;

        if ( (!mountpoint)|| (!mountpoint[0]) )
        {
            rc = EINVAL;
            errorText << "Invalid null parameter mountpoint";
//            cerr << errorText << endl;
            LOG_ERROR_TEXT_ERRNO_AND_BAIL(errorText, rc);
        }

        if ( (!size)|| (!size[0]) )
        {
            rc = EINVAL;
            errorText << "Invalid null parameter size";
//            cerr << errorText << endl;
            LOG_ERROR_TEXT_ERRNO_AND_BAIL(errorText, rc);
        }

        if (check_createlv_flags(flags))
        {
            rc = EINVAL;
            errorText << "Create logical volume option 0x" << hex << uppercase << setfill('0') << flags << setfill(' ') << nouppercase << dec << " is invalid.  Most common value is BBXFS";
//            cerr << errorText << endl;
            LOG_ERROR_TEXT_ERRNO_AND_BAIL(errorText, rc);
        }

        txp::Msg::buildMsg(txp::BB_CREATELOGICALVOLUME, msg);
        msg->addAttribute(txp::mountpoint, mountpoint, strlen(mountpoint)+1);
        msg->addAttribute(txp::mountsize, size, strlen(size)+1);
        msg->addAttribute(txp::flags, (uint64_t)flags);

        rc = sendMessage(ProcessId, msg, reply);
        delete msg;
        if (rc) LOG_RC_AND_BAIL(rc);

        rc = waitReply(reply, msg);
        if (rc) LOG_RC_AND_BAIL(rc);

        rc = bberror.merge(msg);
        delete msg;
    }
    catch(ExceptionBailout& e) { }
    catch(exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

#ifdef PROF_TIMING
    chrono::high_resolution_clock::time_point time_stop = chrono::high_resolution_clock::now();
    std::chrono::duration<double,std::micro> elapsed_microseconds = time_stop - time_start;
    LOG(bb,trace) << __FILE__ <<":"<< __LINE__ << " " << __FUNCTION__ << "() completed after: " << elapsed_microseconds.count() << " microseconds";
#endif
    return rc;
}

int BB_ResizeMountPoint(const char* mountpoint, const char* size, BBRESIZEFLAGS flags)
{
#ifdef PROF_TIMING
    chrono::high_resolution_clock::time_point time_start = chrono::high_resolution_clock::now();
#endif

    int rc = 0;
    stringstream errorText;

    bberror.clear();

    ResponseDescriptor reply;
    txp::Msg* msg = 0;

    try
    {
        // Verify initialization
        rc = ENODEV;
        verifyInit(true);
        rc = 0;

        if ( (!mountpoint)|| (!mountpoint[0]) )
        {
            rc = EINVAL;
            errorText << "Invalid null parameter mountpoint";
//            cerr << errorText << endl;
            LOG_ERROR_TEXT_ERRNO_AND_BAIL(errorText, rc);
        }

        if ( (!size)|| (!size[0]) )
        {
            rc = EINVAL;
            errorText << "Invalid null parameter size";
//            cerr << errorText << endl;
            LOG_ERROR_TEXT_ERRNO_AND_BAIL(errorText, rc);
        }

        if (check_resize_flags(flags))
        {
            rc = EINVAL;
            errorText << "Resize mountpoint option 0x" << hex << uppercase << setfill('0') << flags << setfill(' ') << nouppercase << dec << " is invalid";
//            cerr << errorText << endl;
            LOG_ERROR_TEXT_ERRNO_AND_BAIL(errorText, rc);
        }

        txp::Msg::buildMsg(txp::BB_RESIZEMOUNTPOINT, msg);
        msg->addAttribute(txp::mountpoint, mountpoint, strlen(mountpoint)+1);
        msg->addAttribute(txp::mountsize, size, strlen(size)+1);
        msg->addAttribute(txp::flags, (uint64_t)flags);

        rc = sendMessage(ProcessId, msg, reply);
        delete msg;
        if (rc) LOG_RC_AND_BAIL(rc);

        rc = waitReply(reply, msg);
        if (rc) LOG_RC_AND_BAIL(rc);

        // NOTE:  The storage for logical volume will be deleted by the invoker...
        rc = bberror.merge(msg);
        delete msg;
    }
    catch(ExceptionBailout& e) { }
    catch(exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

#ifdef PROF_TIMING
    chrono::high_resolution_clock::time_point time_stop = chrono::high_resolution_clock::now();
    std::chrono::duration<double,std::micro> elapsed_microseconds = time_stop - time_start;
    LOG(bb,trace) << __FILE__ <<":"<< __LINE__ << " " << __FUNCTION__ << "() completed after: " << elapsed_microseconds.count() << " microseconds";
#endif
    return rc;
}

int BB_RemoveLogicalVolume(const char* mountpoint)
{
#ifdef PROF_TIMING
    chrono::high_resolution_clock::time_point time_start = chrono::high_resolution_clock::now();
#endif

    int rc = 0;
    stringstream errorText;

    bberror.clear();

    ResponseDescriptor reply;
    txp::Msg* msg = 0;

    try
    {
        // Verify initialization
        rc = ENODEV;
        verifyInit(true);
        rc = 0;

        if ( (!mountpoint)|| (!mountpoint[0]) )
        {
            rc = EINVAL;
            errorText << "Invalid null parameter mountpoint";
//            cerr << errorText << endl;
            LOG_ERROR_TEXT_ERRNO_AND_BAIL(errorText, rc);
        }

        txp::Msg::buildMsg(txp::BB_REMOVELOGICALVOLUME, msg);
        msg->addAttribute(txp::mountpoint, mountpoint, strlen(mountpoint)+1);

        rc = sendMessage(ProcessId, msg, reply);
        delete msg;
        if (rc) LOG_RC_AND_BAIL(rc);

        rc = waitReply(reply, msg);
        if (rc) LOG_RC_AND_BAIL(rc);

        rc = bberror.merge(msg);
        delete msg;
    }
    catch(ExceptionBailout& e) { }
    catch(exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

#ifdef PROF_TIMING
    chrono::high_resolution_clock::time_point time_stop = chrono::high_resolution_clock::now();
    std::chrono::duration<double,std::micro> elapsed_microseconds = time_stop - time_start;
    LOG(bb,trace) << __FILE__ <<":"<< __LINE__ << " " << __FUNCTION__ << "() completed after: " << elapsed_microseconds.count() << " microseconds";
#endif
    return rc;
}

int BB_RemoveJobInfo()
{
#ifdef PROF_TIMING
    chrono::high_resolution_clock::time_point time_start = chrono::high_resolution_clock::now();
#endif

    int rc = 0;
    stringstream errorText;

    bberror.clear();

    ResponseDescriptor reply;
    txp::Msg* msg = 0;

    try
    {
        txp::Msg::buildMsg(txp::BB_REMOVEJOBINFO, msg);
        rc = sendMessage(ProcessId, msg, reply);
        delete msg;
        if (rc) LOG_RC_AND_BAIL(rc);

        rc = waitReply(reply, msg);
        if (rc) LOG_RC_AND_BAIL(rc);

        rc = bberror.merge(msg);
        delete msg;
    }
    catch(ExceptionBailout& e) { }
    catch(exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

#ifdef PROF_TIMING
    chrono::high_resolution_clock::time_point time_stop = chrono::high_resolution_clock::now();
    std::chrono::duration<double,std::micro> elapsed_microseconds = time_stop - time_start;
    LOG(bb,trace) << __FILE__ <<":"<< __LINE__ << " " << __FUNCTION__ << "() completed after: " << elapsed_microseconds.count() << " microseconds";
#endif
    return rc;
}

int BB_GetUsage(const char* mountpoint, BBUsage_t* usage)
{
#ifdef PROF_TIMING
    chrono::high_resolution_clock::time_point time_start = chrono::high_resolution_clock::now();
#endif

    int rc = 0;
    stringstream errorText;

    bberror.clear();

    ResponseDescriptor reply;
    txp::Msg* msg = 0;

    try
    {
        // Verify initialization
        rc = ENODEV;
        verifyInit(true);
        rc = 0;

        if ( (!mountpoint)|| (!mountpoint[0]) )
        {
            rc = EINVAL;
            errorText << "Invalid null parameter mountpoint";
//            cerr << errorText << endl;
            LOG_ERROR_TEXT_ERRNO_AND_BAIL(errorText, rc);
        }

        if(usage == NULL)
        {
            rc = EFAULT;
            errorText << "Invalid null parameter usage";
//            cerr << errorText << endl;
            LOG_ERROR_TEXT_ERRNO_AND_BAIL(errorText, rc);
        }

        txp::Msg::buildMsg(txp::BB_GETUSAGE, msg);
        msg->addAttribute(txp::mountpoint, mountpoint, strlen(mountpoint)+1);

        rc = sendMessage(ProcessId, msg, reply);
        delete msg;
        if (rc) LOG_RC_AND_BAIL(rc);

        rc = waitReply(reply, msg);
        if (rc) LOG_RC_AND_BAIL(rc);

        rc = bberror.merge(msg);
        if(rc == 0)
        {
#define ADDFIELD(name) usage->name= ((txp::Attr_uint64*)msg->retrieveAttrs()->at(txp::name))->getData();
            ADDFIELD(totalBytesRead);
            ADDFIELD(totalBytesWritten);
            ADDFIELD(localBytesRead);
            ADDFIELD(localBytesWritten);
#if BBUSAGE_COUNT
            auto vt = getVersionPropertyTree();
            if(vt.get("version.bbusage", 1) >= 2)  // caller has older structure allocation that predates these fields
            {
                ADDFIELD(localReadCount);
                ADDFIELD(localWriteCount);
            }
#endif
            ADDFIELD(burstBytesRead);
            ADDFIELD(burstBytesWritten);
#undef ADDFIELD
        }
        delete msg;
    }
    catch(ExceptionBailout& e) { }
    catch(exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

#ifdef PROF_TIMING
    chrono::high_resolution_clock::time_point time_stop = chrono::high_resolution_clock::now();
    std::chrono::duration<double,std::micro> elapsed_microseconds = time_stop - time_start;
    LOG(bb,trace) << __FILE__ <<":"<< __LINE__ << " " << __FUNCTION__ << "() completed after: " << elapsed_microseconds.count() << " microseconds";
#endif
    return rc;
}

int BB_GetDeviceUsage(uint32_t devicenum, BBDeviceUsage_t* usage)
{
#ifdef PROF_TIMING
    chrono::high_resolution_clock::time_point time_start = chrono::high_resolution_clock::now();
#endif

    int rc = 0;
    stringstream errorText;

    bberror.clear();

    ResponseDescriptor reply;
    txp::Msg* msg = 0;

    try
    {
        // Verify initialization
        rc = ENODEV;
        verifyInit(true);
        rc = 0;

        if(usage == NULL)
        {
            rc = EFAULT;
            errorText << "Invalid null parameter usage";
//            cerr << errorText << endl;
            LOG_ERROR_TEXT_ERRNO_AND_BAIL(errorText, rc);
        }
        memset(usage,0,sizeof(BBDeviceUsage_t) );

        txp::Msg::buildMsg(txp::BB_GETDEVICEUSAGE, msg);
        msg->addAttribute(txp::devicenum, devicenum);

        rc = sendMessage(ProcessId, msg, reply);
        delete msg;
        if (rc) LOG_RC_AND_BAIL(rc);

        rc = waitReply(reply, msg);
        if (rc) LOG_RC_AND_BAIL(rc);

        rc = bberror.merge(msg);
        if(rc == 0)
        {
#define usageattr(name) usage->name = ((txp::Attr_uint64*)msg->retrieveAttrs()->at(txp::name))->getData();
            usageattr(critical_warning);
            usageattr(temperature);
            usageattr(available_spare);
            usageattr(percentage_used);
            usageattr(data_read);
            usageattr(data_written);
            usageattr(num_read_commands);
            usageattr(num_write_commands);
            usageattr(busy_time);
            usageattr(power_cycles);
            usageattr(power_on_hours);
            usageattr(unsafe_shutdowns);
            usageattr(media_errors);
            usageattr(num_err_log_entries);
#undef usageattr
        }
        delete msg;
    }
    catch(ExceptionBailout& e) { }
    catch(exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

#ifdef PROF_TIMING
    chrono::high_resolution_clock::time_point time_stop = chrono::high_resolution_clock::now();
    std::chrono::duration<double,std::micro> elapsed_microseconds = time_stop - time_start;
    LOG(bb,trace) << __FILE__ <<":"<< __LINE__ << " " << __FUNCTION__ << "() completed after: " << elapsed_microseconds.count() << " microseconds";
#endif
    return rc;
}

int BB_SetUsageLimit(const char* mountpoint, BBUsage_t* usage)
{
#ifdef PROF_TIMING
    chrono::high_resolution_clock::time_point time_start = chrono::high_resolution_clock::now();
#endif

    int rc = 0;
    stringstream errorText;

    bberror.clear();

    ResponseDescriptor reply;
    txp::Msg* msg = 0;

    try
    {
        // Verify initialization
        rc = ENODEV;
        verifyInit(true);
        rc = 0;

        if ( (!mountpoint)|| (!mountpoint[0]) )
        {
            rc = EINVAL;
            errorText << "Invalid null parameter mountpoint";
//            cerr << errorText << endl;
            LOG_ERROR_TEXT_ERRNO_AND_BAIL(errorText, rc);
        }

        if(usage == NULL)
        {
            rc = EFAULT;
            errorText << "Invalid null parameter usage";
//            cerr << errorText << endl;
            LOG_ERROR_TEXT_ERRNO_AND_BAIL(errorText, rc);
        }

        txp::Msg::buildMsg(txp::BB_SETUSAGELIMIT, msg);
        msg->addAttribute(txp::mountpoint, mountpoint, strlen(mountpoint)+1);
#define ADDFIELD(name) msg->addAttribute(txp::name, usage->name)
        ADDFIELD(totalBytesRead);
        ADDFIELD(totalBytesWritten);
        ADDFIELD(localBytesRead);
        ADDFIELD(localBytesWritten);
        ADDFIELD(burstBytesRead);
        ADDFIELD(burstBytesWritten);
#undef ADDFIELD

        rc = sendMessage(ProcessId, msg, reply);
        delete msg;
        if (rc) LOG_RC_AND_BAIL(rc);

        rc = waitReply(reply, msg);
        if (rc) LOG_RC_AND_BAIL(rc);

        rc = bberror.merge(msg);
        delete msg;
    }
    catch(ExceptionBailout& e) { }
    catch(exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

#ifdef PROF_TIMING
    chrono::high_resolution_clock::time_point time_stop = chrono::high_resolution_clock::now();
    std::chrono::duration<double,std::micro> elapsed_microseconds = time_stop - time_start;
    LOG(bb,trace) << __FILE__ <<":"<< __LINE__ << " " << __FUNCTION__ << "() completed after: " << elapsed_microseconds.count() << " microseconds";
#endif
    return rc;
}

int BB_GetServerByName(const char* bbserverName, const char* type,size_t bufsize, char* buffer)
{
    int rc=0;
    stringstream errorText;
    bberror.clear();
    ResponseDescriptor reply;
    txp::Msg* msg = 0;
    try{
        string l_type=type;
        BBServerQuery l_query=BBWAITFOREPLYCOUNT;;
        if (l_type=="waitforreplycount"){
            l_query=BBWAITFOREPLYCOUNT;
        }
        else {
            rc=EINVAL;
            errorText << "Parameter type is invalid--expecting all, active, or ready";
            LOG_ERROR_TEXT_ERRNO_AND_BAIL(errorText, rc);
        }
        if (!bufsize){
            rc = EINVAL;
            errorText << "Parameter bufsize is 0";
            LOG_ERROR_TEXT_ERRNO_AND_BAIL(errorText, rc);
        }
        if (!buffer)
        {
            rc = EFAULT;
            errorText << "Parameter buffer is NULL";
            LOG_ERROR_TEXT_ERRNO_AND_BAIL(errorText, rc);
        }

        txp::Msg::buildMsg(txp::BB_GETSERVERBYNAME, msg);
        msg->addAttribute(txp::value64, (int64_t)l_query);
        msg->addAttribute(txp::hostname, bbserverName, strlen(bbserverName)+1);

        rc = sendMessage(ProcessId, msg, reply);
        delete msg;
        if (rc) LOG_RC_AND_BAIL(rc);

        rc = waitReply(reply, msg);
        if (rc) LOG_RC_AND_BAIL(rc);

        rc = bberror.merge(msg);
        if (!rc)
        {
            // Process response data
            uint64_t l_BufferSize = (uint64_t)msg->retrieveAttrs()->at(txp::buffer)->getDataLength();
            char * l_Buffer = (char*)(msg->retrieveAttrs()->at(txp::buffer)->getDataPtr());
            if (bufsize < l_BufferSize)
            {
                rc=ERANGE;
                LOG_RC_AND_BAIL(rc);
            }
            memcpy(buffer,l_Buffer,l_BufferSize);
        }
        delete msg;
    }
    catch(ExceptionBailout& e) { }
    catch(exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

    return rc;
}

int BB_GetServer(const char* type, size_t bufsize, char* buffer)
{
    int rc=0;
    stringstream errorText;
    bberror.clear();
    ResponseDescriptor reply;
    txp::Msg* msg = 0;
    try{
        if (!type)
        {
            rc = EFAULT;
            errorText << "Parameter type is NULL";
            LOG_ERROR_TEXT_ERRNO_AND_BAIL(errorText, rc);
        }
        string l_type=type;
        BBServerQuery l_query=BBALLCONNECTED;;
        if (l_type=="all"){
            l_query=BBALLCONNECTED;
        }
        else if (l_type=="active"){
            l_query=BBACTIVE ;
        }
        else if (l_type=="ready"){
            l_query=BBREADY;
        }
        else if (l_type=="backup"){
            l_query=BBBACKUP;
        }
        else if (l_type=="primary"){
            l_query=BBPRIMARY;
        }
        else {
            rc=EINVAL;
            errorText << "Parameter type is invalid--expecting all, active, or ready";
            LOG_ERROR_TEXT_ERRNO_AND_BAIL(errorText, rc);
        }

        if (!bufsize){
            rc = EINVAL;
            errorText << "Parameter bufsize is 0";
            LOG_ERROR_TEXT_ERRNO_AND_BAIL(errorText, rc);
        }
        if (!buffer)
        {
            rc = EFAULT;
            errorText << "Parameter buffer is NULL";
            LOG_ERROR_TEXT_ERRNO_AND_BAIL(errorText, rc);
        }

        txp::Msg::buildMsg(txp::BB_GETSERVER, msg);

        msg->addAttribute(txp::value64, (int64_t)l_query);

        rc = sendMessage(ProcessId, msg, reply);
        delete msg;
        if (rc) LOG_RC_AND_BAIL(rc);

        rc = waitReply(reply, msg);
        if (rc) LOG_RC_AND_BAIL(rc);

        rc = bberror.merge(msg);
        if (!rc)
        {
            // Process response data
            uint64_t l_BufferSize = (uint64_t)msg->retrieveAttrs()->at(txp::buffer)->getDataLength();
            char * l_Buffer = (char*)(msg->retrieveAttrs()->at(txp::buffer)->getDataPtr());
            if (bufsize < l_BufferSize)
            {
                rc=ERANGE;
                LOG_RC_AND_BAIL(rc);
            }
            memcpy(buffer,l_Buffer,l_BufferSize);
        }
        delete msg;
    }
    catch(ExceptionBailout& e) { }
    catch(exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

    return rc;
}

int BB_SetServer(const char * type, const char* buffer){
    int rc=0;
    stringstream errorText;
    bberror.clear();
    ResponseDescriptor reply;
    txp::Msg* msg = 0;


    try{
        if ( (!type) || (!type[0]) )
        {
            rc = EFAULT;
            errorText << "Parameter type is NULL or 0 length";
            LOG_ERROR_TEXT_ERRNO_AND_BAIL(errorText, rc);
        }
        if ( (!buffer) || (!strlen(buffer) ) )
        {
            rc = EINVAL;
            errorText << "Parameter buffer is NULL or 0 length string";
            LOG_ERROR_TEXT_ERRNO_AND_BAIL(errorText, rc);
        }
        txp::Msg::buildMsg(txp::BB_SETSERVER, msg);
        msg->addAttribute(txp::hostname, buffer, strlen(buffer)+1);
        msg->addAttribute(txp::variable,type,  strlen(type) + 1);
        rc = sendMessage(ProcessId, msg, reply);
        delete msg;
        if (rc) LOG_RC_AND_BAIL(rc);

        rc = waitReply(reply, msg);
        if (rc) LOG_RC_AND_BAIL(rc);

        rc = bberror.merge(msg);
        delete msg;
    }
    catch(ExceptionBailout& e) { }
    catch(exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

    return rc;
}

int BB_OpenServer(const char* buffer){
    int rc=0;
    stringstream errorText;
    bberror.clear();
    ResponseDescriptor reply;
    txp::Msg* msg = 0;


    try{
        if ( (!buffer) || (!strlen(buffer) ) )
        {
            rc = EINVAL;
            errorText << "Parameter buffer is NULL or 0 length string";
            LOG_ERROR_TEXT_ERRNO_AND_BAIL(errorText, rc);
        }
        txp::Msg::buildMsg(txp::BB_OPENSERVER, msg);
        msg->addAttribute(txp::hostname, buffer, strlen(buffer)+1);

        rc = sendMessage(ProcessId, msg, reply);
        delete msg;
        if (rc) LOG_RC_AND_BAIL(rc);

        rc = waitReply(reply, msg);
        if (rc) LOG_RC_AND_BAIL(rc);

        rc = bberror.merge(msg);
        delete msg;
    }
    catch(ExceptionBailout& e) { }
    catch(exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

    return rc;
}

int BB_CloseServer(const char* buffer){
    int rc=0;
    stringstream errorText;
    bberror.clear();
    ResponseDescriptor reply;
    txp::Msg* msg = 0;


    try{
        if ( (!buffer) || (!strlen(buffer) ) )
        {
            rc = EINVAL;
            errorText << "Parameter buffer is NULL or 0 length string";
            LOG_ERROR_TEXT_ERRNO_AND_BAIL(errorText, rc);
        }
        txp::Msg::buildMsg(txp::BB_CLOSESERVER, msg);
        msg->addAttribute(txp::hostname, buffer, strlen(buffer)+1);

        rc = sendMessage(ProcessId, msg, reply);
        delete msg;
        if (rc) LOG_RC_AND_BAIL(rc);

        rc = waitReply(reply, msg);
        if (rc) LOG_RC_AND_BAIL(rc);

        rc = bberror.merge(msg);
        delete msg;
    }
    catch(ExceptionBailout& e) { }
    catch(exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

    return rc;
}


int BB_Suspend(const char* pHostName)
{
    int rc = 0;
    stringstream errorText;

    bberror.clear();

    ResponseDescriptor reply;
    txp::Msg* msg = 0;

    try
    {
        // Verify initialization
        rc = ENODEV;
        verifyInit(true);
        rc = 0;

        txp::Msg::buildMsg(txp::BB_SUSPEND, msg);
        msg->addAttribute(txp::hostname, pHostName, strlen(pHostName)+1);

        rc = sendMessage(ProcessId, msg, reply);
        delete msg;
        if (rc) LOG_RC_AND_BAIL(rc);

        rc = waitReply(reply, msg);
        if (rc) LOG_RC_AND_BAIL(rc);

        rc = bberror.merge(msg);
        delete msg;
    }
    catch(ExceptionBailout& e) { }
    catch(exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

    return rc;
}

int BB_Resume(const char* pHostName)
{
    int rc = 0;
    stringstream errorText;

    bberror.clear();

    ResponseDescriptor reply;
    txp::Msg* msg = 0;

    try
    {
        // Verify initialization
        rc = ENODEV;
        verifyInit(true);
        rc = 0;

        txp::Msg::buildMsg(txp::BB_RESUME, msg);
        msg->addAttribute(txp::hostname, pHostName, strlen(pHostName)+1);

        rc = sendMessage(ProcessId, msg, reply);
        delete msg;
        if (rc) LOG_RC_AND_BAIL(rc);

        rc = waitReply(reply, msg);
        if (rc) LOG_RC_AND_BAIL(rc);

        rc = bberror.merge(msg);
        delete msg;
    }
    catch(ExceptionBailout& e) { }
    catch(exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

    return rc;
}

int BB_RetrieveTransfers(const char* pHostName, const uint64_t pHandle, const BB_RTV_TRANSFERDEFS_FLAGS pFlags, uint32_t* pNumTransferDefs, size_t* pTransferDefsSize, const size_t pBufferSize, char* pBuffer)
{
    int rc = 0;
    stringstream errorText;

    bberror.clear();

    ResponseDescriptor reply;
    txp::Msg* msg = 0;

    uint32_t l_DataObtainedLocally = 0;
    string l_TransferDefsStr;

    try
    {
        // Verify initialization
        rc = ENODEV;
        verifyInit(true);
        rc = 0;

        if (check_retrieve_transfer_defs_flags(pFlags))
        {
            rc = EINVAL;
            errorText << "Retrieve transfer definitions option 0x" << hex << uppercase << setfill('0') << pFlags << setfill(' ') << nouppercase << dec << " is invalid";
            LOG_ERROR_TEXT_ERRNO_AND_BAIL(errorText, rc);
        }

        txp::Msg::buildMsg(txp::BB_RETRIEVE_TRANSFERS, msg);
        msg->addAttribute(txp::hostname, pHostName, strlen(pHostName)+1);
        msg->addAttribute(txp::handle, pHandle);
        msg->addAttribute(txp::flags, (uint64_t)pFlags);

        rc = sendMessage(ProcessId, msg, reply);
        delete msg;
        if (rc) LOG_RC_AND_BAIL(rc);

        rc = waitReply(reply, msg);
        if (rc) LOG_RC_AND_BAIL(rc);

        rc = bberror.merge(msg);

        if (!rc)
        {
            l_DataObtainedLocally = ((txp::Attr_uint32*)msg->retrieveAttrs()->at(txp::dataObtainedLocally))->getData();
            *pNumTransferDefs = ((txp::Attr_uint32*)msg->retrieveAttrs()->at(txp::numTransferDefs))->getData();
            *pTransferDefsSize = (size_t)(msg->retrieveAttrs()->at(txp::transferdefs)->getDataLength());
            // NOTE: archive string is already null terminated
            l_TransferDefsStr.assign((const char*)msg->retrieveAttrs()->at(txp::transferdefs)->getDataPtr(), *pTransferDefsSize);

            // Insert returned data into return variables/errstate...
            bberror.errdirect("out.transferDefsSize", *pTransferDefsSize);
            if (*pTransferDefsSize < pBufferSize)
            {
                bberror.errdirect("out.dataObtainedLocally", l_DataObtainedLocally);
                bberror.errdirect("out.numTransferDefs", *pNumTransferDefs);
                // NOTE: archive string is already null terminated and the length accounts for the null terminator
                l_TransferDefsStr.copy(pBuffer, *pTransferDefsSize);
                bberror.errdirect("out.transferdefs", l_TransferDefsStr);
            }
        }

        delete msg;

    }
    catch(ExceptionBailout& e) { }
    catch(exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

    return rc;
}

int BB_StopTransfers(const char* pHostName, const uint64_t pHandle, uint32_t* pNumStoppedTransferDefs, const char* pTransferDefs, const size_t pTansferDefsSize)
{
    int rc = 0;
    stringstream errorText;

    bberror.clear();

    ResponseDescriptor reply;
    txp::Msg* msg = 0;

    try
    {
        // Verify initialization
        rc = ENODEV;
        verifyInit(true);
        rc = 0;

        txp::Msg::buildMsg(txp::BB_STOP_TRANSFERS, msg);
        msg->addAttribute(txp::hostname, pHostName, strlen(pHostName)+1);
        msg->addAttribute(txp::handle, pHandle);
        msg->addAttribute(txp::transferdefs, pTransferDefs, pTansferDefsSize);

        rc = sendMessage(ProcessId, msg, reply);
        delete msg;
        if (rc) LOG_RC_AND_BAIL(rc);

        rc = waitReply(reply, msg);
        if (rc) LOG_RC_AND_BAIL(rc);

        rc = bberror.merge(msg);

        if (!rc)
        {
            *pNumStoppedTransferDefs = ((txp::Attr_uint32*)msg->retrieveAttrs()->at(txp::numTransferDefs))->getData();
        }

        delete msg;

    }
    catch(ExceptionBailout& e) { }
    catch(exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

    return rc;
}

int BB_RestartTransfers(const char* pHostName, const uint64_t pHandle, uint32_t* pNumRestartedTransferDefs, const char* pTransferDefs, const size_t pTransferDefsSize)
{
    int rc = 0;
    stringstream errorText;

    bberror.clear();

    ResponseDescriptor reply;
    txp::Msg* msg = 0;

    try
    {
        // Verify initialization
        rc = ENODEV;
        verifyInit(true);
        rc = 0;

        txp::Msg::buildMsg(txp::BB_RESTART_TRANSFERS, msg);
        msg->addAttribute(txp::hostname, pHostName, strlen(pHostName)+1);
        msg->addAttribute(txp::handle, pHandle);
        msg->addAttribute(txp::transferdefs, pTransferDefs, pTransferDefsSize);

        rc = sendMessage(ProcessId, msg, reply);
        delete msg;
        if (rc) LOG_RC_AND_BAIL(rc);

        rc = waitReply(reply, msg);
        if (rc) LOG_RC_AND_BAIL(rc);

        rc = bberror.merge(msg);

        if (!rc)
        {
            *pNumRestartedTransferDefs = ((txp::Attr_uint32*)msg->retrieveAttrs()->at(txp::numTransferDefs))->getData();
        }

        delete msg;

    }
    catch(ExceptionBailout& e) { }
    catch(exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

    return rc;
}
