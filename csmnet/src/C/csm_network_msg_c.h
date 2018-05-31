/*================================================================================

    csmnet/src/C/csm_network_msg_c.h

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#ifndef CSM_NETWORK_SRC_CSM_NETWORK_MSG_C_H_
#define CSM_NETWORK_SRC_CSM_NETWORK_MSG_C_H_

#include <string.h>
#include <errno.h>
#include <stddef.h>
#include <csm_network_header.h>

#define CSM_MIN_ERROR_CODE_LEN 8   // max number of digits when error codes are turned into string

#define CSM_HEADER_SET_ACK( aHeader ) { (aHeader)->_Flags |= CSM_HEADER_ACK_BIT; }
#define CSM_HEADER_CLR_ACK( aHeader ) { (aHeader)->_Flags &= ~CSM_HEADER_ACK_BIT; }
#define CSM_HEADER_GET_ACK( aHeader ) ( (aHeader)->_Flags & CSM_HEADER_ACK_BIT )

#define CSM_HEADER_SET_RESP( aHeader ) { (aHeader)->_Flags |= CSM_HEADER_RESP_BIT; }
#define CSM_HEADER_CLR_RESP( aHeader ) { (aHeader)->_Flags &= ~CSM_HEADER_RESP_BIT; }
#define CSM_HEADER_GET_RESP( aHeader ) ( (aHeader)->_Flags & CSM_HEADER_RESP_BIT )

#define CSM_HEADER_SET_ERR( aHeader ) { (aHeader)->_Flags |= CSM_HEADER_ERR_BIT; }
#define CSM_HEADER_CLR_ERR( aHeader ) { (aHeader)->_Flags &= ~CSM_HEADER_ERR_BIT; }
#define CSM_HEADER_GET_ERR( aHeader ) ( (aHeader)->_Flags & CSM_HEADER_ERR_BIT )

#define CSM_HEADER_SET_CBK( aHeader ) { (aHeader)->_Flags |= CSM_HEADER_CBK_BIT; }
#define CSM_HEADER_CLR_CBK( aHeader ) { (aHeader)->_Flags &= ~CSM_HEADER_CBK_BIT; }
#define CSM_HEADER_GET_CBK( aHeader ) ( (aHeader)->_Flags & CSM_HEADER_CBK_BIT )

#define CSM_HEADER_SET_INT( aHeader ) { (aHeader)->_Flags |= CSM_HEADER_INT_BIT; }
#define CSM_HEADER_CLR_INT( aHeader ) { (aHeader)->_Flags &= ~CSM_HEADER_INT_BIT; }
#define CSM_HEADER_GET_INT( aHeader ) ( (aHeader)->_Flags & CSM_HEADER_INT_BIT )

#define CSM_HEADER_SET_MTC( aHeader ) { (aHeader)->_Flags |= CSM_HEADER_MTC_BIT; }
#define CSM_HEADER_CLR_MTC( aHeader ) { (aHeader)->_Flags &= ~CSM_HEADER_MTC_BIT; }
#define CSM_HEADER_GET_MTC( aHeader ) ( (aHeader)->_Flags & CSM_HEADER_MTC_BIT )

#define CSM_HEADER_SET_PCK( aHeader ) { (aHeader)->_Flags |= CSM_HEADER_PCK_BIT; }
#define CSM_HEADER_CLR_PCK( aHeader ) { (aHeader)->_Flags &= ~CSM_HEADER_PCK_BIT; }
#define CSM_HEADER_GET_PCK( aHeader ) ( (aHeader)->_Flags & CSM_HEADER_PCK_BIT )

#define CSM_HEADER_FLAGS_MASK (CSM_HEADER_ACK_BIT | CSM_HEADER_RESP_BIT | CSM_HEADER_ERR_BIT | \
                               CSM_HEADER_CBK_BIT | CSM_HEADER_INT_BIT | CSM_HEADER_MTC_BIT | \
                               CSM_HEADER_PCK_BIT )

#define CSM_HEADER_CHECKSUM_MAGIC ( 0x2EB0C114 )

#define CSM_LOG_RAW_MSG_LIMIT ( 512 )

/** @def CSM_UNIX_CREDENTIAL_LENGTH
 * @bried shortcut for size of credentials in control msg
 */
#define CSM_UNIX_CREDENTIAL_LENGTH ( CMSG_SPACE( sizeof( struct ucred ) ) )

