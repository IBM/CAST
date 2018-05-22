/*******************************************************************************
 |    Msg.cc
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

#include <limits.h>
#include <stdio.h>
#include <unistd.h>

#include "crc.h"
#include "Msg.h"

#if MSG_STALE_CHECK
#include <map>
#include <time.h>
typedef std::pair<txp::Msg, uint64_t> msgSecondsPair;
static std::map<txp::Msg*, uint64_t>  msgSeconds;
static pthread_mutex_t    msgSecondsMutex  = PTHREAD_MUTEX_INITIALIZER;
int addMsg2map(txp::Msg* pMsgPtr)
{
  int rc=0;
  pthread_mutex_lock(&msgSecondsMutex);
  if (msgSeconds.find (pMsgPtr) == msgSeconds.end() )
  {
    time_t when = time(NULL);
    msgSeconds[pMsgPtr]=when;
    LOG(txp,always) <<"addMsg2map@ epochsec="<<when<<" pointer pMsgPtr="<<pMsgPtr ;   
  }
  else abort(); //already exists!  abort!
  pthread_mutex_unlock(&msgSecondsMutex);
  return rc;
}

int removeMsgFrommap(txp::Msg* pMsgPtr)
{
  int rc=0;
  pthread_mutex_lock(&msgSecondsMutex);
  time_t then = (time_t)msgSeconds[pMsgPtr];
  LOG(txp,always) <<"removeMsgFrommap@  epochsec="<<then<<" pointer pMsgPtr="<<pMsgPtr<<" ctime="<<ctime(&then);
  rc = msgSeconds.erase(pMsgPtr);
  pthread_mutex_unlock(&msgSecondsMutex);
  return rc;
}

txp::Msg* checkForStaleMsg(uint64_t pDeclareStale){
   txp::Msg* l_Msg= NULL;
   pthread_mutex_lock(&msgSecondsMutex);
   uint64_t nowSeconds = time(NULL);
   int stalefound=0;
   char l_MsgIdStr[64] = {'\0'};
   for (auto iter : msgSeconds){
      if ( pDeclareStale <= (nowSeconds - iter.second) )
      {
         l_Msg=iter.first;
         stalefound++;
         txp::Msg::msgIdToChar(l_Msg->getMsgId(), l_MsgIdStr, sizeof(l_MsgIdStr));
         time_t then = (time_t)iter.second;
         LOG(txp,always) << "stale item#="<<stalefound<<" Msg is="<<l_MsgIdStr<<" Stale msg msgId="<<l_Msg->getMsgId()<<" msgNumber="<<l_Msg->getMsgNumber()
                         <<std::hex<<" Stale msg hex msgId="<<l_Msg->getMsgId()<<" hex msgNumber="<<l_Msg->getMsgNumber() <<std::dec<<" Msg created@ ctime="<<ctime(&then)<<" epochsec="<<then<<" pointer l_Msg="<<l_Msg;
      }
   }
   if (stalefound) abort();
   pthread_mutex_unlock(&msgSecondsMutex);
   return l_Msg;
}
#endif
//*****************************************************************************
//  Static data members
//*****************************************************************************
txp::Msg::MsgNumber MasterMsgNumber(0);
txp::CRC txp::Msg::DEFAULT_CALCULATE_CRC(txp::DO_NOT_CALCULATE_CRC);

#ifdef TXP_DEVELOPMENT
txp::Log txp::Msg::d_log(txp::Log::DEFAULT_LOG_DESTINATION, txp::Log::DEFAULT_OPEN_LOGSTATE, txp::Log::DEFAULT_OPEN_LOGLEVEL);
char txp::Msg::d_log_buffer[txp::DEFAULT_LINE_LENGTH] = {'\0'};
#endif

char txp::Msg::reserved_2[txp::LENGTH_OF_MSG_RESERVED_2] = {'\0'};

uint16_t txp::Msg::AttrType_CharValue = (uint16_t)txp::CHAR;
uint16_t txp::Msg::AttrType_UINT8_Value = (uint16_t)txp::UINT8;
uint16_t txp::Msg::AttrType_UINT16_Value = (uint16_t)txp::UINT16;
uint16_t txp::Msg::AttrType_UINT32_Value = (uint16_t)txp::UINT32;
uint16_t txp::Msg::AttrType_UINT64_Value = (uint16_t)txp::UINT64;
uint16_t txp::Msg::AttrType_INT8_Value = (uint16_t)txp::INT8;
uint16_t txp::Msg::AttrType_INT16_Value = (uint16_t)txp::INT16;
uint16_t txp::Msg::AttrType_INT32_Value = (uint16_t)txp::INT32;
uint16_t txp::Msg::AttrType_INT64_Value = (uint16_t)txp::INT64;
uint16_t txp::Msg::AttrType_CharArrayValue = (uint16_t)txp::CHAR_ARRAY;
uint16_t txp::Msg::AttrType_ArrayOfCharArraysValue = (uint16_t)txp::ARRAY_OF_CHAR_ARRAYS;

unsigned long txp::Msg::CRC_Value = 0;

int16_t txp::initPerformed = 0;


//*****************************************************************************
//  Non-Msg() related static methods
//*****************************************************************************
void txp::doInit() {
	txp::initPerformed = -1;

	if (curConfig.isLoaded()) {
		config = curConfig.getTree();
		// Retrieve the txp environmental variables housed in the configuration file
		std::string l_DftDefaultCalculateCRC = "DO_NOT_CALCULATE_CRC";
		std::string l_DefaultCalculateCRC = config.get("txp.calculate_crc", l_DftDefaultCalculateCRC);
		txp::Msg::setDefaultCalculateCRC(l_DefaultCalculateCRC);

		std::string l_DftDefaultLogDestination = "BOOST";
		std::string l_DefaultLogDestination = config.get("txp.log.destination", l_DftDefaultLogDestination);
		txp::Log::setDefaultLogDestination(l_DefaultLogDestination);

		std::string l_DefaultOpenLogState;
		std::string l_DftDefaultOpenLogState = "OPEN";
		if (txp::Log::DEFAULT_LOG_DESTINATION != txp::Log::NONE) {
			l_DefaultOpenLogState = config.get("txp.log.default_open_logstate", l_DftDefaultOpenLogState);
		} else {
			l_DefaultOpenLogState = "CLOSED";
		}
		txp::Log::setDefaultOpenLogState(l_DefaultOpenLogState);

		std::string l_DftDefaultOpenLogLevel = "NO_LOGLEVEL";
		std::string l_DefaultOpenLogLevel = config.get("txp.log.default_open_loglevel", l_DftDefaultOpenLogLevel);
		txp::Log::setDefaultOpenLogLevel(l_DefaultOpenLogLevel);

		std::string l_DftDefaultOpenDumpLogLevel = "NO_LOGLEVEL";
		std::string l_DefaultOpenDumpLogLevel = config.get("txp.log.default_open_dump_loglevel", l_DftDefaultOpenDumpLogLevel);
		txp::Log::setDefaultOpenDumpLogLevel(l_DefaultOpenDumpLogLevel);

   		FL_Write6(FLTxp, TXP_STARTUP, "Instance of txp: pid=%lld, uid=%lld gid=%lld ppid=%lld pgid=%lld sid=%lld", getpid(), getuid(), getgid(), getppid(), getpgid(getpid()), getsid(getpid()));

#ifdef TXP_DEVELOPMENT
		// Log other pertinent txp environmental values
		LOG(txp,always) << "Log destination   : " << l_DefaultLogDestination;
		LOG(txp,always) << "Dft Log Open State: " << l_DefaultOpenLogState;
		LOG(txp,always) << "Default Loglevel  : " << l_DefaultOpenLogLevel;
		LOG(txp,always) << "Dft Dump Loglevel : " << l_DefaultOpenDumpLogLevel;
		LOG(txp,always) << "Calc CRC values   : " << l_DefaultCalculateCRC;
#endif
		txp::initPerformed = 1;
    } else {
		LOG(txp,always) << "Configuration file was not already loaded";
    }

	return;
}


//*****************************************************************************
//  Msg() related static methods
//*****************************************************************************
void txp::Msg::calculateCRC_ToChar(const txp::CRC pValue, char* pBuffer, const int32_t pSize) {
	if (pSize) {
		pBuffer[0] = '\0';
		switch (pValue) {
			case txp::DO_NOT_CALCULATE_CRC:
				strCpy(pBuffer, "DO_NOT_CALCULATE_CRC", pSize);
				break;
			case txp::CALCULATE_CRC:
				strCpy(pBuffer, "CALCULATE_CRC", pSize);
				break;

			default:
				snprintf(pBuffer, pSize, "%s (%d)", "UNDEFINED", pValue);
		}
	}

	return;
};


void txp::Msg::setDefaultCalculateCRC(const std::string pValue) {
	if (pValue.compare("DO_NOT_CALCULATE_CRC") == 0)
		txp::Msg::DEFAULT_CALCULATE_CRC = txp::DO_NOT_CALCULATE_CRC;
	else if (pValue.compare("CALCULATE_CRC") == 0)
		txp::Msg::DEFAULT_CALCULATE_CRC = txp::CALCULATE_CRC;
	else {
		LOG(txp,always) << "Invalid value (" << pValue << ") specified for calculate_crc value.  Value set to DO_NOT_CALCULATE_CRC";
		txp::Msg::DEFAULT_CALCULATE_CRC = txp::DO_NOT_CALCULATE_CRC;
	}

	return;
};


//*****************************************************************************
//  Msg() Miscellaneous static methods
//*****************************************************************************
int txp::Msg::isDataLocationValid(const txp::DataLocation pDataLocation) {
	int l_RC = 1;

	switch (pDataLocation) {
		case txp::BUFFER_HAS_DATA:
		case txp::BUFFER_HAS_PTR_TO_DATA:

		default:
            l_RC = 0;
	}

#ifdef TXP_DEVELOPMENT
	if (!l_RC) {
		snprintf(d_log_buffer, sizeof(d_log_buffer), "Msg::isValid(DataLocation(%d)): RC=%d", pDataLocation, l_RC);
		txp::Msg::d_log.write(d_log_buffer, txp::Log::ERROR);
	}
#endif

	return l_RC;
}


int txp::Msg::isDeserializeOptionValid(const txp::DeserializeOption pOption) {
	int l_RC = 1;

	switch (pOption) {
		case txp::DO_NOT_COPY_DATA:
		case txp::COPY_DATA_TO_OBJECT:
		case txp::COPY_DATA_TO_HEAP:
		case txp::COPY_DATA_TO_OBJECT_BUT_DO_NOT_COPY_ARRAYS:

		default:
            l_RC = 0;
	}

#ifdef TXP_DEVELOPMENT
	if (!l_RC) {
		snprintf(d_log_buffer, sizeof(d_log_buffer), "Msg::isValid(DeserializeOption(%d)): RC=%d", pOption, l_RC);
		txp::Msg::d_log.write(d_log_buffer, txp::Log::ERROR);
	}
#endif

	return l_RC;
}


// NOTE:  Need to add support for version  @@DLH
int txp::Msg::isMsgIdSupported(const txp::Id pId) {
    int l_RC = 1;

    switch (pId) {
#include "gened_Msg_CC_Include_1.h"

        default:
            l_RC = 0;
    }

#ifdef TXP_DEVELOPMENT
	if (!l_RC) {
		snprintf(d_log_buffer, sizeof(d_log_buffer), "isSupported(pId(%d)): RC=%d", pId, l_RC);
		txp::Msg::d_log.write(d_log_buffer, txp::Log::WARN);
	}
#endif

    return l_RC;
}


int txp::Msg::isMsgIdSupported(const uint32_t pIdValue) {

    return txp::Msg::isMsgIdSupported((txp::Id)pIdValue);
}


int txp::Msg::isMsgIdValid(const txp::Id pId) {
    int l_RC = 1;

    switch (pId) {
#include "gened_Msg_CC_Include_2.h"

        default:
            l_RC = 0;
    }

#ifdef TXP_DEVELOPMENT
	if (!l_RC) {
		snprintf(d_log_buffer, sizeof(d_log_buffer), "isValid(pId(%d)): RC=%d", pId, l_RC);
		txp::Msg::d_log.write(d_log_buffer, txp::Log::WARN);
	}
#endif

    return l_RC;
}


int txp::Msg::isMsgIdValid(const uint32_t pIdValue) {

    return txp::Msg::isMsgIdValid((txp::Id)pIdValue);
}


void txp::Msg::msgIdToChar(const txp::Id pId, char* pBuffer, const int32_t pSize) {
	if (pSize) {
		pBuffer[0] = '\0';
		switch (pId) {
#include "gened_Msg_CC_Include_3.h"

			default:
				snprintf(pBuffer, pSize, "%s (%d)", "UNDEFINED", pId);
		}
	}

	return;
}


// CLASS:  MsgNumber

int32_t txp::Msg::MsgNumber::incr() {
	if (value != INT_MAX) {
		return ++value;
	} else {
		value = 1;
		return value;
	}
}


// CLASS:  Msg

//*****************************************************************************
//  Constructors
//*****************************************************************************
txp::Msg::Msg() :
    prevMsgNumber(0),
    nextMsgNumber(0),
    requestMsgNumber(0),
    msgRequestorPid(0),
    msgRequestorTid(0),
    msg_IO_VectorCount(0),
    msgId(Id()),
    msgState(State()),
    msgVersion(Version()),
    msgHeapBuffer(0),
    msg_IO_Vector(0),
    msgAttrMap(0),
    msgAttributes(0)
{
#if MSG_STALE_CHECK
 addMsg2map(this);
#endif
    msgNumber = incrMsgNumber();
    setMsgCreatorIds();
    msgLengthWithDataValues = OFFSET_TO_FIRST_ATTRIBUTE;
    msgLengthWithPtrValues = msgLengthWithDataValues;
    msgLengthWithDataValuesExceptForArrays = msgLengthWithDataValues;
    msgState.setSendAllowedFlag(1);
    init();

#ifdef TXP_DEVELOPMENT
	snprintf(d_log_buffer, sizeof(d_log_buffer), "Msg::Msg()=%p, msgNumber=%d", this, msgNumber);
	d_log.write(d_log_buffer, txp::Log::DBG2);
#endif
}


txp::Msg::Msg(const txp::Id pId) :
	prevMsgNumber(0),
	nextMsgNumber(0),
	requestMsgNumber(0),
    msgRequestorPid(0),
    msgRequestorTid(0),
    msg_IO_VectorCount(0),
	msgId(pId),
	msgState(State()),
	msgVersion(Version(pId)),
	msgHeapBuffer(0),
	msg_IO_Vector(0),
    msgAttrMap(0),
    msgAttributes(0)
{
#if MSG_STALE_CHECK
 addMsg2map(this);
#endif
	msgNumber = incrMsgNumber();
    setMsgCreatorIds();
	msgLengthWithDataValues = OFFSET_TO_FIRST_ATTRIBUTE;
	msgLengthWithPtrValues = msgLengthWithDataValues;
    msgLengthWithDataValuesExceptForArrays = msgLengthWithDataValues;
	msgState.setSendAllowedFlag(1);
    init();

#ifdef TXP_DEVELOPMENT
	char l_MsgId[64] = {'\0'};
	msgIdToChar(getMsgId(), l_MsgId, sizeof(l_MsgId));
	snprintf(d_log_buffer, sizeof(d_log_buffer), "Msg::Msg(pId=%s)=%p, msgNumber=%d", l_MsgId, this, msgNumber);
	d_log.write(d_log_buffer, txp::Log::DBG2);
#endif
}


txp::Msg::Msg(const txp::Id pId, Version pVersion) :
	prevMsgNumber(0),
	nextMsgNumber(0),
	requestMsgNumber(0),
    msgRequestorPid(0),
    msgRequestorTid(0),
    msg_IO_VectorCount(0),
	msgId(pId),
	msgState(State()),
	msgVersion(pVersion),
	msgHeapBuffer(0),
	msg_IO_Vector(0),
    msgAttrMap(0),
    msgAttributes(0)
{
#if MSG_STALE_CHECK
 addMsg2map(this);
#endif
	msgNumber = incrMsgNumber();
    setMsgCreatorIds();
	msgLengthWithDataValues = OFFSET_TO_FIRST_ATTRIBUTE;
	msgLengthWithPtrValues = msgLengthWithDataValues;
    msgLengthWithDataValuesExceptForArrays = msgLengthWithDataValues;
	msgState.setSendAllowedFlag(1);
    init();

#ifdef TXP_DEVELOPMENT
	char l_MsgId[64] = {'\0'};
	msgIdToChar(pId, l_MsgId, sizeof(l_MsgId));
	snprintf(d_log_buffer, sizeof(d_log_buffer), "Msg::Msg(pID=%s, pVersion(%d,%d))=%p, msgNumber=%d", l_MsgId, pVersion.getMajor(), pVersion.getMinor(), this, msgNumber);
	d_log.write(d_log_buffer, txp::Log::DBG2);
#endif
}


txp::Msg::Msg(HeapBuffer* pHeapBuffer) :
	msgNumber(pHeapBuffer->getMsgNumber()),
	prevMsgNumber(pHeapBuffer->getPrevMsgNumber()),
	nextMsgNumber(pHeapBuffer->getNextMsgNumber()),
	requestMsgNumber(pHeapBuffer->getRequestMsgNumber()),
	msgCreatorPid(pHeapBuffer->getMsgCreatorPid()),
	msgRequestorPid(pHeapBuffer->getMsgRequestorPid()),
	msgCreatorTid(pHeapBuffer->getMsgCreatorTid()),
	msgRequestorTid(pHeapBuffer->getMsgRequestorTid()),
    msg_IO_VectorCount(0),
	msgId(pHeapBuffer->getMsgId()),
	// NOTE:  We do not use the flags as serialized into the heap buffer to
	//        initialize the state flags in the new message.  This needs to
	//        be investigated further, but it is safer to not preserve those
	//        flags for now...  @@DLH
//	msgState(pHeapBuffer->getMsgState()),
	msgState(State()),
	msgVersion(pHeapBuffer->getMsgVersion()),
	msgHeapBuffer(0),
	msg_IO_Vector(0),
    msgAttrMap(0),
    msgAttributes(0)
{
#if MSG_STALE_CHECK
 addMsg2map(this);
#endif
	msgLengthWithDataValues = OFFSET_TO_FIRST_ATTRIBUTE;
	msgLengthWithPtrValues = msgLengthWithDataValues;
    msgLengthWithDataValuesExceptForArrays = msgLengthWithDataValues;
	msgState.setSendAllowedFlag(0);
    init();

#ifdef TXP_DEVELOPMENT
	char l_MsgId[64] = {'\0'};
	msgIdToChar(getMsgId(), l_MsgId, sizeof(l_MsgId));
	snprintf(d_log_buffer, sizeof(d_log_buffer), "Msg::Msg(pHeapBuffer=%p)=%p, %s, msgNumber=%d", pHeapBuffer, this, l_MsgId, msgNumber);
	d_log.write(d_log_buffer, txp::Log::DBG2);
#endif
}


txp::Msg::Msg(txp::Id pId, int32_t pRequestMsgNumber, txp::Version pVersion, pid_t pRequestorPid, pthread_t pRequestorTid) :
	prevMsgNumber(0),
	nextMsgNumber(0),
	requestMsgNumber(pRequestMsgNumber),
	msgRequestorPid(pRequestorPid),
	msgRequestorTid(pRequestorTid),
    msg_IO_VectorCount(0),
	msgId(pId),
	msgState(State()),
	msgVersion(pVersion),
	msgHeapBuffer(0),
	msg_IO_Vector(0),
    msgAttrMap(0),
    msgAttributes(0)
{
#if MSG_STALE_CHECK
 addMsg2map(this);
#endif
	msgNumber = incrMsgNumber();
    setMsgCreatorIds();
	msgLengthWithDataValues = OFFSET_TO_FIRST_ATTRIBUTE;
	msgLengthWithPtrValues = msgLengthWithDataValues;
    msgLengthWithDataValuesExceptForArrays = msgLengthWithDataValues;
	msgState.setSendAllowedFlag(1);
    init();

#ifdef TXP_DEVELOPMENT
	char l_MsgId[64] = {'\0'};
	msgIdToChar(pId, l_MsgId, sizeof(l_MsgId));
	snprintf(d_log_buffer, sizeof(d_log_buffer), "Msg::Msg(pID=%s, pRequestMsgNumber=%d, pVersion=%p)=%p, msgNumber=%d,  Version(%d,%d)", l_MsgId, pRequestMsgNumber, &pVersion, this, msgNumber, pVersion.getMajor(), pVersion.getMinor());
	d_log.write(d_log_buffer, txp::Log::DBG2);
#endif
}


// Copy constructor
txp::Msg::Msg(const Msg &pMsg) {
#if MSG_STALE_CHECK
 addMsg2map(this);
#endif
	msgNumber = incrMsgNumber();

	// NOTE:  If the source message is a continuation message
	//        or continues to another message, those continuations
	//        are NOT reflected in the copy operation.  Each message
	//        in the entire sequence of messages would have to
	//        copied and then Msg::nextMsg() used to 'link' them
	//        together.
	prevMsgNumber = 0;
	nextMsgNumber = 0;

	// Copy the request message number from the source message
	requestMsgNumber = pMsg.requestMsgNumber;

	// Set the creator pid/tid values in the target Msg()
    setMsgCreatorIds();

	// Set the requestor pid/tid values from the source Msg()
	msgRequestorPid = pMsg.msgRequestorPid;
	msgRequestorTid = pMsg.msgRequestorTid;

	// I/O vector is not copied...
	msg_IO_VectorCount = 0;

	// Copy the message length(s) in buffer from the source message
	msgLengthWithDataValues = pMsg.msgLengthWithDataValues;
	msgLengthWithPtrValues = pMsg.msgLengthWithPtrValues;
	msgLengthWithDataValuesExceptForArrays = pMsg.msgLengthWithDataValuesExceptForArrays;

	// Copy the message id from the source message
	msgId = pMsg.msgId;

	// NOTE:  All flags are initialized to off...
	//        Except, the sendAllowed flag is copied from the source message...
	msgState.clearFlags();
	if (pMsg.msgState.stateFlag.sendAllowed) {
		msgState.setSendAllowedFlag(1);
	}

	// Copy the version number from the source message
	msgVersion = pMsg.msgVersion;

	// NOTE:  The buffer for the message is NOT copied.
	//        It could be provided as an option later on, but for now,
	//        a new buffer must be allocated and populated with
	//        a 'Build' method.
	msgHeapBuffer=0;

	// NOTE:  The I/O vector is not copied and will be populated with
	//        the addAttribute() method below.
	msg_IO_Vector=0;

	// NOTE:  The attribute map is not copied and will be populated with
	//        the addAttribute() method below.
	msgAttrMap=0;

	// NOTE:  All attributes are copied as they are in the source message.
	//        addAttribute also populates the map of attributes.
	if (pMsg.msgAttributes) {
    	for(std::vector<Attribute*>::iterator it = pMsg.msgAttributes->begin(); it != pMsg.msgAttributes->end(); ++it) {
    		addAttribute(*it);
    	}
    } else {
  		msgAttributes=0;
    }

#ifdef TXP_DEVELOPMENT
	char l_MsgId[64] = {'\0'};
	msgIdToChar(getMsgId(), l_MsgId, sizeof(l_MsgId));
	snprintf(d_log_buffer, sizeof(d_log_buffer), "Msg::Msg(const Msg %p)=%p, %s, msgNumber=%d", &pMsg, this, l_MsgId, msgNumber);
	d_log.write(d_log_buffer, txp::Log::DEBUG, txp::Log::C_DTOR_LOGLEVEL);
#endif

	return;
}


//*****************************************************************************
//  Destructor
//*****************************************************************************
txp::Msg::~Msg() {
#ifdef TXP_DEVELOPMENT

	char l_MsgId[64] = {'\0'};
	msgIdToChar(getMsgId(), l_MsgId, sizeof(l_MsgId));
	d_log.write("===  Start dtor  ===============================================================", txp::Log::DEBUG, txp::Log::C_DTOR_LOGLEVEL);
	snprintf(d_log_buffer, sizeof(d_log_buffer), "Msg::~Msg()=%p, %s, msgNumber=%d, msgHeapBuffer=%p, msg_IO_Vector=%p, msgAttrMap=%p, msgAttributes=%p", this, l_MsgId, msgNumber, msgHeapBuffer, msg_IO_Vector, msgAttrMap, msgAttributes);
	d_log.write(d_log_buffer, txp::Log::DEBUG, txp::Log::C_DTOR_LOGLEVEL);
#endif
#if MSG_STALE_CHECK
        removeMsgFrommap(this);
#endif
	freeHeapBuffer();

	// NOTE:  Everything in msgAttrMap points to data in msgAttributes.
	//        Therefore, nothing specifically needs to be deleted related
	//        to delete related to the map.

	if (msg_IO_Vector) {
		struct iovec* l_Temp_IO_Vector = msg_IO_Vector;
		msg_IO_Vector = 0;
		delete l_Temp_IO_Vector;
	}

	if (msgAttrMap) {
		std::map<txp::AttributeName, txp::Attribute*>* l_TempAttrMap = msgAttrMap;
		msgAttrMap = 0;
		delete l_TempAttrMap;
	}

	if (msgAttributes) {
		Attribute* l_AttrPtr;
		while (!msgAttributes->empty()) {
			l_AttrPtr = msgAttributes->back();
			if (l_AttrPtr->isAllocated()) {
				l_AttrPtr->setAllocatedFlag(0);
				Attribute* l_TempAttr = l_AttrPtr;
				l_AttrPtr = 0;
				delete l_TempAttr;
			}
			else
			{
#ifdef TXP_DEVELOPMENT
			    l_AttrPtr->dump(d_log, "~Msg: Attribute not owned and not deleted");
#endif
			}
			msgAttributes->pop_back();
		}
		std::vector<Attribute*>* l_TempMsgAttrs = msgAttributes;
		msgAttributes = 0;
		delete l_TempMsgAttrs;
	}
#ifdef TXP_DEVELOPMENT
	d_log.write("===    End dtor  ===============================================================", txp::Log::DEBUG, txp::Log::C_DTOR_LOGLEVEL);
#endif
}


//*****************************************************************************
//  Static methods
//*****************************************************************************
int txp::Msg::buildMsg(const txp::Id pId, txp::Msg* &oMsg) {
	int l_RC = 0;

	oMsg = new txp::Msg(pId);

	if (oMsg) {
		oMsg->msgState.setAllocatedFlag(1);
	} else {
		l_RC = -3;
	}

#ifdef TXP_DEVELOPMENT
	if (l_RC) {
		char l_MsgId[64] = {'\0'};
		msgIdToChar(pId, l_MsgId, sizeof(l_MsgId));
		snprintf(d_log_buffer, sizeof(d_log_buffer), "Msg::buildMsg(%s): RC=%d", l_MsgId, l_RC);
		d_log.write(d_log_buffer, txp::Log::ERROR);
	}
#endif

	return l_RC;
}


int txp::Msg::buildMsg(const txp::Id pId, Version pVersion, txp::Msg* &oMsg) {
	int l_RC = 0;

	oMsg = new txp::Msg(pId, pVersion);

	if (oMsg) {
		oMsg->msgState.setAllocatedFlag(1);
	} else {
    	l_RC = -4;
	}

#ifdef TXP_DEVELOPMENT
	if (l_RC) {
		char l_MsgId[64] = {'\0'};
		msgIdToChar(pId, l_MsgId, sizeof(l_MsgId));
		snprintf(d_log_buffer, sizeof(d_log_buffer), "Msg::buildMsg(%s, Version(%hhu, %hhu)): RC=%d", l_MsgId, pVersion.getMajor(), pVersion.getMinor(), l_RC);
		d_log.write(d_log_buffer, txp::Log::ERROR);
	}
#endif

	return l_RC;
}


void txp::Msg::calculateMsgHeaderCRC_Value(unsigned long &pCRC) {
#ifdef TXP_DEVELOPMENT
//	txp::Log::dump_buffer_raw(d_log, getDataBufferPtr(), LENGTH_OF_MSG_HEADER, "Data buffer in calculateMsgHeaderCRC_Value()");
#endif
	getBufferPtr()->calculateCRC_Value(pCRC, 0, LENGTH_OF_MSG_HEADER);

	return;
}


int txp::Msg::deserializeHeaderToMsg(txp::Msg* &oMsg, char pBuffer[LENGTH_OF_MSG_HEADER], ssize_t &oTotalBufferSize, char* pDataBuffer, const size_t pDataBufferSize ) {
	int l_RC = 0;

	if (!oMsg) {
		HeapBuffer l_TempHeapBuffer(pBuffer, LENGTH_OF_MSG_HEADER);
    	l_RC = deserializeToMsg(oMsg, &l_TempHeapBuffer, txp::COPY_DATA_TO_OBJECT, LENGTH_OF_MSG_HEADER);
    	if (!l_RC) {
    		if (pDataBuffer) {
    			if (pDataBufferSize) {
    				if (pDataBufferSize >= LENGTH_OF_MSG_HEADER) {
    					l_RC = oMsg->allocateHeapBuffer(pDataBuffer, pDataBufferSize);
    				} else {
						l_RC = -32;
    				}
    			} else {
    				l_RC = oMsg->allocateHeapBuffer(pDataBuffer, l_TempHeapBuffer.getMsgLength());
    			}
    		} else {
				l_RC = oMsg->allocateHeapBuffer(l_TempHeapBuffer.getMsgLength());
			}
			if (!l_RC) {
				memcpy(oMsg->getDataBufferPtr(), pBuffer, LENGTH_OF_MSG_HEADER);
			}
		}

		// NOTE:  l_TempHeapBuffer will be deleted when going out of scope, but the pBuffer that
		//        it is 'built over' will NOT be deleted because l_TempHeapBuffer did NOT allocate
		//        the real data buffer (pBuffer).  @@DLH

		if (!l_RC) {
			oTotalBufferSize = l_TempHeapBuffer.getMsgLength();
		} else {
		    oTotalBufferSize = 0;
            if (oMsg) {
				delete oMsg;
				oMsg = 0;
            }
		}
	} else {
		l_RC = -31;
	}

#ifdef TXP_DEVELOPMENT
	if (l_RC) {
		snprintf(d_log_buffer, sizeof(d_log_buffer), "deserializeHeaderToMsg(%p,%p,%lu,%p,%lu): RC=%d", oMsg, pBuffer, oTotalBufferSize, pDataBuffer, pDataBufferSize, l_RC);
		d_log.write(d_log_buffer, txp::Log::ERROR);
	}
#endif

	return l_RC;
}


int txp::Msg::deserializeToMsg(txp::Msg* &oMsg, HeapBuffer* pHeapBuffer, const size_t pLength) {

	return deserializeToMsg(oMsg, pHeapBuffer, txp::DEFAULT_DESERIALIZE_OPTION, pLength);
}


// NOTE:  If pLength is 0, then the length to be deserialized into a Msg() will be determined from the
//        length found in the buffer (offset=8, for 4 bytes).  Otherwise, the length to be deserialized
//        will be the min(pLength, length found in buffer).
int txp::Msg::deserializeToMsg(txp::Msg* &oMsg, HeapBuffer* pHeapBuffer, const txp::DeserializeOption &pOption, const size_t pLength) {
	int l_RC = 0;

	size_t l_MsgLengthInBuffer, l_MsgLength;

#ifdef TXP_DEVELOPMENT
	pHeapBuffer->dump(d_log, "Buffer to deserialize");
#endif

    if (!oMsg) {
        if (pHeapBuffer->getMsgEyeCatcher() == EYECATCHER) {
            unsigned long l_MsgHeaderCRC = 0, l_MsgCRC = 0, l_CalcMsgCRC = 0;
            unsigned long* l_PtrCalcMsgCRC = 0;

            l_MsgLengthInBuffer = pHeapBuffer->getMsgLength() - pHeapBuffer->getLengthOfPad();

#ifdef TXP_DEVELOPMENT
            char l_Option[64] = {'\0'};
            deserializeOptionToChar(pOption, l_Option, sizeof(l_Option));
            snprintf(d_log_buffer, sizeof(d_log_buffer), "deserializeToMsg(%p,%p,%s,%zu): pHeapBuffer->getMsgNumber()=%d, l_MsgLengthInBuffer=%zu", oMsg, pHeapBuffer, l_Option, pLength, pHeapBuffer->getMsgNumber(), l_MsgLengthInBuffer);
            d_log.write(d_log_buffer, txp::Log::DEBUG);
#endif
            if (!l_RC) {
            	// Sanity check heap buffer size
                if (pHeapBuffer->getSize() >= LENGTH_OF_MSG_HEADER) {
                	// Sanity check size of message in buffer
                	if (l_MsgLengthInBuffer >= LENGTH_OF_MSG_HEADER) {
                    	l_MsgLength = std::min(l_MsgLengthInBuffer, pHeapBuffer->getSize());
                    	if (pLength) {
                    		// Sanity check specified length
                        	if (pLength >= LENGTH_OF_MSG_HEADER) {
                            	l_MsgLength = std::min(l_MsgLength, pLength);
                        	} else {
                            	l_RC = -9;
                        	}
                    	}
                	} else {
                    	l_RC = -56;
                	}
                } else {
                    l_RC = -55;
                }

                if (!l_RC) {
                    txp::State l_TempState = State(pHeapBuffer->getMsgState());
                    if (l_TempState.isMsgCRC_Calculated()) {
                        l_MsgHeaderCRC = pHeapBuffer->getAndZeroMsgHeaderCRC_Value();
                        l_MsgCRC = pHeapBuffer->getAndZeroMsgCRC_Value();
                        l_PtrCalcMsgCRC = &l_CalcMsgCRC;
//                      printf("deserializeToMsg: l_MsgHeaderCRC=0x%016lX, l_MsgCRC=0x%016lX\n", l_MsgHeaderCRC, l_MsgCRC);
                        if (!pHeapBuffer->checkMsgHeaderCRC_Value(l_MsgHeaderCRC)) {
                            l_RC = -1;
                        } else {
                            pHeapBuffer->setMsgHeaderCRC_Value(l_MsgHeaderCRC);
                            pHeapBuffer->setMsgCRC_Value(l_MsgCRC);
                            if (l_MsgLength > LENGTH_OF_MSG_HEADER && l_MsgLength < l_MsgLengthInBuffer) {
                                l_RC = -5;
                            }
                        }
                    }

					if (!l_RC) {
						oMsg = new txp::Msg(pHeapBuffer);
						if (oMsg && oMsg->isValid()) {
							if (l_TempState.isMsgCRC_Calculated()) {
								(oMsg->msgState).setMsgCRC_IsCalculated(1);
							}
							size_t l_Offset = OFFSET_TO_FIRST_ATTRIBUTE;
							txp::Attribute* l_Attr;
							size_t l_AttrLength;
							while (l_Offset<l_MsgLength && l_RC == 0) {
								l_Attr = 0;
								l_AttrLength = 0;
								l_RC = txp::Attribute::buildAttr(pHeapBuffer, l_Offset, l_Attr, l_AttrLength, pOption, l_PtrCalcMsgCRC);
								if (!l_RC) {
									l_RC = oMsg->addAttribute(l_Attr);
									l_Offset += l_AttrLength;
								}

		                    	// If the add failed, delete the attribute...
			                    if (l_RC && l_Attr)
			                    {
                                    delete l_Attr;
			                    }
							}

						} else {
							if (oMsg) {
								l_RC = -6;
							} else {
								l_RC = -7;
							}
						}

						if (!l_RC) {
							oMsg->msgState.setAllocatedFlag(1);
							if (l_TempState.isMsgCRC_Calculated() && l_MsgLength > LENGTH_OF_MSG_HEADER) {
//								printf("deserializeToMsg: l_MsgCRC=0x%016lX, l_CalcMsgCRC=0x%016lX\n", l_MsgCRC, l_CalcMsgCRC);
								if (l_MsgCRC != l_CalcMsgCRC) {
    	    						l_RC = -2;
								}
							}
						} else {
							if (oMsg) {
								txp::Msg* l_TempMsg = oMsg;
								oMsg = 0;
								delete l_TempMsg;
							}
						}
					}
				}
		    }
		} else {
    	    l_RC = -8;
		}
	} else {
    	l_RC = -33;
	}

#ifdef TXP_DEVELOPMENT
 	if (l_RC) {
   		snprintf(d_log_buffer, sizeof(d_log_buffer), "deserializeToMsg(): RC=%d, pHeapBuffer->getMsgNumber()=%d, pHeapBuffer->getSize()=%zu, pHeapBuffer->getOffset()=%zu", l_RC, pHeapBuffer->getMsgNumber(), pHeapBuffer->getSize(), pHeapBuffer->getOffset());
		d_log.write(d_log_buffer, txp::Log::ERROR);

//	    oMsg->dump(d_log, "Deserialized message");
	}
#endif

	return l_RC;
}


int txp::Msg::deserializeToMsg(txp::Msg* &oMsg, char* pBuffer, const size_t pBufferSize, ssize_t &oTotalMsgSize, const size_t pLength) {

    return deserializeToMsg(oMsg, pBuffer, pBufferSize, oTotalMsgSize, txp::DEFAULT_DESERIALIZE_OPTION, pLength);
}


int txp::Msg::deserializeToMsg(txp::Msg* &oMsg, char* pBuffer, const size_t pBufferSize, ssize_t &oTotalMsgSize, const txp::DeserializeOption &pOption, const size_t pLength) {
    int l_RC = 0;

    if (!oMsg) {
        if (pBuffer) {
            if (pBufferSize >= LENGTH_OF_MSG_HEADER) {
                if ((pLength == 0) || pLength <= pBufferSize) {
                    HeapBuffer* l_HeapBuffer = new HeapBuffer(pBuffer, pBufferSize);
                    l_RC = deserializeToMsg(oMsg, l_HeapBuffer, pOption, pLength);
                    if (!l_RC) {
                    	// NOTE: associateHeapBuffer() has the message facility take over ownership
                    	//       of the storage for the heap buffer.  Once assiciated, the heap buffer
                    	//       will be deleted when the message is deleted.  The data buffer is still
                    	//       the responsibility of the client code.
                        l_RC = oMsg->associateHeapBuffer(l_HeapBuffer);
                        if (!l_RC) {
                            oTotalMsgSize = oMsg->getHeapBufferPtr()->getMsgLength();
                        } else {
                        	// NOTE: If associateHeapBuffer() failed, the pointer to the heap buffer in the message is NULL.
                        	//       Delete the heap buffer here and the delete of the message below will NOT double free it.
                        	delete l_HeapBuffer;
                        	l_HeapBuffer = 0;
                        }
                    } else {
                        delete l_HeapBuffer;
                        l_HeapBuffer = 0;
                    }
                } else {
                    l_RC = -54;
                }
            } else {
#ifdef TXP_DEVELOPMENT
            	char l_Option[64] = {'\0'};
            	deserializeOptionToChar(pOption, l_Option, sizeof(l_Option));
            	snprintf(d_log_buffer, sizeof(d_log_buffer), "deserializeToMsg(%p,%p,%zu,%zu,%s,%zu): pBufferSize < LENGTH_OF_MSG_HEADER", oMsg, pBuffer, pBufferSize, oTotalMsgSize, l_Option, pLength);
            	d_log.write(d_log_buffer, txp::Log::DEBUG);
#endif
                l_RC = -53;
            }
        } else {
            l_RC = -52;
        }
    } else {
        l_RC = -51;
    }

    if (l_RC) {
        if (oMsg) {
            Msg* l_Temp = oMsg;
            oMsg = 0;
            delete l_Temp;
        }
    }

    return l_RC;
}


void txp::Msg::dataLocationToChar(const txp::DataLocation pDataLocation, char* pBuffer, const int32_t pSize ) {
	if (pSize) {
		pBuffer[0] = '\0';
		switch (pDataLocation) {
			case txp::DATA_LOCATION_INVALID:
				strCpy(pBuffer, "DATA_LOCATION_INVALID", pSize);
				break;
			case txp::BUFFER_HAS_DATA:
				strCpy(pBuffer, "BUFFER_HAS_DATA", pSize);
				break;
			case txp::BUFFER_HAS_PTR_TO_DATA:
				strCpy(pBuffer, "BUFFER_HAS_PTR_TO_DATA", pSize);
				break;

			default:
				snprintf(pBuffer, pSize, "%s (%d)", "UNDEFINED", pDataLocation);
		}
	}

	return;
}


void txp::Msg::deserializeOptionToChar(const txp::DeserializeOption pOption, char* pBuffer, const int32_t pSize ) {
	if (pSize) {
		pBuffer[0] = '\0';
		switch (pOption) {
			case txp::DESERIALIZE_OPTION_INVALID:
				strCpy(pBuffer, "BUILD_OPTION_INVALID", pSize);
				break;
			case txp::DO_NOT_COPY_DATA:
				strCpy(pBuffer, "DO_NOT_COPY_DATA", pSize);
				break;
			case txp::COPY_DATA_TO_OBJECT:
				strCpy(pBuffer, "COPY_DATA_TO_OBJECT", pSize);
				break;
			case txp::COPY_DATA_TO_HEAP:
				strCpy(pBuffer, "COPY_DATA_TO_HEAP", pSize);
				break;
			case txp::COPY_DATA_TO_OBJECT_BUT_DO_NOT_COPY_ARRAYS:
				strCpy(pBuffer, "COPY_DATA_TO_OBJECT_BUT_DO_NOT_COPY_ARRAYS", pSize);
				break;

			default:
				snprintf(pBuffer, pSize, "%s (%d)", "UNDEFINED", pOption);
		}
	}

	return;
}


int32_t txp::Msg::incrMsgNumber() {

	return MasterMsgNumber.incr();
};


int txp::Msg::isValid() {
	int l_RC = txp::Msg::isMsgIdValid(msgId);

	// NOTE:  Need to add to this validation.  This is called typically
	//        after we have deserialized a 'buffer' into a message.
	//        I think we want to return a unique RC, which is why we
	//        reassign after Id::isValid()...  @@DLH

#ifdef TXP_DEVELOPMENT
	if (!l_RC) {
		snprintf(d_log_buffer, sizeof(d_log_buffer), "Id::Msg::isValid(): RC=%d, msgNumber=%d", l_RC, msgNumber);
		txp::Msg::d_log.write(d_log_buffer, txp::Log::ERROR);
	}
#endif

	return l_RC;
}


//*****************************************************************************
//  Non-static methods
//*****************************************************************************
int txp::Msg::addAttribute(Attribute* pAttribute) {
	int l_RC = 0;

	txp::Attribute* l_AttrPtr = 0;
	//  If this is the first attribute to be added,
	//  get space for the vector and map...
	//  NOTE:  Either both pointers should be null or non-null.
	//         Should we verify that the two pointers are consistent.  @@DLH
	if (!msgAttrMap) {
		msgAttrMap = new std::map<txp::AttributeName, txp::Attribute*>;
	}

	if (msgAttrMap) {
		if (!msgAttributes) {
			msgAttributes = new std::vector<Attribute*>;
			msgAttributes->reserve(txp::MAXIMUM_NUMBER_OF_ATTRIBUTES);
		}

		if (msgAttributes) {
			// First, prevent more than the maximum number of attributes from being added...
			if (msgAttributes->size() < txp::MAXIMUM_NUMBER_OF_ATTRIBUTES) {
				// Second, prevent the attribute from being added a second time...
				txp::AttributeName l_AttrName = pAttribute->getAttrName();
				l_AttrPtr = retrieveAttr(l_AttrName);
				if (!l_AttrPtr) {
					// Next, if the client created the attribute, make a copy of it.
					// Insert the attribute into the vector...
					// NOTE:  If the client created the attribute we copy it so they
					//        can free the storage for that attribute.  However, for
					//        those attributes with char arrays, storage for those
					//        arrays cannot be freed until all processing of the message
					//        is complete.  The char arrays are NOT copied, but referenced
					//        by the copied attribute.
					// NOTE:  Similar to above, if the attribute has already been added to
					//        a message, the attribute will be cloned first before adding the
					//        cloned attribute to the second message.  In the case where
					//        an attribute is owned by the message facility, cloning the
					//        message first prevents a double free for the attribute
					//        when the message destructors are run.
					if (pAttribute->isAllocated() && (!pAttribute->isAddedToMsg())) {
						l_AttrPtr = pAttribute;
					} else {
						l_RC = pAttribute->clone(l_AttrPtr);
					}

					if (!l_RC) {
						msgAttributes->push_back(l_AttrPtr);

	 					// None of the containers keeping attribute data is now current...
	    				msgState.setHeapBufferIsCurrentFlag(0);
	    				msgState.setIO_VectorIsCurrentFlag(0);

						// NOTE:  We calculate three message buffer lengths:
						//        1) One is the message length if all data values are copied to the buffer.
						//        2) The second is the length if those attributes that use pointer indirection
						//           copy those pointers instead of the actual data value.
						//        3) The third is the same as the second, except for any non-array attributes
						//           that use pointer indirection.  The lengths for those non-array attributes
						//           are calculated assuming that the data value will be copied to the buffer.
						//        @@DLH
						msgLengthWithDataValues += l_AttrPtr->getLengthOfValueInBuffer();
						msgLengthWithPtrValues += l_AttrPtr->getLengthInBuffer();
						if (l_AttrPtr->isPtrToArrayAttrType()) {
							msgLengthWithDataValuesExceptForArrays += l_AttrPtr->getLengthInBuffer();
						} else {
							msgLengthWithDataValuesExceptForArrays += l_AttrPtr->getLengthOfValueInBuffer();
						}

						// Finally, insert the attribute in the map...
						std::pair<std::map<txp::AttributeName, Attribute*>::iterator,bool> l_Return;
						l_Return = msgAttrMap->insert(std::make_pair(l_AttrPtr->getAttrName(), l_AttrPtr));
						if (!l_Return.second) {
							l_RC = -12;
#ifdef TXP_DEVELOPMENT
							snprintf(d_log_buffer, sizeof(d_log_buffer), "Msg::addAttribute(l_AttrPtr=%p): AttributeName=%d, RC=%d, msgLengthWithDataValues=%zu, msgLengthWithPtrValues=%zu, msgLengthWithDataValuesExceptForArrays=%zu", l_AttrPtr, l_AttrPtr->getAttrName(), l_RC, msgLengthWithDataValues, msgLengthWithPtrValues, msgLengthWithDataValuesExceptForArrays);
							txp::Msg::d_log.write(d_log_buffer, txp::Log::ERROR);
#endif
						}

						// Indicate this attribute has been added to a message
						l_AttrPtr->setAddedToMsg(1);
					} else {
						l_RC = -45;
					}
				} else {
					l_RC = -30;
#ifdef TXP_DEVELOPMENT
					char l_AttrName[64] = {'\0'};
					txp::Attribute::attrNameToChar(l_AttrPtr->getAttrName(), l_AttrName, sizeof(l_AttrName));
					char l_AttrType[64] = {'\0'};
					txp::Attribute::attrTypeToChar(l_AttrPtr->getAttrType(), l_AttrType, sizeof(l_AttrType));
					snprintf(d_log_buffer, sizeof(d_log_buffer), "Msg::addAttribute(l_AttrPtr=%p): AttributeName=%d, RC=%d, msg with attribute name of %s already exsits. Existing attr at %p, type=%s, length of value in buffer=%zu", l_AttrPtr, l_AttrPtr->getAttrName(), l_RC, l_AttrName, l_AttrPtr, l_AttrType, l_AttrPtr->getLengthOfValueInBuffer());
					txp::Msg::d_log.write(d_log_buffer, txp::Log::ERROR);
#endif
		    	}
		    } else {
				l_RC = -44;
#ifdef TXP_DEVELOPMENT
				snprintf(d_log_buffer, sizeof(d_log_buffer), "Msg::addAttribute(l_AttrPtr=%p): AttributeName=%d, RC=%d, maximum number of attributes (%zu) already added", l_AttrPtr, l_AttrPtr->getAttrName(), l_RC, txp::MAXIMUM_NUMBER_OF_ATTRIBUTES);
				txp::Msg::d_log.write(d_log_buffer, txp::Log::ERROR);
#endif
		    }
		} else {
			l_RC = -34;
#ifdef TXP_DEVELOPMENT
			snprintf(d_log_buffer, sizeof(d_log_buffer), "Msg::addAttribute(l_AttrPtr=%p): AttributeName=%d, RC=%d, msg attributes vector could not be created", l_AttrPtr, l_AttrPtr->getAttrName(), l_RC);
			txp::Msg::d_log.write(d_log_buffer, txp::Log::ERROR);
#endif
		}
	} else {
		l_RC = -35;
#ifdef TXP_DEVELOPMENT
		snprintf(d_log_buffer, sizeof(d_log_buffer), "Msg::addAttribute(l_AttrPtr=%p): AttributeName=%d, RC=%d, msg attribute map could not be created", l_AttrPtr, l_AttrPtr->getAttrName(), l_RC);
		txp::Msg::d_log.write(d_log_buffer, txp::Log::ERROR);
#endif
	}

#ifdef TXP_DEVELOPMENT
	if (!l_RC) {
		snprintf(d_log_buffer, sizeof(d_log_buffer), "Attribute %p number %lu added to msg %p", l_AttrPtr, msgAttributes->size(), this);
		l_AttrPtr->dump(d_log, d_log_buffer);
	}
#endif

	return l_RC;
}

int txp::Msg::addAttribute(const txp::AttributeName pName, const char& pData) {
	int l_RC = 0;

	Attr_char* l_Attr = 0;
	l_RC = txp::Attr_char::buildAttr_char(pName, pData, l_Attr);
	if (!l_RC) {
		l_Attr->setAllocatedFlag(1);
		l_RC = this->addAttribute(l_Attr);
	}

	// If the add failed, delete the attribute...
	if (l_RC && l_Attr)
	{
        delete l_Attr;
	}

	return l_RC;
}


int txp::Msg::addAttribute(const txp::AttributeName pName, const uint8_t& pData) {
	int l_RC = 0;

	Attr_uint8* l_Attr = 0;
	l_RC = txp::Attr_uint8::buildAttr_uint8(pName, pData, l_Attr);
	if (!l_RC) {
		l_Attr->setAllocatedFlag(1);
		l_RC = this->addAttribute(l_Attr);
	}

	// If the add failed, delete the attribute...
	if (l_RC && l_Attr)
	{
        delete l_Attr;
	}

	return l_RC;
}


int txp::Msg::addAttribute(const txp::AttributeName pName, const uint16_t& pData) {
	int l_RC = 0;

	Attr_uint16* l_Attr = 0;
	l_RC = txp::Attr_uint16::buildAttr_uint16(pName, pData, l_Attr);
	if (!l_RC) {
		l_Attr->setAllocatedFlag(1);
		l_RC = this->addAttribute(l_Attr);
	}

	// If the add failed, delete the attribute...
	if (l_RC && l_Attr)
	{
        delete l_Attr;
	}

	return l_RC;
}


int txp::Msg::addAttribute(const txp::AttributeName pName, const uint32_t& pData) {
	int l_RC = 0;

	Attr_uint32* l_Attr = 0;
	l_RC = txp::Attr_uint32::buildAttr_uint32(pName, pData, l_Attr);
	if (!l_RC) {
		l_Attr->setAllocatedFlag(1);
		l_RC = this->addAttribute(l_Attr);
	}

	// If the add failed, delete the attribute...
	if (l_RC && l_Attr)
	{
        delete l_Attr;
	}

	return l_RC;
}


int txp::Msg::addAttribute(const txp::AttributeName pName, const uint64_t& pData) {
	int l_RC = 0;

	Attr_uint64* l_Attr = 0;
	l_RC = txp::Attr_uint64::buildAttr_uint64(pName, pData, l_Attr);
	if (!l_RC) {
		l_Attr->setAllocatedFlag(1);
		l_RC = this->addAttribute(l_Attr);
	}

	// If the add failed, delete the attribute...
	if (l_RC && l_Attr)
	{
        delete l_Attr;
	}

	return l_RC;
}


int txp::Msg::addAttribute(const txp::AttributeName pName, const int8_t& pData) {
	int l_RC = 0;

	Attr_int8* l_Attr = 0;
	l_RC = txp::Attr_int8::buildAttr_int8(pName, pData, l_Attr);
	if (!l_RC) {
		l_Attr->setAllocatedFlag(1);
		l_RC = this->addAttribute(l_Attr);
	}

	// If the add failed, delete the attribute...
	if (l_RC && l_Attr)
	{
        delete l_Attr;
	}

	return l_RC;
}


int txp::Msg::addAttribute(const txp::AttributeName pName, const int16_t& pData) {
	int l_RC = 0;

	Attr_int16* l_Attr = 0;
	l_RC = txp::Attr_int16::buildAttr_int16(pName, pData, l_Attr);
	if (!l_RC) {
		l_Attr->setAllocatedFlag(1);
		l_RC = this->addAttribute(l_Attr);
	}

	// If the add failed, delete the attribute...
	if (l_RC && l_Attr)
	{
        delete l_Attr;
	}

	return l_RC;
}


int txp::Msg::addAttribute(const txp::AttributeName pName, const int32_t& pData) {
	int l_RC = 0;

	Attr_int32* l_Attr = 0;
	l_RC = txp::Attr_int32::buildAttr_int32(pName, pData, l_Attr);
	if (!l_RC) {
		l_Attr->setAllocatedFlag(1);
		l_RC = this->addAttribute(l_Attr);
	}

	// If the add failed, delete the attribute...
	if (l_RC && l_Attr)
	{
        delete l_Attr;
	}

	return l_RC;
}


int txp::Msg::addAttribute(const txp::AttributeName pName, const int64_t& pData) {
	int l_RC = 0;

	Attr_int64* l_Attr = 0;
	l_RC = txp::Attr_int64::buildAttr_int64(pName, pData, l_Attr);
	if (!l_RC) {
		l_Attr->setAllocatedFlag(1);
		l_RC = this->addAttribute(l_Attr);
	}

	// If the add failed, delete the attribute...
	if (l_RC && l_Attr)
	{
        delete l_Attr;
	}

	return l_RC;
}


int txp::Msg::addAttribute(const txp::AttributeName pName, const char* pData) {
	int l_RC = 0;

	AttrPtr_char* l_Attr = 0;
	l_RC = txp::AttrPtr_char::buildAttrPtr_char(pName, pData, l_Attr);
	if (!l_RC) {
		l_Attr->setAllocatedFlag(1);
		l_RC = this->addAttribute(l_Attr);
		if (!l_RC) {
			msgState.setAttrExistsWithPtrToNonArrayData(1);
		}

	}

	// If the add failed, delete the attribute...
	if (l_RC && l_Attr)
	{
        delete l_Attr;
	}

	return l_RC;
}


int txp::Msg::addAttribute(const txp::AttributeName pName, const uint8_t* pData) {
	int l_RC = 0;

	AttrPtr_uint8* l_Attr = 0;
	l_RC = txp::AttrPtr_uint8::buildAttrPtr_uint8(pName, pData, l_Attr);
	if (!l_RC) {
		l_Attr->setAllocatedFlag(1);
		l_RC = this->addAttribute(l_Attr);
		if (!l_RC) {
			msgState.setAttrExistsWithPtrToNonArrayData(1);
		}
	}

	// If the add failed, delete the attribute...
	if (l_RC && l_Attr)
	{
        delete l_Attr;
	}

	return l_RC;
}


int txp::Msg::addAttribute(const txp::AttributeName pName, const uint16_t* pData) {
	int l_RC = 0;

	AttrPtr_uint16* l_Attr = 0;
	l_RC = txp::AttrPtr_uint16::buildAttrPtr_uint16(pName, pData, l_Attr);
	if (!l_RC) {
		l_Attr->setAllocatedFlag(1);
		l_RC = this->addAttribute(l_Attr);
		if (!l_RC) {
			msgState.setAttrExistsWithPtrToNonArrayData(1);
		}
	}

	// If the add failed, delete the attribute...
	if (l_RC && l_Attr)
	{
        delete l_Attr;
	}

	return l_RC;
}


int txp::Msg::addAttribute(const txp::AttributeName pName, const uint32_t* pData) {
	int l_RC = 0;

	AttrPtr_uint32* l_Attr = 0;
	l_RC = txp::AttrPtr_uint32::buildAttrPtr_uint32(pName, pData, l_Attr);
	if (!l_RC) {
		l_Attr->setAllocatedFlag(1);
		l_RC = this->addAttribute(l_Attr);
		if (!l_RC) {
			msgState.setAttrExistsWithPtrToNonArrayData(1);
		}
	}

	// If the add failed, delete the attribute...
	if (l_RC && l_Attr)
	{
        delete l_Attr;
	}

	return l_RC;
}


int txp::Msg::addAttribute(const txp::AttributeName pName, const uint64_t* pData) {
	int l_RC = 0;

	AttrPtr_uint64* l_Attr = 0;
	l_RC = txp::AttrPtr_uint64::buildAttrPtr_uint64(pName, pData, l_Attr);
	if (!l_RC) {
		l_Attr->setAllocatedFlag(1);
		l_RC = this->addAttribute(l_Attr);
		if (!l_RC) {
			msgState.setAttrExistsWithPtrToNonArrayData(1);
		}
	}

	// If the add failed, delete the attribute...
	if (l_RC && l_Attr)
	{
        delete l_Attr;
	}

	return l_RC;
}


int txp::Msg::addAttribute(const txp::AttributeName pName, const int8_t* pData) {
	int l_RC = 0;

	AttrPtr_int8* l_Attr = 0;
	l_RC = txp::AttrPtr_int8::buildAttrPtr_int8(pName, pData, l_Attr);
	if (!l_RC) {
		l_Attr->setAllocatedFlag(1);
		l_RC = this->addAttribute(l_Attr);
		if (!l_RC) {
			msgState.setAttrExistsWithPtrToNonArrayData(1);
		}
	}

	// If the add failed, delete the attribute...
	if (l_RC && l_Attr)
	{
        delete l_Attr;
	}

	return l_RC;
}


int txp::Msg::addAttribute(const txp::AttributeName pName, const int16_t* pData) {
	int l_RC = 0;

	AttrPtr_int16* l_Attr = 0;
	l_RC = txp::AttrPtr_int16::buildAttrPtr_int16(pName, pData, l_Attr);
	if (!l_RC) {
		l_Attr->setAllocatedFlag(1);
		l_RC = this->addAttribute(l_Attr);
		if (!l_RC) {
			msgState.setAttrExistsWithPtrToNonArrayData(1);
		}
	}

	// If the add failed, delete the attribute...
	if (l_RC && l_Attr)
	{
        delete l_Attr;
	}

	return l_RC;
}


int txp::Msg::addAttribute(const txp::AttributeName pName, const int32_t* pData) {
	int l_RC = 0;

	AttrPtr_int32* l_Attr = 0;
	l_RC = txp::AttrPtr_int32::buildAttrPtr_int32(pName, pData, l_Attr);
	if (!l_RC) {
		l_Attr->setAllocatedFlag(1);
		l_RC = this->addAttribute(l_Attr);
		if (!l_RC) {
			msgState.setAttrExistsWithPtrToNonArrayData(1);
		}
	}

	// If the add failed, delete the attribute...
	if (l_RC && l_Attr)
	{
        delete l_Attr;
	}

	return l_RC;
}


int txp::Msg::addAttribute(const txp::AttributeName pName, const int64_t* pData) {
	int l_RC = 0;

	AttrPtr_int64* l_Attr = 0;
	l_RC = txp::AttrPtr_int64::buildAttrPtr_int64(pName, pData, l_Attr);
	if (!l_RC) {
		l_Attr->setAllocatedFlag(1);
		l_RC = this->addAttribute(l_Attr);
		if (!l_RC) {
			msgState.setAttrExistsWithPtrToNonArrayData(1);
		}
	}

	// If the add failed, delete the attribute...
	if (l_RC && l_Attr)
	{
        delete l_Attr;
	}

	return l_RC;
}


int txp::Msg::addAttribute(const txp::AttributeName pName, const char* pData, const size_t pLength, const txp::COPY_DATA_OPTION pCopyDataOption) {
	int l_RC = 0;

	AttrPtr_char_array* l_Attr = 0;
	l_RC = txp::AttrPtr_char_array::buildAttrPtr_char_array(pName, pData, pLength, l_Attr, pCopyDataOption);
	if (!l_RC) {
		l_Attr->setAllocatedFlag(1);
		l_RC = this->addAttribute(l_Attr);
	}

	// If the add failed, delete the attribute...
	if (l_RC && l_Attr)
	{
        delete l_Attr;
	}

	return l_RC;
}


int txp::Msg::addAttribute(const txp::AttributeName pName, const txp::CharArray* pData) {
	int l_RC = 0;

	AttrPtr_array_of_char_arrays* l_Attr = 0;
	l_RC = txp::AttrPtr_array_of_char_arrays::buildAttrPtr_array_of_char_arrays(pName, pData, l_Attr);
	if (!l_RC) {
		l_Attr->setAllocatedFlag(1);
		l_RC = this->addAttribute(l_Attr);
	}

	// If the add failed, delete the attribute...
	if (l_RC && l_Attr)
	{
        delete l_Attr;
	}

	return l_RC;
}


int txp::Msg::allocateHeapBuffer(const size_t pSize) {
	int l_RC = 0;

	size_t l_Size = pSize;
	if (!l_Size) {
		l_Size = txp::DEFAULT_BUFFER_SIZE;
	} else {
		if (l_Size < LENGTH_OF_MSG_HEADER) {
			l_RC = -60;
		}
	}

	if (!l_RC) {
    	freeHeapBuffer();

	    //NOTE:  The 'data allocated' flag is turned on by the constructor and
	    //       this heap buffer and associated data buffer will be deleted when
	    //       the Msg() object is destroyed.
	    msgHeapBuffer = new HeapBuffer(l_Size);
	    msgState.setHeapBufferAllocatedFlag(1);

	    if (msgHeapBuffer) {
#ifdef TXP_DEVELOPMENT
		    char l_MsgId[64] = {'\0'};
		    msgIdToChar(getMsgId(), l_MsgId, sizeof(l_MsgId));
		    snprintf(d_log_buffer, sizeof(d_log_buffer), "Msg(%p) HB(%p), DB(%p), %s, Msg#=%d, #Attrs=%lu, RC=%d", this, msgHeapBuffer, msgHeapBuffer->getDataBufferPtr(), l_MsgId, msgNumber, (msgAttrMap ? msgAttrMap->size() : 0), l_RC);
		    d_log.write(d_log_buffer, txp::Log::DEBUG);
#endif
	    } else {
	    	l_RC = -38;
#ifdef TXP_DEVELOPMENT
		    char l_MsgId[64] = {'\0'};
		    msgIdToChar(getMsgId(), l_MsgId, sizeof(l_MsgId));
		    snprintf(d_log_buffer, sizeof(d_log_buffer), "allocateHeapBuffer(pSize=%zu), RC=%d, Msg=%p, %s, could not create heap buffer", pSize, l_RC, this, l_MsgId);
		    d_log.write(d_log_buffer, txp::Log::DEBUG);
#endif
	    }
    }

	return l_RC;
}


int txp::Msg::allocateHeapBuffer(char* pDataBuffer, const size_t pDataBufferSize) {
	int l_RC = 0;

	if (pDataBufferSize < LENGTH_OF_MSG_HEADER) {
		l_RC = -61;
	} else {

	    freeHeapBuffer();

	    msgHeapBuffer = new HeapBuffer(pDataBuffer, pDataBufferSize);
	    msgState.setHeapBufferAllocatedFlag(1);

	    if (msgHeapBuffer) {
#ifdef TXP_DEVELOPMENT
		    char l_MsgId[64] = {'\0'};
		    msgIdToChar(getMsgId(), l_MsgId, sizeof(l_MsgId));
		    snprintf(d_log_buffer, sizeof(d_log_buffer), "Msg(%p) associated with heap buffer(%p), RC=%d, %s, %lu attributes, existing data buffer=%p", this, msgHeapBuffer, l_RC, l_MsgId, (msgAttrMap ? msgAttrMap->size() : 0), msgHeapBuffer->getDataBufferPtr());
		    d_log.write(d_log_buffer, txp::Log::DEBUG);
#endif
	    } else {
	    	l_RC = -39;
#ifdef TXP_DEVELOPMENT
		    char l_MsgId[64] = {'\0'};
		    msgIdToChar(getMsgId(), l_MsgId, sizeof(l_MsgId));
		    snprintf(d_log_buffer, sizeof(d_log_buffer), "allocateHeapBuffer(pDataBuffer=%p, pDataBufferSize=%zu), RC=%d, Msg=%p, %s, could not create heap buffer", pDataBuffer, pDataBufferSize, l_RC, this, l_MsgId);
		    d_log.write(d_log_buffer, txp::Log::DEBUG);
#endif
	    }
	}

	return l_RC;
}


int txp::Msg::associateHeapBuffer(HeapBuffer* pHeapBuffer) {
	int l_RC = 0;

	if (pHeapBuffer) {
		freeHeapBuffer();

		msgHeapBuffer = pHeapBuffer;
		// NOTE: The message facility takes over the ownership of the heap buffer.
		//       On the delete of the message, the heap buffer will also be deleted.
		msgState.setHeapBufferAllocatedFlag(1);

#ifdef TXP_DEVELOPMENT
		char l_MsgId[64] = {'\0'};
		msgIdToChar(getMsgId(), l_MsgId, sizeof(l_MsgId));
		snprintf(d_log_buffer, sizeof(d_log_buffer), "Msg(%p) associated with heap buffer(%p), RC=%d, %s, %lu attributes, existing data buffer=%p", this, msgHeapBuffer, l_RC, l_MsgId, (msgAttrMap ? msgAttrMap->size() : 0), msgHeapBuffer->getDataBufferPtr());
		d_log.write(d_log_buffer, txp::Log::DEBUG);
#endif
	} else {
		l_RC = -57;
#ifdef TXP_DEVELOPMENT
		char l_MsgId[64] = {'\0'};
		msgIdToChar(getMsgId(), l_MsgId, sizeof(l_MsgId));
		snprintf(d_log_buffer, sizeof(d_log_buffer), "associateHeapBuffer(pHeapBuffer=%p), RC=%d, Msg=%p, %s, null pointer passed for heap buffer", pHeapBuffer, l_RC, this, l_MsgId);
		d_log.write(d_log_buffer, txp::Log::DEBUG);
#endif
	}

	return l_RC;
}


int txp::Msg::build_IO_Vector() {
	int l_RC = 0;

	if (msgHeapBuffer && msgState.isHeapBufferCurrent()) {

		if (msg_IO_Vector) {
			struct iovec* l_Temp_IO_Vector = msg_IO_Vector;
			msg_IO_VectorCount = 0;
			msg_IO_Vector = 0;
			delete l_Temp_IO_Vector;
		}

		msg_IO_Vector = new struct iovec[txp::MAXIMUM_NUMBER_OF_ATTRIBUTES];

		if (msg_IO_Vector) {
#ifdef TXP_DEVELOPMENT
			dump(d_log, "Msg, before building the I/O vectorAttribute");
#endif
    		// Build the I/O vector
    		int32_t l_NumberOfVectorElements = 0;
    		if (msgAttributes) {
    			for(std::vector<Attribute*>::iterator it = msgAttributes->begin(); it != msgAttributes->end(); ++it) {
#ifdef TXP_DEVELOPMENT
					(*it)->dump(d_log, "Attribute");
#endif
    				if (l_NumberOfVectorElements==0) {
    					// Add the message header
    	 				msg_IO_Vector[l_NumberOfVectorElements].iov_base = getDataBufferPtr();
    	 				msg_IO_Vector[l_NumberOfVectorElements].iov_len = txp::OFFSET_TO_FIRST_ATTRIBUTE;
#ifdef TXP_DEVELOPMENT
   						snprintf(d_log_buffer, sizeof(d_log_buffer), "Element %d: iov_base=%p, iov_len=%lu", l_NumberOfVectorElements, msg_IO_Vector[l_NumberOfVectorElements].iov_base, msg_IO_Vector[l_NumberOfVectorElements].iov_len);
						d_log.write(d_log_buffer, txp::Log::DBG2);
  	            		txp::Log::dump_buffer_raw(d_log, (const char*)msg_IO_Vector[l_NumberOfVectorElements].iov_base, msg_IO_Vector[l_NumberOfVectorElements].iov_len, "Msg Header");
#endif
    	 				++l_NumberOfVectorElements;
    	 			}

    	 			if ((*it)->isPtrAttrType()) {
						// Add the type for this attribute
						// NOTE:  We must convert from PTR type to non-PTR type...
						switch ((*it)->getAttrType()) {
							case txp::PTR_CHAR:
								msg_IO_Vector[l_NumberOfVectorElements].iov_base = &AttrType_CharArrayValue;
								break;
            		        case txp::PTR_UINT8:
								msg_IO_Vector[l_NumberOfVectorElements].iov_base = &AttrType_UINT8_Value;
								break;
            		        case txp::PTR_UINT16:
								msg_IO_Vector[l_NumberOfVectorElements].iov_base = &AttrType_UINT16_Value;
								break;
            		        case txp::PTR_UINT32:
								msg_IO_Vector[l_NumberOfVectorElements].iov_base = &AttrType_UINT32_Value;
								break;
            		        case txp::PTR_UINT64:
								msg_IO_Vector[l_NumberOfVectorElements].iov_base = &AttrType_UINT64_Value;
								break;
            		        case txp::PTR_INT8:
								msg_IO_Vector[l_NumberOfVectorElements].iov_base = &AttrType_INT8_Value;
								break;
            		        case txp::PTR_INT16:
								msg_IO_Vector[l_NumberOfVectorElements].iov_base = &AttrType_INT16_Value;
								break;
            		        case txp::PTR_INT32:
								msg_IO_Vector[l_NumberOfVectorElements].iov_base = &AttrType_INT32_Value;
								break;
            		        case txp::PTR_INT64:
								msg_IO_Vector[l_NumberOfVectorElements].iov_base = &AttrType_INT64_Value;
								break;
            		        case txp::PTR_CHAR_ARRAY:
								msg_IO_Vector[l_NumberOfVectorElements].iov_base = &AttrType_CharArrayValue;
								break;
            		        case txp::PTR_ARRAY_OF_CHAR_ARRAYS:
								msg_IO_Vector[l_NumberOfVectorElements].iov_base = &AttrType_ArrayOfCharArraysValue;
								break;

							default:
								// Never should get here...  @@DLH
								l_RC = -13;
						}
					} else {
    					msg_IO_Vector[l_NumberOfVectorElements].iov_base = (*it)->getAttrTypeAddr();
					}
    				msg_IO_Vector[l_NumberOfVectorElements].iov_len = txp::LENGTH_OF_ATTR_TYPE;
#ifdef TXP_DEVELOPMENT
					char l_AttrType[64] = {'\0'};
					txp::Attribute::attrTypeToChar(*((txp::AttributeType*)msg_IO_Vector[l_NumberOfVectorElements].iov_base), l_AttrType, sizeof(l_AttrType));
   					snprintf(d_log_buffer, sizeof(d_log_buffer), "Element %d: iov_base=%p, iov_len=%lu, value=0x%04X (%s)", l_NumberOfVectorElements, msg_IO_Vector[l_NumberOfVectorElements].iov_base, msg_IO_Vector[l_NumberOfVectorElements].iov_len, *((uint16_t*)msg_IO_Vector[l_NumberOfVectorElements].iov_base), l_AttrType);
					d_log.write(d_log_buffer, txp::Log::DBG2);
#endif
    	 			++l_NumberOfVectorElements;

					if (!l_RC) {
						// Add the name for this attribute
    					msg_IO_Vector[l_NumberOfVectorElements].iov_base = (*it)->getAttrNameAddr();
    					msg_IO_Vector[l_NumberOfVectorElements].iov_len = txp::LENGTH_OF_ATTR_NAME;
#ifdef TXP_DEVELOPMENT
						char l_AttrName[64] = {'\0'};
						txp::Attribute::attrNameToChar(*((txp::AttributeName*)msg_IO_Vector[l_NumberOfVectorElements].iov_base), l_AttrName, sizeof(l_AttrName));
   						snprintf(d_log_buffer, sizeof(d_log_buffer), "Element %d: iov_base=%p, iov_len=%lu, value=0x%04X (%s)", l_NumberOfVectorElements, msg_IO_Vector[l_NumberOfVectorElements].iov_base, msg_IO_Vector[l_NumberOfVectorElements].iov_len, *((uint16_t*)msg_IO_Vector[l_NumberOfVectorElements].iov_base), l_AttrName);
						d_log.write(d_log_buffer, txp::Log::DBG2);
#endif
    	 				++l_NumberOfVectorElements;

						switch ((*it)->getAttrType()) {
							case txp::PTR_CHAR_ARRAY:
							{
								// If PTR_CHAR_ARRAY, add the char array length
    	 						msg_IO_Vector[l_NumberOfVectorElements].iov_base = (*it)->getLengthCharArrayAddr();
    	 						msg_IO_Vector[l_NumberOfVectorElements].iov_len = txp::LENGTH_OF_ATTR_CHAR_ARRAY_LENGTH;
#ifdef TXP_DEVELOPMENT
   								snprintf(d_log_buffer, sizeof(d_log_buffer), "Element %d: iov_base=%p, iov_len=%lu, value=0x%08X (%d)", l_NumberOfVectorElements, msg_IO_Vector[l_NumberOfVectorElements].iov_base, msg_IO_Vector[l_NumberOfVectorElements].iov_len, *((int32_t*)msg_IO_Vector[l_NumberOfVectorElements].iov_base), *((int32_t*)msg_IO_Vector[l_NumberOfVectorElements].iov_base));
								d_log.write(d_log_buffer, txp::Log::DBG2);
#endif
 				   				++l_NumberOfVectorElements;
 				   				break;
							}

							case txp::PTR_ARRAY_OF_CHAR_ARRAYS:
							{
								// If PTR_ARRAY_OF_CHAR_ARRAYS, add the number of elements and char array length
    	 						msg_IO_Vector[l_NumberOfVectorElements].iov_base = (*it)->getNumberOfElementsArrayOfCharArraysAddr();
    	 						msg_IO_Vector[l_NumberOfVectorElements].iov_len = txp::LENGTH_OF_ATTR_ARRAY_OF_CHAR_ARRAYS_NUMBER_OF_ELEMENTS + LENGTH_OF_ATTR_ARRAY_OF_CHAR_ARRAYS_LENGTH;
#ifdef TXP_DEVELOPMENT
   								snprintf(d_log_buffer, sizeof(d_log_buffer), "Element %d: iov_base=%p, iov_len=%lu, value=0x%08X (%d)", l_NumberOfVectorElements, msg_IO_Vector[l_NumberOfVectorElements].iov_base, msg_IO_Vector[l_NumberOfVectorElements].iov_len, *((int32_t*)msg_IO_Vector[l_NumberOfVectorElements].iov_base), *((int32_t*)msg_IO_Vector[l_NumberOfVectorElements].iov_base));
								d_log.write(d_log_buffer, txp::Log::DBG2);
#endif
 				   				++l_NumberOfVectorElements;
 				   				break;
 				   			}

							default:
								break;
						}

						// Add the data for this attribute
    					msg_IO_Vector[l_NumberOfVectorElements].iov_base = (*it)->getDataPtr();
    					msg_IO_Vector[l_NumberOfVectorElements].iov_len = (*it)->getLengthOfValueInBuffer() - (*it)->getLengthPriorToData();
#ifdef TXP_DEVELOPMENT
   						snprintf(d_log_buffer, sizeof(d_log_buffer), "Element %d: iov_base=%p, iov_len=%lu\n", l_NumberOfVectorElements, msg_IO_Vector[l_NumberOfVectorElements].iov_base, msg_IO_Vector[l_NumberOfVectorElements].iov_len);
						d_log.write(d_log_buffer, txp::Log::DBG2);
  	            		txp::Log::dump_buffer_raw(d_log, (const char*)msg_IO_Vector[l_NumberOfVectorElements].iov_base, msg_IO_Vector[l_NumberOfVectorElements].iov_len, "Attr data");
#endif
	    				++l_NumberOfVectorElements;

	    				// Calculate the worst case scenario for filling iov...
	    				// Each pass through the attribute vector 'could' add 4 elements
	    				// to iov...  @@DLH
	    				if ( l_NumberOfVectorElements+4 > IOV_MAX ) {
	    					l_RC = -14;
	    					break;
	    				}
	    			}
	    		}
    		}

    		if (!l_RC) {
				getHeapBufferPtr()->setMsgLength(getMsgLengthWithDataValues());
				msg_IO_VectorCount = l_NumberOfVectorElements;
			}

		} else {
			l_RC = -36;
		}

	} else {
		l_RC = -15;
	}

	if (!l_RC) {
	    msgState.setIO_VectorIsCurrentFlag(1);
	    l_RC = getMsgLengthWithDataValues();
#ifdef TXP_DEVELOPMENT
		char l_MsgId[64] = {'\0'};
		msgIdToChar(getMsgId(), l_MsgId, sizeof(l_MsgId));
		snprintf(d_log_buffer, sizeof(d_log_buffer), "Msg(%p) associated with IO_Vector(%p), %s, %lu attributes, %d vectors", this, msg_IO_Vector, l_MsgId, (msgAttrMap ? msgAttrMap->size() : 0), msg_IO_VectorCount);
		d_log.write(d_log_buffer, txp::Log::DBG2, txp::Log::C_DTOR_LOGLEVEL);
	} else {
		snprintf(d_log_buffer, sizeof(d_log_buffer), "Msg::build_IO_Vector(), RC=%d", l_RC);
		txp::Msg::d_log.write(d_log_buffer, txp::Log::ERROR);
#endif
	}

	return l_RC;
}


int txp::Msg::buildResponseMsg(txp::Msg* &oMsg) {
	int l_RC = 0;
    oMsg=NULL;
	if (!requestMsgNumber) {
		oMsg = new txp::Msg(getMsgId(), getMsgNumber(), getVersion(), getMsgCreatorPid(), getMsgCreatorTid());
		if (oMsg) {
			oMsg->msgState.setAllocatedFlag(1);
#ifdef TXP_DEVELOPMENT
			char l_MsgId[64] = {'\0'};
			msgIdToChar(oMsg->getMsgId(), l_MsgId, sizeof(l_MsgId));
			snprintf(d_log_buffer, sizeof(d_log_buffer), "Msg::Msg(%p), %s, RMsg#=%d is response to Msg(%p), Msg#=%d", oMsg, l_MsgId, oMsg->getMsgNumber(), this, getMsgNumber());
			d_log.write(d_log_buffer, txp::Log::DEBUG);
#endif
		} else {
	    	l_RC = -37;
#ifdef TXP_DEVELOPMENT
			snprintf(d_log_buffer, sizeof(d_log_buffer), "Msg::buildResponseMsg(%p), RC=%d, msgNumber=%d, could not create response message", oMsg, l_RC, msgNumber);
			txp::Msg::d_log.write(d_log_buffer, txp::Log::ERROR);
#endif
		}
	} else {
		// This message already has a response message
	    l_RC = -16;
#ifdef TXP_DEVELOPMENT
		snprintf(d_log_buffer, sizeof(d_log_buffer), "Msg::buildResponseMsg(%p), RC=%d, msgNumber=%d", oMsg, l_RC, msgNumber);
		txp::Msg::d_log.write(d_log_buffer, txp::Log::ERROR);
#endif
	}

	return l_RC;
}


/*
ssize_t txp::Msg::calculateLengthWithDataValues() {
	ssize_t l_Value = OFFSET_TO_FIRST_ATTRIBUTE;

	if (msgAttributes) }
		for (std::vector<Attribute*>::iterator it = msgAttributes.begin(); it != msgAttributes.end(); ++it) {
			l_Value += (*it)->getLengthOfValueInBuffer();
		}
	}

	return l_Value;
}


ssize_t txp::Msg::calculateLengthWithPtrValues() {
	ssize_t l_Value = OFFSET_TO_FIRST_ATTRIBUTE;

	if (msgAttributes) {
		for (std::vector<Attribute*>::iterator it = msgAttributes.begin(); it != msgAttributes.end(); ++it) {
			if ((*it)->isPtrDataType()) {
				l_Value += (*it)->getLengthInBuffer();
			} else {
				Attribute l_Attr = Attribute(Attribute::ConvertToPtrDataType((*it)->getDataType()), (*it)->getId());
				l_Value += l_Attr.getLengthInBuffer();
			}
		}
	}

	return l_Value;
}
*/


