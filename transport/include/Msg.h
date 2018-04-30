/*******************************************************************************
 |    Msg.h
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
 * \file  Msg.h
 * \brief Header file for the Msg() class.
 *
 * \ingroup transport
 */

#ifndef MSG_H_
#define MSG_H_

#include <map>
#include <vector>

#include <string.h>
#include <pthread.h>

#include "stdint.h"
#include "stdlib.h"

#include "Common.h"

#include "Attribute.h"
#include "HeapBuffer.h"
#include "Log.h"
#include "State.h"
#include "Version.h"
#include "util.h"

#include <sys/uio.h>

//#define MSG_STALE_CHECK 1

namespace txp {


#if 0
// A noop that you can set a debug stop on...
#define noop my_noop();

static void my_noop() { return; };
#endif

inline int isBB_MsgId(txp::Id pId) {
    return (pId >= BB_ALL_FILE_TRANSFERS_COMPLETE ? 1 : 0);
}

inline int isCORAL_MsgId_ToLog(txp::Id pId) {
    int rc = 0;
    switch (pId)
    {
        case CORAL_STAGEOUT_START:
        {
            rc = 1;
            break;
        }

        default:
            break;
    }

    return rc;
}

extern inline int isMsgIdToLog(txp::Id pId)
{
    return ((isBB_MsgId(pId) || isCORAL_MsgId_ToLog(pId)) ? 1 : 0);
}


/**
 * \class Msg
 * Message call used to communicate data from one process to another.
 */
class Msg {
public:

	// Type defs
	typedef std::pair<txp::AttributeType, Attribute> AttributePair;
	typedef std::map<txp::AttributeName, txp::Attribute*> AttributeMap;

	// Constants
	static const size_t DEFAULT_DATA_ALIGNMENT_VALUE = 0;

    // Static data
#ifdef TXP_DEVELOPMENT
    static txp::Log d_log;
    static char d_log_buffer[255];
#endif

    static char reserved_2[txp::LENGTH_OF_MSG_RESERVED_2];

    // Msg() related static methods

    /**
     * \brief Convert Calculate CRC value to a printable array of character data.
     *
     * \param[in]   pValue CRC value.
     * \param[out]  pBuffer Output buffer for character data.
     * \param[in]   pSize Size available in buffer for character data.
     */
    static void calculateCRC_ToChar(const txp::CRC pValue, char* pBuffer, const int32_t pSize);

    /**
     * \brief Convert data location to a printable array of character data.
     *
     * \param[in]   pDataLocation Data location.
     * \param[out]  pBuffer Output buffer for character data.
     * \param[in]   pSize Size available in buffer for character data.
     */
	static void dataLocationToChar(const txp::DataLocation pDataLocation, char* pBuffer, const int32_t pSize);

    /**
     * \brief Convert deserialize option to a printable array of character data.
     *
     * \param[in]   pOption Data location.
     * \param[out]  pBuffer Output buffer for character data.
     * \param[in]   pSize Size available in buffer for character data.
     */
	static void deserializeOptionToChar(const txp::DeserializeOption pOption, char* pBuffer, const int32_t pSize);

    /**
     * \brief Is Data Location Valid
     *
     * \param[in]   pDataLocation Data Location
     * \return      int  0 -> not valid;  1 -> valid
     */
    static int isDataLocationValid(const txp::DataLocation pDataLocation);

    /**
     * \brief Is Deserialize Option Valid
     *
     * \param[in]   pOption Deserialize Option
     * \return      int  0 -> not valid;  1 -> valid
     */
    static int isDeserializeOptionValid(const txp::DeserializeOption pOption);

    /**
     * \brief Is MsgId Valid
     *
     * \param[in]   pId Message Id
     * \return      int  0 -> not valid;  1 -> valid
     */
    static int isMsgIdValid(const txp::Id pId);

    /**
     * \brief Is MsgId Value Valid
     *
     * \param[in]   pIdValue Message Id value
     * \return      int  0 -> not valid;  1 -> valid
     */
    static int isMsgIdValid(const uint32_t pIdValue);

    /**
     * \brief Is Message ID Supported
     *
     * \param[in]   pId Message Id
     * \return      int  0 -> not supported;  1 -> supported
     * \note  Need to add support for version on isSupported()  @@DLH
     */
    static int isMsgIdSupported(const txp::Id pId);

    /**
     * \brief Is Message ID Supported
     *
     * \param[in]   pIdValue Message Id
     * \return      int  0 -> not supported;  1 -> supported
     * \note  Need to add support for version on isSupported()  @@DLH
     */
    static int isMsgIdSupported(const uint32_t pIdValue);

    /**
     * \brief Convert message id option to a printable array of character data.
     *
     * \param[in]   pId Message id.
     * \param[out]  pBuffer Output buffer for character data.
     * \param[in]   pSize Size available in buffer for character data.
     */
    static void msgIdToChar(const txp::Id pId, char* pBuffer, const int32_t pSize);

    /**
     * \brief Set Default Calculate CRC Value.
     *
     * \param[in]   pValue      Calculate CRC value.
     */
	static void setDefaultCalculateCRC(std::string pValue);


    /**
     * \class MsgNumber
     * Uniquely identifies each message.
     */
	class MsgNumber {
	public:
		// Non-static methods

        /**
         * \brief Get value.
         *
         * \return   int32_t Current message number value
         */
		int32_t get() { return value; };

        /**
         * \brief Increments the message number object.
         */
		int32_t incr();

		// Constructors

        /**
         * \brief Default constructor
         *
         * Sets the message number value to zero.
         */
		MsgNumber() : value(0) {
		};

        /**
         * \brief Constructor with value.
         *
         * \param[in]   pValue Message number value
         */
		MsgNumber(int32_t pValue) : value(pValue) {
		};

        /**
         * \brief Destructor
         */
		virtual ~MsgNumber() {};

