/*******************************************************************************
 |    bbserver.cc
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

#include <semaphore.h>
#include <stdio.h>
#include <unistd.h>

#include <iostream>
#include <string>

#include <sys/resource.h>
#include <sys/socket.h>
#include "bbapi_version.h"

#include "connections.h"
extern void setThresholdTimes4Syscall(const uint64_t pWarnSeconds, const uint64_t pStuckSeconds);

using namespace std;

#ifdef PROF_TIMING
#include <ctime>
#include <chrono>
#endif


#include "bbinternal.h"
#include "BBLV_Info.h"
#include "BBLV_Metadata.h"
#include "bbserver_flightlog.h"
#include "BBTagID.h"
#include "BBTransferDef.h"
#include "bbwrkqmgr.h"
#include "CnxSock.h"
#include "ContribIdFile.h"
#include "identity.h"
#include "HandleFile.h"
#include "LVLookup.h"
#include "Msg.h"
#include "Uuid.h"
#include "xfer.h"
#include "usage.h"

namespace po = boost::program_options;

// Define flightlog for bbServer metadata
FL_SetName(FLMetaData, "bbServer MetaData Flightlog")
FL_SetSize(FLMetaData, 65536)

// Metadata that is kept on each bbserver...
BBLV_Metadata metadata;
WRKQMGR wrkqmgr;

// Timer used to for resize SSD messages sent to bbproxy
Timer ResizeSSD_Timer;
double ResizeSSD_TimeInterval;

// Timer used for throttle intervals
Timer Throttle_Timer;
double Throttle_TimeInterval;

// High priority work queue variables
LVKey HPWrkQE_LVKeyStg;
const LVKey* HPWrkQE_LVKey = &HPWrkQE_LVKeyStg;
WRKQE* HPWrkQE;

// Metadata counter for flight logging
AtomicCounter metadataCounter;


//*****************************************************************************
//  Support routines
//*****************************************************************************

int hasContribId(const uint32_t pContribId, const uint64_t pNumOfContribsInArray, uint32_t* pContribArray)
{
    int rc = 0;

    for (uint64_t i=0; i<pNumOfContribsInArray; ++i) {
        if (pContribId == pContribArray[i])
        {
            rc = 1;
            break;
        }
    }

    return rc;
}

void processContrib(std::vector<uint32_t>* pContrib, uint32_t* &pContribArray, stringstream &pContribStr)
{
    pContribStr.clear();
    pContribStr.str(string());

    uint64_t l_NumContrib = (uint64_t)pContrib->size();
    pContribArray = (uint32_t*)(new char[l_NumContrib*sizeof(uint32_t)]);
    for (uint64_t i=0; i<l_NumContrib; ++i)
    {
        pContribArray[i] = (*pContrib)[i];
    }

    uint32_t* l_ContribPtr = pContribArray;
    pContribStr << "(";
    for(uint64_t i=0; i<l_NumContrib; ++i)
    {
        if (i!=l_NumContrib-1)
        {
            pContribStr << *l_ContribPtr << ",";
        }
        else
        {
            pContribStr << *l_ContribPtr;
        }
        ++l_ContribPtr;
    }
    pContribStr << ")";

    return;
}

void switchIds(txp::Msg* pMsg)
{
    const uid_t l_Owner = (uid_t)((txp::Attr_uint32*)pMsg->retrieveAttrs()->at(txp::uid))->getData();
    const gid_t l_Group = (gid_t)((txp::Attr_uint32*)pMsg->retrieveAttrs()->at(txp::gid))->getData();

    int rc = becomeUser(l_Owner, l_Group);
    if (!rc)
    {
        bberror << err("in.misc.uid", l_Owner) << err("in.misc.gid", l_Group);
    }
    else
    {
        stringstream errorText;
        errorText << "becomeUser failed";
        bberror << err("error.uid", l_Owner) << err("error.gid", l_Group);
        LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
    }

    return;
}


//*****************************************************************************
//  Requests from bbproxy
//*****************************************************************************

#define DELAY_SECONDS 120
void msgin_canceltransfer(txp::Id id, const std::string& pConnectionName,  txp::Msg* msg)
{
    ENTRY(__FILE__,__FUNCTION__);
    int rc = 0;

    bberror.clear(pConnectionName);

    stringstream errorText;

    LVKey l_LVKeyStg;
    LVKey* l_LVKey = &l_LVKeyStg;
    bool l_LockHeld = false;

    FL_Write(FLServer, Msg_CancelFromProxy, "bbCancelTransfer command received",0,0,0,0);
    try
    {
        // Demarshall data from the message
        uint64_t l_FromJobId = ((txp::Attr_uint64*)msg->retrieveAttrs()->at(txp::jobid))->getData();
        uint64_t l_FromJobStepId = ((txp::Attr_uint64*)msg->retrieveAttrs()->at(txp::jobstepid))->getData();
        uint32_t l_ContribId = ((txp::Attr_uint32*)msg->retrieveAttrs()->at(txp::contribid))->getData();
        uint64_t l_Handle = ((txp::Attr_uint64*)msg->retrieveAttrs()->at(txp::handle))->getData();
        BBCANCELSCOPE l_CancelScope = (BBCANCELSCOPE)((txp::Attr_uint64*)msg->retrieveAttrs()->at(txp::flags))->getData();
        char l_Temp[64] = {'\0'};
        getStrFromCancelScope(l_CancelScope, l_Temp, sizeof(l_Temp));

        LOG(bb,info) << "msgin_canceltransfer: From JobId = " << l_FromJobId << ", From JobStepId = " \
                     << l_FromJobStepId << ", From ContribId = " << l_ContribId << ", Handle = " << l_Handle \
                     << std::hex << std::uppercase << setfill('0') \
                     << ", cancel scope = " << l_Temp \
                     << setfill(' ') << std::nouppercase << std::dec;

        switchIds(msg);

        lockLocalMetadata((LVKey*)0, "msgin_canceltransfer");
        l_LockHeld = true;

        {
            // Process cancel transfer message
            switch (l_CancelScope)
            {
                case BBSCOPETRANSFER:
                {
                    //  NOTE:  We wait up to 2 minutes for the necessary LVKey to appear if we can't find
                    //         it right away.  This closes the window during activate server between the activation
                    //         of the connection to the new server and the registering of any LVKeys with the new server.
                    int l_Continue = DELAY_SECONDS;
                    while ((!rc) && (l_Continue--))
                    {
                        rc = metadata.getLVKey(pConnectionName, l_LVKey, l_FromJobId, l_ContribId);
                        switch (rc)
                        {
                            case -2:
                            {
                                if (l_Continue)
                                {
                                    rc = 0;
#ifndef __clang_analyzer__  // l_LockHeld is never read, but keep it for debug
                                    l_LockHeld = false;
#endif
                                    unlockLocalMetadata((LVKey*)0, "msgin_canceltransfer - Waiting for LVKey to be registered");
                                    {
                                        int l_SecondsWaiting = DELAY_SECONDS - l_Continue;
                                        if ((l_SecondsWaiting % 15) == 1)
                                        {
                                            // Display this message every 15 seconds...
                                            FL_Write6(FLDelay, CancelWaitForLVKey, "Attempting to cancel a transfer definition for jobid %ld, jobstepid %ld, handle %ld, contribid %ld. Delay of 1 second before retry. %ld seconds remain waiting for the LVKey.",
                                                      l_FromJobId, l_FromJobStepId, l_Handle, (uint64_t)l_ContribId, (uint64_t)l_Continue, 0);
                                            LOG(bb,info) << ">>>>> DELAY <<<<< msgin_canceltransfer: Attempting to cancel a transfer definition for jobid " << l_FromJobId \
                                                         << ", jobstepid " << l_FromJobStepId << ", handle " << l_Handle << ", contribid " << l_ContribId \
                                                         << ". Delay of 1 second before retry. " << l_Continue << " seconds remain waiting for the LVKey.";
                                        }
                                        usleep((useconds_t)1000000);    // Delay 1 second
                                    }
                                    lockLocalMetadata((LVKey*)0, "msgin_canceltransfer - Waiting for LVKey to be registered");
                                    l_LockHeld = true;
                                }
                                else
                                {
                                    // Cannot find an LVKey registered for this jobid
                                    errorText << "A logical volume (LVKey) is not currently associated with jobid " << l_FromJobId << ", contribid " << l_ContribId \
                                              << " on this bbServer. A cancel request for an individual transfer definition must be directed to the bbServer servicing that jobid and contribid.";
                                    rc = -1;
                                    LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
                                }
                                break;
                            }

                            case 1:
                            {
                                rc = 0;
                                l_Continue = 0;
                                break;
                            }

                            default:
                            {
                                errorText << "Error when attempting to find a local LVKey for jobid " << l_FromJobId \
                                          << ", contribid " << l_ContribId;
                                LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
                            }
                        }
                    }

                    //  NOTE:  We wait up to 2 minutes for the necessary BBLV_Info object to appear if we can't find
                    //         it right away.  This closes the window during activate server between the activation
                    //         of the connection to the new server and the restarting of any transfer definitions to
                    //         the new server.
                    l_Continue = DELAY_SECONDS;
                    BBLV_Info* l_LV_Info = 0;
                    while ((!l_LV_Info) && l_Continue--)
                    {
                        l_LV_Info = metadata.getLV_Info(l_LVKey);
                        if (!l_LV_Info)
                        {
                            unlockLocalMetadata(l_LVKey, "msgin_canceltransfer - Waiting for BBLV_Info to be registered");
                            {
                                int l_SecondsWaiting = DELAY_SECONDS - l_Continue;
                                if ((l_SecondsWaiting % 15) == 1)
                                {
                                    // Display this message every 15 seconds...
                                    FL_Write6(FLDelay, CancelWaitForBBLV_Info, "Attempting to cancel a transfer definition for jobid %ld, jobstepid %ld, handle %ld, contribid %ld. Delay of 1 second before retry. %ld seconds remain waiting for the BBLV_Info.",
                                              l_FromJobId, l_FromJobStepId, l_Handle, (uint64_t)l_ContribId, (uint64_t)l_Continue, 0);
                                    LOG(bb,info) << ">>>>> DELAY <<<<< msgin_canceltransfer: Attempting to cancel a transfer definition for jobid " << l_FromJobId \
                                                 << ", jobstepid " << l_FromJobStepId << ", handle " << l_Handle << ", contribid " << l_ContribId \
                                                 << ". Delay of 1 second before retry. " << l_Continue << " seconds remain waiting for the BBLV_Info.";
                                }
                                usleep((useconds_t)1000000);    // Delay 1 second
                            }
                            lockLocalMetadata(l_LVKey, "BBLV_Metadata::getLVKey - Waiting for BBLV_Info to be registered");

                            // Check to make sure the job still exists after releasing/re-acquiring the lock
                            if (!jobStillExists(pConnectionName, l_LVKey, (BBLV_Info*)0, (BBTagInfo*)0, l_FromJobId, l_ContribId))
                            {
                                rc = -1;
                                l_Continue = 0;
                            }
                        }
                    }
                    if (!l_LV_Info)
                    {
                        errorText << "Could not resolve to BBLV_Info for " << *l_LVKey << ", jobid " << l_FromJobId << ", contribid " << l_ContribId;
                        rc = -1;
                        LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
                    }

                    //  NOTE:  We wait up to 2 minutes for the necessary BBTagInfo object to appear if we can't find
                    //         it right away.  This closes the window during activate server between the activation
                    //         of the connection to the new server and the restarting of any transfer definitions to
                    //         the new server.
                    l_Continue = DELAY_SECONDS;
                    BBTagInfo* l_TagInfo = 0;
                    BBTagID l_TagId;
                    while ((!l_TagInfo) && l_Continue--)
                    {
                        if (!l_LV_Info->getTagInfo(l_Handle, l_ContribId, l_TagId, l_TagInfo))
                        {
                            unlockLocalMetadata(l_LVKey, "msgin_canceltransfer - Waiting for BBTagInfo to be registered");
                            {
                                int l_SecondsWaiting = DELAY_SECONDS - l_Continue;
                                if ((l_SecondsWaiting % 15) == 1)
                                {
                                    // Display this message every 15 seconds...
                                    FL_Write6(FLDelay, CancelWaitForBBTagInfo, "Attempting to cancel a transfer definition for jobid %ld, jobstepid %ld, handle %ld, contribid %ld. Delay of 1 second before retry. %ld seconds remain waiting for the BBTagInfo.",
                                              l_FromJobId, l_FromJobStepId, l_Handle, (uint64_t)l_ContribId, (uint64_t)l_Continue, 0);
                                    LOG(bb,info) << ">>>>> DELAY <<<<< msgin_canceltransfer: Attempting to cancel a transfer definition for jobid " << l_FromJobId \
                                                 << ", jobstepid " << l_FromJobStepId << ", handle " << l_Handle << ", contribid " << l_ContribId \
                                                 << ". Delay of 1 second before retry. " << l_Continue << " seconds remain waiting for the BBTagInfo.";
                                }
                                usleep((useconds_t)1000000);    // Delay 1 second
                            }
                            lockLocalMetadata(l_LVKey, "BBLV_Metadata::getLVKey - Waiting for BBTagInfo to be registered");

                            // Check to make sure the job still exists after releasing/re-acquiring the lock
                            if (!jobStillExists(pConnectionName, l_LVKey, l_LV_Info, (BBTagInfo*)0, l_FromJobId, l_ContribId))
                            {
                                rc = -1;
                                l_Continue = 0;
                            }
                        }
                    }
                    if (!l_TagInfo)
                    {
                        errorText << "Could not resolve to BBTagInfo for " << *l_LVKey << ", jobid " << l_FromJobId << ", contribid " << l_ContribId;
                        rc = -1;
                        LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
                    }

                    if (l_LV_Info->getExtentInfo()->moreExtentsToTransfer((int64_t)l_Handle, (int32_t)l_ContribId, 0))
                    {
                        if (!l_TagInfo->allExtentsTransferred())
                        {
                            // Cancel the transfer for the transfer definition
                            // NOTE:  The jobid and jobstepid are not needed to be passed as
                            //        those values are determined from the transfer definition.
                            l_TagInfo->setCanceled(l_LVKey, l_Handle, l_ContribId);

                            // Sort the extents, moving the canceled extents to the front of
                            // the work queue so they are immediately removed...
                            LOCAL_METADATA_RELEASED l_LockWasReleased = LOCAL_METADATA_LOCK_NOT_RELEASED;
                            l_LV_Info->cancelExtents(l_LVKey, &l_Handle, &l_ContribId, l_LockWasReleased, REMOVE_TARGET_PFS_FILES);
                        }
                        else
                        {
                            rc = -2;
                            errorText << "A cancel request was made for the transfer definition associated with " << *l_LVKey << ", handle " << l_Handle \
                                      << ", contribid " << l_ContribId << ". However no extents are left to be transferred (via TagInfo). Cancel request ignored.";
                            LOG_INFO_TEXT_RC_AND_BAIL(errorText, rc);
                        }
                    }
                    else
                    {
                        rc = -2;
                        errorText << "A cancel request was made for the transfer definition associated with " << *l_LVKey << ", handle " << l_Handle \
                                  << ", contribid " << l_ContribId << ". However no extents are left to be transferred (via ExtentInfo). Cancel request ignored.";
                        LOG_INFO_TEXT_RC_AND_BAIL(errorText, rc);
                    }
                }

                break;

                case BBSCOPETAG:
                {
                    // Retrieve the real jobid, jobstepid for the handle
                    uint64_t l_JobId = UNDEFINED_JOBID;
                    uint64_t l_JobStepId = UNDEFINED_JOBSTEPID;
                    rc = HandleFile::get_xbbServerGetJobForHandle(l_JobId, l_JobStepId, l_Handle);
                    if (!rc)
                    {
                        BBSTATUS l_Status;
                        rc = HandleFile::get_xbbServerHandleStatus(l_Status, (LVKey*)0, l_JobId, l_JobStepId, l_Handle);
                        if (!rc)
                        {
                            switch (l_Status)
                            {
                                case BBFULLSUCCESS:
                                {
                                    // If the handle status is BBFULLSUCCESS, do not allow the cancel
                                    rc = -2;
                                    errorText << "A cancel request was made for handle " << l_Handle \
                                              << ". However, the handle currently has a status of BBFULLSUCCESS. Cancel request ignored.";
                                    LOG_INFO_TEXT_RC_AND_BAIL(errorText, rc);
                                }
                                break;

                                default:
                                {
                                    //  Cancel the transfer for the entire handle
                                    string l_HostName;
                                    activecontroller->gethostname(l_HostName);
                                    rc = cancelTransferForHandle(l_HostName, l_JobId, l_JobStepId, l_Handle, REMOVE_TARGET_PFS_FILES);
                                }
                                break;
                            }
                        }
                        else
                        {
                            errorText << "Could not determine the handle status for job " << l_JobId << ", jobstepid " << l_JobStepId << ", handle " << l_Handle \
                                      << ". Request to cancel a transfer with a cancel scope of BBSCOPETAG failed.";
                            LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
                        }
                    }
                    else
                    {
                        errorText << "Could not determine the jobid for handle " << l_Handle \
                                  << ". Request to cancel a transfer with a cancel scope of BBSCOPETAG failed.";
                        LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
                    }
                }

                break;

                default:
                {
                    rc = -1;
                    errorText << "Undefined cancel scope value of " << l_CancelScope;
                    LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
                }
            }
        }
    }
    catch (ExceptionBailout& e) { }
    catch (exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

    if (l_LockHeld)
    {
        unlockLocalMetadata(l_LVKey, "msgin_canceltransfer");
    }

    // Build the response message
    txp::Msg* response;
    msg->buildResponseMsg(response);

    addReply(msg, response);

    CurrentWrkQE = (WRKQE*)0;

    RESPONSE_AND_EXIT(__FILE__,__FUNCTION__);

    return;
}
#undef DELAY_SECONDS

void msgin_stageout_start(txp::Id id, const std::string& pConnectionName, txp::Msg* msg)
{
    ENTRY(__FILE__,__FUNCTION__);
    int rc = 0;

    bberror.clear(pConnectionName);

    try
    {
        // Demarshall data from the message
        Uuid l_lvuuid = Uuid((char*)(msg->retrieveAttrs()->at(txp::uuid)->getDataPtr()));
        LVKey l_LVKey = std::make_pair(pConnectionName, l_lvuuid);
        LOG(bb,debug) << "msgin_stageout_started: Local Port = " << getConnex(pConnectionName)->getSockfd() << ", LV Uuid = " << (char*)(msg->retrieveAttrs()->at(txp::uuid)->getDataPtr());

        switchIds(msg);

        // Process stageout start message
        // NOTE: If stageoutStart() fails, it will fill in errstate...
        rc = stageoutStart(pConnectionName, &l_LVKey);
    }
    catch (ExceptionBailout& e) { }
    catch (exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

    // Build the response message
    txp::Msg* response;
    msg->buildResponseMsg(response);

    addReply(msg, response);

    RESPONSE_AND_EXIT(__FILE__,__FUNCTION__);
    return;
}


void msgin_createlogicalvolume(txp::Id id, const std::string& pConnectionName, txp::Msg* msg)
{
    ENTRY(__FILE__,__FUNCTION__);
    int rc = 0;
    stringstream errorText;

    bberror.clear(pConnectionName);

    // Demarshall data from the message
    Uuid l_lvuuid = Uuid((char*)(msg->retrieveAttrs()->at(txp::uuid)->getDataPtr()));
    LVKey l_LVKey = std::make_pair(pConnectionName, l_lvuuid);
    LVKey* l_LVKeyPtr = &l_LVKey;
    string l_HostName = (const char*)msg->retrieveAttrs()->at(txp::hostname)->getDataPtr();
    uint64_t l_JobId = ((txp::Attr_uint64*)msg->retrieveAttrs()->at(txp::jobid))->getData();
    //  NOTE: A non-zero option indicates a registration of an LVKey due to a new bbServer being activated
    uint32_t l_Option = ((txp::Attr_uint32*)msg->retrieveAttrs()->at(txp::option))->getData();

    // Log the input/results
    Uuid lv_uuid = l_LVKey.second;
    char lv_uuid_str[LENGTH_UUID_STR] = {'\0'};
    lv_uuid.copyTo(lv_uuid_str);
    if (!l_Option)
    {
        LOG(bb,info) << "msgin_createlogicalvolume: From hostname " << l_HostName << ", l_LVKey.first= " << l_LVKey.first << ", LV Uuid = " << lv_uuid_str << ", jobid = " << l_JobId;
    }
    else
    {
        LOG(bb,info) << "msgin_createlogicalvolume: Register LVKey, from hostname " << l_HostName << ", l_LVKey.first= " << l_LVKey.first << ", LV Uuid = " << lv_uuid_str << ", jobid = " << l_JobId;
    }

    lockLocalMetadata(l_LVKeyPtr, "msgin_createlogicalvolume");

    try
    {
        // NOTE:  Need to first process all outstanding async requests.  In the restart scenarios, we must make sure
        //        that all prior suspend/resume requests have first been processed by this bbServer.
        wrkqmgr.processAllOutstandingHP_Requests(l_LVKeyPtr);

        // Insert the LVKey with jobid into the metadata
        // NOTE: We switch to the uid/gid of the mount point, as those ids should 'own' the xbbbserver metadata entries...
        switchIdsToMountPoint(msg);

        // NOTE: If needed, addLogicalVolume() fills in errstate...
        rc = addLogicalVolume(pConnectionName, l_HostName, msg, l_LVKeyPtr, l_JobId, (TOLERATE_ALREADY_EXISTS_OPTION)l_Option);
        if (rc > 0)
        {
            // Positive rc...
            // Send -2 back to bbProxy as a possible tolerated exception...
            // NOTE:  addLogicalVolume() filled in the errstate, except for rc...
            rc = -2;
            SET_RC(rc);
        }
    }
    catch (ExceptionBailout& e) { }
    catch (exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

    unlockLocalMetadata(l_LVKeyPtr, "msgin_createlogicalvolume");

    // Build the response message
    txp::Msg* response;
    msg->buildResponseMsg(response);

    if (rc)
    {
        bberror << err("error.jobid", l_JobId) << err("error.uuid",lv_uuid_str);
    }

    addReply(msg, response);

    EXIT_NO_CLOCK(__FILE__,__FUNCTION__);

    addBBErrorToMsg(response);

    // Send the response
    sendMessage(pConnectionName,response);
    delete response;

#ifdef PROF_TIMING
    std::chrono::high_resolution_clock::time_point time_stop = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double,std::micro> elapsed_microseconds = time_stop - time_start;
    LOG(bb,trace) << __FILE__ <<":"<< __LINE__ << " " << __FUNCTION__ << "() completed after: " << elapsed_microseconds.count() << " microseconds";
#endif
    return;
}


void msgin_getthrottlerate(txp::Id id, const std::string& pConnectionName, txp::Msg* msg)
{
    ENTRY(__FILE__,__FUNCTION__);
    int rc = 0;

    bberror.clear(pConnectionName);

    uint64_t l_Rate = 0;
    try
    {
        // Demarshall data from the message
        Uuid l_lvuuid = Uuid((char*)(msg->retrieveAttrs()->at(txp::uuid)->getDataPtr()));
        LVKey l_LVKey = std::make_pair(pConnectionName, l_lvuuid);

        switchIds(msg);

        // Process get throttle rate message
        rc = getThrottleRate(pConnectionName, &l_LVKey, l_Rate);

        if (rc) {
            SET_RC_AND_BAIL(rc);
        }

        LOG(bb,info) << "msgin_getthrottlerate: Local Connection Name " << pConnectionName << ", LV Uuid " \
                     << (char*)(msg->retrieveAttrs()->at(txp::uuid)->getDataPtr()) << ", rate " << l_Rate;
    }
    catch (ExceptionBailout& e) { }
    catch (exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

    // Build the response message
    txp::Msg* response;
    msg->buildResponseMsg(response);

    addReply(msg, response);

    EXIT_NO_CLOCK(__FILE__,__FUNCTION__);

    if (!rc)
    {
        response->addAttribute(txp::rate, l_Rate);
    }
    addBBErrorToMsg(response);

    // Send the response
    sendMessage(pConnectionName,response);
    delete response;

#ifdef PROF_TIMING
    std::chrono::high_resolution_clock::time_point time_stop = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double,std::micro> elapsed_microseconds = time_stop - time_start;
    LOG(bb,trace) << __FILE__ <<":"<< __LINE__ << " " << __FUNCTION__ << "() completed after: " << elapsed_microseconds.count() << " microseconds";
#endif
    return;
}

#define DELAY_SECONDS 120
void msgin_gettransferhandle(txp::Id id, const std::string& pConnectionName, txp::Msg* msg)
{
    ENTRY(__FILE__,__FUNCTION__);
    int rc = -2;
    stringstream errorText;

    bberror.clear(pConnectionName);

    uint64_t l_Handle = UNDEFINED_HANDLE;
    LVKey l_LVKey;
    LVKey* l_LVKeyPtr = &l_LVKey;
    char lv_uuid_str[LENGTH_UUID_STR] = {'\0'};

    try
    {
        // Demarshall data from the message
        string l_HostName = (const char*)msg->retrieveAttrs()->at(txp::hostname)->getDataPtr();
        BBJob l_Job(((txp::Attr_uint64*)msg->retrieveAttrs()->at(txp::jobid))->getData(), ((txp::Attr_uint64*)msg->retrieveAttrs()->at(txp::jobstepid))->getData());
        uint64_t l_Tag = ((txp::Attr_uint64*)msg->retrieveAttrs()->at(txp::tag))->getData();
        // NOTE:  bbproxy ensures that l_NumContrib does not come in as a zero...
        uint64_t l_NumContrib = ((txp::Attr_uint64*)msg->retrieveAttrs()->at(txp::numcontrib))->getData();
        uint32_t l_Contrib[l_NumContrib];
        uint32_t* l_Contribs = (uint32_t*)msg->retrieveAttrs()->at(txp::contrib)->getDataPtr();

        uint32_t* l_ContribValue = l_Contribs;
        for(uint64_t x=0; x<l_NumContrib; ++x)
        {
            l_Contrib[x] = *l_ContribValue;
            ++l_ContribValue;
        }

        stringstream l_Temp;
        uint32_t* l_ContribPtr = l_Contrib;
        l_Temp << "(";
        for(uint64_t i=0; i<l_NumContrib; ++i)
        {
            if (i!=l_NumContrib-1)
            {
                l_Temp << *l_ContribPtr << ",";
            }
            else
            {
                l_Temp << *l_ContribPtr;
            }
            ++l_ContribPtr;
        }
        l_Temp << ")";

        stringstream l_JobStr;
        l_Job.getStr(l_JobStr);
        LOG(bb,info) << "msgin_gettransferhandle: job" << l_JobStr.str() << ", tag " << l_Tag << ", numcontrib " << l_NumContrib << ", contrib " << l_Temp.str();

        switchIds(msg);

        //  NOTE:  We set up to wait 2 minutes for the necessary LVKey to appear if we can't find
        //         it right away and the handle is not in the cross-bbServer metadata.
        //         This closes the window during activate server between the activation
        //         of the connection to the new server and the registering of any LVKeys
        //         with the new server.
        int l_Continue = DELAY_SECONDS;
        while ((rc) && (l_Continue--))
        {
            rc = getHandle(pConnectionName, l_LVKeyPtr, l_Job, l_Tag, l_NumContrib, l_Contrib, l_Handle);
            if (rc >= 0)
            {
                // rc=0 indicates that an existing LVKey was not found and a new one was added
                // rc=1 indicates that an existing LVKey was found
                string l_Text = (rc == 0 ? "created new into" : "found and reused from");
                rc = 0;
                l_Continue = 0;
                // Log the input/results
                Uuid lv_uuid = l_LVKey.second;
                lv_uuid.copyTo(lv_uuid_str);
                LOG(bb,info) << "getHandle: hostname " << l_HostName << ", " << l_LVKey << ", job" << l_JobStr.str() \
                             << ", tag " << l_Tag << ", numcontrib " << l_NumContrib << ", contrib " << l_Temp.str() \
                             << " -> handle " << l_Handle << ". Handle value was " << l_Text << " the local cache.";
            }
            else
            {
                // Negative return codes indicate an error...
                if (rc == -2)
                {
                    // This is a request for an existing handle to a new bbServer
                    // from a bbProxy that does not have an LVKey associated with
                    // the job.  This is possible if the logical volume was created
                    // when the CN was being serviced by a different bbServer and is
                    // now being serviced by a new bbServer.
                    //
                    // We cannot add the LVKey for the logical volume to the local metadata.
                    // The necessary LVKey should be registered to this new bbServer as part
                    // of activating the connection.
                    //
                    // Continue to wait...

                    // Hang out for a bit (120 x 1 second each, 2 minutes total) and see if the necessary LVKey appears...
                    int l_SecondsWaiting = wrkqmgr.getDeclareServerDeadCount() - l_Continue;
                    if ((l_SecondsWaiting % 15) == 1)
                    {
                        // Display this message every 15 seconds...
                        FL_Write(FLDelay, GetHandleWaitForLVKey, "Attempting to get the transfer handle for jobid %ld, jobstepid %ld. Delay of 1 second before retry. %ld seconds remain waiting for the LVKey.",
                                 l_Job.getJobId(), l_Job.getJobStepId(), (uint64_t)l_Continue, 0);
                        LOG(bb,info) << ">>>>> DELAY <<<<< msgin_gettransferhandle: Hostname " << l_HostName << ", " << l_LVKey \
                                     << " attempting to get the transfer handle for jobid " << l_Job.getJobId() << ", jobstepid " << l_Job.getJobStepId() \
                                     << ". Delay of 1 second before retry. " << l_Continue << " seconds remain waiting for the LVKey.";
                    }
                    usleep((useconds_t)1000000);    // Delay 1 second
                }
                else
                {
                    l_Continue = 0;
                    // NOTE: errstate already filled in by getHandle() for non-tolerated exceptions...
                }
            }
        }

        switch (rc)
        {
            case 0:
            {
                // Successful processing above...
            }
            break;

            case -2:
            {
                // LVKey not found on this bbServer
                // Send the error back as a -2...  Might be tolerated...
                errorText << "A logical volume (LVKey) is not currently associated with job" << l_JobStr.str() \
                          << " on this bbServer. Before a handle value can be generated," \
                          << " a logical volume must first be created for this job, associated with this bbServer.";
                LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
            }
            break;

            default:
            {
                // LVKey not found on this bbServer, error encountered by getHandle()
                // NOTE: errstate already filled in by gethandle() for non-tolerated exceptions...
                rc = -1;
            }
            break;
        }
    }
    catch (ExceptionBailout& e) { }
    catch (exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

    // Build the response message
    txp::Msg* response;
    msg->buildResponseMsg(response);

    addReply(msg, response);

    EXIT_NO_CLOCK(__FILE__,__FUNCTION__);

    if (rc == 0)
    {
        // NOTE:  The char array is copied to heap by addAttribute and the storage for
        //        the logical volume uuid attribute is owned by the message facility.
        //        Our copy can then go out of scope...
        response->addAttribute(txp::uuid, lv_uuid_str, sizeof(lv_uuid_str), txp::COPY_TO_HEAP);
        txp::Attr_uint64 transferHandle(txp::transferHandle, l_Handle);
        response->addAttribute(&transferHandle);
    }
    addBBErrorToMsg(response);

    // Send the response
    sendMessage(pConnectionName,response);
    delete response;

    CurrentWrkQE = (WRKQE*)0;

#ifdef PROF_TIMING
    std::chrono::high_resolution_clock::time_point time_stop = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double,std::micro> elapsed_microseconds = time_stop - time_start;
    LOG(bb,trace) << __FILE__ <<":"<< __LINE__ << " " << __FUNCTION__ << "() completed after: " << elapsed_microseconds.count() << " microseconds";
#endif
    return;
}
#undef DELAY_SECONDS

void msgin_gettransferinfo(txp::Id id, const std::string& pConnectionName, txp::Msg* msg)
{
    ENTRY(__FILE__,__FUNCTION__);
    int rc = 0;
    stringstream errorText;

    bberror.clear(pConnectionName);

    uint64_t l_Handle = UNDEFINED_HANDLE;
    uint32_t l_ContribId;

    uint64_t l_JobId = UNDEFINED_JOBID;
    uint64_t l_JobStepId = UNDEFINED_JOBSTEPID;
    uint64_t l_Tag = 0;
    uint64_t l_NumContrib = 0;
    uint64_t l_NumOfContribsInArray = 0;
    uint64_t l_NumberOfReportingContribs = 0;
    HandleFile* l_HandleFile = 0;
    ContribIdFile* l_ContribIdFile = 0;
    uint32_t* l_ContribArray = 0;

//    LVKey l_LVKeyStg;
//    LVKey* l_LVKey = &l_LVKeyStg;
//    char lv_uuid_str[LENGTH_UUID_STR] = {'\0'};

    uint64_t l_LengthOfTransferKeys = 0;
    uint64_t l_TransferKeyBufferSize = 15;
    char l_TransferKeyBuffer[16] = {'\0'};
//    bool l_LockHeld = false;

    try
    {
        // Demarshall data from the message
        l_Handle = ((txp::Attr_uint64*)msg->retrieveAttrs()->at(txp::handle))->getData();
        l_ContribId = ((txp::Attr_uint32*)msg->retrieveAttrs()->at(txp::contribid))->getData();
        LOG(bb,debug) << "msgin_gettransferinfo: handle = " << l_Handle << ", contribid = " << l_ContribId;

        switchIds(msg);

//        lockLocalMetadata((LVKey*)0, "msgin_gettransferinfo");
//        l_LockHeld = true;

        {
            if (l_Handle)
            {
                // NOTE: get_xbbServerHandleInfo() will only return a non-zero return code if the xbbServer data store cannot be found/loaded.
                //       Otherwise, the l_HandleFile and l_ContribIdFile pointers can come back null if they could not be found with the given
                //       input.
                rc = HandleFile::get_xbbServerHandleInfo(l_JobId, l_JobStepId, l_NumberOfReportingContribs, l_HandleFile, l_ContribIdFile, l_Handle, l_ContribId);

                if (!rc)
                {
                    if (l_HandleFile)
                    {
                        l_Tag = l_HandleFile->tag;
                        l_NumContrib = l_HandleFile->numContrib;
                        l_HandleFile->getContribArray(l_NumOfContribsInArray, l_ContribArray);

                        if (l_NumOfContribsInArray != l_NumContrib) {
                            rc = -1;
                            errorText << "Number of expected contributors, " << l_NumContrib << ", does not match the number of elements found in the expectContrib array, " << l_NumOfContribsInArray;
                            LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
                        }

                        // NOTE: No need to check the return code... The required size (without the training null terminator) is always returned...
                        HandleFile::getTransferKeys(l_JobId, l_Handle, l_LengthOfTransferKeys, l_TransferKeyBufferSize, l_TransferKeyBuffer);
                    }
                    else
                    {
                        rc = -2;
                        errorText << "Handle " << l_Handle << ", contribid " << l_ContribId << " could not be found";
                        LOG_WARNING_TEXT_RC(errorText, rc);
                    }
                }
            }
            else
            {
                rc = -1;
                errorText << "msgin_gettransferinfo(): Invalid handle value " << l_Handle << " passed";
                LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
            }
        }
    }

    catch (ExceptionBailout& e) { }
    catch (exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

//    if (l_LockHeld)
//        unlockLocalMetadata(l_LVKey, "msgin_gettransferinfo");

    // Build the response message
    txp::Msg* response;
    msg->buildResponseMsg(response);

    addReply(msg, response);

    EXIT_NO_CLOCK(__FILE__,__FUNCTION__);

    BBSTATUS l_OverallStatus = BBNONE;
    BBSTATUS l_LocalStatus = BBNONE;

    // Add response values here...
    response->addAttribute(txp::handle, l_Handle);
    response->addAttribute(txp::contribid, l_ContribId);
    if (!rc)
    {
        response->addAttribute(txp::jobid, l_JobId);
        response->addAttribute(txp::jobstepid, l_JobStepId);
        response->addAttribute(txp::tag, l_Tag);
        response->addAttribute(txp::numcontrib, l_NumContrib);
        // NOTE:  bbproxy ensures that l_NumContrib does not come in as a zero...
        response->addAttribute(txp::contrib, (const char*)l_ContribArray, sizeof(uint32_t) * l_NumContrib);
        l_OverallStatus = (BBSTATUS)l_HandleFile->status;
        response->addAttribute(txp::numreportingcontribs, l_NumberOfReportingContribs);
        response->addAttribute(txp::totalTransferKeyLength, l_LengthOfTransferKeys);
        response->addAttribute(txp::totalTransferSize, l_HandleFile->totalTransferSize);
        if (l_ContribIdFile) {
            l_LocalStatus = l_HandleFile->getLocalStatus(l_NumberOfReportingContribs, l_ContribIdFile);
            response->addAttribute(txp::localTransferSize, l_ContribIdFile->totalTransferSize);
            delete l_ContribIdFile;
        } else {
            l_LocalStatus = BBNOTACONTRIB;
            for (size_t i=0; i<(size_t)l_NumContrib; ++i) {
                if (l_ContribArray[i] == l_ContribId) {
                    l_LocalStatus = BBNOTREPORTED;
                    break;
                }
            }
            response->addAttribute(txp::localTransferSize, (uint64_t)0);
        }
        delete l_HandleFile;
    }

    // NOTE: The status values are returned even for a non-zero return code.
    //       In that case, both will be BBNONE and are provided so those values
    //       can be logged.
    response->addAttribute(txp::status, (uint64_t)l_OverallStatus);
    response->addAttribute(txp::localstatus, (uint64_t)l_LocalStatus);

    char l_LocalStatusStr[64] = {'\0'};
    char l_OverallStatusStr[64] = {'\0'};
    getStrFromBBStatus(l_LocalStatus, l_LocalStatusStr, sizeof(l_LocalStatusStr));
    getStrFromBBStatus(l_OverallStatus, l_OverallStatusStr, sizeof(l_OverallStatusStr));
    LOG(bb,info) << "msgin_gettransferinfo: handle = " << l_Handle << ", contribid = " << l_ContribId << ", returned status = " << l_OverallStatusStr << ", localstatus = " << l_LocalStatusStr;

    if (l_NumContrib == 1 && l_LocalStatus == BBFULLSUCCESS && l_OverallStatus == BBINPROGRESS)
    {
        LOG(bb,warning) << "msgin_gettransferinfo: jobid = " << l_JobId << ", jobstepid = " << l_JobStepId << ", handle = " << l_Handle << ", contribid = " << l_ContribId \
                        << " has an inconsistent status.  Local status is BBFULLSUCCESS and overall status is BBINPROGRESS with a single contributor.";
    }

    addBBErrorToMsg(response);

    // Send the response
    sendMessage(pConnectionName, response);
    delete response;

    if (l_ContribArray)
    {
        delete[] l_ContribArray;
        l_ContribArray = 0;
    }

#ifdef PROF_TIMING
    std::chrono::high_resolution_clock::time_point time_stop = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double,std::micro> elapsed_microseconds = time_stop - time_start;
    LOG(bb,trace) << __FILE__ <<":"<< __LINE__ << " " << __FUNCTION__ << "() completed after: " << elapsed_microseconds.count() << " microseconds";
#endif
    return;
}


void msgin_gettransferkeys(txp::Id id, const std::string& pConnectionName, txp::Msg* msg)
{
    ENTRY(__FILE__,__FUNCTION__);
    int rc = 0;
    stringstream errorText;

    bberror.clear(pConnectionName);

    uint64_t l_JobId = UNDEFINED_JOBID;
    uint64_t l_Handle = UNDEFINED_HANDLE;
    uint64_t l_LengthOfTransferKeys = (64*1024)-1;
    uint64_t l_TransferKeyBufferSize = 0;
    uint32_t l_ContribId = 0;
    char* l_TransferKeyBuffer = 0;

//    bool l_LockHeld = false;

    try
    {
        // Demarshall data from the message
        l_JobId = ((txp::Attr_uint64*)msg->retrieveAttrs()->at(txp::jobid))->getData();
        l_ContribId = ((txp::Attr_uint32*)msg->retrieveAttrs()->at(txp::contribid))->getData();
        l_Handle = ((txp::Attr_uint64*)msg->retrieveAttrs()->at(txp::handle))->getData();
        LOG(bb,debug) << "msgin_gettransferkeys: jobid = " << l_JobId << ", handle = " << l_Handle << ", contribid = " << l_ContribId;

        switchIds(msg);

//        lockLocalMetadata((LVKey*)0, "msgin_gettransferkeys");
//        l_LockHeld = true;

        {
            rc = -2;
            while (rc == -2)
            {
                if (l_TransferKeyBuffer)
                {
                    delete[] l_TransferKeyBuffer;
                    l_TransferKeyBuffer = 0;
                }
                // NOTE: l_LengthOfTransferKeys is returned as the length of the transfer keys WITHOUT the trailing null terminator
                //       However, the buffer is returned as null terminated, even if the length of the keys is zero.
                l_TransferKeyBufferSize = l_LengthOfTransferKeys + 1;
                l_TransferKeyBuffer = new char[l_TransferKeyBufferSize];
                rc = HandleFile::getTransferKeys(l_JobId, l_Handle, l_LengthOfTransferKeys, l_TransferKeyBufferSize, l_TransferKeyBuffer);
            }

            if (!rc)
            {
                // Log the results
                if (l_LengthOfTransferKeys < l_TransferKeyBufferSize)
                {
                    LOG(bb,info) << "msgin_gettransferkeys: jobid " << l_JobId << ", handle " << l_Handle << ", contribid " << l_ContribId << ", length of keys " << l_LengthOfTransferKeys;
                    if (l_LengthOfTransferKeys)
                    {
                        LOG(bb,info) << "                       keys " << l_TransferKeyBuffer;
                    }
                }
                else
                {
                    LOG(bb,info) << "msgin_gettransferkeys: jobid " << l_JobId << ", handle " << l_Handle << ", contribid " << l_ContribId << ", length of keys " << l_LengthOfTransferKeys \
                                 << ", provided buffer size " << l_TransferKeyBufferSize << ", buffer " << l_TransferKeyBuffer;
                }
            }
            else
            {
                // Negative return codes indicate an error.
                // NOTE: errstate already filled in by getTransferKeys()...
            }
        }
    }

    catch (ExceptionBailout& e) { }
    catch (exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

//    if (l_LockHeld)
//        unlockLocalMetadata((LVKey*)0, "msgin_gettransferkeys");

    // Build the response message
    txp::Msg* response;
    msg->buildResponseMsg(response);

    addReply(msg, response);

    EXIT_NO_CLOCK(__FILE__,__FUNCTION__);

    response->addAttribute(txp::buffersize, l_LengthOfTransferKeys);
    response->addAttribute(txp::buffer, l_TransferKeyBuffer, l_LengthOfTransferKeys);
    addBBErrorToMsg(response);

    // Send the response
    sendMessage(pConnectionName,response);
    delete response;

    if (l_TransferKeyBuffer)
    {
        delete[] l_TransferKeyBuffer;
        l_TransferKeyBuffer = 0;
    }

#ifdef PROF_TIMING
    std::chrono::high_resolution_clock::time_point time_stop = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double,std::micro> elapsed_microseconds = time_stop - time_start;
    LOG(bb,trace) << __FILE__ <<":"<< __LINE__ << " " << __FUNCTION__ << "() completed after: " << elapsed_microseconds.count() << " microseconds";
#endif
    return;
}


void msgin_gettransferlist(txp::Id id, const std::string& pConnectionName, txp::Msg* msg)
{
    ENTRY(__FILE__,__FUNCTION__);
    int rc = 0;
	stringstream errorText;

    bberror.clear(pConnectionName);

    BBJob l_Job;
    BBSTATUS l_MatchStatus;
    uint64_t l_NumHandles = 0;
    uint64_t l_NumHandlesReturned = 0;
    uint64_t l_NumAvailHandles = 0;

    uint64_t* l_HandleArray = 0;
    uint64_t l_LengthOfHandleArray = 0;
    std::vector<uint64_t> l_Handles;
//    bool l_LockHeld = false;

    try
    {
        // Demarshall data from the message
        l_Job = BBJob(((txp::Attr_uint64*)msg->retrieveAttrs()->at(txp::jobid))->getData(), ((txp::Attr_uint64*)msg->retrieveAttrs()->at(txp::jobstepid))->getData());
        l_MatchStatus = (BBSTATUS)((txp::Attr_uint64*)msg->retrieveAttrs()->at(txp::matchstatus))->getData();
        l_NumHandles = ((txp::Attr_uint64*)msg->retrieveAttrs()->at(txp::numhandles))->getData();

        stringstream l_JobStr;
        l_Job.getStr(l_JobStr);
        LOG(bb,info) << "msgin_gettransferlist: job" << l_JobStr.str() \
                     << std::hex << std::uppercase << setfill('0') \
                     << ", matchstatus = 0x" << (uint64_t)l_MatchStatus \
                     << setfill(' ') << std::nouppercase << std::dec;

        switchIds(msg);

//        lockLocalMetadata((LVKey*)0, "msgin_gettransferlist");
//        l_LockHeld = true;

        {
            rc = HandleFile::get_xbbServerHandleList(l_Handles, l_Job, l_MatchStatus);

            if (!rc)
            {
                l_NumAvailHandles = l_Handles.size();
                if (l_NumAvailHandles < l_NumHandles)
                {
                    l_NumHandles = l_NumAvailHandles;
                }
                l_LengthOfHandleArray = sizeof(uint64_t)*(l_NumHandles+1);
                l_HandleArray = (uint64_t*)(new char[l_LengthOfHandleArray]);
                memset(l_HandleArray, 0, l_LengthOfHandleArray);
                for(size_t i=0; i<l_NumHandles; ++i)
                {
                    if (l_Handles[i])
                    {
                        l_HandleArray[i] = l_Handles[i];
                        LOG(bb,info) << "msgin_gettransferlist: i=" << i << ", handle=" << l_HandleArray[i];
                        ++l_NumHandlesReturned;
                    }
                    else
                    {
	                    errorText << "Complete list of transfer handles could not be determined from the xbbserver metadata.  Handle instance " << i << " has a handle value of " << l_Handles[i];
                        LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
                    }
                }
            }
            else
            {
	            errorText << "List of transfer handles could not be determined from the xbbserver metadata";
                LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
            }
        }
    }

    catch (ExceptionBailout& e) { }
    catch (exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

//    if (l_LockHeld)
//        unlockLocalMetadata((LVKey*)0, "msgin_gettransferlist");

    // Build the response message
    txp::Msg* response;
    msg->buildResponseMsg(response);

    addReply(msg, response);

    EXIT_NO_CLOCK(__FILE__,__FUNCTION__);

    // Add response values here...
    if (!rc)
    {
        response->addAttribute(txp::numhandles, l_NumHandlesReturned);
        response->addAttribute(txp::numavailhandles, l_NumAvailHandles);
        response->addAttribute(txp::handles, (const char*)l_HandleArray, sizeof(uint64_t)*l_NumHandlesReturned);
    }
    addBBErrorToMsg(response);

    // Send the response
    sendMessage(pConnectionName,response);
    delete response;

    if (l_HandleArray)
    {
        delete[] l_HandleArray;
        l_HandleArray = 0;
    }

#ifdef PROF_TIMING
    std::chrono::high_resolution_clock::time_point time_stop = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double,std::micro> elapsed_microseconds = time_stop - time_start;
    LOG(bb,trace) << __FILE__ <<":"<< __LINE__ << " " << __FUNCTION__ << "() completed after: " << elapsed_microseconds.count() << " microseconds";
#endif
    return;
}


void msgin_removejobinfo(txp::Id id, const std::string&  pConnectionName, txp::Msg* msg)
{
    ENTRY(__FILE__,__FUNCTION__);
    int rc = 0;

    bberror.clear(pConnectionName);

    bool l_LockHeld = false;

    try
    {
        // Demarshall data from the message
        uint64_t l_JobId = ((txp::Attr_uint64*)msg->retrieveAttrs()->at(txp::jobid))->getData();
        LOG(bb,info) << "msgin_removejobinfo: JobId = " << l_JobId;

        switchIds(msg);

        // Process removejobinfo message
        string l_HostName;
        activecontroller->gethostname(l_HostName);

        lockLocalMetadata((LVKey*)0, "msgin_removejobinfo");
        l_LockHeld = true;
        rc = removeJobInfo(l_HostName, l_JobId);
        if (rc)
        {
            // NOTE: errstate already filled in...
            BAIL;
        }
    }
    catch (ExceptionBailout& e) { }
    catch (exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

    if (l_LockHeld)
    {
        unlockLocalMetadata((LVKey*)0, "msgin_removejobinfo");
    }

    // Build the response message
    txp::Msg* response;
    msg->buildResponseMsg(response);

    addReply(msg, response);

    CurrentWrkQE = (WRKQE*)0;

    RESPONSE_AND_EXIT(__FILE__,__FUNCTION__);
    return;
}


void msgin_removelogicalvolume(txp::Id id, const std::string& pConnectionName, txp::Msg* msg)
{
    ENTRY(__FILE__,__FUNCTION__);
    int rc = 0;

    bberror.clear(pConnectionName);

    try
    {
        // Demarshall data from the message
        Uuid l_lvuuid = Uuid((char*)(msg->retrieveAttrs()->at(txp::uuid)->getDataPtr()));
        LVKey l_LVKey = std::make_pair(pConnectionName, l_lvuuid);
        uint32_t l_ContribId = ((txp::Attr_uint32*)msg->retrieveAttrs()->at(txp::contribid))->getData();
        LOG(bb,info) << "msgin_removelogicalvolume: Local ConnectionName = " << getConnex(pConnectionName)->getSockfd() \
                     << ", LV Uuid = " << (char*)(msg->retrieveAttrs()->at(txp::uuid)->getDataPtr()) \
                     << ", issued by contribid = " << l_ContribId;

        switchIds(msg);

        // Process remove logical volume message

        rc = removeLogicalVolume(pConnectionName, &l_LVKey);

        if (rc) {
            SET_RC_AND_BAIL(rc);
        }
    }
    catch (ExceptionBailout& e) { }
    catch (exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

    // Build the response message
    txp::Msg* response;
    msg->buildResponseMsg(response);

    addReply(msg, response);

    CurrentWrkQE = (WRKQE*)0;

    RESPONSE_AND_EXIT(__FILE__,__FUNCTION__);
    return;
}


void msgin_resume(txp::Id id, const std::string&  pConnectionName, txp::Msg* msg)
{
    ENTRY(__FILE__,__FUNCTION__);
    int rc = 0;

    bberror.clear(pConnectionName);

    bool l_LockHeld = false;

    try
    {
        // Demarshall data from the message
        string l_CN_HostName = (const char*)msg->retrieveAttrs()->at(txp::hostname)->getDataPtr();
        string l_CN_HostNamePrt1 = l_CN_HostName;
        if (l_CN_HostName == UNDEFINED_HOSTNAME)
        {
            l_CN_HostNamePrt1 = "''";
        }
        LOG(bb,info) << "msgin_resume: Hostname " << l_CN_HostNamePrt1;

        switchIds(msg);

        // Process resume message
        lockLocalMetadata((LVKey*)0, "msgin_resume");
        l_LockHeld = true;

        // NOTE:  Need to first process all outstanding async requests.  In the restart scenarios, we must make sure
        //        that all prior suspend/resume requests have first been processed by this bbServer.
        wrkqmgr.processAllOutstandingHP_Requests((LVKey*)0);

        // Now perform this resume operation
        string l_HostName;
        activecontroller->gethostname(l_HostName);
        rc = metadata.setSuspended(l_HostName, l_CN_HostName, RESUME);
    }
    catch (ExceptionBailout& e) { }
    catch (exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

    if (l_LockHeld)
    {
        unlockLocalMetadata((LVKey*)0, "msgin_resume");
    }

    // Build the response message
    txp::Msg* response;
    msg->buildResponseMsg(response);

    addReply(msg, response);

    EXIT_NO_CLOCK(__FILE__,__FUNCTION__);

    // Add response values here...

    addBBErrorToMsg(response);

    // Send the response
    sendMessage(pConnectionName,response);
    delete response;

#ifdef PROF_TIMING
    std::chrono::high_resolution_clock::time_point time_stop = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double,std::micro> elapsed_microseconds = time_stop - time_start;
    LOG(bb,trace) << __FILE__ <<":"<< __LINE__ << " " << __FUNCTION__ << "() completed after: " << elapsed_microseconds.count() << " microseconds";
#endif
    return;
}


void msgin_retrievetransfers(txp::Id id, const std::string&  pConnectionName, txp::Msg* msg)
{
    ENTRY(__FILE__,__FUNCTION__);
    int rc = 0;

    bberror.clear(pConnectionName);

    string l_MarshalledTransferDefs;
    int l_DataObtainedLocally = 1;
    size_t l_NumberOfTransferDefs = 0;
    bool l_LockHeld = false;

    try
    {
        // Demarshall data from the message
        uint64_t l_JobId = ((txp::Attr_uint64*)msg->retrieveAttrs()->at(txp::jobid))->getData();
        uint64_t l_JobStepId = ((txp::Attr_uint64*)msg->retrieveAttrs()->at(txp::jobstepid))->getData();
        uint64_t l_Handle = ((txp::Attr_uint64*)msg->retrieveAttrs()->at(txp::handle))->getData();
        uint32_t l_ContribId = ((txp::Attr_uint32*)msg->retrieveAttrs()->at(txp::contribid))->getData();
        BB_RTV_TRANSFERDEFS_FLAGS l_Flags = (BB_RTV_TRANSFERDEFS_FLAGS)((txp::Attr_uint64*)msg->retrieveAttrs()->at(txp::flags))->getData();
        string l_HostName = (const char*)msg->retrieveAttrs()->at(txp::hostname)->getDataPtr();
        string l_HostNamePrt1 = l_HostName;
        if (l_HostName == UNDEFINED_HOSTNAME)
        {
            l_HostNamePrt1 = "''";
        }
        LOG(bb,info) << "msgin_retrievetransfers: From hostname " << l_HostNamePrt1 << ", jobId = " << l_JobId << ", jobStepId = " << l_JobStepId \
                      << ", handle = " << l_Handle << ", contribId = " << l_ContribId << ", flags = " << (uint64_t)l_Flags;

        switchIds(msg);

        // Process retrievetransfers message
        // NOTE: Not sure we need this lock anymore...
        //       \todo @DLH
        lockLocalMetadata((LVKey*)0, "msgin_retrievetransfers");
        l_LockHeld = true;

        BBTransferDefs l_TransferDefs = BBTransferDefs(l_HostName, l_JobId, l_JobStepId, l_Handle, l_ContribId, l_Flags);

        // First, attempt to retrieve ALL of the data from the local metadata.
        //
        // If ALL of the data can be obtained from the local metadata, the data
        // is filled into l_TransferDefs and rc is returned as 0.  Otherwise,
        // no transfer definitions are returned and the rc is returned as 1,
        // indicating that all of the data could not be obtained from the local
        // metadata.  Any negative return code is an error.
        //
        // NOTE: The above may not be true in the case of failing over to a backup
        //       server, as the hostname 'could' be known at more than one server.
        //       In addition, if the retrieve flag is not for ALL_DEFINITIONS,
        //       we would also have to go to the cross-bbServer metadata.
        //       We no longer try to satisfy the retrieve via local metadata and
        //       always use the cross-bbServer metadata.  @DLH
        //
        // rc = metadata.retrieveTransfers(l_TransferDefs);
        // if (rc == 1)
        {
            // Could not obtain all of the data from the local metadata.
            // Now, obtain ALL of the data from the cross-bbserver metadata.
            // Any negative return code is an error.  Otherwise, any data
            // that satisfies the search criteria is filled into l_TransferDefs.
            l_DataObtainedLocally = 0;
            rc = BBTransferDefs::xbbServerRetrieveTransfers(l_TransferDefs);
        }

        l_NumberOfTransferDefs = l_TransferDefs.getNumberOfDefinitions();

        if (rc >= 0)
        {
            rc = 0;
            l_MarshalledTransferDefs = l_TransferDefs.marshall();
            string xDataLocation;
            if (l_DataObtainedLocally)
            {
                xDataLocation = "the local bbserver cache";
            }
            else
            {
                xDataLocation = "the cross-bbserver metadata";
            }
            LOG(bb,info) << l_NumberOfTransferDefs << " transfer definitions retrieved from " << xDataLocation;
            LOG(bb,debug) << "l_MarshalledTransferDefs = |" << l_MarshalledTransferDefs << "|";
            l_TransferDefs.dump("debug", "msgin_retrievetransfers ");
        }
        else
        {
            l_DataObtainedLocally = 0;
            SET_RC_AND_BAIL(rc);
        }
    }
    catch (ExceptionBailout& e) { }
    catch (exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

    if (l_LockHeld)
    {
        unlockLocalMetadata((LVKey*)0, "msgin_retrievetransfers");
    }

    // Build the response message
    txp::Msg* response;
    msg->buildResponseMsg(response);

    addReply(msg, response);

    EXIT_NO_CLOCK(__FILE__,__FUNCTION__);

    // Add response values here...
    if (!rc)
    {
        response->addAttribute(txp::dataObtainedLocally, (uint32_t)l_DataObtainedLocally);
        response->addAttribute(txp::numTransferDefs, (uint32_t)l_NumberOfTransferDefs);
        response->addAttribute(txp::transferdefs, l_MarshalledTransferDefs.c_str(), l_MarshalledTransferDefs.size()+1);
    }
    addBBErrorToMsg(response);

    // Send the response
    sendMessage(pConnectionName,response);
    delete response;

#ifdef PROF_TIMING
    std::chrono::high_resolution_clock::time_point time_stop = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double,std::micro> elapsed_microseconds = time_stop - time_start;
    LOG(bb,trace) << __FILE__ <<":"<< __LINE__ << " " << __FUNCTION__ << "() completed after: " << elapsed_microseconds.count() << " microseconds";
#endif
    return;
}


void msgin_setthrottlerate(txp::Id id, const std::string& pConnectionName, txp::Msg* msg)
{
    ENTRY(__FILE__,__FUNCTION__);
    int rc = 0;

    bberror.clear(pConnectionName);

    try
    {
        // Demarshall data from the message
        Uuid l_lvuuid = Uuid((char*)(msg->retrieveAttrs()->at(txp::uuid)->getDataPtr()));
        uint64_t l_Rate = ((txp::Attr_uint64*)msg->retrieveAttrs()->at(txp::rate))->getData();
        LVKey l_LVKey = std::make_pair(pConnectionName, l_lvuuid);
        LOG(bb,info) << "msgin_setthrottlerate: Local Connection Name " << pConnectionName << ", LV Uuid " \
                     << (char*)(msg->retrieveAttrs()->at(txp::uuid)->getDataPtr()) << ", rate " << l_Rate;

        switchIds(msg);

        // Process set throttle rate message

        rc = setThrottleRate(pConnectionName, &l_LVKey, l_Rate);

        if (rc) {
            SET_RC_AND_BAIL(rc);
        }
    }
    catch (ExceptionBailout& e) { }
    catch (exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

    // Build the response message
    txp::Msg* response;
    msg->buildResponseMsg(response);

    addReply(msg, response);

    RESPONSE_AND_EXIT(__FILE__,__FUNCTION__);
    return;
}

#define DELAY_SECONDS 120
void msgin_starttransfer(txp::Id id, const string& pConnectionName, txp::Msg* msg)
{
    ENTRY(__FILE__,__FUNCTION__);
    int rc = 0;
    stringstream errorText;

    bberror.clear(pConnectionName);

    uint64_t l_Handle = UNDEFINED_HANDLE;
    uint32_t l_ContribId = UNDEFINED_CONTRIBID;
    uint32_t l_PerformOperation = 0;

    LVKey l_LVKey;
    LVKey l_LVKey2;
    BBLV_Info* l_LV_Info = 0;
    BBTagInfo* l_TagInfo = 0;
    BBTagID l_TagId;
    BBJob l_Job;
    uint64_t l_Tag = 0;
    uint32_t l_MarkFailedFromProxy = 0;
    uint32_t* l_ContribArray = 0;
    std::vector<uint32_t>* l_Contrib = 0;
    vector<struct stat*> l_Stats;
    vector<txp::CharArray> l_StatArray;
    vector<txp::CharArray> l_StatArray2;
    char l_Empty = '\0';

    char lv_uuid_str[LENGTH_UUID_STR] = {'\0'};
    char lv_uuid2_str[LENGTH_UUID_STR] = {'\0'};

    HandleFile* l_HandleFile = 0;
    char* l_HandleFileName = 0;
    ContribIdFile* l_ContribIdFile = 0;

    BBTransferDef l_Transfer;
    BBTransferDef* l_TransferPtr = &l_Transfer;
    BBTransferDef* l_OrgTransferPtr = l_TransferPtr;
    bool l_LockHeld = false;
    HANDLEFILE_LOCK_FEEDBACK l_LockFeedback = HANDLEFILE_WAS_NOT_LOCKED;;

    try
    {
        // Demarshall the message
        l_Handle = ((txp::Attr_uint64*)msg->retrieveAttrs()->at(txp::handle))->getData();
        l_ContribId = ((txp::Attr_uint32*)msg->retrieveAttrs()->at(txp::contribid))->getData();
        Uuid l_lvuuid = Uuid((char*)(msg->retrieveAttrs()->at(txp::uuid)->getDataPtr()));
        string l_HostName = (const char*)msg->retrieveAttrs()->at(txp::hostname)->getDataPtr();
        l_PerformOperation = ((txp::Attr_uint32*)msg->retrieveAttrs()->at(txp::performoperation))->getData();
        l_MarkFailedFromProxy = ((txp::Attr_uint32*)msg->retrieveAttrs()->at(txp::markfailed))->getData();

        size_t l_NumStats = (size_t)((txp::AttrPtr_array_of_char_arrays*)msg->retrieveAttrs()->at(txp::statinfo))->getNumberOfElementsArrayOfCharArrays();
        txp::CharArray* l_StatArray = (txp::CharArray*)msg->retrieveAttrs()->at(txp::statinfo)->getDataPtr();
        for(size_t i=0; i<l_NumStats; i++)
        {
            if (((*l_StatArray)[i].first) != 1)
            {
                struct stat* l_Stat = new(struct stat);
                memcpy((void*)l_Stat, ((*l_StatArray)[i].second), ((*l_StatArray)[i].first));
                l_Stats.push_back(l_Stat);
            }
            else
            {
                l_Stats.push_back(0);
            }
        }

        l_LVKey = std::make_pair(pConnectionName, l_lvuuid);
        l_lvuuid.copyTo(lv_uuid_str);
        BBTransferDef::demarshallTransferDef(msg, l_TransferPtr);
        for(size_t i=0; i<l_TransferPtr->files.size(); i++)
        {
            l_TransferPtr->sizeTransferred.push_back(0);
        }

        l_Job = l_TransferPtr->getJob();

        // NOTE: For a new transfer definition (not rebuilt from the metadata), the hostname, contribid,
        //       tag, and handle values were not set in the transfer definition by bbapi/bbproxy.
        //       We set three of those values here and the tag value later down in the code.
        //       If this is a rebuilt transfer definition, the hostname must be set again because this could
        //       be coming from a different CN hostname.
        l_TransferPtr->setContribId(l_ContribId);
        l_TransferPtr->setTransferHandle(l_Handle);
        l_TransferPtr->setHostName(l_HostName);

        LOG(bb,debug) << "msgin_starttransfer: Input " << l_LVKey << ", hostname " << l_HostName \
                      << ", jobid " << l_Job.getJobId() << ", jobstepid " << l_Job.getJobStepId() << ", handle " << l_Handle \
                      << ", contribid " << l_ContribId << ", perform operation=" << (l_PerformOperation ? "true" : "false") \
                      << ", mark_failed_from bbProxy=" << (l_MarkFailedFromProxy ? "true" : "false") \
                      << ", restart=" << (l_TransferPtr->builtViaRetrieveTransferDefinition() ? "true" : "false");
//                      << ", all_CN_CP_TransfersInDefinition=" << (l_TransferPtr->all_CN_CP_TransfersInDefinition() ? "true" : "false")
//                      << ", noStageinOrStageoutTransfersInDefinition=" << (l_TransferPtr->noStageinOrStageoutTransfersInDefinition() ? "true" : "false");

        if (l_PerformOperation && config.get(process_whoami+".bringup.dumpTransferDefinitionAfterDemarshall", 0)) {
            l_TransferPtr->dump("info", "Transfer Definition (after demarshall)");
        }

        switchIds(msg);

        lockLocalMetadata(&l_LVKey, "msgin_starttransfer");
        l_LockHeld = true;
        bool l_AllDone = false;

        {
            if ((l_PerformOperation) && (!l_TransferPtr->builtViaRetrieveTransferDefinition()))
            {
                // Second volley from bbProxy and not a restart scenario...
                // First, ensure that this transfer definition is NOT marked as stopped in the cross bbServer metadata...
                rc = ContribIdFile::isStopped(l_Job, l_Handle, l_ContribId);
                if (rc == 1)
                {
                    // This condition overrides any failure detected on bbProxy...
                    l_MarkFailedFromProxy = 0;

                    // rc is already 1 and this will be returned as a -2 back to bbProxy...
                    errorText << "Transfer definition for jobid " << l_Job.getJobId() << ", jobstepid " << l_Job.getJobStepId() \
                              << ", handle " << l_Handle << ", contribid " << l_ContribId << " is currently stopped." \
                              << " This transfer definition can only be submitted via restart transfer definition processing.";
                    LOG_ERROR_TEXT_AND_BAIL(errorText);
                }
            }

            if ((!l_PerformOperation) && l_TransferPtr->builtViaRetrieveTransferDefinition())
            {
                // NOTE:  Need to first process all outstanding async requests.  In the restart scenarios, we must make sure that all stop/cancel
                //        requests for this transfer definition have been processed by this bbServer.
                wrkqmgr.processAllOutstandingHP_Requests(&l_LVKey);
            }

            while ((!rc) && (!l_AllDone))
            {
                rc = metadata.getInfo(pConnectionName, l_LVKey2, l_LV_Info, l_TagInfo, l_TagId, l_Job, l_Contrib, l_Handle, l_ContribId);
                if (rc > 0)
                {
                    // Check to see if the hostname is suspended and whether this is a start or restart for the transfer definition
                    // NOTE:  bbServer cannot catch the case of a suspended hostname and this is the first transfer definition to be started.
                    //        Such a check would have to be in the else leg of getInfo().  We rely on bbProxy to prevent that from happening...
                    // NOTE:  The check for the suspend state is done on both volleys.  If the suspend state changes between the two volleys,
                    //        the start or restart transfer will fail.
                    if (((!l_LV_Info->isSuspended()) && (!l_TransferPtr->builtViaRetrieveTransferDefinition())) ||
                        (l_LV_Info->isSuspended() && l_TransferPtr->builtViaRetrieveTransferDefinition()))
                    {
                        // ** VALID REQUEST with current suspend state **
                        // Not suspended and start transfer -or- suspended and restart transfer...
                        // NOTE: We may have to spin for a while waiting for the work queue.
                        //       This is the case where we are in the process of activating this
                        //       bbServer, but we have not finished registering all of the LVKeys.
                        //       If necessary, spin for up to 2 minutes waiting for the work queue.
                        int l_Continue = DELAY_SECONDS;
                        rc = -1;
                        while (rc && l_Continue--)
                        {
                            rc = wrkqmgr.getWrkQE(&l_LVKey2, CurrentWrkQE);
                            if (rc || (!CurrentWrkQE))
                            {
                                unlockLocalMetadata(&l_LVKey2, "msgin_starttransfer (restart) - Waiting for LVKey's work queue");
                                {
                                    int l_SecondsWaiting = DELAY_SECONDS - l_Continue;
                                    if ((l_SecondsWaiting % 15) == 1)
                                    {
                                        // Display this message every 15 seconds...
                                        FL_Write6(FLDelay, StartTransferWaitForWrkQ, "Attempting to restart a transfer for jobid %ld, jobstepid %ld, handle %ld, contribid %ld. Delay of 1 second before retry. %ld seconds remain waiting for the WrkQ.",
                                                  l_Job.getJobId(), l_Job.getJobStepId(), l_Handle, (uint64_t)l_ContribId, (uint64_t)l_Continue, 0);
                                        LOG(bb,info) << ">>>>> DELAY <<<<< msgin_starttransfer (restart): Attempting to restart a transfer definition for jobid " << l_Job.getJobId() \
                                                     << ", jobstepid " << l_Job.getJobStepId() << ", handle " << l_Handle << ", contribid " << l_ContribId \
                                                     << ". Delay of 1 second before retry. " << l_Continue << " seconds remain waiting for the WrkQ.";
                                    }
                                    usleep((useconds_t)1000000);    // Delay 1 second
                                }
                                lockLocalMetadata(&l_LVKey2, "msgin_starttransfer (restart) - Waiting for LVKey's work queue");

                                // Check to make sure the job still exists after releasing/re-acquiring the lock
                                if (!jobStillExists(pConnectionName, &l_LVKey2, (BBLV_Info*)0, (BBTagInfo*)0, l_Job.getJobId(), l_ContribId))
                                {
                                    // Jobid no longer exists...
                                    // This condition overrides any failure detected on bbProxy
                                    l_MarkFailedFromProxy = 0;
                                    rc = -1;

                                    BAIL;
                                }
                            }
                        }
                        if (!rc)
                        {
                            // We drop the lock on the local metadata here so other threads can process in parallel.
                            // We may have some I/O intensive paths later, like acquiring stats for source files,
                            // where dropping the lock now is very beneficial.  Any later non-thread-safe code paths
                            // will re-acquire/drop the lock on the local metadata.  Examples of this are insertion
                            // into the BBTagParts map and adding extents to the work queue.
                            l_LockHeld = false;
                            unlockLocalMetadata(&l_LVKey, "msgin_starttransfer_early");

                            Uuid l_lvuuid2 = l_LVKey2.second;
                            l_lvuuid2.copyTo(lv_uuid2_str);
                            // NOTE: bbproxy verified that the jobstep for this transfer matches the jobstep
                            //       for the transfer handle.  Therefore, we take the jobid and jobstepid from
                            //       the transfer handle...
                            // Set the tag information in the transfer definition.  It was not set by bbapi/bbproxy...
                            l_Tag = l_TagId.getTag();
                            l_TransferPtr->setTag(l_Tag);

                            // NOTE:  Perform the lvuuid checks only during the second pass of processing...
                            LOG(bb,info ) << "msgin_starttransfer: Start processing " << l_LVKey << ", hostname " << l_HostName \
                                          << ", jobid " << l_Job.getJobId() << ", jobstepid " << l_Job.getJobStepId() << ", handle " << l_Handle \
                                          << ", contribid " << l_ContribId << ", perform operation=" << (l_PerformOperation ? "true" : "false") \
                                          << ", restart=" << (l_TransferPtr->builtViaRetrieveTransferDefinition() ? "true" : "false") \
                                          << ", mark_failed_from_bbProxy=" << (l_MarkFailedFromProxy ? "true" : "false") \
                                          << ", all_CN_CP_TransfersInDefinition=" << (l_TransferPtr->all_CN_CP_TransfersInDefinition() ? "true" : "false") \
                                          << ", noStageinOrStageoutTransfersInDefinition=" << (l_TransferPtr->noStageinOrStageoutTransfersInDefinition() ? "true" : "false") \
                                          << ", lvuuid " << lv_uuid_str << ", lvuuid2 " << lv_uuid2_str;
                            if (!l_PerformOperation || l_lvuuid == l_lvuuid2 || l_TransferPtr->noStageinOrStageoutTransfersInDefinition())
                            {
                                if (l_TagInfo->inExpectContrib(l_ContribId))
                                {
                                    uint64_t l_NumContrib = (uint64_t)l_Contrib->size();

                                    // Process the returned contrib into an array of uint32_t's and a formatted stringstream
                                    stringstream l_ContribStr;
                                    processContrib(l_Contrib, l_ContribArray, l_ContribStr);

                                    // Process the job into a formatted stringstream
                                    stringstream l_JobStr;
                                    l_Job.getStr(l_JobStr);
                                    LOG(bb,debug) << "msgin_starttransfer: Found " << l_LVKey2 << ", job" << l_JobStr.str() << ", tag " << l_Tag \
                                                  << ", handle " << l_Handle << ", numcontrib " << l_NumContrib << ", contrib " << l_ContribStr.str();

                                    // Schedule the transfer
                                    // NOTE:  Must use l_LVKey2 as it could be an noStageinOrStageoutTransfersInDefinition.  In that case, we want to
                                    //        pass the LVKey we received from getInfo() above and not the null one passed in the message...
                                    rc = queueTransfer(pConnectionName, &l_LVKey2, l_Job, l_Tag, l_TransferPtr, l_ContribId,
                                                       l_NumContrib, l_ContribArray, l_Handle, l_PerformOperation, l_MarkFailedFromProxy, &l_Stats);
                                    if (rc)
                                    {
                                        if (rc > 0)
                                        {
                                            // Normal case where the start transfer request (via restart) is not needed.
                                            // NOTE:  The appropriate logging has already been performed.

                                            BAIL;
                                        }
                                        else
                                        {
                                            // Some type of failure
                                            // This condition overrides any failure detected on bbProxy
                                            // NOTE:  errstate is filled in by queueTransfer()
                                            l_MarkFailedFromProxy = 0;

                                            errorText << "queueTransfer() failed";
                                            LOG_ERROR_AND_BAIL(errorText);
                                        }
                                    }
                                    else
                                    {
                                        l_AllDone = true;
                                    }
                                }
                                else
                                {
                                    // This condition overrides any failure detected on bbProxy...
                                    l_MarkFailedFromProxy = 0;

                                    rc = -1;
                                    errorText << "Contribid not found in the expected contrib values for the handle";
                                    LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
                                }
                            }
                            else
                            {
                                // This condition overrides any failure detected on bbProxy...
                                l_MarkFailedFromProxy = 0;

                                rc = -1;
                                errorText << "LVKeys do not match for the handle verses the files in the transfer definition." \
                                          << " The most likely causes for this error are that the source or target file does not" \
                                          << " reside in the mounted file system for the logical volume associated with the job/contribid" \
                                          << " or the incorrect handle is being used for the start transfer operation.";
                                bberror << err("error.proxy_lvuuid", lv_uuid_str) << err("error.server_lvuuid", lv_uuid2_str);
                                LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
                            }
                        }
                        else
                        {
                            // This condition overrides any failure detected on bbProxy...
                            l_MarkFailedFromProxy = 0;

                            rc = -1;
                            errorText << "Workqueue could not be found";
                            bberror << err("error.lvuuid", lv_uuid_str);
                            LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
                        }
                    }
                    else
                    {
                        // ** INVALID REQUEST with current suspend state **
                        // This condition overrides any failure detected on bbProxy...
                        l_MarkFailedFromProxy = 0;

                        if (!l_TransferPtr->builtViaRetrieveTransferDefinition())
                        {
                            // Start transfer request
                            rc = -2;
                            if (!l_PerformOperation)
                            {
                                // Start transfer request, first message volley...  Suggest to submit again...
                                errorText << "Hostname " << l_LV_Info->getHostName() << " is currently suspended. Therefore, no transfer is allowed to start at this time." \
                                          << " Suspended condition detected during the first message volley to bbServer. Attempt to retry the start transfer request when the connection is not suspended.";
                                LOG_INFO_TEXT_RC_AND_BAIL(errorText, rc);
                            }
                            else
                            {
                                // Start transfer request, second message volley...  Indicate to not submit again, as restart logic will resubmit...
                                errorText << "Hostname " << l_LV_Info->getHostName() << " is currently suspended. Therefore, no transfer is allowed to start at this time." \
                                          << " Suspended condition detected during the second message volley to bbServer.  A following restart transfer request will resubmit this transfer definition.";
                                LOG_INFO_TEXT_RC_AND_BAIL(errorText, rc);
                            }
                        }
                        else
                        {
                            // Restart transfer
                            rc = -1;
                            errorText << "Hostname " << l_LV_Info->getHostName() << " is not suspended. The transfer definition associated with jobid " \
                                      << l_Job.getJobId() << ", jobstepid " << l_Job.getJobStepId() << ", handle " << l_Handle << ", contribid " << l_ContribId \
                                      << " cannot be restarted unless the hostname is first suspended.";
                            LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
                        }
                    }
                }
                else
                {
                    if (!l_PerformOperation)
                    {
                        // Initial pass...  Attempt to determine special cases...
                        //
                        // Information regarding the LVKey and related local metadata objects for this handle
                        // could not be found on this server.  This means the client is either:
                        //   A) using an invalid handle -or-
                        //   B) received the handle to use via some application means -or-
                        //   C) this transfer definition is being restarted to a different bbServer
                        //   D) this transfer definition is being started and the logical volume was
                        //      registered to another bbServer and the contribid is not associated
                        //      with a different bbServer
                        // The getHandle() code would have primed that information into this server's local
                        // metadata if the client had obtained the handle to use via getHandle().
                        //
                        // First, verify that the LVKey was registered...
                        if (metadata.hasLVKey(&l_LVKey, l_Job.getJobId()))
                        {
                            // LVKey was found...  Continue on to reconstruct the remaining local cached metadata.
                            //
                            // NOTE: We only restart transfer definitions if the original LV on the CN still exists.
                            //       This logic will need to change if we ever attempt to failover CNs or create 'substitute'
                            //       LVs for prior failing transfer definitions.
                            //
                            //       We now 'register' each existing LVKey from the CN to the 'new' bbServer as part of the
                            //       activiate of the connection to the 'new' bbServer. That registration processing to the
                            //       'new' bbServer will be complete by the time any restart transfer processing is attempted.
                            //       In addition, processing for the CN is now suspended during the stop/retart processing.
                            //       Therefore, if the LVKey was not registered as part of the activation of the new connection,
                            //       any associated transfer definitions found in the cross-bbServer metadata will not be restarted.
                            //
                            // Check the cross bbServer metadata for the handle value.  If found and this contributor is
                            // valid for that handle, prime the local metadata and retry the getInfo() operation above...
                            rc = HandleFile::loadHandleFile(l_HandleFile, l_HandleFileName, l_Job.getJobId(), l_Job.getJobStepId(), l_Handle, DO_NOT_LOCK_HANDLEFILE);
                            if (!rc)
                            {
                                // The handle exists on one or more other servers...
                                l_Tag = l_HandleFile->tag;

                                // Get the contrib array from the Handlefile, which is an array of unit32_t's
                                uint64_t l_NumContrib = 0;
                                l_HandleFile->getContribArray(l_NumContrib, l_ContribArray);
                                if (hasContribId(l_ContribId, l_NumContrib, l_ContribArray))
                                {
                                    rc = 0;
                                    delete[] l_HandleFileName;
                                    l_HandleFileName = 0;
                                    delete l_HandleFile;
                                    l_HandleFile = 0;

                                    //  Next, determine if this contributor has already been registered
                                    bfs::path l_HandleFilePath(config.get("bb.bbserverMetadataPath", DEFAULT_BBSERVER_METADATAPATH));
                                    l_HandleFilePath /= bfs::path(to_string(l_Job.getJobId()));
                                    l_HandleFilePath /= bfs::path(to_string(l_Job.getJobStepId()));
                                    l_HandleFilePath /= bfs::path(to_string(l_Handle));
                                    Uuid l_lvuuid3 = Uuid();
                                    Uuid* l_lvuuid3_Ptr = &l_lvuuid3;

                                    if (l_TransferPtr->builtViaRetrieveTransferDefinition())
                                    {
                                        // Restart case...
                                        // First ensure that this transfer definition is marked as stopped in the cross bbServer metadata
                                        int l_Attempts = 1;
                                        bool l_AllDone2 = false;
                                        while (!l_AllDone2)
                                        {
                                            l_AllDone2 = true;

                                            rc = 1;
                                            uint64_t l_OriginalDeclareServerDeadCount = wrkqmgr.getDeclareServerDeadCount(l_Job, l_Handle, l_ContribId);
                                            uint64_t l_Continue = l_OriginalDeclareServerDeadCount;
                                            while ((rc) && (l_Continue--))
                                            {
                                                // NOTE: The handle file is locked exclusive here to serialize between this bbServer and another
                                                //       bbServer that is marking the handle/contribid file as 'stopped'
                                                rc = HandleFile::loadHandleFile(l_HandleFile, l_HandleFileName, l_Job.getJobId(), l_Job.getJobStepId(), l_Handle, LOCK_HANDLEFILE, &l_LockFeedback);
                                                if (!rc)
                                                {
                                                    rc = ContribIdFile::loadContribIdFile(l_ContribIdFile, l_HandleFilePath, l_ContribId, l_lvuuid3_Ptr);
                                                    if (rc == 1)
                                                    {
                                                        if (l_ContribIdFile)
                                                        {
                                                            // Valid contribid file
                                                            if (!l_ContribIdFile->stopped())
                                                            {
                                                                // Contribid is not marked as stopped
                                                                if (l_ContribIdFile->notRestartable())
                                                                {
                                                                    // All extents have been processed, all files closed, no failed files, not canceled.
                                                                    // The transfer definition is not marked as stopped, therefore, no need to restart this
                                                                    // transfer definition.

                                                                    // This condition overrides any failure detected on bbProxy...
                                                                    l_MarkFailedFromProxy = 0;
                                                                    errorText << "msgin_starttransfer(): For jobid " << l_Job.getJobId() << ", jobstepid " << l_Job.getJobStepId() \
                                                                              << ", handle " << l_Handle << ", contribid " << l_ContribId \
                                                                              << ", all extents for the handle file have been processed.  The transfer either finished or was canceled." \
                                                                              << "  See previous messages.";
                                                                    LOG_INFO_TEXT_AND_BAIL(errorText);
                                                                }
                                                                else
                                                                {
                                                                    // Transfer definition is restartable.
                                                                    // First ensure that the handle file is stopped.
                                                                    if (l_HandleFile->stopped())
                                                                    {
                                                                        // Stopped, will exit both loops...
                                                                        rc = 0;
                                                                    }
                                                                    else
                                                                    {
                                                                        // Handle file not stopped.
                                                                        // Continue to spin...
                                                                    }
                                                                }
                                                            }
                                                            else
                                                            {
                                                                // Contribid is marked as stopped.
                                                                // First ensure that the handle file is also stopped.
                                                                if (l_HandleFile->stopped())
                                                                {
                                                                    // Stopped, will exit both loops...
                                                                    rc = 0;
                                                                }
                                                                else
                                                                {
                                                                    // Handle file not stopped.
                                                                    // Continue to spin...
                                                                }
                                                            }
                                                        }
                                                        else
                                                        {
                                                            rc = 1;
                                                            // This condition overrides any failure detected on bbProxy...
                                                            l_MarkFailedFromProxy = 0;
                                                            errorText << "msgin_starttransfer(): ContribId " << l_ContribId << " was not found in the cross bbServer metadata (ContribIdFile pointer is NULL)." \
                                                                      << " All transfers for this contributor may have already finished.  See previous messages.";
                                                            LOG_INFO_TEXT_AND_BAIL(errorText);
                                                        }
                                                    }
                                                    else
                                                    {
                                                        // The Contrib file could not be loaded -or-
                                                        // the ContribId file could not be found in the Contrib file
                                                        rc = 1;
                                                        // This condition overrides any failure detected on bbProxy...
                                                        l_MarkFailedFromProxy = 0;
                                                        errorText << "msgin_starttransfer(): Error occurred when attempting to load the contrib file for contribid " << l_ContribId << " (Negative rc from loadContribIdFile())." \
                                                                  << " All transfers for this contributor may have already finished.  See previous messages.";
                                                        LOG_INFO_TEXT_AND_BAIL(errorText);
                                                    }

                                                    if (rc)
                                                    {
                                                        if (l_Continue)
                                                        {
                                                            // Release the lock on the handle file
                                                            l_HandleFile->close(l_LockFeedback);

                                                            int l_SecondsWaiting = l_OriginalDeclareServerDeadCount - l_Continue;
                                                            if ((l_SecondsWaiting % 15) == 5)
                                                            {
                                                                // Display this message every 15 seconds, after an initial wait of 5 seconds...
                                                                FL_Write6(FLDelay, RestartWaitForStop1, "Attempting to restart a transfer definition for jobid %ld, jobstepid %ld, handle %ld, contribid %ld. Delay of 1 second before retry. %ld seconds remain waiting for the original bbServer to act before an unconditional stop is performed.",
                                                                          l_Job.getJobId(), l_Job.getJobStepId(), l_Handle, (uint64_t)l_ContribId, (uint64_t)l_Continue, 0);
                                                                LOG(bb,info) << ">>>>> DELAY <<<<< msgin_starttransfer (restart): Attempting to restart a transfer definition for jobid " << l_Job.getJobId() \
                                                                             << ", jobstepid " << l_Job.getJobStepId() << ", handle " << l_Handle << ", contribid " << l_ContribId \
                                                                             << ". Waiting for transfer definition to be marked as stopped. Delay of 1 second before retry. " << l_Continue \
                                                                             << " seconds remain waiting for the original bbServer to act before an unconditional stop is performed.";
                                                            }
                                                            // If we will wait for at least another minute, re-append the stop transfer request
                                                            // every 60 seconds in case the 'old' bbServer just came online...
                                                            if ((l_Continue > 60) && (l_SecondsWaiting % 60) == 0)
                                                            {
                                                                BBLV_Metadata::appendAsyncRequestForStopTransfer(l_TransferPtr->getHostName(), (uint64_t)l_Job.getJobId(),
                                                                                                                 (uint64_t)l_Job.getJobStepId(), l_Handle, l_ContribId, (uint64_t)BBSCOPETRANSFER);
                                                            }
                                                            unlockLocalMetadata(&l_LVKey, "msgin_starttransfer (restart) - Waiting for transfer definition to be marked as stopped");
                                                            {
                                                                usleep((useconds_t)1000000);    // Delay 1 second
                                                            }
                                                            lockLocalMetadata(&l_LVKey, "msgin_starttransfer (restart) - Waiting for transfer definition to be marked as stopped");
                                                        }

                                                        // Check to make sure the job still exists after releasing/re-acquiring the lock
                                                        if (!jobStillExists(pConnectionName, &l_LVKey, (BBLV_Info*)0, (BBTagInfo*)0, l_Job.getJobId(), l_ContribId))
                                                        {
                                                            // Job no longer exists... Indicate to not restart this transfer definition.
                                                            rc = 1;
                                                            // This condition overrides any failure detected on bbProxy...
                                                            l_MarkFailedFromProxy = 0;
                                                            BAIL;
                                                        }
                                                    }

                                                    // Clean up for the next iteration...
                                                    if (l_ContribIdFile)
                                                    {
                                                        delete l_ContribIdFile;
                                                        l_ContribIdFile = 0;
                                                    }
                                                    if (l_HandleFileName)
                                                    {
                                                        delete[] l_HandleFileName;
                                                        l_HandleFileName = 0;
                                                    }
                                                    if (l_HandleFile)
                                                    {
                                                        l_HandleFile->close(l_LockFeedback);
                                                        delete l_HandleFile;
                                                        l_HandleFile = 0;
                                                    }
                                                }
                                                else
                                                {
                                                    // Handle file could not be loaded
                                                    //
                                                    // This condition overrides any failure detected on bbProxy...
                                                    l_MarkFailedFromProxy = 0;
                                                    if (l_TransferPtr->builtViaRetrieveTransferDefinition())
                                                    {
                                                        rc = 1;
                                                        errorText << "msgin_starttransfer(): Error occurred when attempting to re-prime the metadata for contribid " << l_ContribId << " (Negative rc from loadHandleFile() (2))." \
                                                                  << " All transfers for this contributor may have already finished.  See previous messages.";
                                                        LOG_INFO_TEXT_AND_BAIL(errorText);                                                    }
                                                    else
                                                    {
                                                        rc = -1;
                                                        errorText << "Transfer handle could not be found on any server (2)";
                                                        LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
                                                    }
                                                }
                                            }

                                            if (rc == 1)
                                            {
                                                if (l_Attempts--)
                                                {
                                                    rc = prepareForRestartOriginalServerDead(pConnectionName, &l_LVKey, l_Handle, l_Job, l_ContribId);
                                                    switch (rc)
                                                    {
                                                        case 1:
                                                        {
                                                            // Reset of cross bbServer metadata was successful...  Continue...
                                                            rc = 0;
                                                            LOG(bb,info) << "ContribId " << l_ContribId << " was found in the cross bbServer metadata and was successfully stopped" \
                                                                      << " after the original bbServer was unresponsive";
                                                            l_AllDone2 = false;
                                                        }
                                                        break;

                                                        case 2:
                                                        {
                                                            // Indicate to not restart this transfer definition
                                                            rc = 1;
                                                            // This condition overrides any failure detected on bbProxy...
                                                            l_MarkFailedFromProxy = 0;
                                                            errorText << "ContribId " << l_ContribId << " was found in the cross bbServer metadata, but no file associated with the transfer definition needed to be restarted." \
                                                                      << " Most likely, the transfer completed for the contributor or was canceled. Therefore, the transfer definition cannot be restarted. See any previous messages.";
                                                            LOG_INFO_TEXT_AND_BAIL(errorText);
                                                        }
                                                        break;

                                                        default:
                                                        {
                                                            // Indicate to not restart this transfer definition
                                                            rc = 1;
                                                            // This condition overrides any failure detected on bbProxy...
                                                            l_MarkFailedFromProxy = 0;
                                                            errorText << "Attempt to reset the cross bbServer metadata for the transfer definition associated with contribid " << l_ContribId << " to stopped failed." \
                                                                      << " Therefore, the transfer definition cannot be restarted. See any previous messages.";
                                                            LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
                                                        }
                                                        break;
                                                    }
                                                }
                                                else
                                                {
                                                    // Indicate to not restart this transfer definition (Should not get here...)
                                                    rc = 1;
                                                    // This condition overrides any failure detected on bbProxy...
                                                    l_MarkFailedFromProxy = 0;
                                                    errorText << "ContribId " << l_ContribId << " was found in the cross bbServer metadata, but it does not have a status of stopped." \
                                                              << " The transfer definition was marked as stopped after the original bbServer was unresponsive," \
                                                              << " but the cross bbServer metadata no longer shows the transfer definition as stopped." \
                                                              << " Therefore, the transfer definition cannot be restarted. See any previous messages.";
                                                    LOG_ERROR_TEXT_AND_BAIL(errorText);
                                                }
                                            }
                                        }
                                    }

                                    if (!rc)
                                    {
                                        //  getHandle() will create the necessary local metadata for this LVKey and handle value.
                                        //
                                        //  Objective is to get the local metadata created and then we will iterate through this
                                        //  loop again.  Next time through, the local metadata will be found and we can then go on
                                        //  to registering the transfer definition with queueTransfer().
                                        LVKey* l_LVKeyPtr = &l_LVKey;

                                        // NOTE: We may have to spin for a while waiting for the LVKey to be registered.
                                        //       This is the case where we are in the process of activating this
                                        //       bbServer, but we have not finished registering all of the LVKeys.
                                        //       If necessary, spin for up to 2 minutes.
                                        int l_Continue = DELAY_SECONDS;
                                        rc = -2;
                                        while (rc && l_Continue--)
                                        {
                                            rc = getHandle(pConnectionName, l_LVKeyPtr, l_Job, l_Tag, l_NumContrib, l_ContribArray, l_Handle);
                                            switch (rc)
                                            {
                                                case 0:
                                                {
                                                    // Local metadata was successfully created.  Iterate and continue processing...
                                                }
                                                break;

                                                case -2:
                                                {
                                                    // LVKey could not be found...  Spin for a while waiting for the
                                                    // registration of the necessary LVKey...
                                                    //
                                                    // NOTE: Not sure we can get here anymore...  @DLH
                                                    unlockLocalMetadata(&l_LVKey, "msgin_starttransfer (restart) - Waiting for LVKey");
                                                    {
                                                        int l_SecondsWaiting = DELAY_SECONDS - l_Continue;
                                                        if ((l_SecondsWaiting % 15) == 1)
                                                        {
                                                            // Display this message every 15 seconds...
                                                            FL_Write6(FLDelay, StartTransferWaitForLVKey, "Attempting to restart a transfer definition for jobid %ld, jobstepid %ld, handle %ld, contribid %ld. Delay of 1 second before retry. %ld seconds remain waiting for the LVKey.",
                                                                      l_Job.getJobId(), l_Job.getJobStepId(), l_Handle, (uint64_t)l_ContribId, (uint64_t)l_Continue, 0);
                                                            LOG(bb,info) << ">>>>> DELAY <<<<< msgin_starttransfer (restart): Attempting to restart a transfer definition for jobid " << l_Job.getJobId() \
                                                                         << ", jobstepid " << l_Job.getJobStepId() << ", handle " << l_Handle << ", contribid " << l_ContribId \
                                                                         << ". Delay of 1 second before retry. " << l_Continue << " seconds remain waiting for the LVKey.";
                                                        }
                                                        usleep((useconds_t)1000000);    // Delay 1 second
                                                    }
                                                    lockLocalMetadata(&l_LVKey, "msgin_starttransfer (restart) - Waiting for LVKey");
                                                }
                                                break;

                                                default:
                                                {
                                                    if (rc > 0)
                                                    {
                                                        // Local metadata was found.  We hit he window where we were activating this bbServer and the
                                                        // LVKey was not yet registered when checking above, but it now exists on this bbServer.
                                                        // Continue...
                                                        //
                                                        // NOTE: Not sure we can get here anymore...  @DLH
                                                        rc = 0;
                                                    }
                                                    else
                                                    {
                                                        // Negative return codes indicate an error.
                                                        // NOTE: errstate already filled in by gethandle()...
                                                        // This condition overrides any failure detected on bbProxy...
                                                        l_MarkFailedFromProxy = 0;
                                                        SET_RC_AND_BAIL(rc);
                                                    }
                                                }
                                                break;
                                            }
                                        }
                                        if (rc)
                                        {
                                            // NOTE: Not sure we can get here anymore...  @DLH
                                            stringstream l_JobStr;
                                            l_Job.getStr(l_JobStr);
                                            if (l_TransferPtr->builtViaRetrieveTransferDefinition())
                                            {
                                                // This is a restart transfer scenrio and the LVKey cannot be found
                                                rc = -1;
                                                // This condition overrides any failure detected on bbProxy...
                                                l_MarkFailedFromProxy = 0;
                                                errorText << "A logical volume (LVKey) is not currently associated with job" << l_JobStr.str() \
                                                          << " on this bbServer for contribid " << l_ContribId \
                                                          << ".  This is a restart scenario and the expected LVKey was not found.";
                                                LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
                                            }
                                            else
                                            {
                                                // This is a start transfer and the LVKey cannot be found
                                                rc = -1;
                                                // This condition overrides any failure detected on bbProxy...
                                                l_MarkFailedFromProxy = 0;
                                                errorText << "A logical volume (LVKey) is not currently associated with job" << l_JobStr.str() \
                                                          << " on this bbServer for contribid " << l_ContribId \
                                                          << ".  This is not a restart scenario and either the contribid is already known" \
                                                          << " for this job to another bbServer in the cluster or a logical volume has yet" \
                                                          << " to be created for the job on this bbServer.";
                                                LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
                                            }
                                        }
                                    }
                                }
                                else
                                {
                                    rc = -1;
                                    // This condition overrides any failure detected on bbProxy...
                                    l_MarkFailedFromProxy = 0;
                                    errorText << "Contribid is not a listed contributor for the specified handle";
                                    LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
                                }
                            }
                            else
                            {
                                // This condition overrides any failure detected on bbProxy...
                                l_MarkFailedFromProxy = 0;
                                if (l_TransferPtr->builtViaRetrieveTransferDefinition())
                                {
                                    rc = 1;
                                    LOG(bb,info) << "msgin_starttransfer(): Error occurred when attempting to re-prime the metadata for contribid " << l_ContribId << " (Negative rc from loadHandleFile() (1))." \
                                                 << " All transfers for this contributor may have already finished.  See previous messages.";
                                    BAIL;
                                }
                                else
                                {
                                    rc = -1;
                                    errorText << "Transfer handle could not be found on any server (1)";
                                    LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
                                }
                            }
                        }
                        else
                        {
                            // This condition overrides any failure detected on bbProxy...
                            l_MarkFailedFromProxy = 0;
                            if (l_TransferPtr->builtViaRetrieveTransferDefinition())
                            {
                                // For a restart operation, this is a case where residual jobs/transfer definitions exist
                                // in the cross-bbServer metadata that the retrieve/stop/restart processing found and is
                                // now trying to restart.  We skip over such residual jobs/transfer definitions.
                                rc = 1;
                                errorText << "Transfer definition associated with " << l_LVKey << ", hostname " << l_HostName \
                                          << ", jobid " << l_Job.getJobId() << ", jobstepid " << l_Job.getJobStepId() \
                                          << ", handle " << l_Handle << ", contribid " << l_ContribId << " was not considered for restart."\
                                          << " The necessary local metadata for the transfer definition could not be found on this server." \
                                          << " This is caused by residual jobs/transfer definitions that are not currently active" \
                                          << " being found in the metadata by the retrieve/stop processing.";
                                LOG_INFO_TEXT_AND_BAIL(errorText);                            }
                            else
                            {
                                rc = -1;
                                errorText << "Failure occurred when attempting to start the transfer definition associated with " \
                                          << l_LVKey << ", hostname " << l_HostName << ", jobid " << l_Job.getJobId() << ", jobstepid " << l_Job.getJobStepId() \
                                          << ", handle " << l_Handle << ", contribid " << l_ContribId << ", rc " << rc \
                                          << ".  The necessary local metadata for the transfer definition could not be found on this server.";
                                LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
                            }
                        }
                    }
                    else
                    {
                        rc = -1;
                        // This condition overrides any failure detected on bbProxy...
                        l_MarkFailedFromProxy = 0;
                        errorText << "Failure occurred between the first and second calls to the servicing bbServer " \
                                  << "when attempting to start or restart the transfer definition associated with " \
                                  << l_LVKey << ", hostname " << l_HostName << ", handle " << l_Handle << ", contribid " << l_ContribId \
                                  << ".  The necessary local metadata for the transfer definition could not be found.";
                        LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
                    }
                }
            }

            if ((!rc) && (l_PerformOperation))
            {
                // If the transfer definition has keys, insert those into the cross bbserver metadata (handlefile)
                // NOTE:  This is the last thing we do in the l_PerformOperation pass.  Once these keys are added,
                //        there is no backout code if the start transfer request were to fail.  However, on failure,
                //        the transfer definition and the handle would be marked as being failed.
                if (l_TransferPtr->getNumberOfKeys())
                {
                    // NOTE:  Must use l_LVKey2 as it could be an noStageinOrStageoutTransfersInDefinition.  In that case, we want to
                    //        pass the LVKey we received from getInfo() above and not the null one passed in the message...
                    rc = HandleFile::update_xbbServerHandleTransferKeys(l_TransferPtr, &l_LVKey2, l_Job, l_Handle);
                }
                if (!rc)
                {
                    // If for a restart, update the handle status (it might transistion from STOPPED)
                    if (l_TransferPtr->builtViaRetrieveTransferDefinition())
                    {
                        // NOTE: We don't handle the return code here...  Any anomalies will be logged by update_xbbServerHandleStatus().
                        //       We don't want to fail this transfer if for some reason we can't update the handle status here...
                        HandleFile::update_xbbServerHandleStatus(&l_LVKey2, l_Job.getJobId(), l_Job.getJobStepId(), l_Handle, l_ContribId, 0, 0, FULL_SCAN);
                    }
                }
            }
        }
    }
    catch (ExceptionBailout& e) { }
    catch (exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

    // NOTE: Lock protocol requires that if both the handle file and transfer queue are locked,
    //       the handle file must be released first.  The remaining 'cleanup' items are at the
    //       end of this procedure.
    if (l_HandleFile)
    {
        l_HandleFile->close(l_LockFeedback);
        delete l_HandleFile;
        l_HandleFile = 0;
    }

    if (l_LockHeld)
    {
        l_LockHeld = false;
        unlockLocalMetadata(&l_LVKey, "msgin_starttransfer");
    }

    // Build the response message
    txp::Msg* response;
    msg->buildResponseMsg(response);

    if (!rc)
    {
        if (!l_PerformOperation)
        {
            l_StatArray2.resize(1);
            for(auto e : l_Stats)
            {
                if (e != NULL)
                {
                    l_StatArray2[0].push_back(make_pair(sizeof(*e), (char*)e));
                }
                else
                {
                    l_StatArray2[0].push_back(make_pair(1, &l_Empty));
                }
            }
            response->addAttribute(txp::statinfo, &l_StatArray2[0]);
        }

        if (l_MarkFailedFromProxy)
        {
            markTransferFailed(&l_LVKey2, l_TransferPtr, l_LV_Info, l_Handle, l_ContribId);
            // NOTE: errstate filled in by bbProxy
            rc = -1;
            SET_RC(rc);
        }
    }
    else
    {
        // If we got far enough along to insert this transfer definition into the local metadata,
        // clean up the I/O map
        if (l_TransferPtr != l_OrgTransferPtr)
        {
            l_TransferPtr->cleanUpIOMap();
        }

        if (rc != 1)
        {
            if (rc != -2)
            {
                // Mark the transfer definition and the handle failed if this is the second pass -or-
                // we got far enough along to insert this transfer definition into the local metadata
                if (l_PerformOperation || (l_TransferPtr != l_OrgTransferPtr))
                {
                    markTransferFailed(&l_LVKey2, l_TransferPtr, l_LV_Info, l_Handle, l_ContribId);
                }
            }
        }
        else
        {
            // Transfer definition is not to be restarted
            // Send rc=-2 back to bbProxy as a tolerated exception...
            // NOTE:  Shouldn't be possible to have a positive rc
            //        and l_MarkFailedFromProxy still be set on at this
            //        point in the code...  @DLH
            rc = -2;
            SET_RC(rc);
        }
    }

    addReply(msg, response);

    EXIT_NO_CLOCK(__FILE__,__FUNCTION__);

    // Add response values here...
    response->addAttribute(txp::markfailed, l_MarkFailedFromProxy);

    addBBErrorToMsg(response);

    // Send the response
    sendMessage(pConnectionName,response);
    delete response;

    // Clean up...
    CurrentWrkQE = (WRKQE*)0;
    if (l_ContribIdFile)
    {
        delete l_ContribIdFile;
        l_ContribIdFile = 0;
    }

    if (l_HandleFileName)
    {
        delete[] l_HandleFileName;
        l_HandleFileName = 0;
    }

    if (l_ContribArray)
    {
        delete[] l_ContribArray;
        l_ContribArray = 0;
    }

    for (auto e : l_Stats)
    {
        if (e)
        {
            delete e;
        }
    }

#ifdef PROF_TIMING
    std::chrono::high_resolution_clock::time_point time_stop = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double,std::micro> elapsed_microseconds = time_stop - time_start;
    LOG(bb,trace) << __FILE__ <<":"<< __LINE__ <<" msgin_starttransfer() completed after: " << elapsed_microseconds.count() << " microseconds";
#endif
    return;
}
#undef DELAY_SECONDS

void msgin_stoptransfers(txp::Id id, const std::string&  pConnectionName, txp::Msg* msg)
{
    ENTRY(__FILE__,__FUNCTION__);
    int rc = 0;

    bberror.clear(pConnectionName);

    string l_Archive;
    BBTransferDefs* l_TransferDefs = 0;
    uint32_t l_NumStoppedTransferDefs = 0;
    bool l_LockHeld = false;

    try
    {
        // Demarshall data from the message
        uint64_t l_JobId = ((txp::Attr_uint64*)msg->retrieveAttrs()->at(txp::jobid))->getData();
        uint64_t l_JobStepId = ((txp::Attr_uint64*)msg->retrieveAttrs()->at(txp::jobstepid))->getData();
        uint64_t l_Handle = ((txp::Attr_uint64*)msg->retrieveAttrs()->at(txp::handle))->getData();
        uint32_t l_ContribId = ((txp::Attr_uint32*)msg->retrieveAttrs()->at(txp::contribid))->getData();
        string l_HostName = (const char*)msg->retrieveAttrs()->at(txp::hostname)->getDataPtr();
        string l_HostNamePrt1 = l_HostName;
        if (l_HostName == UNDEFINED_HOSTNAME)
        {
            l_HostNamePrt1 = "''";
        }
        // NOTE: archive string is already null terminated and the length accounts for the null terminator
        l_Archive.assign((const char*)msg->retrieveAttrs()->at(txp::transferdefs)->getDataPtr(), (uint64_t)(msg->retrieveAttrs()->at(txp::transferdefs)->getDataLength()));
        LOG(bb,info) << "msgin_stoptransfers: From hostname " << l_HostNamePrt1 << ", jobId = " << l_JobId << ", jobStepId = " << l_JobStepId \
                     << ", handle = " << l_Handle << ", contribId = " << l_ContribId << ", archive for transferdefs = |" << l_Archive << "|";

        switchIds(msg);

        // Process stop transfers message
        lockLocalMetadata((LVKey*)0, "msgin_stoptransfers");
        l_LockHeld = true;

        l_TransferDefs = new BBTransferDefs();

        // Demarshall the archive into the transfer definitions object
        l_TransferDefs->demarshall(l_Archive);

        // NOTE:  Need to first process all outstanding async requests.  In the restart scenarios, we must make sure
        //        that all prior restart related requests have first been processed by this bbServer.
        wrkqmgr.processAllOutstandingHP_Requests((LVKey*)0);

        // Process the transfer definitions object for the stop transfers operation
        l_TransferDefs->stopTransfers(l_HostName, l_JobId, l_JobStepId, l_Handle, l_ContribId, l_NumStoppedTransferDefs);

        // Dump the work queues...
        wrkqmgr.dump("info", " Work Queue Mgr (After msgin_stoptransfers())", DUMP_ALWAYS);
    }
    catch (ExceptionBailout& e) { }
    catch (exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

    if (l_LockHeld)
    {
        unlockLocalMetadata((LVKey*)0, "msgin_stoptransfers");
    }

    if (l_TransferDefs)
    {
        delete l_TransferDefs;
        l_TransferDefs = 0;
    }

    // Build the response message
    txp::Msg* response;
    msg->buildResponseMsg(response);

    addReply(msg, response);

    EXIT_NO_CLOCK(__FILE__,__FUNCTION__);

    // Add response values here...
    if (!rc)
    {
        response->addAttribute(txp::numTransferDefs, l_NumStoppedTransferDefs);
    }

    addBBErrorToMsg(response);

    // Send the response
    sendMessage(pConnectionName,response);
    delete response;

    CurrentWrkQE = (WRKQE*)0;

#ifdef PROF_TIMING
    std::chrono::high_resolution_clock::time_point time_stop = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double,std::micro> elapsed_microseconds = time_stop - time_start;
    LOG(bb,trace) << __FILE__ <<":"<< __LINE__ << " " << __FUNCTION__ << "() completed after: " << elapsed_microseconds.count() << " microseconds";
#endif
    return;
}


void msgin_suspend(txp::Id id, const std::string&  pConnectionName, txp::Msg* msg)
{
    ENTRY(__FILE__,__FUNCTION__);
    int rc = 0;

    bberror.clear(pConnectionName);

    bool l_LockHeld = false;

    try
    {
        // Demarshall data from the message
        string l_CN_HostName = (const char*)msg->retrieveAttrs()->at(txp::hostname)->getDataPtr();
        string l_CN_HostNamePrt1 = l_CN_HostName;
        if (l_CN_HostName == UNDEFINED_HOSTNAME)
        {
            l_CN_HostNamePrt1 = "''";
        }
        LOG(bb,info) << "msgin_suspend: Hostname " << l_CN_HostNamePrt1;

        switchIds(msg);

        // Process suspend message
        lockLocalMetadata((LVKey*)0, "msgin_suspend");
        l_LockHeld = true;

        // NOTE:  Need to first process all outstanding async requests.  In the restart scenarios, we must make sure
        //        that all prior suspend/resume requests have first been processed by this bbServer.
        wrkqmgr.processAllOutstandingHP_Requests((LVKey*)0);

        // Now perform this suspend operation
        string l_HostName;
        activecontroller->gethostname(l_HostName);
        rc = metadata.setSuspended(l_HostName, l_CN_HostName, SUSPEND);
    }
    catch (ExceptionBailout& e) { }
    catch (exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

    if (l_LockHeld)
    {
        unlockLocalMetadata((LVKey*)0, "msgin_suspend");
    }

    // Build the response message
    txp::Msg* response;
    msg->buildResponseMsg(response);

    addReply(msg, response);

    EXIT_NO_CLOCK(__FILE__,__FUNCTION__);

    // Add response values here...

    addBBErrorToMsg(response);

    // Send the response
    sendMessage(pConnectionName,response);
    delete response;

#ifdef PROF_TIMING
    std::chrono::high_resolution_clock::time_point time_stop = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double,std::micro> elapsed_microseconds = time_stop - time_start;
    LOG(bb,trace) << __FILE__ <<":"<< __LINE__ << " " << __FUNCTION__ << "() completed after: " << elapsed_microseconds.count() << " microseconds";
#endif
    return;
}


//*****************************************************************************
//  Initial handshake with bbproxy
//*****************************************************************************

int nvmfConnectPath(const string& serial, const string& connectionKey);
void msgin_hello(txp::Id id, const string& pConnectionName,  txp::Msg* msg)
{
    ENTRY_NO_CLOCK(__FILE__,__FUNCTION__);
    int rc = 0;
    txp::CharArray nackSerials_attr;

    bberror.clear(pConnectionName);

    FL_Write(FLServer, Msg_HelloFromProxy, "bbServerLoginHello command received",0,0,0,0);
    time_t l_seconds = time(NULL);
    try
    {
        time_t remote_epoch = ((txp::Attr_uint64*)msg->retrieveAttrs()->at(txp::epochtimeinseconds))->getData();
        LOG(bb,info) << "Local epoch time in seconds: " << l_seconds << " GMT="<<  asctime(gmtime(&l_seconds));
        LOG(bb,info) << "Remote epoch time in seconds: " << remote_epoch << " GMT="<<  asctime(gmtime((const time_t*)&remote_epoch));
        const uint64_t l_MAXDIFFSECONDS=1;
        const uint64_t l_DIFFSECONDS= abs(l_seconds - remote_epoch);
        if ( l_DIFFSECONDS > l_MAXDIFFSECONDS ){
            LOG(bb,warning) << "Time difference between bbserver and bbproxy name="<<pConnectionName<<" is "<<l_DIFFSECONDS<<" seconds, more than "<<l_MAXDIFFSECONDS << " seconds";
        }

        uint64_t numserials          = ((txp::AttrPtr_array_of_char_arrays*)msg->retrieveAttrs()->at(txp::knownSerials))->getNumberOfElementsArrayOfCharArrays();
        txp::CharArray* knownSerials = (txp::CharArray*)msg->retrieveAttrs()->at(txp::knownSerials)->getDataPtr();

        string connectionKey =  string((char*)(msg->retrieveAttrs()->at(txp::connectionKey)->getDataPtr()));

        for(uint64_t x=0; x<numserials; x++)
        {
            try
            {
                getDeviceBySerial((*knownSerials)[x].second);
                LOG(bb,info) << "Serial (" << (*knownSerials)[x].second << ") can be used between bbProxy <-> bbServer.";
            }
            catch(exception& e)
            {
                int nCrc=nvmfConnectPath( (*knownSerials)[x].second, connectionKey);
                if (nCrc)
                {
                    LOG(bb,info) << "Serial (" << (*knownSerials)[x].second << ") not available on bbServer.  Telling bbProxy to remove from its list.";
                    nackSerials_attr.push_back(make_pair((*knownSerials)[x].first, (*knownSerials)[x].second));
                }

            }
        }
    }
    catch(ExceptionBailout& e) { }
    catch(exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

    txp::Msg* response;
    msg->buildResponseMsg(response);
    addReply(msg, response);

    EXIT_NO_CLOCK(__FILE__,__FUNCTION__);

    // Add response values here...
    if (!rc)
    {
        response->addAttribute(txp::epochtimeinseconds, (uint64_t)l_seconds);
        response->addAttribute(txp::nackSerials, &nackSerials_attr);
    }

    addBBErrorToMsg(response);
    sendMessage(pConnectionName,response);
    LOG(bb,info) << __FUNCTION__ <<" response to "<<pConnectionName;
    delete response;
}


//*****************************************************************************
//  Main routines
//*****************************************************************************


int registerHandlers()
{
    registerMessageHandler(txp::BB_CANCELTRANSFER, msgin_canceltransfer);
    registerMessageHandler(txp::BB_CREATELOGICALVOLUME, msgin_createlogicalvolume);
    registerMessageHandler(txp::BB_GETTHROTTLERATE, msgin_getthrottlerate);
    registerMessageHandler(txp::BB_GETTRANSFERHANDLE, msgin_gettransferhandle);
    registerMessageHandler(txp::BB_GETTRANSFERINFO, msgin_gettransferinfo);
    registerMessageHandler(txp::BB_GETTRANSFERKEYS, msgin_gettransferkeys);
    registerMessageHandler(txp::BB_GETTRANSFERLIST, msgin_gettransferlist);
    registerMessageHandler(txp::BB_REMOVEJOBINFO, msgin_removejobinfo);
    registerMessageHandler(txp::BB_REMOVELOGICALVOLUME, msgin_removelogicalvolume);
    registerMessageHandler(txp::BB_RESUME, msgin_resume);
    registerMessageHandler(txp::BB_RETRIEVE_TRANSFERS, msgin_retrievetransfers);
    registerMessageHandler(txp::BB_SETTHROTTLERATE, msgin_setthrottlerate);
    registerMessageHandler(txp::BB_STARTTRANSFER, msgin_starttransfer);
    registerMessageHandler(txp::BB_SUSPEND, msgin_suspend);
    registerMessageHandler(txp::BB_STOP_TRANSFERS, msgin_stoptransfers);
    registerMessageHandler(txp::CORAL_HELLO, msgin_hello);
    registerMessageHandler(txp::CORAL_STAGEOUT_START, msgin_stageout_start);

    return 0;
}
int setupBBproxyListener(string whoami);
int bb_main(std::string who)
{
    ENTRY_NO_CLOCK(__FILE__,__FUNCTION__);
    int rc = 0;
    stringstream errorText;

    try
    {
        LOG(bb,always) << "bbserver bb_main BBAPI_CLIENTVERSIONSTR="<<BBAPI_CLIENTVERSIONSTR;

        // Increase the number of allowed file descriptors...
        struct rlimit l_Limits;
        rc = getrlimit(RLIMIT_NOFILE, &l_Limits);
        if (rc)
        {
            rc = errno;
            errorText << "getrlimit failed";
            LOG_ERROR_TEXT_ERRNO_AND_BAIL(errorText, rc);
        }

        l_Limits.rlim_cur = l_Limits.rlim_max;
        rc = setrlimit(RLIMIT_NOFILE, &l_Limits);
        if (rc)
        {
            rc = errno;
            errorText << "setrlimit failed";
            LOG_ERROR_TEXT_ERRNO_AND_BAIL(errorText, rc);
        }
        LOG(bb,always) << "Maximum number of file descriptors set to " << l_Limits.rlim_cur;

        // Initialize values to be used by this bbServer instance
        wrkqmgr.setServerLoggingLevel(config.get(who + ".default_sev", "info"));
        ResizeSSD_TimeInterval = config.get("bb.bbserverResizeSSD_TimeInterval", DEFAULT_BBSERVER_RESIZE_SSD_TIME_INTERVAL);
        Throttle_TimeInterval = min(config.get("bb.bbserverThrottle_TimeInterval", DEFAULT_BBSERVER_THROTTLE_TIME_INTERVAL), MAXIMUM_BBSERVER_THROTTLE_TIME_INTERVAL);
        wrkqmgr.setThrottleTimerPoppedCount(Throttle_TimeInterval);
        LOG(bb,always) << "Timer interval is set to " << Throttle_TimeInterval << " seconds with a multiplier of " << wrkqmgr.getThrottleTimerPoppedCount() << " to implement throttle rate intervals";
        wrkqmgr.setHeartbeatTimerPoppedCount(Throttle_TimeInterval);
        wrkqmgr.setHeartbeatDumpPoppedCount(Throttle_TimeInterval);

        // NOTE: We will only dequeue from the high priority work queue if the current number of cancel requests
        //       (i.e., thread waiting for the canceled extents to be removed from a work queue) is consuming no more than 50%
        //       of the total number of transfer threads available.  We need to leave some transfer threads available to perform the
        //       efficient removal of canceled extents from the work queue(s).
        uint32_t l_NumberOfTransferThreads = (uint32_t)(config.get(resolveServerConfigKey("numTransferThreads"), DEFAULT_BBSERVER_NUMBER_OF_TRANSFER_THREADS));

        wrkqmgr.setNumberOfAllowedConcurrentCancelRequests(l_NumberOfTransferThreads >= 4 ? l_NumberOfTransferThreads/2 : 1);
        wrkqmgr.setAllowDumpOfWorkQueueMgr(config.get("bb.bbserverAllowDumpOfWorkQueueMgr", DEFAULT_ALLOW_DUMP_OF_WORKQUEUE_MGR));
        wrkqmgr.setDumpOnRemoveWorkItem(config.get("bb.bbserverDumpWorkQueueMgrOnRemoveWorkItem", DEFAULT_DUMP_MGR_ON_REMOVE_WORK_ITEM));
        wrkqmgr.setDumpOnDelay(config.get("bb.bbserverDumpWorkQueueMgrOnDelay", DEFAULT_DUMP_MGR_ON_DELAY));
        wrkqmgr.setDumpOnRemoveWorkItemInterval((uint64_t)(config.get("bb.bbserverDumpWorkQueueMgrOnRemoveWorkItemInterval", DEFAULT_DUMP_MGR_ON_REMOVE_WORK_ITEM_INTERVAL)));
        wrkqmgr.setDumpTimerPoppedCount(Throttle_TimeInterval);
        if (wrkqmgr.getDumpTimerPoppedCount())
        {
            LOG(bb,always) << "Timer interval is set to " << Throttle_TimeInterval << " seconds with a multiplier of " << wrkqmgr.getDumpTimerPoppedCount() << " to implement work queue manager dump intervals";
        }
        wrkqmgr.setNumberOfAllowedSkippedDumpRequests(config.get("bb.bbserverNumberOfAllowedSkippedDumpRequests", DEFAULT_NUMBER_OF_ALLOWED_SKIPPED_DUMP_REQUESTS));
        l_LockDebugLevel = config.get(who + ".bringup.lockDebugLevel", DEFAULT_LOCK_DEBUG_LEVEL);

        // Check for the existence of the file used to communicate high-priority async requests between instances
        // of bbServers.  Correct permissions are also ensured for the cross-bbServer metadata.
        char* l_AsyncRequestFileNamePtr = 0;
        int l_SeqNbr = 0;
        rc = wrkqmgr.verifyAsyncRequestFile(l_AsyncRequestFileNamePtr, l_SeqNbr, START_BBSERVER);
        if (l_AsyncRequestFileNamePtr)
        {
            delete [] l_AsyncRequestFileNamePtr;
            l_AsyncRequestFileNamePtr = 0;
        }
        if (rc) SET_RC_AND_BAIL(rc);

        /* Start memory/io fs/io monitor */
        uint64_t bbserverFileIOwarnSeconds = config.get("bb.bbserverFileIOwarnSeconds", 300);
        uint64_t bbserverFileIOStuckSeconds = config.get("bb.bbserverFileIOStuckSeconds", 600);
        setThresholdTimes4Syscall(bbserverFileIOwarnSeconds,  bbserverFileIOStuckSeconds);
        LOG(bb,info) << "bbserverFileIOwarnSeconds="<<bbserverFileIOwarnSeconds;
        LOG(bb,info) << "bbserverFileIOStuckSeconds="<<bbserverFileIOStuckSeconds;
        pthread_t tid;
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        pthread_create(&tid, &attr, mountMonitorThread, NULL);

        // Initialize SSD WriteDirect
        bool ssdwritedirect = config.get("bb.ssdwritedirect", true);
        LOG(bb,info) << "ssdwritedirect="<<ssdwritedirect;
        setSsdWriteDirect(ssdwritedirect);

        // Add a work queue for high priority cross bbserver 'requests'
        char FOXFOX = 0xFF;
        uuid_t l_HPWrkQEUuid_Value;
        for (unsigned int i=0; i<sizeof(l_HPWrkQEUuid_Value); i++) {
            l_HPWrkQEUuid_Value[i] = FOXFOX;
        }
        Uuid l_HPWrkQE_Uuid = Uuid(l_HPWrkQEUuid_Value);
        HPWrkQE_LVKeyStg = std::make_pair("None", l_HPWrkQE_Uuid);
        rc = wrkqmgr.addWrkQ(&HPWrkQE_LVKeyStg, (BBLV_Info*)0, (uint64_t)0, 0);
        if (!rc)
        {
            wrkqmgr.getWrkQE(HPWrkQE_LVKey, HPWrkQE);
            if (!HPWrkQE)
            {
                rc = -1;
	            errorText << "Error occurred when retrieving the high priority work queue";
                LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
            }
        }
        else
        {
            rc = -1;
	        errorText << "Error occurred when adding the high priority work queue";
            LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
        }

        // Log which jobs are currently known by the system in the cross bbServer metadata.
        // NOTE:  Our design depends on jobids NOT being reused, unless such jobids are first removed
        //        from all local metadata caches and the cross-bbServer metadata.  This may occur in
        //        test environments...
        uint32_t l_Count = 0;
        boost::filesystem::path datastore(config.get("bb.bbserverMetadataPath", DEFAULT_BBSERVER_METADATAPATH));
        if (boost::filesystem::is_directory(datastore))
        {
            bool l_AllDone = false;
            while (!l_AllDone)
            {
                l_AllDone = true;
                for (auto& job : boost::make_iterator_range(boost::filesystem::directory_iterator(datastore), {}))
                {
                    try
                    {
                        if (boost::filesystem::is_directory(job))
                        {
                            struct stat l_stat;
                            int rcVal = stat(job.path().c_str(), &l_stat);
                            std::string uid = "Unknown";
                            std::string gid = "Unknown";
                            if (!rcVal){
                                uid = to_string(l_stat.st_uid);
                                gid = to_string(l_stat.st_gid);
                            }
                            LOG(bb,warning) << "bb_main(): (" << ++l_Count << ") Job number " << job.path().filename().string() << " is already known to the cross-bbServer metadata"<<" uid="<<uid<<" gid="<<gid;;
                        }
                    }
                    catch(exception& e)
                    {
                        // More than likely, the job just got deleted out from under us...  Restart the walk of the job directories...
                        // NOTE: We could get multiple lines of output for jobs that already exist, but we will live with that...
                        l_AllDone = false;
                        break;
                    }
                }
            }
        }

        Throttle_Timer.forcePop();

        // NOTE: Transfer threads are started here so that async requests
        //       can be immediately honored from other bbServers.
        startTransferThreads();
        rc = setupBBproxyListener(who);
        if(rc)
       {
        stringstream errorText;
        errorText<<"Listening socket error.  rc=" << rc;
        LOG_ERROR_TEXT_RC_AND_RAS(errorText, rc, bb.net.bbproxyListenerSocketFailed);
       }
    }
    catch(ExceptionBailout& e) { }
    catch(exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

    EXIT_NO_CLOCK(__FILE__,__FUNCTION__);
    return rc;
}

int bb_exit(std::string who)
{
    ENTRY_NO_CLOCK(__FILE__,__FUNCTION__);

    EXIT_NO_CLOCK(__FILE__,__FUNCTION__);
    return 0;
}
