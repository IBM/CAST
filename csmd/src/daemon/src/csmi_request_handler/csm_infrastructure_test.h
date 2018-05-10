/*================================================================================
   
    csmd/src/daemon/src/csmi_request_handler/csm_infrastructure_test.h

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
 
================================================================================*/
#ifndef __CSM_INFRASTRUCTURE_TEST_UPDATE_H__
#define __CSM_INFRASTRUCTURE_TEST_UPDATE_H__
#include <set>
#include <mutex>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <iostream>
#include <sstream>

#include "include/csm_healthcheck_data.h"

#include "csmi_base.h"
#include "csmi_db_base.h"
#include "include/csm_core_event.h"

#include "csmi/include/csm_api_consts.h"

#include "csmutil/include/csm_version.h"

class EventContextTest : public csm::daemon::EventContext {

public:
   EventContextTest(void *aEventHandler, uint64_t aUid, csm::daemon::CoreEvent *aReqEvent = nullptr)
   : csm::daemon::EventContext(aEventHandler, aUid, aReqEvent)
   { }

   EventContextTest(const csm::daemon::EventContext_sptr aCtx)
   : csm::daemon::EventContext(aCtx->GetEventHandler(), aCtx->GetAuxiliaryId(), aCtx->GetReqEvent())
   { }
   
   virtual ~EventContextTest()
   { }

   // make sure we report AND KEEP the very first error message
   void SetErrorMsg( const std::string i_Msg )
   {
     std::lock_guard<std::mutex> guard( _Lock );
     if( _cached_data._errmsg.empty() )
       _cached_data._errmsg = i_Msg;
   }

public:
   HealthCheckData _cached_data;
   mutable std::mutex _Lock;
};

class EventContextTestMaster : public EventContextTest {

public:
   EventContextTestMaster(void *aEventHandler, uint64_t aUid, csm::daemon::CoreEvent *aReqEvent = nullptr)
   : EventContextTest(aEventHandler, aUid, aReqEvent)
   {
     _state = 0;
     _NumCNs = 0;
     _RecvCNs = 0;
     _dbConnection = nullptr;
   }

   EventContextTestMaster(const csm::daemon::EventContext_sptr aCtx)
   : EventContextTest(aCtx->GetEventHandler(), aCtx->GetAuxiliaryId(), aCtx->GetReqEvent())
   {
     _state = 0;
     _NumCNs = 0;
     _RecvCNs = 0;
     _dbConnection = nullptr;
   }
   
   virtual ~EventContextTestMaster()
   { }

   inline void SetState(const uint64_t aState) { _state = aState; }
   inline uint64_t GetState() const { return _state; }
 
   inline void SetNumCNs(size_t aNum) { _NumCNs = aNum; }
   inline size_t GetNumCNs() const { return _NumCNs; }

   inline void IncrementRecvCNs() { _RecvCNs++; }
   inline size_t GetRecvCNs() const { return _RecvCNs; }

   inline void SetDBConnection( const csm::db::DBConnection_sptr i_Conn )
   {
     std::lock_guard<std::mutex> guard( _Lock );
     _dbConnection = i_Conn;
   }
   inline csm::db::DBConnection_sptr GetDBConnection() const
   {
     std::lock_guard<std::mutex> guard( _Lock );
     return _dbConnection;
   }

   // initialize a list of Utility node address codes that are expected to respond
   // after init, this is contains the utility node address codes that haven't responded yet
   void InitUtilList( const std::set<csm::network::AddressCode> i_UtilInfo );

   // Remove the Utility entry with i_AddrCode from the list of expected utility nodes
   // returns the number of remaining entries after removal
   // throws std::out_of_range exception if the entry is not in the set
   int ReceivedUtilInfo( const csm::network::AddressCode i_AddrCode );

   std::set<csm::network::AddressCode>& GetRemainingUtility()
   {
     return _UtilExpect;
   }

   // initialize a list of Aggregator address codes that are expected to respond
   // after init, this is contains the aggregator address codes that haven't responded yet
   void InitAggList( const std::set<csm::network::AddressCode> i_AggInfo );

   // Remove the Aggregator entry with i_AddrCode from the list of expected aggregators
   // returns the number of remaining entries after removal
   // throws std::out_of_range exception if the entry is not in the set
   int ReceivedAggInfo( const csm::network::AddressCode i_AddrCode );

   std::set<csm::network::AddressCode>& GetRemainingAggregator()
   {
     return _AggExpect;
   }


private:
   // state to keep track of multiple stages
   std::atomic<uint64_t> _state;
   csm::db::DBConnection_sptr _dbConnection;

