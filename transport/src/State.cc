/*******************************************************************************
 |    State.cc
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

#include "State.h"

//*****************************************************************************
//  Static data members
//*****************************************************************************
#ifdef TXP_DEVELOPMENT
txp::Log txp::State::d_log(txp::Log::DEFAULT_LOG_DESTINATION, txp::Log::DEFAULT_OPEN_LOGSTATE, txp::Log::DEFAULT_OPEN_LOGLEVEL);
#endif
char txp::State::d_log_buffer[txp::DEFAULT_LINE_LENGTH] = {'\0'};


//*****************************************************************************
//  Constructors
//*****************************************************************************
txp::State::State() {
    clearFlags();

#ifdef TXP_DEVELOPMENT
	snprintf(d_log_buffer, sizeof(d_log_buffer), "State::State()=%p", this);
	d_log.write(d_log_buffer, txp::Log::DEBUG, txp::Log::C_DTOR_LOGLEVEL_2);
#endif
}


txp::State::State(const uint16_t pFlags) {
    stateFlag.flags = pFlags;

#ifdef TXP_DEVELOPMENT
	snprintf(d_log_buffer, sizeof(d_log_buffer), "State::State(pFlags=%hu)=%p", pFlags, this);
	d_log.write(d_log_buffer, txp::Log::DEBUG, txp::Log::C_DTOR_LOGLEVEL_2);
#endif
}


//*****************************************************************************
//  Destructor
//*****************************************************************************
txp::State::~State() {

#ifdef TXP_DEVELOPMENT
	snprintf(d_log_buffer, sizeof(d_log_buffer), "State::~State()=%p", this);
	d_log.write(d_log_buffer, txp::Log::DEBUG, txp::Log::C_DTOR_LOGLEVEL_2);
#endif

    // TODO Auto-generated destructor stub
}


//*****************************************************************************
//  Static methods
//*****************************************************************************


//*****************************************************************************
//  Non-static methods
//*****************************************************************************
int txp::State::add(HeapBuffer* pBuffer) {
    int l_RC = 0;

    if (pBuffer->add((void*)&(stateFlag.flags), sizeof(stateFlag.flags))) {
        l_RC = -201;

#ifdef TXP_DEVELOPMENT
        snprintf(d_log_buffer, sizeof(d_log_buffer), "State::add(): l_RC=%d", l_RC);
        d_log.write(d_log_buffer, txp::Log::ERROR);
#endif
    }

    return l_RC;
}


void txp::State::dump(Log& pLog, const char* pPrefix) {
    START_PREFIX(pPrefix, ">>>>> Start dump for message state:  %s");

    strCpy(d_log_buffer, "State:", sizeof(d_log_buffer));
    pLog.write(d_log_buffer, txp::Log::DEBUG, txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL);

    pLog.incrIndent(2);

    snprintf(d_log_buffer, sizeof(d_log_buffer), "MsgAl: %c, HB_Al: %c, P_NAD: %c, C_CRC: %c, SndA : %c",
             (stateFlag.allocated ? '1' : '0'), (stateFlag.heapBufferAllocated ? '1' : '0'), (stateFlag.msgAttrExistsWithPtrToNonArrayData ? '1' : '0'), (stateFlag.msgCRC_IsCalculated ? '1' : '0'), (stateFlag.sendAllowed ? '1' : '0'));
    pLog.write(d_log_buffer, txp::Log::DEBUG, txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL);
    snprintf(d_log_buffer, sizeof(d_log_buffer), "HBCur: %c, IOCur: %c, Sent : %c, Resp : %c",
             (stateFlag.heapBufferIsCurrent ? '1' : '0'), (stateFlag.IO_VectorIsCurrent ? '1' : '0'), (stateFlag.sent ? '1' : '0'), (stateFlag.response ? '1' : '0'));
/*
    snprintf(d_log_buffer, sizeof(d_log_buffer), "Allocated:             %s", (stateFlag.allocated ? "True" : "False"));
    pLog.write(d_log_buffer, txp::Log::DEBUG, TXP_DEFAULT_DUMP_LOGLEVEL);
    snprintf(d_log_buffer, sizeof(d_log_buffer), "Heap Buffer Allocated: %s", (stateFlag.heapBufferAllocated ? "True" : "False"));
    pLog.write(d_log_buffer, txp::Log::DEBUG, TXP_DEFAULT_DUMP_LOGLEVEL);
    snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr to non-array:  %s", (stateFlag.msgAttrExistsWithPtrToNonArrayData ? "True" : "False"));
    pLog.write(d_log_buffer, txp::Log::DEBUG, TXP_DEFAULT_DUMP_LOGLEVEL);
    snprintf(d_log_buffer, sizeof(d_log_buffer), "Send Allowed:          %s", (stateFlag.sendAllowed ? "True" : "False"));
    pLog.write(d_log_buffer, txp::Log::DEBUG, TXP_DEFAULT_DUMP_LOGLEVEL);
    snprintf(d_log_buffer, sizeof(d_log_buffer), "Msg CRC Calculated:    %s", (stateFlag.msgCRC_IsCalculated ? "True" : "False"));
    pLog.write(d_log_buffer, txp::Log::DEBUG, TXP_DEFAULT_DUMP_LOGLEVEL);
    snprintf(d_log_buffer, sizeof(d_log_buffer), "Heap Buffer Current:   %s", (stateFlag.heapBufferIsCurrent ? "True" : "False"));
    pLog.write(d_log_buffer, txp::Log::DEBUG, TXP_DEFAULT_DUMP_LOGLEVEL);
    snprintf(d_log_buffer, sizeof(d_log_buffer), "I/O Vector Current:    %s", (stateFlag.IO_VectorIsCurrent ? "True" : "False"));
    pLog.write(d_log_buffer, txp::Log::DEBUG, TXP_DEFAULT_DUMP_LOGLEVEL);
    snprintf(d_log_buffer, sizeof(d_log_buffer), "Response:              %s", (stateFlag.response ? "True" : "False"));
    pLog.write(d_log_buffer, txp::Log::DEBUG, TXP_DEFAULT_DUMP_LOGLEVEL);
    snprintf(d_log_buffer, sizeof(d_log_buffer), "Sent:                  %s", (stateFlag.sent ? "True" : "False"));
*/
    pLog.write(d_log_buffer, txp::Log::DEBUG, txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL);
    pLog.decrIndent(2);

    END_PREFIX(pPrefix, ">>>>>   End dump for message state:  %s", 1);

    return;
}