/** @brief calculates the header checksum over header and data
 *
 * Checksum is calculated over header and data section of the
 * message (excluding the checksum field).
 * Checksum is never 0 unless there's an error.
 * If checksum calculation result is 0, CSM_HEADER_CHECKSUM_MAGIC is returned
 *
 * @param aHeader   pointer to header of message
 * @param aData     pointer to data buffer
 *
 * @return checksum integer
 * @return 0 in case of any errors
 */
static inline
uint32_t csm_header_check_sum( csm_network_header_t const *aHeader,
                               char const *aData )
{
  if( ! aHeader ) return 0;
  if( ( aHeader->_DataLen > 0 ) && ( !aData ) ) return 0;

  uint32_t *Pos = (uint32_t*)aHeader;

  // preset to value of current chksum in header (will be removed by
  // later xor to exclude any existing chksum value from calculation
  uint32_t ChkSum = Pos[ offsetof(csm_network_header_t, _CheckSum) / sizeof( uint32_t ) ];
  size_t DataLen = aHeader->_DataLen;

  size_t i;
  for( i=0; i < sizeof( csm_network_header_t ) / sizeof( uint32_t ); ++i )
    ChkSum ^= Pos[i];

  if( aData )
  {
    Pos = (uint32_t*)aData;
    for( i=0; i<(DataLen / sizeof( uint32_t )); ++i )
      ChkSum ^= Pos[i];

    if( DataLen % sizeof( uint32_t ) )
    {
      uint32_t endval = 0;
      char *endchars = (char*)(&(Pos[ DataLen / sizeof( uint32_t ) ]));
      for( i=0; i < (DataLen % sizeof( uint32_t )); ++i )
        endval = (endval << 8) + (uint32_t)endchars[i];
      ChkSum ^= endval;
    }
  }

  // prevent checksum being 0
  if( ! ChkSum ) ChkSum = 0x2EB0C114;

  return ChkSum;
}


/** @brief validates the header content
 *
 * Checks the fields in the header for any invalid values
 *
 * @param aHeader   pointer to header of message
 *
 * @return 0 if content is not valid
 * @return 1 if content is valid
 */
static inline
int csm_header_validate( csm_network_header_t const *aHeader )
{
  int valid = 1;
  if( ! aHeader ) return 0;
  valid &= (aHeader->_ProtocolVersion == CSM_NETWORK_PROTOCOL_VERSION);
  valid &= (aHeader->_Priority < CSM_NETWORK_PRIORITY_INVALID);
  valid &= ( csmi_cmd_is_valid( aHeader->_CommandType ));
//  valid &= (aHeader->_MessageID != 0); // messageID = 0 causes the network stack to generate a new one
  valid &= (aHeader->_Flags == ( aHeader->_Flags & CSM_HEADER_FLAGS_MASK ) );

  valid &= ( aHeader->_DataLen < DGRAM_PAYLOAD_MAX - sizeof( csm_network_header_t ) );
  valid &= (aHeader->_CheckSum != 0 );
  valid &= (( aHeader->_UserID != CSM_CREDENTIAL_ID_UNKNOWN ) && ( aHeader->_GroupID != CSM_CREDENTIAL_ID_UNKNOWN ));
  // ...
  return valid;
}


typedef struct csm_network_msg
{
  csm_network_header_t _Header;
  char* _Data;
} csm_net_msg_t;

static inline

/** @brief validates the header of a message
 *
 * @param aMsg  pointer to message
 *
 * @return 0 if content is not valid
 * @return 1 if content is valid
 */
int csm_net_msg_Validate( csm_net_msg_t const *aMsg )
{ return csm_header_validate( &(aMsg->_Header) ); }

/** @brief retrieve protocol version field in header of message
 *
 * @param aMsg  pointer to message
 *
 * @return protocol version data (8 bit integer)
 */
static inline
uint8_t csm_net_msg_GetProtocolVersion( csm_net_msg_t const *aMsg )
{ if( ! aMsg ) return 0xFF; return aMsg->_Header._ProtocolVersion; }

/** @brief set protocol version to current implementation
 *
 * @param aMsg  pointer to message
 *
 * @return none
 */
static inline
void csm_net_msg_SetProtocolVersion( csm_net_msg_t *aMsg )
{ if( ! aMsg ) return;  aMsg->_Header._ProtocolVersion = CSM_NETWORK_PROTOCOL_VERSION; }

/** @brief Get the ACK bit setting of the message
 *
 * @param aMsg  pointer to message
 *
 * @return ACK bit
 */
