/*******************************************************************************
 |    MonitorHandler.h
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

#ifndef MONITOR_HANDLER_H
#define MONITOR_HANDLER_H
//! \file
//! \brief
// Includes


#include <map>

#include <errno.h>
#include <stddef.h>
#include <stdlib.h>

#include "fshipcld.h"
#include "CnxSock.h"
#include "Log.h"
#include "MemChunk.h"
#include "Msg.h"

#define FSHIPMOND_VERSIONSTR "Bringup" ///< Version string for fshipmond

/**
   \brief Fetch the version string
   \par Description
   The fshipmond_GetVersion routine returns the version string for fshipmond.
   This routine is intended for version mismatch debug.

   \param[in] pSize The amount of space provided to hold pVersion
   \param[out] pVersion The string containing the expected version.

   \return Error code
   \retval 0 Success
   \retval errno Positive non-zero values correspond with errno.  strerror() can be used to interpret.
*/
extern int fshipmond_GetVersion(size_t pSize, char* pVersion);

class MonitorHandler
{
public:
    MonitorHandler(txp::ConnexPtr pConnectPtr) :
        _HEADERLENGTH(txp::OFFSET_TO_FIRST_ATTRIBUTE),
        _connectPtr(pConnectPtr) {
        txp::Log _txplog(txp::Log::OPEN);//writes to stdout
    }

    ~MonitorHandler() {
    }

    static void signalHandler(int pSignal, siginfo_t* pSignalInfo, void* pContext);

    int readInMessage(txp::ConnexPtr pCnxptr);
    const int _HEADERLENGTH; //txp::OFFSET_TO_FIRST_ATTRIBUTE in Common.h is the header length.
    void * run();  //!< method for running a thread when threads get coded

private:
    int Hello(txp::Msg* pMsg);

    void dump2txplog(char * buff, int buffSize) {
        txp::Log::dump_buffer_raw(_txplog, buff, buffSize, "dumpbuffer");
    };
    void dumpMsgHeader(char * buff){ txp::Log::dump_buffer_raw(_txplog, buff, _HEADERLENGTH, "header"); }

    /* data */
    txp::ConnexPtr _connectPtr;
    txp::Log _txplog;

//for using file in future
//    txp::Log d_log(txp::Log::LOGFILE, "fshipcld.log");
//    d_log.open();

};  //endclass MonitorHandler

#endif //MONITOR_HANDLER_H

