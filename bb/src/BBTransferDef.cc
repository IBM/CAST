/*******************************************************************************
 |    BBTransferDef.cc
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

#include "BBTransferDef.h"

using namespace std;
using namespace boost::archive;

#include "bberror.h"
#include "bbflags.h"
#include "bbinternal.h"
#include "bbio.h"
#include "BBLVKey_ExtentInfo.h"
#include "BBTagInfo.h"
#include "BBTagInfoMap.h"
#include "BBTagInfoMap2.h"

#include "connections.h"
#include "ContribFile.h"
#include "ContribIdFile.h"
#include "HandleFile.h"
#include "identity.h"
#include "LVUtils.h"
#include "LVUuidFile.h"


//
// BBTransferDefs class
//

BBTransferDefs::~BBTransferDefs()
{
    for (size_t i=0; i<transferdefs.size(); i++)
    {
        if (transferdefs[i])
        {
            delete transferdefs[i];
            transferdefs[i] = 0;
        }
    }
}


//
// BBTransferDefs - Static methods
//

#if BBSERVER
int BBTransferDefs::xbbServerRetrieveTransfers(BBTransferDefs& pTransferDefs)
{
    int rc = 0;
    stringstream errorText;

    HandleFile* l_HandleFile = 0;
    ContribFile* l_ContribFile = 0;
    bool l_HostNameFound = false;

    // Iterate through the jobs...
    bfs::path jobpath(config.get("bb.bbserverMetadataPath", DEFAULT_BBSERVER_METADATAPATH));
    bool l_AllDone = false;
    while (!l_AllDone)
    {
        l_AllDone = true;
        l_HostNameFound = false;
        for (auto& l_JobId : boost::make_iterator_range(bfs::directory_iterator(jobpath), {}))
        {
            try
            {
                if (!bfs::is_directory(l_JobId)) continue;
                if ((pTransferDefs.jobid == UNDEFINED_JOBID) || (l_JobId.path().filename().string() == to_string(pTransferDefs.jobid)))
                {
                    // Iterate through the jobsteps...
                    for (auto& l_JobStepId : boost::make_iterator_range(bfs::directory_iterator(l_JobId.path()), {}))
                    {
                        if ((pTransferDefs.jobstepid == UNDEFINED_JOBSTEPID) || (l_JobStepId.path().filename().string() == to_string(pTransferDefs.jobstepid)))
                        {
                            // Iterate through the handles...
                            for (auto& l_Handle : boost::make_iterator_range(bfs::directory_iterator(l_JobStepId.path()), {}))
                            {
                                if ((pTransferDefs.handle == UNDEFINED_HANDLE) || (l_Handle.path().filename().string() == to_string(pTransferDefs.handle)))
                                {
                                    // Handle of interest...
                                    if (l_HandleFile)
                                    {
                                        delete l_HandleFile;
                                        l_HandleFile = 0;
                                    }
                                    bfs::path handlefile = l_Handle.path() / bfs::path(l_Handle.path().filename());
                                    rc = HandleFile::loadHandleFile(l_HandleFile, handlefile.string().c_str());
                                    if (!rc)
                                    {
                                        // Early exit if we already have a 'final' status
                                        if ((pTransferDefs.flags == ALL_DEFINITIONS) || (!(l_HandleFile->status == BBFULLSUCCESS && l_HandleFile->status == BBCANCELED)))
                                        {
                                            // NOTE: Cannot early exit for ONLY_DEFINITIONS_WITH_UNFINISHED_FILES case based upon extents being processed
                                            //       because we want to also include transfer definitions with files that are not all closed or have failed.

                                            // Iterate through the logical volumes...
                                            bool l_Continue = true;
                                            for (auto& l_LVUuid : boost::make_iterator_range(bfs::directory_iterator(l_Handle.path()), {}))
                                            {
                                                if (!bfs::is_directory(l_LVUuid)) continue;
                                                l_Continue = true;
                                                bfs::path lvuuidfile = l_LVUuid.path() / l_LVUuid.path().filename();
                                                LVUuidFile l_LVUuidFile;
                                                rc = l_LVUuidFile.load(lvuuidfile.string());
                                                if (!rc)
                                                {
                                                    if ((pTransferDefs.flags & ALL_DEFINITIONS) || (!(l_LVUuidFile.flags & BBLVK_Stage_Out_End)))
                                                    {
                                                        // NOTE: Cannot early exit for ONLY_DEFINITIONS_WITH_UNFINISHED_FILES case based upon extents being processed
                                                        //       because we want to also include transfer definitions with files that are not all closed or have failed.

                                                        if (!((pTransferDefs.hostname == UNDEFINED_HOSTNAME) || (l_LVUuidFile.hostname == pTransferDefs.hostname)))
                                                        {
                                                            l_Continue = false;
                                                        }
                                                        // Note if we found a specifically named hostname...
                                                        if (l_LVUuidFile.hostname == pTransferDefs.hostname)
                                                        {
                                                            l_HostNameFound = true;
                                                        }

                                                        if (l_Continue)
                                                        {
                                                            if (l_ContribFile)
                                                            {
                                                                delete l_ContribFile;
                                                                l_ContribFile = 0;
                                                            }
                                                            bfs::path contribs_file = l_LVUuid.path() / "contribs";
                                                            rc = ContribFile::loadContribFile(l_ContribFile, contribs_file.c_str());
                                                            if (!rc)
                                                            {
                                                                // Iterate through the contributors...
                                                                for (map<uint32_t,ContribIdFile>::iterator ce = l_ContribFile->contribs.begin(); ce != l_ContribFile->contribs.end(); ce++)
                                                                {
                                                                    l_Continue = true;
                                                                    if ((pTransferDefs.contribid == UNDEFINED_CONTRIBID) || (ce->first == pTransferDefs.contribid))
                                                                    {
                                                                        // ContribId of interest...
                                                                        switch (pTransferDefs.flags)
                                                                        {
                                                                            case ONLY_DEFINITIONS_WITH_UNFINISHED_FILES:
                                                                            {
                                                                                // If we are only interested in obtaining transfer definitions with unfinished files
                                                                                // then check to see if all the extents have been transferred for this contributor...
                                                                                if ((ce->second).notRestartable())
                                                                                {
                                                                                    l_Continue = false;
                                                                                }
                                                                                break;
                                                                            }
                                                                            case ONLY_DEFINITIONS_WITH_STOPPED_FILES:
                                                                            {
                                                                                // If we are only interested in obtaining transfer definitions with stopped files
                                                                                // then check to see if this contributor has been stopped...
                                                                                if (!((ce->second).flags & BBTD_Stopped))
                                                                                {
                                                                                    l_Continue = false;
                                                                                }
                                                                                break;
                                                                            }

                                                                            default:
                                                                            {
                                                                                break;
                                                                            }
                                                                        }
                                                                    }
                                                                    else
                                                                    {
                                                                        l_Continue = false;
                                                                    }

                                                                    if ((!rc) && l_Continue)
                                                                    {
                                                                        // Copy this transfer definition...
                                                                        rc = (ce->second).copyForRetrieveTransferDefinitions(pTransferDefs, l_LVUuidFile.hostname, stoull(l_JobId.path().filename().string()), stoull(l_JobStepId.path().filename().string()),
                                                                                                                             stoull(l_Handle.path().filename().string()), ce->first, l_HandleFile->transferKeys);
                                                                    }
                                                                }
                                                            }
                                                            else
                                                            {
                                                                // NOTE:  We don't have the cross-bbserver data locked down, so it can 'change'...
                                                                LOG(bb,warning) << "Could not load contrib file " << contribs_file.string();
                                                            }
                                                        }
                                                    }
                                                    else
                                                    {
                                                        // Not retrieving all transfer definitions and stage out end has started.
                                                        // Do not include this transfer definition.
                                                    }
                                                }
                                                else
                                                {
                                                    // NOTE:  We don't have the cross-bbserver data locked down, so it can 'change'...
                                                    LOG(bb,warning) << "Could not load LVUuid file " << lvuuidfile.string();
                                                }
                                            }
                                        }
                                        else
                                        {
                                            // Status of handle file is a 'final' status and the request is not for all definitions.
                                            // Continue to next handle...
                                        }
                                    }
                                    else
                                    {
                                        // NOTE:  We don't have the cross-bbserver data locked down, so it can 'change'...
                                        LOG(bb,warning) << "Could not load handle file " << handlefile.string();
                                    }

                                    if (rc)
                                    {
                                        LOG(bb,warning) << "Failure when attempting to retrieve transfer definitions from the cross-bbserver metadata.  rc=" << rc;
                                        rc = 0;
                                        BAIL;
                                    }
                                }
                            }
                        }
                    }
                }
            }
            catch(ExceptionBailout& e) { }
            catch(exception& e)
            {
                // More than likely, the cross-bbServer data was altered under us...  Restart the walk of the job directories...
                LOG(bb,info) << "Attempting to retrieve transfer definitions from the cross-bbserver metadata again...";

                rc = 0;
                l_AllDone = false;
                break;
            }
        }
    }

    if (l_HandleFile)
    {
        delete l_HandleFile;
        l_HandleFile = 0;
    }

    if (l_ContribFile)
    {
        delete l_ContribFile;
        l_ContribFile = 0;
    }

    if ((!rc) && l_HostNameFound)
    {
        // We found a specifically named hostname...
        // NOTE:  This indication is not currently needed, but included for
        //        completeness of the overall design...
        rc = 2;
    }

    return rc;
}
#endif

//
// BBTransferDefs - Non-static methods
//

int BBTransferDefs::add(BBTransferDef* pTransferDef)
{
    int rc = 0;

    transferdefs.push_back(pTransferDef);

    return rc;
}

void BBTransferDefs::clear()
{
    while (transferdefs.size())
    {
        if (transferdefs.back())
        {
            delete transferdefs.back();
            transferdefs.pop_back();
        }
    }

    return;
}

void BBTransferDefs::demarshall(string& pMarshalledTransferDefs)
{
    stringstream l_Buffer;

    l_Buffer << pMarshalledTransferDefs;
    text_iarchive l_Archive(l_Buffer);
    l_Archive >> *this;

    return;
}

void BBTransferDefs::dump(const char* pSev, const char* pPrefix)
{
    int i = 1;

    if (!strcmp(pSev,"debug"))
    {
        LOG(bb,debug) << "Start: " << (pPrefix ? pPrefix : "Transfer Definitions");
    }
    else if (!strcmp(pSev,"info"))
    {
        LOG(bb,info) << "Start: " << (pPrefix ? pPrefix : "Transfer Definition");
    }

    char l_Prefix[64] = {'\0'};
    for (auto& def : transferdefs)
    {
        if (def)
        {
            snprintf(l_Prefix, sizeof(l_Prefix), "Definition number %d", i);
            def->dump(pSev, l_Prefix);
        }
        ++i;
    }

    if (!strcmp(pSev,"debug"))
    {
        LOG(bb,debug) << "  End: " << (pPrefix ? pPrefix : "Transfer Definition");
    }
    else if (!strcmp(pSev,"info"))
    {
        LOG(bb,info) << "  End: " << (pPrefix ? pPrefix : "Transfer Definitions");
    }

    return;
}

string BBTransferDefs::marshall()
{
    stringstream l_Buffer;

    text_oarchive l_Archive(l_Buffer);
    l_Archive << *this;

    return l_Buffer.str();
}

#if BBPROXY
void BBTransferDefs::restartTransfers(const string& pHostName, const uint64_t pJobId, const uint64_t pJobStepId, const uint64_t pHandle, const uint32_t pContribId, uint32_t& pNumRestartedTransferDefs)
{
    int rc = 0;
    uint32_t l_NumberFailed = 0;
    uint32_t l_NumberRestarted = 0;
    uint32_t l_NotInStoppedState = 0;

    pNumRestartedTransferDefs = 0;

    for (size_t i=0; ((!rc) && i<transferdefs.size()); i++)
    {
        if ((pHostName == UNDEFINED_HOSTNAME || transferdefs[i]->getHostName() == pHostName) &&
            (pJobId == UNDEFINED_JOBID || transferdefs[i]->getJobId() == pJobId) &&
            (pJobStepId == UNDEFINED_JOBSTEPID || transferdefs[i]->getJobStepId() == pJobStepId) &&
            (pHandle == UNDEFINED_HANDLE || transferdefs[i]->getTransferHandle() == pHandle) &&
            (pContribId == UNDEFINED_CONTRIBID || transferdefs[i]->getContribId() == pContribId))
        {
            // Found a transfer definition in the archive where the file transfers should be restarted.
            rc = becomeUser(transferdefs[i]->getUserId(), transferdefs[i]->getGroupId());
            if (!rc)
            {
                if (config.get(process_whoami+".bringup.dumpTransferDefinitionAfterDemarshall", 0)) {
                    transferdefs[i]->dump("info", "restart - demarshallTransferDef");
                }

                rc = startTransfer(transferdefs[i], transferdefs[i]->getJobId(), transferdefs[i]->getJobStepId(), transferdefs[i]->getTransferHandle(), transferdefs[i]->getContribId());
                switch (rc)
                {
                    case -2:
                    {
                        ++l_NotInStoppedState;
                        LOG(bb,info) << "Transfer definition associated with jobid " << transferdefs[i]->getJobId() << ", jobstepid " << transferdefs[i]->getJobStepId() \
                                     << ", handle " << transferdefs[i]->getTransferHandle() << ", contribid " << transferdefs[i]->getContribId() \
                                     << " was not restarted because it was not in a stopped state." \
                                     << " If a prior attempt was made to stop the transfer, it may have not been stopped" \
                                     << " because transfers for all extents had already completed.  See prior messages for this handle.";

                        // Clear bberror for the tolerated exception
                        bberror.clear();

                        break;
                    }

                    case 0:
                    {
                        ++l_NumberRestarted;
                        LOG(bb,info) << "Transfer definition associated with jobid " << transferdefs[i]->getJobId() << ", jobstepid " << transferdefs[i]->getJobStepId() \
                                     << ", handle " << transferdefs[i]->getTransferHandle() << ", contribid " << transferdefs[i]->getContribId() \
                                     << " was restarted";

                        break;
                    }

                    default:
                    {
                        // Failed restart...
                        ++l_NumberFailed;
                        LOG(bb,info) << "Transfer definition associated with jobid " << transferdefs[i]->getJobId() << ", jobstepid " << transferdefs[i]->getJobStepId() \
                                     << ", handle " << transferdefs[i]->getTransferHandle() << ", contribid " << transferdefs[i]->getContribId() \
                                     << " failed.  The job may have exited the system.";

                        break;
                    }
                }
                rc = 0;
                becomeUser(0,0);
            }
            else
            {
                rc = -1;
                stringstream errorText;
                errorText << "becomeUser failed when attempting to restart the transfer definition associated with jobid " << transferdefs[i]->getJobId()
                          << ", jobstepid " << transferdefs[i]->getJobStepId() << ", handle " << transferdefs[i]->getTransferHandle() << ", contribId " << transferdefs[i]->getContribId()
                          << " when attempting to become uid=" << transferdefs[i]->getUserId() << ", gid=" << transferdefs[i]->getGroupId();
                bberror << err("error.uid", transferdefs[i]->getUserId()) << err("error.gid", transferdefs[i]->getGroupId());
                LOG_ERROR_TEXT_RC(errorText, rc);
            }
        }
    }

    if (!rc)
    {
        string l_HostNamePrt1 = "For host name " + hostname;
        if (hostname == UNDEFINED_HOSTNAME)
        {
            l_HostNamePrt1 = "For all host names";
        }
        string l_HostNamePrt2 = "host name " + hostname;
        if (hostname == UNDEFINED_HOSTNAME)
        {
            l_HostNamePrt2 = "all host names";
        }

        ostringstream l_ContribIdPrt;
        l_ContribIdPrt << pContribId;
        string l_ContribIdPrt1 = "contribid " + l_ContribIdPrt.str();
        if (pContribId == UNDEFINED_CONTRIBID)
        {
            l_ContribIdPrt1 = "all contribids";
        }
        l_ContribIdPrt.str("");
        l_ContribIdPrt.clear();
        l_ContribIdPrt << contribid;
        string l_ContribIdPrt2 = "contribid " + l_ContribIdPrt.str();
        if (contribid == UNDEFINED_CONTRIBID)
        {
            l_ContribIdPrt2 = "all contribids";
        }

        bberror.errdirect("out.numTransferDefs", transferdefs.size());
        bberror.errdirect("out.numberRestarted", l_NumberRestarted);
        bberror.errdirect("out.numberNotInStoppedState", l_NotInStoppedState);
        bberror.errdirect("out.numberFailed", l_NumberFailed);
        LOG(bb,info) << l_HostNamePrt1 << ", jobid " << pJobId << ", jobstepid " << pJobStepId << ", handle " << pHandle << ", and " << l_ContribIdPrt1 \
                     << " using an archive of " << transferdefs.size() << " transfer definition(s) generated using the criteria of " \
                     << l_HostNamePrt2 << ", jobid " << jobid << ", jobstepid " << jobstepid << ", handle " << handle << ", and " << l_ContribIdPrt2 \
                     << ", a restart operation was completed for " << l_NumberRestarted << " transfer definition(s), " << l_NotInStoppedState \
                     << " transfer definition(s) were not in a stopped state, and a restart was attempted but failed for " \
                     << l_NumberFailed << " transfer definition(s) . See previous messages for additional details.";

        pNumRestartedTransferDefs = l_NumberRestarted;
    }

    return;
}
#endif

#if BBSERVER
void BBTransferDefs::stopTransfers(const string& pHostName, const uint64_t pJobId, const uint64_t pJobStepId, const uint64_t pHandle, const uint32_t pContribId, uint32_t& pNumStoppedTransferDefs)
{
    int rc = 0;
    uint32_t l_NotFound = 0;
    uint32_t l_Stopped = 0;
    uint32_t l_Finished = 0;
    uint32_t l_AlreadyStopped = 0;
    uint32_t l_AlreadyCanceled = 0;
    uint32_t l_Failed = 0;
    uint32_t l_DidNotMatchSelectionCriteria = 0;
    uint32_t l_NotProcessed = (uint32_t)transferdefs.size();

    pNumStoppedTransferDefs = 0;

    for (size_t i=0; (rc != -1) && i<transferdefs.size(); i++)
    {
        if ((pHostName == UNDEFINED_HOSTNAME || transferdefs[i]->getHostName() == pHostName) &&
            (pJobId == UNDEFINED_JOBID || transferdefs[i]->getJobId() == pJobId) &&
            (pJobStepId == UNDEFINED_JOBSTEPID || transferdefs[i]->getJobStepId() == pJobStepId) &&
            (pHandle == UNDEFINED_HANDLE || transferdefs[i]->getTransferHandle() == pHandle) &&
            (pContribId == UNDEFINED_CONTRIBID || transferdefs[i]->getContribId() == pContribId))
        {
            // Found a transfer definition in the archive where the file transfers should be stopped.
            string l_HostName;
            activecontroller->gethostname(l_HostName);
            rc = metadata.stopTransfer(l_HostName, transferdefs[i]->getHostName(), transferdefs[i]->getJobId(), transferdefs[i]->getJobStepId(), transferdefs[i]->getTransferHandle(), transferdefs[i]->getContribId());
            switch (rc)
            {
                case 0:
                {
                    // The transfer definition being searched for could not be found with this LVKey.
                    // Continue to the next LVKey...
                    ++l_NotFound;

                    break;
                }

                case 1:
                {
                    // Found the transfer definition on this bbServer.
                    // It was processed, and operation logged.
                    // The cross bbserver metadata was also appropriately reset
                    // as part of the operation.
                    ++l_Stopped;

                    break;
                }

                case 2:
                {
                    // Found the transfer definition on this bbServer.
                    // However, no extents were left to be transferred.
                    // Situation was logged, and nothing more to do...
                    ++l_Finished;

                    break;
                }

                case 3:
                {
                    // Found the transfer definition on this bbServer.
                    // However, it was already in a stopped state.
                    // Situation was logged, and nothing more to do...
                    ++l_AlreadyStopped;

                    break;
                }

                case 4:
                {
                    // Found the transfer definition on this bbServer.
                    // However, it was already in a canceled state.
                    // Situation was logged, and nothing more to do...
                    ++l_AlreadyCanceled;

                    break;
                }

                case -2:
                {
                    // Recoverable error occurred....  Log it and continue...
                    ++l_Failed;
                    LOG(bb,error) << "Failed when processing a stop transfer request for CN hostname " << pHostName << ", jobid " << pJobId << ", jobstepid " << pJobStepId
                                  << ", handle " << pHandle << ", contribId " << pContribId << ", rc=" << rc << ". Stop transfer processing continues for the remaining transfer definitions.";

                    break;
                }

                default:
                {
                    // Fatal error occurred....  Log it and exit...
                    LOG(bb,error) << "Failed when processing a stop transfer request for CN hostname " << pHostName << ", jobid " << pJobId << ", jobstepid " << pJobStepId
                                  << ", handle " << pHandle << ", contribId " << pContribId << ", rc=" << rc << ". Stop transfer processing is ended for the remainder of the transfer definitions.";

                    break;
                }
            }
        }
        else
        {
            l_DidNotMatchSelectionCriteria++;
        }
        --l_NotProcessed;
    }

    string l_HostNamePrt1 = "For host name " + pHostName + ", ";
    if (hostname == UNDEFINED_HOSTNAME)
    {
        l_HostNamePrt1 = "For all host names, ";
    }
    string l_HostNamePrt2 = "host name " + hostname;
    if (hostname == UNDEFINED_HOSTNAME)
    {
        l_HostNamePrt2 = "all host names";
    }

    ostringstream l_ContribIdPrt;
    l_ContribIdPrt << pContribId;
    string l_ContribIdPrt1 = "contribid " + l_ContribIdPrt.str();
    if (pContribId == UNDEFINED_CONTRIBID)
    {
        l_ContribIdPrt1 = "all contribids";
    }
    l_ContribIdPrt.str("");
    l_ContribIdPrt.clear();
    l_ContribIdPrt << contribid;
    string l_ContribIdPrt2 = "contribid " + l_ContribIdPrt.str();
    if (contribid == UNDEFINED_CONTRIBID)
    {
        l_ContribIdPrt2 = "all contribids";
    }

    bberror.errdirect("out.#TransferDefs", transferdefs.size());
    bberror.errdirect("out.numberNotFoundOnThisServer", l_NotFound);
    bberror.errdirect("out.numberStopped", l_Stopped);
    bberror.errdirect("out.numberAlreadyFinished", l_Finished);
    bberror.errdirect("out.numberAlreadyStopped", l_AlreadyStopped);
    bberror.errdirect("out.numberAlreadyCanceled", l_AlreadyCanceled);
    bberror.errdirect("out.numberFailed", l_Failed);
    bberror.errdirect("out.numberNotMatchingSelectionCriteria", l_DidNotMatchSelectionCriteria);
    bberror.errdirect("out.numberNotProcessed", l_NotProcessed);

    string l_ServerHostName;
    activecontroller->gethostname(l_ServerHostName);

    LOG(bb,info) << l_HostNamePrt1 << "jobid " << pJobId << ", jobstepid " << pJobStepId \
                 << ", handle " << pHandle << ", and " << l_ContribIdPrt1 \
                 << " using an archive of " << transferdefs.size() << " transfer definition(s) generated using the criteria of " \
                 << l_HostNamePrt2 << ", jobid " << jobid << ", jobstepid " << jobstepid << ", handle " << handle << ", and " << l_ContribIdPrt2 \
                 << ", a stop transfer operation was completed for " << l_Stopped << " transfer definition(s), " \
                 << l_NotFound << " transfer definition(s) were not found on the bbServer at " << l_ServerHostName << ", " << l_Finished \
                 << " transfer definition(s) were already finished, " << l_AlreadyStopped << " transfer definition(s) were already stopped, " \
                 << l_AlreadyCanceled << " transfer definition(s) were already canceled, " \
                 << l_DidNotMatchSelectionCriteria << " transfer definition(s) did not match the selection criteria, " \
                 << l_NotProcessed << " transfer definition(s) were not processed, and " << l_Failed << " failure(s) occurred during this processing." \
                 << " See previous messages for additional details.";

    pNumStoppedTransferDefs = l_AlreadyStopped + l_Stopped;

    return;
}
#endif


//
// BBTransferDef class
//

//
// BBTransferDef - Static methods
//

/**
   \brief Demarshalls data from a txp Message to BBTransferDef.

   \param[in]    msg Message pointer to construct
   \param[in]    transfer Transfer definition to send
   \param[inout] arr vector of CharArrays.  Caller is responsible to deallocate once message has been written or discarded.

 */