static inline
int csm_net_msg_GetAck( csm_net_msg_t const *aMsg )
{  if( ! aMsg ) return 0; return CSM_HEADER_GET_ACK( &(aMsg->_Header) ); }

/** @brief Set the ACK bit of the message to true
 *
 * @param aMsg  pointer to message
 *
 * @return none
 */
static inline
void csm_net_msg_SetAck( csm_net_msg_t *aMsg )
{ if( ! aMsg ) return; CSM_HEADER_SET_ACK( &(aMsg->_Header) ); }

/** @brief Clear the ACK bit of the message
 *
 * @param aMsg  pointer to message
 *
 * @return none
 */
static inline
void csm_net_msg_ClrAck( csm_net_msg_t *aMsg )
{ if( ! aMsg ) return; CSM_HEADER_CLR_ACK( &(aMsg->_Header) ); }

/** @brief Get the RESP flag of the message
 *
 * @param aMsg  pointer to message
 *
 * @return RESP bit flag
 */
static inline
int csm_net_msg_GetResponseFlag( csm_net_msg_t const *aMsg )
{  if( ! aMsg ) return 0; return CSM_HEADER_GET_RESP( &(aMsg->_Header) ); }

/** @brief Set the RESP flag to true
 *
 * @param aMsg  pointer to message
 *
 * @return none
 */
static inline
void csm_net_msg_SetResponseFlag( csm_net_msg_t *aMsg )
{ if( ! aMsg ) return; CSM_HEADER_SET_RESP( &(aMsg->_Header) ); }

/** @brief clear the RESP flag
 *
 * @param aMsg  pointer to message
 *
 * @return none
 */
static inline
void csm_net_msg_ClrResponseFlag( csm_net_msg_t *aMsg )
{ if( ! aMsg ) return; CSM_HEADER_CLR_RESP( &(aMsg->_Header) ); }

/** @brief Get the ERROR flag
 *
 * @param aMsg  pointer to message
 *
 * @return value of ERROR flag
 */
static inline
int csm_net_msg_GetErrorFlag( csm_net_msg_t const *aMsg )
{  if( ! aMsg ) return 0; return CSM_HEADER_GET_ERR( &(aMsg->_Header) ); }

/** @brief Set the ERROR flag to true
 *
 * @param aMsg  pointer to message
 *
 * @return none
 */
static inline
void csm_net_msg_SetErrorFlag( csm_net_msg_t *aMsg )
{ if( ! aMsg ) return; CSM_HEADER_SET_ERR( &(aMsg->_Header) ); }

/** @brief clear the RESP flag
 *
 * @param aMsg  pointer to message
 *
 * @return none
 */
static inline
void csm_net_msg_ClrErrorFlag( csm_net_msg_t *aMsg )
{ if( ! aMsg ) return; CSM_HEADER_CLR_ERR( &(aMsg->_Header) ); }

/** @brief Get the status of the INTERNAL flag
 *
 * @param aMsg  pointer to message
 *
 * @return  value of INTERNAL flag
 */
static inline
uint8_t csm_net_msg_GetInternalFlag( csm_net_msg_t const *aMsg )
{ if( ! aMsg ) return 0; return CSM_HEADER_GET_INT( &(aMsg->_Header) ); }

/** @brief Set the INTERNAL flag to true
 *
 * @param  aMsg   pointer to message\
 *
 * @return  none
 */
static inline
void csm_net_msg_SetInternalFlag( csm_net_msg_t *aMsg )
{ if( ! aMsg ) return; CSM_HEADER_SET_INT( &(aMsg->_Header) ); }

/** @brief clear the INTERNAL flag
 *
 * @param aMsg  pointer to message
 *
 * @return none
 */
static inline
void csm_net_msg_ClrInternalFlag( csm_net_msg_t *aMsg )
{ if( ! aMsg ) return; CSM_HEADER_CLR_INT( &(aMsg->_Header) ); }

/** @brief Get the flags field of the message
 *
 * @param aMsg  pointer to message
 *
 * @return flags header field
 */
static inline
uint8_t csm_net_msg_GetFlags( csm_net_msg_t const *aMsg )
{ if( ! aMsg ) return 0xFF; return aMsg->_Header._Flags; }

/** @brief Set the flags field of the message
 *
 * @param aMsg  pointer to message
 *
 * @return none
 */
static inline
void csm_net_msg_SetFlags( csm_net_msg_t *aMsg, const uint8_t aFlags )
{ if( ! aMsg ) return; aMsg->_Header._Flags = aFlags; }