		// Data members
		int32_t value;      //! Message number value
	};

	//*****************************************************************************
	//  CLASS:  Msg
	//*****************************************************************************

	// Default constructor

    /**
     * \brief Default constructor
     *
     * Sets the message id as undefined, default value for state, and latest version for id.
     */
	Msg();

	// Construct message with latest version for id

    /**
     * \brief Constructor with the id specified
     *
     * Sets the message id as input, default value for state, and latest version for id.
     */
	Msg(const txp::Id pId);

    /**
     * \brief Constructor with the id and version specified
     *
     * Sets the message id and version as input, default value for state.
     */
	Msg(const txp::Id pId, Version pVersion);

    /**
     * \brief Copy constructor
     *
     * Copies the definitional attributes of the message.  Runtime attributes are set
     * to default values.
     */
	Msg(const Msg &pMsg);

    /**
     * \brief Destructor
     */
	virtual ~Msg();

    static txp::CRC DEFAULT_CALCULATE_CRC;

    static uint16_t AttrType_CharValue;
    static uint16_t AttrType_UINT8_Value;
    static uint16_t AttrType_UINT16_Value;
    static uint16_t AttrType_UINT32_Value;
    static uint16_t AttrType_UINT64_Value;
    static uint16_t AttrType_INT8_Value;
    static uint16_t AttrType_INT16_Value;
    static uint16_t AttrType_INT32_Value;
    static uint16_t AttrType_INT64_Value;
    static uint16_t AttrType_CharArrayValue;
    static uint16_t AttrType_ArrayOfCharArraysValue;

    static unsigned long CRC_Value;

	// Msg() specific static methods

    /**
     * \brief Allocate and build a message in heap, returning addressability in oMsg
     *
     * \param[in]   pId     Id for message.
     * \param[out]  oMsg    Pointer to message object.
     * \return      int  0 -> success;  <0 -> failure
     * \par Error codes
     * -3 Failure to create message.
     * \note The latest version of the message is built.
     * \note Invoker is responsible to delete this message object.
     */
	static int buildMsg(const txp::Id pId, txp::Msg* &oMsg);

    /**
     * \brief Allocate and build a message in heap, returning addressability in oMsg
     *
     * \param[in]   pId         Id for message.
     * \param[in]   pVersion    Version for message.
     * \param[out]  oMsg        Pointer to message object.
     * \return      int  0 -> success;  <0 -> failure
     * \par Error codes
     * -4 Failure to create message.
     * \note Invoker is responsible to delete this message object.
    */
	static int buildMsg(const txp::Id pId, Version pVersion, txp::Msg* &oMsg);

    /**
     * \brief Deserialize a message header from a char buffer into a constructed Msg() object in heap,
     *        associating a heap buffer object large enough to contain the entire message from the connection
     *        object.  Addressability to the created object is in oMsg and the length of the created heap buffer
     *        is returned in oTotalBufferSize.
     *
     * \param[out]  oMsg                Pointer to message object.
     * \param[in]   pBuffer             Pointer to char buffer.
     * \param[out]  oTotalBufferSize    Total length of heap buffer so that the remaining portion of the message
     *                                  can be read from the connection object.
     * \param[in]   pDataBuffer         Pointer to an already existing data buffer for the message object to use.
     *                                  The created heap buffer will use this data buffer.
     *                                  Default is zero and indicates that the Msg() object will create it's
     *                                  own heap buffer, allocating it's own data buffer.
     * \param[in]   pDataBufferSize     Length of the data buffer.  Only valid if pDataBuffer is also provided.
     *                                  Default is zero, which indicates to use the mesage length in the header.
     * \todo
     * \note It is envisioned that in the future we will need to
     *       support multiple buffers for the attributes that make up a message.  In that case, the last two
     *       arguments probably evolve into a vector of buffers and sizes.  Future memory management with respect
     *       to RDMA will dictate how this proceeds...  (Not sure how such a vector would map into one or more
     #       attributes...)  @@DLH
     * \return      int                 0 -> success;  <0 -> failure
     * \par Error codes
     * Any return code from deserializeToMsg()\n
     * Any return code from allocateHeapBuffer()\n
     * -31 Pointer to message (oMsg) was not passed in as null.
     * -32 Input data buffer size too small.  Data buffer size must at least be as large as the message header.
     * \note The length of the memory pointed at by pBuffer must at least be LENGTH_OF_MSG_HEADER.
     * \note Invoker is responsible to delete this message object.
     */
    static int deserializeHeaderToMsg(txp::Msg* &oMsg, char pBuffer[LENGTH_OF_MSG_HEADER], ssize_t &oTotalBufferSize, char* pDataBuffer=0, const size_t pDataBufferSize=0 );

    /**
     * \brief Deserialize from a buffer into a constructed Msg() object in heap, giving addressability back in oMsg.
     *
     * \param[out]  oMsg        Pointer to message object.
     * \param[in]   pHeapBuffer Pointer to heap buffer.
     * \param[in]   pLength     Length to deserialize.  Default is zero.
     * \return      int         0 -> success;  <0 -> failure
     * \par Error codes
     * -1  CRC check for message header failed.\n
     * -2  CRC check for data portion of message (attributes) failed.\n
     * -5  Invalid length specified.  Length must be greater than or equal to the total message length because CRCs are being calculated.\n
     * -6  Message not valid.\n
     * -7  Message not created.\n
     * -8  Message cannot be deserialied from the specified buffer.
     * -9  Invalid length specified.  Length must be greater than the deserialized length of a message header.\n
     * -33 Pointer to message (oMsg) was not passed in as null.
     * -40 CRC check for data portion of message (attributes) failed. (When only deserializing the attributes.)\n
     * -41 Invalid length specified.  Length must be greater than or equal to the total message length because CRCs are being calculated. (When only deserializing the attributes.)\n
     * -55 Invalid heapbuffer size.  Heap buffer size must be as large as the length of the message in the header.
     * -56 Invalid message length in buffer.
     * \note If pLength is 0, then the length to be deserialized into a Msg() will be the minimum of the
     *       length found in the buffer (offset=8, for 4 bytes) and size of the heap buffer.  Otherwise,
     *       the length to be deserialized will be the min(pLength, size of heap buffer, length found in buffer).
     * \note Invoker is responsible to delete this message object.
     */
	static int deserializeToMsg(txp::Msg* &oMsg, HeapBuffer* pHeapBuffer, const size_t pLength=0);

