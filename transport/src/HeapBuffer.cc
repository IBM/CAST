/*******************************************************************************
 |    HeapBuffer.cc
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
#include <string.h>

#include "crc.h"
#include "HeapBuffer.h"

//*****************************************************************************
//  Static data members
//*****************************************************************************
#ifdef TXP_DEVELOPMENT
txp::Log txp::HeapBuffer::d_log(txp::Log::DEFAULT_LOG_DESTINATION, txp::Log::DEFAULT_OPEN_LOGSTATE, txp::Log::DEFAULT_OPEN_LOGLEVEL);
#endif
char txp::HeapBuffer::d_log_buffer[txp::DEFAULT_LINE_LENGTH] = {'\0'};


//*****************************************************************************
//  Constructors
//*****************************************************************************
txp::HeapBuffer::HeapBuffer() :
	size(txp::DEFAULT_BUFFER_SIZE),
 	offset(MAX_OFFSET),
 	padLength(0) {
	setDataAllocatedFlag(1);
	buffer = new char[txp::DEFAULT_BUFFER_SIZE];
	reset();

#ifdef TXP_DEVELOPMENT
	snprintf(d_log_buffer, sizeof(d_log_buffer), "HeapBuffer::HeapBuffer()=%p, allocated buffer=%p", this, buffer);
	d_log.write(d_log_buffer, txp::Log::DBG2);
#endif
}


txp::HeapBuffer::HeapBuffer(const size_t pSize) :
 	size(pSize),
 	offset(MAX_OFFSET),
 	padLength(0) {
	setDataAllocatedFlag(1);
	buffer = new char[pSize];
	reset();

#ifdef TXP_DEVELOPMENT
	snprintf(d_log_buffer, sizeof(d_log_buffer), "HeapBuffer::HeapBuffer(pSize=%zu)=%p, allocated buffer=%p", pSize, this, buffer);
	d_log.write(d_log_buffer, txp::Log::DBG2);
#endif
}


txp::HeapBuffer::HeapBuffer(char* pBuffer, const size_t pSize) :
 	size(pSize),
 	offset(MAX_OFFSET),
 	padLength(0) {
	setDataAllocatedFlag(0);
	buffer = pBuffer;

#ifdef TXP_DEVELOPMENT
	snprintf(d_log_buffer, sizeof(d_log_buffer), "HeapBuffer::HeapBuffer(pBuffer=%p, pSize=%zu)=%p, using existing buffer=%p", pBuffer, pSize, this, buffer);
	d_log.write(d_log_buffer, txp::Log::DBG2);
#endif
}


//*****************************************************************************
//  Destructor
//*****************************************************************************
txp::HeapBuffer::~HeapBuffer() {

#ifdef TXP_DEVELOPMENT
	snprintf(d_log_buffer, sizeof(d_log_buffer), "HeapBuffer::~HeapBuffer()=%p", this);
	d_log.write(d_log_buffer, txp::Log::DEBUG, txp::Log::C_DTOR_LOGLEVEL);
#endif

	if (buffer && isDataAllocated()) {
#ifdef TXP_DEVELOPMENT
		snprintf(d_log_buffer, sizeof(d_log_buffer), "HeapBuffer::~HeapBuffer() buffer=%p", buffer);
		d_log.write(d_log_buffer, txp::Log::DEBUG, txp::Log::C_DTOR_LOGLEVEL);
#endif
		setDataAllocatedFlag(0);
		char* l_TempHeapBuffer = buffer;
		size = 0;
		buffer = 0;
		delete[] l_TempHeapBuffer;
	}
}


//*****************************************************************************
//  Static methods
//*****************************************************************************
size_t txp::HeapBuffer::calculatePad(const size_t pLength, const size_t pDataAlignment) {
    size_t l_Pad = 0;

    if (pDataAlignment>1 && validateDataAlignment(pDataAlignment)) {
        size_t l_Temp = pLength%pDataAlignment;
    	l_Pad = (l_Temp ? pDataAlignment-l_Temp : 0);
    }

    return l_Pad;
}


int txp::HeapBuffer::validateDataAlignment(const size_t pDataAlignment) {
	int l_RC = 1;

	if (pDataAlignment>MAX_DATA_ALIGNMENT_VALUE) {
		l_RC = 0;
#ifdef TXP_DEVELOPMENT
		snprintf(d_log_buffer, sizeof(d_log_buffer), "HeapBuffer::validateDataAlignment(%zu): Invalid data alignment value passed", pDataAlignment);
		d_log.write(d_log_buffer, txp::Log::ERROR);
#endif
	}

	return l_RC;
}


//*****************************************************************************
//  Non-static methods
//*****************************************************************************
int txp::HeapBuffer::add(void* pData, const size_t pSize) {
	int l_RC = 0;

	if (offset+pSize <= size) {
		memcpy(buffer+offset, pData, pSize);
		incrMsgLength(pSize);
		offset += pSize;
	} else {
		l_RC = -301;

#ifdef TXP_DEVELOPMENT
		snprintf(d_log_buffer, sizeof(d_log_buffer), "HeapBuffer::add(%p, %zu): offset=%zu, size=%zu, l_RC=%d", pData, pSize, offset, size, l_RC);
		d_log.write(d_log_buffer, txp::Log::ERROR);
#endif
	}

	return l_RC;
}


void txp::HeapBuffer::calculateCRC_Value(unsigned long &pCRC, const size_t pOffset, const size_t pLength) {
	pCRC = Crc32n(pCRC, (unsigned char*)(buffer+pOffset), pLength);

    return;
}


int txp::HeapBuffer::checkDataAlignment(const size_t pDataAlignment) {
	int l_RC = 1;
	if (pDataAlignment>1 && validateDataAlignment(pDataAlignment)) {
		if ((*((uint64_t*)(void*)buffer))%pDataAlignment != 0) {
			l_RC = 0;
		}
	}

	return l_RC;
}


int txp::HeapBuffer::checkMsgHeaderCRC_Value(const unsigned long &pCRC) {
	int l_RC = 1;
	unsigned long l_CRC = 0;

#ifdef TXP_DEVELOPMENT
//	txp::Log::dump_buffer_raw(d_log, buffer, LENGTH_OF_MSG_HEADER, "Data buffer in checkMsgHeaderCRC_Value()");
#endif
	calculateCRC_Value(l_CRC, 0, LENGTH_OF_MSG_HEADER);
//	printf("checkMsgHeaderCRC_Value: pCRC = 0x%016lX, l_CRC = 0x%016lX\n", pCRC, l_CRC);
	if (l_CRC != pCRC) {
		l_RC = 0;
	}

	return l_RC;
}


void txp::HeapBuffer::dump(Log& pLog, const char* pPrefix) {
	if (pLog.logLevelCheck(txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL)) {
		START_PREFIX(pPrefix, ">>>>> Start dump for message Heap Buffer:  %s");

		snprintf(d_log_buffer, sizeof(d_log_buffer), "Heap Buffer:          %p", this);
		pLog.write(d_log_buffer, txp::Log::DEBUG, txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL);

		pLog.incrIndent(2);

		snprintf(d_log_buffer, sizeof(d_log_buffer), "Data Allocated:      %s", (isDataAllocated() ? "True" : "False"));
		pLog.write(d_log_buffer, txp::Log::DEBUG, txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL);
		snprintf(d_log_buffer, sizeof(d_log_buffer), "Buffer Size:         %zu", size);
		pLog.write(d_log_buffer, txp::Log::DEBUG, txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL);
		snprintf(d_log_buffer, sizeof(d_log_buffer), "Buffer Ptr:          %p", buffer);
		pLog.write(d_log_buffer, txp::Log::DEBUG, txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL);
		if (offset != MAX_OFFSET) {
			snprintf(d_log_buffer, sizeof(d_log_buffer), "Next Insert Offset:  0x%08X (%zu)", (uint32_t)offset, offset);
			pLog.write(d_log_buffer, txp::Log::DEBUG, txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL);
		}
		snprintf(d_log_buffer, sizeof(d_log_buffer), "Pad Length:          0x%08X (%zu)", (uint32_t)padLength, padLength);
		pLog.write(d_log_buffer, txp::Log::DEBUG, txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL);

		if (buffer) {
			pLog.incrIndent(2);

			snprintf(d_log_buffer, sizeof(d_log_buffer), "Msg Length:        0x%08X (%zu)", (uint32_t)getMsgLength(), getMsgLength());
			pLog.write(d_log_buffer, txp::Log::DEBUG, txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL);

			if (getMsgLength() >= (size_t)(LENGTH_OF_MSG_EYECATCHER + LENGTH_OF_MSG_LENGTH)) {
				dump_buffer_raw(pLog, pPrefix);
			}

			pLog.decrIndent(2);
		}

		pLog.decrIndent(2);

		END_PREFIX(pPrefix, ">>>>>   End dump for message Heap Buffer:  %s", 1);
	}

	return;
}


void txp::HeapBuffer::dump_buffer_raw(Log& pLog, const char* pPrefix) {
	if (buffer) {
		// NOTE:  We pass the entire size of the heap buffer.
		//        dump_buffer_raw() will compress out zeroed memory.
		dump_buffer_raw(pLog, std::min(size, txp::MAXIMUM_DUMP_SIZE), pPrefix);
	}

	return;
}


void txp::HeapBuffer::dump_buffer_raw(Log& pLog, const size_t pSize, const char* pPrefix) {
	if (pLog.logLevelCheck(txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL)) {
		txp::Log::dump_buffer_raw(pLog, buffer, pSize, pPrefix);
	}

	return;
}


void txp::HeapBuffer::dumpMsgAttrs(Log& pLog, const char* pPrefix) {
	if (pLog.logLevelCheck(txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL)) {
		if (buffer) {
			if (size > LENGTH_OF_MSG_HEADER) {
			// NOTE:  We pass the entire size of the heap buffer , minus the header.
			//        dump_buffer_raw() will compress out zeroed memory.
				txp::Log::dump_buffer_raw(pLog, buffer+LENGTH_OF_MSG_HEADER, std::min(size-LENGTH_OF_MSG_HEADER, txp::MAXIMUM_DUMP_SIZE), pPrefix);
			}
		}
	}

	return;
}


ssize_t txp::HeapBuffer::getAttrLength(const size_t pOffset) {
	ssize_t l_Value = txp::LENGTH_OF_ATTR_TYPE + txp::LENGTH_OF_ATTR_NAME;
	txp::AttributeType l_Type = getAttrType(pOffset);
	switch(l_Type) {
		case txp::ATTRIBUTE_TYPE_INVALID:
			//  Should never get here...  @@DLH
#ifdef TXP_DEVELOPMENT
			snprintf(d_log_buffer, sizeof(d_log_buffer), "HeapBuffer::getAttrLength(%zu): l_Type=%d", pOffset, l_Type);
			d_log.write(d_log_buffer, txp::Log::ERROR);
#endif
			break;
		case txp::CHAR:
			l_Value += sizeof(char);
			break;
		case txp::UINT8:
			l_Value += sizeof(uint8_t);
			break;
		case txp::UINT16:
			l_Value += sizeof(uint16_t);
			break;
		case txp::UINT32:
			l_Value += sizeof(uint32_t);
			break;
		case txp::UINT64:
			l_Value += sizeof(uint64_t);
			break;
		case txp::INT8:
			l_Value += sizeof(int8_t);
			break;
		case txp::INT16:
			l_Value += sizeof(int16_t);
			break;
		case txp::INT32:
			l_Value += sizeof(int32_t);
			break;
		case txp::INT64:
			l_Value += sizeof(int64_t);
			break;
		case txp::CHAR_ARRAY:
			l_Value += txp::LENGTH_OF_ATTR_CHAR_ARRAY_LENGTH;
			l_Value += getAttr_char_array_length(pOffset);
			break;
		case txp::ARRAY_OF_CHAR_ARRAYS:
			l_Value += txp::LENGTH_OF_ATTR_ARRAY_OF_CHAR_ARRAYS_NUMBER_OF_ELEMENTS;
			l_Value += txp::LENGTH_OF_ATTR_ARRAY_OF_CHAR_ARRAYS_LENGTH;
			l_Value += getAttr_array_of_char_arrays_length(pOffset);
			break;
		case txp::PTR_CHAR:
			l_Value += sizeof(char*);
			break;
		case txp::PTR_UINT8:
			l_Value += sizeof(uint8_t*);
			break;
		case txp::PTR_UINT16:
			l_Value += sizeof(uint16_t*);
			break;
		case txp::PTR_UINT32:
			l_Value += sizeof(uint32_t*);
			break;
		case txp::PTR_UINT64:
			l_Value += sizeof(uint64_t*);
			break;
		case txp::PTR_INT8:
			l_Value += sizeof(int8_t*);
			break;
		case txp::PTR_INT16:
			l_Value += sizeof(int16_t*);
			break;
		case txp::PTR_INT32:
			l_Value += sizeof(int32_t*);
			break;
		case txp::PTR_INT64:
			l_Value += sizeof(int64_t*);
			break;
		case txp::PTR_CHAR_ARRAY:
			l_Value += sizeof(char*);
			break;
		case txp::PTR_ARRAY_OF_CHAR_ARRAYS:
			l_Value += sizeof(char*);
			break;

		default:
			//  Should never get here...  @@DLH
#ifdef TXP_DEVELOPMENT
			snprintf(d_log_buffer, sizeof(d_log_buffer), "HeapBuffer::getAttrLength(%zu): l_Type=%d", pOffset, l_Type);
			d_log.write(d_log_buffer, txp::Log::ERROR);
#endif
			break;
	};

	return l_Value;
}


// getAddrData()
char* txp::HeapBuffer::getAddrData_char(const size_t pOffset) {
	return (char*)((this->buffer)+pOffset+txp::OFFSET_TO_ATTR_DATA);
}


uint8_t* txp::HeapBuffer::getAddrData_uint8(const size_t pOffset) {
	return (uint8_t*)((this->buffer)+pOffset+txp::OFFSET_TO_ATTR_DATA);
}


uint16_t* txp::HeapBuffer::getAddrData_uint16(const size_t pOffset) {
	return (uint16_t*)((this->buffer)+pOffset+txp::OFFSET_TO_ATTR_DATA);
}


uint32_t* txp::HeapBuffer::getAddrData_uint32(const size_t pOffset) {
	return (uint32_t*)((this->buffer)+pOffset+txp::OFFSET_TO_ATTR_DATA);
}


uint64_t* txp::HeapBuffer::getAddrData_uint64(const size_t pOffset) {
	return (uint64_t*)((this->buffer)+pOffset+txp::OFFSET_TO_ATTR_DATA);
}

int8_t* txp::HeapBuffer::getAddrData_int8(const size_t pOffset) {
	return (int8_t*)((this->buffer)+pOffset+txp::OFFSET_TO_ATTR_DATA);
}

int16_t* txp::HeapBuffer::getAddrData_int16(const size_t pOffset) {
	return (int16_t*)((this->buffer)+pOffset+txp::OFFSET_TO_ATTR_DATA);
}


int32_t* txp::HeapBuffer::getAddrData_int32(const size_t pOffset) {
	return (int32_t*)((this->buffer)+pOffset+txp::OFFSET_TO_ATTR_DATA);
}


int64_t* txp::HeapBuffer::getAddrData_int64(const size_t pOffset) {
	return (int64_t*)((this->buffer)+pOffset+txp::OFFSET_TO_ATTR_DATA);
}


size_t* txp::HeapBuffer::getAddrData_sizet(const size_t pOffset) {
	return (size_t*)((this->buffer)+pOffset+txp::OFFSET_TO_ATTR_DATA);
}


size_t* txp::HeapBuffer::getAddrData_char_array_length(const size_t pOffset) {
	return (size_t*)((this->buffer)+pOffset+txp::OFFSET_TO_ATTR_CHAR_ARRAY_LENGTH);
}


char* txp::HeapBuffer::getAddrData_char_array_data(const size_t pOffset) {
	return (char*)((this->buffer)+pOffset+txp::OFFSET_TO_ATTR_CHAR_ARRAY_DATA);
}


size_t* txp::HeapBuffer::getAddrData_array_of_char_arrays_number_of_elements(const size_t pOffset) {
	return (size_t*)((this->buffer)+pOffset+txp::OFFSET_TO_ATTR_ARRAY_OF_CHAR_ARRAYS_NUMBER_OF_ELEMENTS);
}


size_t* txp::HeapBuffer::getAddrData_array_of_char_arrays_length(const size_t pOffset) {
	return (size_t*)((this->buffer)+pOffset+txp::OFFSET_TO_ATTR_ARRAY_OF_CHAR_ARRAYS_LENGTH);
}


char* txp::HeapBuffer::getAddrData_array_of_char_arrays_data(const size_t pOffset) {
	return (char*)((this->buffer)+pOffset+txp::OFFSET_TO_ATTR_ARRAY_OF_CHAR_ARRAYS_DATA);
}


size_t* txp::HeapBuffer::getAddrData_array_of_char_arrays_element_size(const size_t pOffset) {
	return (size_t*)((this->buffer)+pOffset+txp::OFFSET_TO_ATTR_ARRAY_OF_CHAR_ARRAYS_ELEMENT_SIZE);
}


char* txp::HeapBuffer::getAddrData_array_of_char_arrays_element_data(const size_t pOffset) {
	return (char*)((this->buffer)+pOffset+txp::OFFSET_TO_ATTR_ARRAY_OF_CHAR_ARRAYS_ELEMENT_DATA);
}


// get()
char txp::HeapBuffer::getAttr_char(const size_t pOffset) {
	return *(getAddrData_char(pOffset));
}


uint8_t txp::HeapBuffer::getAttr_uint8(const size_t pOffset) {
	return *(getAddrData_uint8(pOffset));
}


uint16_t txp::HeapBuffer::getAttr_uint16(const size_t pOffset) {
	return *(getAddrData_uint16(pOffset));
}


uint32_t txp::HeapBuffer::getAttr_uint32(const size_t pOffset) {
	return *(getAddrData_uint32(pOffset));
}


uint64_t txp::HeapBuffer::getAttr_uint64(const size_t pOffset) {
	return *(getAddrData_uint64(pOffset));
}


int8_t txp::HeapBuffer::getAttr_int8(const size_t pOffset) {
	return *(getAddrData_int8(pOffset));
}


int16_t txp::HeapBuffer::getAttr_int16(const size_t pOffset) {
	return *(getAddrData_int16(pOffset));
}


int32_t txp::HeapBuffer::getAttr_int32(const size_t pOffset) {
	return *(getAddrData_int32(pOffset));
}


int64_t txp::HeapBuffer::getAttr_int64(const size_t pOffset) {
	return *(getAddrData_int64(pOffset));
}


size_t txp::HeapBuffer::getAttr_char_array_length(const size_t pOffset) {
	return *(getAddrData_char_array_length(pOffset));
}


/*
char* txp::HeapBuffer::getAttr_char_array_data(const size_t pOffset) {
	return getAddrData_char_array_data(pOffset);
}
*/