int BBTransferDef::demarshallTransferDef(txp::Msg* msg, BBTransferDef* transfer)
{
    transfer->serializeVersion = ((txp::Attr_uint32*)msg->retrieveAttrs()->at(txp::serializeversion))->getData();
    transfer->objectVersion = ((txp::Attr_uint32*)msg->retrieveAttrs()->at(txp::objectversion))->getData();
    transfer->setJob(((txp::Attr_uint64*)msg->retrieveAttrs()->at(txp::bbjob_serializeversion))->getData(), ((txp::Attr_uint64*)msg->retrieveAttrs()->at(txp::bbjob_objectversion))->getData(),
                     ((txp::Attr_uint64*)msg->retrieveAttrs()->at(txp::jobid))->getData(), ((txp::Attr_uint64*)msg->retrieveAttrs()->at(txp::jobstepid))->getData());
    transfer->uid = (uid_t)((txp::Attr_int32*)msg->retrieveAttrs()->at(txp::userid))->getData();
    transfer->gid = (gid_t)((txp::Attr_int32*)msg->retrieveAttrs()->at(txp::groupid))->getData();
    transfer->contribid = ((txp::Attr_int32*)msg->retrieveAttrs()->at(txp::transfercontribid))->getData();
    transfer->flags = ((txp::Attr_uint64*)msg->retrieveAttrs()->at(txp::flags))->getData();
    transfer->tag = ((txp::Attr_uint64*)msg->retrieveAttrs()->at(txp::tag))->getData();
    transfer->transferHandle = ((txp::Attr_uint64*)msg->retrieveAttrs()->at(txp::transferhandle))->getData();
    transfer->hostname.assign((const char*)msg->retrieveAttrs()->at(txp::hostname)->getDataPtr(), (uint64_t)(msg->retrieveAttrs()->at(txp::hostname)->getDataLength()));

    uint64_t numfiles   = ((txp::AttrPtr_array_of_char_arrays*)msg->retrieveAttrs()->at(txp::files))->getNumberOfElementsArrayOfCharArrays();
    uint64_t numkeys    = ((txp::AttrPtr_array_of_char_arrays*)msg->retrieveAttrs()->at(txp::keys))->getNumberOfElementsArrayOfCharArrays();
    uint64_t numextents = ((txp::AttrPtr_array_of_char_arrays*)msg->retrieveAttrs()->at(txp::extents))->getNumberOfElementsArrayOfCharArrays();

    txp::CharArray* files         = (txp::CharArray*)msg->retrieveAttrs()->at(txp::files)->getDataPtr();
    txp::CharArray* files_keys    = (txp::CharArray*)msg->retrieveAttrs()->at(txp::keys)->getDataPtr();
    txp::CharArray* files_values  = (txp::CharArray*)msg->retrieveAttrs()->at(txp::values)->getDataPtr();
    txp::CharArray* files_extents = (txp::CharArray*)msg->retrieveAttrs()->at(txp::extents)->getDataPtr();

    uint64_t x;
    for(x=0; x<numfiles; x++)
    {
        transfer->files.push_back((*files)[x].second);
    }

    for(x=0; x<numkeys; x++)
    {
        transfer->keyvalues[(*files_keys)[x].second] = (*files_values)[x].second;
    }

    for(x=0; x<numextents; x++)
    {
        Extent* tmp = (Extent*)(*files_extents)[x].second;
        transfer->extents.push_back(*tmp);
    }

    if (numextents && config.get(process_whoami+".bringup.dumpExtentsAfterDemarshall", 0)) {
        transfer->dumpExtents("info", "demarshallTransferDef");
    }

    return 0;
}

