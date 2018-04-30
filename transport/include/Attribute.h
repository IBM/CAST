/*******************************************************************************
 |    Attribute.h
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
 * \file  Attribute.h
 * \brief Header file for the message Attribute() class.
 *
 * \ingroup transport
 */

#ifndef ATTRIBUTE_H_
#define ATTRIBUTE_H_

#include <stdio.h>

#include <utility>
#include <vector>

#include "Common.h"

#include "crc.h"
#include "HeapBuffer.h"
#include "Log.h"

namespace txp {

//*****************************************************************************
//  Macros
//*****************************************************************************

/**
 * \brief During serialize, adds data value to buffer
 *
 * \param[in]   CLASS Class of object
 * \param[in]   CRC   Optional calculated CRC for the data
 */
#define ADD_DATA(CLASS,CRC) { \
    l_RC = Attribute::add(pBuffer, CRC); \
    if (!l_RC) { \
        if (CRC) { \
            size_t l_Length = LENGTH_OF_ATTR_TYPE+LENGTH_OF_ATTR_NAME; \
            unsigned char* l_Buffer = (unsigned char*)(pBuffer->getDataBufferPtr() + pBuffer->offset - l_Length); \
        	*CRC = Crc32n(*CRC, l_Buffer, l_Length); \
        } \
        l_RC = pBuffer->add((void*)&(data), sizeof(data)); \
        if (!l_RC) { \
            if (CRC) { \
                unsigned char* l_Buffer = (unsigned char*)(pBuffer->getDataBufferPtr() + pBuffer->offset - sizeof(data)); \
            	*CRC = Crc32n(*CRC, l_Buffer, sizeof(data)); \
            } \
        } else { \
            l_RC = -403; \
        } \
    } \
}

/**
 * \brief During serialize, adds pointer to data value to buffer
 *
 * \param[in]   CLASS Class of object
 * \param[in]   CRC   Optional calculated CRC for the data
 */
#define ADD_DATA_VALUE_WITH_PTR(CLASS,CRC) { \
    l_RC = Attribute::addValue(pBuffer, CRC); \
    if (!l_RC) { \
        if (CRC) { \
            size_t l_Length = LENGTH_OF_ATTR_TYPE + LENGTH_OF_ATTR_NAME; \
            unsigned char* l_Buffer = (unsigned char*)(pBuffer->getDataBufferPtr() + pBuffer->offset - l_Length); \
        	*CRC = Crc32n(*CRC, l_Buffer, l_Length); \
        } \
        l_RC = pBuffer->add((void*)&(*data), sizeof(*data)); \
        if (!l_RC) { \
            if (CRC) { \
                unsigned char* l_Buffer = (unsigned char*)(pBuffer->getDataBufferPtr() + pBuffer->offset - sizeof(*data)); \
            	*CRC = Crc32n(*CRC, l_Buffer, sizeof(*data)); \
            } \
        } else { \
            l_RC = -406; \
        } \
    } \
}

/**
 * \brief Instantiates a derived attribute class object
 *
 * \param[in]   CLASS Class of object
 */
#define BUILD_ATTR(CLASS) { \
    int l_RC = 0; \
    oAttribute = new CLASS(pName, pData); \
    if (!l_RC) { \
        oAttribute->setAllocatedFlag(1); \
    } else { \
        if (oAttribute) { \
            delete oAttribute; \
            oAttribute = 0; \
        } \
    } \
    return l_RC; \
}

/**
 * \brief Copies data value from attribute
 *
 * \param[in]   CLASS Class of object
 */
#define CPY_DATA(CLASS) { \
    ssize_t l_DataLength = getDataLength(); \
    if (l_DataLength == (ssize_t)pLength) { \
        memcpy(pTarget, getDataPtr(), l_DataLength); \
    } else { \
        l_RC = l_DataLength * -1; \
    } \
}

typedef std::pair<size_t, char*> CharArray_Element;
typedef std::vector<CharArray_Element> CharArray;
typedef CharArray::iterator CharArrayIter;

/**
 * \class Attribute
 * Identifies a data element to be associated with a Msg() object.  Each attribute
 * has a type, name and data value.
 *
 * Attributes are 'added' to a Msg() object and the order in which they are added
 * is preserved.
 */
class Attribute {
public:
    friend class Msg;

    // Constants
    static const size_t OFFSET_TO_ATTR_TYPE = 0;
    static const size_t OFFSET_TO_ATTR_NAME = OFFSET_TO_ATTR_TYPE + LENGTH_OF_ATTR_TYPE;

    // Static data
#ifdef TXP_DEVELOPMENT
    static txp::Log d_log;
    static char d_log_buffer[255];
#endif

    // Inlined static methods

    /**
     * \brief Convert attribute name to a printable array of character data.
     *
     * \param[in]   pName Attribute name.
     * \param[out]  pBuffer Output buffer for character data.
     * \param[in]   pSize Size available in buffer for character data.
     */
    static void attrNameToChar(const txp::AttributeName pName, char* pBuffer, const int32_t pSize);

    /**
     * \brief Convert attribute type to a printable array of character data.
     *
     * \param[in]   pType Attribute type.
     * \param[out]  pBuffer Output buffer for character data.
     * \param[in]   pSize Size available in buffer for character data.
     */
    static void attrTypeToChar(const txp::AttributeType pType, char* pBuffer, const int32_t pSize);

    /**
     * \brief Build and return addressability to an attribute as defined by input.
     *
     * \param[in]   pType Attribute type.
     * \param[in]   pName Attribute name.
     * \param[out]  oAttribute Pointer to returned attribute.
     * \return      int  0 -> success;  <0 -> failure
     * \par Error codes
     * Any error return code from new().
     * \note If the attribute is added to a message, the storage used for the attribute
     *       will be deleted when the Msg() object is deleted.  If the attribute is NOT
     *       added to a Msg() object, it is the invoker's responsibility to delete this
     *       attribute object.
     */
    static int buildAttr(const txp::AttributeType pType, const txp::AttributeName pName, txp::Attribute* &oAttribute);

    /**
     * \brief Build and return addressability to an attribute as defined in a serialized buffer.
     *
     * \param[in]       pBuffer Serialized buffer.
     * \param[in]       pOffset Offset within serialized buffer.
     * \param[out]      oAttribute Pointer to returned attribute.
     * \param[out]      pLength Returned length consumed in the serialized buffer for this attribute.
     * \param[in]       pOption Deserialize option.
     * \param[inout]    pCRC Pointer to CRC value.  Null indicates no CRC to be calculated..  Null indicates no CRC to be calculated.
     * \return          int  0 -> success;  <0 -> failure
     * \par Error codes
     * -419 If deserialize option is DO_NOT_COPY_DATA or COPY_DATA_TO_HEAP, and attribute type is invalid.\n
     * -420 If deserialize option is DO_NOT_COPY_DATA or COPY_DATA_TO_HEAP, and attribute type value is invalid.\n
     * -421 If deserialize option is COPY_DATA_TO_OBJECT or COPY_DATA_TO_OBJECT_BUT_DO_NOT_COPY_ARRAYS, and attribute type is invalid.\n
     * -422 If deserialize option is COPY_DATA_TO_OBJECT or COPY_DATA_TO_OBJECT_BUT_DO_NOT_COPY_ARRAYS, and attribute type value is invalid.\n
     * -423 Invalid deserialize option.\n
     * Any error return code from new().
     */
    static int buildAttr(txp::HeapBuffer* pBuffer, const size_t pOffset, txp::Attribute* &oAttribute, size_t &pLength, const txp::DeserializeOption &pOption, unsigned long* pCRC);

    // Pure virtual methods

    /**
     * \brief Clone attribute.
     *
     * \param[out]  pTargetAttr Pointer to target attribute.
     * \return      int  0 -> success;  <0 -> failure
     */
    virtual int clone(Attribute* &pTargetAttr)=0;

    /**
     * \brief Copy data value.
     *
     * \param[in]   pTarget Pointer to target of copy operation.
     * \param[in]   pLength Actual length of data to be copied.
     * \return      int  0 -> success;  <0 -> absval give required length
     */
    virtual int cpyData(void* pTarget, const size_t pLength)=0;

    /**
     * \brief Return length of data
     *
     * \return      ssize_t Length of data.
     * \note If the attribute type is a pointer to the data, the length returned
     *       by this method is the actual length of the data value.
     */
    virtual ssize_t getDataLength()=0;

    /**
     * \brief Return pointer to data
     *
     * \return  void* Pointer to the data.
     */
    virtual void* getDataPtr()=0;

    // Virtual methods

    /**
     * \brief Dumps Attribute to a Log.
     *
     * \param[in]   pLog
     * \param[in]   pPrefix Text to identify this operation.  Default is no prefix data.
     */
    virtual void dump(txp::Log& pLog, const char* pPrefix=0);

    /**
     * \brief Destructor
     */
    virtual ~Attribute();

    // Inlined non-static methods

    /**
     * \brief Get attribute name
     *
     * \return      AttributeName Attribute name
     */
    inline txp::AttributeName getAttrName() { return attrName; };

    /**
     * \brief Get attribute type
     *
     * \return      AttributeType Attribute type
     */
    inline txp::AttributeType getAttrType() { return attrType; };

    /**
     * \brief Is Added To Message
     *
     * \return      int  0 -> not added;  1 -> added
     */
    inline int isAddedToMsg() { return attrFlag.addedToMsg; };

    /**
     * \brief Is Storage Allocated for Attribute by Facility
     *
     * \return      int  0 -> not allocated;  1 -> allocated
     */
    inline int isAllocated() { return attrFlag.allocated; };

    /**
     * \brief Is Storage Allocated for Data by Facility
     *
     * \return      int  0 -> not allocated;  1 -> allocated
     */
    inline int isDataAllocated() { return attrFlag.dataAllocated; };

    /**
     * \brief Is Pointer Attribute Type
     *
     * \return      int  0 -> not pointer type;  1 -> pointer type
     */
    inline int isPtrAttrType() { return ((attrType & 0x00000100) >> 8); };

    /**
     * \brief Is Pointer Attribute Type
     *
     * \return      int  0 -> not pointer to array type;  1 -> pointer to array type
     */
    inline int isPtrToArrayAttrType() { return (attrType == PTR_CHAR_ARRAY || attrType == PTR_ARRAY_OF_CHAR_ARRAYS ? 1 : 0);};

    /**
     * \brief Set Storage Allocated for Attribute() by Facility.
     *
     * \return      int  0 -> not allocated;  1 -> allocated
     */
    inline void setAllocatedFlag(const int pValue) { attrFlag.allocated = pValue; return; };

protected:

    // Static methods

    /**
     * \brief Converts a non-pointer attribute type to the associated pointer attribute type.
     *
     * \param[in]   pType Attribute type.
     * \return      AttributeType Associated pointer attribute type.
     */
    static txp::AttributeType ConvertToPtrAttrType(txp::AttributeType pType) { return (txp::AttributeType)(pType | 0x00000100); };

    /**
     * \brief Converts a pointer attribute type to the associated non-pointer attribute type.
     *
     * \param[in]   pType Attribute type.
     * \return      AttributeType Associated non-pointer attribute type.
     */
    static txp::AttributeType ConvertToValueAttrType(txp::AttributeType pType) { return (txp::AttributeType)(pType & 0x0000FEFF); };

    /**
     * \brief Is Attribute Name Valid
     *
     * \param[in]   pAttrName Attribute Name
     * \return      int  0 -> not valid;  1 -> valid
     */
    static int isAttrNameValid(const txp::AttributeName pAttrName);

    /**
     * \brief Is Attribute Name Value Valid
     *
     * \param[in]   pAttrNameValue Attribute Name value
     * \return      int  0 -> not valid;  1 -> valid
     */
    static int isAttrNameValid(const int32_t pAttrNameValue);

    /**
     * \brief Is Attribute Type Valid
     *
     * \param[in]   pAttrType Attribute Type
     * \return      int  0 -> not valid;  1 -> valid
     */
    static int isAttrTypeValid(const txp::AttributeType pAttrType);

    /**
     * \brief Is Attribute Type Value Valid
     *
     * \param[in]   pAttrTypeValue Attribute Type value
     * \return      int  0 -> not valid;  1 -> valid
     */
    static int isAttrTypeValid(const int32_t pAttrTypeValue);

    // Virtual methods

    /**
     * \brief Adds the Attribute data to a buffer.
     * This adds a pointer to the data into the heap buffer.
     *
     * \param[in]       pBuffer Pointer to heap buffer
     * \param[inout]    pCRC Pointer to CRC value.  Null indicates no CRC to be calculated.
     * \return          int  0 -> success;  <0 -> failure
     * \par Error codes
     * -401 Failure when adding the attribute type.\n
     * -402 Failure when adding the attribute name.\n
     * -403 Failure when adding the attribute name.
     */
    virtual int add(txp::HeapBuffer* pBuffer, unsigned long* pCRC);

    /**
     * \brief Adds the Attribute data value to a buffer.
     * This adds a copy of the data into the heap buffer.
     *
     * \param[in]       pBuffer Pointer to heap buffer
     * \param[inout]    pCRC Pointer to CRC value.  Null indicates no CRC to be calculated.
     * \return          int  0 -> success;  <0 -> failure
     * \par Error codes
     * -404 Failure when adding the attribute type.\n
     * -405 Failure when adding the attribute name.\n
     * -406 Failure when adding the attribute name.
     */
    virtual int addValue(txp::HeapBuffer* pBuffer, unsigned long* pCRC);

    /**
     * \brief Get pointer to the length of char array.
     *
     * \return      void* Pointer to length of char array.
     */
    virtual void* getLengthCharArrayAddr() { return 0; };

    /**
     * \brief Get pointer to the array of char arrays number of elements.
     *
     * \return      void* Pointer to array of char arrays number of elements.
     */
    virtual void* getNumberOfElementsArrayOfCharArraysAddr() { return 0; };

    /**
     * \brief Get length of attribute as serialized in buffer with pointer to the data.
     *
     * \return      size_t Length of attribute as a pointer to the data.
     */
    virtual size_t getLengthInBuffer();

    /**
     * \brief Get length of attribute as serialized real data value in buffer.
     *
     * \return      size_t Length of attribute with real data copied into the buffer.
     */
    virtual size_t getLengthOfValueInBuffer();