/** @brief Get the message priority field
 *
 * @param aMsg   pointer to message
 * @param aFlags new flags field
 *
 * @return priority
 */
static inline
uint8_t csm_net_msg_GetPriority( csm_net_msg_t const *aMsg)
{ if( ! aMsg ) return CSM_NETWORK_PRIORITY_INVALID; return ( aMsg->_Header._Priority ); }

/** @brief Set the message priority field
 *
 * @param aMsg      pointer to message
 * @param aNewPrio  new priority setting
 *
 * @return none
 */
static inline
void csm_net_msg_SetPriority( csm_net_msg_t *aMsg, const uint8_t aNewPrio )
{ if( ! aMsg ) return; aMsg->_Header._Priority = aNewPrio; }

/** @brief Get the command field of the message
 *
 * @param aMsg  pointer to message
 *
 * @return command
 */
static inline
uint8_t csm_net_msg_GetCommandType( csm_net_msg_t const *aMsg)
{ if( ! aMsg ) return CSM_CMD_INVALID; return aMsg->_Header._CommandType; }

/** @brief Set the command field of the message
 *
 * @param aMsg  pointer to message
 * @param aCmd  new command entry
 *
 * @return none
 */
static inline
void csm_net_msg_SetCommandType( csm_net_msg_t *aMsg, const uint8_t aCmd )
{ if( ! aMsg ) return; aMsg->_Header._CommandType = aCmd; }

/** @brief Get the message ID field of the message
 *
 * @param aMsg  pointer to message
 *
 * @return messageID
 */
static inline
uint64_t csm_net_msg_GetMessageID( csm_net_msg_t const *aMsg )
{ if( ! aMsg ) return 0; return aMsg->_Header._MessageID; }

/** @brief Set the message ID field of the message
 *
 * @param aMsg  pointer to message
 *
 * @return none
 */
static inline
void csm_net_msg_SetMessageID( csm_net_msg_t *aMsg, const uint64_t aMsgID )
{ if( ! aMsg ) return; aMsg->_Header._MessageID = aMsgID; }

/** @brief Get the checksum field of the message
 *
 * @param aMsg   pointer to message
 * @param aMsgID new msgID
 *
 * @return checksum
 */
static inline
uint32_t csm_net_msg_GetCheckSum( csm_net_msg_t const *aMsg )
{ if( ! aMsg ) return 0; return aMsg->_Header._CheckSum; }

/** @brief Calculate but not update the message checksum
 *
 * @param aMsg  pointer to message
 *
 * @return checksum value
 */
static inline
uint32_t csm_net_msg_CheckSumCalculate( csm_net_msg_t const *aMsg )
{ if( ! aMsg ) return 0; return csm_header_check_sum( &(aMsg->_Header), aMsg->_Data ); }

/** @brief Calculate and update the message checksum
 *
 * @param aMsg  pointer to message
 *
 * @return checksum
 */
static inline
uint32_t csm_net_msg_CheckSumUpdate( csm_net_msg_t *aMsg )
{ if( ! aMsg ) return 0; aMsg->_Header._CheckSum = csm_net_msg_CheckSumCalculate( aMsg ); return aMsg->_Header._CheckSum; }

/** @brief Get the data length field entry of the message
 *
 * This only equals the actual data length if the message is valid
 *
 * @param aMsg  pointer to message
 *
 * @return data length
 */
static inline
uint32_t csm_net_msg_GetDataLen( csm_net_msg_t const *aMsg )
{ if( ! aMsg ) return 0; return aMsg->_Header._DataLen; }

/** @brief return the UserID of the message
 *
 * @param  aMsg   pointer to message
 *
 * @return UserID
 */
static inline
uint32_t csm_net_msg_GetUserID( csm_net_msg_t const *aMsg )
{ if( ! aMsg ) return CSM_CREDENTIAL_ID_UNKNOWN; return aMsg->_Header._UserID; }

/** @brief set the user ID of the message
 *
 * @param aMsg     pointer to message
 * @param aUserID  new userID to set
 *
 * @return none
 */
static inline
void csm_net_msg_SetUserID( csm_net_msg_t *aMsg, const uint32_t aUserID )
{ if( ! aMsg ) return; aMsg->_Header._UserID = aUserID; }

/** @brief return the GroupID of the message
 *
 * @param  aMsg   pointer to message
 *
 * @return GroupID
 */
