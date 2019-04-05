/*******************************************************************************
 |    ContribIdFile.cc
 |
 |   Copyright IBM Corporation 2015,2016. All Rights Reserved
 |
 |    This program is licensed under the terms of the Eclipse Public License
 |    v1.0 as published by the Eclipse Foundation and available at
 |    http://www.eclipse.org/legal/epl-v10.html
 |
 |    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
 |    restricted by GSA ADP Schedule Contract with IBM Corp.
 *******************************************************************************/

#include "bbfileflags.h"
#include "bbserver_flightlog.h"
#include "BBTagID.h"
#include "ContribFile.h"
#include "ContribIdFile.h"
#include "HandleFile.h"
#include "LVUuidFile.h"


/*
 * Static methods
 */

int ContribIdFile::allExtentsTransferredButThisContribId(const uint64_t pHandle, const BBTagID& pTagId, const uint32_t pContribId)
{
    int rc = 1;   // Success, all extents transferred...

    bfs::path handle(config.get("bb.bbserverMetadataPath", DEFAULT_BBSERVER_METADATAPATH));
    handle /= bfs::path(to_string(pTagId.getJobId()));
    handle /= bfs::path(to_string(pTagId.getJobStepId()));
    handle /= bfs::path(to_string(pHandle));

    uint64_t l_FL_Counter = metadataCounter.getNext();
    FL_Write(FLMetaData, CIF_AllExtentsXfered, "ContribIdFile all extents transferred but this contribid, counter=%ld, jobid=%ld, handle=%ld, contribid=%ld", l_FL_Counter, pTagId.getJobId(), pHandle, pContribId);

    if (bfs::exists(handle))
    {
        for (auto& lvuuid: boost::make_iterator_range(bfs::directory_iterator(handle), {}))
        {
            if ((rc != 1) || (!bfs::is_directory(lvuuid))) continue;
            bfs::path contribs_file = lvuuid.path() / "contribs";
            ContribFile* l_ContribFile = 0;
            int rc2 = ContribFile::loadContribFile(l_ContribFile, contribs_file.c_str());
            if (!rc2)
            {
                for (map<uint32_t,ContribIdFile>::iterator ce = l_ContribFile->contribs.begin(); ce != l_ContribFile->contribs.end(); ce++)
                {
                    if (ce->first != pContribId)
                    {
                        if (!(ce->second).allExtentsTransferred())
                        {
                            rc = 0;   // Not all extents transferred...
                            break;
                        }
                    }
                }
            }
            else
            {
                rc = -1;
                LOG(bb,error) << "Could not load the contrib file for jobid " << pTagId.getJobId() << ", jobstepid " << pTagId.getJobStepId() << ", handle " << pHandle << ", from file " << contribs_file.string();
            }

            if (l_ContribFile)
            {
                delete l_ContribFile;
                l_ContribFile=NULL;
            }
        }
    }
    else
    {
        rc = -1;  //  Error case...
        LOG(bb,error) << "Handle file for jobid " << pTagId.getJobId() << ", jobstepid " << pTagId.getJobStepId() << ", handle " << pHandle << ", from file " << handle.string() << " does not exist.";
    }

    FL_Write6(FLMetaData, CIF_AllExtentsXfered_End, "ContribIdFile all extents transferred but this contribid, counter=%ld, jobid=%ld, handle=%ld, contribid=%ld, rc=%ld",
              l_FL_Counter, pTagId.getJobId(), pHandle, pContribId, rc, 0);

    return rc;
}

int ContribIdFile::isStopped(const BBJob pJob, const uint64_t pHandle, const uint32_t pContribId)
{
    int rc = 0;

    ContribIdFile* l_ContribIdFile = 0;

    bfs::path l_HandleFilePath(config.get("bb.bbserverMetadataPath", DEFAULT_BBSERVER_METADATAPATH));
    l_HandleFilePath /= bfs::path(to_string(pJob.getJobId()));
    l_HandleFilePath /= bfs::path(to_string(pJob.getJobStepId()));
    l_HandleFilePath /= bfs::path(to_string(pHandle));

    // NOTE: The handle file does not have to be locked exclusive here because the stop transfer processsing 'waits'
    //       for the extents to be enqueued.  The processing of competing start/restart transfer definition processing is
    //       serialized via the transfer queue that should already be held when this method is invoked.
    int rc2 = ContribIdFile::loadContribIdFile(l_ContribIdFile, l_HandleFilePath, pContribId);
    if (rc2 >= 0)
    {
        // Process the contribid file
        if (rc2 == 1 && l_ContribIdFile)
        {
            if (l_ContribIdFile->stopped())
            {
                // Set rc to 1...
                rc = 1;
            }
        }
        else
        {
            // Could be normal...
        }
    }
    else
    {
        // Could be normal...
    }

    if (l_ContribIdFile)
    {
        delete l_ContribIdFile;
        l_ContribIdFile = 0;
    }

    return rc;
}