    /**
     * \brief Deserialize from a buffer into a constructed Msg() object in heap, giving addressability back in oMsg.
     *
     * \param[out]  oMsg        Pointer to message object.
     * \param[in]   pHeapBuffer Pointer to heap buffer.
     * \param[in]   pOption     Deserialization option.
     * \param[in]   pLength     Length to deserialize.  Default is zero.
     * \return      int         0 -> success;  <0 -> failure
     * \par Error codes
     * -1  CRC check for message header failed.\n
     * -2  CRC check for data portion of message (attributes) failed.\n
     * -5  Invalid length specified.  Length must be greater than the deserialized length of a message header.\n
     * -6  Message not valid.\n
     * -7  Message not created.\n
     * -8  Message cannot be deserialied from the specified buffer.
     * -9  Invalid length specified.  Length must be greater than the deserialized length of a message header.\n
     * -33 Pointer to message (oMsg) was not passed in as null.
     * -40 CRC check for data portion of message (attributes) failed. (When only deserializing the attributes.)\n
     * -41 Invalid length specified.  Length must be greater than or equal to the total message length because CRCs are being calculated. (When only deserializing the attributes.)\n
     * -55 Invalid heapbuffer size.  Heap buffer size must be as large as the length of the message in the header.
     * -56 Invalid message length in buffer.
     * \note If pLength is 0, then the length to be deserialized into a Msg() will be the minimum of the
     *       length found in the buffer (offset=8, for 4 bytes) and size of the heap buffer.  Otherwise,
     *       the length to be deserialized will be the min(pLength, size of heap buffer, length found in buffer).
     * \note Invoker is responsible to delete this message object.
     */
	static int deserializeToMsg(txp::Msg* &oMsg, HeapBuffer* pHeapBuffer, const txp::DeserializeOption &pOption, const size_t pLength=0);

    /**
     * \brief Deserialize from a char buffer into a constructed Msg() object in heap, giving addressability back in oMsg.
     *
     * \param[out]  oMsg            Pointer to message object.
     * \param[in]   pBuffer         Pointer to char buffer.
     * \param[in]   pBufferSize     Buffer size
     * \param[out]  oTotalMsgSize   Length of the unserialized message in the buffer
     * \param[in]   pLength         Length to deserialize.  Default is zero.
     * \return      int             0 -> success;  <0 -> failure
     * \par Error codes
     * -1  CRC check for message header failed.\n
     * -2  CRC check for data portion of message (attributes) failed.\n
     * -5  Invalid length specified.  Length must be greater than the deserialized length of a message header.\n
     * -6  Message not valid.\n
     * -7  Message not created.\n
     * -8  Message cannot be deserialied from the specified buffer.
     * -9  Invalid length specified.  Length must be greater than the deserialized length of a message header.\n
     * -38 Heap buffer could not be created
     * -40 CRC check for data portion of message (attributes) failed. (When only deserializing the attributes.)\n
     * -41 Invalid length specified.  Length must be greater than or equal to the total message length because CRCs are being calculated. (When only deserializing the attributes.)\n
     * -51 Pointer to message (oMsg) was not passed in as null.
     * -52 Pointer to buffer passed as null.
     * -53 Invalid buffer size.  Buffer size must be as large as a message header.
     * -54 Invalid length.  Length must be less than of equal to the the buffer size.
     * -55 Invalid heapbuffer size.  Heap buffer size must be as large as the length of the message in the header.
     * -56 Invalid message length in buffer.
     * \note If pLength is 0, then the length to be deserialized into a Msg() will be the minimum of the
     *       length found in the buffer (offset=8, for 4 bytes) and size of the heap buffer.  Otherwise,
     *       the length to be deserialized will be the min(pLength, size of heap buffer, length found in buffer).
     * \note Invoker is responsible to delete this message object.
     */
    static int deserializeToMsg(txp::Msg* &oMsg, char* pBuffer, const size_t pBufferSize, ssize_t &oTotalMsgSize, const size_t pLength=0);

    /**
     * \brief Deserialize from a char buffer into a constructed Msg() object in heap, giving addressability back in oMsg.
     *
     * \param[out]  oMsg            Pointer to message object.
     * \param[in]   pBuffer         Pointer to char buffer.
     * \param[in]   pBufferSize     Buffer size
     * \param[out]  oTotalMsgSize   Length of the unserialized message in the buffer
     * \param[in]   pOption         Deserialization option.
     * \param[in]   pLength         Length to deserialize.  Default is zero.
     * \return      int             0 -> success;  <0 -> failure
     * \par Error codes
     * -1  CRC check for message header failed.\n
     * -2  CRC check for data portion of message (attributes) failed.\n
     * -5  Invalid length specified.  Length must be greater than the deserialized length of a message header.\n
     * -6  Message not valid.\n
     * -7  Message not created.\n
     * -8  Message cannot be deserialied from the specified buffer.
     * -9  Invalid length specified.  Length must be greater than the deserialized length of a message header.\n
     * -38 Heap buffer could not be created
     * -40 CRC check for data portion of message (attributes) failed. (When only deserializing the attributes.)\n
     * -41 Invalid length specified.  Length must be greater than or equal to the total message length because CRCs are being calculated. (When only deserializing the attributes.)\n
     * -51 Pointer to message (oMsg) was not passed in as null.
     * -52 Pointer to buffer passed as null.
     * -53 Invalid buffer size.  Buffer size must be as large as a message header.
     * -54 Invalid length.  Length must be less than of equal to the the buffer size.
     * -55 Invalid heapbuffer size.  Heap buffer size must be as large as the length of the message in the header.
     * -56 Invalid message length in buffer.
     * \note If pLength is 0, then the length to be deserialized into a Msg() will be the minimum of the
     *       length found in the buffer (offset=8, for 4 bytes) and size of the heap buffer.  Otherwise,
     *       the length to be deserialized will be the min(pLength, size of heap buffer, length found in buffer).
     * \note Invoker is responsible to delete this message object.
     */
	static int deserializeToMsg(txp::Msg* &oMsg, char* pBuffer, const size_t pBufferSize, ssize_t &oTotalMsgSize, const txp::DeserializeOption &pOption, const size_t pLength=0);