size_t txp::HeapBuffer::getAttr_array_of_char_arrays_number_of_elements(const size_t pOffset) {
	return *(getAddrData_array_of_char_arrays_number_of_elements(pOffset));
}


size_t txp::HeapBuffer::getAttr_array_of_char_arrays_length(const size_t pOffset) {
	return *(getAddrData_array_of_char_arrays_length(pOffset));
}


/*
char txp::HeapBuffer::getAttr_array_of_char_arrays_data(const size_t pOffset) {
	return *(getAddrData_array_of_char_arrays_data(pOffset));
}
*/


size_t txp::HeapBuffer::getAttr_array_of_char_arrays_element_size(const size_t pOffset) {
	return *(getAddrData_array_of_char_arrays_element_size(pOffset));
}


char* txp::HeapBuffer::getAttr_array_of_char_arrays_element_data(const size_t pOffset) {
	return getAddrData_array_of_char_arrays_element_data(pOffset);
}


char* txp::HeapBuffer::getAttrPtr_char(const size_t pOffset) {
	return getAddrData_char(pOffset);
}


uint8_t* txp::HeapBuffer::getAttrPtr_uint8(const size_t pOffset) {
	return getAddrData_uint8(pOffset);
}


uint16_t* txp::HeapBuffer::getAttrPtr_uint16(const size_t pOffset) {
	return getAddrData_uint16(pOffset);
}