size_t BBTransferDef::getTotalTransferSize(BBTransferDef* pTransferDef) {
    size_t l_TotalSize = 0;

    if (pTransferDef->extents.size()) {
        for (auto& e : pTransferDef->extents) {
            l_TotalSize += e.len;
        }
    }

    return l_TotalSize;
}

size_t BBTransferDef::getTotalTransferSize(BBTransferDef* pTransferDef, const uint32_t pSourceIndex) {
    size_t l_TotalSize = 0;

    if (pTransferDef->extents.size()) {
        for (auto& e : pTransferDef->extents) {
            if (e.sourceindex == pSourceIndex) {
                l_TotalSize += e.len;
            }
        }
    }

    return l_TotalSize;
}

/**
   \brief Marshalls data from a BBTransferDef into a txp Message.

   \note
   The  vector <txp::CharArray> arr parameter is intended for the caller to reclaim storage after the message has been sent.
   The easiest implementation would be the vector residing on the stack, with the message being sent before the caller returns (and pops the stack)

   \note
   Caller must not make any changes to the transfer definition between calling marshallTransferDef() and deallocating the vector<txp::CharArray>.

   \param[in]    msg Message pointer to construct
   \param[in]    transfer Transfer definition to send
   \param[inout] arr vector of CharArrays.  Caller is responsible to deallocate once message has been written or discarded.

 */