    /**
     * \brief Get length of attribute prior to the data portion of the attribute.
     *
     * \return      size_t Length of attribute prior to the data portion.
     */
    virtual size_t getLengthPriorToData() { return Attribute::getLengthInBuffer(); };

    // Inlined, non-virtual methods

    /**
     * \brief Get address of attribute name
     *
     * \return      AttributeName* Pointer to attribute name
     */
    inline txp::AttributeName* getAttrNameAddr() { return &attrName; };

    /**
     * \brief Get address of attribute type
     *
     * \return      AttributeType* Pointer to attribute type
     */
    inline txp::AttributeType* getAttrTypeAddr() { return &attrType; };

    /**
     * \brief Set Attribute Added to Message flag.
     *
     * \return      int  0 -> not added;  1 -> added
     */
    inline void setAddedToMsg(const int pValue) { attrFlag.addedToMsg = pValue; return; };

    /**
     * \brief Set Storage Allocated for the data by Facility.
     *
     * \return      int  0 -> not allocated;  1 -> allocated
     */
    inline void setDataAllocatedFlag(const int pValue) { attrFlag.dataAllocated = pValue; return; };

    /**
     * \brief Initialization
     *
     */
    inline void init() {
        if (txp::init()) {
            // NOTE:  The default Attribute log is created before the configuration file is read/processed.
            //        Set those values from the configuration files for the default log.
#ifdef TXP_DEVELOPMENT
            d_log.setLogLevel(txp::Log::DEFAULT_OPEN_LOGLEVEL);
            if (txp::Log::DEFAULT_OPEN_LOGSTATE == txp::Log::OPEN) {
                d_log.open();
            } else {
                d_log.close();
            }
#endif
        }
    }

    // Constructors

    /**
     * \brief Default constructor
     *
     * Sets invalid type and name.
     */
    Attribute();

    /**
     * \brief Construct with type and name.
     *
     * \param[in]   pType   Attribute type
     * \param[in]   pName   Attribute name
     */
    Attribute(const txp::AttributeType pType, const txp::AttributeName pName);

    /**
     * \brief Construct from serialized data in buffer.
     *
     * \param[in]   pBuffer Pointer to heap buffer
     * \param[in]   pOffset Offset within buffer
     */
    Attribute(HeapBuffer* pBuffer, const size_t pOffset);

    /**
     * \brief Construct from serialized data in buffer, specifying type.
     *
     * \param[in]   pType   Attribute type
     * \param[in]   pBuffer Pointer to heap buffer
     * \param[in]   pOffset Offset within buffer
     */
    Attribute(const txp::AttributeType pType, HeapBuffer* pBuffer, const size_t pOffset);

    /**
     * \brief Copy constructor
     *
     * \param[in]   pAttribute
     */
    Attribute(const Attribute &pAttribute);

private:

    // Data members
    union {
        uint16_t    flags;
        struct {
            uint8_t reserved_1:8;

            uint8_t reserved_2:5;
            uint8_t addedToMsg:1;           //! Attribute() has been added to a Msg() object
                                            //! and therefore, cannot be added to another message.
                                            //! Such an attribute will be cloned first.
            uint8_t allocated:1;            //! Attribute() is allocated by message facility
                                            //! and therefore, will be deleted by the
                                            //! message facility.
            uint8_t dataAllocated:1;        //! Data associated with attribute is allocated
                                            //! by message facility and therefore, will be
                                            //! deleted by the message facility.
        };
    }  attrFlag;

    txp::AttributeType  attrType;           //! Attribute type
    txp::AttributeName  attrName;           //! Attribute name
};


//*****************************************************************************
//  DERIVED CLASSES from CLASS Attribute()
//
//  NOTE:  Each derived class defines a specific data type with the data.
//*****************************************************************************


class Attr_char : public Attribute {
public:
    // Static methods

    /**
     * \brief Build and return addressability to an attribute as defined by input.
     *
     * \param[in]   pName Attribute name.
     * \param[in]   pData Attribute data.
     * \param[out]  oAttribute Pointer to returned attribute.
     * \return      int  0 -> success;  <0 -> failure
     * \par Error codes
     * Any error return code from new().
     * \note If the attribute is added to a message, the storage used for the attribute
     *       will be deleted when the Msg() object is deleted.  If the attribute is NOT
     *       added to a Msg() object, it is the invoker's responsibility to delete this
     *       attribute object.
     */
    static int buildAttr_char(const txp::AttributeName pName, const char& pData, Attr_char* &oAttribute);

    // Inlined pure virtual methods

    /**
     * \brief Return pointer to data
     *
     * \return  void* Pointer to the data.
     */
    void* getDataPtr() { return (void*)&data; };

    // Pure virtual methods

    /**
     * \brief Clone attribute.
     *
     * \param[out]  pTargetAttr Pointer to target attribute.
     * \return      int  0 -> success;  <0 -> failure
     */
    int clone(Attribute* &pTargetAttr);

    /**
     * \brief Copy data value.
     *
     * \param[in]   pTarget Pointer to target of copy operation.
     * \param[in]   pLength Actual length of data to be copied.
     * \return      int  0 -> success;  <0 -> absval give required length
     */
    int cpyData(void* pTarget, const size_t pLength);

    /**
     * \brief Return length of data
     *
     * \return      ssize_t Length of data.
     */
    ssize_t getDataLength();

    // Virtual methods

    /**
     * \brief Adds the Attribute data to a buffer.
     * This adds a pointer to the data into the heap buffer.
     *
     * \param[in]       pBuffer Pointer to heap buffer
     * \param[inout]    pCRC Pointer to CRC value.  Null indicates no CRC to be calculated.
     * \return          int  0 -> success;  <0 -> failure
     * \par Error codes
     * -401 Failure when adding the attribute type.\n
     * -402 Failure when adding the attribute name.\n
     * -403 Failure when adding the attribute name.
     */
    int add(HeapBuffer* pBuffer, unsigned long* pCRC);

    /**
     * \brief Adds the Attribute data value to a buffer.
     * This adds a copy of the data into the heap buffer.
     *
     * \param[in]       pBuffer Pointer to heap buffer
     * \param[inout]    pCRC Pointer to CRC value.  Null indicates no CRC to be calculated.
     * \return          int  0 -> success;  <0 -> failure
     * \par Error codes
     * -404 Failure when adding the attribute type.\n
     * -405 Failure when adding the attribute name.\n
     * -406 Failure when adding the attribute name.
     */
    int addValue(HeapBuffer* pBuffer, unsigned long* pCRC);

    /**
     * \brief Dumps Attribute to a Log.
     *
     * \param[in]   pLog
     * \param[in]   pPrefix Text to identify this operation.  Default is no prefix data.
     */
    void dump(Log& pLog, const char* pPrefix=0);

    // Non-static methods

    /**
     * \brief Get the data
     *
     * \return  char Data
     */
    char getData() { return data; };

    // Constructors

    /**
     * \brief Construct from name and data.
     *
     * \param[in]   pName Attribute name
     * \param[in]   pData Data
     */
    Attr_char(const txp::AttributeName pName, const char pData);

    /**
     * \brief Construct from serialized data in buffer.
     *
     * \param[in]   pBuffer Pointer to heap buffer
     * \param[in]   pOffset Offset within buffer
     * \param[in]   pOption Deserialize option
     * \param[in]   pDataLocation Data location
     */
    Attr_char(HeapBuffer* pBuffer, const size_t pOffset, const txp::DeserializeOption &pOption, const txp::DataLocation &pDataLocation);

    /**
     * \brief Copy constructor
     *
     * \param[in]   pAttribute
     */
    Attr_char(const Attr_char &pAttribute);

private:

    // Virtual methods

    /**
     * \brief Get length of attribute as serialized in buffer with pointer to the data.
     *
     * \return      size_t Length of attribute as a pointer to the data.
     */
    size_t getLengthInBuffer();

    /**
     * \brief Get length of attribute as serialized real data value in buffer.
     *
     * \return      size_t Length of attribute with real data copied into the buffer.
     */
    size_t getLengthOfValueInBuffer();

    // Data members
    char data;      //! Data
};

class Attr_uint8 : public Attribute {
public:
    // Static methods

    /**
     * \brief Build and return addressability to an attribute as defined by input.
     *
     * \param[in]   pName Attribute name.
     * \param[in]   pData Attribute data.
     * \param[out]  oAttribute Pointer to returned attribute.
     * \return      int  0 -> success;  <0 -> failure
     * \par Error codes
     * Any error return code from new().
     * \note If the attribute is added to a message, the storage used for the attribute
     *       will be deleted when the Msg() object is deleted.  If the attribute is NOT
     *       added to a Msg() object, it is the invoker's responsibility to delete this
     *       attribute object.
     */
    static int buildAttr_uint8(const txp::AttributeName pName, const uint8_t& pData, Attr_uint8* &oAttribute);

    // Inlined pure virtual methods

    /**
     * \brief Return pointer to data
     *
     * \return  void* Pointer to the data.
     */
    void* getDataPtr() { return (void*)&data; };

    // Pure virtual methods

    /**
     * \brief Clone attribute.
     *
     * \param[out]  pTargetAttr Pointer to target attribute.
     * \return      int  0 -> success;  <0 -> failure
     */
    int clone(Attribute* &pTargetAttr);

    /**
     * \brief Copy data value.
     *
     * \param[in]   pTarget Pointer to target of copy operation.
     * \param[in]   pLength Actual length of data to be copied.
     * \return      int  0 -> success;  <0 -> absval give required length
     */
    int cpyData(void* pTarget, const size_t pLength);

    /**
     * \brief Return length of data
     *
     * \return      ssize_t Length of data.
     */
    ssize_t getDataLength();

    // Virtual methods
    /**
     * \brief Adds the Attribute data to a buffer.
     * This adds a pointer to the data into the heap buffer.
     *
     * \param[in]       pBuffer Pointer to heap buffer
     * \param[inout]    pCRC Pointer to CRC value.  Null indicates no CRC to be calculated.
     * \return          int  0 -> success;  <0 -> failure
     * \par Error codes
     * -401 Failure when adding the attribute type.\n
     * -402 Failure when adding the attribute name.\n
     * -403 Failure when adding the attribute name.
     */
    int add(HeapBuffer* pBuffer, unsigned long* pCRC);

    /**
     * \brief Adds the Attribute data value to a buffer.
     * This adds a copy of the data into the heap buffer.
     *
     * \param[in]       pBuffer Pointer to heap buffer
     * \param[inout]    pCRC Pointer to CRC value.  Null indicates no CRC to be calculated.
     * \return          int  0 -> success;  <0 -> failure
     * \par Error codes
     * -404 Failure when adding the attribute type.\n
     * -405 Failure when adding the attribute name.\n
     * -406 Failure when adding the attribute name.
     */
    int addValue(HeapBuffer* pBuffer, unsigned long* pCRC);

    /**
     * \brief Dumps Attribute to a Log.
     *
     * \param[in]   pLog
     * \param[in]   pPrefix Text to identify this operation.  Default is no prefix data.
     */
    void dump(Log& pLog, const char* pPrefix=0);

    // Non-static methods

    /**
     * \brief Get the data
     *
     * \return  uint8_t Data
     */
    uint8_t getData() { return data; };

    // Constructors

    /**
     * \brief Construct from name and data.
     *
     * \param[in]   pName Attribute name
     * \param[in]   pData Data
     */
    Attr_uint8(const txp::AttributeName pName, const uint8_t pData);

    /**
     * \brief Construct from serialized data in buffer.
     *
     * \param[in]   pBuffer Pointer to heap buffer
     * \param[in]   pOffset Offset within buffer
     * \param[in]   pOption Deserialize option
     * \param[in]   pDataLocation Data location
     */
    Attr_uint8(HeapBuffer* pBuffer, const size_t pOffset, const txp::DeserializeOption &pOption, const txp::DataLocation &pDataLocation);

    /**
     * \brief Copy constructor
     *
     * \param[in]   pAttribute
     */
    Attr_uint8(const Attr_uint8 &pAttribute);

private:

    // Virtual methods

    /**
     * \brief Get length of attribute as serialized in buffer with pointer to the data.
     *
     * \return      size_t Length of attribute as a pointer to the data.
     */
    size_t getLengthInBuffer();

    /**
     * \brief Get length of attribute as serialized real data value in buffer.
     *
     * \return      size_t Length of attribute with real data copied into the buffer.
     */
    size_t getLengthOfValueInBuffer();

    // Data members
    uint8_t data;   //! Data
};

class Attr_uint16 : public Attribute {
public:
    // Static methods

    /**
     * \brief Build and return addressability to an attribute as defined by input.
     *
     * \param[in]   pName Attribute name.
     * \param[in]   pData Attribute data.
     * \param[out]  oAttribute Pointer to returned attribute.
     * \return      int  0 -> success;  <0 -> failure
     * \par Error codes
     * Any error return code from new().
     * \note If the attribute is added to a message, the storage used for the attribute
     *       will be deleted when the Msg() object is deleted.  If the attribute is NOT
     *       added to a Msg() object, it is the invoker's responsibility to delete this
     *       attribute object.
     */
    static int buildAttr_uint16(const txp::AttributeName pName, const uint16_t& pData, Attr_uint16* &oAttribute);

    // Inlined pure virtual methods

    /**
     * \brief Return pointer to data
     *
     * \return  void* Pointer to the data.
     */
    void* getDataPtr() { return (void*)&data; };

    // Pure virtual methods

    /**
     * \brief Clone attribute.
     *
     * \param[out]  pTargetAttr Pointer to target attribute.
     * \return      int  0 -> success;  <0 -> failure
     */
    int clone(Attribute* &pTargetAttr);

    /**
     * \brief Copy data value.
     *
     * \param[in]   pTarget Pointer to target of copy operation.
     * \param[in]   pLength Actual length of data to be copied.
     * \return      int  0 -> success;  <0 -> absval give required length
     */
    int cpyData(void* pTarget, const size_t pLength);

    /**
     * \brief Return length of data
     *
     * \return      ssize_t Length of data.
     */
    ssize_t getDataLength();