    // Inlined non-static methods

    /**
     * \brief Get Attributes Pointer
     *
     * \return  std::vector<Attribute*>*  Pointer to message attributes vector
     */
    inline std::vector<Attribute*>* getAttributesPtr() { return msgAttributes; };

    /**
     * \brief Get Heap Buffer Pointer
     *
     * \return  HeapBuffer*  Pointer to heap buffer
     */
	inline HeapBuffer* getHeapBufferPtr() { return msgHeapBuffer; };

    /**
     * \brief Get I/O Vector Count
     *
     * \return  int32_t  I/O vector count
     */
    inline int32_t get_IO_VectorCount() { return msg_IO_VectorCount; };

    /**
     * \brief Get I/O Vector Pointer
     *
     * \return  iovec*  Pointer to I/O vector
     */
    inline struct iovec* get_IO_VectorPtr() { return msg_IO_Vector; };

    /**
     * \brief Get Buffer Pointer.
     *
     * \return  HeapBuffer*  Addressability to the heap buffer within the message object
     */
    inline HeapBuffer* getBufferPtr() { return getHeapBufferPtr(); };

    /**
     * \brief Get Length of Pad in Message
     *
     * \return  size_t  Length of pad
     */
	inline size_t getLengthOfPad() { return getHeapBufferPtr()->getLengthOfPad(); };

    /**
     * \brief Get message creator pid
     *
     * \return  pid_t  Creator pid
     */
	inline pid_t getMsgCreatorPid() { return msgCreatorPid; };

    /**
     * \brief Get message creator tid
     *
     * \return  pthread_t  Creator tid
     */
	inline pthread_t getMsgCreatorTid() { return msgCreatorTid; };

    /**
     * \brief Get message id
     *
     * \return  Id  Message id
     */
	inline txp::Id getMsgId() { return msgId; };

    /**
     * \brief Get message length with data values
     *
     * \param[in]   pDataAlignment  Data alignment value.
     * \return  size_t  Message length serialized as all data values
     */
	inline size_t getMsgLengthWithDataValues(const size_t pDataAlignment=DEFAULT_DATA_ALIGNMENT_VALUE) {
    	return (pDataAlignment ? msgLengthWithDataValues+calculatePad(msgLengthWithDataValues, pDataAlignment) : msgLengthWithDataValues);
    }

    /**
     * \brief Get message length with data values except pointer values for arrays
     *
     * \param[in]   pDataAlignment  Data alignment value.
     * \return  size_t  Message length serialized as data values, except pointers for array values
     */
	inline size_t getMsgLengthWithDataValuesExceptForArrays(const size_t pDataAlignment=DEFAULT_DATA_ALIGNMENT_VALUE) {
        return (pDataAlignment ? msgLengthWithDataValuesExceptForArrays+calculatePad(msgLengthWithDataValuesExceptForArrays, pDataAlignment) : msgLengthWithDataValuesExceptForArrays);
	}

    /**
     * \brief Get message length with data pointers
     *
     * \param[in]   pDataAlignment  Data alignment value.
     * \return  size_t  Message length serialized as pointers to all data values
     */
	inline size_t getMsgLengthWithPtrValues(const size_t pDataAlignment=DEFAULT_DATA_ALIGNMENT_VALUE) {
		return (pDataAlignment ? msgLengthWithPtrValues+calculatePad(msgLengthWithPtrValues, pDataAlignment) : msgLengthWithPtrValues);
	}

    /**
     * \brief Get message number
     *
     * \return  int32_t  Message number
     */
	inline int32_t getMsgNumber() { return msgNumber; };

    /**
     * \brief Get message requestor pid
     *
     * \return  pid_t  Requestor pid
     */
	inline pid_t getMsgRequestorPid() { return msgRequestorPid; };

    /**
     * \brief Get message requestor tid
     *
     * \return  pthread_t  Requestor tid
     */
	inline pthread_t getMsgRequestorTid() { return msgRequestorTid; };

    /**
     * \brief Get next message number
     *
     * \return  int32_t  Next message number
     */
	inline int32_t getNextMsgNumber() { return nextMsgNumber; };

    /**
     * \brief Get number of message attributes
     *
     * \return  int32_t  Number of message attributes
     */
	inline int32_t getNumberOfAttributes() { return (msgAttributes ? msgAttributes->size() : 0); };

    /**
     * \brief Get previous message number
     *
     * \return  int32_t  Previous message number
     */
	inline int32_t getPrevMsgNumber() { return prevMsgNumber; };

    /**
     * \brief Get request message number
     *
     * \return  int32_t  Request message number
     */
	inline int32_t getRequestMsgNumber() { return requestMsgNumber; };

    /**
     * \brief Get Serialized Length
     *
     * \return  size_t  Serialized length
     */
    inline ssize_t getSerializedLen() { return (msgHeapBuffer ? (ssize_t)msgHeapBuffer->getMsgLength() : 0); };

