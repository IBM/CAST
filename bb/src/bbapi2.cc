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
int BB_InitLibrary2(uint32_t contribId, const char* clientVersion, const char* unixpath, bool& performCleanup)
{
    int rc = 0;
    stringstream errorText;
    const char* tmp;

    try
    {

        if (unixpath && strlen(unixpath))
        {
            config.put("bb.unixpath", unixpath);
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

        std::string l_clientVersion = clientVersion;
        rc=versionCheck(l_clientVersion);

        char myjobid[256];

        if(BBJOBID == 0)
        {
            char* jobid                    = getenv(config.get(who + ".env_jobid",    "LSF_STAGE_JOBID").c_str());
            if(jobid == NULL)    jobid     = getenv(config.get(who + ".env_jobid",    "LSB_JOBID").c_str());

            char* jobindex                 = getenv(config.get(who + ".env_jobindex", "LSF_STAGE_JOBINDEX").c_str());
            if(jobindex == NULL)  jobindex = getenv(config.get(who + ".env_jobindex", "LSB_JOBINDEX").c_str());

            if(jobid)
            {
                BBJOBID = stoull(jobid);
                if(jobindex)
                    BBJOBID |= (stoull(jobindex)<<32);
            }
            else
            {
                BBJOBID = stoul(DEFAULT_JOBID_STR);
            }
        }
        snprintf(myjobid, sizeof(myjobid), "%ld", BBJOBID);

        if(config.get(who + ".noproxyinit", false))
        {
            rc = 0;
            bberror << bailout;
        }

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

        rc = makeConnection(contribId, ProcessId, DEFAULT_PROXY_ALIAS);
        if (rc)
        {
            errorText << "Unable to create bb.proxy connection";
            LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
        }

        rc = bbapi_SetVariable("jobid", myjobid);
        if(rc)
        {
            errorText << "bbProxy setVariable for jobid failed";
            LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
        }

        tmp = getenv(config.get(who + ".env_jobstepid", "PMIX_NAMESPACE").c_str());
        if(tmp == NULL) tmp = DEFAULT_JOBSTEPID_STR.c_str();
        rc = bbapi_SetVariable("jobstepid", tmp);
        if(rc)
        {
            errorText << "bbProxy setVariable for jobstepid failed";
            LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
        }

        tmp = getenv(config.get(who + ".env_contribid", "LSFCONTRIBID").c_str());
        if (contribId == UNDEFINED_CONTRIBID)
        {
            if(tmp == NULL) tmp = DEFAULT_CONTRIBID_STR.c_str();
            rc = bbapi_SetVariable("contribid", tmp);
        }
        else if (contribId == NO_CONTRIBID)
        {
            if(tmp == NULL) tmp = NO_CONTRIBID_STR.c_str();
            rc = bbapi_SetVariable("contribid", tmp);
        }
        else
        {
            rc = bbapi_SetVariable("contribid", to_string(contribId));
        }
        if(rc)
        {
            errorText << "bbProxy setVariable for contribid failed";
            LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
        }

        performCleanup = false;
    }
    catch(ExceptionBailout& e) { performCleanup = true; }
    catch(exception& e)
    {
        performCleanup = true;
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

    return rc;
}

void cleanupInit()
{

    return;
}


/*******************************************************************************
 | Version of BB_InitLibrary() that allows an alternate configuration file
 | NOTE:  Use is for test purposes only
 *******************************************************************************/
int Coral_InitLibrary(uint32_t contribId, const char* clientVersion, const char* configfile, const char* unixpath)
{
    int rc = 0;
    bool l_PerformCleanup = true;
    stringstream errorText;

    try
    {
        // Verify initialization has not yet been invoked
        rc = EBUSY;
        verifyInit(false);
        rc = 0;

        if(!curConfig.isLoaded())
        {
            if (configfile && strlen(configfile))
            {
                if (!curConfig.load(configfile))
                {
                    config = curConfig.getTree();
                }
                else
                {
                    rc = ENOENT;
                    errorText << "Error loading alternate configuration from " << configfile;
                    cerr << errorText << endl;
                    bberror << err("error.configfile", configfile);
                    LOG_ERROR_TEXT_ERRNO_AND_BAIL(errorText, rc);
                }
            }
            else
            {
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

        rc = BB_InitLibrary2(contribId, clientVersion, unixpath, l_PerformCleanup);

        if (rc) bberror << bailout;
    }
    catch(ExceptionBailout& e) { l_PerformCleanup = true; }
    catch(exception& e)
    {
        l_PerformCleanup = true;
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

    if (l_PerformCleanup)
    {
        cleanupInit();
    }

    return rc;
}


/*******************************************************************************
 | BB API Test APIs
 *******************************************************************************/

int Coral_GetVar(const char* pVariable)
{
    int rc = 0;
    ResponseDescriptor reply;

    bberror.clear();

    txp::Msg* msg = 0;
    int64_t l_Value = -1;

    try
    {
        // Verify initialization
        rc = ENODEV;
        verifyInit(true);
        rc = 0;

        txp::Msg::buildMsg(txp::CORAL_GETVAR, msg);
        msg->addAttribute(txp::variable, pVariable, strlen(pVariable)+1);

        rc = sendMessage(ProcessId, msg, reply);
        delete msg;
        if (rc) LOG_RC_AND_BAIL(rc);

        rc = waitReply(reply, msg);
        if (rc) LOG_RC_AND_BAIL(rc);

        rc = bberror.merge(msg);

        if (!rc)
            l_Value = ((txp::Attr_int64*)msg->retrieveAttrs()->at(txp::value64))->getData();

        delete msg;
    }
    catch(ExceptionBailout& e) { }
    catch(exception& e)
    {
        rc = -1;
        l_Value = rc;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

    return l_Value;
}

int Coral_SetVar(const char* pVariable, const char* pValue)
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

        txp::Msg::buildMsg(txp::CORAL_SETVAR, msg);
        msg->addAttribute(txp::variable, pVariable, strlen(pVariable)+1);
        msg->addAttribute(txp::value, pValue, strlen(pValue)+1);

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
        if(rc) bberror << errloc(rc) <<bailout;

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