uint32_t* txp::HeapBuffer::getAttrPtr_uint32(const size_t pOffset) {
	return getAddrData_uint32(pOffset);
}


uint64_t* txp::HeapBuffer::getAttrPtr_uint64(const size_t pOffset) {
	return getAddrData_uint64(pOffset);
}


int8_t* txp::HeapBuffer::getAttrPtr_int8(const size_t pOffset) {
	return getAddrData_int8(pOffset);
}


int16_t* txp::HeapBuffer::getAttrPtr_int16(const size_t pOffset) {
	return getAddrData_int16(pOffset);
}


int32_t* txp::HeapBuffer::getAttrPtr_int32(const size_t pOffset) {
	return getAddrData_int32(pOffset);
}


int64_t* txp::HeapBuffer::getAttrPtr_int64(const size_t pOffset) {
	return getAddrData_int64(pOffset);
}


char* txp::HeapBuffer::getAttrPtr_char_array_data(const size_t pOffset) {
	return getAddrData_char_array_data(pOffset);
}


char* txp::HeapBuffer::getAttrPtr_array_of_char_arrays_data(const size_t pOffset) {
	return getAddrData_array_of_char_arrays_data(pOffset);
}


char* txp::HeapBuffer::getAttrPtr_array_of_char_arrays_element_data(const size_t pOffset) {
	return getAddrData_array_of_char_arrays_element_data(pOffset);
}