    /**
     * \brief Get Version
     *
     * \return  Version  Message version
     */
	inline txp::Version getVersion() { return msgVersion; };

    /**
     * \brief Initialization
     *
     */
    inline void init() {
        if (txp::init()) {
#ifdef TXP_DEVELOPMENT
            // NOTE:  The default Msg log is created before the configuration file is read/processed.
            //        Set those values from the configuration files for the default log.
            d_log.setLogLevel(txp::Log::DEFAULT_OPEN_LOGLEVEL);
            if (txp::Log::DEFAULT_OPEN_LOGSTATE == txp::Log::OPEN) {
                d_log.open();
            } else {
                d_log.close();
            }
#endif
        }
    }

    /**
     * \brief Is Response Message
     *
     * \return      int  0 -> true;  <0 -> false
     *
     */
	inline int isResponseMsg() {return (requestMsgNumber ? 1 : 0);};

    /**
     * \brief Set Requestor Pid.
     *
     * \param[in]   pPid Pid value.
     *
     */
	inline void setRequestorPid(pid_t &pPid) {msgRequestorPid = pPid; return;};

    /**
     * \brief Set Requestor Tid.
     *
     * \param[in]   pTid Tid value.
     *
     */
	inline void setRequestorTid(pthread_t &pTid) {msgRequestorTid = pTid; return;};

	// Non-static methods

    /**
     * \brief Add attribute to message.
     *
     * \param[in]   pAttribute Pointer to attribute
     * \return      int  0 -> success;  <0 -> failure
     * \par Error codes
     * -12 Failure when inserting attribute into message attribute map\n
     * -30 Attribute with this name already exists for message attributes.\n
     * -34 Message attribute vector could not be created.\n
     * -35 Message attribute map could not be created.\n
     * -44 Maximum number of attributes added to message.
     */
	int addAttribute(Attribute* pAttribute);

    /**
     * \brief Add attribute to message.
     *
     * \param[in]   pName Attribute name.
     * \param[in]   pData Data.
     * \return      int  0 -> success;  <0 -> failure
     * \par Error codes
     * Same errors as returned from Msg::addAttribute(Attribute*)\n
     * Same errors as returned from Attribute::buildAttr(...)\n
     */
    int addAttribute(const txp::AttributeName pName, const char& pData);

    /**
     * \brief Add attribute to message.
     *
     * \param[in]   pName Attribute name.
     * \param[in]   pData Data.
     * \return      int  0 -> success;  <0 -> failure
     * \par Error codes
     * Same errors as returned from Msg::addAttribute(Attribute*)\n
     * Same errors as returned from Attribute::buildAttr(...)\n
     */
    int addAttribute(const txp::AttributeName pName, const uint8_t& pData);

    /**
     * \brief Add attribute to message.
     *
     * \param[in]   pName Attribute name.
     * \param[in]   pData Data.
     * \return      int  0 -> success;  <0 -> failure
     * \par Error codes
     * Same errors as returned from Msg::addAttribute(Attribute*)\n
     * Same errors as returned from Attribute::buildAttr(...)\n
     */
    int addAttribute(const txp::AttributeName pName, const uint16_t& pData);

    /**
     * \brief Add attribute to message.
     *
     * \param[in]   pName Attribute name.
     * \param[in]   pData Data.
     * \return      int  0 -> success;  <0 -> failure
     * \par Error codes
     * Same errors as returned from Msg::addAttribute(Attribute*)\n
     * Same errors as returned from Attribute::buildAttr(...)\n
     */
    int addAttribute(const txp::AttributeName pName, const uint32_t& pData);

    /**
     * \brief Add attribute to message.
     *
     * \param[in]   pName Attribute name.
     * \param[in]   pData Data.
     * \return      int  0 -> success;  <0 -> failure
     * \par Error codes
     * Same errors as returned from Msg::addAttribute(Attribute*)\n
     * Same errors as returned from Attribute::buildAttr(...)\n
     */
    int addAttribute(const txp::AttributeName pName, const uint64_t& pData);

    /**
     * \brief Add attribute to message.
     *
     * \param[in]   pName Attribute name.
     * \param[in]   pData Data.
     * \return      int  0 -> success;  <0 -> failure
     * \par Error codes
     * Same errors as returned from Msg::addAttribute(Attribute*)\n
     * Same errors as returned from Attribute::buildAttr(...)\n
     */
    int addAttribute(const txp::AttributeName pName, const int8_t& pData);

    /**
     * \brief Add attribute to message.
     *
     * \param[in]   pName Attribute name.
     * \param[in]   pData Data.
     * \return      int  0 -> success;  <0 -> failure
     * \par Error codes
     * Same errors as returned from Msg::addAttribute(Attribute*)\n
     * Same errors as returned from Attribute::buildAttr(...)\n
     */
    int addAttribute(const txp::AttributeName pName, const int16_t& pData);

    /**
     * \brief Add attribute to message.
     *
     * \param[in]   pName Attribute name.
     * \param[in]   pData Data.
     * \return      int  0 -> success;  <0 -> failure
     * \par Error codes
     * Same errors as returned from Msg::addAttribute(Attribute*)\n
     * Same errors as returned from Attribute::buildAttr(...)\n
     */
    int addAttribute(const txp::AttributeName pName, const int32_t& pData);

    /**
     * \brief Add attribute to message.
     *
     * \param[in]   pName Attribute name.
     * \param[in]   pData Data.
     * \return      int  0 -> success;  <0 -> failure
     * \par Error codes
     * Same errors as returned from Msg::addAttribute(Attribute*)\n
     * Same errors as returned from Attribute::buildAttr(...)\n
     */
    int addAttribute(const txp::AttributeName pName, const int64_t& pData);

    /**
     * \brief Add attribute to message.
     *
     * \param[in]   pName Attribute name.
     * \param[in]   pData Pointer to data.
     * \return      int  0 -> success;  <0 -> failure
     * \par Error codes
     * Same errors as returned from Msg::addAttribute(Attribute*)\n
     * Same errors as returned from Attribute::buildAttr(...)\n
     */
    int addAttribute(const txp::AttributeName pName, const char* pData);

