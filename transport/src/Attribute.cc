/*******************************************************************************
 |    Attribute.cc
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

#include <string.h>

#include "Attribute.h"

//*****************************************************************************
//  Static data members
//*****************************************************************************
#ifdef TXP_DEVELOPMENT
txp::Log txp::Attribute::d_log(txp::Log::DEFAULT_LOG_DESTINATION, txp::Log::DEFAULT_OPEN_LOGSTATE, txp::Log::DEFAULT_OPEN_LOGLEVEL);
char txp::Attribute::d_log_buffer[txp::DEFAULT_LINE_LENGTH] = {'\0'};
#endif



//*****************************************************************************
//  Attribute constructors
//*****************************************************************************
txp::Attribute::Attribute() :
    attrType(txp::ATTRIBUTE_TYPE_INVALID),
    attrName(txp::ATTRIBUTE_NAME_INVALID) {
    attrFlag.flags = 0;
    init();
};


txp::Attribute::Attribute(const txp::AttributeType pType, const txp::AttributeName pName) :
    attrType(pType),
    attrName(pName) {
    attrFlag.flags = 0;
    init();

#ifdef TXP_DEVELOPMENT
    char l_Type[64] = {'\0'};
    attrTypeToChar(pType, l_Type, sizeof(l_Type));
    char l_Name[64] = {'\0'};
    attrNameToChar(pName, l_Name, sizeof(l_Name));
    snprintf(d_log_buffer, sizeof(d_log_buffer), "Attribute::Attribute(pType=%s, pName=%s)=%p", l_Type, l_Name, this);
    d_log.write(d_log_buffer, txp::Log::DEBUG, txp::Log::C_DTOR_LOGLEVEL_2);
#endif
};


txp::Attribute::Attribute(HeapBuffer* pBuffer, const size_t pOffset) :
    attrType(pBuffer->getAttrType(pOffset)),
    attrName(pBuffer->getAttrName(pOffset)) {
    attrFlag.flags = 0;
    init();

#ifdef TXP_DEVELOPMENT
    snprintf(d_log_buffer, sizeof(d_log_buffer), "Attribute::Attribute(pBuffer=%p, pOffset=%zu)=%p", pBuffer, pOffset, this);
    d_log.write(d_log_buffer, txp::Log::DEBUG, txp::Log::C_DTOR_LOGLEVEL_2);
#endif
};


txp::Attribute::Attribute(const txp::AttributeType pType, HeapBuffer* pBuffer, const size_t pOffset) :
    attrType(pType),
    attrName(pBuffer->getAttrName(pOffset)) {
    attrFlag.flags = 0;
    init();

#ifdef TXP_DEVELOPMENT
    char l_Type[64] = {'\0'};
    attrTypeToChar(pType, l_Type, sizeof(l_Type));
    snprintf(d_log_buffer, sizeof(d_log_buffer), "Attribute::Attribute(pType=%s, pBuffer=%p, pOffset=%zu)=%p", l_Type, pBuffer, pOffset, this);
    d_log.write(d_log_buffer, txp::Log::DEBUG, txp::Log::C_DTOR_LOGLEVEL_2);
#endif
};


txp::Attr_char::Attr_char(const txp::AttributeName pName, const char pData) :
    Attribute(txp::CHAR, pName), data(pData) {

#ifdef TXP_DEVELOPMENT
    char l_Name[64] = {'\0'};
    attrNameToChar(pName, l_Name, sizeof(l_Name));
    snprintf(d_log_buffer, sizeof(d_log_buffer), "Attr_char::Attr_char(pName=%s, pData=%c)=%p", l_Name, pData, this);
    d_log.write(d_log_buffer, txp::Log::DEBUG, txp::Log::C_DTOR_LOGLEVEL_2);
#endif
};


txp::Attr_uint8::Attr_uint8(const txp::AttributeName pName, const uint8_t pData) :
    Attribute(txp::UINT8, pName), data(pData) {

#ifdef TXP_DEVELOPMENT
    char l_Name[64] = {'\0'};
    attrNameToChar(pName, l_Name, sizeof(l_Name));
    snprintf(d_log_buffer, sizeof(d_log_buffer), "Attr_uint8::Attr_uint8(pName=%s, pData=%hhu)=%p", l_Name, pData, this);
    d_log.write(d_log_buffer, txp::Log::DEBUG, txp::Log::C_DTOR_LOGLEVEL_2);
#endif
};


txp::Attr_uint16::Attr_uint16(const txp::AttributeName pName, const uint16_t pData) :
    Attribute(txp::UINT16, pName), data(pData) {

#ifdef TXP_DEVELOPMENT
    char l_Name[64] = {'\0'};
    attrNameToChar(pName, l_Name, sizeof(l_Name));
    snprintf(d_log_buffer, sizeof(d_log_buffer), "Attr_uint16::Attr_uint16(pName=%s, pData=%hu)=%p", l_Name, pData, this);
    d_log.write(d_log_buffer, txp::Log::DEBUG, txp::Log::C_DTOR_LOGLEVEL_2);
#endif
};


txp::Attr_uint32::Attr_uint32(const txp::AttributeName pName, const uint32_t pData) :
    Attribute(txp::UINT32, pName), data(pData) {

#ifdef TXP_DEVELOPMENT
    char l_Name[64] = {'\0'};
    attrNameToChar(pName, l_Name, sizeof(l_Name));
    snprintf(d_log_buffer, sizeof(d_log_buffer), "Attr_uint32::Attr_uint32(pName=%s, pData=%u)=%p", l_Name, pData, this);
    d_log.write(d_log_buffer, txp::Log::DEBUG, txp::Log::C_DTOR_LOGLEVEL_2);
#endif
};


txp::Attr_uint64::Attr_uint64(const txp::AttributeName pName, const uint64_t pData) :
    Attribute(txp::UINT64, pName), data(pData) {

#ifdef TXP_DEVELOPMENT
    char l_Name[64] = {'\0'};
    attrNameToChar(pName, l_Name, sizeof(l_Name));
    snprintf(d_log_buffer, sizeof(d_log_buffer), "Attr_uint64::Attr_uint64(pName=%s, pData=%lu)=%p", l_Name, pData, this);
    d_log.write(d_log_buffer, txp::Log::DEBUG, txp::Log::C_DTOR_LOGLEVEL_2);
#endif
};


txp::Attr_int8::Attr_int8(const txp::AttributeName pName, const int8_t pData) :
    Attribute(txp::INT8, pName), data(pData) {

#ifdef TXP_DEVELOPMENT
    char l_Name[64] = {'\0'};
    attrNameToChar(pName, l_Name, sizeof(l_Name));
    snprintf(d_log_buffer, sizeof(d_log_buffer), "Attr_int8::Attr_int8(pName=%s, pData=%hhd)=%p", l_Name, pData, this);
    d_log.write(d_log_buffer, txp::Log::DEBUG, txp::Log::C_DTOR_LOGLEVEL_2);
#endif
};


txp::Attr_int16::Attr_int16(const txp::AttributeName pName, const int16_t pData) :
    Attribute(txp::INT16, pName), data(pData) {

#ifdef TXP_DEVELOPMENT
    char l_Name[64] = {'\0'};
    attrNameToChar(pName, l_Name, sizeof(l_Name));
    snprintf(d_log_buffer, sizeof(d_log_buffer), "Attr_int16::Attr_int16(pName=%s, pData=%hd)=%p", l_Name, pData, this);
    d_log.write(d_log_buffer, txp::Log::DEBUG, txp::Log::C_DTOR_LOGLEVEL_2);
#endif
};


txp::Attr_int32::Attr_int32(const txp::AttributeName pName, const int32_t pData) :
    Attribute(txp::INT32, pName), data(pData) {

#ifdef TXP_DEVELOPMENT
    char l_Name[64] = {'\0'};
    attrNameToChar(pName, l_Name, sizeof(l_Name));
    snprintf(d_log_buffer, sizeof(d_log_buffer), "Attr_int32::Attr_int32(pName=%s, pData=%d)=%p", l_Name, pData, this);
    d_log.write(d_log_buffer, txp::Log::DEBUG, txp::Log::C_DTOR_LOGLEVEL_2);
#endif
};


txp::Attr_int64::Attr_int64(const txp::AttributeName pName, const int64_t pData) :
    Attribute(txp::INT64, pName), data(pData) {

#ifdef TXP_DEVELOPMENT
    char l_Name[64] = {'\0'};
    attrNameToChar(pName, l_Name, sizeof(l_Name));
    snprintf(d_log_buffer, sizeof(d_log_buffer), "Attr_int32::Attr_int32(pName=%s, pData=%ld)=%p", l_Name, pData, this);
    d_log.write(d_log_buffer, txp::Log::DEBUG, txp::Log::C_DTOR_LOGLEVEL_2);
#endif
};


//  NOTE:  Attr_char_array cannot be constructed.
//         Instead, only AttrPtr_char_array can be constructed.
//txp::Attr_char_array::Attr_char_array(const txp::AttributeName pName, const char* pData, const size_t pLength) :
//  Attribute(txp::CHAR_ARRAY, pName), length(pLength) {
//  data = const_cast<char*>(pData);
//
//#ifdef TXP_DEVELOPMENT
//  char l_Name[64] = {'\0'};
//  attrNameToChar(pName, l_Name, sizeof(l_Name));
//  snprintf(d_log_buffer, sizeof(d_log_buffer), "Attr_char_array::Attr_char_array(pName=%s, pData=%p, pLength=%zu)=%p", l_Name, pData, pLength, this);
//  d_log.write(d_log_buffer, txp::Log::DEBUG, txp::Log::C_DTOR_LOGLEVEL_2);
//#endif
//};


//  NOTE:  Attr_array_of_char_arrays cannot be constructed.
//         Instead, only AttrPtr_array_of_char_arrays can be constructed.
//txp::Attr_array_of_char_arrays::Attr_array_of_char_arrays(const txp::AttributeName pName, const CharArray* pData) :
//  Attribute(txp::ARRAY_OF_CHAR_ARRAYS, pName), number_of_elements(0), length_of_data(0), length_of_values(0) {
//  attrFlag2.flags2 = 0;
//  data = const_cast<CharArray*>(pData);
//  number_of_elements = data->size();
//  for (CharArrayIter it = data->begin(); it != data->end(); ++it) {
//      NOT_IMPLEMENTED  @@DLH
//   }
//
//#ifdef TXP_DEVELOPMENT
//  char l_Name[64] = {'\0'};
//  attrNameToChar(pName, l_Name, sizeof(l_Name));
//  snprintf(d_log_buffer, sizeof(d_log_buffer), "Attr_array_of_char_arrays::Attr_array_of_char_arrays(pName=%s, pData=%p)=%p", l_Name, pData, this);
//  d_log.write(d_log_buffer, txp::Log::DEBUG, txp::Log::C_DTOR_LOGLEVEL_2);
//#endif
//};

// NOTE:  Implementation of AttrPtr_ objects from a heap buffer prevent us from storing a
//        pointer to the data as a const.  Thus, the signature provided is const *pData
//        for simplicity to our clients, but we const_cast the const away in the constructor.
//        In reality, only in the static methods where data is being assigned from a
//        heap buffer location is the value addressed by *pData actually 'altered' by the
//        transport facility.  @@DLH

txp::AttrPtr_char::AttrPtr_char(const txp::AttributeName pName, const char* pData) :
    Attribute(txp::PTR_CHAR, pName) {
    data = const_cast<char*>(pData);

#ifdef TXP_DEVELOPMENT
    char l_Name[64] = {'\0'};
    attrNameToChar(pName, l_Name, sizeof(l_Name));
    snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_char::AttrPtr_char(pName=%s, pData=%p)=%p, *pData=%c", l_Name, pData, this, *pData);
    d_log.write(d_log_buffer, txp::Log::DEBUG, txp::Log::C_DTOR_LOGLEVEL_2);
#endif
};

txp::AttrPtr_uint8::AttrPtr_uint8(const txp::AttributeName pName, const uint8_t* pData) :
    Attribute(txp::PTR_UINT8, pName) {
    data = const_cast<uint8_t*>(pData);

#ifdef TXP_DEVELOPMENT
    char l_Name[64] = {'\0'};
    attrNameToChar(pName, l_Name, sizeof(l_Name));
    snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_uint8::AttrPtr_uint8(pName=%s, pData=%p)=%p, *pData=%hhu", l_Name, pData, this, *pData);
    d_log.write(d_log_buffer, txp::Log::DEBUG, txp::Log::C_DTOR_LOGLEVEL_2);
#endif
};


txp::AttrPtr_uint16::AttrPtr_uint16(const txp::AttributeName pName, const uint16_t* pData) :
    Attribute(txp::PTR_UINT16, pName) {
    data = const_cast<uint16_t*>(pData);

#ifdef TXP_DEVELOPMENT
    char l_Name[64] = {'\0'};
    attrNameToChar(pName, l_Name, sizeof(l_Name));
    snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_uint16::AttrPtr_uint16(pName=%s, pData=%p)=%p, *pData=%hu", l_Name, pData, this, *pData);
    d_log.write(d_log_buffer, txp::Log::DEBUG, txp::Log::C_DTOR_LOGLEVEL_2);
#endif
};


txp::AttrPtr_uint32::AttrPtr_uint32(const txp::AttributeName pName, const uint32_t* pData) :
    Attribute(txp::PTR_UINT32, pName) {
    data = const_cast<uint32_t*>(pData);

#ifdef TXP_DEVELOPMENT
    char l_Name[64] = {'\0'};
    attrNameToChar(pName, l_Name, sizeof(l_Name));
    snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_uint32::AttrPtr_uint32(pName=%s, pData=%p)=%p, *pData=%u", l_Name, pData, this, *pData);
    d_log.write(d_log_buffer, txp::Log::DEBUG, txp::Log::C_DTOR_LOGLEVEL_2);
#endif
};


txp::AttrPtr_uint64::AttrPtr_uint64(const txp::AttributeName pName, const uint64_t* pData) :
    Attribute(txp::PTR_UINT64, pName) {
    data = const_cast<uint64_t*>(pData);

#ifdef TXP_DEVELOPMENT
    char l_Name[64] = {'\0'};
    attrNameToChar(pName, l_Name, sizeof(l_Name));
    snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_uint64::AttrPtr_uint64(pName=%s, pData=%p)=%p, *pData=%lu", l_Name, pData, this, *pData);
    d_log.write(d_log_buffer, txp::Log::DEBUG, txp::Log::C_DTOR_LOGLEVEL_2);
#endif
};


txp::AttrPtr_int8::AttrPtr_int8(const txp::AttributeName pName, const int8_t* pData) :
    Attribute(txp::PTR_INT8, pName) {
    data = const_cast<int8_t*>(pData);

#ifdef TXP_DEVELOPMENT
    char l_Name[64] = {'\0'};
    attrNameToChar(pName, l_Name, sizeof(l_Name));
    snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_int8::AttrPtr_int8(pName=%s, pData=%p)=%p, *pData=%hhd", l_Name, pData, this, *pData);
    d_log.write(d_log_buffer, txp::Log::DEBUG, txp::Log::C_DTOR_LOGLEVEL_2);
#endif
};


txp::AttrPtr_int16::AttrPtr_int16(const txp::AttributeName pName, const int16_t* pData) :
    Attribute(txp::PTR_INT16, pName) {
    data = const_cast<int16_t*>(pData);

#ifdef TXP_DEVELOPMENT
    char l_Name[64] = {'\0'};
    attrNameToChar(pName, l_Name, sizeof(l_Name));
    snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_int16::AttrPtr_int16(pName=%s, pData=%p)=%p, *pData=%hd", l_Name, pData, this, *pData);
    d_log.write(d_log_buffer, txp::Log::DEBUG, txp::Log::C_DTOR_LOGLEVEL_2);
#endif
};


txp::AttrPtr_int32::AttrPtr_int32(const txp::AttributeName pName, const int32_t* pData) :
    Attribute(txp::PTR_INT32, pName) {
    data = const_cast<int32_t*>(pData);

#ifdef TXP_DEVELOPMENT
    char l_Name[64] = {'\0'};
    attrNameToChar(pName, l_Name, sizeof(l_Name));
    snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_int32::AttrPtr_int32(pName=%s, pData=%p)=%p, *pData=%d", l_Name, pData, this, *pData);
    d_log.write(d_log_buffer, txp::Log::DEBUG, txp::Log::C_DTOR_LOGLEVEL_2);
#endif
};


txp::AttrPtr_int64::AttrPtr_int64(const txp::AttributeName pName, const int64_t* pData) :
    Attribute(txp::PTR_INT64, pName) {
    data = const_cast<int64_t*>(pData);

#ifdef TXP_DEVELOPMENT
    char l_Name[64] = {'\0'};
    attrNameToChar(pName, l_Name, sizeof(l_Name));
    snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_int64::AttrPtr_int64(pName=%s, pData=%p)=%p, *pData=%ld", l_Name, pData, this, *pData);
    d_log.write(d_log_buffer, txp::Log::DEBUG, txp::Log::C_DTOR_LOGLEVEL_2);
#endif
};


txp::AttrPtr_char_array::AttrPtr_char_array(const txp::AttributeName pName, const char* pData, const size_t pLength) :
    Attribute(txp::PTR_CHAR_ARRAY, pName), length(pLength) {
    data = const_cast<char*>(pData);

#ifdef TXP_DEVELOPMENT
    char l_Name[64] = {'\0'};
    attrNameToChar(pName, l_Name, sizeof(l_Name));
    snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_char_array::AttrPtr_char_array(pName=%s, pData=%p, pLength=%zu)=%p", l_Name, pData, pLength, this);
    d_log.write(d_log_buffer, txp::Log::DEBUG, txp::Log::C_DTOR_LOGLEVEL_2);
#endif
};


txp::AttrPtr_array_of_char_arrays::AttrPtr_array_of_char_arrays(txp::AttributeName pName, const CharArray* pData) :
    Attribute(txp::PTR_ARRAY_OF_CHAR_ARRAYS, pName), number_of_elements(0), length_of_data(0), length_of_values(0) {
    attrFlag2.flags2 = 0;
    data = new CharArray();
    setDataAllocatedFlag(1);
    number_of_elements = pData->size();
    data->reserve(number_of_elements);
    length_of_data = (size_t)(number_of_elements * (sizeof(TXP_ARRAY_OF_CHAR_ARRAYS_ELEMENT_SIZE_TYPE) + sizeof(TXP_ARRAY_OF_CHAR_ARRAYS_ELEMENT_DATA_TYPE)));
    for (CharArrayIter it = (const_cast<CharArray*>(pData))->begin(); it != (const_cast<CharArray*>(pData))->end(); ++it) {
    	length_of_values += (sizeof(it->first) + it->first);
        data->push_back(std::make_pair(it->first, it->second));
    }

#ifdef TXP_DEVELOPMENT
    char l_Name[64] = {'\0'};
    attrNameToChar(pName, l_Name, sizeof(l_Name));
    snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_array_of_char_arrays::AttrPtr_array_of_char_arrays(pName=%s, pData=%p)=%p", l_Name, pData, this);
    d_log.write(d_log_buffer, txp::Log::DEBUG, txp::Log::C_DTOR_LOGLEVEL_2);
#endif
};


// Copy constructors...


txp::Attribute::Attribute(const Attribute &pAttribute) :
	attrType(pAttribute.attrType),
	attrName(pAttribute.attrName) {
	attrFlag.flags = 0;
#ifdef TXP_DEVELOPMENT
    snprintf(d_log_buffer, sizeof(d_log_buffer), "Attribute::Attribute(const Attribute &pAttribute=%p), TypeId=%d, NameId=%d", &pAttribute, pAttribute.attrType, pAttribute.attrName);
    d_log.write(d_log_buffer, txp::Log::DEBUG, txp::Log::C_DTOR_LOGLEVEL_2);
#endif
}


txp::Attr_char::Attr_char(const Attr_char &pAttribute) :
	Attribute::Attribute(pAttribute),
	data(pAttribute.data) {}


txp::Attr_uint8::Attr_uint8(const Attr_uint8 &pAttribute) :
	Attribute::Attribute(pAttribute),
	data(pAttribute.data) {}


txp::Attr_uint16::Attr_uint16(const Attr_uint16 &pAttribute) :
	Attribute::Attribute(pAttribute),
	data(pAttribute.data) {}


txp::Attr_uint32::Attr_uint32(const Attr_uint32 &pAttribute) :
	Attribute::Attribute(pAttribute),
	data(pAttribute.data) {}


txp::Attr_uint64::Attr_uint64(const Attr_uint64 &pAttribute) :
	Attribute::Attribute(pAttribute),
	data(pAttribute.data) {}


txp::Attr_int8::Attr_int8(const Attr_int8 &pAttribute) :
	Attribute::Attribute(pAttribute),
	data(pAttribute.data) {}


txp::Attr_int16::Attr_int16(const Attr_int16 &pAttribute) :
	Attribute::Attribute(pAttribute),
	data(pAttribute.data) {}


txp::Attr_int32::Attr_int32(const Attr_int32 &pAttribute) :
	Attribute::Attribute(pAttribute),
	data(pAttribute.data) {}


txp::Attr_int64::Attr_int64(const Attr_int64 &pAttribute) :
	Attribute::Attribute(pAttribute),
	data(pAttribute.data) {}


txp::AttrPtr_char::AttrPtr_char(const AttrPtr_char &pAttribute) :
	Attribute::Attribute(pAttribute),
	data(pAttribute.data) {}


txp::AttrPtr_uint8::AttrPtr_uint8(const AttrPtr_uint8 &pAttribute) :
	Attribute::Attribute(pAttribute),
	data(pAttribute.data) {}


txp::AttrPtr_uint16::AttrPtr_uint16(const AttrPtr_uint16 &pAttribute) :
	Attribute::Attribute(pAttribute),
	data(pAttribute.data) {}


txp::AttrPtr_uint32::AttrPtr_uint32(const AttrPtr_uint32 &pAttribute) :
	Attribute::Attribute(pAttribute),
	data(pAttribute.data) {}


txp::AttrPtr_uint64::AttrPtr_uint64(const AttrPtr_uint64 &pAttribute) :
	Attribute::Attribute(pAttribute),
	data(pAttribute.data) {}


txp::AttrPtr_int8::AttrPtr_int8(const AttrPtr_int8 &pAttribute) :
	Attribute::Attribute(pAttribute),
	data(pAttribute.data) {}


txp::AttrPtr_int16::AttrPtr_int16(const AttrPtr_int16 &pAttribute) :
	Attribute::Attribute(pAttribute),
	data(pAttribute.data) {}


txp::AttrPtr_int32::AttrPtr_int32(const AttrPtr_int32 &pAttribute) :
	Attribute::Attribute(pAttribute),
	data(pAttribute.data) {}


txp::AttrPtr_int64::AttrPtr_int64(const AttrPtr_int64 &pAttribute) :
	Attribute::Attribute(pAttribute),
	data(pAttribute.data) {}


txp::AttrPtr_char_array::AttrPtr_char_array(const AttrPtr_char_array &pAttribute) :
	Attribute::Attribute(pAttribute),
	length(pAttribute.length),
	data(pAttribute.data) {}


txp::AttrPtr_array_of_char_arrays::AttrPtr_array_of_char_arrays(const AttrPtr_array_of_char_arrays &pAttribute) :
	Attribute::Attribute(pAttribute),
	number_of_elements(pAttribute.number_of_elements),
	length_of_data(pAttribute.length_of_data),
	length_of_values(pAttribute.length_of_values) {
	attrFlag2.flags2 = 0;
	data = new CharArray;
    data->reserve(txp::DEFAULT_NUMBER_OF_CHAR_ARRAYS);
    setDataAllocatedFlag(1);
    for (CharArrayIter it = pAttribute.data->begin(); it != pAttribute.data->end(); ++it) {
        data->push_back(std::make_pair(it->first, it->second));
    }
}


// Constructed from a heap buffer (deserialize)...
txp::Attr_char::Attr_char(HeapBuffer* pBuffer, const size_t pOffset, const txp::DeserializeOption &pOption, const txp::DataLocation &pDataLocation) :
    Attribute(txp::CHAR, pBuffer, pOffset) {
    data = '\0';
    switch(pOption) {
        case txp::DO_NOT_COPY_DATA:
            //  CANNOT OBEY DO_NOT_COPY_DATA - COPY DATA TO OBJECT
        case txp::COPY_DATA_TO_HEAP:
            //  CANNOT OBEY COPY_DATA_TO_HEAP - COPY DATA TO OBJECT
        case txp::COPY_DATA_TO_OBJECT:
        case txp::COPY_DATA_TO_OBJECT_BUT_DO_NOT_COPY_ARRAYS:
            switch(pDataLocation) {
                case txp::BUFFER_HAS_DATA:
                    data = pBuffer->getAttr_char(pOffset);
                    break;
                case txp::BUFFER_HAS_PTR_TO_DATA:
                    {
                        char* l_Data = pBuffer->getAttrPtr_char(pOffset);
                        data = *l_Data;
                    }
                    break;
                default:
                    // Should never get here...
#ifdef TXP_DEVELOPMENT
                    snprintf(d_log_buffer, sizeof(d_log_buffer), "Attr_char::Attr_char(): pDataLocation=%d", pDataLocation);
                    d_log.write(d_log_buffer, txp::Log::ERROR);
#endif
                    break;
            }

            break;

        default:
            // Should never get here...
#ifdef TXP_DEVELOPMENT
            snprintf(d_log_buffer, sizeof(d_log_buffer), "Attr_char::Attr_char(): pOption=%d", pOption);
            d_log.write(d_log_buffer, txp::Log::ERROR);
#endif
            break;
    }

#ifdef TXP_DEVELOPMENT
    snprintf(d_log_buffer, sizeof(d_log_buffer), "Attr_char::Attr_char(pBuffer=%p, pOffset=%zu, pOption=%d, pDataLocation=%d)=%p", pBuffer, pOffset, pOption, pDataLocation, this);
    d_log.write(d_log_buffer, txp::Log::DEBUG, txp::Log::C_DTOR_LOGLEVEL_2);
#endif
};


txp::Attr_uint8::Attr_uint8(HeapBuffer* pBuffer, const size_t pOffset, const txp::DeserializeOption &pOption, const txp::DataLocation &pDataLocation) :
    Attribute(txp::UINT8, pBuffer, pOffset) {
    data = 0;
    switch(pOption) {
        case txp::DO_NOT_COPY_DATA:
            //  CANNOT OBEY DO_NOT_COPY_DATA - COPY DATA TO OBJECT
        case txp::COPY_DATA_TO_HEAP:
            //  CANNOT OBEY COPY_DATA_TO_HEAP - COPY DATA TO OBJECT
        case txp::COPY_DATA_TO_OBJECT:
        case txp::COPY_DATA_TO_OBJECT_BUT_DO_NOT_COPY_ARRAYS:
            switch(pDataLocation) {
                case txp::BUFFER_HAS_DATA:
                    data = pBuffer->getAttr_uint8(pOffset);
                    break;
                case txp::BUFFER_HAS_PTR_TO_DATA:
                    {
                        uint8_t* l_Data = pBuffer->getAttrPtr_uint8(pOffset);
                        data = *l_Data;
                    }
                    break;
                default:
                    // Should never get here...
#ifdef TXP_DEVELOPMENT
                    snprintf(d_log_buffer, sizeof(d_log_buffer), "Attr_uint8::Attr_uint8(): pDataLocation=%d", pDataLocation);
                    d_log.write(d_log_buffer, txp::Log::ERROR);
#endif
                    break;
            }

            break;

        default:
            // Should never get here...
#ifdef TXP_DEVELOPMENT
            snprintf(d_log_buffer, sizeof(d_log_buffer), "Attr_uint8::Attr_uint8(): pOption=%d", pOption);
            d_log.write(d_log_buffer, txp::Log::ERROR);
#endif
            break;
    }

#ifdef TXP_DEVELOPMENT
    snprintf(d_log_buffer, sizeof(d_log_buffer), "Attr_uint8::Attr_uint8(pBuffer=%p, pOffset=%zu, pOption=%d, pDataLocation=%d)=%p", pBuffer, pOffset, pOption, pDataLocation, this);
    d_log.write(d_log_buffer, txp::Log::DEBUG, txp::Log::C_DTOR_LOGLEVEL_2);
#endif
};


txp::Attr_uint16::Attr_uint16(HeapBuffer* pBuffer, const size_t pOffset, const txp::DeserializeOption &pOption, const txp::DataLocation &pDataLocation) :
    Attribute(txp::UINT16, pBuffer, pOffset) {
    data = 0;
    switch(pOption) {
        case txp::DO_NOT_COPY_DATA:
            //  CANNOT OBEY DO_NOT_COPY_DATA - COPY DATA TO OBJECT
        case txp::COPY_DATA_TO_HEAP:
            //  CANNOT OBEY COPY_DATA_TO_HEAP - COPY DATA TO OBJECT
        case txp::COPY_DATA_TO_OBJECT:
        case txp::COPY_DATA_TO_OBJECT_BUT_DO_NOT_COPY_ARRAYS:
            switch(pDataLocation) {
                case txp::BUFFER_HAS_DATA:
                    data = pBuffer->getAttr_uint16(pOffset);
                    break;
                case txp::BUFFER_HAS_PTR_TO_DATA:
                    {
                        uint16_t* l_Data = pBuffer->getAttrPtr_uint16(pOffset);
                        data = *l_Data;
                    }
                    break;
                default:
                    // Should never get here...
#ifdef TXP_DEVELOPMENT
                    snprintf(d_log_buffer, sizeof(d_log_buffer), "Attr_uint1::Attr_uint16(): pDataLocation=%d", pDataLocation);
                    d_log.write(d_log_buffer, txp::Log::ERROR);
#endif
                    break;
            }

            break;

        default:
            // Should never get here...
#ifdef TXP_DEVELOPMENT
            snprintf(d_log_buffer, sizeof(d_log_buffer), "Attr_uint16::Attr_uint16(): pOption=%d", pOption);
            d_log.write(d_log_buffer, txp::Log::ERROR);
#endif
            break;
    }

#ifdef TXP_DEVELOPMENT
    snprintf(d_log_buffer, sizeof(d_log_buffer), "Attr_uint16::Attr_uint16(pBuffer=%p, pOffset=%zu, pOption=%d, pDataLocation=%d)=%p", pBuffer, pOffset, pOption, pDataLocation, this);
    d_log.write(d_log_buffer, txp::Log::DEBUG, txp::Log::C_DTOR_LOGLEVEL_2);
#endif
};


txp::Attr_uint32::Attr_uint32(HeapBuffer* pBuffer, const size_t pOffset, const txp::DeserializeOption &pOption, const txp::DataLocation &pDataLocation) :
    Attribute(txp::UINT32, pBuffer, pOffset) {
    data = 0;
    switch(pOption) {
        case txp::DO_NOT_COPY_DATA:
            //  CANNOT OBEY DO_NOT_COPY_DATA - COPY DATA TO OBJECT
        case txp::COPY_DATA_TO_HEAP:
            //  CANNOT OBEY COPY_DATA_TO_HEAP - COPY DATA TO OBJECT
        case txp::COPY_DATA_TO_OBJECT:
        case txp::COPY_DATA_TO_OBJECT_BUT_DO_NOT_COPY_ARRAYS:
            switch(pDataLocation) {
                case txp::BUFFER_HAS_DATA:
                    data = pBuffer->getAttr_uint32(pOffset);
                    break;
                case txp::BUFFER_HAS_PTR_TO_DATA:
                    {
                        uint32_t* l_Data = pBuffer->getAttrPtr_uint32(pOffset);
                        data = *l_Data;
                    }
                    break;
                default:
                    // Should never get here...
#ifdef TXP_DEVELOPMENT
                    snprintf(d_log_buffer, sizeof(d_log_buffer), "Attr_uint32::Attr_uint32(): pDataLocation=%d", pDataLocation);
                    d_log.write(d_log_buffer, txp::Log::ERROR);
#endif
                    break;
            }

            break;

        default:
            // Should never get here...
#ifdef TXP_DEVELOPMENT
            snprintf(d_log_buffer, sizeof(d_log_buffer), "Attr_uint32::Attr_uint32(): pOption=%d", pOption);
            d_log.write(d_log_buffer, txp::Log::ERROR);
#endif
            break;
    }

#ifdef TXP_DEVELOPMENT
    snprintf(d_log_buffer, sizeof(d_log_buffer), "Attr_uint32::Attr_uint32(pBuffer=%p, pOffset=%zu, pOption=%d, pDataLocation=%d)=%p", pBuffer, pOffset, pOption, pDataLocation, this);
    d_log.write(d_log_buffer, txp::Log::DEBUG, txp::Log::C_DTOR_LOGLEVEL_2);
#endif
};


txp::Attr_uint64::Attr_uint64(HeapBuffer* pBuffer, const size_t pOffset, const txp::DeserializeOption &pOption, const txp::DataLocation &pDataLocation) :
    Attribute(txp::UINT64, pBuffer, pOffset) {
    data = 0;
    switch(pOption) {
        case txp::DO_NOT_COPY_DATA:
            //  CANNOT OBEY DO_NOT_COPY_DATA - COPY DATA TO OBJECT
        case txp::COPY_DATA_TO_HEAP:
            //  CANNOT OBEY COPY_DATA_TO_HEAP - COPY DATA TO OBJECT
        case txp::COPY_DATA_TO_OBJECT:
        case txp::COPY_DATA_TO_OBJECT_BUT_DO_NOT_COPY_ARRAYS:
            switch(pDataLocation) {
                case txp::BUFFER_HAS_DATA:
                    data = pBuffer->getAttr_uint64(pOffset);
                    break;
                case txp::BUFFER_HAS_PTR_TO_DATA:
                    {
                        uint64_t* l_Data = pBuffer->getAttrPtr_uint64(pOffset);
                        data = *l_Data;
                    }
                    break;
                default:
                    // Should never get here...
#ifdef TXP_DEVELOPMENT
                    snprintf(d_log_buffer, sizeof(d_log_buffer), "Attr_uint64::Attr_uint64(): pDataLocation=%d", pDataLocation);
                    d_log.write(d_log_buffer, txp::Log::ERROR);
#endif
                    break;
            }

            break;

        default:
            // Should never get here...
#ifdef TXP_DEVELOPMENT
            snprintf(d_log_buffer, sizeof(d_log_buffer), "Attr_uint64::Attr_uint64(): pOption=%d", pOption);
            d_log.write(d_log_buffer, txp::Log::ERROR);
#endif
            break;
    }

#ifdef TXP_DEVELOPMENT
    snprintf(d_log_buffer, sizeof(d_log_buffer), "Attr_uint64::Attr_uint64(pBuffer=%p, pOffset=%zu, pOption=%d, pDataLocation=%d)=%p", pBuffer, pOffset, pOption, pDataLocation, this);
    d_log.write(d_log_buffer, txp::Log::DEBUG, txp::Log::C_DTOR_LOGLEVEL_2);
#endif
};


txp::Attr_int8::Attr_int8(HeapBuffer* pBuffer, const size_t pOffset, const txp::DeserializeOption &pOption, const txp::DataLocation &pDataLocation) :
    Attribute(txp::INT8, pBuffer, pOffset) {
    data = 0;
    switch(pOption) {
        case txp::DO_NOT_COPY_DATA:
            //  CANNOT OBEY DO_NOT_COPY_DATA - COPY DATA TO OBJECT
        case txp::COPY_DATA_TO_HEAP:
            //  CANNOT OBEY COPY_DATA_TO_HEAP - COPY DATA TO OBJECT
        case txp::COPY_DATA_TO_OBJECT:
        case txp::COPY_DATA_TO_OBJECT_BUT_DO_NOT_COPY_ARRAYS:
            switch(pDataLocation) {
                case txp::BUFFER_HAS_DATA:
                    data = pBuffer->getAttr_int8(pOffset);
                    break;
                case txp::BUFFER_HAS_PTR_TO_DATA:
                    {
                        int8_t* l_Data = pBuffer->getAttrPtr_int8(pOffset);
                        data = *l_Data;
                    }
                    break;
                default:
                    // Should never get here...
#ifdef TXP_DEVELOPMENT
                    snprintf(d_log_buffer, sizeof(d_log_buffer), "Attr_int8::Attr_int8(): pDataLocation=%d", pDataLocation);
                    d_log.write(d_log_buffer, txp::Log::ERROR);
#endif
                    break;
            }

            break;

        default:
            // Should never get here...
#ifdef TXP_DEVELOPMENT
            snprintf(d_log_buffer, sizeof(d_log_buffer), "Attr_int8::Attr_int8(): pOption=%d", pOption);
            d_log.write(d_log_buffer, txp::Log::ERROR);
#endif
            break;
    }

#ifdef TXP_DEVELOPMENT
    snprintf(d_log_buffer, sizeof(d_log_buffer), "Attr_int8::Attr_int8(pBuffer=%p, pOffset=%zu, pOption=%d, pDataLocation=%d)=%p", pBuffer, pOffset, pOption, pDataLocation, this);
    d_log.write(d_log_buffer, txp::Log::DEBUG, txp::Log::C_DTOR_LOGLEVEL_2);
#endif
};


txp::Attr_int16::Attr_int16(HeapBuffer* pBuffer, const size_t pOffset, const txp::DeserializeOption &pOption, const txp::DataLocation &pDataLocation) :
    Attribute(txp::INT16, pBuffer, pOffset) {
    data = 0;
    switch(pOption) {
        case txp::DO_NOT_COPY_DATA:
            //  CANNOT OBEY DO_NOT_COPY_DATA - COPY DATA TO OBJECT
        case txp::COPY_DATA_TO_HEAP:
            //  CANNOT OBEY COPY_DATA_TO_HEAP - COPY DATA TO OBJECT
        case txp::COPY_DATA_TO_OBJECT:
        case txp::COPY_DATA_TO_OBJECT_BUT_DO_NOT_COPY_ARRAYS:
            switch(pDataLocation) {
                case txp::BUFFER_HAS_DATA:
                    data = pBuffer->getAttr_int16(pOffset);
                    break;
                case txp::BUFFER_HAS_PTR_TO_DATA:
                    {
                        int16_t* l_Data = pBuffer->getAttrPtr_int16(pOffset);
                        data = *l_Data;
                    }
                    break;
                default:
                    // Should never get here...
#ifdef TXP_DEVELOPMENT
                    snprintf(d_log_buffer, sizeof(d_log_buffer), "Attr_int16::Attr_int16(): pDataLocation=%d", pDataLocation);
                    d_log.write(d_log_buffer, txp::Log::ERROR);
#endif
                    break;
            }

            break;

        default:
            // Should never get here...
#ifdef TXP_DEVELOPMENT
            snprintf(d_log_buffer, sizeof(d_log_buffer), "Attr_int16::Attr_int16(): pOption=%d", pOption);
            d_log.write(d_log_buffer, txp::Log::ERROR);
#endif
            break;
    }

#ifdef TXP_DEVELOPMENT
    snprintf(d_log_buffer, sizeof(d_log_buffer), "Attr_int16::Attr_int16(pBuffer=%p, pOffset=%zu, pOption=%d, pDataLocation=%d)=%p", pBuffer, pOffset, pOption, pDataLocation, this);
    d_log.write(d_log_buffer, txp::Log::DEBUG, txp::Log::C_DTOR_LOGLEVEL_2);
#endif
};


txp::Attr_int32::Attr_int32(HeapBuffer* pBuffer, const size_t pOffset, const txp::DeserializeOption &pOption, const txp::DataLocation &pDataLocation) :
    Attribute(txp::INT32, pBuffer, pOffset) {
    data = 0;
    switch(pOption) {
        case txp::DO_NOT_COPY_DATA:
            //  CANNOT OBEY DO_NOT_COPY_DATA - COPY DATA TO OBJECT
        case txp::COPY_DATA_TO_HEAP:
            //  CANNOT OBEY COPY_DATA_TO_HEAP - COPY DATA TO OBJECT
        case txp::COPY_DATA_TO_OBJECT:
        case txp::COPY_DATA_TO_OBJECT_BUT_DO_NOT_COPY_ARRAYS:
            switch(pDataLocation) {
                case txp::BUFFER_HAS_DATA:
                    data = pBuffer->getAttr_int32(pOffset);
                    break;
                case txp::BUFFER_HAS_PTR_TO_DATA:
                    {
                        int32_t* l_Data = pBuffer->getAttrPtr_int32(pOffset);
                        data = *l_Data;
                    }
                    break;
                default:
                    // Should never get here...
#ifdef TXP_DEVELOPMENT
                    snprintf(d_log_buffer, sizeof(d_log_buffer), "Attr_int32::Attr_int32(): pDataLocation=%d", pDataLocation);
                    d_log.write(d_log_buffer, txp::Log::ERROR);
#endif
                    break;
            }

            break;

        default:
            // Should never get here...
#ifdef TXP_DEVELOPMENT
            snprintf(d_log_buffer, sizeof(d_log_buffer), "Attr_int32::Attr_int32(): pOption=%d", pOption);
            d_log.write(d_log_buffer, txp::Log::ERROR);
#endif
            break;
    }

#ifdef TXP_DEVELOPMENT
    snprintf(d_log_buffer, sizeof(d_log_buffer), "Attr_int32::Attr_int32(pBuffer=%p, pOffset=%zu, pOption=%d, pDataLocation=%d)=%p", pBuffer, pOffset, pOption, pDataLocation, this);
    d_log.write(d_log_buffer, txp::Log::DEBUG, txp::Log::C_DTOR_LOGLEVEL_2);
#endif
};


txp::Attr_int64::Attr_int64(HeapBuffer* pBuffer, const size_t pOffset, const txp::DeserializeOption &pOption, const txp::DataLocation &pDataLocation) :
    Attribute(txp::INT64, pBuffer, pOffset) {
    data = 0;
    switch(pOption) {
        case txp::DO_NOT_COPY_DATA:
            //  CANNOT OBEY DO_NOT_COPY_DATA - COPY DATA TO OBJECT
        case txp::COPY_DATA_TO_HEAP:
            //  CANNOT OBEY COPY_DATA_TO_HEAP - COPY DATA TO OBJECT
        case txp::COPY_DATA_TO_OBJECT:
        case txp::COPY_DATA_TO_OBJECT_BUT_DO_NOT_COPY_ARRAYS:
            switch(pDataLocation) {
                case txp::BUFFER_HAS_DATA:
                    data = pBuffer->getAttr_int64(pOffset);
                    break;
                case txp::BUFFER_HAS_PTR_TO_DATA:
                    {
                        int64_t* l_Data = pBuffer->getAttrPtr_int64(pOffset);
                        data = *l_Data;
                    }
                    break;
                default:
                    // Should never get here...
#ifdef TXP_DEVELOPMENT
                    snprintf(d_log_buffer, sizeof(d_log_buffer), "Attr_int64::Attr_int64(): pDataLocation=%d", pDataLocation);
                    d_log.write(d_log_buffer, txp::Log::ERROR);
#endif
                    break;
            }

            break;

        default:
            // Should never get here...
#ifdef TXP_DEVELOPMENT
            snprintf(d_log_buffer, sizeof(d_log_buffer), "Attr_int64::Attr_int64(): pOption=%d", pOption);
            d_log.write(d_log_buffer, txp::Log::ERROR);
#endif
            break;
    }

#ifdef TXP_DEVELOPMENT
    snprintf(d_log_buffer, sizeof(d_log_buffer), "Attr_int64::Attr_int64(pBuffer=%p, pOffset=%zu, pOption=%d, pDataLocation=%d)=%p", pBuffer, pOffset, pOption, pDataLocation, this);
    d_log.write(d_log_buffer, txp::Log::DEBUG, txp::Log::C_DTOR_LOGLEVEL_2);
#endif
};


txp::AttrPtr_char::AttrPtr_char(HeapBuffer* pBuffer, const size_t pOffset, const txp::DeserializeOption &pOption, const txp::DataLocation &pDataLocation) :
    Attribute(txp::PTR_CHAR, pBuffer, pOffset) {
    data = NULL;
    switch(pOption) {
        case txp::DO_NOT_COPY_DATA:
            switch(pDataLocation) {
                case txp::BUFFER_HAS_DATA:
                    data = pBuffer->getAddrData_char(pOffset);
                    break;
                case txp::BUFFER_HAS_PTR_TO_DATA:
                    data = pBuffer->getAttrPtr_char(pOffset);
                    break;
                default:
                    // Should never get here...
#ifdef TXP_DEVELOPMENT
                    snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_char::AttrPtr_char(): pDataLocation=%d, pOption=DO_NOT_COPY_DATA", pDataLocation);
                    d_log.write(d_log_buffer, txp::Log::ERROR);
#endif
                    break;
            }

            break;

        case txp::COPY_DATA_TO_HEAP:
            data = new char;
            setDataAllocatedFlag(1);
            switch(pDataLocation) {
                case txp::BUFFER_HAS_DATA:
                    *data = pBuffer->getAttr_char(pOffset);
                    break;
                case txp::BUFFER_HAS_PTR_TO_DATA:
                    {
                        char* l_Data = pBuffer->getAttrPtr_char(pOffset);
                        *data = *l_Data;
                    }
                    break;
                default:
                    // Should never get here...
#ifdef TXP_DEVELOPMENT
                    snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_char::AttrPtr_char(): pDataLocation=%d, pOption=COPY_DATA_TO_HEAP", pDataLocation);
                    d_log.write(d_log_buffer, txp::Log::ERROR);
#endif
                    break;
            }

            break;

        case txp::COPY_DATA_TO_OBJECT:
            switch(pDataLocation) {
                case txp::BUFFER_HAS_DATA:
                    data = new char;
                    setDataAllocatedFlag(1);
                    *data = pBuffer->getAttr_char(pOffset);
                    break;
                case txp::BUFFER_HAS_PTR_TO_DATA:
                    data = pBuffer->getAttrPtr_char(pOffset);
                    break;
                default:
                    // Should never get here...
#ifdef TXP_DEVELOPMENT
                    snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_char::AttrPtr_char(): pDataLocation=%d, pOption=COPY_DATA_TO_OBJECT", pDataLocation);
                    d_log.write(d_log_buffer, txp::Log::ERROR);
#endif
                    break;
            }

            break;

        case txp::COPY_DATA_TO_OBJECT_BUT_DO_NOT_COPY_ARRAYS:
            switch(pDataLocation) {
                case txp::BUFFER_HAS_DATA:
                    data = new char;
                    setDataAllocatedFlag(1);
                    *data = pBuffer->getAttr_char(pOffset);
                    break;
                case txp::BUFFER_HAS_PTR_TO_DATA:
                    data = pBuffer->getAttrPtr_char(pOffset);
                    break;
                default:
                    // Should never get here...
#ifdef TXP_DEVELOPMENT
                    snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_char::AttrPtr_char(): pDataLocation=%d, pOption=COPY_DATA_TO_OBJECT_BUT_DO_NOT_COPY_ARRAYS", pDataLocation);
                    d_log.write(d_log_buffer, txp::Log::ERROR);
#endif
                    break;
            }

            break;

        default:
            // Should never get here...
#ifdef TXP_DEVELOPMENT
            snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_char::AttrPtr_char(): pOption=%d", pOption);
            d_log.write(d_log_buffer, txp::Log::ERROR);
#endif
            break;
    }

#ifdef TXP_DEVELOPMENT
    snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_char::AttrPtr_char(pBuffer=%p, pOffset=%zu, pOption=%d, pDataLocation=%d)=%p", pBuffer, pOffset, pOption, pDataLocation, this);
    d_log.write(d_log_buffer, txp::Log::DEBUG, txp::Log::C_DTOR_LOGLEVEL_2);
#endif
};


txp::AttrPtr_uint8::AttrPtr_uint8(HeapBuffer* pBuffer, const size_t pOffset, const txp::DeserializeOption &pOption, const txp::DataLocation &pDataLocation) :
    Attribute(txp::PTR_UINT8, pBuffer, pOffset) {
    data = NULL;
    switch(pOption) {
        case txp::DO_NOT_COPY_DATA:
            switch(pDataLocation) {
                case txp::BUFFER_HAS_DATA:
                    data = pBuffer->getAddrData_uint8(pOffset);
                    break;
                case txp::BUFFER_HAS_PTR_TO_DATA:
                    data = pBuffer->getAttrPtr_uint8(pOffset);
                    break;
                default:
                    // Should never get here...
#ifdef TXP_DEVELOPMENT
                    snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_uint8::AttrPtr_uint8(): pDataLocation=%d, pOption=DO_NOT_COPY_DATA", pDataLocation);
                    d_log.write(d_log_buffer, txp::Log::ERROR);
#endif
                    break;
            }

            break;

        case txp::COPY_DATA_TO_HEAP:
            data = new uint8_t;
            setDataAllocatedFlag(1);
            switch(pDataLocation) {
                case txp::BUFFER_HAS_DATA:
                    *data = pBuffer->getAttr_uint8(pOffset);
                    break;
                case txp::BUFFER_HAS_PTR_TO_DATA:
                    {
                        uint8_t* l_Data = pBuffer->getAttrPtr_uint8(pOffset);
                        *data = *l_Data;
                    }
                    break;
                default:
                    // Should never get here...
#ifdef TXP_DEVELOPMENT
                    snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_uint8::AttrPtr_uint8(): pDataLocation=%d, pOption=COPY_DATA_TO_HEAP", pDataLocation);
                    d_log.write(d_log_buffer, txp::Log::ERROR);
#endif
                    break;
            }

            break;

        case txp::COPY_DATA_TO_OBJECT:
            switch(pDataLocation) {
                case txp::BUFFER_HAS_DATA:
                    data = new uint8_t;
                    setDataAllocatedFlag(1);
                    *data = pBuffer->getAttr_uint8(pOffset);
                    break;
                case txp::BUFFER_HAS_PTR_TO_DATA:
                    data = pBuffer->getAttrPtr_uint8(pOffset);
                    break;
                default:
                    // Should never get here...
#ifdef TXP_DEVELOPMENT
                    snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_uint8::AttrPtr_uint8(): pDataLocation=%d, pOption=COPY_DATA_TO_OBJECT", pDataLocation);
                    d_log.write(d_log_buffer, txp::Log::ERROR);
#endif
                    break;
            }

            break;

        case txp::COPY_DATA_TO_OBJECT_BUT_DO_NOT_COPY_ARRAYS:
            switch(pDataLocation) {
                case txp::BUFFER_HAS_DATA:
                    data = new uint8_t;
                    setDataAllocatedFlag(1);
                    *data = pBuffer->getAttr_uint8(pOffset);
                    break;
                case txp::BUFFER_HAS_PTR_TO_DATA:
                    data = pBuffer->getAttrPtr_uint8(pOffset);
                    break;
                default:
                    // Should never get here...
#ifdef TXP_DEVELOPMENT
                    snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_uint8::AttrPtr_uint8(): pDataLocation=%d, pOption=COPY_DATA_TO_OBJECT_BUT_DO_NOT_COPY_ARRAYS", pDataLocation);
                    d_log.write(d_log_buffer, txp::Log::ERROR);
#endif
                    break;
            }

            break;

        default:
            // Should never get here...
#ifdef TXP_DEVELOPMENT
            snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_uint8::AttrPtr_uint8(): pOption=%d", pOption);
            d_log.write(d_log_buffer, txp::Log::ERROR);
#endif
            break;
    }

#ifdef TXP_DEVELOPMENT
    snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_uint8::AttrPtr_uint8(pBuffer=%p, pOffset=%zu, pOption=%d, pDataLocation=%d)=%p", pBuffer, pOffset, pOption, pDataLocation, this);
    d_log.write(d_log_buffer, txp::Log::DEBUG, txp::Log::C_DTOR_LOGLEVEL_2);
#endif
};


txp::AttrPtr_uint16::AttrPtr_uint16(HeapBuffer* pBuffer, const size_t pOffset, const txp::DeserializeOption &pOption, const txp::DataLocation &pDataLocation) :
    Attribute(txp::PTR_UINT16, pBuffer, pOffset) {
    data = NULL;
    switch(pOption) {
        case txp::DO_NOT_COPY_DATA:
            switch(pDataLocation) {
                case txp::BUFFER_HAS_DATA:
                    data = pBuffer->getAddrData_uint16(pOffset);
                    break;
                case txp::BUFFER_HAS_PTR_TO_DATA:
                    data = pBuffer->getAttrPtr_uint16(pOffset);
                    break;
                default:
                    // Should never get here...
#ifdef TXP_DEVELOPMENT
                    snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_uint16::AttrPtr_uint16(): pDataLocation=%d, pOption=DO_NOT_COPY_DATA", pDataLocation);
                    d_log.write(d_log_buffer, txp::Log::ERROR);
#endif
                    break;
            }

            break;

        case txp::COPY_DATA_TO_HEAP:
            data = new uint16_t;
            setDataAllocatedFlag(1);
            switch(pDataLocation) {
                case txp::BUFFER_HAS_DATA:
                    *data = pBuffer->getAttr_uint16(pOffset);
                    break;
                case txp::BUFFER_HAS_PTR_TO_DATA:
                    {
                        uint16_t* l_Data = pBuffer->getAttrPtr_uint16(pOffset);
                        *data = *l_Data;
                    }
                    break;
                default:
                    // Should never get here...
#ifdef TXP_DEVELOPMENT
                    snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_uint16::AttrPtr_uint16(): pDataLocation=%d, pOption=COPY_DATA_TO_HEAP", pDataLocation);
                    d_log.write(d_log_buffer, txp::Log::ERROR);
#endif
                    break;
            }

            break;

        case txp::COPY_DATA_TO_OBJECT:
             switch(pDataLocation) {
                case txp::BUFFER_HAS_DATA:
                    data = new uint16_t;
                    setDataAllocatedFlag(1);
                    *data = pBuffer->getAttr_uint16(pOffset);
                    break;
                case txp::BUFFER_HAS_PTR_TO_DATA:
                    data = pBuffer->getAttrPtr_uint16(pOffset);
                    break;
                default:
                    // Should never get here...
#ifdef TXP_DEVELOPMENT
                    snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_uint16::AttrPtr_uint16(): pDataLocation=%d, pOption=COPY_DATA_TO_OBJECT", pDataLocation);
                    d_log.write(d_log_buffer, txp::Log::ERROR);
#endif
                    break;
            }

            break;

       case txp::COPY_DATA_TO_OBJECT_BUT_DO_NOT_COPY_ARRAYS:
            switch(pDataLocation) {
                case txp::BUFFER_HAS_DATA:
                    data = new uint16_t;
                    setDataAllocatedFlag(1);
                    *data = pBuffer->getAttr_uint16(pOffset);
                    break;
                case txp::BUFFER_HAS_PTR_TO_DATA:
                    data = pBuffer->getAttrPtr_uint16(pOffset);
                    break;
                default:
                    // Should never get here...
#ifdef TXP_DEVELOPMENT
                    snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_uint16::AttrPtr_uint16(): pDataLocation=%d, pOption=COPY_DATA_TO_OBJECT_BUT_DO_NOT_COPY_ARRAYS", pDataLocation);
                    d_log.write(d_log_buffer, txp::Log::ERROR);
#endif
                    break;
            }

            break;

        default:
            // Should never get here...
#ifdef TXP_DEVELOPMENT
            snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_uint16::AttrPtr_uint16(): pOption=%d", pOption);
            d_log.write(d_log_buffer, txp::Log::ERROR);
#endif
            break;
    }

#ifdef TXP_DEVELOPMENT
    snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_uint16::AttrPtr_uint16(pBuffer=%p, pOffset=%zu, pOption=%d, pDataLocation=%d)=%p", pBuffer, pOffset, pOption, pDataLocation, this);
    d_log.write(d_log_buffer, txp::Log::DEBUG, txp::Log::C_DTOR_LOGLEVEL_2);
#endif
};


txp::AttrPtr_uint32::AttrPtr_uint32(HeapBuffer* pBuffer, const size_t pOffset, const txp::DeserializeOption &pOption, const txp::DataLocation &pDataLocation) :
    Attribute(txp::PTR_UINT32, pBuffer, pOffset) {
    data = NULL;
    switch(pOption) {
        case txp::DO_NOT_COPY_DATA:
            switch(pDataLocation) {
                case txp::BUFFER_HAS_DATA:
                    data = pBuffer->getAddrData_uint32(pOffset);
                    break;
                case txp::BUFFER_HAS_PTR_TO_DATA:
                    data = pBuffer->getAttrPtr_uint32(pOffset);
                    break;
                default:
                    // Should never get here...
#ifdef TXP_DEVELOPMENT
                    snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_uint32::AttrPtr_uint32(): pDataLocation=%d, pOption=DO_NOT_COPY_DATA", pDataLocation);
                    d_log.write(d_log_buffer, txp::Log::ERROR);
#endif
                    break;
            }

            break;

        case txp::COPY_DATA_TO_HEAP:
            data = new uint32_t;
            setDataAllocatedFlag(1);
            switch(pDataLocation) {
                case txp::BUFFER_HAS_DATA:
                    *data = pBuffer->getAttr_uint32(pOffset);
                    break;
                case txp::BUFFER_HAS_PTR_TO_DATA:
                    {
                        uint32_t* l_Data = pBuffer->getAttrPtr_uint32(pOffset);
                        *data = *l_Data;
                    }
                    break;
                default:
                    // Should never get here...
#ifdef TXP_DEVELOPMENT
                    snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_uint32::AttrPtr_uint32(): pDataLocation=%d, pOption=COPY_DATA_TO_HEAP", pDataLocation);
                    d_log.write(d_log_buffer, txp::Log::ERROR);
#endif
                    break;
            }

            break;

        case txp::COPY_DATA_TO_OBJECT:
            switch(pDataLocation) {
                case txp::BUFFER_HAS_DATA:
                    data = new uint32_t;
                    setDataAllocatedFlag(1);
                    *data = pBuffer->getAttr_uint32(pOffset);
                    break;
                case txp::BUFFER_HAS_PTR_TO_DATA:
                    data = pBuffer->getAttrPtr_uint32(pOffset);
                    break;
                default:
                    // Should never get here...
#ifdef TXP_DEVELOPMENT
                    snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_uint32::AttrPtr_uint32(): pDataLocation=%d, pOption=COPY_DATA_TO_OBJECT", pDataLocation);
                    d_log.write(d_log_buffer, txp::Log::ERROR);
#endif
                    break;
            }

            break;

        case txp::COPY_DATA_TO_OBJECT_BUT_DO_NOT_COPY_ARRAYS:
            switch(pDataLocation) {
                case txp::BUFFER_HAS_DATA:
                    data = new uint32_t;
                    setDataAllocatedFlag(1);
                    *data = pBuffer->getAttr_uint32(pOffset);
                    break;
                case txp::BUFFER_HAS_PTR_TO_DATA:
                    data = pBuffer->getAttrPtr_uint32(pOffset);
                    break;
                default:
                    // Should never get here...
#ifdef TXP_DEVELOPMENT
                    snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_uint32::AttrPtr_uint32(): pDataLocation=%d, pOption=COPY_DATA_TO_OBJECT_BUT_DO_NOT_COPY_ARRAYS", pDataLocation);
                    d_log.write(d_log_buffer, txp::Log::ERROR);
#endif
                    break;
            }

            break;

        default:
            // Should never get here...
#ifdef TXP_DEVELOPMENT
            snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_uint32::AttrPtr_uint32(): pOption=%d", pOption);
            d_log.write(d_log_buffer, txp::Log::ERROR);
#endif
            break;
    }

#ifdef TXP_DEVELOPMENT
    snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_uint32::AttrPtr_uint32(pBuffer=%p, pOffset=%zu, pOption=%d, pDataLocation=%d)=%p", pBuffer, pOffset, pOption, pDataLocation, this);
    d_log.write(d_log_buffer, txp::Log::DEBUG, txp::Log::C_DTOR_LOGLEVEL_2);
#endif
};


txp::AttrPtr_uint64::AttrPtr_uint64(HeapBuffer* pBuffer, const size_t pOffset, const txp::DeserializeOption &pOption, const txp::DataLocation &pDataLocation) :
    Attribute(txp::PTR_UINT64, pBuffer, pOffset) {
    data = NULL;
    switch(pOption) {
        case txp::DO_NOT_COPY_DATA:
            switch(pDataLocation) {
                case txp::BUFFER_HAS_DATA:
                    data = pBuffer->getAddrData_uint64(pOffset);
                    break;
                case txp::BUFFER_HAS_PTR_TO_DATA:
                    data = pBuffer->getAttrPtr_uint64(pOffset);
                    break;
                default:
                    // Should never get here...
#ifdef TXP_DEVELOPMENT
                    snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_uint64::AttrPtr_uint64(): pDataLocation=%d, pOption=DO_NOT_COPY_DATA", pDataLocation);
                    d_log.write(d_log_buffer, txp::Log::ERROR);
#endif
                    break;
            }

            break;

        case txp::COPY_DATA_TO_HEAP:
            data = new uint64_t;
            setDataAllocatedFlag(1);
            switch(pDataLocation) {
                case txp::BUFFER_HAS_DATA:
                    *data = pBuffer->getAttr_uint64(pOffset);
                    break;
                case txp::BUFFER_HAS_PTR_TO_DATA:
                    {
                        uint64_t* l_Data = pBuffer->getAttrPtr_uint64(pOffset);
                        *data = *l_Data;
                    }
                    break;
                default:
                    // Should never get here...
#ifdef TXP_DEVELOPMENT
                    snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_uint64::AttrPtr_uint64(): pDataLocation=%d, pOption=COPY_DATA_TO_HEAP", pDataLocation);
                    d_log.write(d_log_buffer, txp::Log::ERROR);
#endif
                    break;
            }

            break;

        case txp::COPY_DATA_TO_OBJECT:
            switch(pDataLocation) {
                case txp::BUFFER_HAS_DATA:
                    data = new uint64_t;
                    setDataAllocatedFlag(1);
                    *data = pBuffer->getAttr_uint64(pOffset);
                    break;
                case txp::BUFFER_HAS_PTR_TO_DATA:
                    data = pBuffer->getAttrPtr_uint64(pOffset);
                    break;
                default:
                    // Should never get here...
#ifdef TXP_DEVELOPMENT
                    snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_uint64::AttrPtr_uint64(): pDataLocation=%d, pOption=COPY_DATA_TO_OBJECT", pDataLocation);
                    d_log.write(d_log_buffer, txp::Log::ERROR);
#endif
                    break;
            }

            break;

        case txp::COPY_DATA_TO_OBJECT_BUT_DO_NOT_COPY_ARRAYS:
            switch(pDataLocation) {
                case txp::BUFFER_HAS_DATA:
                    data = new uint64_t;
                    setDataAllocatedFlag(1);
                    *data = pBuffer->getAttr_uint64(pOffset);
                    break;
                case txp::BUFFER_HAS_PTR_TO_DATA:
                    data = pBuffer->getAttrPtr_uint64(pOffset);
                    break;
                default:
                    // Should never get here...
#ifdef TXP_DEVELOPMENT
                    snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_uint64::AttrPtr_uint64(): pDataLocation=%d, pOption=COPY_DATA_TO_OBJECT_BUT_DO_NOT_COPY_ARRAYS", pDataLocation);
                    d_log.write(d_log_buffer, txp::Log::ERROR);
#endif
                    break;
            }

            break;

        default:
            // Should never get here...
#ifdef TXP_DEVELOPMENT
            snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_uint64::AttrPtr_uint64(): pOption=%d", pOption);
            d_log.write(d_log_buffer, txp::Log::ERROR);
#endif
            break;
    }

#ifdef TXP_DEVELOPMENT
    snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_uint64::AttrPtr_uint64(pBuffer=%p, pOffset=%zu, pOption=%d, pDataLocation=%d)=%p", pBuffer, pOffset, pOption, pDataLocation, this);
    d_log.write(d_log_buffer, txp::Log::DEBUG, txp::Log::C_DTOR_LOGLEVEL_2);
#endif
};


txp::AttrPtr_int8::AttrPtr_int8(HeapBuffer* pBuffer, const size_t pOffset, const txp::DeserializeOption &pOption, const txp::DataLocation &pDataLocation) :
    Attribute(txp::PTR_INT8, pBuffer, pOffset) {
    data = NULL;
    switch(pOption) {
        case txp::DO_NOT_COPY_DATA:
            switch(pDataLocation) {
                case txp::BUFFER_HAS_DATA:
                    data = pBuffer->getAddrData_int8(pOffset);
                    break;
                case txp::BUFFER_HAS_PTR_TO_DATA:
                    data = pBuffer->getAttrPtr_int8(pOffset);
                    break;
                default:
                    // Should never get here...
#ifdef TXP_DEVELOPMENT
                    snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_int8::AttrPtr_int8(): pDataLocation=%d, pOption=DO_NOT_COPY_DATA", pDataLocation);
                    d_log.write(d_log_buffer, txp::Log::ERROR);
#endif
                    break;
            }

            break;

        case txp::COPY_DATA_TO_HEAP:
            data = new int8_t;
            setDataAllocatedFlag(1);
            switch(pDataLocation) {
                case txp::BUFFER_HAS_DATA:
                    *data = pBuffer->getAttr_int8(pOffset);
                    break;
                case txp::BUFFER_HAS_PTR_TO_DATA:
                    {
                        int8_t* l_Data = pBuffer->getAttrPtr_int8(pOffset);
                        *data = *l_Data;
                    }
                    break;
                default:
                    // Should never get here...
#ifdef TXP_DEVELOPMENT
                    snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_int8::AttrPtr_int8(): pDataLocation=%d, pOption=COPY_DATA_TO_HEAP", pDataLocation);
                    d_log.write(d_log_buffer, txp::Log::ERROR);
#endif
                    break;
            }

            break;

        case txp::COPY_DATA_TO_OBJECT:
            switch(pDataLocation) {
                case txp::BUFFER_HAS_DATA:
                    data = new int8_t;
                    setDataAllocatedFlag(1);
                    *data = pBuffer->getAttr_int8(pOffset);
                    break;
                case txp::BUFFER_HAS_PTR_TO_DATA:
                    data = pBuffer->getAttrPtr_int8(pOffset);
                    break;
                default:
                    // Should never get here...
#ifdef TXP_DEVELOPMENT
                    snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_int8::AttrPtr_int8(): pDataLocation=%d, pOption=COPY_DATA_TO_OBJECT", pDataLocation);
                    d_log.write(d_log_buffer, txp::Log::ERROR);
#endif
                    break;
            }

            break;

        case txp::COPY_DATA_TO_OBJECT_BUT_DO_NOT_COPY_ARRAYS:
            switch(pDataLocation) {
                case txp::BUFFER_HAS_DATA:
                    data = new int8_t;
                    setDataAllocatedFlag(1);
                    *data = pBuffer->getAttr_int8(pOffset);
                    break;
                case txp::BUFFER_HAS_PTR_TO_DATA:
                    data = pBuffer->getAttrPtr_int8(pOffset);
                    break;
                default:
                    // Should never get here...
#ifdef TXP_DEVELOPMENT
                    snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_int8::AttrPtr_int8(): pDataLocation=%d, pOption=COPY_DATA_TO_OBJECT_BUT_DO_NOT_COPY_ARRAYS", pDataLocation);
                    d_log.write(d_log_buffer, txp::Log::ERROR);
#endif
                    break;
            }

            break;

        default:
            // Should never get here...
#ifdef TXP_DEVELOPMENT
            snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_int8::AttrPtr_int8(): pOption=%d", pOption);
            d_log.write(d_log_buffer, txp::Log::ERROR);
#endif
            break;
    }

#ifdef TXP_DEVELOPMENT
    snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_int8::AttrPtr_int8(pBuffer=%p, pOffset=%zu, pOption=%d, pDataLocation=%d)=%p", pBuffer, pOffset, pOption, pDataLocation, this);
    d_log.write(d_log_buffer, txp::Log::DEBUG, txp::Log::C_DTOR_LOGLEVEL_2);
#endif
};


txp::AttrPtr_int16::AttrPtr_int16(HeapBuffer* pBuffer, const size_t pOffset, const txp::DeserializeOption &pOption, const txp::DataLocation &pDataLocation) :
    Attribute(txp::PTR_INT16, pBuffer, pOffset) {
    data = NULL;
    switch(pOption) {
        case txp::DO_NOT_COPY_DATA:
            switch(pDataLocation) {
                case txp::BUFFER_HAS_DATA:
                    data = pBuffer->getAddrData_int16(pOffset);
                    break;
                case txp::BUFFER_HAS_PTR_TO_DATA:
                    data = pBuffer->getAttrPtr_int16(pOffset);
                    break;
                default:
                    // Should never get here...
#ifdef TXP_DEVELOPMENT
                    snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_int16::AttrPtr_int16(): pDataLocation=%d, pOption=DO_NOT_COPY_DATA", pDataLocation);
                    d_log.write(d_log_buffer, txp::Log::ERROR);
#endif
                    break;
            }

            break;

        case txp::COPY_DATA_TO_HEAP:
            data = new int16_t;
            setDataAllocatedFlag(1);
            switch(pDataLocation) {
                case txp::BUFFER_HAS_DATA:
                    *data = pBuffer->getAttr_int16(pOffset);
                    break;
                case txp::BUFFER_HAS_PTR_TO_DATA:
                    {
                        int16_t* l_Data = pBuffer->getAttrPtr_int16(pOffset);
                        *data = *l_Data;
                    }
                    break;
                default:
                    // Should never get here...
#ifdef TXP_DEVELOPMENT
                    snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_int16::AttrPtr_int16(): pDataLocation=%d, pOption=COPY_DATA_TO_HEAP", pDataLocation);
                    d_log.write(d_log_buffer, txp::Log::ERROR);
#endif
                    break;
            }

            break;

        case txp::COPY_DATA_TO_OBJECT:
            switch(pDataLocation) {
                case txp::BUFFER_HAS_DATA:
                    data = new int16_t;
                    setDataAllocatedFlag(1);
                    *data = pBuffer->getAttr_int16(pOffset);
                    break;
                case txp::BUFFER_HAS_PTR_TO_DATA:
                    data = pBuffer->getAttrPtr_int16(pOffset);
                    break;
                default:
                    // Should never get here...
#ifdef TXP_DEVELOPMENT
                    snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_int16::AttrPtr_int16(): pDataLocation=%d, pOption=COPY_DATA_TO_OBJECT", pDataLocation);
                    d_log.write(d_log_buffer, txp::Log::ERROR);
#endif
                    break;
            }

            break;

        case txp::COPY_DATA_TO_OBJECT_BUT_DO_NOT_COPY_ARRAYS:
            switch(pDataLocation) {
                case txp::BUFFER_HAS_DATA:
                    data = new int16_t;
                    setDataAllocatedFlag(1);
                    *data = pBuffer->getAttr_int16(pOffset);
                    break;
                case txp::BUFFER_HAS_PTR_TO_DATA:
                    data = pBuffer->getAttrPtr_int16(pOffset);
                    break;
                default:
                    // Should never get here...
#ifdef TXP_DEVELOPMENT
                    snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_int16::AttrPtr_int16(): pDataLocation=%d, pOption=COPY_DATA_TO_OBJECT_BUT_DO_NOT_COPY_ARRAYS", pDataLocation);
                    d_log.write(d_log_buffer, txp::Log::ERROR);
#endif
                    break;
            }

            break;

        default:
            // Should never get here...
#ifdef TXP_DEVELOPMENT
            snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_int16::AttrPtr_int16(): pOption=%d", pOption);
            d_log.write(d_log_buffer, txp::Log::ERROR);
#endif
            break;
    }

#ifdef TXP_DEVELOPMENT
    snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_int16::AttrPtr_int16(pBuffer=%p, pOffset=%zu, pOption=%d, pDataLocation=%d)=%p", pBuffer, pOffset, pOption, pDataLocation, this);
    d_log.write(d_log_buffer, txp::Log::DEBUG, txp::Log::C_DTOR_LOGLEVEL_2);
#endif
};


txp::AttrPtr_int32::AttrPtr_int32(HeapBuffer* pBuffer, const size_t pOffset, const txp::DeserializeOption &pOption, const txp::DataLocation &pDataLocation) :
    Attribute(txp::PTR_INT32, pBuffer, pOffset) {
    data = NULL;
    switch(pOption) {
        case txp::DO_NOT_COPY_DATA:
            switch(pDataLocation) {
                case txp::BUFFER_HAS_DATA:
                    data = pBuffer->getAddrData_int32(pOffset);
                    break;
                case txp::BUFFER_HAS_PTR_TO_DATA:
                    data = pBuffer->getAttrPtr_int32(pOffset);
                    break;
                default:
                    // Should never get here...
#ifdef TXP_DEVELOPMENT
                    snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_int32::AttrPtr_int32(): pDataLocation=%d, pOption=DO_NOT_COPY_DATA", pDataLocation);
                    d_log.write(d_log_buffer, txp::Log::ERROR);
#endif
                    break;
            }

            break;

        case txp::COPY_DATA_TO_HEAP:
            data = new int32_t;
            setDataAllocatedFlag(1);
            switch(pDataLocation) {
                case txp::BUFFER_HAS_DATA:
                    *data = pBuffer->getAttr_int32(pOffset);
                    break;
                case txp::BUFFER_HAS_PTR_TO_DATA:
                    {
                        int32_t* l_Data = pBuffer->getAttrPtr_int32(pOffset);
                        *data = *l_Data;
                    }
                    break;
                default:
                    // Should never get here...
#ifdef TXP_DEVELOPMENT
                    snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_int32::AttrPtr_int32(): pDataLocation=%d, pOption=COPY_DATA_TO_HEAP", pDataLocation);
                    d_log.write(d_log_buffer, txp::Log::ERROR);
#endif
                    break;
            }

            break;

        case txp::COPY_DATA_TO_OBJECT:
            switch(pDataLocation) {
                case txp::BUFFER_HAS_DATA:
                    data = new int32_t;
                    setDataAllocatedFlag(1);
                    *data = pBuffer->getAttr_int32(pOffset);
                    break;
                case txp::BUFFER_HAS_PTR_TO_DATA:
                    data = pBuffer->getAttrPtr_int32(pOffset);
                    break;
                default:
                    // Should never get here...
#ifdef TXP_DEVELOPMENT
                    snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_int32::AttrPtr_int32(): pDataLocation=%d, pOption=COPY_DATA_TO_OBJECT", pDataLocation);
                    d_log.write(d_log_buffer, txp::Log::ERROR);
#endif
                    break;
            }

            break;

        case txp::COPY_DATA_TO_OBJECT_BUT_DO_NOT_COPY_ARRAYS:
            switch(pDataLocation) {
                case txp::BUFFER_HAS_DATA:
                    data = new int32_t;
                    setDataAllocatedFlag(1);
                    *data = pBuffer->getAttr_int32(pOffset);
                    break;
                case txp::BUFFER_HAS_PTR_TO_DATA:
                    data = pBuffer->getAttrPtr_int32(pOffset);
                    break;
                default:
                    // Should never get here...
#ifdef TXP_DEVELOPMENT
                    snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_int32::AttrPtr_int32(): pDataLocation=%d, pOption=COPY_DATA_TO_OBJECT_BUT_DO_NOT_COPY_ARRAYS", pDataLocation);
                    d_log.write(d_log_buffer, txp::Log::ERROR);
#endif
                    break;
            }

            break;

        default:
            // Should never get here...
#ifdef TXP_DEVELOPMENT
            snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_int32::AttrPtr_int32(): pOption=%d", pOption);
            d_log.write(d_log_buffer, txp::Log::ERROR);
#endif
            break;
    }

#ifdef TXP_DEVELOPMENT
    snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_int32::AttrPtr_int32(pBuffer=%p, pOffset=%zu, pOption=%d, pDataLocation=%d)=%p", pBuffer, pOffset, pOption, pDataLocation, this);
    d_log.write(d_log_buffer, txp::Log::DEBUG, txp::Log::C_DTOR_LOGLEVEL_2);
#endif
};


txp::AttrPtr_int64::AttrPtr_int64(HeapBuffer* pBuffer, const size_t pOffset, const txp::DeserializeOption &pOption, const txp::DataLocation &pDataLocation) :
    Attribute(txp::PTR_INT64, pBuffer, pOffset) {
    data = NULL;
    switch(pOption) {
        case txp::DO_NOT_COPY_DATA:
            switch(pDataLocation) {
                case txp::BUFFER_HAS_DATA:
                    data = pBuffer->getAddrData_int64(pOffset);
                    break;
                case txp::BUFFER_HAS_PTR_TO_DATA:
                    data = pBuffer->getAttrPtr_int64(pOffset);
                    break;
                default:
                    // Should never get here...
#ifdef TXP_DEVELOPMENT
                    snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_int64::AttrPtr_int64(): pDataLocation=%d, pOption=DO_NOT_COPY_DATA", pDataLocation);
                    d_log.write(d_log_buffer, txp::Log::ERROR);
#endif
                    break;
            }

            break;

        case txp::COPY_DATA_TO_HEAP:
            data = new int64_t;
            setDataAllocatedFlag(1);
            switch(pDataLocation) {
                case txp::BUFFER_HAS_DATA:
                    *data = pBuffer->getAttr_int64(pOffset);
                    break;
                case txp::BUFFER_HAS_PTR_TO_DATA:
                    {
                        int64_t* l_Data = pBuffer->getAttrPtr_int64(pOffset);
                        *data = *l_Data;
                    }
                    break;
                default:
                    // Should never get here...
#ifdef TXP_DEVELOPMENT
                    snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_int64::AttrPtr_int64(): pDataLocation=%d, pOption=COPY_DATA_TO_HEAP", pDataLocation);
                    d_log.write(d_log_buffer, txp::Log::ERROR);
#endif
                    break;
            }

            break;

        case txp::COPY_DATA_TO_OBJECT:
            switch(pDataLocation) {
                case txp::BUFFER_HAS_DATA:
                    data = new int64_t;
                    setDataAllocatedFlag(1);
                    *data = pBuffer->getAttr_int64(pOffset);
                    break;
                case txp::BUFFER_HAS_PTR_TO_DATA:
                    data = pBuffer->getAttrPtr_int64(pOffset);
                    break;
                default:
                    // Should never get here...
#ifdef TXP_DEVELOPMENT
                    snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_int64::AttrPtr_int64(): pDataLocation=%d, pOption=COPY_DATA_TO_OBJECT", pDataLocation);
                    d_log.write(d_log_buffer, txp::Log::ERROR);
#endif
                    break;
            }

            break;

        case txp::COPY_DATA_TO_OBJECT_BUT_DO_NOT_COPY_ARRAYS:
            switch(pDataLocation) {
                case txp::BUFFER_HAS_DATA:
                    data = new int64_t;
                    setDataAllocatedFlag(1);
                    *data = pBuffer->getAttr_int64(pOffset);
                    break;
                case txp::BUFFER_HAS_PTR_TO_DATA:
                    data = pBuffer->getAttrPtr_int64(pOffset);
                    break;
                default:
                    // Should never get here...
#ifdef TXP_DEVELOPMENT
                    snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_int64::AttrPtr_int64(): pDataLocation=%d, pOption=COPY_DATA_TO_OBJECT_BUT_DO_NOT_COPY_ARRAYS", pDataLocation);
                    d_log.write(d_log_buffer, txp::Log::ERROR);
#endif
                    break;
            }

            break;

        default:
            // Should never get here...
#ifdef TXP_DEVELOPMENT
            snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_int64::AttrPtr_int64(): pOption=%d", pOption);
            d_log.write(d_log_buffer, txp::Log::ERROR);
#endif
            break;
    }

#ifdef TXP_DEVELOPMENT
    snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_int64::AttrPtr_int64(pBuffer=%p, pOffset=%zu, pOption=%d, pDataLocation=%d)=%p", pBuffer, pOffset, pOption, pDataLocation, this);
    d_log.write(d_log_buffer, txp::Log::DEBUG, txp::Log::C_DTOR_LOGLEVEL_2);
#endif
};


txp::AttrPtr_char_array::AttrPtr_char_array(HeapBuffer* pBuffer, const size_t pOffset, const txp::DeserializeOption &pOption, const txp::DataLocation &pDataLocation) :
    Attribute(txp::PTR_CHAR_ARRAY, pBuffer, pOffset) {
    length = pBuffer->getAttr_char_array_length(pOffset);
    data = NULL;
    switch(pOption) {
        case txp::DO_NOT_COPY_DATA:
            switch(pDataLocation) {
                case txp::BUFFER_HAS_DATA:
                    data = pBuffer->getAddrData_char_array_data(pOffset);
                    break;
                case txp::BUFFER_HAS_PTR_TO_DATA:
                    data = pBuffer->getAttrPtr_char_array_data(pOffset);
                    break;
                default:
                    // Should never get here...
#ifdef TXP_DEVELOPMENT
                    snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_char_array::AttrPtr_char_array(): pDataLocation=%d, pOption=DO_NOT_COPY_DATA", pDataLocation);
                    d_log.write(d_log_buffer, txp::Log::ERROR);
#endif
                    break;
            }

            break;

        case txp::COPY_DATA_TO_OBJECT_BUT_DO_NOT_COPY_ARRAYS:
            switch(pDataLocation) {
                case txp::BUFFER_HAS_DATA:
                    data = pBuffer->getAddrData_char_array_data(pOffset);
                    break;
                case txp::BUFFER_HAS_PTR_TO_DATA:
                    data = pBuffer->getAttrPtr_char_array_data(pOffset);
                    break;
                default:
                    // Should never get here...
#ifdef TXP_DEVELOPMENT
                    snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_char_array::AttrPtr_char_array(): pDataLocation=%d, pOption=COPY_DATA_TO_OBJECT_BUT_DO_NOT_COPY_ARRAYS", pDataLocation);
                    d_log.write(d_log_buffer, txp::Log::ERROR);
#endif
                    break;
            }

            break;

        case txp::COPY_DATA_TO_HEAP:
            data = new char[length];
            setDataAllocatedFlag(1);
            switch(pDataLocation) {
                case txp::BUFFER_HAS_DATA:
                    memcpy((void*)data, pBuffer->getAddrData_char_array_data(pOffset), length);
                    break;
                case txp::BUFFER_HAS_PTR_TO_DATA:
                    memcpy((void*)data, pBuffer->getAttrPtr_char_array_data(pOffset), length);
                    break;
                default:
                    // Should never get here...
#ifdef TXP_DEVELOPMENT
                    snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_char_array::AttrPtr_char_array(): pDataLocation=%d, pOption=COPY_DATA_TO_HEAP", pDataLocation);
                    d_log.write(d_log_buffer, txp::Log::ERROR);
#endif
                    break;
            }

            break;

        case txp::COPY_DATA_TO_OBJECT:
            switch(pDataLocation) {
                case txp::BUFFER_HAS_DATA:
                    data = new char[length];
                    setDataAllocatedFlag(1);
                    memcpy((void*)data, pBuffer->getAddrData_char_array_data(pOffset), length);
                    break;
                case txp::BUFFER_HAS_PTR_TO_DATA:
                    data = pBuffer->getAttrPtr_char_array_data(pOffset);
                    break;
                default:
                    // Should never get here...
#ifdef TXP_DEVELOPMENT
                    snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_char_array::AttrPtr_char_array(): pDataLocation=%d, pOption=COPY_DATA_TO_OBJECT", pDataLocation);
                    d_log.write(d_log_buffer, txp::Log::ERROR);
#endif
                    break;
            }

            break;

        default:
            // Should never get here...
#ifdef TXP_DEVELOPMENT
            snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_char_array::AttrPtr_char_array(): pOption=%d", pOption);
            d_log.write(d_log_buffer, txp::Log::ERROR);
#endif
            break;
    }

#ifdef TXP_DEVELOPMENT
    snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_char_array::AttrPtr_char_array(pBuffer=%p, pOffset=%zu, pOption=%d, pDataLocation=%d)=%p", pBuffer, pOffset, pOption, pDataLocation, this);
    d_log.write(d_log_buffer, txp::Log::DEBUG, txp::Log::C_DTOR_LOGLEVEL_2);
#endif
};


txp::AttrPtr_array_of_char_arrays::AttrPtr_array_of_char_arrays(HeapBuffer* pBuffer, const size_t pOffset, const txp::DeserializeOption &pOption, const txp::DataLocation &pDataLocation) :
    Attribute(txp::PTR_ARRAY_OF_CHAR_ARRAYS, pBuffer, pOffset) {
    number_of_elements = pBuffer->getAttr_array_of_char_arrays_number_of_elements(pOffset);
    length_of_data = (size_t)(number_of_elements * (sizeof(TXP_ARRAY_OF_CHAR_ARRAYS_ELEMENT_SIZE_TYPE) + sizeof(TXP_ARRAY_OF_CHAR_ARRAYS_ELEMENT_DATA_TYPE)));
    length_of_values = pBuffer->getAttr_array_of_char_arrays_length(pOffset);
    attrFlag2.flags2 = 0;
    data = new CharArray;
    data->reserve(txp::DEFAULT_NUMBER_OF_CHAR_ARRAYS);
    setDataAllocatedFlag(1);

    size_t l_Offset = pOffset;
    switch(pOption) {
        case txp::DO_NOT_COPY_DATA:
            switch(pDataLocation) {
                case txp::BUFFER_HAS_DATA:
                	{
    					for (size_t i = 0; i < number_of_elements; ++i) {
    						CharArray_Element l_Element = CharArray_Element(pBuffer->getAttr_array_of_char_arrays_element_size(l_Offset), pBuffer->getAddrData_array_of_char_arrays_element_data(l_Offset));
    						data->push_back(l_Element);
    						l_Offset += (sizeof(TXP_ARRAY_OF_CHAR_ARRAYS_ELEMENT_SIZE_TYPE) + pBuffer->getAttr_array_of_char_arrays_element_size(l_Offset));
    					}
                    	break;
                    }
                case txp::BUFFER_HAS_PTR_TO_DATA:
                	{
    					for (size_t i = 0; i < number_of_elements; ++i) {
    						CharArray_Element l_Element = CharArray_Element(pBuffer->getAttr_array_of_char_arrays_element_size(l_Offset), pBuffer->getAttrPtr_array_of_char_arrays_element_data(l_Offset));
    						data->push_back(l_Element);
    						l_Offset += (sizeof(TXP_ARRAY_OF_CHAR_ARRAYS_ELEMENT_SIZE_TYPE) + pBuffer->getAttr_array_of_char_arrays_element_size(l_Offset));
    					}
                    	break;
                    }
                default:
                    // Should never get here...
#ifdef TXP_DEVELOPMENT
                    snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_array_of_char_arrays::AttrPtr_array_of_char_arrays(): pDataLocation=%d, pOption=DO_NOT_COPY_DATA", pDataLocation);
                    d_log.write(d_log_buffer, txp::Log::ERROR);
#endif
                    break;
            }

            break;

        case txp::COPY_DATA_TO_OBJECT_BUT_DO_NOT_COPY_ARRAYS:
            switch(pDataLocation) {
                case txp::BUFFER_HAS_DATA:
                	{
    					for (size_t i = 0; i < number_of_elements; ++i) {
    						CharArray_Element l_Element = CharArray_Element(pBuffer->getAttr_array_of_char_arrays_element_size(l_Offset), pBuffer->getAddrData_array_of_char_arrays_element_data(l_Offset));
    						data->push_back(l_Element);
    						l_Offset += (sizeof(TXP_ARRAY_OF_CHAR_ARRAYS_ELEMENT_SIZE_TYPE) + pBuffer->getAttr_array_of_char_arrays_element_size(l_Offset));
    					}
                    	break;
                    }
                case txp::BUFFER_HAS_PTR_TO_DATA:
                	{
    					for (size_t i = 0; i < number_of_elements; ++i) {
    						CharArray_Element l_Element = CharArray_Element(pBuffer->getAttr_array_of_char_arrays_element_size(l_Offset), pBuffer->getAttrPtr_array_of_char_arrays_element_data(l_Offset));
    						data->push_back(l_Element);
    						l_Offset += (sizeof(TXP_ARRAY_OF_CHAR_ARRAYS_ELEMENT_SIZE_TYPE) + pBuffer->getAttr_array_of_char_arrays_element_size(l_Offset));
    					}
                    	break;
                    }
                default:
                    // Should never get here...
#ifdef TXP_DEVELOPMENT
                    snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_array_of_char_arrays::AttrPtr_array_of_char_arrays(): pDataLocation=%d, pOption=COPY_DATA_TO_OBJECT_BUT_DO_NOT_COPY_ARRAYS", pDataLocation);
                    d_log.write(d_log_buffer, txp::Log::ERROR);
#endif
                    break;
            }

            break;

        case txp::COPY_DATA_TO_HEAP:
            switch(pDataLocation) {
                case txp::BUFFER_HAS_DATA:
                	{
    					for (size_t i = 0; i < number_of_elements; ++i) {
                    		char* l_CharArray = new char[pBuffer->getAttr_array_of_char_arrays_element_size(l_Offset)];
                    		memcpy((void*)l_CharArray, pBuffer->getAddrData_array_of_char_arrays_element_data(l_Offset), pBuffer->getAttr_array_of_char_arrays_element_size(l_Offset));
    						CharArray_Element l_Element = CharArray_Element(pBuffer->getAttr_array_of_char_arrays_element_size(l_Offset), l_CharArray);
    						data->push_back(l_Element);
    						l_Offset += (sizeof(TXP_ARRAY_OF_CHAR_ARRAYS_ELEMENT_SIZE_TYPE) + pBuffer->getAttr_array_of_char_arrays_element_size(l_Offset));
    					}
		            	setDataElementCharArraysAllocatedFlag(1);
                    	break;
                    }
                case txp::BUFFER_HAS_PTR_TO_DATA:
                	{
    					for (size_t i = 0; i < number_of_elements; ++i) {
                    		char* l_CharArray = new char[pBuffer->getAttr_array_of_char_arrays_element_size(l_Offset)];
                    		memcpy((void*)l_CharArray, pBuffer->getAttrPtr_array_of_char_arrays_element_data(l_Offset), pBuffer->getAttr_array_of_char_arrays_element_size(l_Offset));
    						CharArray_Element l_Element = CharArray_Element(pBuffer->getAttr_array_of_char_arrays_element_size(l_Offset), l_CharArray);
    						data->push_back(l_Element);
    						l_Offset += (sizeof(TXP_ARRAY_OF_CHAR_ARRAYS_ELEMENT_SIZE_TYPE) + pBuffer->getAttr_array_of_char_arrays_element_size(l_Offset));
    					}
		            	setDataElementCharArraysAllocatedFlag(1);
                    	break;
                    }
                default:
                    // Should never get here...
#ifdef TXP_DEVELOPMENT
                    snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_array_of_char_arrays::AttrPtr_array_of_char_arrays(): pDataLocation=%d, pOption=COPY_DATA_TO_HEAP", pDataLocation);
                    d_log.write(d_log_buffer, txp::Log::ERROR);
#endif
                    break;
            }

            break;

        case txp::COPY_DATA_TO_OBJECT:
        	// We cannot honor this...  Create elements in heap and copy char arrays to heap...  @@DLH
            switch(pDataLocation) {
                case txp::BUFFER_HAS_DATA:
                	{
    					for (size_t i = 0; i < number_of_elements; ++i) {
                    		char* l_CharArray = new char[pBuffer->getAttr_array_of_char_arrays_element_size(l_Offset)];
                    		memcpy((void*)l_CharArray, pBuffer->getAddrData_array_of_char_arrays_element_data(l_Offset), pBuffer->getAttr_array_of_char_arrays_element_size(l_Offset));
    						CharArray_Element l_Element = CharArray_Element(pBuffer->getAttr_array_of_char_arrays_element_size(l_Offset), l_CharArray);
    						data->push_back(l_Element);
    						l_Offset += (sizeof(TXP_ARRAY_OF_CHAR_ARRAYS_ELEMENT_SIZE_TYPE) + pBuffer->getAttr_array_of_char_arrays_element_size(l_Offset));
    					}
		            	setDataElementCharArraysAllocatedFlag(1);
                    	break;
                    }
                case txp::BUFFER_HAS_PTR_TO_DATA:
                	{
    					for (size_t i = 0; i < number_of_elements; ++i) {
                    		char* l_CharArray = new char[pBuffer->getAttr_array_of_char_arrays_element_size(l_Offset)];
                    		memcpy((void*)l_CharArray, pBuffer->getAttrPtr_array_of_char_arrays_element_data(l_Offset), pBuffer->getAttr_array_of_char_arrays_element_size(l_Offset));
    						CharArray_Element l_Element = CharArray_Element(pBuffer->getAttr_array_of_char_arrays_element_size(l_Offset), l_CharArray);
    						data->push_back(l_Element);
    						l_Offset += (sizeof(TXP_ARRAY_OF_CHAR_ARRAYS_ELEMENT_SIZE_TYPE) + pBuffer->getAttr_array_of_char_arrays_element_size(l_Offset));
    					}
		            	setDataElementCharArraysAllocatedFlag(1);
                    	break;
                    }
                default:
                    // Should never get here...
#ifdef TXP_DEVELOPMENT
                    snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_array_of_char_arrays::AttrPtr_array_of_char_arrays(): pDataLocation=%d, pOption=COPY_DATA_TO_OBJECT", pDataLocation);
                    d_log.write(d_log_buffer, txp::Log::ERROR);
#endif
                    break;
            }

            break;

        default:
            // Should never get here...
#ifdef TXP_DEVELOPMENT
            snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_array_of_char_arrays::AttrPtr_array_of_char_arrays(): pOption=%d", pOption);
            d_log.write(d_log_buffer, txp::Log::ERROR);
#endif
            break;
    }

#ifdef TXP_DEVELOPMENT
    snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_array_of_char_arrays::AttrPtr_array_of_char_arrays(pBuffer=%p, pOffset=%zu, pOption=%d, pDataLocation=%d)=%p", pBuffer, pOffset, pOption, pDataLocation, this);
    d_log.write(d_log_buffer, txp::Log::DEBUG, txp::Log::C_DTOR_LOGLEVEL_2);
#endif
};


//*****************************************************************************
//  Attribute destructors
//*****************************************************************************
txp::Attribute::~Attribute() {

#ifdef TXP_DEVELOPMENT
    snprintf(d_log_buffer, sizeof(d_log_buffer), "Attribute::~Attribute()=%p", this);
    d_log.write(d_log_buffer, txp::Log::DEBUG, txp::Log::C_DTOR_LOGLEVEL_2);
#endif

}


txp::AttrPtr_char::~AttrPtr_char() {

#ifdef TXP_DEVELOPMENT
    snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_char::~AttrPtr_char()=%p, isDataAllocated()=%s", this, (isDataAllocated() ? "True" : "False"));
    d_log.write(d_log_buffer, txp::Log::DEBUG, txp::Log::C_DTOR_LOGLEVEL_2);
#endif

    if (isDataAllocated()) {
        setDataAllocatedFlag(0);
        char* l_Temp = data;
        data = 0;
        delete l_Temp;
    }
};


txp::AttrPtr_uint8::~AttrPtr_uint8() {

#ifdef TXP_DEVELOPMENT
    snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_uint8::~AttrPtr_uint8()=%p, isDataAllocated()=%s", this, (isDataAllocated() ? "True" : "False"));
    d_log.write(d_log_buffer, txp::Log::DEBUG, txp::Log::C_DTOR_LOGLEVEL_2);
#endif

    if (isDataAllocated()) {
        setDataAllocatedFlag(0);
        uint8_t* l_Temp = data;
        data = 0;
        delete l_Temp;
    }
};


txp::AttrPtr_uint16::~AttrPtr_uint16() {

#ifdef TXP_DEVELOPMENT
    snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_uint16::~AttrPtr_uint16()=%p, isDataAllocated()=%s", this, (isDataAllocated() ? "True" : "False"));
    d_log.write(d_log_buffer, txp::Log::DEBUG, txp::Log::C_DTOR_LOGLEVEL_2);
#endif

    if (isDataAllocated()) {
        setDataAllocatedFlag(0);
        uint16_t* l_Temp = data;
        data = 0;
        delete l_Temp;
    }
};


txp::AttrPtr_uint32::~AttrPtr_uint32() {

#ifdef TXP_DEVELOPMENT
    snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_uint32::~AttrPtr_uint32()=%p, isDataAllocated()=%s", this, (isDataAllocated() ? "True" : "False"));
    d_log.write(d_log_buffer, txp::Log::DEBUG, txp::Log::C_DTOR_LOGLEVEL_2);
#endif

    if (isDataAllocated()) {
        setDataAllocatedFlag(0);
        uint32_t* l_Temp = data;
        data = 0;
        delete l_Temp;
    }
};


txp::AttrPtr_uint64::~AttrPtr_uint64() {

#ifdef TXP_DEVELOPMENT
    snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_uint64::~AttrPtr_uint64()=%p, isDataAllocated()=%s", this, (isDataAllocated() ? "True" : "False"));
    d_log.write(d_log_buffer, txp::Log::DEBUG, txp::Log::C_DTOR_LOGLEVEL_2);
#endif

    if (isDataAllocated()) {
        setDataAllocatedFlag(0);
        uint64_t* l_Temp = data;
        data = 0;
        delete l_Temp;
    }
};


txp::AttrPtr_int8::~AttrPtr_int8() {

#ifdef TXP_DEVELOPMENT
    snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_int8::~AttrPtr_int8()=%p, isDataAllocated()=%s", this, (isDataAllocated() ? "True" : "False"));
    d_log.write(d_log_buffer, txp::Log::DEBUG, txp::Log::C_DTOR_LOGLEVEL_2);
#endif

    if (isDataAllocated()) {
        setDataAllocatedFlag(0);
        int8_t* l_Temp = data;
        data = 0;
        delete l_Temp;
    }
};


txp::AttrPtr_int16::~AttrPtr_int16() {

#ifdef TXP_DEVELOPMENT
    snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_int16::~AttrPtr_int16()=%p, isDataAllocated()=%s", this, (isDataAllocated() ? "True" : "False"));
    d_log.write(d_log_buffer, txp::Log::DEBUG, txp::Log::C_DTOR_LOGLEVEL_2);
#endif

    if (isDataAllocated()) {
        setDataAllocatedFlag(0);
        int16_t* l_Temp = data;
        data = 0;
        delete l_Temp;
    }
};


txp::AttrPtr_int32::~AttrPtr_int32() {

#ifdef TXP_DEVELOPMENT
    snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_int32::~AttrPtr_int32()=%p, isDataAllocated()=%s", this, (isDataAllocated() ? "True" : "False"));
    d_log.write(d_log_buffer, txp::Log::DEBUG, txp::Log::C_DTOR_LOGLEVEL_2);
#endif

    if (isDataAllocated()) {
        setDataAllocatedFlag(0);
        int32_t* l_Temp = data;
        data = 0;
        delete l_Temp;
    }
};


txp::AttrPtr_int64::~AttrPtr_int64() {

#ifdef TXP_DEVELOPMENT
    snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_int64::~AttrPtr_int64()=%p, isDataAllocated()=%s", this, (isDataAllocated() ? "True" : "False"));
    d_log.write(d_log_buffer, txp::Log::DEBUG, txp::Log::C_DTOR_LOGLEVEL_2);
#endif

    if (isDataAllocated()) {
        setDataAllocatedFlag(0);
        int64_t* l_Temp = data;
        data = 0;
        delete l_Temp;
    }
};


txp::AttrPtr_char_array::~AttrPtr_char_array() {

#ifdef TXP_DEVELOPMENT
    snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_char_array::~AttrPtr_char_array()=%p, isDataAllocated()=%s", this, (isDataAllocated() ? "True" : "False"));
    d_log.write(d_log_buffer, txp::Log::DEBUG, txp::Log::C_DTOR_LOGLEVEL_2);
#endif

    if (isDataAllocated()) {
        setDataAllocatedFlag(0);
        char* l_Temp = data;
        data = 0;
        delete[] l_Temp;
    }
};


txp::AttrPtr_array_of_char_arrays::~AttrPtr_array_of_char_arrays() {

#ifdef TXP_DEVELOPMENT
    snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_array_of_char_arrays::~AttrPtr_array_of_char_arrays()=%p, isDataAllocated()=%s, areDataElementCharArraysAllocated()=%s", this, (isDataAllocated() ? "True" : "False"), (areDataElementCharArraysAllocated() ? "True" : "False"));
    d_log.write(d_log_buffer, txp::Log::DEBUG, txp::Log::C_DTOR_LOGLEVEL_2);
#endif

    int l_DataElementCharArraysAllocated = areDataElementCharArraysAllocated();
    setDataElementCharArraysAllocatedFlag(0);
    for (CharArrayIter it = data->begin(); it != data->end(); ++it) {
    	if (l_DataElementCharArraysAllocated) {
    		char* l_Temp = it->second;
    		it->second = 0;
    		delete[] l_Temp;
    	}
    }

    if (isDataAllocated()) {
        setDataAllocatedFlag(0);
        CharArray* l_Temp = data;
        data = 0;
        l_Temp->clear();
        delete l_Temp;
    }

};


//*****************************************************************************
//  Add an attribute to buffer routines
//*****************************************************************************
int txp::Attribute::add(HeapBuffer* pBuffer, unsigned long* pCRC) {
    int l_RC = 0;

    uint16_t l_AttrType = (uint16_t)attrType;
    l_RC = pBuffer->add((void*)&(l_AttrType), LENGTH_OF_ATTR_TYPE);
    if (!l_RC) {
    	uint16_t l_AttrName = (uint16_t)attrName;
        l_RC = pBuffer->add((void*)&l_AttrName, LENGTH_OF_ATTR_NAME);
        if (l_RC) {
            l_RC = -402;
        }
    } else {
        l_RC = -401;
    }

#ifdef TXP_DEVELOPMENT
    if (l_RC) {
        snprintf(d_log_buffer, sizeof(d_log_buffer), "Attribute::add(pBuffer=%p): RC=%d, attrType=%d, attrName=%d", pBuffer, l_RC, attrType, attrName);
        d_log.write(d_log_buffer, txp::Log::ERROR);
    }
#endif

    return l_RC;
}


int txp::Attr_char::add(HeapBuffer* pBuffer, unsigned long* pCRC) {
    int l_RC = 0;

    ADD_DATA(Attr_Char, pCRC);

#ifdef TXP_DEVELOPMENT
    if (l_RC) {
        snprintf(d_log_buffer, sizeof(d_log_buffer), "Attr_char::add(pBuffer=%p): RC=%d", pBuffer, l_RC);
        d_log.write(d_log_buffer, txp::Log::ERROR);
    }
#endif

    return l_RC;
}


int txp::Attr_uint8::add(HeapBuffer* pBuffer, unsigned long* pCRC) {
    int l_RC = 0;

    ADD_DATA(Attr_uint8, pCRC);

#ifdef TXP_DEVELOPMENT
    if (l_RC) {
        snprintf(d_log_buffer, sizeof(d_log_buffer), "Attr_uint8::add(pBuffer=%p): RC=%d", pBuffer, l_RC);
        d_log.write(d_log_buffer, txp::Log::ERROR);
    }
#endif

    return l_RC;
}


int txp::Attr_uint16::add(HeapBuffer* pBuffer, unsigned long* pCRC) {
    int l_RC = 0;

    ADD_DATA(Attr_uint16, pCRC);

#ifdef TXP_DEVELOPMENT
    if (l_RC) {
        snprintf(d_log_buffer, sizeof(d_log_buffer), "Attr_uint16::add(pBuffer=%p): RC=%d", pBuffer, l_RC);
        d_log.write(d_log_buffer, txp::Log::ERROR);
    }
#endif

    return l_RC;
}


int txp::Attr_uint32::add(HeapBuffer* pBuffer, unsigned long* pCRC) {
    int l_RC = 0;

    ADD_DATA(Attr_uint32, pCRC);

#ifdef TXP_DEVELOPMENT
    if (l_RC) {
        snprintf(d_log_buffer, sizeof(d_log_buffer), "Attr_uint32::add(pBuffer=%p): RC=%d", pBuffer, l_RC);
        d_log.write(d_log_buffer, txp::Log::ERROR);
    }
#endif

    return l_RC;
}


int txp::Attr_uint64::add(HeapBuffer* pBuffer, unsigned long* pCRC) {
    int l_RC = 0;

    ADD_DATA(Attr_uint64, pCRC);

#ifdef TXP_DEVELOPMENT
    if (l_RC) {
        snprintf(d_log_buffer, sizeof(d_log_buffer), "Attr_uint64::add(pBuffer=%p): RC=%d", pBuffer, l_RC);
        d_log.write(d_log_buffer, txp::Log::ERROR);
    }
#endif

    return l_RC;
}


int txp::Attr_int8::add(HeapBuffer* pBuffer, unsigned long* pCRC) {
    int l_RC = 0;

    ADD_DATA(Attr_int8, pCRC);

#ifdef TXP_DEVELOPMENT
    if (l_RC) {
        snprintf(d_log_buffer, sizeof(d_log_buffer), "Attr_int8::add(pBuffer=%p): RC=%d", pBuffer, l_RC);
        d_log.write(d_log_buffer, txp::Log::ERROR);
    }
#endif

    return l_RC;
}


int txp::Attr_int16::add(HeapBuffer* pBuffer, unsigned long* pCRC) {
    int l_RC = 0;

    ADD_DATA(Attr_int16, pCRC);

#ifdef TXP_DEVELOPMENT
    if (l_RC) {
        snprintf(d_log_buffer, sizeof(d_log_buffer), "Attr_int16::add(pBuffer=%p): RC=%d", pBuffer, l_RC);
        d_log.write(d_log_buffer, txp::Log::ERROR);
    }
#endif

    return l_RC;
}


int txp::Attr_int32::add(HeapBuffer* pBuffer, unsigned long* pCRC) {
    int l_RC = 0;

    ADD_DATA(Attr_int32, pCRC);

#ifdef TXP_DEVELOPMENT
    if (l_RC) {
        snprintf(d_log_buffer, sizeof(d_log_buffer), "Attr_int32::add(pBuffer=%p): RC=%d", pBuffer, l_RC);
        d_log.write(d_log_buffer, txp::Log::ERROR);
    }
#endif

    return l_RC;
}


int txp::Attr_int64::add(HeapBuffer* pBuffer, unsigned long* pCRC) {
    int l_RC = 0;

    ADD_DATA(Attr_int64, pCRC);

#ifdef TXP_DEVELOPMENT
    if (l_RC) {
        snprintf(d_log_buffer, sizeof(d_log_buffer), "Attr_int64::add(pBuffer=%p): RC=%d", pBuffer, l_RC);
        d_log.write(d_log_buffer, txp::Log::ERROR);
    }
#endif

    return l_RC;
}


int txp::AttrPtr_char::add(HeapBuffer* pBuffer, unsigned long* pCRC) {
    int l_RC = 0;

    ADD_DATA(AttrPtr_char, pCRC);

#ifdef TXP_DEVELOPMENT
    if (l_RC) {
        snprintf(d_log_buffer, sizeof(d_log_buffer), "Attr_Ptr_char::add(pBuffer=%p): RC=%d", pBuffer, l_RC);
        d_log.write(d_log_buffer, txp::Log::ERROR);
    }
#endif

    return l_RC;
}


int txp::AttrPtr_uint8::add(HeapBuffer* pBuffer, unsigned long* pCRC) {
    int l_RC = 0;

    ADD_DATA(AttrPtr_uint8, pCRC);

#ifdef TXP_DEVELOPMENT
    if (l_RC) {
        snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_uint8::add(pBuffer=%p): RC=%d", pBuffer, l_RC);
        d_log.write(d_log_buffer, txp::Log::ERROR);
    }
#endif

    return l_RC;
}


int txp::AttrPtr_uint16::add(HeapBuffer* pBuffer, unsigned long* pCRC) {
    int l_RC = 0;

    ADD_DATA(AttrPtr_uint16, pCRC);

#ifdef TXP_DEVELOPMENT
    if (l_RC) {
        snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_uint16::add(pBuffer=%p): RC=%d", pBuffer, l_RC);
        d_log.write(d_log_buffer, txp::Log::ERROR);
    }
#endif

    return l_RC;
}


int txp::AttrPtr_uint32::add(HeapBuffer* pBuffer, unsigned long* pCRC) {
    int l_RC = 0;

    ADD_DATA(AttrPtr_uint32, pCRC);

#ifdef TXP_DEVELOPMENT
    if (l_RC) {
        snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_uint32::add(pBuffer=%p): RC=%d", pBuffer, l_RC);
        d_log.write(d_log_buffer, txp::Log::ERROR);
    }
#endif

    return l_RC;
}


int txp::AttrPtr_uint64::add(HeapBuffer* pBuffer, unsigned long* pCRC) {
    int l_RC = 0;

    ADD_DATA(AttrPtr_uint64, pCRC);

#ifdef TXP_DEVELOPMENT
    if (l_RC) {
        snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_uint64::add(pBuffer=%p): RC=%d", pBuffer, l_RC);
        d_log.write(d_log_buffer, txp::Log::ERROR);
    }
#endif

    return l_RC;
}


int txp::AttrPtr_int8::add(HeapBuffer* pBuffer, unsigned long* pCRC) {
    int l_RC = 0;

    ADD_DATA(AttrPtr_int8, pCRC);

#ifdef TXP_DEVELOPMENT
    if (l_RC) {
        snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_int8::add(pBuffer=%p): RC=%d", pBuffer, l_RC);
        d_log.write(d_log_buffer, txp::Log::ERROR);
    }
#endif

    return l_RC;
}


int txp::AttrPtr_int16::add(HeapBuffer* pBuffer, unsigned long* pCRC) {
    int l_RC = 0;

    ADD_DATA(AttrPtr_int16, pCRC);

#ifdef TXP_DEVELOPMENT
    if (l_RC) {
        snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_int16::add(pBuffer=%p): RC=%d", pBuffer, l_RC);
        d_log.write(d_log_buffer, txp::Log::ERROR);
    }
#endif

    return l_RC;
}


int txp::AttrPtr_int32::add(HeapBuffer* pBuffer, unsigned long* pCRC) {
    int l_RC = 0;

    ADD_DATA(AttrPtr_int32, pCRC);

#ifdef TXP_DEVELOPMENT
    if (l_RC) {
        snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_int32::add(pBuffer=%p): RC=%d", pBuffer, l_RC);
        d_log.write(d_log_buffer, txp::Log::ERROR);
    }
#endif

    return l_RC;
}


int txp::AttrPtr_int64::add(HeapBuffer* pBuffer, unsigned long* pCRC) {
    int l_RC = 0;

    ADD_DATA(AttrPtr_int64, pCRC);

#ifdef TXP_DEVELOPMENT
    if (l_RC) {
        snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_int64::add(pBuffer=%p): RC=%d", pBuffer, l_RC);
        d_log.write(d_log_buffer, txp::Log::ERROR);
    }
#endif

    return l_RC;
}


int txp::AttrPtr_char_array::add(HeapBuffer* pBuffer, unsigned long* pCRC) {
    int l_RC = 0;

//	printf("AttrPtr_char_array::add: Before: pCRC=%p, *pCRC=%lu\n", pCRC, *pCRC);
    l_RC = Attribute::add(pBuffer, pCRC);
    if (!l_RC) {
        l_RC = pBuffer->add((void*)&(length), LENGTH_OF_ATTR_CHAR_ARRAY_LENGTH);
        if (!l_RC) {
        	if (pCRC) {
            	size_t l_Length = LENGTH_OF_ATTR_TYPE + LENGTH_OF_ATTR_NAME + LENGTH_OF_ATTR_CHAR_ARRAY_LENGTH;
            	pBuffer->calculateCRC_Value(*pCRC, pBuffer->offset - l_Length, l_Length);
//				printf("AttrPtr_char_array::add: After common: offset=%d, l_Length=%d, pCRC=%p, *pCRC=0x%016lX\n", pBuffer->offset - l_Length, l_Length, pCRC, *pCRC);
        	}
            l_RC = pBuffer->add((void*)&(data), sizeof(data));
        	if (!l_RC) {
            	if (pCRC) {
            		pBuffer->calculateCRC_Value(*pCRC, pBuffer->offset - sizeof(data), sizeof(data));
//					printf("AttrPtr_char_array::add: After data: offset=%d, l_Length=%d, pCRC=%p, *pCRC=0x%016lX\n", pBuffer->offset - sizeof(data), sizeof(data), pCRC, *pCRC);
            	}
            } else {
                l_RC = -408;
            }
        } else {
            l_RC = -407;
        }
    }

#ifdef TXP_DEVELOPMENT
    if (l_RC) {
        snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_char_array::add(pBuffer=%p): RC=%d, getAttrType()=%d, getAttrName()=%d, length=%zu, data=%p", pBuffer, l_RC, getAttrType(), getAttrName(), length, &data);
        d_log.write(d_log_buffer, txp::Log::ERROR);
    }
#endif

    return l_RC;
}


int txp::AttrPtr_array_of_char_arrays::add(HeapBuffer* pBuffer, unsigned long* pCRC) {
    int l_RC = 0;

//	printf("AttrPtr_array_of_char_arrays::add: Before: pCRC=%p, *pCRC=%lu\n", pCRC, *pCRC);
    l_RC = Attribute::add(pBuffer, pCRC);
    if (!l_RC) {
        l_RC = pBuffer->add((void*)&(number_of_elements), LENGTH_OF_ATTR_ARRAY_OF_CHAR_ARRAYS_NUMBER_OF_ELEMENTS);
    	if (!l_RC) {
        	l_RC = pBuffer->add((void*)&(length_of_data), LENGTH_OF_ATTR_ARRAY_OF_CHAR_ARRAYS_LENGTH);
        	if (!l_RC) {
        		if (pCRC) {
        	    	size_t l_Length = LENGTH_OF_ATTR_TYPE + LENGTH_OF_ATTR_NAME + LENGTH_OF_ATTR_ARRAY_OF_CHAR_ARRAYS_NUMBER_OF_ELEMENTS + LENGTH_OF_ATTR_ARRAY_OF_CHAR_ARRAYS_LENGTH;
        	    	pBuffer->calculateCRC_Value(*pCRC, pBuffer->offset - l_Length, l_Length);
//					printf("AttrPtr_char_array::add: After common: offset=%zu, l_Length=%zu, pCRC=%p, *pCRC=0x%016lX\n", pBuffer->offset - l_Length, l_Length, pCRC, *pCRC);
        		}
        		for (CharArrayIter it = data->begin(); it != data->end(); ++it) {
        	    	l_RC = pBuffer->add((void*)&(it->first), LENGTH_OF_ATTR_ARRAY_OF_CHAR_ARRAYS_ELEMENT_SIZE_LENGTH);
        			if (!l_RC) {
        	    		if (pCRC) {
        	    			pBuffer->calculateCRC_Value(*pCRC, pBuffer->offset - LENGTH_OF_ATTR_ARRAY_OF_CHAR_ARRAYS_ELEMENT_SIZE_LENGTH, LENGTH_OF_ATTR_ARRAY_OF_CHAR_ARRAYS_ELEMENT_SIZE_LENGTH);
//							printf("AttrPtr_char_array::add: After data: offset=%zu, l_Length=%zu, pCRC=%p, *pCRC=0x%016lX\n", pBuffer->offset - LENGTH_OF_ATTR_ARRAY_OF_CHAR_ARRAYS_ELEMENT_SIZE_LENGTH, LENGTH_OF_ATTR_ARRAY_OF_CHAR_ARRAYS_ELEMENT_SIZE_LENGTH, pCRC, *pCRC);
        	    		}
        	    		l_RC = pBuffer->add((void*)(it->second), sizeof(it->second));
        				if (!l_RC) {
        	    			if (pCRC) {
        	    				pBuffer->calculateCRC_Value(*pCRC, pBuffer->offset - sizeof(it->second), sizeof(it->second));
//								printf("AttrPtr_char_array::add: After data: offset=%zu, l_Length=%zu, pCRC=%p, *pCRC=0x%016lX\n", pBuffer->offset - sizeof(it->second), sizeof(it->second), pCRC, *pCRC);
        	    			}
        	    		} else {
        	        		l_RC = -414;
        	        	}
        	    	} else {
        	        	l_RC = -413;
        	        }
        		}
        	} else {
        	    l_RC = -412;
        	}
        } else {
            l_RC = -411;
        }
    }

#ifdef TXP_DEVELOPMENT
    if (l_RC) {
        snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_array_of_char_arrays::add(pBuffer=%p): RC=%d, getAttrType()=%d, getAttrName()=%d, number_of_elements=%zu, length_of_data=%zu, length_of_values=%zu, data=%p", pBuffer, l_RC, getAttrType(), getAttrName(), number_of_elements, length_of_data, length_of_values, &data);
        d_log.write(d_log_buffer, txp::Log::ERROR);
    }
#endif

    return l_RC;
}


//*****************************************************************************
//  Add 'the value' of an attribute to buffer routines.
//  NOTE:  For the non-'_ptr_' derived classes, this
//         is the same as add() of the attribute to the
//         buffer.
//*****************************************************************************
int txp::Attribute::addValue(HeapBuffer* pBuffer, unsigned long* pCRC) {
    int l_RC = 0;

    uint16_t l_AttrType = (uint16_t)ConvertToValueAttrType(attrType);
    l_RC = pBuffer->add((void*)&(l_AttrType), LENGTH_OF_ATTR_TYPE);
    if (!l_RC) {
    	uint16_t l_AttrName = (uint16_t)attrName;
        l_RC = pBuffer->add((void*)&l_AttrName, LENGTH_OF_ATTR_NAME);
        if (l_RC) {
            l_RC = -404;
        }
    } else {
        l_RC = -403;
    }

#ifdef TXP_DEVELOPMENT
    if (l_RC) {
        snprintf(d_log_buffer, sizeof(d_log_buffer), "Attribute::addValue(pBuffer=%p): RC=%d, attrType=%d, attrName=%d", pBuffer, l_RC, attrType, attrName);
        d_log.write(d_log_buffer, txp::Log::ERROR);
    }
#endif

    return l_RC;
}


int txp::Attr_char::addValue(HeapBuffer* pBuffer, unsigned long* pCRC) {
    return add(pBuffer, pCRC);
}


int txp::Attr_uint8::addValue(HeapBuffer* pBuffer, unsigned long* pCRC) {
    return add(pBuffer, pCRC);
}


int txp::Attr_uint16::addValue(HeapBuffer* pBuffer, unsigned long* pCRC) {
    return add(pBuffer, pCRC);
}


int txp::Attr_uint32::addValue(HeapBuffer* pBuffer, unsigned long* pCRC) {
    return add(pBuffer, pCRC);
}


int txp::Attr_uint64::addValue(HeapBuffer* pBuffer, unsigned long* pCRC) {
    return add(pBuffer, pCRC);
}


int txp::Attr_int8::addValue(HeapBuffer* pBuffer, unsigned long* pCRC) {
    return add(pBuffer, pCRC);
}


int txp::Attr_int16::addValue(HeapBuffer* pBuffer, unsigned long* pCRC) {
    return add(pBuffer, pCRC);
}


int txp::Attr_int32::addValue(HeapBuffer* pBuffer, unsigned long* pCRC) {
    return add(pBuffer, pCRC);
}


int txp::Attr_int64::addValue(HeapBuffer* pBuffer, unsigned long* pCRC) {
    return add(pBuffer, pCRC);
}


int txp::AttrPtr_char::addValue(HeapBuffer* pBuffer, unsigned long* pCRC) {
    int l_RC = 0;

    ADD_DATA_VALUE_WITH_PTR(AttrPtr_char, pCRC);

#ifdef TXP_DEVELOPMENT
    if (l_RC) {
        snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_char::addValue(pBuffer=%p): RC=%d", pBuffer, l_RC);
        d_log.write(d_log_buffer, txp::Log::ERROR);
    }
#endif

    return l_RC;
}


int txp::AttrPtr_uint8::addValue(HeapBuffer* pBuffer, unsigned long* pCRC) {
    int l_RC = 0;

    ADD_DATA_VALUE_WITH_PTR(AttrPtr_uint8, pCRC);

#ifdef TXP_DEVELOPMENT
    if (l_RC) {
        snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_uint8::addValue(pBuffer=%p): RC=%d", pBuffer, l_RC);
        d_log.write(d_log_buffer, txp::Log::ERROR);
    }
#endif

    return l_RC;
}


int txp::AttrPtr_uint16::addValue(HeapBuffer* pBuffer, unsigned long* pCRC) {
    int l_RC = 0;

    ADD_DATA_VALUE_WITH_PTR(AttrPtr_uint16, pCRC);

#ifdef TXP_DEVELOPMENT
    if (l_RC) {
        snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_uint16::addValue(pBuffer=%p): RC=%d", pBuffer, l_RC);
        d_log.write(d_log_buffer, txp::Log::ERROR);
    }
#endif

    return l_RC;
}


int txp::AttrPtr_uint32::addValue(HeapBuffer* pBuffer, unsigned long* pCRC) {
    int l_RC = 0;

    ADD_DATA_VALUE_WITH_PTR(AttrPtr_uint32, pCRC);

#ifdef TXP_DEVELOPMENT
    if (l_RC) {
        snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_uint32::addValue(pBuffer=%p): RC=%d", pBuffer, l_RC);
        d_log.write(d_log_buffer, txp::Log::ERROR);
    }
#endif

    return l_RC;
}


int txp::AttrPtr_uint64::addValue(HeapBuffer* pBuffer, unsigned long* pCRC) {
    int l_RC = 0;

    ADD_DATA_VALUE_WITH_PTR(AttrPtr_uint64, pCRC);

#ifdef TXP_DEVELOPMENT
    if (l_RC) {
        snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_int64::addValue(pBuffer=%p): RC=%d", pBuffer, l_RC);
        d_log.write(d_log_buffer, txp::Log::ERROR);
    }
#endif

    return l_RC;
}


int txp::AttrPtr_int8::addValue(HeapBuffer* pBuffer, unsigned long* pCRC) {
    int l_RC = 0;

    ADD_DATA_VALUE_WITH_PTR(AttrPtr_int8, pCRC);

#ifdef TXP_DEVELOPMENT
    if (l_RC) {
        snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_int8::addValue(pBuffer=%p): RC=%d", pBuffer, l_RC);
        d_log.write(d_log_buffer, txp::Log::ERROR);
    }
#endif

    return l_RC;
}


int txp::AttrPtr_int16::addValue(HeapBuffer* pBuffer, unsigned long* pCRC) {
    int l_RC = 0;

    ADD_DATA_VALUE_WITH_PTR(AttrPtr_int16, pCRC);

#ifdef TXP_DEVELOPMENT
    if (l_RC) {
        snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_int16::addValue(pBuffer=%p): RC=%d", pBuffer, l_RC);
        d_log.write(d_log_buffer, txp::Log::ERROR);
    }
#endif

    return l_RC;
}


int txp::AttrPtr_int32::addValue(HeapBuffer* pBuffer, unsigned long* pCRC) {
    int l_RC = 0;

    ADD_DATA_VALUE_WITH_PTR(AttrPtr_int32, pCRC);

#ifdef TXP_DEVELOPMENT
    if (l_RC) {
        snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_int32::addValue(pBuffer=%p): RC=%d", pBuffer, l_RC);
        d_log.write(d_log_buffer, txp::Log::ERROR);
    }
#endif

    return l_RC;
}


int txp::AttrPtr_int64::addValue(HeapBuffer* pBuffer, unsigned long* pCRC) {
    int l_RC = 0;

    ADD_DATA_VALUE_WITH_PTR(AttrPtr_int64, pCRC);

#ifdef TXP_DEVELOPMENT
    if (l_RC) {
        snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_int64::addValue(pBuffer=%p): RC=%d", pBuffer, l_RC);
        d_log.write(d_log_buffer, txp::Log::ERROR);
    }
#endif

    return l_RC;
}


int txp::AttrPtr_char_array::addValue(HeapBuffer* pBuffer, unsigned long* pCRC) {
    int l_RC = 0;

    l_RC = Attribute::addValue(pBuffer, pCRC);
    if (!l_RC) {
        l_RC = pBuffer->add((void*)&(length), LENGTH_OF_ATTR_CHAR_ARRAY_LENGTH);
        if (!l_RC) {
	        if (pCRC) {
            	size_t l_Length = LENGTH_OF_ATTR_TYPE + LENGTH_OF_ATTR_NAME + LENGTH_OF_ATTR_CHAR_ARRAY_LENGTH;
            	pBuffer->calculateCRC_Value(*pCRC, pBuffer->offset - l_Length, l_Length);
//				printf("AttrPtr_char_array::addValue: After common: offset=%zu, l_Length=%zu, pCRC=%p, *pCRC=0x%016lX\n", pBuffer->offset - l_Length, l_Length, pCRC, *pCRC);
        	}
            l_RC = pBuffer->add((void*)&(*data), length);
        	if (!l_RC) {
            	if (pCRC) {
            		pBuffer->calculateCRC_Value(*pCRC, pBuffer->offset - length, length);
//					printf("AttrPtr_char_array::addValue: After data: offset=%zu, l_Length=%zu, pCRC=%p, *pCRC=0x%016lX\n", pBuffer->offset - length, length, pCRC, *pCRC);
            	}
            } else {
                l_RC = -410;
            }
        } else {
            l_RC = -409;
        }
    }

#ifdef TXP_DEVELOPMENT
    if (l_RC) {
        snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_char_array::addValue(pBuffer=%p): RC=%d, getAttrType()=%d, getAttrName()=%d, length=%zu, data=%p", pBuffer, l_RC, getAttrType(), getAttrName(), length, data);
        d_log.write(d_log_buffer, txp::Log::ERROR);
    }
#endif

    return l_RC;
}


int txp::AttrPtr_array_of_char_arrays::addValue(HeapBuffer* pBuffer, unsigned long* pCRC) {
    int l_RC = 0;

    l_RC = Attribute::addValue(pBuffer, pCRC);
    if (!l_RC) {
        l_RC = pBuffer->add((void*)&(number_of_elements), LENGTH_OF_ATTR_ARRAY_OF_CHAR_ARRAYS_NUMBER_OF_ELEMENTS);
    	if (!l_RC) {
        	l_RC = pBuffer->add((void*)&(length_of_values), LENGTH_OF_ATTR_ARRAY_OF_CHAR_ARRAYS_LENGTH);
        	if (!l_RC) {
        		if (pCRC) {
        	    	size_t l_Length = LENGTH_OF_ATTR_TYPE + LENGTH_OF_ATTR_NAME + LENGTH_OF_ATTR_ARRAY_OF_CHAR_ARRAYS_NUMBER_OF_ELEMENTS + LENGTH_OF_ATTR_ARRAY_OF_CHAR_ARRAYS_LENGTH;
        	    	pBuffer->calculateCRC_Value(*pCRC, pBuffer->offset - l_Length, l_Length);
//					printf("AttrPtr_char_array::add: After common: offset=%zu, l_Length=%zu, pCRC=%p, *pCRC=0x%016lX\n", pBuffer->offset - l_Length, l_Length, pCRC, *pCRC);
        		}
    			for (CharArrayIter it = data->begin(); it != data->end(); ++it) {
        	    	l_RC = pBuffer->add((void*)&(it->first), LENGTH_OF_ATTR_ARRAY_OF_CHAR_ARRAYS_ELEMENT_SIZE_LENGTH);
        			if (!l_RC) {
        	    		if (pCRC) {
        	    			pBuffer->calculateCRC_Value(*pCRC, pBuffer->offset - LENGTH_OF_ATTR_ARRAY_OF_CHAR_ARRAYS_ELEMENT_SIZE_LENGTH, LENGTH_OF_ATTR_ARRAY_OF_CHAR_ARRAYS_ELEMENT_SIZE_LENGTH);
//							printf("AttrPtr_char_array::add: After data: offset=%zu, l_Length=%zu, pCRC=%p, *pCRC=0x%016lX\n", pBuffer->offset - LENGTH_OF_ATTR_ARRAY_OF_CHAR_ARRAYS_ELEMENT_SIZE_LENGTH, LENGTH_OF_ATTR_ARRAY_OF_CHAR_ARRAYS_ELEMENT_SIZE_LENGTH, pCRC, *pCRC);
        	    		}
        	    		l_RC = pBuffer->add((void*)(it->second), it->first);
        				if (!l_RC) {
        	    			if (pCRC) {
        	    				pBuffer->calculateCRC_Value(*pCRC, pBuffer->offset - it->first, it->first);
//								printf("AttrPtr_char_array::add: After data: offset=%zu, l_Length=%zu, pCRC=%p, *pCRC=0x%016lX\n", pBuffer->offset - it->first, it->first, pCRC, *pCRC);
        	    			}
        	    		} else {
        	        		l_RC = -418;
        	        	}
        	    	} else {
        	        	l_RC = -417;
        	        }
        		}
        	} else {
        	    l_RC = -416;
        	}
        } else {
            l_RC = -415;
        }
    }

#ifdef TXP_DEVELOPMENT
    if (l_RC) {
        snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_array_of_char_arrays::addValue(pBuffer=%p): RC=%d, getAttrType()=%d, getAttrName()=%d, number_of_elements=%zu, length_of_data=%zu, length_of_values=%zu, data=%p", pBuffer, l_RC, getAttrType(), getAttrName(), number_of_elements, length_of_data, length_of_values, data);
        d_log.write(d_log_buffer, txp::Log::ERROR);
    }
#endif

    return l_RC;
}


//*****************************************************************************
//  Clone routines for attributes
//*****************************************************************************
int txp::Attr_char::clone(Attribute* &pTargetAttr) {
	int l_RC = 0;

	if (!pTargetAttr) {
		Attr_char* l_Attr = new Attr_char(*this);
		l_Attr->setAllocatedFlag(1);
		pTargetAttr = l_Attr;
	} else {
		l_RC = -424;
	}

	return l_RC;
}


int txp::Attr_uint8::clone(Attribute* &pTargetAttr) {
	int l_RC = 0;

	if (!pTargetAttr) {
		Attr_uint8* l_Attr = new Attr_uint8(*this);
		l_Attr->setAllocatedFlag(1);
		pTargetAttr = l_Attr;
	} else {
		l_RC = -425;
	}

	return l_RC;
}


int txp::Attr_uint16::clone(Attribute* &pTargetAttr) {
	int l_RC = 0;

	if (!pTargetAttr) {
		Attr_uint16* l_Attr = new Attr_uint16(*this);
		l_Attr->setAllocatedFlag(1);
		pTargetAttr = l_Attr;
	} else {
		l_RC = -426;
	}

	return l_RC;
}


int txp::Attr_uint32::clone(Attribute* &pTargetAttr) {
	int l_RC = 0;

	if (!pTargetAttr) {
		Attr_uint32* l_Attr = new Attr_uint32(*this);
		l_Attr->setAllocatedFlag(1);
		pTargetAttr = l_Attr;
	} else {
		l_RC = -427;
	}

	return l_RC;
}


int txp::Attr_uint64::clone(Attribute* &pTargetAttr) {
	int l_RC = 0;

	if (!pTargetAttr) {
		Attr_uint64* l_Attr = new Attr_uint64(*this);
		l_Attr->setAllocatedFlag(1);
		pTargetAttr = l_Attr;
	} else {
		l_RC = -428;
	}

	return l_RC;
}


int txp::Attr_int8::clone(Attribute* &pTargetAttr) {
	int l_RC = 0;

	if (!pTargetAttr) {
		Attr_int8* l_Attr = new Attr_int8(*this);
		l_Attr->setAllocatedFlag(1);
		pTargetAttr = l_Attr;
	} else {
		l_RC = -429;
	}

	return l_RC;
}


int txp::Attr_int16::clone(Attribute* &pTargetAttr) {
	int l_RC = 0;

	if (!pTargetAttr) {
		Attr_int16* l_Attr = new Attr_int16(*this);
		l_Attr->setAllocatedFlag(1);
		pTargetAttr = l_Attr;
	} else {
		l_RC = -430;
	}

	return l_RC;
}


int txp::Attr_int32::clone(Attribute* &pTargetAttr) {
	int l_RC = 0;

	if (!pTargetAttr) {
		Attr_int32* l_Attr = new Attr_int32(*this);
		l_Attr->setAllocatedFlag(1);
		pTargetAttr = l_Attr;
	} else {
		l_RC = -431;
	}

	return l_RC;
}


int txp::Attr_int64::clone(Attribute* &pTargetAttr) {
	int l_RC = 0;

	if (!pTargetAttr) {
		Attr_int64* l_Attr = new Attr_int64(*this);
		l_Attr->setAllocatedFlag(1);
		pTargetAttr = l_Attr;
	} else {
		l_RC = -432;
	}

	return l_RC;
}


int txp::AttrPtr_char::clone(Attribute* &pTargetAttr) {
	int l_RC = 0;

	if (!pTargetAttr) {
		AttrPtr_char* l_Attr = new AttrPtr_char(*this);
		l_Attr->setAllocatedFlag(1);
		pTargetAttr = l_Attr;
	} else {
		l_RC = -433;
	}

	return l_RC;
}


int txp::AttrPtr_uint8::clone(Attribute* &pTargetAttr) {
	int l_RC = 0;

	if (!pTargetAttr) {
		AttrPtr_uint8* l_Attr = new AttrPtr_uint8(*this);
		l_Attr->setAllocatedFlag(1);
		pTargetAttr = l_Attr;
	} else {
		l_RC = -434;
	}

	return l_RC;
}


int txp::AttrPtr_uint16::clone(Attribute* &pTargetAttr) {
	int l_RC = 0;

	if (!pTargetAttr) {
		AttrPtr_uint16* l_Attr = new AttrPtr_uint16(*this);
		l_Attr->setAllocatedFlag(1);
		pTargetAttr = l_Attr;
	} else {
		l_RC = -435;
	}

	return l_RC;
}


int txp::AttrPtr_uint32::clone(Attribute* &pTargetAttr) {
	int l_RC = 0;

	if (!pTargetAttr) {
		AttrPtr_uint32* l_Attr = new AttrPtr_uint32(*this);
		l_Attr->setAllocatedFlag(1);
		pTargetAttr = l_Attr;
	} else {
		l_RC = -436;
	}

	return l_RC;
}


int txp::AttrPtr_uint64::clone(Attribute* &pTargetAttr) {
	int l_RC = 0;

	if (!pTargetAttr) {
		AttrPtr_uint64* l_Attr = new AttrPtr_uint64(*this);
		l_Attr->setAllocatedFlag(1);
		pTargetAttr = l_Attr;
	} else {
		l_RC = -437;
	}

	return l_RC;
}


int txp::AttrPtr_int8::clone(Attribute* &pTargetAttr) {
	int l_RC = 0;

	if (!pTargetAttr) {
		AttrPtr_int8* l_Attr = new AttrPtr_int8(*this);
		l_Attr->setAllocatedFlag(1);
		pTargetAttr = l_Attr;
	} else {
		l_RC = -438;
	}

	return l_RC;
}


int txp::AttrPtr_int16::clone(Attribute* &pTargetAttr) {
	int l_RC = 0;

	if (!pTargetAttr) {
		AttrPtr_int16* l_Attr = new AttrPtr_int16(*this);
		l_Attr->setAllocatedFlag(1);
		pTargetAttr = l_Attr;
	} else {
		l_RC = -439;
	}

	return l_RC;
}


int txp::AttrPtr_int32::clone(Attribute* &pTargetAttr) {
	int l_RC = 0;

	if (!pTargetAttr) {
		AttrPtr_int32* l_Attr = new AttrPtr_int32(*this);
		l_Attr->setAllocatedFlag(1);
		pTargetAttr = l_Attr;
	} else {
		l_RC = -440;
	}

	return l_RC;
}


int txp::AttrPtr_int64::clone(Attribute* &pTargetAttr) {
	int l_RC = 0;

	if (!pTargetAttr) {
		AttrPtr_int64* l_Attr = new AttrPtr_int64(*this);
		l_Attr->setAllocatedFlag(1);
		pTargetAttr = l_Attr;
	} else {
		l_RC = -441;
	}

	return l_RC;
}


int txp::AttrPtr_char_array::clone(Attribute* &pTargetAttr) {
	int l_RC = 0;

	if (!pTargetAttr) {
		AttrPtr_char_array* l_Attr = new AttrPtr_char_array(*this);
		l_Attr->setAllocatedFlag(1);
		pTargetAttr = l_Attr;
	} else {
		l_RC = -442;
	}

	return l_RC;
}


int txp::AttrPtr_array_of_char_arrays::clone(Attribute* &pTargetAttr) {
	int l_RC = 0;

	if (!pTargetAttr) {
		AttrPtr_array_of_char_arrays* l_Attr = new AttrPtr_array_of_char_arrays(*this);
		l_Attr->setAllocatedFlag(1);
		pTargetAttr = l_Attr;
	} else {
		l_RC = -443;
	}

	return l_RC;
}


//*****************************************************************************
//  Copy data routines for attributes
//*****************************************************************************
int txp::Attr_char::cpyData(void* pTarget, size_t pLength) {
    int l_RC = 0;

    CPY_DATA(Attr_char);

#ifdef TXP_DEVELOPMENT
    if (l_RC) {
        snprintf(d_log_buffer, sizeof(d_log_buffer), "Attr_char::cpyData::cpyData(pTarget=%p, pLength=%zu): RC=%d, getAttrType()=%d, getAttrName()=%d, data=%c", pTarget, pLength, l_RC, getAttrType(), getAttrName(), data);
        d_log.write(d_log_buffer, txp::Log::ERROR);
    }
#endif

    return l_RC;
};


int txp::Attr_uint8::cpyData(void* pTarget, size_t pLength) {
    int l_RC = 0;

    CPY_DATA(Attr_uint8);

#ifdef TXP_DEVELOPMENT
    if (l_RC) {
        snprintf(d_log_buffer, sizeof(d_log_buffer), "Attr_uint8::cpyData::cpyData(pTarget=%p, pLength=%zu): RC=%d, getAttrType()=%d, getAttrName()=%d, data=%hhu", pTarget, pLength, l_RC, getAttrType(), getAttrName(), data);
        d_log.write(d_log_buffer, txp::Log::ERROR);
    }
#endif

    return l_RC;
};


int txp::Attr_uint16::cpyData(void* pTarget, size_t pLength) {
    int l_RC = 0;

    CPY_DATA(Attr_uint16);

#ifdef TXP_DEVELOPMENT
    if (l_RC) {
        snprintf(d_log_buffer, sizeof(d_log_buffer), "Attr_uint16::cpyData::cpyData(pTarget=%p, pLength=%zu): RC=%d, getAttrType()=%d, getAttrName()=%d, data=%hu", pTarget, pLength, l_RC, getAttrType(), getAttrName(), data);
        d_log.write(d_log_buffer, txp::Log::ERROR);
    }
#endif

    return l_RC;
};


int txp::Attr_uint32::cpyData(void* pTarget, size_t pLength) {
    int l_RC = 0;

    CPY_DATA(Attr_uint32);

#ifdef TXP_DEVELOPMENT
    if (l_RC) {
        snprintf(d_log_buffer, sizeof(d_log_buffer), "Attr_uint32::cpyData::cpyData(pTarget=%p, pLength=%zu): RC=%d, getAttrType()=%d, getAttrName()=%d, data=%u", pTarget, pLength, l_RC, getAttrType(), getAttrName(), data);
        d_log.write(d_log_buffer, txp::Log::ERROR);
    }
#endif

    return l_RC;
};


int txp::Attr_uint64::cpyData(void* pTarget, size_t pLength) {
    int l_RC = 0;

    CPY_DATA(Attr_uint64);

#ifdef TXP_DEVELOPMENT
    if (l_RC) {
        snprintf(d_log_buffer, sizeof(d_log_buffer), "Attr_uint64::cpyData::cpyData(pTarget=%p, pLength=%zu): RC=%d, getAttrType()=%d, getAttrName()=%d, data=%lu", pTarget, pLength, l_RC, getAttrType(), getAttrName(), data);
        d_log.write(d_log_buffer, txp::Log::ERROR);
    }
#endif

    return l_RC;
};


int txp::Attr_int8::cpyData(void* pTarget, size_t pLength) {
    int l_RC = 0;

    CPY_DATA(Attr_int8);

#ifdef TXP_DEVELOPMENT
    if (l_RC) {
        snprintf(d_log_buffer, sizeof(d_log_buffer), "Attr_int8::cpyData::cpyData(pTarget=%p, pLength=%zu): RC=%d, getAttrType()=%d, getAttrName()=%d, data=%hhd", pTarget, pLength, l_RC, getAttrType(), getAttrName(), data);
        d_log.write(d_log_buffer, txp::Log::ERROR);
    }
#endif

    return l_RC;
};


int txp::Attr_int16::cpyData(void* pTarget, size_t pLength) {
    int l_RC = 0;

    CPY_DATA(Attr_int16);

#ifdef TXP_DEVELOPMENT
    if (l_RC) {
        snprintf(d_log_buffer, sizeof(d_log_buffer), "Attr_int16::cpyData::cpyData(pTarget=%p, pLength=%zu): RC=%d, getAttrType()=%d, getAttrName()=%d, data=%hd", pTarget, pLength, l_RC, getAttrType(), getAttrName(), data);
        d_log.write(d_log_buffer, txp::Log::ERROR);
    }
#endif

    return l_RC;
};


int txp::Attr_int32::cpyData(void* pTarget, size_t pLength) {
    int l_RC = 0;

    CPY_DATA(Attr_int32);

#ifdef TXP_DEVELOPMENT
    if (l_RC) {
        snprintf(d_log_buffer, sizeof(d_log_buffer), "Attr_int32::cpyData::cpyData(pTarget=%p, pLength=%zu): RC=%d, getAttrType()=%d, getAttrName()=%d, data=%d", pTarget, pLength, l_RC, getAttrType(), getAttrName(), data);
        d_log.write(d_log_buffer, txp::Log::ERROR);
    }
#endif

    return l_RC;
};


int txp::Attr_int64::cpyData(void* pTarget, size_t pLength) {
    int l_RC = 0;

    CPY_DATA(Attr_int64);

#ifdef TXP_DEVELOPMENT
    if (l_RC) {
        snprintf(d_log_buffer, sizeof(d_log_buffer), "Attr_int64::cpyData::cpyData(pTarget=%p, pLength=%zu): RC=%d, getAttrType()=%d, getAttrName()=%d, data=%ld", pTarget, pLength, l_RC, getAttrType(), getAttrName(), data);
        d_log.write(d_log_buffer, txp::Log::ERROR);
    }
#endif

    return l_RC;
};


int txp::AttrPtr_char::cpyData(void* pTarget, size_t pLength) {
    int l_RC = 0;

    CPY_DATA(AttrPtr_char);

#ifdef TXP_DEVELOPMENT
    if (l_RC) {
        snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_char::cpyData::cpyData(pTarget=%p, pLength=%zu): RC=%d, getAttrType()=%d, getAttrName()=%d, data=%p, *data=%c", pTarget, pLength, l_RC, getAttrType(), getAttrName(), data, *data);
        d_log.write(d_log_buffer, txp::Log::ERROR);
    }
#endif

    return l_RC;
};


int txp::AttrPtr_uint8::cpyData(void* pTarget, size_t pLength) {
    int l_RC = 0;

    CPY_DATA(AttrPtr_uint8);

#ifdef TXP_DEVELOPMENT
    if (l_RC) {
        snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_uint8::cpyData::cpyData(pTarget=%p, pLength=%zu): RC=%d, getAttrType()=%d, getAttrName()=%d, data=%p, *data=%hhu", pTarget, pLength, l_RC, getAttrType(), getAttrName(), data, *data);
        d_log.write(d_log_buffer, txp::Log::ERROR);
    }
#endif

    return l_RC;
};


int txp::AttrPtr_uint16::cpyData(void* pTarget, size_t pLength) {
    int l_RC = 0;

    CPY_DATA(AttrPtr_uint16);

#ifdef TXP_DEVELOPMENT
    if (l_RC) {
        snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_uint16::cpyData::cpyData(pTarget=%p, pLength=%zu): RC=%d, getAttrType()=%d, getAttrName()=%d, data=%p, *data=%hu", pTarget, pLength, l_RC, getAttrType(), getAttrName(), data, *data);
        d_log.write(d_log_buffer, txp::Log::ERROR);
    }
#endif

    return l_RC;
};


int txp::AttrPtr_uint32::cpyData(void* pTarget, size_t pLength) {
    int l_RC = 0;

    CPY_DATA(AttrPtr_uint32);

#ifdef TXP_DEVELOPMENT
    if (l_RC) {
        snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_uint32::cpyData::cpyData(pTarget=%p, pLength=%zu): RC=%d, getAttrType()=%d, getAttrName()=%d, data=%p, *data=%u", pTarget, pLength, l_RC, getAttrType(), getAttrName(), data, *data);
        d_log.write(d_log_buffer, txp::Log::ERROR);
    }
#endif

    return l_RC;
};


int txp::AttrPtr_uint64::cpyData(void* pTarget, size_t pLength) {
    int l_RC = 0;

    CPY_DATA(AttrPtr_uint64);

#ifdef TXP_DEVELOPMENT
    if (l_RC) {
        snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_uint64::cpyData::cpyData(pTarget=%p, pLength=%zu): RC=%d, getAttrType()=%d, getAttrName()=%d, data=%p, *data=%lu", pTarget, pLength, l_RC, getAttrType(), getAttrName(), data, *data);
        d_log.write(d_log_buffer, txp::Log::ERROR);
    }
#endif

    return l_RC;
};


int txp::AttrPtr_int8::cpyData(void* pTarget, size_t pLength) {
    int l_RC = 0;

    CPY_DATA(AttrPtr_int8);

#ifdef TXP_DEVELOPMENT
    if (l_RC) {
        snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_int8::cpyData::cpyData(pTarget=%p, pLength=%zu): RC=%d, getAttrType()=%d, getAttrName()=%d, data=%p, *data=%hhd", pTarget, pLength, l_RC, getAttrType(), getAttrName(), data, *data);
        d_log.write(d_log_buffer, txp::Log::ERROR);
    }
#endif

    return l_RC;
};


int txp::AttrPtr_int16::cpyData(void* pTarget, size_t pLength) {
    int l_RC = 0;

    CPY_DATA(AttrPtr_int16);

#ifdef TXP_DEVELOPMENT
    if (l_RC) {
        snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_int16::cpyData::cpyData(pTarget=%p, pLength=%zu): RC=%d, getAttrType()=%d, getAttrName()=%d, data=%p, *data=%hd", pTarget, pLength, l_RC, getAttrType(), getAttrName(), data, *data);
        d_log.write(d_log_buffer, txp::Log::ERROR);
    }
#endif

    return l_RC;
};


int txp::AttrPtr_int32::cpyData(void* pTarget, size_t pLength) {
    int l_RC = 0;

    CPY_DATA(AttrPtr_int32);

#ifdef TXP_DEVELOPMENT
    if (l_RC) {
        snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_int32::cpyData::cpyData(pTarget=%p, pLength=%zu): RC=%d, getAttrType()=%d, getAttrName()=%d, data=%p, *data=%d", pTarget, pLength, l_RC, getAttrType(), getAttrName(), data, *data);
        d_log.write(d_log_buffer, txp::Log::ERROR);
    }
#endif

    return l_RC;
};


int txp::AttrPtr_int64::cpyData(void* pTarget, size_t pLength) {
    int l_RC = 0;

    CPY_DATA(AttrPtr_int64);

#ifdef TXP_DEVELOPMENT
    if (l_RC) {
        snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_int64::cpyData::cpyData(pTarget=%p, pLength=%zu): RC=%d, getAttrType()=%d, getAttrName()=%d, data=%p, *data=%ld", pTarget, pLength, l_RC, getAttrType(), getAttrName(), data, *data);
        d_log.write(d_log_buffer, txp::Log::ERROR);
    }
#endif

    return l_RC;
};


int txp::AttrPtr_char_array::cpyData(void* pTarget, size_t pLength) {
    int l_RC = 0;

    CPY_DATA(AttrPtr_char_array);

#ifdef TXP_DEVELOPMENT
    if (l_RC) {
        snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_char_array::cpyData::cpyData(pTarget=%p, pLength=%zu): RC=%d, getAttrType()=%d, getAttrName()=%d, data=%p", pTarget, pLength, l_RC, getAttrType(), getAttrName(), data);
        d_log.write(d_log_buffer, txp::Log::ERROR);
    }
#endif

    return l_RC;
};


int txp::AttrPtr_array_of_char_arrays::cpyData(void* pTarget, size_t pLength) {
//    int l_RC = 0;
	int l_RC = -445;

	// NOT SURE WHAT TO DO HERE...  @@TODO @@DLH

#ifdef TXP_DEVELOPMENT
    if (l_RC) {
        snprintf(d_log_buffer, sizeof(d_log_buffer), "AttrPtr_array_of_char_arrays::cpyData::cpyData(pTarget=%p, pLength=%zu): RC=%d, getAttrType()=%d, getAttrName()=%d, data=%p", pTarget, pLength, l_RC, getAttrType(), getAttrName(), data);
        d_log.write(d_log_buffer, txp::Log::ERROR);
    }
#endif

    return l_RC;
};


//*****************************************************************************
//  Dump routines for attributes
//*****************************************************************************
void txp::Attribute::dump(Log& pLog, const char* pPrefix) {
#ifdef TXP_DEVELOPMENT
    char l_Buffer[64] = {'\0'};

    attrTypeToChar(attrType, l_Buffer, sizeof(l_Buffer));
    snprintf(d_log_buffer, sizeof(d_log_buffer), "Type:        %s", l_Buffer);
    pLog.write(d_log_buffer, txp::Log::DEBUG, txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL);
    attrNameToChar(attrName, l_Buffer, sizeof(l_Buffer));
    snprintf(d_log_buffer, sizeof(d_log_buffer), "Id:          %s", l_Buffer);
    pLog.write(d_log_buffer, txp::Log::DEBUG, txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL);
    strCpy(d_log_buffer, "Flags:", sizeof(d_log_buffer));
    pLog.write(d_log_buffer, txp::Log::DEBUG, txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL);

    pLog.incrIndent(2);

    snprintf(d_log_buffer, sizeof(d_log_buffer), "Alloc : %c, Data Alloc: %c, Added to Msg: %c", (attrFlag.allocated ? '1' : '0'), (attrFlag.dataAllocated ? '1' : '0'), (attrFlag.addedToMsg ? '1' : '0'));
/*
    snprintf(d_log_buffer, sizeof(d_log_buffer), "Allocated:        %s", (attrFlag.allocated ? "True" : "False"));
    pLog.write(d_log_buffer, txp::Log::DEBUG, txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL);
    snprintf(d_log_buffer, sizeof(d_log_buffer), "Data Allocated:   %s", (attrFlag.dataAllocated ? "True" : "False"));
*/
    pLog.write(d_log_buffer, txp::Log::DEBUG, txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL);
    pLog.decrIndent(2);