// getAttrType()
txp::AttributeType txp::HeapBuffer::getAttrType(const size_t pOffset) {
	return txp::AttributeType(*((uint16_t*)((this->buffer)+pOffset+OFFSET_TO_ATTR_TYPE)));
}


// getAttrName()
txp::AttributeName txp::HeapBuffer::getAttrName(const size_t pOffset) {
	return txp::AttributeName(*((uint16_t*)((this->buffer)+pOffset+OFFSET_TO_ATTR_NAME)));
}


// getAndZeroMsgCRC_Value()
unsigned long txp::HeapBuffer::getAndZeroMsgCRC_Value() {
	unsigned long l_CRC = *((unsigned long*)((this->buffer)+OFFSET_TO_MSG_CRC_VALUE));
	memset((void*)((this->buffer)+OFFSET_TO_MSG_CRC_VALUE), 0, LENGTH_OF_MSG_CRC_VALUE);

	return l_CRC;
}


// getAndZeroMsgHeaderCRC_Value()
unsigned long txp::HeapBuffer::getAndZeroMsgHeaderCRC_Value() {
	unsigned long l_CRC = *((unsigned long*)((this->buffer)+OFFSET_TO_MSG_HEADER_CRC_VALUE));
	memset((void*)((this->buffer)+OFFSET_TO_MSG_HEADER_CRC_VALUE), 0, LENGTH_OF_MSG_HEADER_CRC_VALUE);

	return l_CRC;
}