int BBTransferDef::marshallTransferDef(txp::Msg* msg, BBTransferDef* transfer, vector<txp::CharArray> &arr)
{
    arr.resize(4);
    for(auto& e : transfer->files)
    {
        arr[0].push_back(make_pair(e.length()+1, (char*)e.c_str()));
    }
    for(auto& e : transfer->keyvalues)
    {
        arr[1].push_back(make_pair(e.first.length()+1, (char*)e.first.c_str()));
        arr[2].push_back(make_pair(e.second.length()+1, (char*)e.second.c_str()));
    }
//    LOG(bb,info) << "marshallTransferDef: transfer->extents.size()=" << transfer->extents.size();
    for(auto& e : transfer->extents)
    {
        arr[3].push_back(make_pair(sizeof(Extent), (char*)&(e)));
    }

    msg->addAttribute(txp::files, &arr[0]);
    msg->addAttribute(txp::keys, &arr[1]);
    msg->addAttribute(txp::values, &arr[2]);
    msg->addAttribute(txp::extents, &arr[3]);
    msg->addAttribute(txp::serializeversion, transfer->getSerializeVersion());
    msg->addAttribute(txp::objectversion, transfer->getObjectVersion());
    msg->addAttribute(txp::bbjob_serializeversion, transfer->getJobSerializeVersion());
    msg->addAttribute(txp::bbjob_objectversion, transfer->getJobObjectVersion());
    msg->addAttribute(txp::jobid, transfer->getJobId());
    msg->addAttribute(txp::jobstepid, transfer->getJobStepId());
    msg->addAttribute(txp::userid, transfer->uid);
    msg->addAttribute(txp::groupid, transfer->gid);
    msg->addAttribute(txp::transfercontribid, transfer->contribid);
    msg->addAttribute(txp::flags, transfer->flags);
    msg->addAttribute(txp::tag, transfer->tag);
    msg->addAttribute(txp::transferhandle, transfer->transferHandle);
    msg->addAttribute(txp::hostname, transfer->hostname.c_str(), (transfer->hostname.size())+1);

    return 0;
}


//
// BBTransferDef - Non-static methods
//

BBTransferDef::~BBTransferDef()
{
    cleanUpIOMap();
}

void BBTransferDef::cleanUpIOMap() {
    for (map<uint16_t, BBIO*>::size_type i = 0; i < iomap.size(); ++i)
    {
        if (iomap[i])
        {
            iomap[i]->closeAllFileHandles();

            delete iomap[i];
            iomap[i] = 0;
        }
    }

    iomap.clear();

    return;
}