int ContribIdFile::loadContribIdFile(ContribIdFile* &pContribIdFile, const bfs::path& pHandleFilePath, const uint32_t pContribId, Uuid* pUuid)
{
    int rc = 0;
    ContribFile* l_ContribFile = 0;
    bool l_ContribIdFound = false;

    uint64_t l_FL_Counter = metadataCounter.getNext();
    FL_Write(FLMetaData, CIF_Load1, "loadContribIdFile, counter=%ld, contribid=%ld", l_FL_Counter, pContribId, 0, 0);

    pContribIdFile = 0;
    if(bfs::exists(pHandleFilePath))
    {
        for (auto& lvuuid : boost::make_iterator_range(bfs::directory_iterator(pHandleFilePath), {}))
        {
            if (!bfs::is_directory(lvuuid)) continue;
            bfs::path contribs_file = lvuuid.path() / "contribs";
            rc = ContribFile::loadContribFile(l_ContribFile, contribs_file.string().c_str());
            if (!rc)
            {
                for (map<uint32_t,ContribIdFile>::iterator ce = l_ContribFile->contribs.begin(); (!l_ContribIdFound) && ce != l_ContribFile->contribs.end(); ce++)
                {
                    if (ce->first == pContribId)
                    {
                        pContribIdFile = new ContribIdFile(ce->second);
                        if (pUuid)
                        {
                            (*pUuid).copyFrom(lvuuid.path().filename().string().c_str());
                        }
                        l_ContribIdFound = true;
                        rc = 1;
                        break;
                    }
                }
            }
            else
            {
                rc = -1;
                LOG(bb,error) << "Could not load the contrib file from file " << lvuuid.path().string();
                break;
            }

            delete l_ContribFile;
            l_ContribFile=NULL;

            if (l_ContribIdFound)
            {
                break;
            }
        }

        if (l_ContribFile)
        {
            delete l_ContribFile;
            l_ContribFile=NULL;
        }

        if (rc != 1 && pContribIdFile)
        {
            delete pContribIdFile;
            pContribIdFile = 0;
        }
    }
    else
    {
        // rc is already 0, which indicates that the contribid file was not loaded.
        // NOTE:  Since this is most likely because the job/LVUuid has been deleted,
        //        we don't differentiate this case from any other 'normal' case where
        //        the contribid file was not loaded.
        LOG(bb,warning) << "Path to the handle file does not exist " << pHandleFilePath.string();
    }

    FL_Write(FLMetaData, CIF_Load1_End, "loadContribIdFile1, counter=%ld, contribid=%ld, rc=%ld", l_FL_Counter, pContribId, rc, 0);

    return rc;
}

int ContribIdFile::loadContribIdFile(ContribIdFile* &pContribIdFile, const LVKey* pLVKey, const bfs::path& pHandleFilePath, const uint32_t pContribId)
{
    int rc = 0;
    bool l_ContribIdFound = false;

    pContribIdFile = 0;

    ContribFile* l_ContribFile;
    Uuid lv_uuid = pLVKey->second;
    char lv_uuid_str[LENGTH_UUID_STR] = {'\0'};
    lv_uuid.copyTo(lv_uuid_str);
    bfs::path l_ContribFilePath = pHandleFilePath / bfs::path(lv_uuid_str) / bfs::path("contribs");

    uint64_t l_FL_Counter = metadataCounter.getNext();
    FL_Write(FLMetaData, CIF_Load2, "loadContribIdFile, counter=%ld, contribid=%ld", l_FL_Counter, pContribId, 0, 0);

    if(bfs::exists(l_ContribFilePath))
    {
        rc = ContribFile::loadContribFile(l_ContribFile, l_ContribFilePath);
        if (!rc)
        {
            for (map<uint32_t,ContribIdFile>::iterator ce = l_ContribFile->contribs.begin(); (!l_ContribIdFound) && ce != l_ContribFile->contribs.end(); ce++)
            {
                if (!l_ContribIdFound)
                {
                    if (ce->first == pContribId)
                    {
                        pContribIdFile = new ContribIdFile(ce->second);
                        l_ContribIdFound = true;
                        rc = 1;
                    }
                }
            }
        }
        else
        {
            rc = -1;
            LOG(bb,error) << "Could not load the contrib file from file " << l_ContribFilePath.string();
        }

        if (l_ContribFile)
        {
            delete l_ContribFile;
            l_ContribFile=NULL;
        }

        if (rc != 1 && pContribIdFile)
        {
            delete pContribIdFile;
            pContribIdFile = 0;
        }
    }
    else
    {
        rc = -1;
        LOG(bb,error) << "Could not load the contrib file from file " << l_ContribFilePath.string() << " because it does not exist";
    }

    FL_Write(FLMetaData, CIF_Load2_End, "loadContribIdFile, counter=%ld, contribid=%ld, rc=%ld", l_FL_Counter, pContribId, rc, 0);

    return rc;
}

int ContribIdFile::loadContribIdFile(ContribIdFile* &pContribIdFile, uint64_t& pNumHandleContribs, uint64_t& pNumLVUuidContribs, const bfs::path& pHandleFilePath, const uint32_t pContribId)
{
    int rc = 0;

    pContribIdFile = 0;
    pNumHandleContribs = 0;
    pNumLVUuidContribs = 0;

    uint64_t l_FL_Counter = metadataCounter.getNext();
    FL_Write(FLMetaData, CIF_Load3, "loadContribIdFile, counter=%ld, contribid=%ld", l_FL_Counter, pContribId, 0, 0);

    ContribFile* l_ContribFile = 0;
    for (auto& lvuuid : boost::make_iterator_range(bfs::directory_iterator(pHandleFilePath), {}))
    {
        if(!bfs::is_directory(lvuuid)) continue;
        bfs::path contribs_file = lvuuid.path() / bfs::path("contribs");
        int rc2 = ContribFile::loadContribFile(l_ContribFile, contribs_file.c_str());
        if (!rc2)
        {
            pNumHandleContribs += (uint64_t)(l_ContribFile->numberOfContribs());
            for (map<uint32_t,ContribIdFile>::iterator ce = l_ContribFile->contribs.begin(); ce != l_ContribFile->contribs.end(); ce++)
            {
                if (ce->first == pContribId)
                {
                    pNumLVUuidContribs = (uint64_t)(l_ContribFile->numberOfContribs());
                    pContribIdFile = new ContribIdFile(ce->second);
                    rc = 1;
                }
            }
        }
        else
        {
            rc = -1;
            LOG(bb,error) << "Could not load the contrib file from file " << lvuuid.path().string();
            break;
        }

        if (l_ContribFile)
        {
            delete l_ContribFile;
            l_ContribFile=NULL;
        }
    }

    if (rc != 1 && pContribIdFile)
    {
        delete pContribIdFile;
        pContribIdFile = 0;
    }

    if (l_ContribFile)
    {
        delete l_ContribFile;
        l_ContribFile=NULL;
    }

    pNumHandleContribs = ((rc == -1) ? 0 : pNumHandleContribs);
    pNumLVUuidContribs = ((rc != 1) ? 0 : pNumLVUuidContribs);

    FL_Write6(FLMetaData, CIF_Load3_End, "loadContribIdFile, counter=%ld, contribid=%ld, number of handle contribs=%ld, number of LVUuid contribs=%ld, rc=%ld",
              l_FL_Counter, pContribId, pNumHandleContribs, pNumLVUuidContribs, rc, 0);

    return rc;
}