   // list of expected Utility node responses
   std::set<csm::network::AddressCode> _UtilExpect;
   std::set<csm::network::AddressCode> _AggExpect;

   
public:
   // number of the nodes in the multi-cast msg
   std::atomic<size_t> _NumCNs;
   // number of the received msg from CNs
   std::atomic<size_t> _RecvCNs;
   // stored time stamp to check if timer event works properly
   std::chrono::time_point<std::chrono::system_clock> _timer_reference;
};


class EventContextTestAgg : public EventContextTest {

public:
   EventContextTestAgg(void *aEventHandler, uint64_t aUid, csm::daemon::CoreEvent *aReqEvent = nullptr)
   : EventContextTest(aEventHandler, aUid, aReqEvent)
   {
     _NumCNs = 0;
     _RecvCNs = 0;
   }

   EventContextTestAgg(const csm::daemon::EventContext_sptr aCtx)
   : EventContextTest(aCtx->GetEventHandler(), aCtx->GetAuxiliaryId(), aCtx->GetReqEvent())
   {
     _NumCNs = 0;
     _RecvCNs = 0;
   }
   
   virtual ~EventContextTestAgg()
   { }
   
   void SetNumCNs(size_t aNum) { _NumCNs = aNum; }
   size_t GetNumCNs() const { return _NumCNs; }
   
   void SetComputeExpect( const bool i_Primary,
                          const csm::daemon::AddressListType i_Computes )
   {
     if( i_Primary )
       _PrimaryComputeExpect = std::set<csm::network::Address_sptr>( i_Computes.begin(), i_Computes.end() );
     else
       _SecondaryComputeExpect = std::set<csm::network::Address_sptr>( i_Computes.begin(), i_Computes.end() );
   }
   std::set<csm::network::Address_sptr> GetComputeExpect( const bool i_Primary ) const
   {
     if( i_Primary )
       return _PrimaryComputeExpect;
     else
       return _SecondaryComputeExpect;
   }

   bool IncrementRecvCNs( const csm::network::Address_sptr i_Addr )
   {
     // potential data race condition. Guarded with mutex
     std::lock_guard<std::mutex> guard(_Lock);
     try
     {
       if( _PrimaryComputeExpect.find( i_Addr ) != _PrimaryComputeExpect.end() )
       {
         _PrimaryComputeExpect.erase( i_Addr );
         _RecvCNs++;
       }
     }
     catch( std::exception &e )
     {
       LOG( csmd, debug ) << "The address: " << i_Addr->Dump() << " is not listed as PRIMARY.";
     }
     try
     {
       if( _SecondaryComputeExpect.find( i_Addr ) != _SecondaryComputeExpect.end() )
       {
         _SecondaryComputeExpect.erase( i_Addr );
         _RecvCNs++;
       }
     }
     catch( std::exception &e )
     {
       LOG( csmd, debug ) << "The address: " << i_Addr->Dump() << " is not listed as SECONDARY.";
     }
     if( _RecvCNs > _NumCNs )
     {
       LOG( csmd, error ) << "INFRASTRUCTURE_TEST: Received responses " << _RecvCNs << " > Number of active compute nodes " << _NumCNs;
       _RecvCNs = _NumCNs;
     }
     return (_RecvCNs == _NumCNs);
   }
   size_t GetRecvCNs() { return _RecvCNs; }
   
private:
   // expected number of CNs in non-disconnected status
   std::set<csm::network::Address_sptr> _PrimaryComputeExpect;
   std::set<csm::network::Address_sptr> _SecondaryComputeExpect;

   size_t _NumCNs;
   // number of recv CNs so far. note: though the data may be corrupted
   size_t _RecvCNs;
};

typedef std::shared_ptr<EventContextTest> EventContextTest_sptr;
typedef std::shared_ptr<EventContextTestMaster> EventContextTestMaster_sptr;
typedef std::shared_ptr<EventContextTestAgg> EventContextTestAgg_sptr;

class CSM_INFRASTRUCTURE_TEST : public CSMI_BASE {
  
public:
  CSM_INFRASTRUCTURE_TEST(csm::daemon::HandlerOptions &options) : 
    CSMI_BASE(CSM_infrastructure_test, options)
  {
    //RegisterSystemConnectedEvent(this);
    //RegisterSystemDisconnectedEvent(this);
    SetupTimeout();
    
  }
    
  void SetupTimeout()
  {
    //in milliseconds
    _agg_timeout = csm_get_agg_timeout( CSM_infrastructure_test );
    _master_timeout = csm_get_master_timeout( CSM_infrastructure_test );
    _utility_timeout = csm_get_timeout( CSM_infrastructure_test );
  }
  