    // Virtual methods
    /**
     * \brief Adds the Attribute data to a buffer.
     * This adds a pointer to the data into the heap buffer.
     *
     * \param[in]       pBuffer Pointer to heap buffer
     * \param[inout]    pCRC Pointer to CRC value.  Null indicates no CRC to be calculated.
     * \return          int  0 -> success;  <0 -> failure
     * \par Error codes
     * -401 Failure when adding the attribute type.\n
     * -402 Failure when adding the attribute name.\n
     * -403 Failure when adding the attribute name.
     */
    int add(HeapBuffer* pBuffer, unsigned long* pCRC);

    /**
     * \brief Adds the Attribute data value to a buffer.
     * This adds a copy of the data into the heap buffer.
     *
     * \param[in]       pBuffer Pointer to heap buffer
     * \param[inout]    pCRC Pointer to CRC value.  Null indicates no CRC to be calculated.
     * \return          int  0 -> success;  <0 -> failure
     * \par Error codes
     * -404 Failure when adding the attribute type.\n
     * -405 Failure when adding the attribute name.\n
     * -406 Failure when adding the attribute name.
     */
    int addValue(HeapBuffer* pBuffer, unsigned long* pCRC);

    /**
     * \brief Dumps Attribute to a Log.
     *
     * \param[in]   pLog
     * \param[in]   pPrefix Text to identify this operation.  Default is no prefix data.
     */
    void dump(Log& pLog, const char* pPrefix=0);

    // Non-static methods

    /**
     * \brief Get the data
     *
     * \return  uint16_t Data
     */
    uint16_t getData() { return data; };

    // Constructors

    /**
     * \brief Construct from name and data.
     *
     * \param[in]   pName Attribute name
     * \param[in]   pData Data
     */
    Attr_uint16(const txp::AttributeName pName, const uint16_t pData);

    /**
     * \brief Construct from serialized data in buffer.
     *
     * \param[in]   pBuffer Pointer to heap buffer
     * \param[in]   pOffset Offset within buffer
     * \param[in]   pOption Deserialize option
     * \param[in]   pDataLocation Data location
     */
    Attr_uint16(HeapBuffer* pBuffer, const size_t pOffset, const txp::DeserializeOption &pOption, const txp::DataLocation &pDataLocation);

    /**
     * \brief Copy constructor
     *
     * \param[in]   pAttribute
     */
    Attr_uint16(const Attr_uint16 &pAttribute);

private:

    // Virtual methods

    /**
     * \brief Get length of attribute as serialized in buffer with pointer to the data.
     *
     * \return      size_t Length of attribute as a pointer to the data.
     */
    size_t getLengthInBuffer();

    /**
     * \brief Get length of attribute as serialized real data value in buffer.
     *
     * \return      size_t Length of attribute with real data copied into the buffer.
     */
    size_t getLengthOfValueInBuffer();

    // Data members
    uint16_t data;  //! Data
};

class Attr_uint32 : public Attribute {
public:
    // Static methods

    /**
     * \brief Build and return addressability to an attribute as defined by input.
     *
     * \param[in]   pName Attribute name.
     * \param[in]   pData Attribute data.
     * \param[out]  oAttribute Pointer to returned attribute.
     * \return      int  0 -> success;  <0 -> failure
     * \par Error codes
     * Any error return code from new().
     * \note If the attribute is added to a message, the storage used for the attribute
     *       will be deleted when the Msg() object is deleted.  If the attribute is NOT
     *       added to a Msg() object, it is the invoker's responsibility to delete this
     *       attribute object.
     */
    static int buildAttr_uint32(const txp::AttributeName pName, const uint32_t& pData, Attr_uint32* &oAttribute);

    // Inlined pure virtual methods

    /**
     * \brief Return pointer to data
     *
     * \return  void* Pointer to the data.
     */
    void* getDataPtr() { return (void*)&data; };

    // Pure virtual methods

    /**
     * \brief Clone attribute.
     *
     * \param[out]  pTargetAttr Pointer to target attribute.
     * \return      int  0 -> success;  <0 -> failure
     */
    int clone(Attribute* &pTargetAttr);

    /**
     * \brief Copy data value.
     *
     * \param[in]   pTarget Pointer to target of copy operation.
     * \param[in]   pLength Actual length of data to be copied.
     * \return      int  0 -> success;  <0 -> absval give required length
     */
    int cpyData(void* pTarget, const size_t pLength);

    /**
     * \brief Return length of data
     *
     * \return      ssize_t Length of data.
     */
    ssize_t getDataLength();

    // Virtual methods
    /**
     * \brief Adds the Attribute data to a buffer.
     * This adds a pointer to the data into the heap buffer.
     *
     * \param[in]       pBuffer Pointer to heap buffer
     * \param[inout]    pCRC Pointer to CRC value.  Null indicates no CRC to be calculated.
     * \return          int  0 -> success;  <0 -> failure
     * \par Error codes
     * -401 Failure when adding the attribute type.\n
     * -402 Failure when adding the attribute name.\n
     * -403 Failure when adding the attribute name.
     */
    int add(HeapBuffer* pBuffer, unsigned long* pCRC);

    /**
     * \brief Adds the Attribute data value to a buffer.
     * This adds a copy of the data into the heap buffer.
     *
     * \param[in]       pBuffer Pointer to heap buffer
     * \param[inout]    pCRC Pointer to CRC value.  Null indicates no CRC to be calculated.
     * \return          int  0 -> success;  <0 -> failure
     * \par Error codes
     * -404 Failure when adding the attribute type.\n
     * -405 Failure when adding the attribute name.\n
     * -406 Failure when adding the attribute name.
     */
    int addValue(HeapBuffer* pBuffer, unsigned long* pCRC);

    /**
     * \brief Dumps Attribute to a Log.
     *
     * \param[in]   pLog
     * \param[in]   pPrefix Text to identify this operation.  Default is no prefix data.
     */
    void dump(Log& pLog, const char* pPrefix=0);

    // Non-static methods

    /**
     * \brief Get the data
     *
     * \return  uint32_t Data
     */
    uint32_t getData() { return data; };

    // Constructors

    /**
     * \brief Construct from name and data.
     *
     * \param[in]   pName Attribute name
     * \param[in]   pData Data
     */
    Attr_uint32(const txp::AttributeName pName, const uint32_t pData);

    /**
     * \brief Construct from serialized data in buffer.
     *
     * \param[in]   pBuffer Pointer to heap buffer
     * \param[in]   pOffset Offset within buffer
     * \param[in]   pOption Deserialize option
     * \param[in]   pDataLocation Data location
     */
    Attr_uint32(HeapBuffer* pBuffer, const size_t pOffset, const txp::DeserializeOption &pOption, const txp::DataLocation &pDataLocation);

    /**
     * \brief Copy constructor
     *
     * \param[in]   pAttribute
     */
    Attr_uint32(const Attr_uint32 &pAttribute);

private:

    // Virtual methods

    /**
     * \brief Get length of attribute as serialized in buffer with pointer to the data.
     *
     * \return      size_t Length of attribute as a pointer to the data.
     */
    size_t getLengthInBuffer();

    /**
     * \brief Get length of attribute as serialized real data value in buffer.
     *
     * \return      size_t Length of attribute with real data copied into the buffer.
     */
    size_t getLengthOfValueInBuffer();

    // Data members
    uint32_t data;  //! Data
};

class Attr_uint64 : public Attribute {
public:
    // Static methods

    /**
     * \brief Build and return addressability to an attribute as defined by input.
     *
     * \param[in]   pName Attribute name.
     * \param[in]   pData Attribute data.
     * \param[out]  oAttribute Pointer to returned attribute.
     * \return      int  0 -> success;  <0 -> failure
     * \par Error codes
     * Any error return code from new().
     * \note If the attribute is added to a message, the storage used for the attribute
     *       will be deleted when the Msg() object is deleted.  If the attribute is NOT
     *       added to a Msg() object, it is the invoker's responsibility to delete this
     *       attribute object.
     */
    static int buildAttr_uint64(const txp::AttributeName pName, const uint64_t& pData, Attr_uint64* &oAttribute);

    // Inlined pure virtual methods

    /**
     * \brief Return pointer to data
     *
     * \return  void* Pointer to the data.
     */
    void* getDataPtr() { return (void*)&data; };

    // Pure virtual methods

    /**
     * \brief Clone attribute.
     *
     * \param[out]  pTargetAttr Pointer to target attribute.
     * \return      int  0 -> success;  <0 -> failure
     */
    int clone(Attribute* &pTargetAttr);

    /**
     * \brief Copy data value.
     *
     * \param[in]   pTarget Pointer to target of copy operation.
     * \param[in]   pLength Actual length of data to be copied.
     * \return      int  0 -> success;  <0 -> absval give required length
     */
    int cpyData(void* pTarget, const size_t pLength);

    /**
     * \brief Return length of data
     *
     * \return      ssize_t Length of data.
     */
    ssize_t getDataLength();

    // Virtual methods
    /**
     * \brief Adds the Attribute data to a buffer.
     * This adds a pointer to the data into the heap buffer.
     *
     * \param[in]       pBuffer Pointer to heap buffer
     * \param[inout]    pCRC Pointer to CRC value.  Null indicates no CRC to be calculated.
     * \return          int  0 -> success;  <0 -> failure
     * \par Error codes
     * -401 Failure when adding the attribute type.\n
     * -402 Failure when adding the attribute name.\n
     * -403 Failure when adding the attribute name.
     */
    int add(HeapBuffer* pBuffer, unsigned long* pCRC);

    /**
     * \brief Adds the Attribute data value to a buffer.
     * This adds a copy of the data into the heap buffer.
     *
     * \param[in]       pBuffer Pointer to heap buffer
     * \param[inout]    pCRC Pointer to CRC value.  Null indicates no CRC to be calculated.
     * \return          int  0 -> success;  <0 -> failure
     * \par Error codes
     * -404 Failure when adding the attribute type.\n
     * -405 Failure when adding the attribute name.\n
     * -406 Failure when adding the attribute name.
     */
    int addValue(HeapBuffer* pBuffer, unsigned long* pCRC);

    /**
     * \brief Dumps Attribute to a Log.
     *
     * \param[in]   pLog
     * \param[in]   pPrefix Text to identify this operation.  Default is no prefix data.
     */
    void dump(Log& pLog, const char* pPrefix=0);

    // Non-static methods

    /**
     * \brief Get the data
     *
     * \return  uint64_t Data
     */
    uint64_t getData() { return data; };

    // Constructors

    /**
     * \brief Construct from name and data.
     *
     * \param[in]   pName Attribute name
     * \param[in]   pData Data
     */
    Attr_uint64(const txp::AttributeName pName, const uint64_t pData);

    /**
     * \brief Construct from serialized data in buffer.
     *
     * \param[in]   pBuffer Pointer to heap buffer
     * \param[in]   pOffset Offset within buffer
     * \param[in]   pOption Deserialize option
     * \param[in]   pDataLocation Data location
     */
    Attr_uint64(HeapBuffer* pBuffer, const size_t pOffset, const txp::DeserializeOption &pOption, const txp::DataLocation &pDataLocation);

    /**
     * \brief Copy constructor
     *
     * \param[in]   pAttribute
     */
    Attr_uint64(const Attr_uint64 &pAttribute);

private:

    // Virtual methods

    /**
     * \brief Get length of attribute as serialized in buffer with pointer to the data.
     *
     * \return      size_t Length of attribute as a pointer to the data.
     */
    size_t getLengthInBuffer();

    /**
     * \brief Get length of attribute as serialized real data value in buffer.
     *
     * \return      size_t Length of attribute with real data copied into the buffer.
     */
    size_t getLengthOfValueInBuffer();

    // Data members
    uint64_t data;  //! Data
};

class Attr_int8 : public Attribute {
public:
    // Static methods

    /**
     * \brief Build and return addressability to an attribute as defined by input.
     *
     * \param[in]   pName Attribute name.
     * \param[in]   pData Attribute data.
     * \param[out]  oAttribute Pointer to returned attribute.
     * \return      int  0 -> success;  <0 -> failure
     * \par Error codes
     * Any error return code from new().
     * \note If the attribute is added to a message, the storage used for the attribute
     *       will be deleted when the Msg() object is deleted.  If the attribute is NOT
     *       added to a Msg() object, it is the invoker's responsibility to delete this
     *       attribute object.
     */
    static int buildAttr_int8(const txp::AttributeName pName, const int8_t& pData, Attr_int8* &oAttribute);

    // Inlined pure virtual methods

    /**
     * \brief Return pointer to data
     *
     * \return  void* Pointer to the data.
     */
    void* getDataPtr() { return (void*)&data; };

    // Pure virtual methods

    /**
     * \brief Clone attribute.
     *
     * \param[out]  pTargetAttr Pointer to target attribute.
     * \return      int  0 -> success;  <0 -> failure
     */
    int clone(Attribute* &pTargetAttr);

    /**
     * \brief Copy data value.
     *
     * \param[in]   pTarget Pointer to target of copy operation.
     * \param[in]   pLength Actual length of data to be copied.
     * \return      int  0 -> success;  <0 -> absval give required length
     */
    int cpyData(void* pTarget, const size_t pLength);

    /**
     * \brief Return length of data
     *
     * \return      ssize_t Length of data.
     */
    ssize_t getDataLength();

    // Virtual methods
    /**
     * \brief Adds the Attribute data to a buffer.
     * This adds a pointer to the data into the heap buffer.
     *
     * \param[in]       pBuffer Pointer to heap buffer
     * \param[inout]    pCRC Pointer to CRC value.  Null indicates no CRC to be calculated.
     * \return          int  0 -> success;  <0 -> failure
     * \par Error codes
     * -401 Failure when adding the attribute type.\n
     * -402 Failure when adding the attribute name.\n
     * -403 Failure when adding the attribute name.
     */
    int add(HeapBuffer* pBuffer, unsigned long* pCRC);

    /**
     * \brief Adds the Attribute data value to a buffer.
     * This adds a copy of the data into the heap buffer.
     *
     * \param[in]       pBuffer Pointer to heap buffer
     * \param[inout]    pCRC Pointer to CRC value.  Null indicates no CRC to be calculated.
     * \return          int  0 -> success;  <0 -> failure
     * \par Error codes
     * -404 Failure when adding the attribute type.\n
     * -405 Failure when adding the attribute name.\n
     * -406 Failure when adding the attribute name.
     */
    int addValue(HeapBuffer* pBuffer, unsigned long* pCRC);