#endif

    return;
}


void txp::Attr_char::dump(Log& pLog, const char* pPrefix) {
#ifdef TXP_DEVELOPMENT
    START_PREFIX(pPrefix, ">>>>> Start dump for message attribute:  %s");

    Attribute::dump(pLog);
    if (isprint(data)) {
        snprintf(d_log_buffer, sizeof(d_log_buffer), "Value:       |%c|", data);
    } else {
        snprintf(d_log_buffer, sizeof(d_log_buffer), "Value:       0x%02X", data);
    }
    pLog.write(d_log_buffer, txp::Log::DEBUG, txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL);

    END_PREFIX(pPrefix, ">>>>>   End dump for message attribute:  %s", 0);
#endif

    return;
}


void txp::Attr_uint8::dump(Log& pLog, const char* pPrefix) {
#ifdef TXP_DEVELOPMENT
    START_PREFIX(pPrefix, ">>>>> Start dump for message attribute:  %s");

    Attribute::dump(pLog);
    snprintf(d_log_buffer, sizeof(d_log_buffer), "Value:       0x%02X (%hhu)", data, data);
    pLog.write(d_log_buffer, txp::Log::DEBUG, txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL);

    END_PREFIX(pPrefix, ">>>>>   End dump for message attribute:  %s", 0);
#endif

    return;
}