void BBTransferDef::cleanUp() {
    // \todo - Not sure what we want to do here...  @DLH

    return;
}

#if BBSERVER
void BBTransferDef::copyExtentsForRetrieveTransferDefinitions(BBTransferDef* pSourceTransferDef, BBLVKey_ExtentInfo* pExtentInfo)
{
    for (size_t i=0; i<files.size(); i=i+2)
    {
        Extent* l_ExtentPtr = pExtentInfo->getAnySourceExtent(transferHandle, contribid, (uint32_t)i);
        if (l_ExtentPtr)
        {
            extents.push_back(Extent(l_ExtentPtr->getSourceIndex(), l_ExtentPtr->getTargetIndex(), (l_ExtentPtr->getFlags() & BB_AddFilesFlagsMask)));
            LOG(bb,info) << "When using this archive for restart, the source file associated with jobid " \
                         << job.getJobId() << ", jobstepid " << job.getJobStepId() << ", handle " << transferHandle << ", contrib " \
                         << contribid << ",  source index " << i << " MAY be restarted because not all extents" \
                         << " have yet been transfered.";
        }
        else
        {
            pSourceTransferDef->dump("info", "Retrieve and no extents in work queue for this definition");
            l_ExtentPtr = pSourceTransferDef->getAnySourceExtent((uint32_t)i);
            if (l_ExtentPtr)
            {
                extents.push_back(Extent(l_ExtentPtr->getSourceIndex(), l_ExtentPtr->getTargetIndex(), (l_ExtentPtr->getFlags() & BB_AddFilesFlagsMask)));
                LOG(bb,info) << "When using this archive for restart, the source file associated with jobid " \
                             << job.getJobId() << ", jobstepid " << job.getJobStepId() << ", handle " << transferHandle << ", contrib " \
                             << contribid << ",  source index " << i << " MAY be restarted, even though there were no extents left to be sent on the" \
                             << " work queue.  This may be due to a retrieve operation being performed after a stop operation has already been processed" \
                             << " for the transfer definition.";
            }
            else
            {
                // \todo - Not sure about this...  I think this is simply an error case we shouldn't get to....
                //         More investigation needed...  @DLH
                // NOTE: If we did not find any extent in the queue of extents for the input specs,
                //       any/all extents for the files must have been in the in-flight queue.
                //       If so, we will not restart the transfer for that file, as the transfers
                //       for all extents will complete before the restart operation is issued for
                //       the transfer definition (restart ensures it...).  Therefore, there is no
                //       need to include an extent for this file pair.
                // NOTE: \todo - Not sure what to do if I/O is 'stuck' for the any of this file's extents...  @DLH
                LOG(bb,info) << "When using this archive for restart, the source file associated with jobid " \
                             << job.getJobId() << ", jobstepid " << job.getJobStepId() << ", handle " << transferHandle << ", contrib " \
                             << contribid << ",  source index " << i << " WILL NOT be restarted because all extents" \
                             << " have already been transfered.";
            }
        }
    }

    return;
}

int BBTransferDef::copyForRetrieveTransferDefinitions(BBTransferDefs& pTransferDefs, BBLVKey_ExtentInfo* pExtentInfo)
{
    int rc = 0;

    BBTransferDef* l_TransferDef = new BBTransferDef();
    l_TransferDef->serializeVersion = serializeVersion;
    l_TransferDef->objectVersion = objectVersion;
    l_TransferDef->job = job;
    l_TransferDef->uid = uid;
    l_TransferDef->gid = gid;
    l_TransferDef->contribid = contribid;
    l_TransferDef->flags = flags & BB_RetrieveTransferDefinitionsFlagsMask;
    l_TransferDef->tag = tag;
    l_TransferDef->transferHandle = transferHandle;
    l_TransferDef->hostname = hostname;
    l_TransferDef->update = PTHREAD_MUTEX_INITIALIZER;  // Simply initialized
    l_TransferDef->files = files;
    l_TransferDef->keyvalues = keyvalues;
    l_TransferDef->iomap = map<uint16_t, BBIO*>();      // No BBIO objects copied
    l_TransferDef->extents = vector<Extent>();                                      // Create an empty extent vector
    l_TransferDef->copyExtentsForRetrieveTransferDefinitions(this, pExtentInfo);    // Copy a single extent from each stopped file
    l_TransferDef->sizeTransferred = vector<size_t>();  // No transfer sizes copied
    // NOTE: We don't currently save the values in the sizeTransferred vector because they are not
    //       needed as part of any restart transfer definition processing.  Only those files
    //       that have been successfully transferred and have reported their close back to bbServer
    //       have already updated the total transferred size for the handle.  However, those files
    //       will not be restarted anyway, so those values are not needed.  For other files that
    //       will be restarted, the size transferred for the individual files will be reset to zero.
    //       When they successfully complete their transfer of data and report their close back
    //       to the bbServer after the restart, then those transfer sizes w ill be added to the
    //       total transferred size for the handle.  Therefore, no 'old' values from this vector
    //       are needed as part of restart.
//    l_TransferDef->sizeTransferred = sizeTransferred;

    l_TransferDef->setBuiltViaRetrieveTransferDefinition();

    pTransferDefs.add(l_TransferDef);

    return rc;
}
#endif

void BBTransferDef::dumpExtents(const char* pSev, const char* pPrefix) const {
    if (extents.size()) {
        if (!strcmp(pSev,"debug")) {
            LOG(bb,debug) << ">>>>> Start: " << (pPrefix ? pPrefix : "Extent Vector") << ", " << extents.size() \
                          << (extents.size()==1 ? " extent <<<<<" : " extents <<<<<");
            for (auto& e : extents)
            {
                LOG(bb,debug) << e;
            }
            LOG(bb,debug) << ">>>>>   End: " << (pPrefix ? pPrefix : "Extent Vector") << ", " << extents.size() \
                          << (extents.size()==1 ? " extent <<<<<" : " extents <<<<<");
        } else if (!strcmp(pSev,"info")) {
            LOG(bb,info) << ">>>>> Start: " << (pPrefix ? pPrefix : "Extent Vector") << ", " << extents.size() \
                         << (extents.size()==1 ? " extent <<<<<" : " extents <<<<<");
            for (auto& e : extents)
            {
                LOG(bb,info) << e;
            }
            LOG(bb,info) << ">>>>>   End: " << (pPrefix ? pPrefix : "Extent Vector") << ", " << extents.size() \
                         << (extents.size()==1 ? " extent <<<<<" : " extents <<<<<");
        }
    }

    return;
}

void BBTransferDef::dump(const char* pSev, const char* pPrefix) {
    stringstream l_Job;
    job.getStr(l_Job);
    if (!strcmp(pSev,"debug")) {
        LOG(bb,debug) << "Start: " << (pPrefix ? pPrefix : "Transfer Definition");
        DUMP_TRANSDEF(debug,l_Job.str());
//        dumpExtents(pSev);
        LOG(bb,debug) << "Extent Vector has " << extents.size() \
                      << (extents.size()==1 ? " extent <<<<<" : " extents <<<<<");
        LOG(bb,debug) << "  End: " << (pPrefix ? pPrefix : "Transfer Definition");
    } else if (!strcmp(pSev,"info")) {
        LOG(bb,info) << "Start: " << (pPrefix ? pPrefix : "Transfer Definition");
        DUMP_TRANSDEF(info,l_Job.str());
//        dumpExtents(pSev);
        LOG(bb,info) << "Extent Vector has " << extents.size() \
                      << (extents.size()==1 ? " extent <<<<<" : " extents <<<<<");
        LOG(bb,info) << "  End: " << (pPrefix ? pPrefix : "Transfer Definition");
    }

    return;
}

#if BBSERVER
Extent* BBTransferDef::getAnySourceExtent(const uint32_t pSourceIndex)
{
    Extent* l_Extent = 0;

    for (size_t i=0; i<extents.size(); ++i)
    {
        if (pSourceIndex == extents[i].getSourceIndex())
        {
            l_Extent = &extents[i];
            break;
        }
    }

    return l_Extent;
}