    /**
     * \brief Dumps Attribute to a Log.
     *
     * \param[in]   pLog
     * \param[in]   pPrefix Text to identify this operation.  Default is no prefix data.
     */
    void dump(Log& pLog, const char* pPrefix=0);

    // Non-static methods

    /**
     * \brief Get the data
     *
     * \return  int8_t Data
     */
    int8_t getData() { return data; };

    // Constructors

    /**
     * \brief Construct from name and data.
     *
     * \param[in]   pName Attribute name
     * \param[in]   pData Data
     */
    Attr_int8(const txp::AttributeName pName, const int8_t pData);

    /**
     * \brief Construct from serialized data in buffer.
     *
     * \param[in]   pBuffer Pointer to heap buffer
     * \param[in]   pOffset Offset within buffer
     * \param[in]   pOption Deserialize option
     * \param[in]   pDataLocation Data location
     */
    Attr_int8(HeapBuffer* pBuffer, const size_t pOffset, const txp::DeserializeOption &pOption, const txp::DataLocation &pDataLocation);

    /**
     * \brief Copy constructor
     *
     * \param[in]   pAttribute
     */
    Attr_int8(const Attr_int8 &pAttribute);

private:

    // Virtual methods

    /**
     * \brief Get length of attribute as serialized in buffer with pointer to the data.
     *
     * \return      size_t Length of attribute as a pointer to the data.
     */
    size_t getLengthInBuffer();

    /**
     * \brief Get length of attribute as serialized real data value in buffer.
     *
     * \return      size_t Length of attribute with real data copied into the buffer.
     */
    size_t getLengthOfValueInBuffer();

    // Data members
    int8_t data;    //! Data
};

class Attr_int16 : public Attribute {
public:
    // Static methods

    /**
     * \brief Build and return addressability to an attribute as defined by input.
     *
     * \param[in]   pName Attribute name.
     * \param[in]   pData Attribute data.
     * \param[out]  oAttribute Pointer to returned attribute.
     * \return      int  0 -> success;  <0 -> failure
     * \par Error codes
     * Any error return code from new().
     * \note If the attribute is added to a message, the storage used for the attribute
     *       will be deleted when the Msg() object is deleted.  If the attribute is NOT
     *       added to a Msg() object, it is the invoker's responsibility to delete this
     *       attribute object.
     */
    static int buildAttr_int16(txp::AttributeName pName, const int16_t& pData, Attr_int16* &oAttribute);

    // Inlined pure virtual methods

    /**
     * \brief Return pointer to data
     *
     * \return  void* Pointer to the data.
     */
    void* getDataPtr() { return (void*)&data; };

    // Pure virtual methods

    /**
     * \brief Clone attribute.
     *
     * \param[out]  pTargetAttr Pointer to target attribute.
     * \return      int  0 -> success;  <0 -> failure
     */
    int clone(Attribute* &pTargetAttr);

    /**
     * \brief Copy data value.
     *
     * \param[in]   pTarget Pointer to target of copy operation.
     * \param[in]   pLength Actual length of data to be copied.
     * \return      int  0 -> success;  <0 -> absval give required length
     */
    int cpyData(void* pTarget, const size_t pLength);

    /**
     * \brief Return length of data
     *
     * \return      ssize_t Length of data.
     */
    ssize_t getDataLength();

    // Virtual methods
    /**
     * \brief Adds the Attribute data to a buffer.
     * This adds a pointer to the data into the heap buffer.
     *
     * \param[in]       pBuffer Pointer to heap buffer
     * \param[inout]    pCRC Pointer to CRC value.  Null indicates no CRC to be calculated.
     * \return          int  0 -> success;  <0 -> failure
     * \par Error codes
     * -401 Failure when adding the attribute type.\n
     * -402 Failure when adding the attribute name.\n
     * -403 Failure when adding the attribute name.
     */
    int add(HeapBuffer* pBuffer, unsigned long* pCRC);

    /**
     * \brief Adds the Attribute data value to a buffer.
     * This adds a copy of the data into the heap buffer.
     *
     * \param[in]       pBuffer Pointer to heap buffer
     * \param[inout]    pCRC Pointer to CRC value.  Null indicates no CRC to be calculated.
     * \return          int  0 -> success;  <0 -> failure
     * \par Error codes
     * -404 Failure when adding the attribute type.\n
     * -405 Failure when adding the attribute name.\n
     * -406 Failure when adding the attribute name.
     */
    int addValue(HeapBuffer* pBuffer, unsigned long* pCRC);

    /**
     * \brief Dumps Attribute to a Log.
     *
     * \param[in]   pLog
     * \param[in]   pPrefix Text to identify this operation.  Default is no prefix data.
     */
    void dump(Log& pLog, const char* pPrefix=0);

    // Non-static methods

    /**
     * \brief Get the data
     *
     * \return  int16_t Data
     */
    int16_t getData() { return data; };

    // Constructors

    /**
     * \brief Construct from name and data.
     *
     * \param[in]   pName Attribute name
     * \param[in]   pData Data
     */
    Attr_int16(const txp::AttributeName pName, const int16_t pData);

    /**
     * \brief Construct from serialized data in buffer.
     *
     * \param[in]   pBuffer Pointer to heap buffer
     * \param[in]   pOffset Offset within buffer
     * \param[in]   pOption Deserialize option
     * \param[in]   pDataLocation Data location
     */
    Attr_int16(HeapBuffer* pBuffer, const size_t pOffset, const txp::DeserializeOption &pOption, const txp::DataLocation &pDataLocation);

    /**
     * \brief Copy constructor
     *
     * \param[in]   pAttribute
     */
    Attr_int16(const Attr_int16 &pAttribute);

private:

    // Virtual methods

    /**
     * \brief Get length of attribute as serialized in buffer with pointer to the data.
     *
     * \return      size_t Length of attribute as a pointer to the data.
     */
    size_t getLengthInBuffer();

    /**
     * \brief Get length of attribute as serialized real data value in buffer.
     *
     * \return      size_t Length of attribute with real data copied into the buffer.
     */
    size_t getLengthOfValueInBuffer();

    // Data members
    int16_t data;   //! Data
};

class Attr_int32 : public Attribute {
public:
    // Static methods

    /**
     * \brief Build and return addressability to an attribute as defined by input.
     *
     * \param[in]   pName Attribute name.
     * \param[in]   pData Attribute data.
     * \param[out]  oAttribute Pointer to returned attribute.
     * \return      int  0 -> success;  <0 -> failure
     * \par Error codes
     * Any error return code from new().
     * \note If the attribute is added to a message, the storage used for the attribute
     *       will be deleted when the Msg() object is deleted.  If the attribute is NOT
     *       added to a Msg() object, it is the invoker's responsibility to delete this
     *       attribute object.
     */
    static int buildAttr_int32(const txp::AttributeName pName, const int32_t& pData, Attr_int32* &oAttribute);

    // Inlined pure virtual methods

    /**
     * \brief Return pointer to data
     *
     * \return  void* Pointer to the data.
     */
    void* getDataPtr() { return (void*)&data; };

    // Pure virtual methods

    /**
     * \brief Clone attribute.
     *
     * \param[out]  pTargetAttr Pointer to target attribute.
     * \return      int  0 -> success;  <0 -> failure
     */
    int clone(Attribute* &pTargetAttr);

    /**
     * \brief Copy data value.
     *
     * \param[in]   pTarget Pointer to target of copy operation.
     * \param[in]   pLength Actual length of data to be copied.
     * \return      int  0 -> success;  <0 -> absval give required length
     */
    int cpyData(void* pTarget, const size_t pLength);

    /**
     * \brief Return length of data
     *
     * \return      ssize_t Length of data.
     */
    ssize_t getDataLength();

    // Virtual methods
    /**
     * \brief Adds the Attribute data to a buffer.
     * This adds a pointer to the data into the heap buffer.
     *
     * \param[in]       pBuffer Pointer to heap buffer
     * \param[inout]    pCRC Pointer to CRC value.  Null indicates no CRC to be calculated.
     * \return          int  0 -> success;  <0 -> failure
     * \par Error codes
     * -401 Failure when adding the attribute type.\n
     * -402 Failure when adding the attribute name.\n
     * -403 Failure when adding the attribute name.
     */
    int add(HeapBuffer* pBuffer, unsigned long* pCRC);

    /**
     * \brief Adds the Attribute data value to a buffer.
     * This adds a copy of the data into the heap buffer.
     *
     * \param[in]       pBuffer Pointer to heap buffer
     * \param[inout]    pCRC Pointer to CRC value.  Null indicates no CRC to be calculated.
     * \return          int  0 -> success;  <0 -> failure
     * \par Error codes
     * -404 Failure when adding the attribute type.\n
     * -405 Failure when adding the attribute name.\n
     * -406 Failure when adding the attribute name.
     */
    int addValue(HeapBuffer* pBuffer, unsigned long* pCRC);

    /**
     * \brief Dumps Attribute to a Log.
     *
     * \param[in]   pLog
     * \param[in]   pPrefix Text to identify this operation.  Default is no prefix data.
     */
    void dump(Log& pLog, const char* pPrefix=0);

    // Non-static methods

    /**
     * \brief Get the data
     *
     * \return  int32_t Data
     */
    int32_t getData() { return data; };

    // Constructors

    /**
     * \brief Construct from name and data.
     *
     * \param[in]   pName Attribute name
     * \param[in]   pData Data
     */
    Attr_int32(const txp::AttributeName pName, const int32_t pData);

    /**
     * \brief Construct from serialized data in buffer.
     *
     * \param[in]   pBuffer Pointer to heap buffer
     * \param[in]   pOffset Offset within buffer
     * \param[in]   pOption Deserialize option
     * \param[in]   pDataLocation Data location
     */
    Attr_int32(HeapBuffer* pBuffer, const size_t pOffset, const txp::DeserializeOption &pOption, const txp::DataLocation &pDataLocation);

    /**
     * \brief Copy constructor
     *
     * \param[in]   pAttribute
     */
    Attr_int32(const Attr_int32 &pAttribute);

private:

    // Virtual methods

    /**
     * \brief Get length of attribute as serialized in buffer with pointer to the data.
     *
     * \return      size_t Length of attribute as a pointer to the data.
     */
    size_t getLengthInBuffer();

    /**
     * \brief Get length of attribute as serialized real data value in buffer.
     *
     * \return      size_t Length of attribute with real data copied into the buffer.
     */
    size_t getLengthOfValueInBuffer();

    // Data members
    int32_t data;   //! Data
};

class Attr_int64 : public Attribute {
public:
    // Static methods

    /**
     * \brief Build and return addressability to an attribute as defined by input.
     *
     * \param[in]   pName Attribute name.
     * \param[in]   pData Attribute data.
     * \param[out]  oAttribute Pointer to returned attribute.
     * \return      int  0 -> success;  <0 -> failure
     * \par Error codes
     * Any error return code from new().
     * \note If the attribute is added to a message, the storage used for the attribute
     *       will be deleted when the Msg() object is deleted.  If the attribute is NOT
     *       added to a Msg() object, it is the invoker's responsibility to delete this
     *       attribute object.
     */
    static int buildAttr_int64(const txp::AttributeName pName, const int64_t& pData, Attr_int64* &oAttribute);

    // Inlined pure virtual methods

    /**
     * \brief Return pointer to data
     *
     * \return  void* Pointer to the data.
     */
    void* getDataPtr() { return (void*)&data; };

    // Pure virtual methods

    /**
     * \brief Clone attribute.
     *
     * \param[out]  pTargetAttr Pointer to target attribute.
     * \return      int  0 -> success;  <0 -> failure
     */
    int clone(Attribute* &pTargetAttr);

    /**
     * \brief Copy data value.
     *
     * \param[in]   pTarget Pointer to target of copy operation.
     * \param[in]   pLength Actual length of data to be copied.
     * \return      int  0 -> success;  <0 -> absval give required length
     */
    int cpyData(void* pTarget, const size_t pLength);

    /**
     * \brief Return length of data
     *
     * \return      ssize_t Length of data.
     */
    ssize_t getDataLength();

    // Virtual methods
    /**
     * \brief Adds the Attribute data to a buffer.
     * This adds a pointer to the data into the heap buffer.
     *
     * \param[in]       pBuffer Pointer to heap buffer
     * \param[inout]    pCRC Pointer to CRC value.  Null indicates no CRC to be calculated.
     * \return          int  0 -> success;  <0 -> failure
     * \par Error codes
     * -401 Failure when adding the attribute type.\n
     * -402 Failure when adding the attribute name.\n
     * -403 Failure when adding the attribute name.
     */
    int add(HeapBuffer* pBuffer, unsigned long* pCRC);

    /**
     * \brief Adds the Attribute data value to a buffer.
     * This adds a copy of the data into the heap buffer.
     *
     * \param[in]       pBuffer Pointer to heap buffer
     * \param[inout]    pCRC Pointer to CRC value.  Null indicates no CRC to be calculated.
     * \return          int  0 -> success;  <0 -> failure
     * \par Error codes
     * -404 Failure when adding the attribute type.\n
     * -405 Failure when adding the attribute name.\n
     * -406 Failure when adding the attribute name.
     */
    int addValue(HeapBuffer* pBuffer, unsigned long* pCRC);

    /**
     * \brief Dumps Attribute to a Log.
     *
     * \param[in]   pLog
     * \param[in]   pPrefix Text to identify this operation.  Default is no prefix data.
     */
    void dump(Log& pLog, const char* pPrefix=0);

    // Non-static methods

    /**
     * \brief Get the data
     *
     * \return  int64_t Data
     */
    int64_t getData() { return data; };

    // Constructors

    /**
     * \brief Construct from name and data.
     *
     * \param[in]   pName Attribute name
     * \param[in]   pData Data
     */
    Attr_int64(const txp::AttributeName pName, const int64_t pData);

    /**
     * \brief Construct from serialized data in buffer.
     *
     * \param[in]   pBuffer Pointer to heap buffer
     * \param[in]   pOffset Offset within buffer
     * \param[in]   pOption Deserialize option
     * \param[in]   pDataLocation Data location
     */
    Attr_int64(HeapBuffer* pBuffer, const size_t pOffset, const txp::DeserializeOption &pOption, const txp::DataLocation &pDataLocation);

