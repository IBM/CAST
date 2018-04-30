/*******************************************************************************
 |    HeapBuffer.h
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


/*
 * \file  HeapBuffer.h
 * \brief Header file for the message HeapBuffer() class.
 *
 * \ingroup transport
 */

#ifndef HEAPBUFFER_H_
#define HEAPBUFFER_H_

#include "limits.h"
#include "stdint.h"
#include "stdlib.h"

#include "Common.h"

#include "Log.h"

namespace txp {

/**
 * \class HeapBuffer
 * Allocation of heap storage.
 *
 * Aids in the serialization of a Msg() object into contiguous storage.
 */
class HeapBuffer {
public:
    friend class Attribute;
    friend class Attr_char;
    friend class Attr_uint8;
    friend class Attr_uint16;
    friend class Attr_uint32;
    friend class Attr_uint64;
    friend class Attr_int8;
    friend class Attr_int16;
    friend class Attr_int32;
    friend class Attr_int64;
    friend class AttrPtr_char;
    friend class AttrPtr_uint8;
    friend class AttrPtr_uint16;
    friend class AttrPtr_uint32;
    friend class AttrPtr_uint64;
    friend class AttrPtr_int8;
    friend class AttrPtr_int16;
    friend class AttrPtr_int32;
    friend class AttrPtr_int64;
    friend class AttrPtr_char_array;
    friend class AttrPtr_array_of_char_arrays;
    friend class Msg;

	// Constants
	static const size_t MAX_OFFSET = INT_MAX;
	static const size_t MAX_DATA_ALIGNMENT_VALUE = 64;

    // Static data
    static txp::Log d_log;
    static char d_log_buffer[255];

    // Static methods

    /**
     * \brief Calculate pad.
     *
     * \param[in]   pLength         Length of data.
     * \param[in]   pDataAlignment  Data alignment value.
     * \return      size_t  Pad value
     *
     */
	static size_t calculatePad(const size_t pLength, const size_t pDataAlignment);

    /**
     * \brief Validate data alignment value.
     *
     * \param[in]   pDataAlignment  Data alignment value.
     * \return      int  1 -> success;  0 -> failure
     *
     */
	static int validateDataAlignment(const size_t pDataAlignment);

	// Inlined non-static methods

    /**
     * \brief Get Pad Length.
     *
     * \return      size_t Pad length
     */
	inline size_t getPadLength() { return padLength; };

    /**
     * \brief Returns the size of the buffer.
     *
     * \return      size_t Size
     */
	inline size_t getSize() { return size; };

	// Non-static methods

    /**
     * \brief Add the input data for the input length to the heap buffer.
     *
     * \param[in]   pData Pointer to data
     * \param[in]   pSize Size of data
     * \return      int  0 -> success;  <0 -> failure
     * \par Error codes
     * -301 Size of item to be added does not fit within the size for the heap buffer.
     */
	int add(void* pData, size_t pSize);

    /**
     * \brief Calculate CRC value from heap buffer.
     *
     * \param[inout]    pCRC Value CRC value
     * \param[in]       pOffset Offset in the heap buffer
     * \param[in]       pLength Length of data
     */
    void calculateCRC_Value(unsigned long &pCRC, const size_t pOffset, const size_t pLength);

    /**
     * \brief Check Data Alignment.
     *
     * \param[in]   pDataAlignment  Data alignment value
     * \return      int  0 -> Data buffer is NOT aligned;  1 -> Data buffer is aligned
     * \par Error codes
     */
    int checkDataAlignment(const size_t pDataAlignment);

    /**
     * \brief Check Message Header CRC Value.
     *
     * \param[in]   pCRC_Value CRC value to compare agianst the calculated value.
     * \return      int  0 -> success;  <0 -> failure
     * \par Error codes
     */
    int checkMsgHeaderCRC_Value(const unsigned long &pCRC_Value);