void txp::Attr_uint16::dump(Log& pLog, const char* pPrefix) {
#ifdef TXP_DEVELOPMENT
    START_PREFIX(pPrefix, ">>>>> Start dump for message attribute:  %s");

    Attribute::dump(pLog);
    snprintf(d_log_buffer, sizeof(d_log_buffer), "Value:       0x%04X (%hu)", data, data);
    pLog.write(d_log_buffer, txp::Log::DEBUG, txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL);

    END_PREFIX(pPrefix, ">>>>>   End dump for message attribute:  %s", 0);
#endif

    return;
}


void txp::Attr_uint32::dump(Log& pLog, const char* pPrefix) {
#ifdef TXP_DEVELOPMENT
    START_PREFIX(pPrefix, ">>>>> Start dump for message attribute:  %s");

    Attribute::dump(pLog);
    snprintf(d_log_buffer, sizeof(d_log_buffer), "Value:       0x%08X (%u)", data, data);
    pLog.write(d_log_buffer, txp::Log::DEBUG, txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL);

    END_PREFIX(pPrefix, ">>>>>   End dump for message attribute:  %s", 0);
#endif

    return;
}


void txp::Attr_uint64::dump(Log& pLog, const char* pPrefix) {
#ifdef TXP_DEVELOPMENT
    START_PREFIX(pPrefix, ">>>>> Start dump for message attribute:  %s");

    Attribute::dump(pLog);
    snprintf(d_log_buffer, sizeof(d_log_buffer), "Value:       0x%016llX (%llu)", (unsigned long long)data, (unsigned long long)data);
    pLog.write(d_log_buffer, txp::Log::DEBUG, txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL);

    END_PREFIX(pPrefix, ">>>>>   End dump for message attribute:  %s", 0);
#endif

    return;
}