    /**
     * \brief Copy constructor
     *
     * \param[in]   pAttribute
     */
    Attr_int64(const Attr_int64 &pAttribute);

private:

    // Virtual methods

    /**
     * \brief Get length of attribute as serialized in buffer with pointer to the data.
     *
     * \return      size_t Length of attribute as a pointer to the data.
     */
    size_t getLengthInBuffer();

    /**
     * \brief Get length of attribute as serialized real data value in buffer.
     *
     * \return      size_t Length of attribute with real data copied into the buffer.
     */
    size_t getLengthOfValueInBuffer();

    // Data members
    int64_t data;   //! Data
};

class AttrPtr_char : public Attribute {
public:
    // Static methods

    /**
     * \brief Build and return addressability to an attribute as defined by input.
     *
     * \param[in]   pName Attribute name.
     * \param[in]   pData Attribute data.
     * \note The storage associated with pData should not be freed until the associated Msg() object is destroyed.
     * \param[out]  oAttribute Pointer to returned attribute.
     * \return      int  0 -> success;  <0 -> failure
     * \par Error codes
     * Any error return code from new().
     * \note If the attribute is added to a message, the storage used for the attribute
     *       will be deleted when the Msg() object is deleted.  If the attribute is NOT
     *       added to a Msg() object, it is the invoker's responsibility to delete this
     *       attribute object.
     */
    static int buildAttrPtr_char(const txp::AttributeName pName, const char* pData, AttrPtr_char* &oAttribute);

    /**
     * \brief Destructor
     */
    ~AttrPtr_char();

    // Inlined pure virtual methods

    /**
     * \brief Return pointer to data
     *
     * \return  void* Pointer to the data.
     */
    void* getDataPtr() { return (void*)data; };

    // Pure virtual methods

    /**
     * \brief Clone attribute.
     *
     * \param[out]  pTargetAttr Pointer to target attribute.
     * \return      int  0 -> success;  <0 -> failure
     */
    int clone(Attribute* &pTargetAttr);

    /**
     * \brief Copy data value.
     *
     * \param[in]   pTarget Pointer to target of copy operation.
     * \param[in]   pLength Actual length of data to be copied.
     * \return      int  0 -> success;  <0 -> absval give required length
     */
    int cpyData(void* pTarget, const size_t pLength);

    /**
     * \brief Return length of data
     *
     * \return      ssize_t Length of data.
     * \note The length returned by this method is the actual length of the
     *       character data value.
     */
    ssize_t getDataLength();

    // Virtual methods
    /**
     * \brief Adds the Attribute data to a buffer.
     * This adds a pointer to the data into the heap buffer.
     *
     * \param[in]       pBuffer Pointer to heap buffer
     * \param[inout]    pCRC Pointer to CRC value.  Null indicates no CRC to be calculated.
     * \return          int  0 -> success;  <0 -> failure
     * \par Error codes
     * -401 Failure when adding the attribute type.\n
     * -402 Failure when adding the attribute name.\n
     * -403 Failure when adding the attribute name.
     */
    int add(HeapBuffer* pBuffer, unsigned long* pCRC);

    /**
     * \brief Adds the Attribute data value to a buffer.
     * This adds a copy of the data into the heap buffer.
     *
     * \param[in]       pBuffer Pointer to heap buffer
     * \param[inout]    pCRC Pointer to CRC value.  Null indicates no CRC to be calculated.
     * \return          int  0 -> success;  <0 -> failure
     * \par Error codes
     * -404 Failure when adding the attribute type.\n
     * -405 Failure when adding the attribute name.\n
     * -406 Failure when adding the attribute name.
     */
    int addValue(HeapBuffer* pBuffer, unsigned long* pCRC);

    /**
     * \brief Dumps Attribute to a Log.
     *
     * \param[in]   pLog
     * \param[in]   pPrefix Text to identify this operation.  Default is no prefix data.
     */
    void dump(Log& pLog, const char* pPrefix=0);

    // Non-static methods

    /**
     * \brief Get the data
     *
     * \return  char Data
     */
    char getData() { return *data; };

    // Constructors

    /**
     * \brief Construct from name and data.
     *
     * \param[in]   pName Attribute name
     * \param[in]   pData Pointer to data
     * \note The storage associated with pData should not be freed until the associated Msg() object is destroyed.
     */
    AttrPtr_char(const txp::AttributeName pName, const char* pData);

    /**
     * \brief Construct from serialized data in buffer.
     *
     * \param[in]   pBuffer Pointer to heap buffer
     * \param[in]   pOffset Offset within buffer
     * \param[in]   pOption Deserialize option
     * \param[in]   pDataLocation Data location
     */
    AttrPtr_char(HeapBuffer* pBuffer, const size_t pOffset, const txp::DeserializeOption &pOption, const txp::DataLocation &pDataLocation);

    /**
     * \brief Copy constructor
     *
     * \param[in]   pAttribute
     */
    AttrPtr_char(const AttrPtr_char &pAttribute);

private:

    // Virtual methods

    /**
     * \brief Get length of attribute as serialized in buffer with pointer to the data.
     *
     * \return      size_t Length of attribute as a pointer to the data.
     */
    size_t getLengthInBuffer();

    /**
     * \brief Get length of attribute as serialized real data value in buffer.
     *
     * \return      size_t Length of attribute with real data copied into the buffer.
     */
    size_t getLengthOfValueInBuffer();

    // Data members
    char*   data;   //! Data
};

class AttrPtr_uint8 : public Attribute {
public:
    // Static methods

    /**
     * \brief Build and return addressability to an attribute as defined by input.
     *
     * \param[in]   pName Attribute name.
     * \param[in]   pData Attribute data.
     * \note The storage associated with pData should not be freed until the associated Msg() object is destroyed.
     * \param[out]  oAttribute Pointer to returned attribute.
     * \return      int  0 -> success;  <0 -> failure
     * \par Error codes
     * Any error return code from new().
     * \note If the attribute is added to a message, the storage used for the attribute
     *       will be deleted when the Msg() object is deleted.  If the attribute is NOT
     *       added to a Msg() object, it is the invoker's responsibility to delete this
     *       attribute object.
     */
    static int buildAttrPtr_uint8(const txp::AttributeName pName, const uint8_t* pData, AttrPtr_uint8* &oAttribute);

    /**
     * \brief Destructor
     */
    ~AttrPtr_uint8();

    // Inlined pure virtual methods

    /**
     * \brief Return pointer to data
     *
     * \return  void* Pointer to the data.
     */
    void* getDataPtr() { return (void*)data; };

    // Pure virtual methods

    /**
     * \brief Clone attribute.
     *
     * \param[out]  pTargetAttr Pointer to target attribute.
     * \return      int  0 -> success;  <0 -> failure
     */
    int clone(Attribute* &pTargetAttr);

    /**
     * \brief Copy data value.
     *
     * \param[in]   pTarget Pointer to target of copy operation.
     * \param[in]   pLength Actual length of data to be copied.
     * \return      int  0 -> success;  <0 -> absval give required length
     */
    int cpyData(void* pTarget, const size_t pLength);

    /**
     * \brief Return length of data
     *
     * \return      ssize_t Length of data.
     * \note The length returned by this method is the actual length of the
     *       unsigned interger data value.
     */
    ssize_t getDataLength();

    // Virtual methods
    /**
     * \brief Adds the Attribute data to a buffer.
     * This adds a pointer to the data into the heap buffer.
     *
     * \param[in]       pBuffer Pointer to heap buffer
     * \param[inout]    pCRC Pointer to CRC value.  Null indicates no CRC to be calculated.
     * \return          int  0 -> success;  <0 -> failure
     * \par Error codes
     * -401 Failure when adding the attribute type.\n
     * -402 Failure when adding the attribute name.\n
     * -403 Failure when adding the attribute name.
     */
    int add(HeapBuffer* pBuffer, unsigned long* pCRC);

    /**
     * \brief Adds the Attribute data value to a buffer.
     * This adds a copy of the data into the heap buffer.
     *
     * \param[in]       pBuffer Pointer to heap buffer
     * \param[inout]    pCRC Pointer to CRC value.  Null indicates no CRC to be calculated.
     * \return          int  0 -> success;  <0 -> failure
     * \par Error codes
     * -404 Failure when adding the attribute type.\n
     * -405 Failure when adding the attribute name.\n
     * -406 Failure when adding the attribute name.
     */
    int addValue(HeapBuffer* pBuffer, unsigned long* pCRC);

    /**
     * \brief Dumps Attribute to a Log.
     *
     * \param[in]   pLog
     * \param[in]   pPrefix Text to identify this operation.  Default is no prefix data.
     */
    void dump(Log& pLog, const char* pPrefix=0);

    // Non-static methods

    /**
     * \brief Get the data
     *
     * \return  uint8_t Data
     */
    uint8_t getData() { return *data; };

    // Constructors

    /**
     * \brief Construct from name and data.
     *
     * \param[in]   pName Attribute name
     * \param[in]   pData Pointer to data
     * \note The storage associated with pData should not be freed until the associated Msg() object is destroyed.
     */
    AttrPtr_uint8(const txp::AttributeName pName, const uint8_t* pData);

    /**
     * \brief Construct from serialized data in buffer.
     *
     * \param[in]   pBuffer Pointer to heap buffer
     * \param[in]   pOffset Offset within buffer
     * \param[in]   pOption Deserialize option
     * \param[in]   pDataLocation Data location
     */
    AttrPtr_uint8(HeapBuffer* pBuffer, const size_t pOffset, const txp::DeserializeOption &pOption, const txp::DataLocation &pDataLocation);

    /**
     * \brief Copy constructor
     *
     * \param[in]   pAttribute
     */
    AttrPtr_uint8(const AttrPtr_uint8 &pAttribute);

private:

    // Virtual methods

    /**
     * \brief Get length of attribute as serialized in buffer with pointer to the data.
     *
     * \return      size_t Length of attribute as a pointer to the data.
     */
    size_t getLengthInBuffer();

    /**
     * \brief Get length of attribute as serialized real data value in buffer.
     *
     * \return      size_t Length of attribute with real data copied into the buffer.
     */
    size_t getLengthOfValueInBuffer();

    // Data members
    uint8_t* data;  //! Data
};

class AttrPtr_uint16 : public Attribute {
public:
    // Static methods

    /**
     * \brief Build and return addressability to an attribute as defined by input.
     *
     * \param[in]   pName Attribute name.
     * \param[in]   pData Attribute data.
     * \note The storage associated with pData should not be freed until the associated Msg() object is destroyed.
     * \param[out]  oAttribute Pointer to returned attribute.
     * \return      int  0 -> success;  <0 -> failure
     * \par Error codes
     * Any error return code from new().
     * \note If the attribute is added to a message, the storage used for the attribute
     *       will be deleted when the Msg() object is deleted.  If the attribute is NOT
     *       added to a Msg() object, it is the invoker's responsibility to delete this
     *       attribute object.
     */
    static int buildAttrPtr_uint16(const txp::AttributeName pName, const uint16_t* pData, AttrPtr_uint16* &oAttribute);

    /**
     * \brief Destructor
     */
    ~AttrPtr_uint16();

    // Inlined pure virtual methods

    /**
     * \brief Return pointer to data
     *
     * \return  void* Pointer to the data.
     */
    void* getDataPtr() { return (void*)data; };

    // Pure virtual methods

    /**
     * \brief Clone attribute.
     *
     * \param[out]  pTargetAttr Pointer to target attribute.
     * \return      int  0 -> success;  <0 -> failure
     */
    int clone(Attribute* &pTargetAttr);

    /**
     * \brief Copy data value.
     *
     * \param[in]   pTarget Pointer to target of copy operation.
     * \param[in]   pLength Actual length of data to be copied.
     * \return      int  0 -> success;  <0 -> absval give required length
     */
    int cpyData(void* pTarget, const size_t pLength);

    /**
     * \brief Return length of data
     *
     * \return      ssize_t Length of data.
     * \note The length returned by this method is the actual length of the
     *       unsigned interger data value.
     */
    ssize_t getDataLength();

    // Virtual methods
    /**
     * \brief Adds the Attribute data to a buffer.
     * This adds a pointer to the data into the heap buffer.
     *
     * \param[in]       pBuffer Pointer to heap buffer
     * \param[inout]    pCRC Pointer to CRC value.  Null indicates no CRC to be calculated.
     * \return          int  0 -> success;  <0 -> failure
     * \par Error codes
     * -401 Failure when adding the attribute type.\n
     * -402 Failure when adding the attribute name.\n
     * -403 Failure when adding the attribute name.
     */
    int add(HeapBuffer* pBuffer, unsigned long* pCRC);

    /**
     * \brief Adds the Attribute data value to a buffer.
     * This adds a copy of the data into the heap buffer.
     *
     * \param[in]       pBuffer Pointer to heap buffer
     * \param[inout]    pCRC Pointer to CRC value.  Null indicates no CRC to be calculated.
     * \return          int  0 -> success;  <0 -> failure
     * \par Error codes
     * -404 Failure when adding the attribute type.\n
     * -405 Failure when adding the attribute name.\n
     * -406 Failure when adding the attribute name.
     */
    int addValue(HeapBuffer* pBuffer, unsigned long* pCRC);

    /**
     * \brief Dumps Attribute to a Log.
     *
     * \param[in]   pLog
     * \param[in]   pPrefix Text to identify this operation.  Default is no prefix data.
     */
    void dump(Log& pLog, const char* pPrefix=0);

    // Non-static methods

    /**
     * \brief Get the data
     *
     * \return  uint16_t Data
     */
    uint16_t getData() { return *data; };

    // Constructors

    /**
     * \brief Construct from name and data.
     *
     * \param[in]   pName Attribute name
     * \param[in]   pData Pointer to data
     * \note The storage associated with pData should not be freed until the associated Msg() object is destroyed.
     */
    AttrPtr_uint16(const txp::AttributeName pName, const uint16_t* pData);

    /**
     * \brief Construct from serialized data in buffer.
     *
     * \param[in]   pBuffer Pointer to heap buffer
     * \param[in]   pOffset Offset within buffer
     * \param[in]   pOption Deserialize option
     * \param[in]   pDataLocation Data location
     */
    AttrPtr_uint16(HeapBuffer* pBuffer, const size_t pOffset, const txp::DeserializeOption &pOption, const txp::DataLocation &pDataLocation);