// getLengthOfPad()
size_t txp::HeapBuffer::getLengthOfPad() {
	return (size_t)(*((uint8_t*)((this->buffer)+OFFSET_TO_PAD_VALUE)));
}


// getMsgCreatorPid()
pid_t txp::HeapBuffer::getMsgCreatorPid() {
	return *((pid_t*)((this->buffer)+OFFSET_TO_MSG_CREATOR_PID));
}


// getMsgCreatorTid()
pthread_t txp::HeapBuffer::getMsgCreatorTid() {
	return *((pthread_t*)((this->buffer)+OFFSET_TO_MSG_CREATOR_TID));
}


// getMsgEyeCatcher()
uint32_t txp::HeapBuffer::getMsgEyeCatcher() {
	return *((uint32_t*)((this->buffer)+OFFSET_TO_MSG_EYECATCHER));
}


// getMsgId()
txp::Id txp::HeapBuffer::getMsgId() {
	return txp::Id(*((uint32_t*)((this->buffer)+OFFSET_TO_MSG_ID)));
}


// getMsgLength()
size_t txp::HeapBuffer::getMsgLength() {
	return *((size_t*)((this->buffer)+OFFSET_TO_MSG_LENGTH));
}


// getMsgNumber()
int32_t txp::HeapBuffer::getMsgNumber() {
	return *((int32_t*)((this->buffer)+OFFSET_TO_MSG_NUMBER));
}