int ContribIdFile::saveContribIdFile(ContribIdFile* &pContribIdFile, const LVKey* pLVKey, const bfs::path& pHandleFilePath, const uint32_t pContribId)
{
    int rc = 0;

    Uuid lv_uuid = pLVKey->second;
    char lv_uuid_str[LENGTH_UUID_STR] = {'\0'};
    lv_uuid.copyTo(lv_uuid_str);
    bfs::path l_ContribFilePath = pHandleFilePath / bfs::path(lv_uuid_str) / bfs::path("contribs");

    uint64_t l_FL_Counter = metadataCounter.getNext();
    FL_Write(FLMetaData, CIF_Save, "saveContribIdFile, counter=%ld, contribid=%ld", l_FL_Counter, pContribId, 0, 0);

    if(bfs::exists(l_ContribFilePath))
    {
        ContribFile* l_ContribFile = 0;
        rc = ContribFile::loadContribFile(l_ContribFile, l_ContribFilePath);
        if (!rc)
        {
//            char l_Prefix[64] = "\0";
//            snprintf(l_Prefix, sizeof(l_Prefix), "Save of contribid %u", pContribId);
//            pContribIdFile->dump("l_Prefix");
            l_ContribFile->contribs[pContribId] = *pContribIdFile;
            rc = l_ContribFile->save(l_ContribFilePath.string());
            if (rc)
            {
                LOG(bb,error) << "Could not save the contrib file to file " << l_ContribFilePath.string();

            }
        }
        else
        {
            LOG(bb,error) << "Could not load the contrib file from file " << l_ContribFilePath.string();
        }

        if (l_ContribFile)
        {
            delete l_ContribFile;
            l_ContribFile=NULL;
        }
    }
    else
    {
        LOG(bb,error) << "Could not load the contrib file from file " << l_ContribFilePath.string() << " because it does not exist";
    }

    if (pContribIdFile)
    {
        FL_Write6(FLMetaData, CIF_Save_End, "saveContribIdFile, counter=%ld, contribid=%ld, flags=0x%lx, transfer size=%ld, rc=%ld",
                  l_FL_Counter, pContribId, pContribIdFile->flags, pContribIdFile->totalTransferSize, rc, 0);
    }
    else
    {
        FL_Write(FLMetaData, CIF_Save_ErrEnd, "saveContribIdFile, counter=%ld, contribid=%ld, rc=%ld",
                  l_FL_Counter, pContribId, rc, 0);
    }

    return rc;
}

