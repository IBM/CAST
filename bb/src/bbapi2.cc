/*******************************************************************************
 |    bbapi2.cc
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

#include <boost/program_options.hpp>
#include <boost/property_tree/ptree.hpp>

// NOTE:  Do not put "connections.h" *AFTER*
//        "using namespace std".
//        Logging failures will occur if you do...
#include "connections.h"

using namespace std;

#include "bbapi_flightlog.h"
#include "bbapi.h"
#include "bbapi2.h"
#include "bbinternal.h"
#include "BBTransferDef.h"
#include "logging.h"

#define NAME "bbAPI"

#ifdef TXP_DEVELOPMENT
txp::Log bbapi2_log(txp::Log::DEFAULT_LOG_DESTINATION, txp::Log::DEFAULT_OPEN_LOGSTATE, txp::Log::DEBUG_LOGLEVEL);
#endif

uint64_t BBJOBID = UNDEFINED_JOBID;

/*******************************************************************************
 | Helper routines
 *******************************************************************************/

static int bbapi_SetVariable(const string& pVariable, const string& pValue)
{
    int rc = 0;
    ResponseDescriptor reply;
    txp::Msg* msg = 0;

    try
    {
        txp::Msg::buildMsg(txp::CORAL_SETVAR, msg);
        msg->addAttribute(txp::variable, pVariable.c_str(), pVariable.size()+1);
        msg->addAttribute(txp::value, pValue.c_str(), pValue.size()+1);

        rc = sendMessage(ProcessId, msg, reply);
        delete msg;
        if (rc) SET_RC_AND_BAIL(rc);

        rc = waitReply(reply, msg);
        if (rc) SET_RC_AND_BAIL(rc);

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

int registerHandlers()
{
    int rc = 0;

    return rc;
}


/*******************************************************************************
 | External routines
 *******************************************************************************/
#define COMPARE_GIT_TREE 0
// NOTE:  This procedure is common to both BB_InitLibrary() and Coral_InitLibrary()...
int BB_InitLibrary2(uint32_t pContribId, const char* pClientVersion, const char* pUnixpath)
{
    int rc = 0;
    stringstream errorText;
    const char* tmp;

    try
    {
        if (pUnixpath && strlen(pUnixpath))
        {
            config.put("bb.unixpath", pUnixpath);
        }

        string who("bb.api");
        string instance;
        instance = to_string(getpid());

        initializeLogging(who + ".log", config);
        LOG(bb,always) << "Starting " NAME;
        LOG(bb,always) << "Using configuration file " << curConfig.getPath() << ".";
        LOG(bb,always) << "Using unixpath " << config.get("bb.unixpath", DEFAULT_UNIXPATH) << ".";
        LOG(bb,always) << "Process identifier: " << who;
        LOG(bb,always) << "Process instance  : " << instance;
        LOG(bb,always) << "Flightlog location: " << config.get(who + ".flightlog", NO_CONFIG_VALUE);
        rc = FL_CreateAll(config.get(who + ".flightlog", NO_CONFIG_VALUE).c_str());
        if (rc)
        {
            errorText << "Unable to initialize flightlog (" << config.get(who + ".flightlog", NO_CONFIG_VALUE) << ")";
            bberror << err("error.flightlogPath", config.get(who + ".flightlog", NO_CONFIG_VALUE));
            LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
        }

        std::string l_ClientVersion = pClientVersion;
        rc = versionCheck(l_ClientVersion);

        char myjobid[256];

        if(BBJOBID == 0)
        {
            char* jobid = getenv(config.get(who + ".env_jobid",    "LSF_STAGE_JOBID").c_str());
            if (jobid == NULL)
            {
                jobid = getenv(config.get(who + ".env_jobid",    "LSB_JOBID").c_str());
            }

            char* jobindex = getenv(config.get(who + ".env_jobindex", "LSF_STAGE_JOBINDEX").c_str());
            if (jobindex == NULL)
            {
               jobindex = getenv(config.get(who + ".env_jobindex", "LSB_JOBINDEX").c_str());
            }

            if (jobid)
            {
                BBJOBID = stoull(jobid);
                if (jobindex)
                {
                    BBJOBID |= (stoull(jobindex)<<32);
                }
            }
            else
            {
                BBJOBID = stoul(DEFAULT_JOBID_STR);
            }
        }
        snprintf(myjobid, sizeof(myjobid), "%ld", BBJOBID);

        if (!config.get(who + ".noproxyinit", false))
        {
            rc = registerHandlers();
            if (rc)
            {
                errorText << "Unable to register handlers";
                LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
            }

            rc = setupConnections("bb.api", instance);
            if (rc)
            {
                errorText << "Unable to setup connections for bb.api";
                LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
            }

            rc = makeConnection(pContribId, ProcessId, DEFAULT_PROXY_ALIAS);
            if (rc)
            {
                errorText << "Unable to create bb.proxy connection";
                LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
            }

            rc = bbapi_SetVariable("jobid", myjobid);
            if (rc)
            {
                errorText << "bbProxy setVariable for jobid failed";
                LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
            }

            tmp = getenv(config.get(who + ".env_jobstepid", "PMIX_NAMESPACE").c_str());
            if (tmp == NULL)
            {
                tmp = DEFAULT_JOBSTEPID_STR.c_str();
            }

            rc = bbapi_SetVariable("jobstepid", tmp);
            if (rc)
            {
                errorText << "bbProxy setVariable for jobstepid failed";
                LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
            }

            tmp = getenv(config.get(who + ".env_contribid", "LSFCONTRIBID").c_str());
            if (pContribId == UNDEFINED_CONTRIBID)
            {
                if (tmp == NULL)
                {
                    tmp = DEFAULT_CONTRIBID_STR.c_str();
                }
                rc = bbapi_SetVariable("contribid", tmp);
            }
            else if (pContribId == NO_CONTRIBID)
            {
                if (tmp == NULL)
                {
                    tmp = NO_CONTRIBID_STR.c_str();
                }
                rc = bbapi_SetVariable("contribid", tmp);
            }
            else
            {
                rc = bbapi_SetVariable("contribid", to_string(pContribId));
            }

            if (rc)
            {
                errorText << "bbProxy setVariable for contribid failed";
                LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
            }
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
 | Version of BB_InitLibrary() that allows for an alternate configuration file and/or
 |   an alternate unix path for the unix socket to be used.
 | NOTE: Use is for test purposes only
 | NOTE: This routine is very similar to BB_InitLibrary() in bbapi.cc.
 |       The major differences, other than allowing for an input configuration
 |       file and/or a unix path for the unix socket to use, is that this routine
 |       does not check the static variable initLibraryDone (assumes the library
 |       has not been loaded), nor does it lock the mutex lockLibraryDone.
 |       \todo Need to look into these differences...  @DLH
 *******************************************************************************/
int Coral_InitLibrary(uint32_t pContribId, const char* pClientVersion, const char* pConfigfile, const char* pUnixpath)
{
    int rc = 0;
    stringstream errorText;

    // Initialize the C runtime...
    struct timeval l_CurrentTime = timeval {.tv_sec=0, .tv_usec=0};
    gettimeofday(&l_CurrentTime, NULL);

    try
    {
        // Verify connection does not yet exist
        rc = EBUSY;         // Connection exists if initialization has already been invoked
        verifyInit(false);  // Throws bailout if connection exists
        rc = 0;

        if(!curConfig.isLoaded())
        {
            if (pConfigfile && strlen(pConfigfile))
            {
                bberror << err("env.configfile", pConfigfile);
                if (!curConfig.load(pConfigfile))
                {
                    config = curConfig.getTree();
                }
                else
                {
                    rc = ENOENT;
                    errorText << "Error loading alternate configuration from " << pConfigfile;
                    cerr << errorText << endl;
                    bberror << err("error.configfile", pConfigfile);
                    LOG_ERROR_TEXT_ERRNO_AND_BAIL(errorText, rc);
                }
            }
            else
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
        }

        rc = BB_InitLibrary2(pContribId, pClientVersion, pUnixpath);

        if (rc) SET_RC_AND_BAIL(rc);
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
 | BB API Test APIs
 *******************************************************************************/

int Coral_GetVar(const char* pVariable)
{
    int rc = -1;
    int64_t l_Value = -1;
    ResponseDescriptor reply;

    bberror.clear();

    txp::Msg* msg = 0;

    try
    {
        // Verify initialization
        verifyInit(true);
        rc = 0;

        txp::Msg::buildMsg(txp::CORAL_GETVAR, msg);
        msg->addAttribute(txp::variable, pVariable, strlen(pVariable)+1);

        rc = sendMessage(ProcessId, msg, reply);
        delete msg;
        if (rc) SET_RC_AND_BAIL(rc);

        rc = waitReply(reply, msg);
        if (rc) SET_RC_AND_BAIL(rc);

        rc = bberror.merge(msg);

        if (!rc)
        {
            l_Value = ((txp::Attr_int64*)msg->retrieveAttrs()->at(txp::value64))->getData();
        }

        delete msg;
    }
    catch(ExceptionBailout& e) { }
    catch(exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

    return (rc == 0 ? (int)l_Value : INVALID_CORAL_GETVAR_VALUE);
}

int Coral_SetVar(const char* pVariable, const char* pValue)
{
    int rc = -1;

    bberror.clear();

    ResponseDescriptor reply;
    txp::Msg* msg = 0;

    try
    {
        // Verify initialization
        rc = ENODEV;
        verifyInit(true);
        rc = 0;

        txp::Msg::buildMsg(txp::CORAL_SETVAR, msg);
        msg->addAttribute(txp::variable, pVariable, strlen(pVariable)+1);
        msg->addAttribute(txp::value, pValue, strlen(pValue)+1);

        rc = sendMessage(ProcessId, msg, reply);
        delete msg;
        if (rc) SET_RC_AND_BAIL(rc);

        rc = waitReply(reply, msg);
        if (rc) SET_RC_AND_BAIL(rc);

        rc = bberror.merge(msg);
        delete msg;
    }
    catch(ExceptionBailout& e) { }
    catch(exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

    return (rc == 0 ? rc : INVALID_CORAL_SETVAR_VALUE);
}

int Coral_StageOutStart(const char* pMountpoint)
{
    int rc = 0;

    bberror.clear();

    ResponseDescriptor reply;
    txp::Msg* msg = 0;

    try
    {
        // Verify initialization
        rc = ENODEV;
        verifyInit(true);
        rc = 0;

        txp::Msg::buildMsg(txp::CORAL_STAGEOUT_START, msg);
        msg->addAttribute(txp::mountpoint, pMountpoint, strlen(pMountpoint)+1);

        rc = sendMessage(ProcessId, msg, reply);
        delete msg;
        if (rc) SET_RC_AND_BAIL(rc);

        rc = waitReply(reply, msg);
        if (rc) SET_RC_AND_BAIL(rc);

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