size_t txp::Msg::calculatePad(const size_t pLength, const size_t pDataAlignment) {
	size_t l_Pad = 0;

	if (pDataAlignment>1) {
	    if (txp::HeapBuffer::validateDataAlignment(pDataAlignment)) {
	    	l_Pad = txp::HeapBuffer::calculatePad(pLength, pDataAlignment);
    	}
    }

	return l_Pad;
};


//  NOTE:  There is NO verification of the data in the buffer being interpreted
//         as a consecutive set of attributes.  User beware...
//         As such, pMsgLength must be set to the length of data to deserialize.  @@DLH
int txp::Msg::deserializeAttrsToMsg(HeapBuffer* pHeapBuffer, const size_t pMsgLength) {

	return deserializeAttrsToMsg(pHeapBuffer, txp::COPY_DATA_TO_OBJECT, pMsgLength);
}


//  NOTE:  There is NO verification of the data in the buffer being interpreted
//         as a consecutive set of attributes.  User beware...
//         As such, pMsgLength must be set to the length of data to deserialize.  @@DLH
int txp::Msg::deserializeAttrsToMsg(HeapBuffer* pHeapBuffer, const txp::DeserializeOption &pOption, const size_t pMsgLength) {
	int l_RC = 0;
	unsigned long l_MsgCRC = 0, l_CalcMsgCRC = 0;
	unsigned long* l_PtrCalcMsgCRC = 0;

#ifdef TXP_DEVELOPMENT
	pHeapBuffer->dumpMsgAttrs(d_log, "Portion of buffer with attributes to deserialize");
#endif

	size_t l_MsgLengthInBuffer = pHeapBuffer->getMsgLength();
	size_t l_MsgLength = pMsgLength;

#ifdef TXP_DEVELOPMENT
 	char l_Option[64] = {'\0'};
	deserializeOptionToChar(pOption, l_Option, sizeof(l_Option));
  	snprintf(d_log_buffer, sizeof(d_log_buffer), "deserializeAttrsToMsg(%p,%s,%zu)", pHeapBuffer, l_Option, pMsgLength);
	d_log.write(d_log_buffer, txp::Log::DEBUG);
#endif

	if (msgState.isMsgCRC_Calculated()) {
		if (l_MsgLength>=l_MsgLengthInBuffer) {
			l_MsgCRC = getBufferPtr()->getMsgCRC_Value();
			l_PtrCalcMsgCRC = &l_CalcMsgCRC;
//			printf("deserializeAttrsToMsg: l_MsgCRC=0x%016lX\n", l_MsgCRC);
		} else {
			l_RC = -41;
		}
	}

	// NOTE: We are only going to deserialize the attributes, but we assume that the message header
	//       still exists at offset 0 of the data buffer for the heap buffer.  Bump past the header... @@DLH

	if (!l_RC) {
		size_t l_Offset = OFFSET_TO_FIRST_ATTRIBUTE;
		txp::Attribute* l_Attr;
		size_t l_AttrLength;
		while (l_Offset<l_MsgLength && l_RC == 0) {
			l_Attr = 0;
			l_AttrLength = 0;
			l_RC = txp::Attribute::buildAttr(pHeapBuffer, l_Offset, l_Attr, l_AttrLength, pOption, l_PtrCalcMsgCRC);
			if (!l_RC) {
				l_RC = addAttribute(l_Attr);
				l_Offset += l_AttrLength;
			}

			// If the add failed, delete the attribute...
			if (l_RC && l_Attr)
			{
                delete l_Attr;
			}
		}

		if (msgState.isMsgCRC_Calculated()) {
//			printf("deserializeAttrsToMsg: l_MsgCRC=0x%016lX, l_CalcMsgCRC=0x%016lX\n", l_MsgCRC, l_CalcMsgCRC);
			if (l_MsgCRC != l_CalcMsgCRC) {
    			l_RC = -40;
			}
		}
	}

#ifdef TXP_DEVELOPMENT
	char l_MsgId[64] = {'\0'};
	msgIdToChar(getMsgId(), l_MsgId, sizeof(l_MsgId));
	if (l_RC) {
 		snprintf(d_log_buffer, sizeof(d_log_buffer), "deserializeAttrsToMsg(): %s, RC=%d", l_MsgId, l_RC);
		d_log.write(d_log_buffer, txp::Log::ERROR);

	} else {
		snprintf(d_log_buffer, sizeof(d_log_buffer), "Msg(%p) %s, Msg#=%d, #Attrs=%lu, RC=%d", this, l_MsgId, msgNumber, (msgAttrMap ? msgAttrMap->size() : 0), l_RC);
		d_log.write(d_log_buffer, txp::Log::DEBUG);
		this->dump(d_log, "Deserialized message with attributes");
	}
#endif

	return l_RC;
}