    /**
     * \brief Dumps heap buffer as hex to a Log.
     *
     * \param[in]   pLog
     * \param[in]   pPrefix text to identify this operation
     */
	void dump_buffer_raw(Log& pLog, const char* pPrefix=0);

    /**
     * \brief Dumps heap buffer as hex to a Log, for a given size.
     *
     * \param[in]   pLog
     * \param[in]   pSize Size to dump
     * \param[in]   pPrefix text to identify this operation
     */
	void dump_buffer_raw(Log& pLog, const size_t pSize, const char* pPrefix=0);

    /**
     * \brief Dumps heap buffer to a Log.
     *
     * \param[in]   pLog
     * \param[in]   pPrefix Text to identify this operation.  Default is no prefix data.
     */
	void dump(Log& pLog, const char* pPrefix=0);

    /**
     * \brief Dumps message attributes from heap buffer to a Log.
     *
     * \param[in]   pLog
     * \param[in]   pPrefix Text to identify this operation.  Default is no prefix data.
     */
	void dumpMsgAttrs(Log& pLog, const char* pPrefix=0);

    /**
     * \brief Returns a pointer to the real data buffer.
     *
     * \return      char* Pointer to data buffer
     */
    char* getDataBufferPtr() { return buffer; };

    /**
     * \brief Pad to Data Alignment.
     *
     * \param[in]   pDataAlignment  Data alignment value
     * \return      int  0 -> success;  <0 -> failure
     * \par Error codes
     * -304 Failure when attempting to pad to data alignment value
     */
    int padToDataAlignment(const size_t pDataAlignment);

    /**
     * \brief Resets a heap buffer.
     * This sets the eyecatcher and length values to start serializing the header
     * into the buffer.
     */
	void reset();

    /**
     * \brief Resize heap buffer.
     *
     * \param[in]   pSize New buffer size
     * \return      int  0 -> success;  <0 -> failure
     * \par Error codes
     * -302 Heap buffer was allocated by the facility.\n
     * -303 New size if less then the size of a Msg() header.
     * \note This method is only supported for heap buffers
     *       constructed over a client created buffer.  Resizing
     *       the heap buffer is for bookkeeping purposes only.
     */
    int resize(const size_t pSize);

	// Non-static methods - get data members from the header

    /**
     * \brief Get and Zero Msg CRC Value.
     *
     * \return      unsigned long Message CRC value
     */
	unsigned long getAndZeroMsgCRC_Value();

    /**
     * \brief Get and Zero Msg Header CRC Value.
     *
     * \return      unsigned long Message Header CRC value
     */
	unsigned long getAndZeroMsgHeaderCRC_Value();

    /**
     * \brief Get Length of Pad.
     *
     * \return      size_t Pad length
     */
	size_t getLengthOfPad();

    /**
     * \brief Get Creator pid value.
     *
     * \return      pid_t Creator pid value
     */
	pid_t getMsgCreatorPid();

    /**
     * \brief Get Creator tid value.
     *
     * \return      pid_t Creator tid value
     */
	pthread_t getMsgCreatorTid();

    /**
     * \brief Get Eyecatcher.
     *
     * \return      uint32_t Eyecatcher value as a magic number
     */
	uint32_t getMsgEyeCatcher();

    /**
     * \brief Get Msg Id.
     *
     * \return      txp::Id Message Id
     */
	txp::Id getMsgId();

    /**
     * \brief Get Msg Length.
     *
     * \return      size_t Message length
     */
	size_t getMsgLength();

    /**
     * \brief Get Msg Number.
     *
     * \return      int32_t Message number
     */
	int32_t getMsgNumber();

    /**
     * \brief Get Msg State.
     *
     * \return      int16_t Message state
     */
	uint16_t getMsgState();

    /**
     * \brief Get Msg Version.
     *
     * \return      int16_t Message version
     */
	uint16_t getMsgVersion();

    /**
     * \brief Get Next Msg Number.
     *
     * \return      int32_t Next message number
     * \note Returns zero if there is no next message.
     */
	int32_t getNextMsgNumber();

