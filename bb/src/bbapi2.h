/*******************************************************************************
|    bbapi2.h
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

/**
 *  \file bbapi2.h
 *  This file contains the 'internal' user burst buffer APIs
 *  (NOTE: These are internal interfaces provided to bbcmd that are not to be
 *         made generally available.)
 */

#ifndef BB_BBAPI2_H_
#define BB_BBAPI2_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <string>

#include "bbapi_types.h"

// External data
extern std::string ProcessId;

/*******************************************************************************
 | Enumerators
 *******************************************************************************/

/*******************************************************************************
 | Constants
 *******************************************************************************/
#define DEFAULT_RTV_TRANSFERDEFS_FLAGS ONLY_DEFINITIONS_WITH_UNFINISHED_FILES;
const int INVALID_CORAL_GETVAR_VALUE = 999999997;
const int INVALID_CORAL_SETVAR_VALUE = 999999997;

// External routines needed by bbapi.cc
extern int BB_InitLibrary2(uint32_t contribId, const char* clientVersion, const char* unixpath, bool& performCleanup);
extern void cleanupInit();
extern void verifyInit(const bool pExpectedValue);

/*******************************************************************************
 | Service commands
 *******************************************************************************/
extern int Coral_GetVar(const char* pVariable);
// NOTE: Coral_SetVar currently only supports writing ascii values to be read as positive integers
extern int Coral_SetVar(const char* pVariable, const char* pValue);
extern int Coral_StageOutStart(const char* pMountpoint);

/*******************************************************************************
 | Test commands
 *******************************************************************************/
extern int Coral_InitLibrary(uint32_t pContribId, const char* pClientVersion, const char* pConfigFile, const char* pUnixPath=0);

#ifdef __cplusplus
}
#endif

#endif /* BB_BBAPI2_H_ */