void txp::Msg::dump(Log& pLog, const char* pPrefix, const int pDumpRawHeapBuffer) {
#ifdef TXP_DEVELOPMENT
	char l_MsgId[64] = {'\0'};

	if (pLog.logLevelCheck(txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL)) {

		START_PREFIX(pPrefix, ">>>>> Start dump for message:  %s");

		snprintf(d_log_buffer, sizeof(d_log_buffer), "Msg Number:            0x%08X (%6d)", msgNumber, msgNumber);
		pLog.write(d_log_buffer, txp::Log::DEBUG, txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL);

		snprintf(d_log_buffer, sizeof(d_log_buffer), "Creator's Pid/Tid:     %5u / %-5u", (uint32_t)msgCreatorPid, (uint32_t)msgCreatorTid);
		pLog.write(d_log_buffer, txp::Log::DEBUG, txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL);

		if (prevMsgNumber) {
			snprintf(d_log_buffer, sizeof(d_log_buffer), "Previous Msg Number:   0x%08X (%6d)", prevMsgNumber, prevMsgNumber);
			pLog.write(d_log_buffer, txp::Log::DEBUG, txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL);
		}
//		} else {
//			snprintf(d_log_buffer, sizeof(d_log_buffer), "Previous Msg Number:   None");
//		}

		if (nextMsgNumber) {
			snprintf(d_log_buffer, sizeof(d_log_buffer), "Next Msg Number:       0x%08X (%6d)", nextMsgNumber, nextMsgNumber);
			pLog.write(d_log_buffer, txp::Log::DEBUG, txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL);
		}
//		} else {
//			snprintf(d_log_buffer, sizeof(d_log_buffer), "Next Msg Number:       None");
//		}

		if (requestMsgNumber) {
			snprintf(d_log_buffer, sizeof(d_log_buffer), "Request Msg Number:    0x%08X (%6d)", requestMsgNumber, requestMsgNumber);
			pLog.write(d_log_buffer, txp::Log::DEBUG, txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL);
			snprintf(d_log_buffer, sizeof(d_log_buffer), "Requestor's Pid/Tid:   %5u / %-5u", (uint32_t)msgRequestorPid, (uint32_t)msgRequestorTid);
			pLog.write(d_log_buffer, txp::Log::DEBUG, txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL);
		}
//		} else {
//			snprintf(d_log_buffer, sizeof(d_log_buffer), "Request Msg Number:    None");
//		}

		msgIdToChar(msgId, l_MsgId, sizeof(l_MsgId));
		snprintf(d_log_buffer, sizeof(d_log_buffer), "Id:                    %s", l_MsgId);
		pLog.write(d_log_buffer, txp::Log::DEBUG, txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL);

		msgVersion.dump(pLog);

		msgState.dump(pLog);

		snprintf(d_log_buffer, sizeof(d_log_buffer), "Total MLen, w/Values:               0x%08X (%6zu)", (uint32_t)msgLengthWithDataValues, msgLengthWithDataValues);
		pLog.write(d_log_buffer, txp::Log::DEBUG, txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL);

		snprintf(d_log_buffer, sizeof(d_log_buffer), "Total MLen, w/All Ptrs:             0x%08X (%6zu)", (uint32_t)msgLengthWithPtrValues, msgLengthWithPtrValues);
		pLog.write(d_log_buffer, txp::Log::DEBUG, txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL);

		snprintf(d_log_buffer, sizeof(d_log_buffer), "Total MLen, w/Values Except Arrays: 0x%08X (%6zu)", (uint32_t)msgLengthWithDataValuesExceptForArrays, msgLengthWithDataValuesExceptForArrays);
		pLog.write(d_log_buffer, txp::Log::DEBUG, txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL);

		pLog.write(" ", txp::Log::DEBUG, txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL);

		snprintf(d_log_buffer, sizeof(d_log_buffer), "I/O Vector:            %p", msg_IO_Vector);
		pLog.write(d_log_buffer, txp::Log::DEBUG, txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL);
		snprintf(d_log_buffer, sizeof(d_log_buffer), "I/O Vector Count:      0x%08X (%6d)", msg_IO_VectorCount, msg_IO_VectorCount);
		pLog.write(d_log_buffer, txp::Log::DEBUG, txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL);

		pLog.write(" ", txp::Log::DEBUG, txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL);
		snprintf(d_log_buffer, sizeof(d_log_buffer), "Attribute Map:         %p", msgAttrMap);
		pLog.write(d_log_buffer, txp::Log::DEBUG, txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL);
		if (msgAttrMap) {
			snprintf(d_log_buffer, sizeof(d_log_buffer), "Attribute Map Count:   0x%08X (%6lu)", (unsigned int)msgAttrMap->size(), msgAttrMap->size());
			pLog.write(d_log_buffer, txp::Log::DEBUG, txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL);
		}

		pLog.write(" ", txp::Log::DEBUG, txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL);
		snprintf(d_log_buffer, sizeof(d_log_buffer), "Attributes:            %p", msgAttributes);
		pLog.write(d_log_buffer, txp::Log::DEBUG, txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL);
		if (msgAttributes) {
			snprintf(d_log_buffer, sizeof(d_log_buffer), "Attribute Count:       0x%08X (%6lu)", (unsigned int)msgAttributes->size(), msgAttributes->size());
			pLog.write(d_log_buffer, txp::Log::DEBUG, txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL);
 			dump_msgAttributes(msgAttributes, pLog, pPrefix);
		}

		pLog.write(" ", txp::Log::DEBUG, txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL);
		snprintf(d_log_buffer, sizeof(d_log_buffer), "Heap Buffer:           %p", msgHeapBuffer);
		pLog.write(d_log_buffer, txp::Log::DEBUG, txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL);
		if (pDumpRawHeapBuffer && msgHeapBuffer) {
 			msgHeapBuffer->dump(pLog, pPrefix);
 		}

		END_PREFIX(pPrefix, ">>>>>   End dump for message:  %s", 1);
	}
#endif

	return;
}