void txp::Attr_int8::dump(Log& pLog, const char* pPrefix) {
#ifdef TXP_DEVELOPMENT
    START_PREFIX(pPrefix, ">>>>> Start dump for message attribute:  %s");

    Attribute::dump(pLog);
    snprintf(d_log_buffer, sizeof(d_log_buffer), "Value:       0x%02X (%hhd)", data, data);
    pLog.write(d_log_buffer, txp::Log::DEBUG, txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL);

    END_PREFIX(pPrefix, ">>>>>   End dump for message attribute:  %s", 0);
#endif

    return;
}


void txp::Attr_int16::dump(Log& pLog, const char* pPrefix) {
#ifdef TXP_DEVELOPMENT
    START_PREFIX(pPrefix, ">>>>> Start dump for message attribute:  %s");

    Attribute::dump(pLog);
    snprintf(d_log_buffer, sizeof(d_log_buffer), "Value:       0x%04X (%hd)", data, data);
    pLog.write(d_log_buffer, txp::Log::DEBUG, txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL);

    END_PREFIX(pPrefix, ">>>>>   End dump for message attribute:  %s", 0);
#endif

    return;
}


void txp::Attr_int32::dump(Log& pLog, const char* pPrefix) {
#ifdef TXP_DEVELOPMENT
    START_PREFIX(pPrefix, ">>>>> Start dump for message attribute:  %s");

    Attribute::dump(pLog);
    snprintf(d_log_buffer, sizeof(d_log_buffer), "Value:       0x%08X (%d)", data, data);
    pLog.write(d_log_buffer, txp::Log::DEBUG, txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL);

    END_PREFIX(pPrefix, ">>>>>   End dump for message attribute:  %s", 0);
#endif

    return;
}


