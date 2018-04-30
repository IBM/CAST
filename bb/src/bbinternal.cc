/*******************************************************************************
 |    bbinternal.cc
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

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include <execinfo.h>

#include <boost/tokenizer.hpp>

#include "bbflags.h"

using namespace std;

#include "bbinternal.h"
#include "connections.h"

// Helper methods
int logBacktrace()
{
    void*  traceBuf[256];
    int traceSize;
    char** backtraceList;

    traceSize = backtrace(traceBuf, sizeof(traceBuf));
    backtraceList = backtrace_symbols(traceBuf, traceSize);
    for(int x=0; x<traceSize; x++)
    {
	LOG(bb,info) << "backtrace: " << backtraceList[x] << "  [" << traceBuf[x] << "]";
    }
    return 0;
}

string resolveServerConfigKey(const string& pKey)
{
    string l_Key;

    if (ProcessId.find("bb.proxy") != std::string::npos)
    {
        // bbProxy process
        if (pKey.size()) {
            l_Key = config.get(ProcessId + ".servercfg", "bb.server0") + "." + pKey;
        } else {
            l_Key = config.get(ProcessId + ".servercfg", "bb.server0");
        }
    }
    else
    {
        // bbServer process
        if (pKey.size()) {
            l_Key = ProcessId + "." + pKey;
        } else {
            l_Key = ProcessId;
        }
    }

    return l_Key;
}

// NOTE:  Currently readVar() only supports reading ascii values as positive integers.
//        If no value is found, -1 is returned.
int32_t readVar(const char* pVariable)
{
    int32_t l_Value = -1;

    if (!((strstr(pVariable, "jobid")) || (strstr(pVariable, "jobstepid")) || (strstr(pVariable, "contribid"))))
    {
        string l_VarPath = config.get(process_whoami+".bringup.testVariablePath", DEFAULT_VARIABLE_PATH);
        char l_FileName[PATH_MAX+1] = {'\0'};
        snprintf(l_FileName, sizeof(l_FileName), "%s/%s", l_VarPath.c_str(), pVariable);

        ifstream l_InFile;
        char l_Buffer[256] = {'\0'};

        l_InFile.open (l_FileName, ios::in);
        if (l_InFile.is_open()) {
            l_InFile >> l_Buffer;
            l_InFile.close();
        }

        if (strlen(l_Buffer) > 0) {
            l_Value = atoi(l_Buffer);
        }

    }

    return l_Value;
}

int sameHostName(const string& pHostName)
{
    string l_HostName;
    activecontroller->gethostname(l_HostName);

    return (l_HostName == pHostName ? 1 : 0);
}

// NOTE:  Currently writeVar() only supports writing ascii values to be
//        read as positive integers.
void writeVar(const char* pVariable, const char* pValue)
{
    std::string l_VarPath = config.get(process_whoami+".bringup.testVariablePath", DEFAULT_VARIABLE_PATH);
    char l_FileName[PATH_MAX+1] = {'\0'};
    snprintf(l_FileName, sizeof(l_FileName), "%s/%s", l_VarPath.c_str(), pVariable);

    ofstream l_OutFile;

    l_OutFile.open (l_FileName, ios::out | ios::trunc);
    if (strlen(pValue)) {
        l_OutFile << pValue;
    }
    l_OutFile.close();

    return;
}

BBSTATUS getBBStatusFromStr(const char* pStatusStr)
{
    BBSTATUS l_Status = BBNONE;

    if (strcmp(pStatusStr, "BBNOTSTARTED") == 0)
        l_Status = BBNOTSTARTED;
    else if (strcmp(pStatusStr, "BBINPROGRESS") == 0)
        l_Status = BBINPROGRESS;
    else if (strcmp(pStatusStr, "BBPARTIALSUCCESS") == 0)
        l_Status = BBPARTIALSUCCESS;
    else if (strcmp(pStatusStr, "BBFULLSUCCESS") == 0)
        l_Status = BBFULLSUCCESS;
    else if (strcmp(pStatusStr, "BBCANCELED") == 0)
        l_Status = BBCANCELED;
    else if (strcmp(pStatusStr, "BBFAILED") == 0)
        l_Status = BBFAILED;
    else if (strcmp(pStatusStr, "BBSTOPPED") == 0)
        l_Status = BBSTOPPED;
    else if (strcmp(pStatusStr, "BBNOTACONTRIB") == 0)
        l_Status = BBNOTACONTRIB;
    else if (strcmp(pStatusStr, "BBNOTREPORTED") == 0)
        l_Status = BBNOTREPORTED;
    else if (strcmp(pStatusStr, "BBALL") == 0)
        l_Status = BBALL;

    return l_Status;
}

void getStrFromBBStatus(BBSTATUS pValue, char* pBuffer, const size_t pSize)
{
    if (pSize) {
        pBuffer[0] = '\0';
        switch (pValue) {
            case BBNONE:
                strncpy(pBuffer, "BBNONE", pSize);
                break;
            case BBNOTSTARTED:
                strncpy(pBuffer, "BBNOTSTARTED", pSize);
                break;
            case BBINPROGRESS:
                strncpy(pBuffer, "BBINPROGRESS", pSize);
                break;
            case BBPARTIALSUCCESS:
                strncpy(pBuffer, "BBPARTIALSUCCESS", pSize);
                break;
            case BBFULLSUCCESS:
                strncpy(pBuffer, "BBFULLSUCCESS", pSize);
                break;
            case BBCANCELED:
                strncpy(pBuffer, "BBCANCELED", pSize);
                break;
            case BBFAILED:
                strncpy(pBuffer, "BBFAILED", pSize);
                break;
            case BBSTOPPED:
                strncpy(pBuffer, "BBSTOPPED", pSize);
                break;
            case BBNOTACONTRIB:
                strncpy(pBuffer, "BBNOTACONTRIB", pSize);
                break;
            case BBNOTREPORTED:
                strncpy(pBuffer, "BBNOTREPORTED", pSize);
                break;
            case BBALL:
                strncpy(pBuffer, "BBALL", pSize);
                break;

            default:
                snprintf(pBuffer, pSize, "%s (%zu)", "UNDEFINED", pValue);
        }
    }

    return;
}

void getStrFromTransferType(const uint64_t pFlags, char* pBuffer, const size_t pSize)
{
    if (pSize) {
        pBuffer[0] = '\0';
        if (pFlags & BBI_TargetSSD)
        {
            strCpy(pBuffer, "PFS_to_SSD", pSize);
        }
        else if (pFlags & BBI_TargetPFS)
        {
            strCpy(pBuffer, "SSD_to_PFS", pSize);
        }
        else if (pFlags & BBI_TargetSSDSSD)
        {
            strCpy(pBuffer, "SSD_to_SSD", pSize);
        }
        else if (pFlags & BBI_TargetPFSPFS)
        {
            strCpy(pBuffer, "PFS_to_PFS", pSize);
        }
        else
        {
            strCpy(pBuffer, "Unknown", pSize);
        }
    }

    return;
}