void txp::Msg::dump_msgAttributes(std::vector<Attribute*>* pMsgAttributes, Log& pLog, const char* pPrefix) {
#ifdef TXP_DEVELOPMENT
	if (pLog.logLevelCheck(txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL)) {
		if (pMsgAttributes) {
			START_PREFIX(pPrefix, ">>>>> Start dump for message attributes:  %s");

			unsigned int i=1;
			for(std::vector<Attribute*>::iterator it = pMsgAttributes->begin(); it != pMsgAttributes->end(); ++i, ++it) {
				snprintf(d_log_buffer, sizeof(d_log_buffer), ">>>>> Start dump for attribute %d", i);
				pLog.write(d_log_buffer, txp::Log::DEBUG, txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL);

				pLog.incrIndent(2);

				(**it).dump(pLog);

				pLog.decrIndent(2);

				snprintf(d_log_buffer, sizeof(d_log_buffer), ">>>>>   End dump for attribute %d", i);
				pLog.write(d_log_buffer, txp::Log::DEBUG, txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL);
			}

			END_PREFIX(pPrefix, ">>>>>   End dump for message attributes:  %s", 0);
		}
	}
#endif

	return;
}


void txp::Msg::freeHeapBuffer() {
	if (msgHeapBuffer) {
		// NOTE:  Turn the 'is current' flag off...
		msgState.setHeapBufferIsCurrentFlag(0);

		HeapBuffer* l_Temp = msgHeapBuffer;
		msgHeapBuffer = 0;
		l_Temp->~HeapBuffer();

		// If allocated by the message facility,
		// then delete the associated storage.
		if (msgState.isHeapBufferAllocated()) {
			delete l_Temp;
		}
	}

	return;
}


