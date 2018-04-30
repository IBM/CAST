/*******************************************************************************
 |    Version.cc
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

#include "Version.h"

//*****************************************************************************
//  Static data members
//*****************************************************************************
#ifdef TXP_DEVELOPMENT
txp::Log txp::Version::d_log(txp::Log::DEFAULT_LOG_DESTINATION, txp::Log::DEFAULT_OPEN_LOGSTATE, txp::Log::DEFAULT_OPEN_LOGLEVEL);
#endif
char txp::Version::d_log_buffer[txp::DEFAULT_LINE_LENGTH] = {'\0'};


//*****************************************************************************
//  Constructors
//*****************************************************************************
txp::Version::Version() {
    setMajor(0);
    setMinor(0);

#ifdef TXP_DEVELOPMENT
	snprintf(d_log_buffer, sizeof(d_log_buffer), "Version::Version()=%p", this);
	d_log.write(d_log_buffer, txp::Log::DEBUG, txp::Log::C_DTOR_LOGLEVEL_2);
#endif
}


txp::Version::Version(const txp::Id pId) {
    *this = getLatestVersion(pId);

#ifdef TXP_DEVELOPMENT
	snprintf(d_log_buffer, sizeof(d_log_buffer), "Version::Version(pId.value=%d)=%p", pId, this);
	d_log.write(d_log_buffer, txp::Log::DEBUG, txp::Log::C_DTOR_LOGLEVEL_2);
#endif
}


txp::Version::Version(const uint8_t pMajor, const uint8_t pMinor) {
    setMajor(pMajor);
    setMinor(pMinor);

#ifdef TXP_DEVELOPMENT
	snprintf(d_log_buffer, sizeof(d_log_buffer), "Version::Version(pMajor=%hhu, pMinor=%hhu)=%p", pMajor, pMinor, this);
	d_log.write(d_log_buffer, txp::Log::DEBUG, txp::Log::C_DTOR_LOGLEVEL_2);
#endif
}


txp::Version::Version(const uint16_t pMajorMinor) {
    set(pMajorMinor);

#ifdef TXP_DEVELOPMENT
	snprintf(d_log_buffer, sizeof(d_log_buffer), "Version::Version(pMajorMinor=0x%04X)=%p", pMajorMinor, this);
	d_log.write(d_log_buffer, txp::Log::DEBUG, txp::Log::C_DTOR_LOGLEVEL_2);
#endif
}


//*****************************************************************************
//  Copy constructor
//*****************************************************************************
txp::Version::Version(const Version &pVersion) {
    setMajor(pVersion.value.major);
    setMinor(pVersion.value.minor);

#ifdef TXP_DEVELOPMENT
	snprintf(d_log_buffer, sizeof(d_log_buffer), "Version::Version(Version=%p)=%p, Version(%d,%d)", &pVersion, this, pVersion.value.major, pVersion.value.minor);
	d_log.write(d_log_buffer, txp::Log::DEBUG, txp::Log::C_DTOR_LOGLEVEL_2);
#endif
}


//*****************************************************************************
//  Destructor
//*****************************************************************************
txp::Version::~Version() {

#ifdef TXP_DEVELOPMENT
	snprintf(d_log_buffer, sizeof(d_log_buffer), "Version::~Version()=%p", this);
	d_log.write(d_log_buffer, txp::Log::DEBUG, txp::Log::C_DTOR_LOGLEVEL_2);
#endif

    // TODO Auto-generated destructor stub
}


//*****************************************************************************
//  Static methods
//*****************************************************************************
//  NOTE: Need to implement support on a per ID basis...  @@DLH
txp::Version txp::Version::getLatestVersion(const txp::Id pId) {
    return Version(txp::TXP_FUSE_KERNEL_VERSION, txp::TXP_FUSE_KERNEL_MINOR_VERSION);
}


//*****************************************************************************
//  Non-static methods
//*****************************************************************************
int txp::Version::add(HeapBuffer* pBuffer) {
    int l_RC = 0;

    if (!(pBuffer->add((void*)&(value.major), sizeof(value.major)))) {
        if (pBuffer->add((void*)&(value.minor), sizeof(value.minor))) {
            l_RC = -252;

#ifdef TXP_DEVELOPMENT
            snprintf(d_log_buffer, sizeof(d_log_buffer), "Version::add(): Minor code, l_RC=%d", l_RC);
            d_log.write(d_log_buffer, txp::Log::ERROR);
#endif
        }
    } else {
        l_RC = -251;

#ifdef TXP_DEVELOPMENT
        snprintf(d_log_buffer, sizeof(d_log_buffer), "Version::add(): Major code, l_RC=%d", l_RC);
        d_log.write(d_log_buffer, txp::Log::ERROR);
#endif
    }

    return l_RC;
}


void txp::Version::dump(Log& pLog, const char* pPrefix) {
    snprintf(d_log_buffer, sizeof(d_log_buffer), "Version:               %d.%d", value.major, value.minor);
    pLog.write(d_log_buffer, txp::Log::DEBUG, txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL);

    return;
}


void txp::Version::set(const uint8_t pMajor, const uint8_t pMinor) {
    value.major = pMajor;
    value.minor = pMinor;

    return;
}


void txp::Version::set(const uint16_t pMajorMinor) {
    value.major_minor = pMajorMinor;

    return;
}


void txp::Version::setMajor(const uint8_t pMajor) {
    value.major = pMajor;

    return;
}


void txp::Version::setMinor(const uint8_t pMinor) {
    value.minor = pMinor;

    return;
}