void txp::Attr_int64::dump(Log& pLog, const char* pPrefix) {
#ifdef TXP_DEVELOPMENT
    START_PREFIX(pPrefix, ">>>>> Start dump for message attribute:  %s");

    Attribute::dump(pLog);
    snprintf(d_log_buffer, sizeof(d_log_buffer), "Value:       0x%016llX (%lld)", (unsigned long long)data, (signed long long)data);
    pLog.write(d_log_buffer, txp::Log::DEBUG, txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL);

    END_PREFIX(pPrefix, ">>>>>   End dump for message attribute:  %s", 0);
#endif

    return;
}


void txp::AttrPtr_char::dump(Log& pLog, const char* pPrefix) {
#ifdef TXP_DEVELOPMENT
    START_PREFIX(pPrefix, ">>>>> Start dump for message attribute:  %s");

    Attribute::dump(pLog);
    snprintf(d_log_buffer, sizeof(d_log_buffer), "Ptr:         %p", data);
    pLog.write(d_log_buffer, txp::Log::DEBUG, txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL);
    snprintf(d_log_buffer, sizeof(d_log_buffer), "Length:      %lu", sizeof(*data));
    pLog.write(d_log_buffer, txp::Log::DEBUG, txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL);
    if (isprint(*data)) {
        snprintf(d_log_buffer, sizeof(d_log_buffer), "Value:       |%c|", *data);
    } else {
        snprintf(d_log_buffer, sizeof(d_log_buffer), "Value:       0x%02X", *data);
    }
    pLog.write(d_log_buffer, txp::Log::DEBUG, txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL);

    END_PREFIX(pPrefix, ">>>>>   End dump for message attribute:  %s", 0);
#endif

    return;
}