int ContribIdFile::update_xbbServerContribIdFile(const LVKey* pLVKey, const uint64_t pJobId, const uint64_t pJobStepId, const uint64_t pHandle, const uint32_t pContribId, const ALLOW_BUMP_FOR_REPORTING_CONTRIBS_OPTION pAllowBumpOfReportingContribs, const uint64_t pFlags, const int pValue)
{
    int rc = 0;
    int32_t l_NumOfContribsBump = 0;

    uint64_t l_FL_Counter = metadataCounter.getNext();
    FL_Write(FLMetaData, CIF_UpdateFile, "update contribid file, counter=%ld, jobid=%ld, handle=%ld, contribid=%ld", l_FL_Counter, pJobId, pHandle, pContribId);

    ContribIdFile* l_ContribIdFile = 0;
    bfs::path l_HandleFilePath(config.get("bb.bbserverMetadataPath", DEFAULT_BBSERVER_METADATAPATH));
    l_HandleFilePath /= bfs::path(to_string(pJobId));
    l_HandleFilePath /= bfs::path(to_string(pJobStepId));
    l_HandleFilePath /= bfs::path(to_string(pHandle));
    rc = loadContribIdFile(l_ContribIdFile, pLVKey, l_HandleFilePath, pContribId);
    switch (rc)
    {
        case 1:
        {
#ifndef __clang_analyzer__  // zeroing rc is not necessary, but safer to have this here
            rc = 0;
#endif
            uint64_t l_Flags = l_ContribIdFile->flags;

            uint64_t l_NewFlags = 0;
            SET_FLAG_VAR(l_NewFlags, l_Flags, pFlags, (uint32_t)pValue);

            if (!pValue)
            {
                if (pFlags & BBTD_All_Extents_Transferred)
                {
                    // If we are turning off BBTD_All_Extents_Transferred, also turn off BBTD_All_Files_Closed.
                    // This occurs during restart for a transfer definition.
                    SET_FLAG_VAR(l_NewFlags, l_NewFlags, BBTD_All_Files_Closed, (uint32_t)pValue);
                }
            }
            else
            {
                if (pFlags & BBTD_All_Files_Closed)
                {
                    // If we are turning on BBTD_All_Files_Closed, make sure that BBTD_All_Extents_Transferred
                    // is also turned on.  We only update the handle file status when BBTD_All_Files_Closed is
                    // turned on, so if BBTD_All_Extents_Transferred is not on, the handle status may never be
                    // properly updated to it's final status.  We don't update the handle file status when
                    // BBTD_All_Extents_Transferred is turned on to reduce contention on the handle file/handle file lock.
                    SET_FLAG_VAR(l_NewFlags, l_NewFlags, BBTD_All_Extents_Transferred, (uint32_t)pValue);
                }
            }

            if (l_Flags != l_NewFlags)
            {
                LOG(bb,info) << "xbbServer: For " << *pLVKey << ", handle " << pHandle << ", contribid " << pContribId << ":";
                LOG(bb,info) << "           ContribId flags changing from 0x" << hex << uppercase << l_Flags << " to 0x" << l_NewFlags << nouppercase << dec << ".";
            }

            l_ContribIdFile->flags = l_NewFlags;

            // If the bump of reporting contribs is allowed, *and* 'extents enqueued' is not currently on in the current ContribId flags, *and*
            // we are now turning on 'extents enqueued', bump the number of reporting contributors
            if ((pAllowBumpOfReportingContribs) && (!(l_Flags & BBTD_Extents_Enqueued)) && (pValue && (pFlags & BBTD_Extents_Enqueued)))
            {
                l_NumOfContribsBump = 1;
            }

            rc = saveContribIdFile(l_ContribIdFile, pLVKey, l_HandleFilePath, pContribId);
            if (!rc)
            {
                // Only update the handle file status if the status could change
                if ((!pValue) || pFlags & BB_UpdateHandleStatusMask1)
                {
                    // NOTE:  Pass a zero for the size.  Size is only updated when the last extent for a file is transferred.
                    // NOTE:  We invoke update_xbbServerHandleStatus() here instead of update_xbbServerHandleFile().  This is because
                    //        none of the attributes that we turn on for the contribid file are absolute to the handle file.  When turning
                    //        on an attribute, it has to be calculated along with information from the other contributors.  Update status
                    //        performs this logic.  When turning off an attribute (restart logic), some of these are absolute to the
                    //        handle file but the same calculation logic in update status will turn off those attributes in the handle file.
                    //        This calculation of an attribute is only performed by update status when a FULL_SCAN is performed.
                    // NOTE:  If the stopped, failed, or canceled flag is set, specify that a FULL_SCAN be performed by
                    //        update_xbbServerHandleStatus() so that the handle status is set correctly.
                    // NOTE:  If we are turning a flag off, specify that a FULL_SCAN be performed by update_xbbServerHandleStatus()
                    //        so that the corresponding attribute in the handle file is set correctly.
                    if (HandleFile::update_xbbServerHandleStatus(pLVKey, pJobId, pJobStepId, pHandle, pContribId, l_NumOfContribsBump, 0,
                         (((!pValue) || l_ContribIdFile->flags & BBTD_Stopped || l_ContribIdFile->flags & BBTD_Failed || l_ContribIdFile->flags & BBTD_Canceled) ? FULL_SCAN : NORMAL_SCAN)))
                    {
                        LOG(bb,error) << "ContribIdFile::update_xbbServerContribIdFile():  Failure when attempting to update the cross bbServer handle status for jobid " << pJobId \
                                      << ", jobstepid " << pJobStepId << ", handle " << pHandle << ", contribid " << pContribId;
                    }
                }
            }
            else
            {
                LOG(bb,error) << "ContribIdFile::update_xbbServerContribIdFile():  Failure when attempting to save the cross bbServer contribs file for jobid " << pJobId \
                              << ", jobstepid " << pJobStepId << ", handle " << pHandle << ", contribid " << pContribId;
            }

            break;
        }
        case 0:
        {
            rc = -1;
            LOG(bb,error) << "ContribId " << pContribId << " could not be found in the contrib file for jobid " << pJobId << ", jobstepid " << pJobStepId << ", handle " << pHandle << ", " << *pLVKey << ", using handle path " << l_HandleFilePath.string();

            break;
        }
        default:
        {
            LOG(bb,error) << "Could not load the contrib file for jobid " << pJobId << ", jobstepid " << pJobStepId << ", handle " << pHandle << ", " << *pLVKey << ", using handle path " << l_HandleFilePath.string();
        }
    }


    if (l_ContribIdFile)
    {
        FL_Write6(FLMetaData, CIF_UpdateFile_End, "update contribid file, counter=%ld, jobid=%ld, handle=%ld, contribid=%ld, flags=0x%lx, rc=%ld",
                  l_FL_Counter, pJobId, pHandle, pContribId, l_ContribIdFile->flags, rc);
        delete l_ContribIdFile;
        l_ContribIdFile = 0;
    }
    else
    {
        FL_Write6(FLMetaData, CIF_UpdateFile_ErrEnd, "update contribid file, counter=%ld, jobid=%ld, handle=%ld, contribid=%ld, rc=%ld",
                  l_FL_Counter, pJobId, pHandle, pContribId, rc, 0);
    }

    return rc;
}