char* txp::Msg::getDataBufferPtr() {
	if (msgHeapBuffer) {
		return msgHeapBuffer->getDataBufferPtr();
	} else {
		return 0;
	}
}


int txp::Msg::nextMsg(Msg &pNextMsg) {
	int l_RC = 0;

	if (!(getNextMsgNumber())) {
		if (!(pNextMsg.getPrevMsgNumber())) {
			// Additional validation?  Error cases??  @@DLH

			// Chain the messages together
			this->nextMsgNumber = pNextMsg.getMsgNumber();
			pNextMsg.prevMsgNumber = getMsgNumber();

			// Copy the response message number to the added message
			pNextMsg.requestMsgNumber = this->getRequestMsgNumber();
		} else {
			// Next message already has a previous message
            l_RC = -17;
		}
	} else {
		// This message already has a next message
        l_RC = -18;
	}

#ifdef TXP_DEVELOPMENT
	char l_MsgId[64] = {'\0'};
	msgIdToChar(getMsgId(), l_MsgId, sizeof(l_MsgId));
 	snprintf(d_log_buffer, sizeof(d_log_buffer), "Msg::nextMsg(Msg): RC=%d, %s, msgNumber=%d, msgNumber of msg being added=%d", l_RC, l_MsgId, msgNumber, pNextMsg.getMsgNumber());
	if (l_RC) {
		d_log.write(d_log_buffer, txp::Log::ERROR);
	} else {
		d_log.write(d_log_buffer, txp::Log::DEBUG);
	}
#endif

	return l_RC;
}