    /**
     * \brief Add attribute to message.
     *
     * \param[in]   pName Attribute name.
     * \param[in]   pData Pointer to data.
     * \return      int  0 -> success;  <0 -> failure
     * \par Error codes
     * Same errors as returned from Msg::addAttribute(Attribute*)\n
     * Same errors as returned from Attribute::buildAttr(...)\n
     */
    int addAttribute(const txp::AttributeName pName, const uint8_t* pData);

    /**
     * \brief Add attribute to message.
     *
     * \param[in]   pName Attribute name.
     * \param[in]   pData Pointer to data.
     * \return      int  0 -> success;  <0 -> failure
     * \par Error codes
     * Same errors as returned from Msg::addAttribute(Attribute*)\n
     * Same errors as returned from Attribute::buildAttr(...)\n
     */
    int addAttribute(const txp::AttributeName pName, const uint16_t* pData);

    /**
     * \brief Add attribute to message.
     *
     * \param[in]   pName Attribute name.
     * \param[in]   pData Pointer to data.
     * \return      int  0 -> success;  <0 -> failure
     * \par Error codes
     * Same errors as returned from Msg::addAttribute(Attribute*)\n
     * Same errors as returned from Attribute::buildAttr(...)\n
     */
    int addAttribute(const txp::AttributeName pName, const uint32_t* pData);

    /**
     * \brief Add attribute to message.
     *
     * \param[in]   pName Attribute name.
     * \param[in]   pData Pointer to data.
     * \return      int  0 -> success;  <0 -> failure
     * \par Error codes
     * Same errors as returned from Msg::addAttribute(Attribute*)\n
     * Same errors as returned from Attribute::buildAttr(...)\n
     */
    int addAttribute(const txp::AttributeName pName, const uint64_t* pData);

    /**
     * \brief Add attribute to message.
     *
     * \param[in]   pName Attribute name.
     * \param[in]   pData Pointer to data.
     * \return      int  0 -> success;  <0 -> failure
     * \par Error codes
     * Same errors as returned from Msg::addAttribute(Attribute*)\n
     * Same errors as returned from Attribute::buildAttr(...)\n
     */
    int addAttribute(const txp::AttributeName pName, const int8_t* pData);

    /**
     * \brief Add attribute to message.
     *
     * \param[in]   pName Attribute name.
     * \param[in]   pData Pointer to data.
     * \return      int  0 -> success;  <0 -> failure
     * \par Error codes
     * Same errors as returned from Msg::addAttribute(Attribute*)\n
     * Same errors as returned from Attribute::buildAttr(...)\n
     */
    int addAttribute(const txp::AttributeName pName, const int16_t* pData);

    /**
     * \brief Add attribute to message.
     *
     * \param[in]   pName Attribute name.
     * \param[in]   pData Pointer to data.
     * \return      int  0 -> success;  <0 -> failure
     * \par Error codes
     * Same errors as returned from Msg::addAttribute(Attribute*)\n
     * Same errors as returned from Attribute::buildAttr(...)\n
     */
    int addAttribute(const txp::AttributeName pName, const int32_t* pData);

    /**
     * \brief Add attribute to message.
     *
     * \param[in]   pName Attribute name.
     * \param[in]   pData Pointer to data.
     * \return      int  0 -> success;  <0 -> failure
     * \par Error codes
     * Same errors as returned from Msg::addAttribute(Attribute*)\n
     * Same errors as returned from Attribute::buildAttr(...)\n
     */
    int addAttribute(const txp::AttributeName pName, const int64_t* pData);

    /**
     * \brief Add attribute to message.
     *
     * \param[in]   pName   Attribute name.
     * \param[in]   pData   Pointer to data.
     * \param[in]   pLength Length of data.
     * \param[in]   pCopyDataOption  0 -> Do not copy data;  Otherwise, copy data and message facility owns data storage.
     * \return      int  0 -> success;  <0 -> failure
     * \par Error codes
     * Same errors as returned from Msg::addAttribute(Attribute*)\n
     * Same errors as returned from Attribute::buildAttr(...)\n
     */
    int addAttribute(const txp::AttributeName pName, const char* pData, const size_t pLength, const txp::COPY_DATA_OPTION pCopyDataOption=DO_NOT_COPY);

    /**
     * \brief Add attribute to message.
     *
     * \param[in]   pName   Attribute name.
     * \param[in]   pData   Pointer to char array data.
     * \return      int  0 -> success;  <0 -> failure
     * \par Error codes
     * Same errors as returned from Msg::addAttribute(Attribute*)\n
     * Same errors as returned from Attribute::buildAttr(...)\n
     * -65 Message does not exist.\n
     */
    int addAttribute(const txp::AttributeName pName, const txp::CharArray* pData);

    /**
     * \brief Allocate heap buffer.
     *
     * \param[in]   pSize   Size.  Default is 0, which then uses a default for size.
     * \return      int  0 -> success;  <0 -> failure
     * \par Error codes
     * -38 Heap buffer could not be created
     * -60 Heap buffer size must be at least as large as the length of the message header
     */
	int allocateHeapBuffer(const size_t pSize=0);

    /**
     * \brief Allocate heap buffer.
     *
     * \param[in]   pDataBuffer     Pointer to the data buffer to use with the heap buffer.
     * \param[in]   pDataBufferSize Size of the data buffer.
     * \return      int  0 -> success;  <0 -> failure
     * \par Error codes
     * -39 Heap buffer could not be created
     * -61 Heap buffer size must be at least as large as the length of the message header
     */
	int allocateHeapBuffer(char* pDataBuffer, const size_t pDataBufferSize);