// getMsgRequestorPid()
pid_t txp::HeapBuffer::getMsgRequestorPid() {
	return *((pid_t*)((this->buffer)+OFFSET_TO_MSG_REQUESTOR_PID));
}


// getMsgRequestorTid()
pthread_t txp::HeapBuffer::getMsgRequestorTid() {
	return *((pthread_t*)((this->buffer)+OFFSET_TO_MSG_REQUESTOR_TID));
}


// getMsgState()
uint16_t txp::HeapBuffer::getMsgState() {
	return *((uint16_t*)((this->buffer)+OFFSET_TO_MSG_STATE));
}


// getMsgVersion()
uint16_t txp::HeapBuffer::getMsgVersion() {
	return *((uint16_t*)((this->buffer)+OFFSET_TO_MSG_VERSION));
}


// getNextMsgNumber()
int32_t txp::HeapBuffer::getNextMsgNumber() {
	return *((int32_t*)((this->buffer)+OFFSET_TO_NEXT_MSG_NUMBER));
}


// getPrevMsgNumber()
int32_t txp::HeapBuffer::getPrevMsgNumber() {
	return *((int32_t*)((this->buffer)+OFFSET_TO_PREV_MSG_NUMBER));
}


// getRequestMsgNumber()
int32_t txp::HeapBuffer::getRequestMsgNumber() {
	return *((int32_t*)((this->buffer)+OFFSET_TO_REQUEST_MSG_NUMBER));
}