Extent* BBTransferDef::getAnyTargetExtent(const uint32_t pTargetIndex)
{
    Extent* l_Extent = 0;

    for (size_t i=0; i<extents.size(); ++i)
    {
        if (pTargetIndex == extents[i].getTargetIndex())
        {
            l_Extent = &extents[i];
            break;
        }
    }

    return l_Extent;
}

BBFILESTATUS BBTransferDef::getFileStatus(const LVKey* pLVKey, ExtentInfo& pExtentInfo) {
    BBFILESTATUS l_FileStatus = BBFILE_NONE;

    ContribIdFile* l_ContribIdFile = 0;
    bfs::path l_HandleFilePath(config.get("bb.bbserverMetadataPath", DEFAULT_BBSERVER_METADATAPATH));
    l_HandleFilePath /= bfs::path(to_string(getJobId()));
    l_HandleFilePath /= bfs::path(to_string(getJobStepId()));
    l_HandleFilePath /= bfs::path(to_string(pExtentInfo.getHandle()));
    int l_RC = ContribIdFile::loadContribIdFile(l_ContribIdFile, pLVKey, l_HandleFilePath, pExtentInfo.getContrib());
    switch (l_RC)
    {
        case 1:
        {
            // NOTE: First check for 'stopped", 'failed' and then 'canceled' in that order.
            // NOTE: For file status, we will indicate that the status is BBFILE_SUCCESS
            //       if all the extents have been processed.  In a restart case where the
            //       file close was not reported back to the original bbServer, this file
            //       may still be restarted and transfered again.
            Extent* l_Extent = pExtentInfo.getExtent();
            uint32_t l_FileIndex = (l_Extent->sourceindex)/2;
            ContribIdFile::FileData* l_FileData = &(l_ContribIdFile->files[l_FileIndex]);
            if (l_FileData->flags & BBTD_Stopped)
            {
                l_FileStatus = BBFILE_STOPPED;
            }
            else if (l_FileData->flags & BBTD_Failed)
            {
                l_FileStatus = BBFILE_FAILED;
            }
            else if (l_FileData->flags & BBTD_Canceled)
            {
                l_FileStatus = BBFILE_CANCELED;
            }
            else if (l_FileData->flags & BBTD_All_Extents_Transferred)
            {
                l_FileStatus = BBFILE_SUCCESS;
            }
            else
            {
                // Leave as BBFILE_NONE
            }
            break;
        }
        case 0:
        {
            LOG(bb,error) << "ContribId " << pExtentInfo.getContrib() << "could not be found in the contribid file for jobid " << getJobId() << ", jobstepid " << getJobStepId() << ", handle " << pExtentInfo.getHandle() << ", " << *pLVKey << ", using handle path " << l_HandleFilePath;

            break;
        }
        default:
        {
            LOG(bb,error) << "Could not load the contribid file for jobid " << getJobId() << ", jobstepid " << getJobStepId() << ", handle " << pExtentInfo.getHandle() << ", contribid " << pExtentInfo.getContrib() << ", " << *pLVKey << ", using handle path " << l_HandleFilePath;
        }
    }

    if (l_ContribIdFile)
    {
        delete l_ContribIdFile;
        l_ContribIdFile = NULL;
    }

    return l_FileStatus;
}
#endif

BBSTATUS BBTransferDef::getStatus() {
    BBSTATUS l_Status = BBNONE;

    if (!stopped()) {
        if (!failed()) {
            if (!canceled()) {
                if (!allExtentsTransferred()) {
                    l_Status = BBINPROGRESS;
                } else {
                    l_Status = BBFULLSUCCESS;
                }
            } else {
                l_Status = BBCANCELED;
            }
        } else {
            l_Status = BBFAILED;
        }
    } else {
        l_Status = BBSTOPPED;
    }

    return l_Status;
}

void BBTransferDef::incrSizeTransferred(Extent* pExtent) {
    sizeTransferred[pExtent->getSourceIndex()] += pExtent->getLength();

    return;
}

void BBTransferDef::lock() {
    pthread_mutex_lock(&update);

    return;
}

#if BBSERVER
void BBTransferDef::markAsStopped(const LVKey* pLVKey, const uint64_t pHandle, const uint32_t pContribId)
{
    // Mark this transfer definition as stopped
    setStopped(pLVKey, pHandle, pContribId);

    // Mark this transfer definition as canceled
    setCanceled(pLVKey, pHandle, pContribId);

    return;
}

int BBTransferDef::prepareForRestart(const LVKey* pLVKey, const BBJob pJob, const uint64_t pHandle, const int32_t pContribId, BBTransferDef* pRebuiltTransferDef, const int pPass)
{
    int rc = 0;

    if (pPass == SECOND_PASS)
    {
        // Clean up the transfer definition for restart...
        // NOTE:
        cleanUpIOMap();
    }
    else if (pPass == THIRD_PASS)
    {
        // First, reset the following transfer definition/ContribId file flags
        setAllFilesClosed(pLVKey, pHandle, pContribId, 0);
        setAllExtentsTransferred(pLVKey, pHandle, pContribId, 0);
        setExtentsEnqueued(pLVKey, pHandle, pContribId, 0);
        setCanceled(pLVKey, pHandle, pContribId, 0);
        setFailed(pLVKey, pHandle, pContribId, 0);
        setStopped(pLVKey, pHandle, pContribId, 0);

        // Next, reset the appropriate flags for each individual file transfer to be restarted.
        // The size transferred for the file in the local metadata is also reset.
        int64_t l_Size = 0;
        rc = ContribIdFile::update_xbbServerFileStatusForRestart(pLVKey, pRebuiltTransferDef, pHandle, pContribId, l_Size);

        if (!rc)
        {
            // Update the handle status to recalculate the total transfer size
            HandleFile::update_xbbServerHandleStatus(pLVKey, pJob.getJobId(), pJob.getJobStepId(), pHandle, l_Size);

        }
        else
        {
            LOG(bb,error) << "BBTransferDef::prepareForRestart(): Attempting to reset cross bbServer metadata for jobid " << job.getJobId() \
                          <<   ", jobstepid " << job.getJobStepId() << ", handle " << pHandle << ", contribid " << pContribId << " failed";
        }
    }

    return rc;
}

void BBTransferDef::removeFile(const char* pFileName)
{
    int rc = 0;

    try
    {
        LOG(bb,debug) << "removeFile(): Attempting to remove file " << pFileName << " as part of the cancel operation issued for handle " \
                      << transferHandle << ", contribid " << contribid;
        rc = remove(pFileName);
    }
    catch(ExceptionBailout& e) { }
    catch(exception& e)
    {
        rc = -1;
        LOG(bb,error) << "Exception thrown in " << __func__ << " was " << e.what() << " when attempting to remove target file " << pFileName;
    }

    if (!rc)
    {
        LOG(bb,info) << "Target file " << pFileName << " successfully removed as part of the cancel operation issued for handle " \
                     << transferHandle << ", contribid " << contribid;
    }
    else
    {
        LOG(bb,warning) << "Target file " << pFileName << " could not be removed as part of the cancel operation issued for handle " \
                        << transferHandle << ", contribid " << contribid \
                        << ", errno " << errno << " (" << strerror(errno) << ")";
    }

    return;
}

void BBTransferDef::removeTargetFiles(const LVKey* pLVKey)
{
    Extent* l_ExtentPtr = 0;
    for(size_t i=1; i<files.size(); i=i+2)
    {
        l_ExtentPtr = getAnyTargetExtent((uint32_t)i);
        if (l_ExtentPtr)
        {
            uint64_t l_Flags = l_ExtentPtr->getFlags();
            if (l_Flags & BBI_TargetPFS || l_Flags & BBI_TargetPFSPFS)
            {
                removeFile((char*)files[i].c_str());
            }
        }
    }

    return;
}
#endif

int BBTransferDef::replaceExtentVector(vector<Extent>* pNewList)
{
    int l_RC = 0;

    extents.clear();
    for(auto& e : *pNewList)
    {
        extents.push_back(e);
    }

    return l_RC;
}

int BBTransferDef::replaceExtentVector(BBTransferDef* pTransferDef)
{
    int l_RC = 0;

    extents.clear();
    for(auto& e : pTransferDef->extents)
    {
        extents.push_back(e);
    }

    return l_RC;
}