    /**
     * \brief Get Previous Msg Number.
     *
     * \return      int32_t Previous message number
     * \note Returns zero if there is no previous message.
     */
	int32_t getPrevMsgNumber();

    /**
     * \brief Get Request Msg Number.
     *
     * \return      int32_t Request message number
     * \note Returns zero if this not a response message.
     */
	int32_t getRequestMsgNumber();

    /**
     * \brief Get Requestor pid value.
     *
     * \return      pid_t Requestor pid value
     */
	pid_t getMsgRequestorPid();

    /**
     * \brief Get Requestor tid value.
     *
     * \return      pid_t Requestor tid value
     */
	pthread_t getMsgRequestorTid();


	// Constructors

    /**
     * \brief Default constructor
     *
     * Allocates a buffer with a size of txp::DEFAULT_BUFFER_SIZE.
     */
	HeapBuffer();

    /**
     * \brief Constructor specifying size
     *
     * Allocates a buffer with the specified size.
     * \param[in]   pSize Buffer size
     */
	HeapBuffer(const size_t pSize);

    /**
     * \brief Constructor specifying existing buffer and size
     *
     * Defines a heap buffer over the input existing buffer,
     * using the specified input size.
     * \param[in]   pBuffer Pointer to the existing buffer
     * \param[in]   pSize Buffer size
     */
	HeapBuffer(char* pBuffer, const size_t pSize);

    /**
     * \brief Destructor
     */
	virtual ~HeapBuffer();

private:
	// Inlined non-static methods

    /**
     * \brief Returns the offset to the next free location within the buffer.
     *
     * \return      size_t Offset
     */
	inline size_t getOffset() { return offset; };

	// Non-static methods - get the address of the data for an attribute

    /**
     * \brief Return address to char data
     *
     * \param[in]   pOffset Offset within buffer
     * \return      char* Pointer to data
     */
	char* getAddrData_char(const size_t pOffset);

    /**
     * \brief Return address to uint8_t data
     *
     * \param[in]   pOffset Offset within buffer
     * \return      uint8_t* Pointer to data
     */
	uint8_t* getAddrData_uint8(const size_t pOffset);

    /**
     * \brief Return address to uint16_t data
     *
     * \param[in]   pOffset Offset within buffer
     * \return      uint16_t* Pointer to data
     */
	uint16_t* getAddrData_uint16(const size_t pOffset);

    /**
     * \brief Return address to uint32_t data
     *
     * \param[in]   pOffset Offset within buffer
     * \return      uint32_t* Pointer to data
     */
	uint32_t* getAddrData_uint32(const size_t pOffset);

    /**
     * \brief Return address to uint64_t data
     *
     * \param[in]   pOffset Offset within buffer
     * \return      uint64_t* Pointer to data
     */
	uint64_t* getAddrData_uint64(const size_t pOffset);

    /**
     * \brief Return address to int8_t data
     *
     * \param[in]   pOffset Offset within buffer
     * \return      int8_t* Pointer to data
     */
	int8_t* getAddrData_int8(const size_t pOffset);

    /**
     * \brief Return address to int16_t data
     *
     * \param[in]   pOffset Offset within buffer
     * \return      int16_t* Pointer to data
     */
	int16_t* getAddrData_int16(const size_t pOffset);

    /**
     * \brief Return address to int32_t data
     *
     * \param[in]   pOffset Offset within buffer
     * \return      int32_t* Pointer to data
     */
	int32_t* getAddrData_int32(const size_t pOffset);

    /**
     * \brief Return address to int64_t data
     *
     * \param[in]   pOffset Offset within buffer
     * \return      int64_t* Pointer to data
     */
	int64_t* getAddrData_int64(const size_t pOffset);

    /**
     * \brief Return address to size_t data
     *
     * \param[in]   pOffset Offset within buffer
     * \return      size_t* Pointer to data
     */
	size_t* getAddrData_sizet(const size_t pOffset);