  HealthCheckData CreateHCDAndSetLDaemon(csm::network::Message &in_out_msg)
  {
    HealthCheckData data;
    CSMI_BASE::ConvertToClass<HealthCheckData>( in_out_msg.GetData(), data );

    std::string hostname = csm::daemon::Configuration::Instance()->GetHostname();    
    data._local = HealthNodeInfo( CSMDaemonRole_to_string( _handlerOptions.GetRole()),
                                  hostname,
                                  std::string(CSM_VERSION, 0, strnlen( CSM_VERSION, 10 )),
                                  0, true );
    data._local.SetDaemonID( csm::daemon::Configuration::Instance()->GetDaemonState()->GetDaemonID() );
    
    in_out_msg.SetData( CSMI_BASE::ConvertToBytes<HealthCheckData>(data) );
    in_out_msg.CheckSumUpdate();
    
    return data;
  }

  virtual void Process( const csm::daemon::CoreEvent &aEvent,
                std::vector<csm::daemon::CoreEvent*>& postEventList ) = 0;
  
protected:
  // a list of tasks we need to test at every daemon
  enum TEST_SETUP
  {
    INITIAL_TEST=0,
    TIMER,
    DBRESP,
    SKIP_DBRESP,
    FLOW,
    MTC,
    FAIL_VCHAN,
    END_TEST
  };

  enum TEST_STATE {
    INITIAL_STATE=0, // must be 0
    WAIT_STATE,
    DONE_STATE
  };
  void TestSystemEvent( const csm::daemon::CoreEvent &aEvent);
  
  uint64_t GetFirstTest() { return _FirstTest; };
  
  uint64_t _FirstTest;
  
  uint64_t _utility_timeout;
  uint64_t _agg_timeout;
  uint64_t _master_timeout;
  std::mutex _Lock;
};

class CSM_INFRASTRUCTURE_TEST_MASTER : public CSM_INFRASTRUCTURE_TEST
{

public:
  CSM_INFRASTRUCTURE_TEST_MASTER(csm::daemon::HandlerOptions &options)
  : CSM_INFRASTRUCTURE_TEST(options)
  {
    _FirstTest = INITIAL_TEST;
  }
  
  void FlowTest( const csm::daemon::CoreEvent &aEvent,
                 std::vector<csm::daemon::CoreEvent*>& postEventList );
  void Process( const csm::daemon::CoreEvent &aEvent,
                std::vector<csm::daemon::CoreEvent*>& postEventList );

private:
  void GenerateMTCMessage( EventContextTestMaster_sptr context,
                           std::vector<csm::daemon::CoreEvent*>& postEventList );
  void RcvMessagesFromAgg( const csm::daemon::CoreEvent &aEvent,
                           std::vector<csm::daemon::CoreEvent*>& postEventList );
  void GenerateReply( EventContextTestMaster_sptr context,
                      std::vector<csm::daemon::CoreEvent*>& postEventList );
  bool NetworkVChannelTest( const uint64_t stage,
                            const EventContextTestMaster_sptr context );

  void UpdateHealthInfo( std::vector<csm::network::AddressCode> &remaining,
                         EventContextTestMaster_sptr context );

  void DaemonIDUniqueTest( EventContextTestMaster_sptr context );

};

class CSM_INFRASTRUCTURE_TEST_AGG : public CSM_INFRASTRUCTURE_TEST
{
  enum FORWARDING_STATE
  {
    LOCAL_REQUEST = INITIAL_STATE,
    LOCAL_UNUSED = WAIT_STATE,
    LOCAL_COMPLETE = DONE_STATE,
    LOCAL_RESPONSE
  };

public:
  CSM_INFRASTRUCTURE_TEST_AGG(csm::daemon::HandlerOptions &options)
  : CSM_INFRASTRUCTURE_TEST(options)
  {
    _FirstTest = FLOW;
  }

  void Process( const csm::daemon::CoreEvent &aEvent,
                std::vector<csm::daemon::CoreEvent*>& postEventList );

};

class CSM_INFRASTRUCTURE_TEST_AGENT : public CSM_INFRASTRUCTURE_TEST
{
public:
  CSM_INFRASTRUCTURE_TEST_AGENT(csm::daemon::HandlerOptions &options)
  : CSM_INFRASTRUCTURE_TEST(options)
  {
    _FirstTest = FLOW;
  }

  void Process( const csm::daemon::CoreEvent &aEvent,
                std::vector<csm::daemon::CoreEvent*>& postEventList );

};

class CSM_INFRASTRUCTURE_TEST_UTILITY : public CSM_INFRASTRUCTURE_TEST
{
public:
  CSM_INFRASTRUCTURE_TEST_UTILITY(csm::daemon::HandlerOptions &options)
  : CSM_INFRASTRUCTURE_TEST(options)
  {
    _FirstTest = FLOW;
  }

  void Process( const csm::daemon::CoreEvent &aEvent,
                std::vector<csm::daemon::CoreEvent*>& postEventList );
};

#endif