void txp::AttrPtr_uint8::dump(Log& pLog, const char* pPrefix) {
#ifdef TXP_DEVELOPMENT
    START_PREFIX(pPrefix, ">>>>> Start dump for message attribute:  %s");

    Attribute::dump(pLog);
    snprintf(d_log_buffer, sizeof(d_log_buffer), "Ptr:         %p", data);
    pLog.write(d_log_buffer, txp::Log::DEBUG, txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL);
    snprintf(d_log_buffer, sizeof(d_log_buffer), "Length:      %lu", sizeof(*data));
    pLog.write(d_log_buffer, txp::Log::DEBUG, txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL);
    snprintf(d_log_buffer, sizeof(d_log_buffer), "Data Value:  0x%02X (%hhu)", *data, *data);
    pLog.write(d_log_buffer, txp::Log::DEBUG, txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL);

    END_PREFIX(pPrefix, ">>>>>   End dump for message attribute:  %s", 0);
#endif

    return;
}


void txp::AttrPtr_uint16::dump(Log& pLog, const char* pPrefix) {
#ifdef TXP_DEVELOPMENT
    START_PREFIX(pPrefix, ">>>>> Start dump for message attribute:  %s");

    Attribute::dump(pLog);
    snprintf(d_log_buffer, sizeof(d_log_buffer), "Ptr:         %p", data);
    pLog.write(d_log_buffer, txp::Log::DEBUG, txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL);
    snprintf(d_log_buffer, sizeof(d_log_buffer), "Length:      %lu", sizeof(*data));
    pLog.write(d_log_buffer, txp::Log::DEBUG, txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL);
    snprintf(d_log_buffer, sizeof(d_log_buffer), "Data Value:  0x%04X (%hu)", *data, *data);
    pLog.write(d_log_buffer, txp::Log::DEBUG, txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL);

    END_PREFIX(pPrefix, ">>>>>   End dump for message attribute:  %s", 0);
#endif

    return;
}