int ContribIdFile::update_xbbServerContribIdFileResetForRestart(const LVKey* pLVKey, const uint64_t pJobId, const uint64_t pJobStepId, const uint64_t pHandle, const uint32_t pContribId)
{
    int rc = 0;

    uint64_t l_FL_Counter = metadataCounter.getNext();
    FL_Write(FLMetaData, CIF_UpdateResetRestart, "update contribid file, counter=%ld, jobid=%ld, handle=%ld, contribid=%ld", l_FL_Counter, pJobId, pHandle, pContribId);

    ContribIdFile* l_ContribIdFile = 0;
    bfs::path l_HandleFilePath(config.get("bb.bbserverMetadataPath", DEFAULT_BBSERVER_METADATAPATH));
    l_HandleFilePath /= bfs::path(to_string(pJobId));
    l_HandleFilePath /= bfs::path(to_string(pJobStepId));
    l_HandleFilePath /= bfs::path(to_string(pHandle));
    rc = loadContribIdFile(l_ContribIdFile, pLVKey, l_HandleFilePath, pContribId);
    switch (rc)
    {
        case 1:
        {
#ifndef __clang_analyzer__  // zeroing rc is not necessary, but safer to have this here
            rc = 0;
#endif
            uint64_t l_Flags = l_ContribIdFile->flags;
            uint64_t l_NewFlags = l_Flags & BB_ResetContribIdForRestartFlagsMask;

            if (l_Flags != l_NewFlags)
            {
                LOG(bb,info) << "xbbServer: For " << *pLVKey << ", handle " << pHandle << ", contribid " << pContribId << ":";
                LOG(bb,info) << "           ContribId flags changing from 0x" << hex << uppercase << l_Flags << " to 0x" << l_NewFlags << nouppercase << dec << ".";
            }

            l_ContribIdFile->flags = l_NewFlags;

            rc = saveContribIdFile(l_ContribIdFile, pLVKey, l_HandleFilePath, pContribId);
            if (!rc) {
                // NOTE:  Pass a zero for the size.  Size is only updated when the last extent for a file is transferred.
                // NOTE:  We invoke update_xbbServerHandleStatus() here instead of update_xbbServerHandleFile().  This is because
                //        none of the attributes that we turn on for the contribid file are absolute to the handle file.  When turning
                //        on an attribute, it has to be calculated along with information from the other contributors.  Update status
                //        performs this logic.  When turning off attribute(s) (as here, with restart logic), some of these are absolute to the
                //        handle file but the same calculation logic in update status will turn off those attributes in the handle file.
                //        This calculation of an attribute is only performed by update status when a FULL_SCAN is performed.
                // NOTE:  If we are turning a flag off, specify that a FULL_SCAN be performed by update_xbbServerHandleStatus()
                //        so that the corresponding attribute in the handle file is set correctly.
                if (HandleFile::update_xbbServerHandleStatus(pLVKey, pJobId, pJobStepId, pHandle, pContribId, 0, 0, FULL_SCAN))
                {
                    LOG(bb,error) << "ContribIdFile::update_xbbServerContribIdFileResetForRestart():  Failure when attempting to update the cross bbServer handle status for jobid " << pJobId \
                                  << ", jobstepid " << pJobStepId << ", handle " << pHandle << ", contribid " << pContribId;
                }
            } else {
                LOG(bb,error) << "ContribIdFile::update_xbbServerContribIdFileResetForRestart():  Failure when attempting to save the cross bbServer contribs file for jobid " << pJobId \
                              << ", jobstepid " << pJobStepId << ", handle " << pHandle << ", contribid " << pContribId;
            }

            break;
        }
        case 0:
        {
            rc = -1;
            LOG(bb,error) << "ContribId " << pContribId << " could not be found in the contrib file for jobid " << pJobId << ", jobstepid " << pJobStepId << ", handle " << pHandle << ", " << *pLVKey << ", using handle path " << l_HandleFilePath.string();

            break;
        }
        default:
        {
            LOG(bb,error) << "Could not load the contrib file for jobid " << pJobId << ", jobstepid " << pJobStepId << ", handle " << pHandle << ", " << *pLVKey << ", using handle path " << l_HandleFilePath.string();
        }
    }


    if (l_ContribIdFile)
    {
        FL_Write6(FLMetaData, CIF_UpdateResetRestart_End, "update contribid file, counter=%ld, jobid=%ld, handle=%ld, contribid=%ld, flags=0x%lx, rc=%ld",
                  l_FL_Counter, pJobId, pHandle, pContribId, l_ContribIdFile->flags, rc);
        delete l_ContribIdFile;
        l_ContribIdFile = 0;
    }
    else
    {
        FL_Write6(FLMetaData, CIF_UpdateResetRestart_ErrEnd, "update contribid file, counter=%ld, jobid=%ld, handle=%ld, contribid=%ld, rc=%ld",
                  l_FL_Counter, pJobId, pHandle, pContribId, rc, 0);
    }

    return rc;
}

int ContribIdFile::update_xbbServerFileStatus(const LVKey* pLVKey, BBTransferDef* pTransferDef, ExtentInfo& pExtentInfo, const uint64_t pFlags, const int pValue) {

    return update_xbbServerFileStatus(pLVKey, pTransferDef, pExtentInfo.getHandle(), pExtentInfo.getContrib(), pExtentInfo.getExtent(), pFlags, pValue);
};