static inline
uint32_t csm_net_msg_GetGroupID( csm_net_msg_t const *aMsg )
{ if( ! aMsg ) return CSM_CREDENTIAL_ID_UNKNOWN; return aMsg->_Header._GroupID; }

/** @brief set the GroupID of the message
 *
 * @param aMsg     pointer to message
 * @param aGroupID  new groupID to set
 *
 * @return none
 */
static inline
void csm_net_msg_SetGroupID( csm_net_msg_t *aMsg, const uint32_t aGroupID )
{ if( ! aMsg ) return; aMsg->_Header._GroupID = aGroupID; }

/** @brief get the Reserved field of the message header
 *
 * @param aMsg       pointer to message

 * @return reserved value
 */
static inline
uint32_t csm_net_msg_GetReserved( csm_net_msg_t *aMsg )
{ if( ! aMsg ) return 0; return aMsg->_Header._Reserved; }



/** @brief set the Reserved field of the message header
 *
 * @param aMsg       pointer to message
 * @param aReserved  new reserved value to set
 *
 * @return none
 */
static inline
void csm_net_msg_SetReserved( csm_net_msg_t *aMsg, const uint32_t aReserved )
{ if( ! aMsg ) return; aMsg->_Header._Reserved = aReserved; }

/** @brief return the data buffer of the message
 *
 * @param aMsg  pointer to message
 *
 * @return pointer to data
 */
static inline
const char* csm_net_msg_GetData( csm_net_msg_t const *aMsg )
{ if( ! aMsg ) return NULL; return aMsg->_Data; }

/** @brief Copy aLen bytes of the aData input buffer into the message
 *
 * Checks the existing length of the message buffer and limits the amount of copied data
 * Only the message content changes
 * No changes are made to the address and size of the existing message buffer.
 * Note that a smaller input message leaves dead space at the end of the message buffer.
 *
 * @param aMsg   pointer to message
 * @param aData  input data
 * @param aLen   size of input data to copy
 *
 * @return returns the size of copied data
 */
static inline
ssize_t csm_net_msg_CopyData( csm_net_msg_t *aMsg, const char *aData, const size_t aLen )
{
  if(( ! aMsg ) || ( ! aData ) || ( ! aMsg->_Data )) return -1;
  size_t copylen = ( aMsg->_Header._DataLen < aLen ) ? aMsg->_Header._DataLen : aLen;
  aMsg->_Header._DataLen = copylen;
  memcpy( aMsg->_Data, aData, copylen );
  return copylen;
}

/** @brief Assign the input aData to the message, updates the length but not checksum
 *
 * @param aMsg  pointer to message
 *
 * @return new size of data field (or negative on error)
 */
static inline
ssize_t csm_net_msg_SetData( csm_net_msg_t *aMsg, const char *aData, const size_t aLen )
{
  if( ! aMsg ) return -1;
  aMsg->_Header._DataLen = aLen;
  if( aLen > 0 )
    aMsg->_Data = (char*)aData;
  else
    aMsg->_Data = NULL;
  return csm_net_msg_GetDataLen( aMsg );
}

/** @brief Assign input aData to the message, updates length and checksum
 *
 * @param aMsg  pointer to message
 *
 * @return new size of data field (or negative on error)
 */
static inline
ssize_t csm_net_msg_SetDataAndChksum( csm_net_msg_t *aMsg, const char *aData, const size_t aLen )
{
  if( ! aMsg ) return -1;
  csm_net_msg_SetData( aMsg, aData, aLen );
  csm_net_msg_CheckSumUpdate( aMsg );
  return csm_net_msg_GetDataLen( aMsg );
}

/** @brief Copy the input data into the message and update the checksum
 *
 * @param aMsg  pointer to message
 * @param aData input data
 * @param aLen  size of input data
 *
 * @return new size of data field (or negative on error)
 */
static inline
ssize_t csm_net_msg_CopyDataAndChksum( csm_net_msg_t *aMsg, const char *aData, const size_t aLen )
{
  if( ! aMsg ) return -1;
  if(( aData ) && ( aLen > 0 ))
    if( csm_net_msg_CopyData( aMsg, aData, aLen ) < 0 )
      return -1;
  csm_net_msg_CheckSumUpdate( aMsg );
  return csm_net_msg_GetDataLen( aMsg );
}

/** @brief Returns a pointer to the header buffer (e.g. to copy or send/recv the raw header data)
 *
 * @param aMsg  pointer to message
 *
 * @return char pointer to the header buffer
 */