void txp::AttrPtr_uint32::dump(Log& pLog, const char* pPrefix) {
#ifdef TXP_DEVELOPMENT
    START_PREFIX(pPrefix, ">>>>> Start dump for message attribute:  %s");

    Attribute::dump(pLog);
    snprintf(d_log_buffer, sizeof(d_log_buffer), "Ptr:         %p", data);
    pLog.write(d_log_buffer, txp::Log::DEBUG, txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL);
    snprintf(d_log_buffer, sizeof(d_log_buffer), "Length:      %lu", sizeof(*data));
    pLog.write(d_log_buffer, txp::Log::DEBUG, txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL);
    snprintf(d_log_buffer, sizeof(d_log_buffer), "Data Value:  0x%08X (%u)", *data, *data);
    pLog.write(d_log_buffer, txp::Log::DEBUG, txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL);

    END_PREFIX(pPrefix, ">>>>>   End dump for message attribute:  %s", 0);
#endif

    return;
}


void txp::AttrPtr_uint64::dump(Log& pLog, const char* pPrefix) {
#ifdef TXP_DEVELOPMENT
    START_PREFIX(pPrefix, ">>>>> Start dump for message attribute:  %s");

    Attribute::dump(pLog);
    snprintf(d_log_buffer, sizeof(d_log_buffer), "Ptr:         %p", data);
    pLog.write(d_log_buffer, txp::Log::DEBUG, txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL);
    snprintf(d_log_buffer, sizeof(d_log_buffer), "Length:      %lu", sizeof(*data));
    pLog.write(d_log_buffer, txp::Log::DEBUG, txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL);
    snprintf(d_log_buffer, sizeof(d_log_buffer), "Value:       0x%016llX (%llu)", (unsigned long long)*data, (unsigned long long)*data);
    pLog.write(d_log_buffer, txp::Log::DEBUG, txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL);

    END_PREFIX(pPrefix, ">>>>>   End dump for message attribute:  %s", 0);
#endif

    return;
}


void txp::AttrPtr_int8::dump(Log& pLog, const char* pPrefix) {
#ifdef TXP_DEVELOPMENT
    START_PREFIX(pPrefix, ">>>>> Start dump for message attribute:  %s");

    Attribute::dump(pLog);
    snprintf(d_log_buffer, sizeof(d_log_buffer), "Ptr:         %p", data);
    pLog.write(d_log_buffer, txp::Log::DEBUG, txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL);
    snprintf(d_log_buffer, sizeof(d_log_buffer), "Length:      %lu", sizeof(*data));
    pLog.write(d_log_buffer, txp::Log::DEBUG, txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL);
    snprintf(d_log_buffer, sizeof(d_log_buffer), "Data Value:  0x%02X (%hhd)", *data, *data);
    pLog.write(d_log_buffer, txp::Log::DEBUG, txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL);

    END_PREFIX(pPrefix, ">>>>>   End dump for message attribute:  %s", 0);
#endif

    return;
}


void txp::AttrPtr_int16::dump(Log& pLog, const char* pPrefix) {
#ifdef TXP_DEVELOPMENT
    START_PREFIX(pPrefix, ">>>>> Start dump for message attribute:  %s");

    Attribute::dump(pLog);
    snprintf(d_log_buffer, sizeof(d_log_buffer), "Ptr:         %p", data);
    pLog.write(d_log_buffer, txp::Log::DEBUG, txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL);
    snprintf(d_log_buffer, sizeof(d_log_buffer), "Length:      %lu", sizeof(*data));
    pLog.write(d_log_buffer, txp::Log::DEBUG, txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL);
    snprintf(d_log_buffer, sizeof(d_log_buffer), "Data Value:  0x%04X (%hd)", *data, *data);
    pLog.write(d_log_buffer, txp::Log::DEBUG, txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL);

    END_PREFIX(pPrefix, ">>>>>   End dump for message attribute:  %s", 0);
#endif

    return;
}


void txp::AttrPtr_int32::dump(Log& pLog, const char* pPrefix) {
#ifdef TXP_DEVELOPMENT
    START_PREFIX(pPrefix, ">>>>> Start dump for message attribute:  %s");

    Attribute::dump(pLog);
    snprintf(d_log_buffer, sizeof(d_log_buffer), "Ptr:         %p", data);
    pLog.write(d_log_buffer, txp::Log::DEBUG, txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL);
    snprintf(d_log_buffer, sizeof(d_log_buffer), "Length:      %lu", sizeof(*data));
    pLog.write(d_log_buffer, txp::Log::DEBUG, txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL);
    snprintf(d_log_buffer, sizeof(d_log_buffer), "Data Value:  0x%08X (%d)", *data, *data);
    pLog.write(d_log_buffer, txp::Log::DEBUG, txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL);

    END_PREFIX(pPrefix, ">>>>>   End dump for message attribute:  %s", 0);
#endif

    return;
}


void txp::AttrPtr_int64::dump(Log& pLog, const char* pPrefix) {
#ifdef TXP_DEVELOPMENT
    START_PREFIX(pPrefix, ">>>>> Start dump for message attribute:  %s");

    Attribute::dump(pLog);
    snprintf(d_log_buffer, sizeof(d_log_buffer), "Ptr:         %p", data);
    pLog.write(d_log_buffer, txp::Log::DEBUG, txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL);
    snprintf(d_log_buffer, sizeof(d_log_buffer), "Length:      %lu", sizeof(*data));
    pLog.write(d_log_buffer, txp::Log::DEBUG, txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL);
    snprintf(d_log_buffer, sizeof(d_log_buffer), "Value:       0x%016llX (%lld)", (unsigned long long)*data, (signed long long)*data);
    pLog.write(d_log_buffer, txp::Log::DEBUG, txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL);

    END_PREFIX(pPrefix, ">>>>>   End dump for message attribute:  %s", 0);
#endif

    return;
}


void txp::AttrPtr_char_array::dump(Log& pLog, const char* pPrefix) {
#ifdef TXP_DEVELOPMENT
    const size_t l_BufferLen = MAXIMUM_ATTRIBUTE_DUMP_SIZE;
    char l_Buffer[l_BufferLen] = {'\0'};

    START_PREFIX(pPrefix, ">>>>> Start dump for message attribute:  %s");

    Attribute::dump(pLog);
    snprintf(d_log_buffer, sizeof(d_log_buffer), "Ptr:         %p", data);
    pLog.write(d_log_buffer, txp::Log::DEBUG, txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL);
    snprintf(d_log_buffer, sizeof(d_log_buffer), "Length:      0x%08X (%zu)", (uint32_t)length, length);
    pLog.write(d_log_buffer, txp::Log::DEBUG, txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL);

    size_t l_Length = length<l_BufferLen ? length : l_BufferLen;
    memcpy(l_Buffer, data, l_Length);

    strCpy(d_log_buffer, "Data Value:", sizeof(d_log_buffer));
    pLog.write(d_log_buffer, txp::Log::DEBUG, txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL);

    pLog.incrIndent(2);
    txp::Log::dump_buffer_raw(pLog, l_Buffer, l_Length, 0);
    pLog.decrIndent(2);

    END_PREFIX(pPrefix, ">>>>>   End dump for message attribute:  %s", 0);
#endif

    return;
}


void txp::AttrPtr_array_of_char_arrays::dump(Log& pLog, const char* pPrefix) {
#ifdef TXP_DEVELOPMENT
    const size_t l_BufferLen = MAXIMUM_ATTRIBUTE_DUMP_SIZE;
    char l_Buffer[l_BufferLen] = {'\0'};

    START_PREFIX(pPrefix, ">>>>> Start dump for message attribute:  %s");

    Attribute::dump(pLog);
    snprintf(d_log_buffer, sizeof(d_log_buffer), "Ptr:         %p", data);
    pLog.write(d_log_buffer, txp::Log::DEBUG, txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL);
    snprintf(d_log_buffer, sizeof(d_log_buffer), "# Elements:  0x%08X (%zu)", (uint32_t)number_of_elements, number_of_elements);
    pLog.write(d_log_buffer, txp::Log::DEBUG, txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL);
    snprintf(d_log_buffer, sizeof(d_log_buffer), "Len/data:    0x%08X (%zu)", (uint32_t)length_of_data, length_of_data);
    pLog.write(d_log_buffer, txp::Log::DEBUG, txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL);
    snprintf(d_log_buffer, sizeof(d_log_buffer), "Len/values:  0x%08X (%zu)", (uint32_t)length_of_values, length_of_values);
    pLog.write(d_log_buffer, txp::Log::DEBUG, txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL);
    snprintf(d_log_buffer, sizeof(d_log_buffer), "Char Arrays Allocated:  %s", (attrFlag2.dataElementCharArraysAllocated ? "True" : "False"));
    pLog.write(d_log_buffer, txp::Log::DEBUG, txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL);
    pLog.write(" ", txp::Log::DEBUG, txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL);

	int i=1;
	size_t l_Length;
    for (CharArrayIter it = data->begin(); it != data->end(); ++it) {
    	snprintf(d_log_buffer, sizeof(d_log_buffer), "Element %3d", i);
    	pLog.write(d_log_buffer, txp::Log::DEBUG, txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL);

    	pLog.incrIndent(2);

    	snprintf(d_log_buffer, sizeof(d_log_buffer), "Length                %zu", it->first);
    	pLog.write(d_log_buffer, txp::Log::DEBUG, txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL);
   	    l_Length = it->first<l_BufferLen ? it->first : l_BufferLen;
	    memcpy(l_Buffer, it->second, l_Length);
    	strCpy(d_log_buffer, "Data Value:", sizeof(d_log_buffer));
    	pLog.write(d_log_buffer, txp::Log::DEBUG, txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL);
    	txp::Log::dump_buffer_raw(pLog, l_Buffer, l_Length, 0);

    	pLog.decrIndent(2);

       	++i;
    }

    END_PREFIX(pPrefix, ">>>>>   End dump for message attribute:  %s", 0);
#endif

    return;
}


//*****************************************************************************
//  Get length in buffer routines
//*****************************************************************************
size_t txp::Attribute::getLengthInBuffer() {
    return txp::LENGTH_OF_ATTR_TYPE + txp::LENGTH_OF_ATTR_NAME;
}


size_t txp::Attr_char::getLengthInBuffer() {
    return Attribute::getLengthInBuffer() + sizeof(char);
}


size_t txp::Attr_uint8::getLengthInBuffer() {
    return Attribute::getLengthInBuffer() + sizeof(uint8_t);
}


size_t txp::Attr_uint16::getLengthInBuffer() {
    return Attribute::getLengthInBuffer() + sizeof(uint16_t);
}


size_t txp::Attr_uint32::getLengthInBuffer() {
    return Attribute::getLengthInBuffer() + sizeof(uint32_t);
}


size_t txp::Attr_uint64::getLengthInBuffer() {
    return Attribute::getLengthInBuffer() + sizeof(uint64_t);
}


size_t txp::Attr_int8::getLengthInBuffer() {
    return Attribute::getLengthInBuffer() + sizeof(int8_t);
}


size_t txp::Attr_int16::getLengthInBuffer() {
    return Attribute::getLengthInBuffer() + sizeof(int16_t);
}


size_t txp::Attr_int32::getLengthInBuffer() {
    return Attribute::getLengthInBuffer() + sizeof(int32_t);
}


size_t txp::Attr_int64::getLengthInBuffer() {
    return Attribute::getLengthInBuffer() + sizeof(int64_t);
}


size_t txp::AttrPtr_char::getLengthInBuffer() {
    return Attribute::getLengthInBuffer() + sizeof(char*);
}


size_t txp::AttrPtr_uint8::getLengthInBuffer() {
    return Attribute::getLengthInBuffer() + sizeof(uint8_t*);
}


size_t txp::AttrPtr_uint16::getLengthInBuffer() {
    return Attribute::getLengthInBuffer() + sizeof(uint16_t*);
}


size_t txp::AttrPtr_uint32::getLengthInBuffer() {
    return Attribute::getLengthInBuffer() + sizeof(uint32_t*);
}


size_t txp::AttrPtr_uint64::getLengthInBuffer() {
    return Attribute::getLengthInBuffer() + sizeof(uint64_t*);
}


size_t txp::AttrPtr_int8::getLengthInBuffer() {
    return Attribute::getLengthInBuffer() + sizeof(int8_t*);
}


size_t txp::AttrPtr_int16::getLengthInBuffer() {
    return Attribute::getLengthInBuffer() + sizeof(int16_t*);
}


size_t txp::AttrPtr_int32::getLengthInBuffer() {
    return Attribute::getLengthInBuffer() + sizeof(int32_t*);
}


size_t txp::AttrPtr_int64::getLengthInBuffer() {
    return Attribute::getLengthInBuffer() + sizeof(int64_t*);
}


size_t txp::AttrPtr_char_array::getLengthInBuffer() {
    return Attribute::getLengthInBuffer() + LENGTH_OF_ATTR_CHAR_ARRAY_LENGTH + sizeof(char*);
}


size_t txp::AttrPtr_array_of_char_arrays::getLengthInBuffer() {
    return Attribute::getLengthInBuffer() + LENGTH_OF_ATTR_ARRAY_OF_CHAR_ARRAYS_NUMBER_OF_ELEMENTS + LENGTH_OF_ATTR_ARRAY_OF_CHAR_ARRAYS_LENGTH + length_of_data;
}


//*****************************************************************************
//  Get length of value in buffer routines
//*****************************************************************************
size_t txp::Attribute::getLengthOfValueInBuffer() {
    return Attribute::getLengthInBuffer();
}


size_t txp::Attr_char::getLengthOfValueInBuffer() {
    return Attr_char::getLengthInBuffer();
}


size_t txp::Attr_uint8::getLengthOfValueInBuffer() {
    return Attr_uint8::getLengthInBuffer();
}


size_t txp::Attr_uint16::getLengthOfValueInBuffer() {
    return Attr_uint16::getLengthInBuffer();
}


size_t txp::Attr_uint32::getLengthOfValueInBuffer() {
    return Attr_uint32::getLengthInBuffer();
}


size_t txp::Attr_uint64::getLengthOfValueInBuffer() {
    return Attr_uint64::getLengthInBuffer();
}


size_t txp::Attr_int8::getLengthOfValueInBuffer() {
    return Attr_int8::getLengthInBuffer();
}


size_t txp::Attr_int16::getLengthOfValueInBuffer() {
    return Attr_int16::getLengthInBuffer();
}


size_t txp::Attr_int32::getLengthOfValueInBuffer() {
    return Attr_int32::getLengthInBuffer();
}


size_t txp::Attr_int64::getLengthOfValueInBuffer() {
    return Attr_int64::getLengthInBuffer();
}


size_t txp::AttrPtr_char::getLengthOfValueInBuffer() {
    return Attribute::getLengthOfValueInBuffer() + sizeof(char);
}


size_t txp::AttrPtr_uint8::getLengthOfValueInBuffer() {
    return Attribute::getLengthOfValueInBuffer() + sizeof(uint8_t);
}


size_t txp::AttrPtr_uint16::getLengthOfValueInBuffer() {
    return Attribute::getLengthOfValueInBuffer() + sizeof(uint16_t);
}


size_t txp::AttrPtr_uint32::getLengthOfValueInBuffer() {
    return Attribute::getLengthOfValueInBuffer() + sizeof(uint32_t);
}


size_t txp::AttrPtr_uint64::getLengthOfValueInBuffer() {
    return Attribute::getLengthOfValueInBuffer() + sizeof(uint64_t);
}


size_t txp::AttrPtr_int8::getLengthOfValueInBuffer() {
    return Attribute::getLengthOfValueInBuffer() + sizeof(int8_t);
}


size_t txp::AttrPtr_int16::getLengthOfValueInBuffer() {
    return Attribute::getLengthOfValueInBuffer() + sizeof(int16_t);
}


size_t txp::AttrPtr_int32::getLengthOfValueInBuffer() {
    return Attribute::getLengthOfValueInBuffer() + sizeof(int32_t);
}


size_t txp::AttrPtr_int64::getLengthOfValueInBuffer() {
    return Attribute::getLengthOfValueInBuffer() + sizeof(int64_t);
}


size_t txp::AttrPtr_char_array::getLengthOfValueInBuffer() {
    return Attribute::getLengthOfValueInBuffer() + sizeof(LENGTH_OF_ATTR_CHAR_ARRAY_LENGTH) + length;
}


size_t txp::AttrPtr_array_of_char_arrays::getLengthOfValueInBuffer() {
    return Attribute::getLengthOfValueInBuffer() + LENGTH_OF_ATTR_ARRAY_OF_CHAR_ARRAYS_NUMBER_OF_ELEMENTS + LENGTH_OF_ATTR_ARRAY_OF_CHAR_ARRAYS_LENGTH + length_of_values;
}