int ContribIdFile::update_xbbServerFileStatus(const LVKey* pLVKey, BBTransferDef* pTransferDef, uint64_t pHandle, uint32_t pContribId, Extent* pExtent, const uint64_t pFlags, const int pValue)
{
    int rc = 0;
    int32_t l_NumOfContribsBump = 0;

    uint64_t l_FL_Counter = metadataCounter.getNext();
    FL_Write(FLMetaData, CIF_UpdateStatus, "update contribid status, counter=%ld, jobid=%ld, handle=%ld, contribid=%ld", l_FL_Counter, pTransferDef->getJobId(), pHandle, pContribId);
    LOG(bb,debug) << "update_xbbServerFileStatus(): Input " << *pLVKey << ", handle " << pHandle << ", contribid " << pContribId << ", sourceindex " << pExtent->sourceindex << ":" \
                  << " pFlags=0x" << hex << uppercase << pFlags << nouppercase << dec << ", pValue=" << pValue;

    ContribIdFile* l_ContribIdFile = 0;
    bfs::path l_HandleFilePath(config.get("bb.bbserverMetadataPath", DEFAULT_BBSERVER_METADATAPATH));
    l_HandleFilePath /= bfs::path(to_string(pTransferDef->getJobId()));
    l_HandleFilePath /= bfs::path(to_string(pTransferDef->getJobStepId()));
    l_HandleFilePath /= bfs::path(to_string(pHandle));
    rc = loadContribIdFile(l_ContribIdFile, pLVKey, l_HandleFilePath, pContribId);
    switch (rc)
    {
        case 1:
        {
            rc = 0;
            if ((pExtent->sourceindex%2) == 0)
            {
                uint32_t l_FileIndex = (pExtent->sourceindex)/2;
                ContribIdFile::FileData* l_FileData = &(l_ContribIdFile->files[l_FileIndex]);

                uint64_t l_ContribIdFlags = l_ContribIdFile->flags;
                uint64_t l_NewContribIdFlags = l_ContribIdFlags;

                uint64_t l_Flags = l_FileData->flags;
                uint64_t l_IncomingFlags = l_Flags;
                uint64_t l_NewFlags;

                bool l_UpdateSize = false;

                // NOTE: If we are about to turn on the 'all_extents_transferred' flag
                //       for this individual file, first check to make sure that the 'stopped', 'failed'
                //       or 'canceled' flag is not on for the transfer definition.  If so,
                //       first reflect that "stopped", 'failed' or 'canceled' status in this individual
                //       file's status flags.
                // NOTE: When the 'stopped', 'failed' or 'canceled' indication is turned on for a transfer
                //       definition, that indication is not cascaded down to the individual files
                //       at that time.  Therefore, we reflect such indications at the time the
                //       last extent is being processed for a file.
                // NOTE: If we were to later turn any of these flags off for the transfer definition
                //       or an individual file, backing out the following 'cascade logic' is not supported.
                //       However, current processing never tries to 'back out' such indications.
                // NOTE: With respect to the above note, for the restart transfer logic, the entire
                //       handle/LVUuid/transfer definition flags and all associated cross bbServer metadata
                //       for the effected transfer definition is entirely reset.  Status is then
                //       recalculated after the reset and then transfers are queued.
                // NOTE: If we are turning on an attribute, we check to see if the BBTD_All_Extents_Transferred
                //       bit is on for the transfer definition.  We could have previously indicated that all
                //       extents were processed (and incremented the total size transfered for the handle by the
                //       file size at that time), but then had a problem in sending the message to bbProxy
                //       to sync/close the file.  In that case, we come back after the BBTD_All_Extents_Transferred
                //       bit has been set and indicate that the transfer for the file is now BBTD_Failed.
                //       Therefore, in this case, we re-check for any files that are stopped/failed/canceled
                //       and in the failed case, adjust the total amount of data transfered for the handle.
                //       In the 'normal' failover cases, this file will be restarted, re-transferred,
                //       and that size then added back to the size transfered for the handle.
                // NOTE: If we are turning on an attribute, we check to see if the BBTD_All_Files_Closed
                //       bit is on for the transfer definition or we are now turning it on for the transfer
                //       definition.  In those cases, we again check to see if the transfer definition is
                //       stopped/failed/canceled.  In the case that we are now turning on the
                //       BBTD_All_Files_Closed bit, the close may have still failed and the transfer
                //       definition is now marked as failed.  In that case, we want to make sure that
                //       is reflected in the handle status.
                if (pValue)
                {
                    if ((pFlags & BBTD_All_Extents_Transferred) || (l_ContribIdFlags & BBTD_All_Extents_Transferred) ||
                        (pFlags & BBTD_All_Files_Closed) || (l_ContribIdFlags & BBTD_All_Files_Closed))
                    {
                        if (pFlags & BBTD_All_Extents_Transferred)
                        {
                            l_UpdateSize = true;
                        }
                        if (l_ContribIdFlags & BBTD_Stopped || pFlags & BBTD_Stopped)
                        {
                            SET_FLAG_VAR(l_Flags, l_Flags, BBTD_Stopped, pValue);
                            l_UpdateSize = false;
                        }
                        if (l_ContribIdFlags & BBTD_Failed || pFlags & BBTD_Failed)
                        {
                            SET_FLAG_VAR(l_Flags, l_Flags, BBTD_Failed, pValue);
                            l_UpdateSize = false;
                        }
                        if (l_ContribIdFlags & BBTD_Canceled || pFlags & BBTD_Canceled)
                        {
                            SET_FLAG_VAR(l_Flags, l_Flags, BBTD_Canceled, pValue);
                            l_UpdateSize = false;
                        }
                    }
                }

                // For restart logic, if BBTD_All_Extents_Transferred is being turned off, turn off BBTD_All_Files_Closed also.
                if ((!pValue) && (pFlags & BBTD_All_Extents_Transferred))
                {
                    SET_FLAG_VAR(l_Flags, l_Flags, BBTD_All_Files_Closed, pValue);
                    SET_FLAG_VAR(l_NewContribIdFlags, l_NewContribIdFlags, BBTD_All_Files_Closed, pValue);
                }

                // Set the new file flags...
                SET_FLAG_VAR(l_NewFlags, l_Flags, pFlags, pValue);
                l_FileData->flags = l_NewFlags;

                // Adjust the transfer size if all the extents have been successfully transferred...
                uint64_t l_IncomingTransferSize = l_ContribIdFile->totalTransferSize;
                if (l_UpdateSize)
                {
                    l_ContribIdFile->totalTransferSize += BBTransferDef::getTotalTransferSize(pTransferDef, pExtent->sourceindex);
                }

                // Adjust the transfer size if we are now failing an individual file transfer after BBTD_All_Extents_Transferred is set...
                // NOTE: Today, only BBTD_Failed after BBTD_All_Extents_Transferred has been set for a file would cause the transfer
                //       size to be decremented.  BBTD_Stopped and BBTD_Canceled are included below for completeness.
                if (l_IncomingFlags & BBTD_All_Extents_Transferred)
                {
                    if ((!(l_IncomingFlags & BBTD_Stopped)) && (!(l_IncomingFlags & BBTD_Failed)) && (!(l_IncomingFlags & BBTD_Canceled)))
                    {
                        if ((l_NewFlags & BBTD_Stopped) || (l_NewFlags & BBTD_Failed) || (l_NewFlags & BBTD_Canceled))
                        {
                            l_ContribIdFile->totalTransferSize -= BBTransferDef::getTotalTransferSize(pTransferDef, pExtent->sourceindex);
                        }
                    }
                }

                // Now, adjust any ContribId flag values...
                //
                // Only set the BBTD_All_Files_Closed flag for the ContribIdFile if the BBTD_All_Extents_Transferred is set for the overall ContribId file
                if (!(l_NewContribIdFlags & BBTD_All_Files_Closed))
                {
                    if ((l_NewContribIdFlags & BBTD_All_Extents_Transferred) || (pValue && (pFlags & BBTD_All_Extents_Transferred)))
                    {
                        if (l_ContribIdFile->allFilesClosed())
                        {
                            SET_FLAG_VAR(l_NewContribIdFlags, l_NewContribIdFlags, BBTD_All_Files_Closed, 1);
                        }
                    }
                }

                // Set the new ContribId flag values...
                l_ContribIdFile->flags = l_NewContribIdFlags;

                // If not for restart, *and* 'extents enqueued' is not currently on in the current ContribId flags, *and*
                // we are now turning on 'extents enqueued', bump the number of reporting contributors
                if ((!(pTransferDef->builtViaRetrieveTransferDefinition())) && (!(l_ContribIdFlags & BBTD_Extents_Enqueued)) && (pValue && (pFlags & BBTD_Extents_Enqueued)))
                {
                    l_NumOfContribsBump = 1;
                }

                if ((l_ContribIdFlags != l_NewContribIdFlags) || (l_IncomingFlags != l_NewFlags) || (l_IncomingTransferSize != l_ContribIdFile->totalTransferSize))
                {
                    LOG(bb,info) << "xbbServer: For " << *pLVKey << ", handle " << pHandle << ", contribid " << pContribId << ", sourceindex " << pExtent->sourceindex << ":";
                    if (l_ContribIdFlags != l_NewContribIdFlags)
                    {
                        LOG(bb,info) << "           ContribId flags changing from 0x" << hex << uppercase << l_ContribIdFlags << " to 0x" << l_NewContribIdFlags << nouppercase << dec << ".";
                    }
                    if (l_IncomingFlags != l_NewFlags)
                    {
                        LOG(bb,info) << "           File flags changing from 0x" << hex << uppercase << l_IncomingFlags << " to 0x" << l_NewFlags << nouppercase << dec << ".";
                    }
                    if (l_IncomingTransferSize != l_ContribIdFile->totalTransferSize)
                    {
                        LOG(bb,info) << "           ContribId transferred size changing from " << l_IncomingTransferSize << " to " << l_ContribIdFile->totalTransferSize << ".";
                    }

                    // Save the contribid file
                    rc = saveContribIdFile(l_ContribIdFile, pLVKey, l_HandleFilePath, pContribId);
                    if (!rc)
                    {
                        int64_t l_SizeDelta = (int64_t)(l_ContribIdFile->totalTransferSize-l_IncomingTransferSize);
                        // Only update the handle file status if the transfer size/status could change
                        if ((!pValue) || l_SizeDelta || pFlags & BB_UpdateHandleStatusMask1)
                        {
                            if (HandleFile::update_xbbServerHandleStatus(pLVKey, pTransferDef->getJobId(), pTransferDef->getJobStepId(), pHandle, pContribId, l_NumOfContribsBump, l_SizeDelta, NORMAL_SCAN))
                            {
                                LOG(bb,error) << "ContribIdFile::update_xbbServerFileStatus():  Failure when attempting to update the cross bbServer handle status for jobid " << pTransferDef->getJobId() \
                                              << ", jobstepid " << pTransferDef->getJobStepId() << ", handle " << pHandle << ", contribid " << pContribId;
                            }
                        }
                    }
                    else
                    {
                        LOG(bb,error) << "ContribIdFile::update_xbbServerFileStatus():  Failure when attempting to save the cross bbServer contribs file for jobid " << pTransferDef->getJobId() \
                                      << ", jobstepid " << pTransferDef->getJobStepId() << ", handle " << pHandle << ", contribid " << pContribId;
                    }
                }
            }
            else
            {
                LOG(bb,error) << "ContribIdFile::update_xbbServerFileStatus():  Invalid sourceindex " << pExtent->sourceindex << " for jobid " << pTransferDef->getJobId() \
                              << ", jobstepid " << pTransferDef->getJobStepId() << ", handle " << pHandle << ", contribid " << pContribId;
            }

            break;
        }
        case 0:
        {
            rc = -1;
            LOG(bb,error) << "ContribId " << pContribId << " could not be found in the contrib file for jobid " << pTransferDef->getJobId() << ", jobstepid " << pTransferDef->getJobStepId() << ", handle " << pHandle << ", " << *pLVKey << ", using handle path " << l_HandleFilePath.string();

            break;
        }
        default:
        {
            LOG(bb,error) << "Could not load the contribid file for jobid " << pTransferDef->getJobId() << ", jobstepid " << pTransferDef->getJobStepId() << ", handle " << pHandle << ", " << *pLVKey << ", contribid " << pContribId << ", using handle path " << l_HandleFilePath.string();
        }
    }


    if (l_ContribIdFile)
    {
        FL_Write6(FLMetaData, CIF_UpdateStatus_End, "update contribid status, counter=%ld, jobid=%ld, handle=%ld, contribid=%ld, flags=0x%lx, rc=%ld", \
                  l_FL_Counter, pTransferDef->getJobId(), pHandle, pContribId, l_ContribIdFile->flags, rc);
        delete l_ContribIdFile;
        l_ContribIdFile = 0;
    }
    else
    {
        FL_Write6(FLMetaData, CIF_UpdateStatus_ErrEnd, "update contribid status, counter=%ld, jobid=%ld, handle=%ld, contribid=%ld, rc=%ld", \
                  l_FL_Counter, pTransferDef->getJobId(), pHandle, pContribId, rc, 0);
    }

    return rc;
}