txp::Attribute* txp::Msg::retrieveAttr(const int32_t pNameValue) {
	txp::Attribute* l_AttrPtr = 0;

	if (txp::Attribute::isAttrNameValid(pNameValue)) {
		AttributeName l_AttrName((txp::AttributeName)pNameValue);
		l_AttrPtr = retrieveAttr(l_AttrName);
	}

	return l_AttrPtr;
}


txp::Attribute* txp::Msg::retrieveAttr(const txp::AttributeName &pName) {
	std::map<txp::AttributeName, Attribute*>::iterator it;
	Attribute* l_AttrPtr = 0;

	if (msgAttrMap) {
		it = msgAttrMap->find(pName);
  		if (it != msgAttrMap->end()) {
  			l_AttrPtr = it->second;
  		}
  	}

	return l_AttrPtr;
}


std::map<txp::AttributeName, txp::Attribute*>* txp::Msg::retrieveAttrs() {

	return msgAttrMap;
}


void txp::Msg::sent() {
	if (!isResponseMsg()) {
		msgState.setSentFlag(1);
	} else {
		msgState.setResponseSentFlag(1);
	}

	return;
}


int txp::Msg::serializeCommonHeader(HeapBuffer* pBuffer, size_t pDataAlignment) {
	int l_RC = 0;

	//  Total Common Header length = 0x30 (48 bytes)
	if (pBuffer) {
		if (pBuffer->checkDataAlignment(pDataAlignment)) {
		    // NOTE:  Keep the serialized common header a multiple of 4 bytes for CRC calculation purposes.
		    // (1) First four bytes is the eyecatcher
		    // (2) Next four bytes is reserved
		    // (3) Next eight bytes is the total length
		    // Reset the buffer (which will set those first sixteen bytes...)
		    pBuffer->reset();
		    // (4) Next four bytes is the message number
		    int32_t l_MsgNumber = msgNumber;
		    if (!(pBuffer->add((void*)&(l_MsgNumber), sizeof(l_MsgNumber)))) {
		    	// (5) Next four bytes is the previous message number for a continuation message
		    	int32_t l_PrevMsgNumber = prevMsgNumber;
		    	if (!(pBuffer->add((void*)&(l_PrevMsgNumber), sizeof(l_PrevMsgNumber)))) {
		    		// (6) Next four bytes is the next message number for a continuation message
		    		int32_t l_NextMsgNumber = nextMsgNumber;
		    		if (!(pBuffer->add((void*)&(l_NextMsgNumber), sizeof(l_NextMsgNumber)))) {
		    			// (7) Next four bytes is the request message number for a continuation message
		    			int32_t l_RequestMsgNumber = requestMsgNumber;
		    			if (!(pBuffer->add((void*)&(l_RequestMsgNumber), sizeof(l_RequestMsgNumber)))) {
		    				// (8) Next four bytes is the creator pid
		    				if (!(pBuffer->add((void*)&(msgCreatorPid), sizeof(msgCreatorPid)))) {
		    					// (9) Next four bytes is the requestor pid
		    					if (!(pBuffer->add((void*)&(msgRequestorPid), sizeof(msgRequestorPid)))) {
		    						// (10) Next eight bytes is the creator tid
		    						if (!(pBuffer->add((void*)&(msgCreatorTid), sizeof(msgCreatorTid)))) {
		    							// (11) Next eight bytes is the requestor tid
		    							if (!(pBuffer->add((void*)&(msgRequestorTid), sizeof(msgRequestorTid)))) {
		    								// (12) Next four bytes is the message id
		    								if (!(pBuffer->add((void*)&(msgId), sizeof(msgId)))) {
		    									// (13) Next two bytes is the version, (major, minor)
		    									if (!(msgVersion.add(pBuffer))) {
		    										// (14) Next two bytes are the message state flags
		    										if (!(msgState.add(pBuffer))) {
		    											// (15) Next eight bytes is the message header CRC value
		    											//! NOTE:  It is set to zero here and 'may' be calculated and filled in later.
		    											if (!(pBuffer->add((void*)&(CRC_Value), sizeof(CRC_Value)))) {
		    												// (16) Next eight bytes is the message attributes CRC value
		    												//! NOTE:  It is set to zero here and 'may' be calculated and filled in later.
		    												if (!(pBuffer->add((void*)&(CRC_Value), sizeof(CRC_Value)))) {
		    													// (17) Next byte is the pad length
		    													uint8_t l_PadLength = (uint8_t)(pBuffer->getPadLength());
   	                            				        		if (!(pBuffer->add((void*)&(l_PadLength), sizeof(l_PadLength)))) {
			    													// (18) Next fifteenteen bytes are reserved
	   	                            				        		if (!(pBuffer->add((void*)&(reserved_2), sizeof(reserved_2)))) {
			    														pBuffer->setMsgLength(OFFSET_TO_FIRST_ATTRIBUTE);
        	                        				        		} else {
            	                    				            		l_RC = -19;
                	                				        		}
                                				        		} else {
                                				            		l_RC = -59;
                                				        		}
                                				        	} else {
                                				            	l_RC = -43;
                                				        	}
                                				        } else {
                                				            l_RC = -42;
                                				        }
		    										} else {
                                				    	l_RC = -20;
		    										}
		    									} else {
                                				    l_RC = -21;
		    									}
		    								} else {
		    		                            l_RC = -22;
		    								}
		    							} else {
                            			    l_RC = -46;
		    							}
		    						} else {
                            		    l_RC = -47;
		    						}
		    					} else {
                            	    l_RC = -48;
		    					}
		    				} else {
                                l_RC = -49;
		    				}
		    			} else {
                            l_RC = -23;
		    			}
		    		} else {
                        l_RC = -24;
		    		}
		    	} else {
                    l_RC = -25;
		    	}
		    } else {
                l_RC = -26;
		    }
		} else {
			l_RC = -58;
                        (void)l_RC;
			abort();
		}
	} else {
        l_RC = -27;
	}

#ifdef TXP_DEVELOPMENT
	if (l_RC) {
		char l_MsgId[64] = {'\0'};
		msgIdToChar(getMsgId(), l_MsgId, sizeof(l_MsgId));
 		snprintf(d_log_buffer, sizeof(d_log_buffer), "Msg::serializeCommonHeader(%p): RC=%d, %s, msgNumber=%d", pBuffer, l_RC, l_MsgId, msgNumber);
		d_log.write(d_log_buffer, txp::Log::ERROR);
	}
#endif

	return l_RC;
}