    /**
     * \brief Return address to char array length
     *
     * \param[in]   pOffset Offset within buffer
     * \return      size_t* Pointer to data
     */
	size_t* getAddrData_char_array_length(const size_t pOffset);

    /**
     * \brief Return address to char array data
     *
     * \param[in]   pOffset Offset within buffer
     * \return      char* Pointer to data
     */
	char* getAddrData_char_array_data(const size_t pOffset);

    /**
     * \brief Return address to array of char arrays number of elements
     *
     * \param[in]   pOffset Offset within buffer
     * \return      size_t* Pointer to data
     */
	size_t* getAddrData_array_of_char_arrays_number_of_elements(const size_t pOffset);

    /**
     * \brief Return address to array of char arrays length
     *
     * \param[in]   pOffset Offset within buffer
     * \return      size_t* Pointer to length
     */
	size_t* getAddrData_array_of_char_arrays_length(const size_t pOffset);

    /**
     * \brief Return address to array of char arrays data
     *
     * \param[in]   pOffset Offset within buffer
     * \return      char* Pointer to data
     */
	char* getAddrData_array_of_char_arrays_data(const size_t pOffset);

    /**
     * \brief Return address to array of char arrays element size
     *
     * \param[in]   pOffset Offset within buffer
     * \return      size_t* Pointer to element size
     */
	size_t* getAddrData_array_of_char_arrays_element_size(const size_t pOffset);

    /**
     * \brief Return address to array of char arrays element data
     *
     * \param[in]   pOffset Offset within buffer
     * \return      char*   Pointer to data
     */
	char* getAddrData_array_of_char_arrays_element_data(const size_t pOffset);

	// Non-static methods - get data members from an attribute

    /**
     * \brief Return attribute length
     *
     * \param[in]   pOffset Offset within buffer
     * \return      ssize_t Attribute length
     */
	ssize_t getAttrLength(const size_t pOffset);

    /**
     * \brief Return attribute type
     *
     * \param[in]   pOffset Offset within buffer
     * \return      txp::AttributeType Attribute type
     */
	txp::AttributeType getAttrType(const size_t pOffset);

    /**
     * \brief Return attribute name
     *
     * \param[in]   pOffset Offset within buffer
     * \return      txp::AttributeName Attribute name
     */
	txp::AttributeName getAttrName(const size_t pOffset);

	// Non-static methods - get attribute(s)

    /**
     * \brief Return char data
     *
     * \param[in]   pOffset Offset within buffer
     * \return      char Data
     */
	char getAttr_char(const size_t pOffset);

    /**
     * \brief Return uint8_t data
     *
     * \param[in]   pOffset Offset within buffer
     * \return      uint8_t Data
     */
	uint8_t getAttr_uint8(const size_t pOffset);

    /**
     * \brief Return uint16_t data
     *
     * \param[in]   pOffset Offset within buffer
     * \return      uint16_t Data
     */
	uint16_t getAttr_uint16(const size_t pOffset);

    /**
     * \brief Return uint32_t data
     *
     * \param[in]   pOffset Offset within buffer
     * \return      uint32_t Data
     */
	uint32_t getAttr_uint32(const size_t pOffset);

    /**
     * \brief Return uint64_t data
     *
     * \param[in]   pOffset Offset within buffer
     * \return      uint64_t Data
     */
	uint64_t getAttr_uint64(const size_t pOffset);

    /**
     * \brief Return int8_t data
     *
     * \param[in]   pOffset Offset within buffer
     * \return      int8_t Data
     */
	int8_t getAttr_int8(const size_t pOffset);

    /**
     * \brief Return int16_t data
     *
     * \param[in]   pOffset Offset within buffer
     * \return      int16_t Data
     */
	int16_t getAttr_int16(const size_t pOffset);

    /**
     * \brief Return int32_t data
     *
     * \param[in]   pOffset Offset within buffer
     * \return      int32_t Data
     */
	int32_t getAttr_int32(const size_t pOffset);