    /**
     * \brief Associate heap buffer.
     *
     * \param[in]   pHeapBuffer     Pointer to the heap buffer to associate with the message.
     * \return      int  0 -> success;  <0 -> failure
     * \par Error codes
     * -39 Heap buffer could not be created
     */
	int associateHeapBuffer(HeapBuffer* pHeapBuffer);

    /**
     * \brief Build I/O Vector.
     *
     * This is the structure that is required by a socket writev() operation.
     * \note This should be private.  Need to fix testcases first...  @@DLH
     *
     * \return      int  0 -> success, <0 -> failure
     * \par Error codes
     * -13 Invalid attribute pointer type\n
     * -14 Too many I/O vector elements\n
     * -15 Either the heap buffer does not exist or it is not current with the message attributes.\n
     * -36 I/O vector could not be created
     */
	int build_IO_Vector();

    /**
     * \brief Build response message.
     *
     * \param[out]  oMsg        Pointer to message object.
     * \return      int  0 -> success;  <0 -> failure
     * \par Error codes
     * -16 Message already has a response message built for it\n
     * -37 Response message could not be created
     */
	int buildResponseMsg(txp::Msg* &oMsg);

    /**
     * \brief Calculate Message Header CRC Value.
     *
     * \param[inout]    pCRC CRC value
     */
    void calculateMsgHeaderCRC_Value(unsigned long &pCRC);

    /**
     * \brief Calculate Pad
     *
     * \param[in]   pLength         Length of data.
     * \param[in]   pDataAlignment  Data alignment value.
     * \return  size_t  Pad value
     */
    size_t calculatePad(const size_t pLength, const size_t pDataAlignment);

    /**
     * \brief Deserialize Attributes to Message.
     *
     * \param[in]   pHeapBuffer Pointer to heap buffer.
     * \param[in]   pMsgLength  Message length.
     * \return      int         0 -> success;  <0 -> failure
     * \par Error codes
     * Same error codes as generated by buildAttr()
     * \note There is NO verification of the data in the buffer being interpreted
     *       as a consecutive set of attributes.  User beware...
     *       As such, pMsgLength must be set to the length of data to deserialize.  @@DLH
     */
	int deserializeAttrsToMsg(HeapBuffer* pHeapBuffer, const size_t pMsgLength);

    /**
     * \brief Deserialize Attributes to Message.
     *
     * \param[in]   pHeapBuffer Pointer to heap buffer.
     * \param[in]   pOption Deserialize Option.
     * \param[in]   pMsgLength  Message length.
     * \return      int         0 -> success;  <0 -> failure
     * \par Error codes
     * Same error codes as generated by buildAttr()
     * \note There is NO verification of the data in the buffer being interpreted
     *       as a consecutive set of attributes.  User beware...
     *       As such, pMsgLength must be set to the length of data to deserialize.  @@DLH
     */
	int deserializeAttrsToMsg(HeapBuffer* pHeapBuffer, const txp::DeserializeOption &pOption, const size_t pMsgLength);

    /**
     * \brief Dumps message object to a Log.
     *
     * \param[in]   pLog
     * \param[in]   pPrefix Text to identify this operation.  Default is no prefix data.
     * \param[in]   pDumpRawHeapBuffer Default is yes.
     */
	void dump(Log& pLog, const char* pPrefix=0, const int pDumpRawHeapBuffer=1);

    /**
     * \brief Dumps message attributes to a Log.
     *
     * \param[in]   pMsgAttributes Message attributes
     * \param[in]   pLog
     * \param[in]   pPrefix Text to identify this operation.  Default is no prefix data.
     */
	void dump_msgAttributes(std::vector<Attribute*>* pMsgAttributes, Log& pLog, const char* pPrefix=0);

    /**
     * \brief Frees a heap buffer.
     *
     * Disassociates the heap buffer from the message object and deletes the buffer.
     */
	void freeHeapBuffer();

    /**
     * \brief Get Data Buffer Pointer.
     *
     * \return  char* Addressability to the buffer within the heap buffer used for serialization
     */
    char* getDataBufferPtr();

    /**
     * \brief Is Messsage Valid.
     *
     * \return      int  0 -> not valid;  1 -> valid
     */
	int isValid();

    /**
     * \brief Next message.
     *
     * Sets the next message to logically follow after this message.
     *
     * \param[in]   pNextMsg Next message.
     * \return      int  0 -> success;  <0 -> failure
     * \par Error codes
     * -17 The next message already has a previous message\n
     * -18 This message already has a next message
     */
	int nextMsg(Msg &pNextMsg);

    /**
     * \brief Retrieve attribute.
     *
     * \param[in]   pNameValue  Enum value for attribute name
     * \return      Attribute*  Pointer to the attribute
     */
	txp::Attribute* retrieveAttr(const int32_t pNameValue);

    /**
     * \brief Retrieve attribute.
     *
     * \param[in]   pName       Attribute name
     * \return      Attribute*  Pointer to the attribute
     */
	txp::Attribute* retrieveAttr(const txp::AttributeName &pName);

    /**
     * \brief Retrieve attributes.
     *
     * \return      std::map<txp::AttributeName, txp::Attribute*>* Pointer to the attribute map
     */
	std::map<txp::AttributeName, txp::Attribute*>* retrieveAttrs();

    /**
     * \brief Sent
     *
     * For a non-response message, the sent flag is set;  Otherwise, the response flag is set.
     *
     */
    void sent();