uint64_t BBTransferDef::retrieveJobId() {
    return job.getJobId();
}

uint64_t BBTransferDef::retrieveJobStepId() {
    return job.getJobStepId();
}

#ifdef BBSERVER
int BBTransferDef::retrieveTransfers(BBTransferDefs& pTransferDefs, BBLVKey_ExtentInfo* pExtentInfo)
{
    int rc = 0;
    stringstream errorText;

    if (pTransferDefs.getJobStepId() == UNDEFINED_JOBSTEPID || pTransferDefs.getJobStepId() == job.getJobStepId())
    {
        // Copy the transfer definition...
        rc = copyForRetrieveTransferDefinitions(pTransferDefs, pExtentInfo);
    }

    return rc;
}

void BBTransferDef::setAllExtentsTransferred(const LVKey* pLVKey, const uint64_t pHandle, const uint32_t pContribId, const int pValue)
{
    if (pValue && (allExtentsTransferred())) {
        LOG(bb,debug) << "For " << *pLVKey << ", processing complete for the transfer definition associated with contribid " << pContribId;
    }

    if ((((flags & BBTD_All_Extents_Transferred) == 0) && pValue) || ((flags & BBTD_All_Extents_Transferred) && (!pValue)))
    {
        LOG(bb,debug) << "BBTransferDef::setAllExtentsTransferred(): Jobid " << job.getJobId() << ", jobstepid " << job.getJobStepId() << ", handle " << pHandle << ", contribid " << pContribId \
                      << " -> Changing from: " << ((flags & BBTD_All_Extents_Transferred) ? "true" : "false") << " to " << (pValue ? "true" : "false");
    }
    SET_FLAG(BBTD_All_Extents_Transferred, pValue);

    // Now update the status for the ContribId and Handle files in the xbbServer data...
    // \todo - We do not handle the return code... @DLH
    ContribIdFile::update_xbbServerContribIdFile(pLVKey, getJobId(), getJobStepId(), pHandle, pContribId, BBTD_All_Extents_Transferred, pValue);

    return;
}

void BBTransferDef::setAllFilesClosed(const LVKey* pLVKey, const uint64_t pHandle, const uint32_t pContribId, const int pValue)
{
    // NOTE: This attribute is NOT maintained in the transfer definition and only in the contribid file

    // Update the status for the ContribId and Handle files in the xbbServer data...
    // \todo - We do not handle the return code... @DLH
    ContribIdFile::update_xbbServerContribIdFile(pLVKey, getJobId(), getJobStepId(), pHandle, pContribId, BBTD_All_Files_Closed, pValue);

    return;
}

void BBTransferDef::setCanceled(const LVKey* pLVKey, const uint64_t pHandle, const uint32_t pContribId, const int pValue)
{
    if (pValue && (!canceled()))
    {
        LOG(bb,info) << "For " << *pLVKey << ", the transfer of all remaining extents canceled for the transfer definition associated with jobid " \
                     << getJobId() << ", jobstepid " << getJobStepId() << ", handle " << pHandle << ", contribid " << pContribId;
    }

    if ((((flags & BBTD_Canceled) == 0) && pValue) || ((flags & BBTD_Canceled) && (!pValue)))
    {
        LOG(bb,debug) << "BBTransferDef::setCanceled(): Jobid " << job.getJobId() << ", jobstepid " << job.getJobStepId() << ", handle " << pHandle << ", contribid " << pContribId \
                      << " -> Changing from: " << ((flags & BBTD_Canceled) ? "true" : "false") << " to " << (pValue ? "true" : "false");
    }

    if (pValue && stopped() && canceled())
    {
        SET_FLAG(BBTD_Stopped, 0);
        LOG(bb,info) << "BBTransferDef::setCanceled(): Jobid " << job.getJobId() << ", jobstepid " << job.getJobStepId() << ", handle " << pHandle << ", contribid " << pContribId \
                     << " was previously stopped. It will now be canceled and no longer be restartable.";
    }
    SET_FLAG(BBTD_Canceled, pValue);

    // Now update the status for the ContribId and Handle files in the xbbServer data...
    // \todo - We do not handle the return code... @DLH
    ContribIdFile::update_xbbServerContribIdFile(pLVKey, getJobId(), getJobStepId(), pHandle, pContribId, BBTD_Canceled, pValue);

    return;
}

void BBTransferDef::setExtentsEnqueued(const LVKey* pLVKey, const uint64_t pHandle, const uint32_t pContribId, const int pValue)
{
    if ((((flags & BBTD_Extents_Enqueued) == 0) && pValue) || ((flags & BBTD_Extents_Enqueued) && (!pValue)))
    {
        LOG(bb,debug) << "BBTransferDef::setExtentsEnqueued(): Jobid " << job.getJobId() << ", jobstepid " << job.getJobStepId() << ", handle " << pHandle << ", contribid " << pContribId \
                      << " -> Changing from: " << ((flags & BBTD_Extents_Enqueued) ? "true" : "false") << " to " << (pValue ? "true" : "false");
    }
    SET_FLAG(BBTD_Extents_Enqueued, pValue);

    // Now update the status for the ContribIdfiles in the xbbServer data...
    // \todo - We do not handle the return code... @DLH
    ContribIdFile::update_xbbServerContribIdFile(pLVKey, getJobId(), getJobStepId(), pHandle, pContribId, BBTD_Extents_Enqueued, pValue);

    return;
}

void BBTransferDef::setFailed(const LVKey* pLVKey, const uint64_t pHandle, const uint32_t pContribId, const int pValue)
{
    // NOTE: Doesn't matter if all extents have been processed.  We can fail a transfer definition after processing
    //       all of the extents.  (e.g., close for a file)
    if (pValue)
    {
        LOG(bb,info) << "For " << *pLVKey << ", I/O for one or more extents failed for one or more file(s), a final close failed for one or more file(s)," \
                        " a cancel operation occurred for the contributor, or some other failure occurred for the transfer definition associated with contribid " << pContribId;
    }

    if ((((flags & BBTD_Failed) == 0) && pValue) || ((flags & BBTD_Failed) && (!pValue)))
    {
        LOG(bb,debug) << "BBTransferDef::setFailed(): Job " << job.getJobId() << ", jobstep " << job.getJobStepId() << ", handle " << pHandle << ", contribid " << pContribId \
                      << " -> Changing from: " << ((flags & BBTD_Failed) ? "true" : "false") << " to " << (pValue ? "true" : "false");
    }
    SET_FLAG(BBTD_Failed, pValue);

    // Now update the status for the ContribId and Handle files in the xbbServer data...
    // \todo - We do not handle the return code... @DLH
    ContribIdFile::update_xbbServerContribIdFile(pLVKey, getJobId(), getJobStepId(), pHandle, pContribId, BBTD_Failed, pValue);

    return;
}
#endif

void BBTransferDef::setJob()
{
    if (!getJobId())
    {
        setJob(retrieveJobId(), retrieveJobStepId());
    }
    else
    {
        // NOTE:  If jobid is already set, then bbcmd must have
        //        invoked bbapi with an already filled in value for
        //        the jobid and jobstepid...
    }

    return;
}

#ifdef BBSERVER
void BBTransferDef::setStopped(const LVKey* pLVKey, const uint64_t pHandle, const uint32_t pContribId, const int pValue)
{
    if (pValue)
    {
        LOG(bb,info) << "For " << *pLVKey << ", the transfer of all remaining extents stopped for the transfer definition associated with contribid " << pContribId;
    }

    if ((((flags & BBTD_Stopped) == 0) && pValue) || ((flags & BBTD_Stopped) && (!pValue)))
    {
        LOG(bb,debug) << "BBTransferDef::setStopped(): Jobid " << job.getJobId() << ", jobstepid " << job.getJobStepId() << ", handle " << pHandle << ", contribid " << pContribId \
                      << " -> Changing from: " << ((flags & BBTD_Stopped) ? "true" : "false") << " to " << (pValue ? "true" : "false");
    }
    SET_FLAG(BBTD_Stopped, pValue);

    // Now update the status for the ContribId and Handle files in the xbbServer data...
    // \todo - We do not handle the return code... @DLH
    ContribIdFile::update_xbbServerContribIdFile(pLVKey, getJobId(), getJobStepId(), pHandle, pContribId, BBTD_Stopped, pValue);

    return;
}