    /**
     * \brief Copy constructor
     *
     * \param[in]   pAttribute
     */
    AttrPtr_uint16(const AttrPtr_uint16 &pAttribute);

private:

    // Virtual methods

    /**
     * \brief Get length of attribute as serialized in buffer with pointer to the data.
     *
     * \return      size_t Length of attribute as a pointer to the data.
     */
    size_t getLengthInBuffer();

    /**
     * \brief Get length of attribute as serialized real data value in buffer.
     *
     * \return      size_t Length of attribute with real data copied into the buffer.
     */
    size_t getLengthOfValueInBuffer();

    // Data members
    uint16_t* data; //! Data
};

class AttrPtr_uint32 : public Attribute {
public:
    // Static methods

    /**
     * \brief Build and return addressability to an attribute as defined by input.
     *
     * \param[in]   pName Attribute name.
     * \param[in]   pData Attribute data.
     * \note The storage associated with pData should not be freed until the associated Msg() object is destroyed.
     * \param[out]  oAttribute Pointer to returned attribute.
     * \return      int  0 -> success;  <0 -> failure
     * \par Error codes
     * Any error return code from new().
     * \note If the attribute is added to a message, the storage used for the attribute
     *       will be deleted when the Msg() object is deleted.  If the attribute is NOT
     *       added to a Msg() object, it is the invoker's responsibility to delete this
     *       attribute object.
     */
    static int buildAttrPtr_uint32(const txp::AttributeName pName, const uint32_t* pData, AttrPtr_uint32* &oAttribute);

    /**
     * \brief Destructor
     */
    ~AttrPtr_uint32();

    // Inlined pure virtual methods

    /**
     * \brief Return pointer to data
     *
     * \return  void* Pointer to the data.
     */
    void* getDataPtr() { return (void*)data; };

    // Pure virtual methods

    /**
     * \brief Clone attribute.
     *
     * \param[out]  pTargetAttr Pointer to target attribute.
     * \return      int  0 -> success;  <0 -> failure
     */
    int clone(Attribute* &pTargetAttr);

    /**
     * \brief Copy data value.
     *
     * \param[in]   pTarget Pointer to target of copy operation.
     * \param[in]   pLength Actual length of data to be copied.
     * \return      int  0 -> success;  <0 -> absval give required length
     */
    int cpyData(void* pTarget, const size_t pLength);

    /**
     * \brief Return length of data
     *
     * \return      ssize_t Length of data.
     * \note The length returned by this method is the actual length of the
     *       unsigned interger data value.
     */
    ssize_t getDataLength();

    // Virtual methods
    /**
     * \brief Adds the Attribute data to a buffer.
     * This adds a pointer to the data into the heap buffer.
     *
     * \param[in]       pBuffer Pointer to heap buffer
     * \param[inout]    pCRC Pointer to CRC value.  Null indicates no CRC to be calculated.
     * \return          int  0 -> success;  <0 -> failure
     * \par Error codes
     * -401 Failure when adding the attribute type.\n
     * -402 Failure when adding the attribute name.\n
     * -403 Failure when adding the attribute name.
     */
    int add(HeapBuffer* pBuffer, unsigned long* pCRC);

    /**
     * \brief Adds the Attribute data value to a buffer.
     * This adds a copy of the data into the heap buffer.
     *
     * \param[in]       pBuffer Pointer to heap buffer
     * \param[inout]    pCRC Pointer to CRC value.  Null indicates no CRC to be calculated.
     * \return          int  0 -> success;  <0 -> failure
     * \par Error codes
     * -404 Failure when adding the attribute type.\n
     * -405 Failure when adding the attribute name.\n
     * -406 Failure when adding the attribute name.
     */
    int addValue(HeapBuffer* pBuffer, unsigned long* pCRC);

    /**
     * \brief Dumps Attribute to a Log.
     *
     * \param[in]   pLog
     * \param[in]   pPrefix Text to identify this operation.  Default is no prefix data.
     */
    void dump(Log& pLog, const char* pPrefix=0);

    // Non-static methods

    /**
     * \brief Get the data
     *
     * \return  uint32_t Data
     */
    uint32_t getData() { return *data; };

    // Constructors

    /**
     * \brief Construct from name and data.
     *
     * \param[in]   pName Attribute name
     * \param[in]   pData Pointer to data
     * \note The storage associated with pData should not be freed until the associated Msg() object is destroyed.
     */
    AttrPtr_uint32(const txp::AttributeName pName, const uint32_t* pData);

    /**
     * \brief Construct from serialized data in buffer.
     *
     * \param[in]   pBuffer Pointer to heap buffer
     * \param[in]   pOffset Offset within buffer
     * \param[in]   pOption Deserialize option
     * \param[in]   pDataLocation Data location
     */
    AttrPtr_uint32(HeapBuffer* pBuffer, const size_t pOffset, const txp::DeserializeOption &pOption, const txp::DataLocation &pDataLocation);

    /**
     * \brief Copy constructor
     *
     * \param[in]   pAttribute
     */
    AttrPtr_uint32(const AttrPtr_uint32 &pAttribute);

private:

    // Virtual methods

    /**
     * \brief Get length of attribute as serialized in buffer with pointer to the data.
     *
     * \return      size_t Length of attribute as a pointer to the data.
     */
    size_t getLengthInBuffer();

    /**
     * \brief Get length of attribute as serialized real data value in buffer.
     *
     * \return      size_t Length of attribute with real data copied into the buffer.
     */
    size_t getLengthOfValueInBuffer();

    // Data members
    uint32_t* data; //! Data
};

class AttrPtr_uint64 : public Attribute {
public:
    // Static methods

    /**
     * \brief Build and return addressability to an attribute as defined by input.
     *
     * \param[in]   pName Attribute name.
     * \param[in]   pData Attribute data.
     * \note The storage associated with pData should not be freed until the associated Msg() object is destroyed.
     * \param[out]  oAttribute Pointer to returned attribute.
     * \return      int  0 -> success;  <0 -> failure
     * \par Error codes
     * Any error return code from new().
     * \note If the attribute is added to a message, the storage used for the attribute
     *       will be deleted when the Msg() object is deleted.  If the attribute is NOT
     *       added to a Msg() object, it is the invoker's responsibility to delete this
     *       attribute object.
     */
    static int buildAttrPtr_uint64(const txp::AttributeName pName, const uint64_t* pData, AttrPtr_uint64* &oAttribute);

    /**
     * \brief Destructor
     */
    ~AttrPtr_uint64();

    // Inlined pure virtual methods

    /**
     * \brief Return pointer to data
     *
     * \return  void* Pointer to the data.
     */
    void* getDataPtr() { return (void*)data; };

    // Pure virtual methods

    /**
     * \brief Clone attribute.
     *
     * \param[out]  pTargetAttr Pointer to target attribute.
     * \return      int  0 -> success;  <0 -> failure
     */
    int clone(Attribute* &pTargetAttr);

    /**
     * \brief Copy data value.
     *
     * \param[in]   pTarget Pointer to target of copy operation.
     * \param[in]   pLength Actual length of data to be copied.
     * \return      int  0 -> success;  <0 -> absval give required length
     */
    int cpyData(void* pTarget, const size_t pLength);

    /**
     * \brief Return length of data
     *
     * \return      ssize_t Length of data.
     * \note The length returned by this method is the actual length of the
     *       unsigned interger data value.
     */
    ssize_t getDataLength();

    // Virtual methods
    /**
     * \brief Adds the Attribute data to a buffer.
     * This adds a pointer to the data into the heap buffer.
     *
     * \param[in]       pBuffer Pointer to heap buffer
     * \param[inout]    pCRC Pointer to CRC value.  Null indicates no CRC to be calculated.
     * \return          int  0 -> success;  <0 -> failure
     * \par Error codes
     * -401 Failure when adding the attribute type.\n
     * -402 Failure when adding the attribute name.\n
     * -403 Failure when adding the attribute name.
     */
    int add(HeapBuffer* pBuffer, unsigned long* pCRC);

    /**
     * \brief Adds the Attribute data value to a buffer.
     * This adds a copy of the data into the heap buffer.
     *
     * \param[in]       pBuffer Pointer to heap buffer
     * \param[inout]    pCRC Pointer to CRC value.  Null indicates no CRC to be calculated.
     * \return          int  0 -> success;  <0 -> failure
     * \par Error codes
     * -404 Failure when adding the attribute type.\n
     * -405 Failure when adding the attribute name.\n
     * -406 Failure when adding the attribute name.
     */
    int addValue(HeapBuffer* pBuffer, unsigned long* pCRC);

    /**
     * \brief Dumps Attribute to a Log.
     *
     * \param[in]   pLog
     * \param[in]   pPrefix Text to identify this operation.  Default is no prefix data.
     */
    void dump(Log& pLog, const char* pPrefix=0);

    // Non-static methods

    /**
     * \brief Get the data
     *
     * \return  uint64_t Data
     */
    uint64_t getData() { return *data; };

    // Constructors

    /**
     * \brief Construct from name and data.
     *
     * \param[in]   pName Attribute name
     * \param[in]   pData Pointer to data
     * \note The storage associated with pData should not be freed until the associated Msg() object is destroyed.
     */
    AttrPtr_uint64(const txp::AttributeName pName, const uint64_t* pData);

    /**
     * \brief Construct from serialized data in buffer.
     *
     * \param[in]   pBuffer Pointer to heap buffer
     * \param[in]   pOffset Offset within buffer
     * \param[in]   pOption Deserialize option
     * \param[in]   pDataLocation Data location
     */
    AttrPtr_uint64(HeapBuffer* pBuffer, const size_t pOffset, const txp::DeserializeOption &pOption, const txp::DataLocation &pDataLocation);

    /**
     * \brief Copy constructor
     *
     * \param[in]   pAttribute
     */
    AttrPtr_uint64(const AttrPtr_uint64 &pAttribute);

private:

    // Virtual methods

    /**
     * \brief Get length of attribute as serialized in buffer with pointer to the data.
     *
     * \return      size_t Length of attribute as a pointer to the data.
     */
    size_t getLengthInBuffer();

    /**
     * \brief Get length of attribute as serialized real data value in buffer.
     *
     * \return      size_t Length of attribute with real data copied into the buffer.
     */
    size_t getLengthOfValueInBuffer();

    // Data members
    uint64_t* data; //! Data
};

class AttrPtr_int8 : public Attribute {
public:
    // Static methods

    /**
     * \brief Build and return addressability to an attribute as defined by input.
     *
     * \param[in]   pName Attribute name.
     * \param[in]   pData Attribute data.
     * \note The storage associated with pData should not be freed until the associated Msg() object is destroyed.
     * \param[out]  oAttribute Pointer to returned attribute.
     * \return      int  0 -> success;  <0 -> failure
     * \par Error codes
     * Any error return code from new().
     * \note If the attribute is added to a message, the storage used for the attribute
     *       will be deleted when the Msg() object is deleted.  If the attribute is NOT
     *       added to a Msg() object, it is the invoker's responsibility to delete this
     *       attribute object.
     */
    static int buildAttrPtr_int8(const txp::AttributeName pName, const int8_t* pData, AttrPtr_int8* &oAttribute);

    /**
     * \brief Destructor
     */
    ~AttrPtr_int8();

    // Inlined pure virtual methods

    /**
     * \brief Return pointer to data
     *
     * \return  void* Pointer to the data.
     */
    void* getDataPtr() { return (void*)data; };

    // Pure virtual methods

    /**
     * \brief Clone attribute.
     *
     * \param[out]  pTargetAttr Pointer to target attribute.
     * \return      int  0 -> success;  <0 -> failure
     */
    int clone(Attribute* &pTargetAttr);

    /**
     * \brief Copy data value.
     *
     * \param[in]   pTarget Pointer to target of copy operation.
     * \param[in]   pLength Actual length of data to be copied.
     * \return      int  0 -> success;  <0 -> absval give required length
     */
    int cpyData(void* pTarget, const size_t pLength);

    /**
     * \brief Return length of data
     *
     * \return      ssize_t Length of data.
     * \note The length returned by this method is the actual length of the
     *       signed interger data value.
     */
    ssize_t getDataLength();

    // Virtual methods
    /**
     * \brief Adds the Attribute data to a buffer.
     * This adds a pointer to the data into the heap buffer.
     *
     * \param[in]       pBuffer Pointer to heap buffer
     * \param[inout]    pCRC Pointer to CRC value.  Null indicates no CRC to be calculated.
     * \return          int  0 -> success;  <0 -> failure
     * \par Error codes
     * -401 Failure when adding the attribute type.\n
     * -402 Failure when adding the attribute name.\n
     * -403 Failure when adding the attribute name.
     */
    int add(HeapBuffer* pBuffer, unsigned long* pCRC);

    /**
     * \brief Adds the Attribute data value to a buffer.
     * This adds a copy of the data into the heap buffer.
     *
     * \param[in]       pBuffer Pointer to heap buffer
     * \param[inout]    pCRC Pointer to CRC value.  Null indicates no CRC to be calculated.
     * \return          int  0 -> success;  <0 -> failure
     * \par Error codes
     * -404 Failure when adding the attribute type.\n
     * -405 Failure when adding the attribute name.\n
     * -406 Failure when adding the attribute name.
     */
    int addValue(HeapBuffer* pBuffer, unsigned long* pCRC);

    /**
     * \brief Dumps Attribute to a Log.
     *
     * \param[in]   pLog
     * \param[in]   pPrefix Text to identify this operation.  Default is no prefix data.
     */
    void dump(Log& pLog, const char* pPrefix=0);

    // Non-static methods

    /**
     * \brief Get the data
     *
     * \return  int8_t Data
     */
    int8_t getData() { return *data; };

    // Constructors

    /**
     * \brief Construct from name and data.
     *
     * \param[in]   pName Attribute name
     * \param[in]   pData Pointer to data
     * \note The storage associated with pData should not be freed until the associated Msg() object is destroyed.
     */
    AttrPtr_int8(const txp::AttributeName pName, const int8_t* pData);

    /**
     * \brief Construct from serialized data in buffer.
     *
     * \param[in]   pBuffer Pointer to heap buffer
     * \param[in]   pOffset Offset within buffer
     * \param[in]   pOption Deserialize option
     * \param[in]   pDataLocation Data location
     */
    AttrPtr_int8(HeapBuffer* pBuffer, const size_t pOffset, const txp::DeserializeOption &pOption, const txp::DataLocation &pDataLocation);