    /**
     * \brief Serialize to heap buffer.
     *
     * The message is serialized into the heap buffer and pointers to the actual data values
     * for all attributes are also serialized into the buffer.
     *
     * \note To send this serialized message, something similar to a socket writev()
     *       must be used.  To perform this type of write, the msg_IO_Vector must be
     *       built and used for the writev operation.
     *
     * \param[in]   pDataAlignment  Default is 0.
     * \param[in]   pCalculateCRC   0=false; otherwise=true.  Default is DEFAULT_CALCULATE_CRC.
     * \return      ssize_t         >0 -> success, serialized length;  <0 -> failure
     * \par Error codes
     * Any return code from Msg.serializeCommonHeader(HeapBuffer*)\n
     * Any return code from Attribute.add(HeapBuffer*)\n
     * -28 The length to serialize is greater than what is available in the buffer (Attr with ptr to non-array data does exist)
     * -50 The length to serialize is greater than what is available in the buffer (Attr with ptr to non-array data does not exist)
     */
	ssize_t serializeToHeapBuffer(size_t pDataAlignment=DEFAULT_DATA_ALIGNMENT_VALUE, int pCalculateCRC=DEFAULT_CALCULATE_CRC);

    /**
     * \brief Serialize with values to heap buffer.
     *
     * The message is serialized into the heap buffer and the actual data values for all
     * attributes are also serialized into the buffer.
     *
     * \param[in]   pDataAlignment  Default is 0.
     * \param[in]   pCalculateCRC   0=false; otherwise=true.  Default is DEFAULT_CALCULATE_CRC.
     * \return      ssize_t         >0 -> success, serialized length;  <0 -> failure
     * \par Error codes
     * Any return code from Msg.serializeCommonHeader(HeapBuffer*)\n
     * Any return code from Attribute.add(HeapBuffer*)\n
     * -29 The length to serialize is greater than what is available in the buffer
     */
	ssize_t serializeWithValuesToHeapBuffer(size_t pDataAlignment=DEFAULT_DATA_ALIGNMENT_VALUE, int pCalculateCRC=DEFAULT_CALCULATE_CRC);

private:
	// Static methods

    /**
     * \brief Increment message number
     *
     * \return  int32_t  Incremented message number value
     */
	static int32_t incrMsgNumber();

	// Non-static methods

//	size_t calculateLengthWithDataValues();
//	size_t calculateLengthWithPtrValues();

    /**
     * \brief Serialize Common Header.
     *
     * \param[in]   pBuffer         Pointer to heap buffer.
     * \param[in]   pDataAlignment  Data alignment value.
     * \return      int     >0 -> success, serialized length;  <0 -> failure
     * \par Error codes
     * -19 Failure during serialization of the reserved value\n
     * -20 Failure during serialization of state value\n
     * -21 Failure during serialization of version value\n
     * -22 Failure during serialization of the message id value\n
     * -23 Failure during the serialization of the request message number value\n
     * -24 Failure during the serialization of the next message number value\n
     * -25 Failure during the serialization of the previous message number value\n
     * -26 Failure during the serialization of the message number value\n
     * -27 No buffer is present for the serialization operation\n
     * -42 Failure during the serialization of the message header CRC value
     * -43 Failure during the serialization of the message attributes CRC value\n
     * -58 Data alignment failure\n
     */
	int serializeCommonHeader(HeapBuffer* pBuffer, size_t pDataAlignment);

    /**
     * \brief Set creator ids.
     */
	void setMsgCreatorIds();

    /**
     * \brief Set Message Creator Pid.
     *
     * \param[in]   pPid* Pointer to pid value.  If null, current pid value is set.
     *
     */
	void setMsgCreatorPid(pid_t* pPid=0);

    /**
     * \brief Set Message Creator Tid.
     *
     * \param[in]   pTid* Pointer to tid value.  If null, current tid value is set.
     *
     */
	void setMsgCreatorTid(pthread_t* pTid=0);

	// Construct message from buffer

    /**
     * \brief Construct message from buffer
     *
     * \param[in]   pBuffer Pointer to heap buffer.
     */
	Msg(HeapBuffer* pBuffer);

    /**
     * \brief Construct response message from another message
     *
     * \param[in]   pId Message id.
     * \param[in]   pRequestMsgNumber Request message number.
     * \param[in]   pVersion Version.
     * \param[in]   pRequestorPid Requestor pid.
     * \param[in]   pRequestorTid Requestor tid.
     */
	Msg(txp::Id pId, int32_t pRequestMsgNumber, txp::Version pVersion, pid_t pRequestorPid, pthread_t pRequestorTid);

	// Data members
	int32_t                 msgNumber;                  //! Message number
	int32_t                 prevMsgNumber;              //! Previous message number
	int32_t                 nextMsgNumber;              //! Next message number
	int32_t                 requestMsgNumber;           //! Request message number
	                                                    //! \note Only set for response messages
	pid_t                   msgCreatorPid;              //! Process id that created the message
	pid_t                   msgRequestorPid;            //! Process id that made the message request
	                                                    //! \note Only set for response messages
	pthread_t               msgCreatorTid;              //! Thread id that created the message
	pthread_t               msgRequestorTid;            //! Thread id that made the message request
	                                                    //! \note Only set for response messages
	size_t                  msgLengthWithDataValues;    //! Serialized message length with the actual data values
	size_t                  msgLengthWithPtrValues;     //! Serialized message length with pointers to the actual data values
	size_t                  msgLengthWithDataValuesExceptForArrays;
	                                                    //! Serialized message length with the actual data values except for
	                                                    //! pointers to array data
	int32_t                 msg_IO_VectorCount;         //! Number of elements in the I/O vector
	txp::Id     			msgId;                      //! Message id
	State					msgState;                   //! Message state
	Version 				msgVersion;                 //! Messate version
	HeapBuffer*             msgHeapBuffer;              //! Pointer to heap buffer
    struct iovec*           msg_IO_Vector;              //! Pointer to I/O vector
	AttributeMap*           msgAttrMap;                 //! Pointer to the message attribute map
	std::vector<Attribute*>*	msgAttributes;          //! Pointer to the message attribute vector
};

} // namespace
#if MSG_STALE_CHECK
txp::Msg* checkForStaleMsg(uint64_t pDeclareStale=600);
#endif

#endif /* MSG_H_ */
