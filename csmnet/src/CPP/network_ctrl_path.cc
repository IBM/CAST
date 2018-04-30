/*================================================================================

    csmnet/src/CPP/network_ctrl_path.cc

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#include <logging.h>
#include "address.h"
#include "network_ctrl_path.h"

typedef std::lock_guard<std::mutex> MutexGuard;

csm::network::NetworkCtrlPath::NetworkCtrlPath()
{
}

csm::network::NetworkCtrlPath::~NetworkCtrlPath()
{
}

void
csm::network::NetworkCtrlPath::AddCtrlEvent( const csm::network::NetworkCtrlInfo_sptr i_Info )
{
  MutexGuard guard( _Lock );
  LOG(csmnet, debug ) << "Adding Net Ctrl Event copy";
  _CtrlEvents.push( i_Info );
}

void
csm::network::NetworkCtrlPath::AddCtrlEvent( const csm::network::NetworkCtrlEventType i_Type,
                                             const uint32_t i_MsgId )
{
  MutexGuard guard( _Lock );
  LOG(csmnet, debug ) << "Adding Net Ctrl Event for msgID: " << i_MsgId;
  _CtrlEvents.push( std::make_shared<csm::network::NetworkCtrlInfo>( i_Type, i_MsgId, nullptr ) );
}

void
csm::network::NetworkCtrlPath::AddCtrlEvent( const csm::network::NetworkCtrlEventType i_Type,
                                             const csm::network::Address_sptr i_Address,
                                             const csm::network::VersionStruct *i_VersionData )
{
  MutexGuard guard( _Lock );
  if( i_Address != nullptr )
  {
    LOG(csmnet, debug ) << "Adding Net Ctrl Event " << i_Type << " for address: " << i_Address->Dump();
  }
  else
  {
    LOG(csmnet, debug ) << "Adding Net Ctrl Event " << i_Type << " for address: EMPTY";
  }
  _CtrlEvents.push( std::make_shared<csm::network::NetworkCtrlInfo>( i_Type, 0, i_Address, nullptr, i_VersionData ) );
}

const csm::network::NetworkCtrlInfo_sptr
csm::network::NetworkCtrlPath::GetCtrlEvent()
{
  MutexGuard guard( _Lock );
  if( _CtrlEvents.empty() )
    return nullptr;

  csm::network::NetworkCtrlInfo_sptr ret = _CtrlEvents.front();
  _CtrlEvents.pop();
  LOG(csmnet, debug ) << "Extracting Net Ctrl EventType: " << ret->_Type << " from " << ( ret->_Address != nullptr ? ret->_Address->Dump() : "EMPTY_ADDR" );
  return ret;
}

void
csm::network::NetworkCtrlPath::PushBack( csm::network::NetworkCtrlInfo *i_List )
{
  MutexGuard guard( _Lock );
  for( csm::network::NetworkCtrlInfo *itr = i_List;
      itr != nullptr;
      itr = itr->_Next )
  {
    if( itr->_Address != nullptr )
    {
      LOG(csmnet, debug) << "Appending Action: " << itr->_Type << " msgID: " << itr->_MsgId << " addr: " << itr->_Address->Dump();
    }
    else
    {
      LOG(csmnet, debug) << "Appending Action: " << itr->_Type << " msgID: " << itr->_MsgId << " addr: EMPTY";
    }
    _CtrlEvents.push( std::make_shared<csm::network::NetworkCtrlInfo>(*itr) );
  }
  for( csm::network::NetworkCtrlInfo *itr = i_List;
      itr != nullptr; )
  {
    csm::network::NetworkCtrlInfo *next_itr = itr->_Next;
    delete itr;
    itr = next_itr;
  }

}