int ContribIdFile::update_xbbServerFileStatusForRestart(const LVKey* pLVKey, BBTransferDef* pRebuiltTransferDef, uint64_t pHandle, uint32_t pContribId, int64_t &pSize)
{
    int rc = 0;
    typedef pair<uint16_t, size_t> BUNDLE_ID_ENTRY;

    uint64_t l_FL_Counter = metadataCounter.getNext();
    FL_Write(FLMetaData, CIF_UpdateStatusForRestart, "update contribid status for restart, counter=%ld, jobid=%ld, handle=%ld, contribid=%ld", l_FL_Counter, pRebuiltTransferDef->getJobId(), pHandle, pContribId);

    ContribIdFile* l_ContribIdFile = 0;
    int64_t l_Size = 0;
    bfs::path l_HandleFilePath(config.get("bb.bbserverMetadataPath", DEFAULT_BBSERVER_METADATAPATH));
    l_HandleFilePath /= bfs::path(to_string(pRebuiltTransferDef->getJobId()));
    l_HandleFilePath /= bfs::path(to_string(pRebuiltTransferDef->getJobStepId()));
    l_HandleFilePath /= bfs::path(to_string(pHandle));
    rc = loadContribIdFile(l_ContribIdFile, pLVKey, l_HandleFilePath, pContribId);
    switch (rc)
    {
        case 1:
        {
#ifndef __clang_analyzer__  // zeroing rc is not necessary, but defensive coding
            rc = 0;
#endif
            vector<BUNDLE_ID_ENTRY> l_NonZeroBundlesWithStoppedFile;
            for (size_t i=0; i<l_ContribIdFile->files.size(); ++i)
            {
                FileData* l_File = &((l_ContribIdFile->files)[i]);
                if (l_File->fileToBeRestarted())
                {
                    l_File->flags &= BB_ResetContribIdFileForRestartFlagsMask;
                    // NOTE:  Currently, we don't preserve the previous size transferred
                    //        in the rebuilt transfer definition.  It is not needed
                    //        as part of any restart transfer definition processing.
                    //        It will be zero.
                    l_Size += (int64_t)(pRebuiltTransferDef->sizeTransferred[i*2]);
                    pRebuiltTransferDef->sizeTransferred[i*2] = 0;
                    uint16_t l_BundleId = BBFileBundleIDFromFlags(l_File->flags);
                    if (l_BundleId)
                    {
                        BUNDLE_ID_ENTRY l_Value = make_pair(l_BundleId, i);
                        l_NonZeroBundlesWithStoppedFile.push_back(l_Value);
                    }
                    LOG(bb,info) << "xbbServer: Flags and size transferred reset prior to restart for " << *pLVKey << ", handle " << pHandle << ", contribid " << pContribId << ", sourceindex " << i*2;
                }
            }

            for (size_t i=0; i<l_ContribIdFile->files.size(); ++i)
            {
                FileData* l_File = &((l_ContribIdFile->files)[i]);
                uint16_t l_BundleId = BBFileBundleIDFromFlags(l_File->flags);
                if (l_BundleId)
                {
                    for (size_t j=0; i<l_NonZeroBundlesWithStoppedFile.size(); ++j)
                    {
                        if (l_NonZeroBundlesWithStoppedFile[j].first == l_BundleId && l_NonZeroBundlesWithStoppedFile[j].second != i)
                        {
                            l_File->flags &= BB_ResetContribIdFileForRestartFlagsMask;
                            // NOTE:  Currently, we don't preserve the previous size transferred
                            //        in the rebuilt transfer definition.  It is not needed
                            //        as part of any restart transfer definition processing.
                            //        It will be zero.
                            l_Size += (int64_t)(pRebuiltTransferDef->sizeTransferred[i*2]);
                            pRebuiltTransferDef->sizeTransferred[i*2] = 0;
                            LOG(bb,info) << "xbbServer: Flags and size transferred reset prior to restart for " << *pLVKey << ", handle " << pHandle << ", contribid " << pContribId << ", sourceindex " << i*2;
                        }
                    }
                }
            }

            string l_Hostname;
            activecontroller->gethostname(l_Hostname);
            if (l_ContribIdFile->hostname != l_Hostname)
            {
                LOG(bb,info) << "xbbServer: Servicing hostname for " << *pLVKey << ", handle " << pHandle << ", contribid " << pContribId << " changed from " << l_ContribIdFile->hostname << " to " << l_Hostname;
                l_ContribIdFile->hostname = l_Hostname;
            }

            rc = saveContribIdFile(l_ContribIdFile, pLVKey, l_HandleFilePath, pContribId);
            if (rc)
            {
                LOG(bb,error) << "ContribIdFile::update_xbbServerFileStatus():  Failure when attempting to save the cross bbServer contribs file for jobid " << pRebuiltTransferDef->getJobId() \
                              << ", jobstepid " << pRebuiltTransferDef->getJobStepId() << ", handle " << pHandle << ", contribid " << pContribId;
            }

            break;
        }

        case 0:
        {
            rc = -1;
            LOG(bb,error) << "ContribId " << pContribId << " could not be found in the contrib file for jobid " << pRebuiltTransferDef->getJobId() << ", jobstepid " << pRebuiltTransferDef->getJobStepId() << ", handle " << pHandle << ", " << *pLVKey << ", using handle path " << l_HandleFilePath.string();

            break;
        }

        default:
        {
            LOG(bb,error) << "Could not load the contribid file for jobid " << pRebuiltTransferDef->getJobId() << ", jobstepid " << pRebuiltTransferDef->getJobStepId() << ", handle " << pHandle << ", " << *pLVKey << ", contribid " << pContribId << ", using handle path " << l_HandleFilePath.string();
        }
    }

    if (l_ContribIdFile)
    {
        delete l_ContribIdFile;
        l_ContribIdFile = 0;
    }

    if (!rc)
    {
        pSize = -l_Size;
    }

    FL_Write6(FLMetaData, CIF_UpdateStatusForRestart_End, "update contribid status for restart, counter=%ld, jobid=%ld, handle=%ld, contribid=%ld, rc=%ld",
              l_FL_Counter, pRebuiltTransferDef->getJobId(), pHandle, pContribId, rc, 0);

    return rc;
}