static inline
char * csm_net_msg_GetHeaderBuffer( csm_net_msg_t const *aMsg )
{ if( ! aMsg ) return NULL; return (char*)&( aMsg->_Header ); }


/** @brief Initialize all fields of a message including data
 *
 * allocates the memory for the message and initializes it including validation
 *
 * @param aCmdType    command entry
 * @param aFlags      flags to set
 * @param aPriority   message priority
 * @param aMessageID  message ID
 * @param aSrcAddr    source address
 * @param aDstAddr    destination address
 * @param aData       data buffer
 * @param aDataLen    data length
 *
 * @return new allocated and initialized message
 * @return NULL if message can't be initialized
 *         errno will be set:
 *            ENOMEM - if memory allocation failed
 *            EINVAL - any invalid data was used to initialize
 */
static inline
csm_net_msg_t * csm_net_msg_Init( const uint8_t aCmdType,
                                  const uint8_t aFlags,
                                  const uint8_t aPriority,
                                  const uint64_t aMessageID,
                                  const uint32_t aUserID,
                                  const uint32_t aGroupID,
                                  const char *aData, const size_t aDataLen,
                                  const uint32_t aReserved )
{
    csm_net_msg_t *msg = (csm_net_msg_t*)malloc( sizeof( csm_net_msg_t ) );
    if( ! msg )
    {
        errno = ENOMEM;
        return NULL;
    }

    memset( (void*)msg, 0, sizeof( csm_net_msg_t ) );
    csm_net_msg_SetProtocolVersion( msg );
    csm_net_msg_SetCommandType( msg, aCmdType );
    csm_net_msg_SetPriority( msg, aPriority );
    csm_net_msg_SetFlags( msg, aFlags );
    csm_net_msg_SetMessageID( msg, aMessageID );
    csm_net_msg_SetUserID( msg, aUserID );
    csm_net_msg_SetGroupID( msg, aGroupID );
    csm_net_msg_SetReserved( msg, aReserved );

    if( aData )
        csm_net_msg_SetDataAndChksum( msg, aData, aDataLen );
    else
        csm_net_msg_SetDataAndChksum( msg, NULL, 0 );

    // validate and return allocated msg
    errno = 0;
    if( csm_header_validate( &(msg->_Header) ) )
        return msg;

    // otherwise free
    free(msg);
    errno = EINVAL;

    return NULL;
}

/** @brief Initializes the header of a message from a buffer
 *
 * @param aMsg    pointer to message
 * @param aInput  input buffer that contains a header
 *
 * @return 0  if successfully initialized and validated
 * @return -1 otherwise
 */
static inline
int csm_net_msg_InitHdr( csm_net_msg_t *aMsg,
                         const char * aInput )
{
  if( ! aMsg ) return -1;
  csm_network_header_t *hdr = (csm_network_header_t*)aInput;
  if( csm_header_validate( hdr ) )
    // todo LS: memcpy is not robust against architecture differences
    memcpy( &(aMsg->_Header), hdr, sizeof( csm_network_header_t ) );
  else
    return -1;

  return 0;
}


/** @brief Checks whether the settings in header require an ACK to be send
 *
 * @param aMsg    pointer to header
 *
 * @return 1 - ACK required
 * @return 0 - ACK not required
 */
static inline
int csm_net_msg_RequiresAck( const csm_network_header_t *aHdr )
{
  return ( ( CSM_HEADER_GET_ACK( aHdr ) == 0 ) && ( aHdr->_Priority > CSM_PRIORITY_NO_ACK ) );
}


/** @brief print message content on stdout
 *
 * @param aMsg  pointer to message
 *
 * @return 0 always
 */
static inline
int csm_net_msg_Dump( csm_net_msg_t *aMsg )
{
  printf("Dumping Msg: @0x%p\n", aMsg);
  if( aMsg == NULL ) return 0;

  csm_network_header_t *hdr = &(aMsg->_Header);
  printf("\t v=%d; cmd=%d; fl=0x%x; p=%d; id=%ld; len=%d; cs=0x%x; u:gid=%d:%d\n",
         hdr->_ProtocolVersion, hdr->_CommandType, hdr->_Flags,
         hdr->_Priority, hdr->_MessageID, hdr->_DataLen,
         hdr->_CheckSum,
         hdr->_UserID, hdr->_GroupID );
  return 0;
}


#endif /* CSM_NETWORK_SRC_CSM_NETWORK_MSG_C_H_ */