    /**
     * \brief Return int64_t data
     *
     * \param[in]   pOffset Offset within buffer
     * \return      int64_t Data
     */
	int64_t getAttr_int64(const size_t pOffset);

    /**
     * \brief Return char array length
     *
     * \param[in]   pOffset Offset within buffer
     * \return      size_t Char array length
     */
	size_t getAttr_char_array_length(const size_t pOffset);

    /**
     * \brief Return pointer to char array data
     *
     * \param[in]   pOffset Offset within buffer
     * \return      char* Char data
     */
//	char* getAttr_char_array_data(const size_t pOffset);

    /**
     * \brief Return array of char arrays number of elements
     *
     * \param[in]   pOffset Offset within buffer
     * \return      size_t  Char array length
     */
	size_t getAttr_array_of_char_arrays_number_of_elements(const size_t pOffset);

    /**
     * \brief Return array of char arrays length
     *
     * \param[in]   pOffset Offset within buffer
     * \return      size_t  Char array length
     */
	size_t getAttr_array_of_char_arrays_length(const size_t pOffset);

    /**
     * \brief Return pointer to array of char array data
     *
     * \param[in]   pOffset Offset within buffer
     * \return      size_t  Offset to array of char array data
     */
//	size_t getAttr_array_of_char_arrays_data(const size_t pOffset);

    /**
     * \brief Return array of char arrays element size
     *
     * \param[in]   pOffset Offset within buffer
     * \return      size_t  Char array length
     */
	size_t getAttr_array_of_char_arrays_element_size(const size_t pOffset);

    /**
     * \brief Return pointer to array of char arrays element data
     *
     * \param[in]   pOffset Offset within buffer
     * \return      char*   Char data
     */
	char* getAttr_array_of_char_arrays_element_data(const size_t pOffset);

    /**
     * \brief Return pointer to char data
     *
     * \param[in]   pOffset Offset within buffer
     * \return      char* Data
     */
	char* getAttrPtr_char(const size_t pOffset);

    /**
     * \brief Return pointer to uint8_t data
     *
     * \param[in]   pOffset Offset within buffer
     * \return      uint8_t* Data
     */
	uint8_t* getAttrPtr_uint8(const size_t pOffset);

    /**
     * \brief Return pointer to uint16_t data
     *
     * \param[in]   pOffset Offset within buffer
     * \return      uint16_t* Data
     */
	uint16_t* getAttrPtr_uint16(const size_t pOffset);

    /**
     * \brief Return pointer to uint32_t data
     *
     * \param[in]   pOffset Offset within buffer
     * \return      uint32_t* Data
     */
	uint32_t* getAttrPtr_uint32(const size_t pOffset);

    /**
     * \brief Return pointer to uint64_t data
     *
     * \param[in]   pOffset Offset within buffer
     * \return      uint64_t* Data
     */
	uint64_t* getAttrPtr_uint64(const size_t pOffset);

    /**
     * \brief Return pointer to int8_t data
     *
     * \param[in]   pOffset Offset within buffer
     * \return      int8_t* Data
     */
	int8_t* getAttrPtr_int8(const size_t pOffset);

    /**
     * \brief Return pointer to int16_t data
     *
     * \param[in]   pOffset Offset within buffer
     * \return      int16_t* Data
     */
	int16_t* getAttrPtr_int16(const size_t pOffset);

    /**
     * \brief Return pointer to int32_t data
     *
     * \param[in]   pOffset Offset within buffer
     * \return      int32_t* Data
     */
	int32_t* getAttrPtr_int32(const size_t pOffset);

    /**
     * \brief Return pointer to int64_t data
     *
     * \param[in]   pOffset Offset within buffer
     * \return      int64_t* Data
     */
	int64_t* getAttrPtr_int64(const size_t pOffset);

    /**
     * \brief Return pointer to char array data
     *
     * \param[in]   pOffset Offset within buffer
     * \return      char* Data
     */
	char* getAttrPtr_char_array_data(const size_t pOffset);