ssize_t txp::Msg::serializeToHeapBuffer(size_t pDataAlignment, int pCalculateCRC) {
	ssize_t l_RC = 0;

#ifdef TXP_DEVELOPMENT
	this->dump(d_log, "Message to serialize to heap buffer", 0);
#endif

	// Verify that the length to serialize is not greater than the size available in the heap buffer...
	// NOTE:  Need to add back the OFFSET_TO_MSG_NUMBER because that is the amount of data that is
	//        already set by the construction of Msg() and will be skipped over by this method.  @@DLH
	if (msgState.attrExistsWithPtrToNonArrayData()) {
		if (msgLengthWithPtrValues > (msgHeapBuffer->getSize()-msgHeapBuffer->getOffset()+OFFSET_TO_MSG_NUMBER)) {
			l_RC = -28;
		} else {
			if (msgLengthWithDataValuesExceptForArrays > (msgHeapBuffer->getSize()-msgHeapBuffer->getOffset()+OFFSET_TO_MSG_NUMBER)) {
				l_RC = -50;
			}
		}
	}

	if (!l_RC) {
		// Build the common message header...

		// Optimistically set flags in the message header
		// before we serialize the header
		//! NOTE:  The values in the message header that get serialized
		//         into the heap buffer cannot be changed after the
		//         serializeCommonHeader() invocation returns.  This
		//         means that all flags have to be set before the CRC
		//         message header value is calculated.
		//         The only exception to this rule is the actual CRC messsage
		//         header value itself.  If requested, it will be written
		//         directly into the heap buffer.
		msgState.setHeapBufferIsCurrentFlag(1);
		if (pCalculateCRC) {
			msgState.setMsgCRC_IsCalculated(1);
		}

		unsigned long l_MsgHeaderCRC = 0, l_MsgCRC = 0;
		unsigned long* l_PtrMsgCRC = 0;
		if (!(serializeCommonHeader(msgHeapBuffer, pDataAlignment))) {
			if (pCalculateCRC) {
				l_PtrMsgCRC = &l_MsgCRC;
			}

			// Build each of the message attributes...
			if (msgAttributes) {
				for(std::vector<Attribute*>::iterator it = msgAttributes->begin(); it != msgAttributes->end() && l_RC == 0; ++it) {
//					if (pCalculateCRC) {
//						printf("serializeToHeapBuffer: Before: l_PtrMsgCRC=%p, *l_PtrMsgCRC=0x%016lX\n", l_PtrMsgCRC, *l_PtrMsgCRC);
//				    }
					l_RC = (*it)->add(msgHeapBuffer, l_PtrMsgCRC);
//					if (pCalculateCRC) {
//						printf("serializeToHeapBuffer:  After: l_PtrMsgCRC=%p, *l_PtrMsgCRC=0x%016lX\n", l_PtrMsgCRC, *l_PtrMsgCRC);
//					}
				}
			}

			if (!l_RC) {
				l_RC = getBufferPtr()->padToDataAlignment(pDataAlignment);
				if ((!l_RC) && pCalculateCRC) {
					// NOTE:  The message header CRC cannot be calculated until all lof the attributes have been serialized.
					//        The total length of the message is in the header and it is updated for each serialized attributes...
					calculateMsgHeaderCRC_Value(l_MsgHeaderCRC);
					// Set the CRC values in the heap buffer
					getBufferPtr()->setMsgHeaderCRC_Value(l_MsgHeaderCRC);
					getBufferPtr()->setMsgCRC_Value(l_MsgCRC);
//					printf("serializeToHeapBuffer: l_MsgHeaderCRC=0x%016lX, l_MsgCRC=0x%016lX\n", l_MsgHeaderCRC, l_MsgCRC);
				}
			}
		}

		if (l_RC) {
			// Unset the optimistically set flags...
			msgState.setHeapBufferIsCurrentFlag(0);
			msgState.setMsgCRC_IsCalculated(0);
		} else {
			l_RC = getSerializedLen();
		}
	}

#ifdef TXP_DEVELOPMENT
	char l_MsgId[64] = {'\0'};
	msgIdToChar(getMsgId(), l_MsgId, sizeof(l_MsgId));
	snprintf(d_log_buffer, sizeof(d_log_buffer),
	         "serializeToHeapBuffer(): %s, Msg#=%d, #Attrs=%lu, RC=%zu",
	         l_MsgId, msgNumber, (msgAttributes ? msgAttributes->size() : 0), l_RC);
	if (l_RC > 0) {
		d_log.write(d_log_buffer, txp::Log::DEBUG);
	} else {
		d_log.write(d_log_buffer, txp::Log::ERROR);
	}
	msgHeapBuffer->dump(d_log, "Serialized message in buffer");
#endif

	return l_RC;
}


