/*******************************************************************************
 |    bbflags.h
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

#ifndef BB_BBFLAGS_H_
#define BB_BBFLAGS_H_

/*******************************************************************************
 | Macro definitions
 *******************************************************************************/
#define RETURN_FLAG(FLAG) \
    return ((flags & FLAG) ? 1 : 0);

#define SET_FLAG(FLAG,VALUE) \
    if (VALUE) { \
        flags |= FLAG; \
    } else { \
        flags &= (~FLAG); \
    }

#define SET_FLAG_AND_RETURN(FLAG,VALUE) \
    SET_FLAG(FLAG,VALUE); \
    return;

#define SET_FLAG_VAR(VAR,CURFLAG,FLAG,VALUE) \
    VAR = CURFLAG; \
    if (VALUE) { \
        VAR |= FLAG; \
    } else { \
        VAR &= (~FLAG); \
    }

/*******************************************************************************
 | Flag definitions
 *******************************************************************************/
// NOTE: Low order first nibble of Extent.flags and xbbServer metadata (ContribIdFile).
//       These are the values for BBFILEFLAGS.  These are the bit values externalized on bbapi/bbcmd.
// NOTE: The high order two bits of the first nibble are currently used in testing...  @DLH
#define BBI_Recursive                                 0x0000000000000001

// NOTE: Low order second nibble of Extent.flags and xbbServer metadata (ContribIdFile).
// NOTE: Because 'regular extents' do not have a bit value, use the
//       BBTransferTypeFromFlags(flag) macro and check for a zero value.
#define BBI_BSCFS_File_Transfer                       0x0000000000000010

// NOTE: Low order third and fourth nibbles of Extent.flags and xbbServer metadata (ContribIdFile).
//       These are the values for BBFILEFLAGS (BundleID value).

// NOTE: Low order fifth nibble of Extent.flags and xbbServer metadata (ContribIdFile)
#define BBI_TargetSSD                                 0x0000000000010000
#define BBI_TargetPFS                                 0x0000000000020000
#define BBI_TargetSSDSSD                              0x0000000000040000
#define BBI_TargetPFSPFS                              0x0000000000080000

// NOTE: Low order sixth nibble of Extent.flags and xbbServer metadata (ContribIdFile)
#define BBI_First_Extent                              0x0000000000100000
#define BBI_Last_Extent                               0x0000000000200000
#define BBI_Trim_Anchor                               0x0000000000400000
#define BBI_CP_Transfer                               0x0000000000800000

// NOTE: Low order seventh and eighth nibbles of Extent.flags and xbbServer metadata (ContribIdFile) (not defined)

// NOTE: Low order ninth nibble of BBTagInfo.flags and xbbServer metadata (HandleFile)
// NOTE: For stop transfers, this bit is unconditionally turned off because one or more contribids
//       will be removed from the contribs file.
#define BBTI_All_Contribs_Reported                    0x0000000100000000

// NOTE: Low order tenth nibble of Extent.flags and xbbServer metadata (ContribIdFile) (not defined)

// NOTE: Low order eleventh nibble of BBLV_ExtentInfo.flags and xbbServer metadata (LVUuidFile, first three bits only)
// NOTE: For stop transfers, these bits are not altered.  If new CNs will be introduced as part of the ensuing restart,
//       new LVUuidFile(s) will be created and they will have their own set of bits.  If a new ESS is introduced as part
//       of the ensuing restart, these bits will still apply.
#define BBLV_Stage_Out_Start                         0x0000010000000000
#define BBLV_Stage_Out_End                           0x0000020000000000
#define BBLV_Stage_Out_End_Complete                  0x0000040000000000
#define BBLV_Suspended                               0x0000080000000000

// NOTE: Low order twelfth nibble of BBTransferDef.flags
// NOTE: BBTD_All_CN_CP_Transfers is set in the transfer definition by bbbproxy processing,
//       but bbserver does not use this for decision making...
#define BBTD_All_CN_CP_Transfers                      0x0000100000000000
#define BBTD_No_Stagein_Or_Stageout_Transfers         0x0000200000000000
#define BBTD_Built_Via_Retrieve_Transfer_Definition   0x0000400000000000
#define BBTD_Extents_Enqueued                         0x0000800000000000

// NOTE: Low order thirteenth nibble of BBTransferDef.flags and BBLV_ExtentInfo.flags
// NOTE: BBLV_ExtentInfo::mergeFlags() is dependent on the nibble location of the flags shared
//       between BBTransferDef.flags and BBLV_ExtentInfo.flags.  If these nibbles move, verify that the
//       merge code is still correct.
#define BBTD_Resize_Logical_Volume_During_Stageout    0x0002000000000000
#define BBTD_BSCFS_In_Request                         0x0004000000000000

// NOTE: Low order fourteenth nibble of BBTransferDef.flags, BBTagInfo.flags, BBLV_ExtentInfo.flags, and
//       xbbServer metadata (ContribIdFile (contribid and each file) and HandleFile)
// NOTE: 'All_Extents_Transferred' doesn't indicate success and/or failure for any of the
//       individual extent transfers.  It only indicates that there are no additional extents
//       to transfer.
// NOTE: The bits in this nibble are also set in the Extent.flags for the dummy extent that is
//       passed to bbServer on the second pass for Start Transfer processing so that the cross
//       bbServer metadata can accurately reflect the status of the local copy for this file.
// NOTE: For stop transfers, this bit will already be off.
#define BBTD_All_Extents_Transferred                  0x0010000000000000

// NOTE: Low order fifteenth nibble of BBTransferDef.flags and xbbServer metadata
//       (HandleFile, ContribIdFile (contribid and each file))
// NOTE: The bits in this nibble are also set in the Extent.flags for the dummy extent that is
//       passed to bbServer on the second pass for Start Transfer processing so that the cross
//       bbServer metadata can accurately reflect the status of the local copy for this file.
// NOTE: The canceled bit in this nibble is also set in the Extent.flags for extents that have
//       had their handle or contribid canceled.
// NOTE: For stop transfers, the BBTD_Failed bit is unconditionally turned off, the BBTD_Canceled bit
//       will already be off, and the BBTD_Stopped is turned on.
#define BBTD_Canceled                                 0x0100000000000000
#define BBTD_Failed                                   0x0200000000000000
#define BBTD_Stopped                                  0x0400000000000000

// NOTE: Low order sixteenth nibble of xbbServer metadata
//       (ContribIdFile (contribid and each file) and HandleFile)
// NOTE: The bits in this nibble are also set in the Extent.flags for the dummy extent that is
//       passed to bbServer on the second pass for Start Transfer processing so that the cross
//       bbServer metadata can accurately reflect the status of the local copy for this file.
// NOTE: For stop transfers, this bit will already be off in the HandleFile.
#define BBTD_All_Files_Closed                         0x1000000000000000

#define BB_AddFilesFlagsMask                          0x00000000000FFFFF
#define BB_ResetContribIdFileForRestartFlagsMask      0x000F7000008FFFFF
#define BB_ResetHandleFileAttributesFlagsMask         0xE8EFFFFFFFFFFFFF
#define BB_RetrieveTransferDefinitionsFlagsMask       0xFFFFF000008FFFFF
#endif /* BB_BBFLAGS_H_ */