int BBTransferDef::stopTransfer(const LVKey* pLVKey, const string& pHostName, const uint64_t pJobId, const uint64_t pJobStepId, const uint64_t pHandle, const uint32_t pContribId)
{
    int rc = 0;
    stringstream errorText;

    HandleFile* l_HandleFile = 0;
    char* l_HandleFileName = 0;
    bool l_HandleFileLocked = false;

    bool l_StopDefinition = false;

    if (job.getJobStepId() == pJobStepId)
    {
        // NOTE: We cannot check the local metadata for stopped() nor canceled().
        //       In the case where a transfer definition has already failed over
        //       once, the local metadata will still show that the definition is
        //       already stopped when attempting to fail back.
        //       Must check the cross bbServer metadata, which is current.
        // NOTE: We may have to spin for a while waiting for the extents to be
        //       enqueued on the work queue.  This is the case where we are
        //       performing a start transfer for this definition but it hasn't
        //       completed the processing performed by the second volley from
        //       bbProxy.  We have to make sure that the extents are first enqueued
        //       so that the stop processing is properly performed.
        //
        //       We spin for up to 2 minutes...
        string l_ConnectionName = string();
        int l_Attempts = 120;
        while (!rc && l_Attempts--)
        {
            // NOTE: The Handlefile is locked exclusive here to serialize between this bbServer checking for
            //       the extents to be enqueued and another thread/bbServer enqueuing those extents.
            rc = HandleFile::loadHandleFile(l_HandleFile, l_HandleFileName, pJobId, pJobStepId, pHandle, LOCK_HANDLEFILE);
            if (!rc)
            {
                l_HandleFileLocked = true;
                rc = extentsAreEnqueued();
                if (!rc)
                {
                    // Release the handle file
                    l_HandleFileLocked = false;
                    l_HandleFile->close();

                    unlockTransferQueue(pLVKey, "stopTransfer - Waiting for transfer definition's extents to be enqueued");
                    {
                        usleep((useconds_t)1000000);    // Delay 1 second
                    }
                    lockTransferQueue(pLVKey, "stopTransfer - Waiting for transfer definition's extents to be enqueued");

                    // Check to make sure the job still exists after releasing/re-acquiring the lock
                    // NOTE: The connection name is optional, and is potentially different for every
                    //       transfer definition that is being stopped from the retrieve transfer archive.
                    if (!jobStillExists(l_ConnectionName, pLVKey, (BBTagInfo2*)0, (BBTagInfo*)0, pJobId, pContribId))
                    {
                        rc = -1;
                        l_Attempts = 0;
                    }
                }
            }
            else
            {
                // Handle file could not be locked
                rc = -1;
                l_Attempts = 0;
                LOG(bb,error) << "Could not lock the handle file for " << *pLVKey << ", jobid " << pJobId << ", jobstepid " << pJobStepId << ", handle " << pHandle \
                              << " when attempting to determine if all extents had been enqueued during stop transfer processing";
            }

            if (l_HandleFileLocked)
            {
                // Release the handle file
                l_HandleFileLocked = false;
                l_HandleFile->close();
            }
            if (l_HandleFileName)
            {
                delete[] l_HandleFileName;
                l_HandleFileName = 0;
            }

            if (l_HandleFile)
            {
                delete l_HandleFile;
                l_HandleFile = 0;
            }
        }

        bool l_UnconditionalRestart = false;
        switch (rc)
        {
            case 0:
            {
                l_UnconditionalRestart = true;
                LOG(bb,info) << "Transfer definition associated with CN host " << pHostName << ", jobid " << pJobId << ", jobstepid " << pJobStepId \
                             << ", handle " << pHandle << ", contribId " << pContribId << " was interrupted during the processing of the original start transfer request."\
                             << " The transfer definition does not currently have any enqueued extents to transfer for any file, but the original start transfer request is not reponding." \
                             << " The transfer definition will be stopped and then restarted.";
            }
            // Fall through is intended...

            case 1:
            {
                rc = 0;
                ContribIdFile* l_ContribIdFile = 0;
                if (!l_UnconditionalRestart)
                {
                    bfs::path l_HandleFilePath(config.get("bb.bbserverMetadataPath", DEFAULT_BBSERVER_METADATAPATH));
                    l_HandleFilePath /= bfs::path(to_string(pJobId));
                    l_HandleFilePath /= bfs::path(to_string(pJobStepId));
                    l_HandleFilePath /= bfs::path(to_string(pHandle));
                    int l_RC = ContribIdFile::loadContribIdFile(l_ContribIdFile, pLVKey, l_HandleFilePath, pContribId);
                    switch (l_RC)
                    {
                        case 1:
                        {
                            // We stop any transfer definition that does not have all of its files transferred/closed -or-
                            // has a failed transfer
                            if (!l_ContribIdFile->notRestartable())
                            {
                                l_StopDefinition = true;
                            }

                            break;
                        }
                        case 0:
                        {
                            LOG(bb,error) << "ContribId " << pContribId << "could not be found in the contribid file for jobid " << pJobId << ", jobstepid " << pJobStepId << ", handle " << pHandle << ", " << *pLVKey << ", using handle path " << l_HandleFilePath;

                            break;
                        }
                        default:
                        {
                            LOG(bb,error) << "Could not load the contribid file for jobid " << pJobId << ", jobstepid " << pJobStepId << ", handle " << pHandle << ", contribid " << pContribId << ", " << *pLVKey << ", using handle path " << l_HandleFilePath;
                        }
                    }
                }
                else
                {
                    l_StopDefinition = true;
                }

                if (l_StopDefinition)
                {
                    rc = becomeUser(getUserId(), getGroupId());
                    if (!rc)
                    {
                        // Mark the transfer definition as stopped
                        markAsStopped(pLVKey, pHandle, pContribId);

                        // If an unconditional restart, in addition to marking the transfer definition
                        // as stopped, indicate that all extents have been enqueued/processed and all files closed.
                        // NOTE:  No extents were enqueued/processed for an unconditional restart so these bits
                        //        need to be set on here...
                        if (l_UnconditionalRestart)
                        {
                            setExtentsEnqueued(pLVKey, pHandle, pContribId);
                            setAllExtentsTransferred(pLVKey, pHandle, pContribId);
                            setAllFilesClosed(pLVKey, pHandle, pContribId);
                        }
                        rc = 1;

                        becomeUser(0,0);
                    }
                    else
                    {
                        rc = -2;
                        errorText << "becomeUser failed when attempting to stop the transfer definition associated with host " << pHostName \
                                  << ", jobid " << pJobId << ", jobstepid " << pJobStepId << ", handle " << pHandle << ", contribId " << pContribId \
                                  << " when attempting to become uid=" << getUserId() << ", gid=" << getGroupId();
                        bberror << err("error.uid", getUserId()) << err("error.gid", getGroupId());
                        LOG_ERROR_TEXT_RC(errorText, rc);
                    }
                }
                else
                {
                    LOG(bb,info) << "A stop transfer request was made for the transfer definition associated with " << *pLVKey << ", jobid " << pJobId << ", jobstepid " << pJobStepId \
                                 << ", handle " << pHandle << ", contribid " << pContribId << ", however no extents are left to be transferred (via BBTransferDef), rc = " << rc \
                                 << ". Stop transfer request ignored.";
                    rc = 2;
                }

                if (l_ContribIdFile)
                {
                    delete l_ContribIdFile;
                    l_ContribIdFile = NULL;
                }
            }
            break;

            default:
            {
                // rc already -1
                errorText << "Job no longer exists or the handle file could not be locked for the transfer definition associated with host " << pHostName \
                          << ", jobid " << pJobId << ", jobstepid " << pJobStepId << ", handle " << pHandle << ", contribId " << pContribId \
                          << ".  Stop transfer request ignored.";
                LOG_ERROR_TEXT_RC(errorText, rc);
            }
        }
    }

    return rc;
}
#endif

void BBTransferDef::unlock()
{
    pthread_mutex_unlock(&update);

    return;
}