//*****************************************************************************
//  Get length of data routines
//*****************************************************************************
ssize_t txp::Attr_char::getDataLength() {
    return sizeof(data);
}


ssize_t txp::Attr_uint8::getDataLength() {
    return sizeof(data);
}


ssize_t txp::Attr_uint16::getDataLength() {
    return sizeof(data);
}


ssize_t txp::Attr_uint32::getDataLength() {
    return sizeof(data);
}


ssize_t txp::Attr_uint64::getDataLength() {
    return sizeof(data);
}


ssize_t txp::Attr_int8::getDataLength() {
    return sizeof(data);
}


ssize_t txp::Attr_int16::getDataLength() {
    return sizeof(data);
}


ssize_t txp::Attr_int32::getDataLength() {
    return sizeof(data);
}


ssize_t txp::Attr_int64::getDataLength() {
    return sizeof(data);
}


ssize_t txp::AttrPtr_char::getDataLength() {
    return sizeof(data);
}


ssize_t txp::AttrPtr_uint8::getDataLength() {
    return sizeof(*data);
}


ssize_t txp::AttrPtr_uint16::getDataLength() {
    return sizeof(*data);
}


ssize_t txp::AttrPtr_uint32::getDataLength() {
    return sizeof(*data);
}


ssize_t txp::AttrPtr_uint64::getDataLength() {
    return sizeof(*data);
}


ssize_t txp::AttrPtr_int8::getDataLength() {
    return sizeof(*data);
}


ssize_t txp::AttrPtr_int16::getDataLength() {
    return sizeof(*data);
}


ssize_t txp::AttrPtr_int32::getDataLength() {
    return sizeof(*data);
}


ssize_t txp::AttrPtr_int64::getDataLength() {
    return sizeof(*data);
}


ssize_t txp::AttrPtr_char_array::getDataLength() {
    return length;
}


ssize_t txp::AttrPtr_array_of_char_arrays::getDataLength() {
	// Not valid for this attribute type...  @@DLH
    return -444;
}


//*****************************************************************************
//  Non-Static, Non-Virtual methods
//*****************************************************************************

//*****************************************************************************
//  Static methods
//*****************************************************************************
void txp::Attribute::attrNameToChar(const txp::AttributeName pName, char* pBuffer, const int32_t pSize) {
    pBuffer[0] = '\0';
    if (pSize) {
        switch (pName) {
            //  Valid FUSE message ids
            case txp::ATTRIBUTE_NAME_INVALID:
                strCpy(pBuffer, "ATTRIBUTE_NAME_INVALID", pSize);
#ifdef TXP_DEVELOPMENT
                snprintf(d_log_buffer, sizeof(d_log_buffer), "Attribute::attrNameToChar(%d, %p, %d)", pName, pBuffer, pSize);
                d_log.write(d_log_buffer, txp::Log::ERROR);
#endif
                break;
#include "gened_Attribute_CC_Include_2.h"

            default:
                snprintf(pBuffer, pSize, "%s (%d)", "UNDEFINED", pName);
#ifdef TXP_DEVELOPMENT
                snprintf(d_log_buffer, sizeof(d_log_buffer), "Attribute::attrNameToChar(%d, %p, %d)", pName, pBuffer, pSize);
                d_log.write(d_log_buffer, txp::Log::ERROR);
#endif
                break;
        }
    }

    return;
}


void txp::Attribute::attrTypeToChar(const txp::AttributeType pType, char* pBuffer, const int32_t pSize) {
    if (pSize) {
        pBuffer[0] = '\0';
        switch (pType) {
            case txp::ATTRIBUTE_TYPE_INVALID:
                strCpy(pBuffer, "ATTRIBUTE_TYPE_INVALID", pSize);
#ifdef TXP_DEVELOPMENT
                snprintf(d_log_buffer, sizeof(d_log_buffer), "Attribute::attrTypeToChar(%d, %p, %d)", pType, pBuffer, pSize);
                d_log.write(d_log_buffer, txp::Log::ERROR);
#endif
                break;
            case txp::CHAR:
                strCpy(pBuffer, "CHAR", pSize);
                break;
            case txp::UINT8:
                strCpy(pBuffer, "UINT8", pSize);
                break;
            case txp::UINT16:
                strCpy(pBuffer, "UINT16", pSize);
                break;
            case txp::UINT32:
                strCpy(pBuffer, "UINT32", pSize);
                break;
            case txp::UINT64:
                strCpy(pBuffer, "UINT64", pSize);
                break;
            case txp::INT8:
                strCpy(pBuffer, "INT8", pSize);
                break;
            case txp::INT16:
                strCpy(pBuffer, "INT16", pSize);
                break;
            case txp::INT32:
                strCpy(pBuffer, "INT32", pSize);
                break;
            case txp::INT64:
                strCpy(pBuffer, "INT64", pSize);
                break;
            case txp::CHAR_ARRAY:
                strCpy(pBuffer, "CHAR_ARRAY", pSize);
                break;
            case txp::ARRAY_OF_CHAR_ARRAYS:
                strCpy(pBuffer, "ARRAY_OF_CHAR_ARRAYS", pSize);
                break;
            case txp::PTR_CHAR:
                strCpy(pBuffer, "PTR_CHAR", pSize);
                break;
            case txp::PTR_UINT8:
                strCpy(pBuffer, "PTR_UINT8", pSize);
                break;
            case txp::PTR_UINT16:
                strCpy(pBuffer, "PTR_UINT16", pSize);
                break;
            case txp::PTR_UINT32:
                strCpy(pBuffer, "PTR_UINT32", pSize);
                break;
            case txp::PTR_UINT64:
                strCpy(pBuffer, "PTR_UINT64", pSize);
                break;
            case txp::PTR_INT8:
                strCpy(pBuffer, "PTR_INT8", pSize);
                break;
            case txp::PTR_INT16:
                strCpy(pBuffer, "PTR_INT16", pSize);
                break;
            case txp::PTR_INT32:
                strCpy(pBuffer, "PTR_INT32", pSize);
                break;
            case txp::PTR_INT64:
                strCpy(pBuffer, "PTR_INT64", pSize);
                break;
            case txp::PTR_CHAR_ARRAY:
                strCpy(pBuffer, "PTR_CHAR_ARRAY", pSize);
                break;
            case txp::PTR_ARRAY_OF_CHAR_ARRAYS:
                strCpy(pBuffer, "PTR_ARRAY_OF_CHAR_ARRAYS", pSize);
                break;

            default:
                snprintf(pBuffer, pSize, "%s (%d)", "UNDEFINED", pType);
#ifdef TXP_DEVELOPMENT
                snprintf(d_log_buffer, sizeof(d_log_buffer), "Attribute::attrTypeToChar(%d, %p, %d)", pType, pBuffer, pSize);
                d_log.write(d_log_buffer, txp::Log::ERROR);
#endif
                break;
        }
    }

    return;
}


int txp::Attribute::isAttrNameValid(const txp::AttributeName pAttrName) {
    int l_RC = 1;

    switch (pAttrName) {
#include "gened_Attribute_CC_Include_1.h"

        default:
            l_RC = 0;
    }

#ifdef TXP_DEVELOPMENT
	if (!l_RC) {
		snprintf(d_log_buffer, sizeof(d_log_buffer), "isAttrNameValid(pAttrName(%d)): RC=%d", pAttrName, l_RC);
		d_log.write(d_log_buffer, txp::Log::WARN);
	}
#endif

    return l_RC;
}


int txp::Attribute::isAttrNameValid(const int32_t pAttrNameValue) {

    return txp::Attribute::isAttrNameValid((txp::AttributeName)pAttrNameValue);
}


int txp::Attribute::isAttrTypeValid(const txp::AttributeType pAttrType) {
    int l_RC = 1;

    switch (pAttrType) {
    	case CHAR:
        case UINT8:
        case UINT16:
        case UINT32:
        case UINT64:
        case INT8:
        case INT16:
        case INT32:
        case INT64:
        case CHAR_ARRAY:
        case ARRAY_OF_CHAR_ARRAYS:
        case PTR_CHAR:
        case PTR_UINT8:
        case PTR_UINT16:
        case PTR_UINT32:
        case PTR_UINT64:
        case PTR_INT8:
        case PTR_INT16:
        case PTR_INT32:
        case PTR_INT64:
        case PTR_CHAR_ARRAY:
        case PTR_ARRAY_OF_CHAR_ARRAYS:
        	break;

        default:
            l_RC = 0;
    }

#ifdef TXP_DEVELOPMENT
	if (!l_RC) {
		snprintf(d_log_buffer, sizeof(d_log_buffer), "isAttrTypeValid(pAttrType(%d)): RC=%d", pAttrType, l_RC);
		d_log.write(d_log_buffer, txp::Log::WARN);
	}
#endif

    return l_RC;
}


int txp::Attribute::isAttrTypeValid(const int32_t pAttrTypeValue) {

    return txp::Attribute::isAttrTypeValid((txp::AttributeType)pAttrTypeValue);
}


//*****************************************************************************
//  Building attributes (facility allocated)
//*****************************************************************************
int txp::Attr_char::buildAttr_char(const txp::AttributeName pName, const char& pData, Attr_char* &oAttribute) {
    BUILD_ATTR(Attr_char);
}


int txp::Attr_uint8::buildAttr_uint8(const txp::AttributeName pName, const uint8_t& pData, Attr_uint8* &oAttribute) {
    BUILD_ATTR(Attr_uint8);
}


int txp::Attr_uint16::buildAttr_uint16(const txp::AttributeName pName, const uint16_t& pData, Attr_uint16* &oAttribute) {
    BUILD_ATTR(Attr_uint16);
}


int txp::Attr_uint32::buildAttr_uint32(const txp::AttributeName pName, const uint32_t& pData, Attr_uint32* &oAttribute) {
    BUILD_ATTR(Attr_uint32);
}


int txp::Attr_uint64::buildAttr_uint64(const txp::AttributeName pName, const uint64_t& pData, Attr_uint64* &oAttribute) {
    BUILD_ATTR(Attr_uint64);
}


int txp::Attr_int8::buildAttr_int8(const txp::AttributeName pName, const int8_t& pData, Attr_int8* &oAttribute) {
    BUILD_ATTR(Attr_int8);
}


int txp::Attr_int16::buildAttr_int16(const txp::AttributeName pName, const int16_t& pData, Attr_int16* &oAttribute) {
    BUILD_ATTR(Attr_int16);
}


int txp::Attr_int32::buildAttr_int32(const txp::AttributeName pName, const int32_t& pData, Attr_int32* &oAttribute) {
    BUILD_ATTR(Attr_int32);
}


int txp::Attr_int64::buildAttr_int64(const txp::AttributeName pName, const int64_t& pData, Attr_int64* &oAttribute) {
    BUILD_ATTR(Attr_int64);
}


int txp::AttrPtr_char::buildAttrPtr_char(const txp::AttributeName pName, const char* pData, AttrPtr_char* &oAttribute) {
    BUILD_ATTR(AttrPtr_char);
}


int txp::AttrPtr_uint8::buildAttrPtr_uint8(const txp::AttributeName pName, const uint8_t* pData, AttrPtr_uint8* &oAttribute) {
    BUILD_ATTR(AttrPtr_uint8);
}


int txp::AttrPtr_uint16::buildAttrPtr_uint16(const txp::AttributeName pName, const uint16_t* pData, AttrPtr_uint16* &oAttribute) {
    BUILD_ATTR(AttrPtr_uint16);
}


int txp::AttrPtr_uint32::buildAttrPtr_uint32(const txp::AttributeName pName, const uint32_t* pData, AttrPtr_uint32* &oAttribute) {
    BUILD_ATTR(AttrPtr_uint32);
}


int txp::AttrPtr_uint64::buildAttrPtr_uint64(const txp::AttributeName pName, const uint64_t* pData, AttrPtr_uint64* &oAttribute) {
    BUILD_ATTR(AttrPtr_uint64);
}


int txp::AttrPtr_int8::buildAttrPtr_int8(const txp::AttributeName pName, const int8_t* pData, AttrPtr_int8* &oAttribute) {
    BUILD_ATTR(AttrPtr_int8);
}


int txp::AttrPtr_int16::buildAttrPtr_int16(const txp::AttributeName pName, const int16_t* pData, AttrPtr_int16* &oAttribute) {
    BUILD_ATTR(AttrPtr_int16);
}


int txp::AttrPtr_int32::buildAttrPtr_int32(const txp::AttributeName pName, const int32_t* pData, AttrPtr_int32* &oAttribute) {
    BUILD_ATTR(AttrPtr_int32);
}


int txp::AttrPtr_int64::buildAttrPtr_int64(const txp::AttributeName pName, const int64_t* pData, AttrPtr_int64* &oAttribute) {
    BUILD_ATTR(AttrPtr_int64);
}


int txp::AttrPtr_char_array::buildAttrPtr_char_array(const txp::AttributeName pName, const char* pData, const size_t pLength, txp::AttrPtr_char_array* &oAttribute, const txp::COPY_DATA_OPTION pCopyDataOption) {
	int l_RC = 0;

	if (pCopyDataOption == COPY_TO_HEAP) {
		char* l_Temp = new char[pLength];
		strCpy(l_Temp, pData, pLength);
		oAttribute = new AttrPtr_char_array(pName, l_Temp, pLength);
	} else {
		oAttribute = new AttrPtr_char_array(pName, pData, pLength);
	}

	if (!l_RC) {
		oAttribute->setAllocatedFlag(1);
		if (pCopyDataOption == COPY_TO_HEAP) {
			oAttribute->setDataAllocatedFlag(1);
		}
	} else {
		if (oAttribute) {
			delete oAttribute;
			oAttribute = 0;
		}
	}

	return l_RC;
}


int txp::AttrPtr_array_of_char_arrays::buildAttrPtr_array_of_char_arrays(const txp::AttributeName pName, const txp::CharArray* pData, txp::AttrPtr_array_of_char_arrays* &oAttribute) {
	int l_RC = 0;

	oAttribute = new AttrPtr_array_of_char_arrays(pName, pData);

	if (!l_RC) {
		oAttribute->setAllocatedFlag(1);
	} else {
		if (oAttribute) {
			delete oAttribute;
			oAttribute = 0;
		}
	}

	return l_RC;
}


//  Build an attribute from a heap buffer location
int txp::Attribute::buildAttr(txp::HeapBuffer* pBuffer, const size_t pOffset, txp::Attribute* &oAttribute, size_t &pLength, const txp::DeserializeOption &pOption, unsigned long* pCRC) {
    int l_RC = 0;

    txp::AttributeType l_Type = pBuffer->getAttrType(pOffset);

    switch(pOption) {
        case txp::DO_NOT_COPY_DATA:
        case txp::COPY_DATA_TO_HEAP:
            switch(l_Type) {
                case txp::ATTRIBUTE_TYPE_INVALID:
                    l_RC = -419;
                    break;
                case txp::CHAR:
                    oAttribute = new AttrPtr_char(pBuffer, pOffset, pOption, txp::BUFFER_HAS_DATA);
                    break;
                case txp::UINT8:
                    oAttribute = new AttrPtr_uint8(pBuffer, pOffset, pOption, txp::BUFFER_HAS_DATA);
                    break;
                case txp::UINT16:
                    oAttribute = new AttrPtr_uint16(pBuffer, pOffset, pOption, txp::BUFFER_HAS_DATA);
                    break;
                case txp::UINT32:
                    oAttribute = new AttrPtr_uint32(pBuffer, pOffset, pOption, txp::BUFFER_HAS_DATA);
                    break;
                case txp::UINT64:
                    oAttribute = new AttrPtr_uint64(pBuffer, pOffset, pOption, txp::BUFFER_HAS_DATA);
                    break;
                case txp::INT8:
                    oAttribute = new AttrPtr_int8(pBuffer, pOffset, pOption, txp::BUFFER_HAS_DATA);
                    break;
                case txp::INT16:
                    oAttribute = new AttrPtr_int16(pBuffer, pOffset, pOption, txp::BUFFER_HAS_DATA);
                    break;
                case txp::INT32:
                    oAttribute = new AttrPtr_int32(pBuffer, pOffset, pOption, txp::BUFFER_HAS_DATA);
                    break;
                case txp::INT64:
                    oAttribute = new AttrPtr_int64(pBuffer, pOffset, pOption, txp::BUFFER_HAS_DATA);
                    break;
                case txp::CHAR_ARRAY:
                    oAttribute = new AttrPtr_char_array(pBuffer, pOffset, pOption, txp::BUFFER_HAS_DATA);
                    break;
                case txp::ARRAY_OF_CHAR_ARRAYS:
                    oAttribute = new AttrPtr_array_of_char_arrays(pBuffer, pOffset, pOption, txp::BUFFER_HAS_DATA);
                    break;
                case txp::PTR_CHAR:
                    oAttribute = new AttrPtr_char(pBuffer, pOffset, pOption, txp::BUFFER_HAS_PTR_TO_DATA);
                    break;
                case txp::PTR_UINT8:
                    oAttribute = new AttrPtr_uint8(pBuffer, pOffset, pOption, txp::BUFFER_HAS_PTR_TO_DATA);
                    break;
                case txp::PTR_UINT16:
                    oAttribute = new AttrPtr_uint16(pBuffer, pOffset, pOption, txp::BUFFER_HAS_PTR_TO_DATA);
                    break;
                case txp::PTR_UINT32:
                    oAttribute = new AttrPtr_uint32(pBuffer, pOffset, pOption, txp::BUFFER_HAS_PTR_TO_DATA);
                    break;
                case txp::PTR_UINT64:
                    oAttribute = new AttrPtr_uint64(pBuffer, pOffset, pOption, txp::BUFFER_HAS_PTR_TO_DATA);
                    break;
                case txp::PTR_INT8:
                    oAttribute = new AttrPtr_int8(pBuffer, pOffset, pOption, txp::BUFFER_HAS_PTR_TO_DATA);
                    break;
                case txp::PTR_INT16:
                    oAttribute = new AttrPtr_int16(pBuffer, pOffset, pOption, txp::BUFFER_HAS_PTR_TO_DATA);
                    break;
                case txp::PTR_INT32:
                    oAttribute = new AttrPtr_int32(pBuffer, pOffset, pOption, txp::BUFFER_HAS_PTR_TO_DATA);
                    break;
                case txp::PTR_INT64:
                    oAttribute = new AttrPtr_int64(pBuffer, pOffset, pOption, txp::BUFFER_HAS_PTR_TO_DATA);
                    break;
                case txp::PTR_CHAR_ARRAY:
                    oAttribute = new AttrPtr_char_array(pBuffer, pOffset, pOption, txp::BUFFER_HAS_PTR_TO_DATA);
                    break;
                case txp::PTR_ARRAY_OF_CHAR_ARRAYS:
                    oAttribute = new AttrPtr_array_of_char_arrays(pBuffer, pOffset, pOption, txp::BUFFER_HAS_PTR_TO_DATA);
                    break;

                default:
                    l_RC = -420;
                    break;
            }
            break;

        case txp::COPY_DATA_TO_OBJECT:
        case txp::COPY_DATA_TO_OBJECT_BUT_DO_NOT_COPY_ARRAYS:
            switch(l_Type) {
                case txp::ATTRIBUTE_TYPE_INVALID:
                    l_RC = -421;
                    break;
                case txp::CHAR:
                    oAttribute = new Attr_char(pBuffer, pOffset, pOption, txp::BUFFER_HAS_DATA);
                    break;
                case txp::UINT8:
                    oAttribute = new Attr_uint8(pBuffer, pOffset, pOption, txp::BUFFER_HAS_DATA);
                    break;
                case txp::UINT16:
                    oAttribute = new Attr_uint16(pBuffer, pOffset, pOption, txp::BUFFER_HAS_DATA);
                    break;
                case txp::UINT32:
                    oAttribute = new Attr_uint32(pBuffer, pOffset, pOption, txp::BUFFER_HAS_DATA);
                    break;
                case txp::UINT64:
                    oAttribute = new Attr_uint64(pBuffer, pOffset, pOption, txp::BUFFER_HAS_DATA);
                    break;
                case txp::INT8:
                    oAttribute = new Attr_int8(pBuffer, pOffset, pOption, txp::BUFFER_HAS_DATA);
                    break;
                case txp::INT16:
                    oAttribute = new Attr_int16(pBuffer, pOffset, pOption, txp::BUFFER_HAS_DATA);
                    break;
                case txp::INT32:
                    oAttribute = new Attr_int32(pBuffer, pOffset, pOption, txp::BUFFER_HAS_DATA);
                    break;
                case txp::INT64:
                    oAttribute = new Attr_int64(pBuffer, pOffset, pOption, txp::BUFFER_HAS_DATA);
                    break;
                case txp::CHAR_ARRAY:
                    oAttribute = new AttrPtr_char_array(pBuffer, pOffset, pOption, txp::BUFFER_HAS_DATA);
                    break;
                case txp::ARRAY_OF_CHAR_ARRAYS:
                    oAttribute = new AttrPtr_array_of_char_arrays(pBuffer, pOffset, pOption, txp::BUFFER_HAS_DATA);
                    break;
                case txp::PTR_CHAR:
                    oAttribute = new Attr_char(pBuffer, pOffset, pOption, txp::BUFFER_HAS_PTR_TO_DATA);
                    break;
                case txp::PTR_UINT8:
                    oAttribute = new Attr_uint8(pBuffer, pOffset, pOption, txp::BUFFER_HAS_PTR_TO_DATA);
                    break;
                case txp::PTR_UINT16:
                    oAttribute = new Attr_uint16(pBuffer, pOffset, pOption, txp::BUFFER_HAS_PTR_TO_DATA);
                    break;
                case txp::PTR_UINT32:
                    oAttribute = new Attr_uint32(pBuffer, pOffset, pOption, txp::BUFFER_HAS_PTR_TO_DATA);
                    break;
                case txp::PTR_UINT64:
                    oAttribute = new Attr_uint64(pBuffer, pOffset, pOption, txp::BUFFER_HAS_PTR_TO_DATA);
                    break;
                case txp::PTR_INT8:
                    oAttribute = new Attr_int8(pBuffer, pOffset, pOption, txp::BUFFER_HAS_PTR_TO_DATA);
                    break;
                case txp::PTR_INT16:
                    oAttribute = new Attr_int16(pBuffer, pOffset, pOption, txp::BUFFER_HAS_PTR_TO_DATA);
                    break;
                case txp::PTR_INT32:
                    oAttribute = new Attr_int32(pBuffer, pOffset, pOption, txp::BUFFER_HAS_PTR_TO_DATA);
                    break;
                case txp::PTR_INT64:
                    oAttribute = new Attr_int64(pBuffer, pOffset, pOption, txp::BUFFER_HAS_PTR_TO_DATA);
                    break;
                case txp::PTR_CHAR_ARRAY:
                    oAttribute = new AttrPtr_char_array(pBuffer, pOffset, pOption, txp::BUFFER_HAS_PTR_TO_DATA);
                    break;
                case txp::PTR_ARRAY_OF_CHAR_ARRAYS:
                    oAttribute = new AttrPtr_array_of_char_arrays(pBuffer, pOffset, pOption, txp::BUFFER_HAS_PTR_TO_DATA);
                    break;

                default:
                    l_RC = -422;
                    break;
            }
            break;

        default:
            l_RC = -423;
            break;
    }

    if (l_RC == 0) {
        pLength = pBuffer->getAttrLength(pOffset);
        oAttribute->setAllocatedFlag(1);

    	if (pCRC) {
    		//NOTE:  We expect no pointers in the heap buffer.  This should be the buffer that
    		//       received the data and the data should be 'inline'...  @@DLH
//			printf("Attribute::buildAttr: Before: pCRC=%p, *pCRC=0x%016lX\n", pCRC, *pCRC);

    		size_t l_Length = LENGTH_OF_ATTR_TYPE + LENGTH_OF_ATTR_NAME;
			if (l_Type == txp::CHAR_ARRAY) {
				l_Length += LENGTH_OF_ATTR_CHAR_ARRAY_LENGTH;
			} else {
				if (l_Type == txp::ARRAY_OF_CHAR_ARRAYS) {
					l_Length += (LENGTH_OF_ATTR_ARRAY_OF_CHAR_ARRAYS_NUMBER_OF_ELEMENTS + LENGTH_OF_ATTR_ARRAY_OF_CHAR_ARRAYS_LENGTH);
				}
			}
			pBuffer->calculateCRC_Value(*pCRC, pOffset, l_Length);
//			printf("Attribute::buildAttr: After common: pOffset=%d, l_Length=%d, pCRC=%p, *pCRC=0x%016lX\n", pOffset, l_Length, pCRC, *pCRC);

			pBuffer->calculateCRC_Value(*pCRC, pOffset+l_Length, pLength-l_Length);
//			printf("Attribute::buildAttr:   After data: pOffset=%d, l_Length=%d, pCRC=%p, *pCRC=0x%016lX\n", pOffset+l_Length, pLength-l_Length, pCRC, *pCRC);
    	}
    } else {
        if (oAttribute) {
            delete oAttribute;
			oAttribute = 0;
        }
    }

    return l_RC;
}