    /**
     * \brief Return pointer to array of char arrays data
     *
     * \param[in]   pOffset Offset within buffer
     * \return      char* Data
     */
	char* getAttrPtr_array_of_char_arrays_data(const size_t pOffset);

    /**
     * \brief Return pointer to array of char arrays element data
     *
     * \param[in]   pOffset Offset within buffer
     * \return      char*   Char data
     */
	char* getAttrPtr_array_of_char_arrays_element_data(const size_t pOffset);

    // Inlined non-static MsgLength methods

    /**
     * \brief Get Msg CRC Value.
     *
     * \return      unsigned long Message CRC value
     */
	inline unsigned long getMsgCRC_Value() { return *((unsigned long*)((this->buffer)+OFFSET_TO_MSG_CRC_VALUE)); };

    /**
     * \brief Get Msg Header CRC Value.
     *
     * \return      unsigned long Message Header CRC value
     */
	inline unsigned long getMsgHeaderCRC_Value() { return *((unsigned long*)((this->buffer)+OFFSET_TO_MSG_HEADER_CRC_VALUE)); };

    /**
     * \brief Increment message length in buffer
     *
     * \param[in]   pLength Bump length
     */
	inline void incrMsgLength(const size_t pLength) { *((size_t*)(buffer + OFFSET_TO_MSG_LENGTH)) += pLength; };

    /**
     * \brief Is Data Allocated by Facility
     *
     * \return      int  0 -> not allocated;  1 -> allocated
     */
    inline int isDataAllocated() { return heapBufferFlag.dataAllocated; };

    /**
     * \brief Set Data Allocated Flag.
     *
     * \param[in]   pValue 0 -> not allocated;  1 -> allocated
     */
    inline void setDataAllocatedFlag(const int pValue) { heapBufferFlag.dataAllocated = pValue; return; };

    /**
     * \brief Set message length in buffer
     *
     * \param[in]   pLength Message length
     */
	inline void setMsgLength(const size_t pLength) { *((size_t*)(buffer + OFFSET_TO_MSG_LENGTH)) = pLength; };

    /**
     * \brief Set message CRC value
     *
     * \param[in]   pCRC CRC value
     */
//	inline void setMsgCRC_Value(const unsigned long pCRC) { printf("setMsgCRC_Value: pCRC=0x%016lX\n", pCRC); *((unsigned long*)(buffer + OFFSET_TO_MSG_CRC_VALUE)) = pCRC; };
	inline void setMsgCRC_Value(const unsigned long pCRC) { *((unsigned long*)(buffer + OFFSET_TO_MSG_CRC_VALUE)) = pCRC; };

    /**
     * \brief Set message header CRC value
     *
     * \param[in]   pCRC CRC value
     */
//	inline void setMsgHeaderCRC_Value(const unsigned long pCRC) { printf("setMsgHeaderCRC_Value: pCRC=0x%016lX\n", pCRC); *((unsigned long*)(buffer + OFFSET_TO_MSG_HEADER_CRC_VALUE)) = pCRC; };
	inline void setMsgHeaderCRC_Value(const unsigned long pCRC) { *((unsigned long*)(buffer + OFFSET_TO_MSG_HEADER_CRC_VALUE)) = pCRC; };

	// Data members
	union {
	    uint16_t    flags;
		struct {
			uint8_t	reserved_1:8;

			uint8_t	reserved_2:7;
		    uint8_t	dataAllocated:1;//! Data buffer was allocated by the message facility,
			                        //! and therefore, will be deleted by the message
			                        //! facility
		};
	}  heapBufferFlag;
	size_t          size;           //! Size of buffer
	size_t          offset;         //! Next available offset within buffer.
	                                //! \note Only valid for buffers where data is being
	                                //!       serialized into the buffer.  Otherwise, this
	                                //!       offset value is set to 0xFFFFFFFF.
	size_t          padLength;      //! Length of pad.
	char*           buffer;         //! Pointer to the buffer
};

} // namespace

#endif /* HEAPBUFFER_H_ */