    /**
     * \brief Copy constructor
     *
     * \param[in]   pAttribute
     */
    AttrPtr_int8(const AttrPtr_int8 &pAttribute);

private:

    // Virtual methods

    /**
     * \brief Get length of attribute as serialized in buffer with pointer to the data.
     *
     * \return      size_t Length of attribute as a pointer to the data.
     */
    size_t getLengthInBuffer();

    /**
     * \brief Get length of attribute as serialized real data value in buffer.
     *
     * \return      size_t Length of attribute with real data copied into the buffer.
     */
    size_t getLengthOfValueInBuffer();

    // Data members
    int8_t* data;   //! Data
};

class AttrPtr_int16 : public Attribute {
public:
    // Static methods

    /**
     * \brief Build and return addressability to an attribute as defined by input.
     *
     * \param[in]   pName Attribute name.
     * \param[in]   pData Attribute data.
     * \note The storage associated with pData should not be freed until the associated Msg() object is destroyed.
     * \param[out]  oAttribute Pointer to returned attribute.
     * \return      int  0 -> success;  <0 -> failure
     * \par Error codes
     * Any error return code from new().
     * \note If the attribute is added to a message, the storage used for the attribute
     *       will be deleted when the Msg() object is deleted.  If the attribute is NOT
     *       added to a Msg() object, it is the invoker's responsibility to delete this
     *       attribute object.
     */
    static int buildAttrPtr_int16(const txp::AttributeName pName, const int16_t* pData, AttrPtr_int16* &oAttribute);

    /**
     * \brief Destructor
     */
    ~AttrPtr_int16();

    // Inlined pure virtual methods

    /**
     * \brief Return pointer to data
     *
     * \return  void* Pointer to the data.
     */
    void* getDataPtr() { return (void*)data; };

    // Pure virtual methods

    /**
     * \brief Clone attribute.
     *
     * \param[out]  pTargetAttr Pointer to target attribute.
     * \return      int  0 -> success;  <0 -> failure
     */
    int clone(Attribute* &pTargetAttr);

    /**
     * \brief Copy data value.
     *
     * \param[in]   pTarget Pointer to target of copy operation.
     * \param[in]   pLength Actual length of data to be copied.
     * \return      int  0 -> success;  <0 -> absval give required length
     */
    int cpyData(void* pTarget, const size_t pLength);

    /**
     * \brief Return length of data
     *
     * \return      ssize_t Length of data.
     * \note The length returned by this method is the actual length of the
     *       signed interger data value.
     */
    ssize_t getDataLength();

    // Virtual methods
    /**
     * \brief Adds the Attribute data to a buffer.
     * This adds a pointer to the data into the heap buffer.
     *
     * \param[in]       pBuffer Pointer to heap buffer
     * \param[inout]    pCRC Pointer to CRC value.  Null indicates no CRC to be calculated.
     * \return          int  0 -> success;  <0 -> failure
     * \par Error codes
     * -401 Failure when adding the attribute type.\n
     * -402 Failure when adding the attribute name.\n
     * -403 Failure when adding the attribute name.
     */
    int add(HeapBuffer* pBuffer, unsigned long* pCRC);

    /**
     * \brief Adds the Attribute data value to a buffer.
     * This adds a copy of the data into the heap buffer.
     *
     * \param[in]       pBuffer Pointer to heap buffer
     * \param[inout]    pCRC Pointer to CRC value.  Null indicates no CRC to be calculated.
     * \return          int  0 -> success;  <0 -> failure
     * \par Error codes
     * -404 Failure when adding the attribute type.\n
     * -405 Failure when adding the attribute name.\n
     * -406 Failure when adding the attribute name.
     */
    int addValue(HeapBuffer* pBuffer, unsigned long* pCRC);

    /**
     * \brief Dumps Attribute to a Log.
     *
     * \param[in]   pLog
     * \param[in]   pPrefix Text to identify this operation.  Default is no prefix data.
     */
    void dump(Log& pLog, const char* pPrefix=0);

    // Non-static methods

    /**
     * \brief Get the data
     *
     * \return  int16_t Data
     */
    int16_t getData() { return *data; };

    // Constructors

    /**
     * \brief Construct from name and data.
     *
     * \param[in]   pName Attribute name
     * \param[in]   pData Pointer to data
     * \note The storage associated with pData should not be freed until the associated Msg() object is destroyed.
     */
    AttrPtr_int16(const txp::AttributeName pName, const int16_t* pData);

    /**
     * \brief Construct from serialized data in buffer.
     *
     * \param[in]   pBuffer Pointer to heap buffer
     * \param[in]   pOffset Offset within buffer
     * \param[in]   pOption Deserialize option
     * \param[in]   pDataLocation Data location
     */
    AttrPtr_int16(HeapBuffer* pBuffer, const size_t pOffset, const txp::DeserializeOption &pOption, const txp::DataLocation &pDataLocation);

    /**
     * \brief Copy constructor
     *
     * \param[in]   pAttribute
     */
    AttrPtr_int16(const AttrPtr_int16 &pAttribute);

private:

    // Virtual methods

    /**
     * \brief Get length of attribute as serialized in buffer with pointer to the data.
     *
     * \return      size_t Length of attribute as a pointer to the data.
     */
    size_t getLengthInBuffer();

    /**
     * \brief Get length of attribute as serialized real data value in buffer.
     *
     * \return      size_t Length of attribute with real data copied into the buffer.
     */
    size_t getLengthOfValueInBuffer();

    // Data members
    int16_t* data;  //! Data
};

class AttrPtr_int32 : public Attribute {
public:
    // Static methods

    /**
     * \brief Build and return addressability to an attribute as defined by input.
     *
     * \param[in]   pName Attribute name.
     * \param[in]   pData Attribute data.
     * \note The storage associated with pData should not be freed until the associated Msg() object is destroyed.
     * \param[out]  oAttribute Pointer to returned attribute.
     * \return      int  0 -> success;  <0 -> failure
     * \par Error codes
     * Any error return code from new().
     * \note If the attribute is added to a message, the storage used for the attribute
     *       will be deleted when the Msg() object is deleted.  If the attribute is NOT
     *       added to a Msg() object, it is the invoker's responsibility to delete this
     *       attribute object.
     */
    static int buildAttrPtr_int32(const txp::AttributeName pName, const int32_t* pData, AttrPtr_int32* &oAttribute);

    /**
     * \brief Destructor
     */
    ~AttrPtr_int32();

    // Inlined pure virtual methods

    /**
     * \brief Return pointer to data
     *
     * \return  void* Pointer to the data.
     */
    void* getDataPtr() { return (void*)data; };

    // Pure virtual methods

    /**
     * \brief Clone attribute.
     *
     * \param[out]  pTargetAttr Pointer to target attribute.
     * \return      int  0 -> success;  <0 -> failure
     */
    int clone(Attribute* &pTargetAttr);

    /**
     * \brief Copy data value.
     *
     * \param[in]   pTarget Pointer to target of copy operation.
     * \param[in]   pLength Actual length of data to be copied.
     * \return      int  0 -> success;  <0 -> absval give required length
     */
    int cpyData(void* pTarget, const size_t pLength);

    /**
     * \brief Return length of data
     *
     * \return      ssize_t Length of data.
     * \note The length returned by this method is the actual length of the
     *       signed interger data value.
     */
    ssize_t getDataLength();

    // Virtual methods
    /**
     * \brief Adds the Attribute data to a buffer.
     * This adds a pointer to the data into the heap buffer.
     *
     * \param[in]       pBuffer Pointer to heap buffer
     * \param[inout]    pCRC Pointer to CRC value.  Null indicates no CRC to be calculated.
     * \return          int  0 -> success;  <0 -> failure
     * \par Error codes
     * -401 Failure when adding the attribute type.\n
     * -402 Failure when adding the attribute name.\n
     * -403 Failure when adding the attribute name.
     */
    int add(HeapBuffer* pBuffer, unsigned long* pCRC);

    /**
     * \brief Adds the Attribute data value to a buffer.
     * This adds a copy of the data into the heap buffer.
     *
     * \param[in]       pBuffer Pointer to heap buffer
     * \param[inout]    pCRC Pointer to CRC value.  Null indicates no CRC to be calculated.
     * \return          int  0 -> success;  <0 -> failure
     * \par Error codes
     * -404 Failure when adding the attribute type.\n
     * -405 Failure when adding the attribute name.\n
     * -406 Failure when adding the attribute name.
     */
    int addValue(HeapBuffer* pBuffer, unsigned long* pCRC);

    /**
     * \brief Dumps Attribute to a Log.
     *
     * \param[in]   pLog
     * \param[in]   pPrefix Text to identify this operation.  Default is no prefix data.
     */
    void dump(Log& pLog, const char* pPrefix=0);

    // Non-static methods

    /**
     * \brief Get the data
     *
     * \return  int32_t Data
     */
    int32_t getData() { return *data; };

    // Constructors

    /**
     * \brief Construct from name and data.
     *
     * \param[in]   pName Attribute name
     * \param[in]   pData Pointer to data
     * \note The storage associated with pData should not be freed until the associated Msg() object is destroyed.
     */
    AttrPtr_int32(const txp::AttributeName pName, const int32_t* pData);

    /**
     * \brief Construct from serialized data in buffer.
     *
     * \param[in]   pBuffer Pointer to heap buffer
     * \param[in]   pOffset Offset within buffer
     * \param[in]   pOption Deserialize option
     * \param[in]   pDataLocation Data location
     */
    AttrPtr_int32(HeapBuffer* pBuffer, const size_t pOffset, const txp::DeserializeOption &pOption, const txp::DataLocation &pDataLocation);

    /**
     * \brief Copy constructor
     *
     * \param[in]   pAttribute
     */
    AttrPtr_int32(const AttrPtr_int32 &pAttribute);

private:

    // Virtual methods

    /**
     * \brief Get length of attribute as serialized in buffer with pointer to the data.
     *
     * \return      size_t Length of attribute as a pointer to the data.
     */
    size_t getLengthInBuffer();

    /**
     * \brief Get length of attribute as serialized real data value in buffer.
     *
     * \return      size_t Length of attribute with real data copied into the buffer.
     */
    size_t getLengthOfValueInBuffer();

    // Data members
    int32_t* data;  //! Data
};

class AttrPtr_int64 : public Attribute {
public:
    // Static methods

    /**
     * \brief Build and return addressability to an attribute as defined by input.
     *
     * \param[in]   pName Attribute name.
     * \param[in]   pData Attribute data.
     * \note The storage associated with pData should not be freed until the associated Msg() object is destroyed.
     * \param[out]  oAttribute Pointer to returned attribute.
     * \return      int  0 -> success;  <0 -> failure
     * \par Error codes
     * Any error return code from new().
     * \note If the attribute is added to a message, the storage used for the attribute
     *       will be deleted when the Msg() object is deleted.  If the attribute is NOT
     *       added to a Msg() object, it is the invoker's responsibility to delete this
     *       attribute object.
     */
    static int buildAttrPtr_int64(const txp::AttributeName pName, const int64_t* pData, AttrPtr_int64* &oAttribute);

    /**
     * \brief Destructor
     */
    ~AttrPtr_int64();

    // Inlined pure virtual methods

    /**
     * \brief Return pointer to data
     *
     * \return  void* Pointer to the data.
     */
    void* getDataPtr() { return (void*)data; };

    // Pure virtual methods

    /**
     * \brief Clone attribute.
     *
     * \param[out]  pTargetAttr Pointer to target attribute.
     * \return      int  0 -> success;  <0 -> failure
     */
    int clone(Attribute* &pTargetAttr);

    /**
     * \brief Copy data value.
     *
     * \param[in]   pTarget Pointer to target of copy operation.
     * \param[in]   pLength Actual length of data to be copied.
     * \return      int  0 -> success;  <0 -> absval give required length
     */
    int cpyData(void* pTarget, const size_t pLength);

    /**
     * \brief Return length of data
     *
     * \return      ssize_t Length of data.
     * \note The length returned by this method is the actual length of the
     *       signed interger data value.
     */
    ssize_t getDataLength();

    // Virtual methods
    /**
     * \brief Adds the Attribute data to a buffer.
     * This adds a pointer to the data into the heap buffer.
     *
     * \param[in]       pBuffer Pointer to heap buffer
     * \param[inout]    pCRC Pointer to CRC value.  Null indicates no CRC to be calculated.
     * \return          int  0 -> success;  <0 -> failure
     * \par Error codes
     * -401 Failure when adding the attribute type.\n
     * -402 Failure when adding the attribute name.\n
     * -403 Failure when adding the attribute name.
     */
    int add(HeapBuffer* pBuffer, unsigned long* pCRC);

    /**
     * \brief Adds the Attribute data value to a buffer.
     * This adds a copy of the data into the heap buffer.
     *
     * \param[in]       pBuffer Pointer to heap buffer
     * \param[inout]    pCRC Pointer to CRC value.  Null indicates no CRC to be calculated.
     * \return          int  0 -> success;  <0 -> failure
     * \par Error codes
     * -404 Failure when adding the attribute type.\n
     * -405 Failure when adding the attribute name.\n
     * -406 Failure when adding the attribute name.
     */
    int addValue(HeapBuffer* pBuffer, unsigned long* pCRC);

    /**
     * \brief Dumps Attribute to a Log.
     *
     * \param[in]   pLog
     * \param[in]   pPrefix Text to identify this operation.  Default is no prefix data.
     */
    void dump(Log& pLog, const char* pPrefix=0);

    // Non-static methods

    /**
     * \brief Get the data
     *
     * \return  int64_t Data
     */
    int64_t getData() { return *data; };

    // Constructors

    /**
     * \brief Construct from name and data.
     *
     * \param[in]   pName Attribute name
     * \param[in]   pData Pointer to data
     * \note The storage associated with pData should not be freed until the associated Msg() object is destroyed.
     */
    AttrPtr_int64(const txp::AttributeName pName, const int64_t* pData);