ssize_t txp::Msg::serializeWithValuesToHeapBuffer(size_t pDataAlignment, int pCalculateCRC) {
	ssize_t l_RC = 0;

#ifdef TXP_DEVELOPMENT
	this->dump(d_log, "Message to serialize with value to heap buffer", 0);
#endif

	// Verify that the length to serialize is not greater than the size available in the heap buffer...
	// NOTE:  Need to add back the OFFSET_TO_MSG_NUMBER because that is the amount of data that is
	//        already set by the construction of Msg() and will be skipped over by this method.  @@DLH
	if (msgLengthWithDataValues > (msgHeapBuffer->getSize()-msgHeapBuffer->getOffset()+OFFSET_TO_MSG_NUMBER)) {
		l_RC = -29;
	}

	if (!l_RC) {
		// Build the common message header...

		// Optimistically set flags in the message header
		// before we serialize the header
		//! NOTE:  The values in the message header that get serialized
		//         into the heap buffer cannot be changed after the
		//         serializeCommonHeader() invocation returns.  This
		//         means that all flags have to be set before the CRC
		//         message header value is calculated.
		//         The only exception to this rule is the actual CRC messsage
		//         header value itself.  If requested, it will be written
		//         directly into the heap buffer.
		msgState.setHeapBufferIsCurrentFlag(1);
		if (pCalculateCRC) {
			msgState.setMsgCRC_IsCalculated(1);
		}

		unsigned long l_MsgHeaderCRC = 0, l_MsgCRC = 0;
		unsigned long* l_PtrMsgCRC = 0;
		if (!(serializeCommonHeader(msgHeapBuffer, pDataAlignment))) {
			if (pCalculateCRC) {
				l_PtrMsgCRC = &l_MsgCRC;
			}

			// Build each of the message attributes...
			if (msgAttributes) {
				for(std::vector<Attribute*>::iterator it = msgAttributes->begin(); it != msgAttributes->end() && l_RC == 0; ++it) {
//					if (pCalculateCRC) {
//						printf("serializeWithValuesToHeapBuffer: Before: l_PtrMsgCRC=%p, *l_PtrMsgCRC=0x%016lX\n", l_PtrMsgCRC, *l_PtrMsgCRC);
//					}
					l_RC = (*it)->addValue(msgHeapBuffer, l_PtrMsgCRC);
//					if (pCalculateCRC) {
//						printf("serializeWithValuesToHeapBuffer:  After: l_PtrMsgCRC=%p, *l_PtrMsgCRC=0x%016lX\n", l_PtrMsgCRC, *l_PtrMsgCRC);
//					}
				}
			}

			if (!l_RC) {
				l_RC = getBufferPtr()->padToDataAlignment(pDataAlignment);
				if ((!l_RC) && pCalculateCRC) {
					// NOTE:  The message header CRC cannot be calculated until all lof the attributes have been serialized.
					//        The total length of the message is in the header and it is updated for each serialized attributes...
					calculateMsgHeaderCRC_Value(l_MsgHeaderCRC);
					// Set the CRC values in the heap buffer
					getBufferPtr()->setMsgHeaderCRC_Value(l_MsgHeaderCRC);
					getBufferPtr()->setMsgCRC_Value(l_MsgCRC);
//					printf("serializeWithValuesToHeapBuffer: l_MsgHeaderCRC=0x%016lX, l_MsgCRC=0x%016lX\n", l_MsgHeaderCRC, l_MsgCRC);
				}
			}
		}

		if (l_RC) {
			// Unset the optimistically set flags...
			msgState.setHeapBufferIsCurrentFlag(0);
			msgState.setMsgCRC_IsCalculated(0);
		} else {
			l_RC = getSerializedLen();
		}
	}

#ifdef TXP_DEVELOPMENT
	char l_MsgId[64] = {'\0'};
	msgIdToChar(getMsgId(), l_MsgId, sizeof(l_MsgId));
 	snprintf(d_log_buffer, sizeof(d_log_buffer),
 	         "serializeWithValuesToHeapBuffer(): %s, Msg#=%d, #Attrs=%lu, RC=%zu",
 	         l_MsgId, msgNumber, (msgAttributes ? msgAttributes->size() : 0), l_RC);
	if (l_RC > 0) {
		d_log.write(d_log_buffer, txp::Log::DEBUG);
	} else {
		d_log.write(d_log_buffer, txp::Log::ERROR);
	}
	msgHeapBuffer->dump(d_log, "SerializeWithValues message in buffer");
#endif

	return l_RC;
}


void txp::Msg::setMsgCreatorIds() {
	setMsgCreatorPid();
	setMsgCreatorTid();

	return;
}


void txp::Msg::setMsgCreatorPid(pid_t* pPid) {
	if (pPid) {
		msgCreatorPid = *pPid;
	} else {
		msgCreatorPid = getpid();
	}

	return;
}


void txp::Msg::setMsgCreatorTid(pthread_t* pTid) {
	if (pTid) {
		msgCreatorTid = *pTid;
	} else {
		msgCreatorTid = pthread_self();
	}

	return;
}