/*
 * Non-static methods
 */

#if BBSERVER
int ContribIdFile::copyForRetrieveTransferDefinitions(BBTransferDefs& pTransferDefs, const string& pHostName, const uint64_t pJobId, const uint64_t pJobStepId, const uint64_t pHandle, const uint32_t pContribId, const string& pTransferKeys)
{
    int rc = 0;

    BBTransferDef* l_TransferDef = new BBTransferDef();
    l_TransferDef->serializeVersion = serializeVersion;
    l_TransferDef->objectVersion = objectVersion;
    l_TransferDef->job = BBJob(pJobId, pJobStepId);
    l_TransferDef->uid = uid;
    l_TransferDef->gid = gid;
    l_TransferDef->contribid = pContribId;
    l_TransferDef->flags = flags & BB_RetrieveTransferDefinitionsFlagsMask;
    l_TransferDef->tag = tag;
    l_TransferDef->transferHandle = pHandle;
    l_TransferDef->hostname = pHostName;
    l_TransferDef->update = PTHREAD_MUTEX_INITIALIZER;  // Simply initialized
    l_TransferDef->extents = vector<Extent>();          // Create an empty extent vector

    // Copy the file entries over, adding a extent to the extent vector for each pair
    uint32_t i = 0;
    for (auto& fdata : files)
    {
        l_TransferDef->files.push_back(fdata.sourceFile);
        l_TransferDef->files.push_back(fdata.targetFile);
        Extent l_Extent = Extent(i, i+1, (fdata.flags & BB_AddFilesFlagsMask));
        l_TransferDef->extents.push_back(l_Extent);
        i += 2;
    }

//    l_TransferDef->keyvalues = keyvalues;     // \todo - Not sure to include all keys or no keys...
    l_TransferDef->iomap = map<uint16_t, BBIO*>();      // No BBIO objects
    l_TransferDef->sizeTransferred = vector<size_t>();  // No transfer sizes

    l_TransferDef->setBuiltViaRetrieveTransferDefinition();

    pTransferDefs.add(l_TransferDef);

    return rc;
}
#endif