    /**
     * \brief Construct from serialized data in buffer.
     *
     * \param[in]   pBuffer Pointer to heap buffer
     * \param[in]   pOffset Offset within buffer
     * \param[in]   pOption Deserialize option
     * \param[in]   pDataLocation Data location
     */
    AttrPtr_int64(HeapBuffer* pBuffer, const size_t pOffset, const txp::DeserializeOption &pOption, const txp::DataLocation &pDataLocation);

    /**
     * \brief Copy constructor
     *
     * \param[in]   pAttribute
     */
    AttrPtr_int64(const AttrPtr_int64 &pAttribute);

private:

    // Virtual methods

    /**
     * \brief Get length of attribute as serialized in buffer with pointer to the data.
     *
     * \return      size_t Length of attribute as a pointer to the data.
     */
    size_t getLengthInBuffer();

    /**
     * \brief Get length of attribute as serialized real data value in buffer.
     *
     * \return      size_t Length of attribute with real data copied into the buffer.
     */
    size_t getLengthOfValueInBuffer();

    // Data members
    int64_t* data;  //! Data
};

class AttrPtr_char_array : public Attribute {
public:
    // Static methods

    /**
     * \brief Build and return addressability to an attribute as defined by input.
     *
     * \param[in]   pName Attribute name.
     * \param[in]   pData Attribute data.
     * \note The storage associated with pData should not be freed until the associated Msg() object is destroyed.
     * \param[in]   pLength Length of data.
     * \param[out]  oAttribute Pointer to returned attribute.
     * \param[in]   pCopyDataOption  0 -> Do not copy data;  Otherwise, copy data and message facility owns data storage.
     * \return      int  0 -> success;  <0 -> failure
     * \par Error codes
     * Any error return code from new().
     * \note If the attribute is added to a message, the storage used for the attribute
     *       will be deleted when the Msg() object is deleted.  If the attribute is NOT
     *       added to a Msg() object, it is the invoker's responsibility to delete this
     *       attribute object.
     */
	static int buildAttrPtr_char_array(const txp::AttributeName pName, const char* pData, const size_t pLength, txp::AttrPtr_char_array* &oAttribute, const txp::COPY_DATA_OPTION pCopyDataOption=DO_NOT_COPY);

    /**
     * \brief Destructor
     */
    ~AttrPtr_char_array();

    // Inlined pure virtual methods

    /**
     * \brief Return pointer to data
     *
     * \return  void* Pointer to the data.
     */
    void* getDataPtr() { return (void*)data; };

    // Pure virtual methods

    /**
     * \brief Clone attribute.
     *
     * \param[out]  pTargetAttr Pointer to target attribute.
     * \return      int  0 -> success;  <0 -> failure
     */
    int clone(Attribute* &pTargetAttr);

    /**
     * \brief Copy data value.
     *
     * \param[in]   pTarget Pointer to target of copy operation.
     * \param[in]   pLength Actual length of data to be copied.
     * \return      int  0 -> success;  <0 -> absval give required length
     */
    int cpyData(void* pTarget, const size_t pLength);

    /**
     * \brief Return length of data
     *
     * \return      ssize_t Length of data.
     * \note The length returned by this method is the actual length of the
     *       char array data value.
     */
    ssize_t getDataLength();

    // Virtual methods
    /**
     * \brief Adds the Attribute data to a buffer.
     * This adds a pointer to the data into the heap buffer.
     *
     * \param[in]       pBuffer Pointer to heap buffer
     * \param[inout]    pCRC Pointer to CRC value.  Null indicates no CRC to be calculated.
     * \return          int  0 -> success;  <0 -> failure
     * \par Error codes
     * -401 Failure when adding the attribute type.\n
     * -402 Failure when adding the attribute name.\n
     * -403 Failure when adding the attribute name.\n
     * -407 Failure when adding the length of char array.\n
     * -408 Failure when adding the char array data.
     */
    int add(HeapBuffer* pBuffer, unsigned long* pCRC);

    /**
     * \brief Adds the Attribute data value to a buffer.
     * This adds a copy of the data into the heap buffer.
     *
     * \param[in]       pBuffer Pointer to heap buffer
     * \param[inout]    pCRC Pointer to CRC value.  Null indicates no CRC to be calculated.
     * \return          int  0 -> success;  <0 -> failure
     * \par Error codes
     * -404 Failure when adding the attribute type.\n
     * -405 Failure when adding the attribute name.\n
     * -406 Failure when adding the attribute name.
     * -409 Failure when adding the length of char array.\n
     * -410 Failure when adding the char array data.
     */
    int addValue(HeapBuffer* pBuffer, unsigned long* pCRC);

    /**
     * \brief Dumps Attribute to a Log.
     *
     * \param[in]   pLog
     * \param[in]   pPrefix Text to identify this operation.  Default is no prefix data.
     */
    void dump(Log& pLog, const char* pPrefix=0);

    // Inlined non-static methods

    /**
     * \brief Get a pointer to the char array length value
     *
     * \return  void* Pointer to char array length
     */
    void* getLengthCharArrayAddr() { return (void*)&length; };

    /**
     * \brief Get length of attribute prior to the data portion.
     *
     * \return  size_t Length of attribute prior to the data portion.
     */
    size_t getLengthPriorToData() { return Attribute::getLengthPriorToData() + sizeof(LENGTH_OF_ATTR_CHAR_ARRAY_LENGTH); };

    // Constructors

    /**
     * \brief Construct from name and data.
     *
     * \param[in]   pName Attribute name
     * \param[in]   pData Pointer to data
     * \note The storage associated with pData should not be freed until the associated Msg() object is destroyed.
     * \param[in]   pLength Length of data
     */
	AttrPtr_char_array(const txp::AttributeName pName, const char* pData, const size_t pLength);

    /**
     * \brief Construct from serialized data in buffer.
     *
     * \param[in]   pBuffer Pointer to heap buffer
     * \param[in]   pOffset Offset within buffer
     * \param[in]   pOption Deserialize option
     * \param[in]   pDataLocation Data location
     */
    AttrPtr_char_array(HeapBuffer* pBuffer, const size_t pOffset, const txp::DeserializeOption &pOption, const txp::DataLocation &pDataLocation);

    /**
     * \brief Copy constructor
     *
     * \param[in]   pAttribute
     */
    AttrPtr_char_array(const AttrPtr_char_array &pAttribute);

private:

    // Virtual methods

    /**
     * \brief Get length of attribute as serialized in buffer with pointer to the data.
     *
     * \return      size_t Length of attribute as a pointer to the data.
     */
    size_t getLengthInBuffer();

    /**
     * \brief Get length of attribute as serialized real data value in buffer.
     *
     * \return      size_t Length of attribute with real data copied into the buffer.
     */
    size_t getLengthOfValueInBuffer();

    // Data members
    size_t length;  //! Length of char array
	char* data;     //! Data
};

class AttrPtr_array_of_char_arrays : public Attribute {
public:
    // Static methods

    /**
     * \brief Build and return addressability to an attribute as defined by input.
     *
     * \param[in]   pName Attribute name.
     * \param[in]   pData Attribute data.
     * \param[out]  oAttribute Pointer to returned attribute.
     * \return      int  0 -> success;  <0 -> failure
     * \par Error codes
     * Any error return code from new().
     * \note If the attribute is added to a message, the storage used for the attribute
     *       will be deleted when the Msg() object is deleted.  If the attribute is NOT
     *       added to a Msg() object, it is the invoker's responsibility to delete this
     *       attribute object.
     *
     * \note Upon return from this static method, the storage to CharArray* can be freed and, if required,
     *       the storage for each of the CharArray_Element objects can be freed.  However, the storage for
     *       char* within each CharArray_Element should not be freed until the associated Msg() object is
     *       destroyed.
     */
	static int buildAttrPtr_array_of_char_arrays(const txp::AttributeName pName, const txp::CharArray* pData, txp::AttrPtr_array_of_char_arrays* &oAttribute);

    /**
     * \brief Destructor
     */
    ~AttrPtr_array_of_char_arrays();

    // Inlined pure virtual methods

    /**
     * \brief Return pointer to data
     *
     * \return  void* Pointer to the data.
     */
    void* getDataPtr() { return (void*)data; };

    // Pure virtual methods

    /**
     * \brief Clone attribute.
     *
     * \param[out]  pTargetAttr Pointer to target attribute.
     * \return      int  0 -> success;  <0 -> failure
     */
    int clone(Attribute* &pTargetAttr);

    /**
     * \brief Copy data value.
     *
     * \param[in]   pTarget Pointer to target of copy operation.
     * \param[in]   pLength Actual length of data to be copied.
     * \return      int  0 -> success;  <0 -> absval give required length
     */
    int cpyData(void* pTarget, const size_t pLength);

    /**
     * \brief Return length of data
     *
     * \return      ssize_t Length of data.
     * \note The length returned by this method is the TOTAL length of the
     *       array of char arrays data value.
     */
    ssize_t getDataLength();

    // Virtual methods
    /**
     * \brief Adds the Attribute data to a buffer.
     * This adds a pointer to the data into the heap buffer.
     *
     * \param[in]       pBuffer Pointer to heap buffer
     * \param[inout]    pCRC Pointer to CRC value.  Null indicates no CRC to be calculated.
     * \return          int  0 -> success;  <0 -> failure
     * \par Error codes
     * -401 Failure when adding the attribute type.\n
     * -402 Failure when adding the attribute name.\n
     * -403 Failure when adding the attribute name.
     * -411 Failure when adding the number of elements.\n
     * -412 Failure when adding the length of char array data.\n
     * -413 Failure when adding the size for an array element.\n
     * -414 Failure when adding the char array data.
     */
    int add(HeapBuffer* pBuffer, unsigned long* pCRC);

    /**
     * \brief Adds the Attribute data value to a buffer.
     * This adds a copy of the data into the heap buffer.
     *
     * \param[in]       pBuffer Pointer to heap buffer
     * \param[inout]    pCRC Pointer to CRC value.  Null indicates no CRC to be calculated.
     * \return          int  0 -> success;  <0 -> failure
     * \par Error codes
     * -404 Failure when adding the attribute type.\n
     * -405 Failure when adding the attribute name.\n
     * -406 Failure when adding the attribute name.
     * -415 Failure when adding the number of elements.\n
     * -416 Failure when adding the length of char array data.\n
     * -417 Failure when adding the size for an array element.\n
     * -418 Failure when adding the char array data.
     */
    int addValue(HeapBuffer* pBuffer, unsigned long* pCRC);

    /**
     * \brief Dumps Attribute to a Log.
     *
     * \param[in]   pLog
     * \param[in]   pPrefix Text to identify this operation.  Default is no prefix data.
     */
    void dump(Log& pLog, const char* pPrefix=0);

    // Inlined non-static methods

    /**
     * \brief Are Data Element Char Arrays Allocated
     *
     * \return  int  0 -> not allocated;  1 -> allocated
     */
    inline int areDataElementCharArraysAllocated() { return attrFlag2.dataElementCharArraysAllocated; };

    /**
     * \brief Get a pointer to the array of char arrays number of elements value
     *
     * \return  void* Pointer to number of elements value
     */
    inline void* getNumberOfElementsArrayOfCharArraysAddr() { return (void*)&number_of_elements; };

    /**
     * \brief Get the array of char arrays number of elements value
     *
     * \return  size_t  number of elements
     */
    inline size_t getNumberOfElementsArrayOfCharArrays() { return number_of_elements; };

    /**
     * \brief Get length of attribute prior to the data portion.
     *
     * \return  size_t Length of attribute prior to the data portion.
     */
    inline size_t getLengthPriorToData() { return Attribute::getLengthPriorToData() + sizeof(LENGTH_OF_ATTR_ARRAY_OF_CHAR_ARRAYS_NUMBER_OF_ELEMENTS) + sizeof(LENGTH_OF_ATTR_ARRAY_OF_CHAR_ARRAYS_LENGTH); };

    /**
     * \brief Set Data Element Chaar Arrays Allocated for the data by Facility.
     *
     * \return      int  0 -> not allocated;  1 -> allocated
     */
    inline void setDataElementCharArraysAllocatedFlag(const int pValue) { attrFlag2.dataElementCharArraysAllocated = pValue; return; };

    // Constructors

    /**
     * \brief Construct from name and data.
     *
     * \param[in]   pName Attribute name
     * \param[in]   pData Pointer to data
     *
     * \note Upon return from this constructor, the storage to CharArray* can be freed and, if required,
     *       the storage for each of the CharArray_Element objects can be freed.  However, the storage for
     *       char* within each CharArray_Element should not be freed until the associated Msg() object is
     *       destroyed.
     */
	AttrPtr_array_of_char_arrays(const txp::AttributeName pName, const txp::CharArray* pData);

    /**
     * \brief Construct from serialized data in buffer.
     *
     * \param[in]   pBuffer Pointer to heap buffer
     * \param[in]   pOffset Offset within buffer
     * \param[in]   pOption Deserialize option
     * \param[in]   pDataLocation Data location
     */
    AttrPtr_array_of_char_arrays(HeapBuffer* pBuffer, const size_t pOffset, const txp::DeserializeOption &pOption, const txp::DataLocation &pDataLocation);

    /**
     * \brief Copy constructor
     *
     * \param[in]   pAttribute
     */
    AttrPtr_array_of_char_arrays(const AttrPtr_array_of_char_arrays &pAttribute);

private:

    // Virtual methods

    /**
     * \brief Get length of attribute as serialized in buffer with pointer to the data.
     *
     * \return      size_t Length of attribute as a pointer to the data.
     */
    size_t getLengthInBuffer();

    /**
     * \brief Get length of attribute as serialized real data value in buffer.
     *
     * \return      size_t Length of attribute with real data copied into the buffer.
     */
    size_t getLengthOfValueInBuffer();

    // Data members
    union {
        uint16_t    flags2;
        struct {
            uint8_t reserved_1:8;

            uint8_t reserved_2:7;
            uint8_t dataElementCharArraysAllocated:1;
                                                //! Char arrays within data elements are allocated
                                                //! by message facility and therefore, will be deleted
                                                //! by the message facility.
        };
    }  attrFlag2;
    size_t number_of_elements;  //! Number of array elements
    size_t length_of_data;      //! Total byte length of CharArray data
    size_t length_of_values;    //! Total byte length of CharArray data values
	CharArray* data;            //! CharArray data
};

} // namespace

#endif /* ATTRIBUTE_H_ */