// padToDataAlignment()
int txp::HeapBuffer::padToDataAlignment(const size_t pDataAlignment) {
	int l_RC = 0;

	if (pDataAlignment>1 && validateDataAlignment(pDataAlignment)) {
		size_t l_NewSize = calculatePad(offset, pDataAlignment);
		if (l_NewSize <= size) {
			if (l_NewSize > offset) {
				memset((void*)((this->buffer)+offset), 0, l_NewSize-offset);
				offset = l_NewSize;
                setMsgLength(offset);
			}
		} else {
#ifdef TXP_DEVELOPMENT
			snprintf(d_log_buffer, sizeof(d_log_buffer), "HeapBuffer::padToDataAlignment(%zu): l_RC=%d, offset=%zu, size=%zu, l_NewSize=%zu", pDataAlignment, l_RC, offset, size, l_NewSize);
			d_log.write(d_log_buffer, txp::Log::ERROR);
#endif
			l_RC = -304;
		}
	}

	return l_RC;
}


// reset()
void txp::HeapBuffer::reset() {
	*((uint32_t*)(buffer + OFFSET_TO_MSG_EYECATCHER)) = EYECATCHER;
	*((uint32_t*)(buffer + OFFSET_TO_MSG_RESERVED_1)) = 0;
	offset = LENGTH_OF_MSG_EYECATCHER + LENGTH_OF_MSG_NUMBER + LENGTH_OF_MSG_LENGTH;
	setMsgLength(offset);
	padLength = 0;

	return;
}


// resize()
int txp::HeapBuffer::resize(const size_t pSize) {
	int l_RC = 0;

#ifdef TXP_DEVELOPMENT
	size_t l_CurrentSize = size;
#endif

	// NOTE:  Currently, only HeapBuffers that have their space
	//        allocated by the client can be resized...  @@DLH
	if (!isDataAllocated()) {
		if (offset < pSize && pSize >= OFFSET_TO_FIRST_ATTRIBUTE) {
			size = pSize;
		} else {
			l_RC = -303;
		}
	} else {
		l_RC = -302;
	}

#ifdef TXP_DEVELOPMENT
	if (l_RC) {
		snprintf(d_log_buffer, sizeof(d_log_buffer), "HeapBuffer::resize(%zu): RC=%d, l_CurrentSize=%zu, offset=%zu", pSize, l_RC, l_CurrentSize, offset);
		d_log.write(d_log_buffer, txp::Log::ERROR);
	}
#endif

	return l_RC;
}
