/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/csm_infrastructure_test_master.cc

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
 
================================================================================*/
// implement the CSM api node attributes update command...
//

#include <malloc.h>
#include <unordered_set>

#include "csm_infrastructure_test.h"
#include "include/csm_event_type_definitions.h"

//#define CSM_INFRASTRUCTURE_TEST_PARALLEL


void CSM_INFRASTRUCTURE_TEST_MASTER::Process( const csm::daemon::CoreEvent &aEvent,
                std::vector<csm::daemon::CoreEvent*>& postEventList )
{
  
  if ( isSystemEvent(aEvent) )
  {
    TestSystemEvent(aEvent);
    return;
  }

  EventContextTestMaster_sptr context;
  csm::daemon::EventContext_sptr gen_ctx = aEvent.GetEventContext();
  if( ! gen_ctx )
  {
    context = std::make_shared<EventContextTestMaster>(this, INITIAL_TEST, CopyEvent(aEvent));
    if( ! context )
    {
      postEventList.push_back( CreateErrorEvent(EINVAL,
                                                "ERROR: Context cannot be created.",
                                                aEvent ) );
      return;
    }
  }
  else
  {
    context = std::dynamic_pointer_cast<EventContextTestMaster> ( gen_ctx );
    if( ! context )
    {
      postEventList.push_back( CreateErrorEvent(EINVAL,
                                                "ERROR: Invalid context type. Expected EventContextTestMaster.",
                                                aEvent ) );
      return;
    }
  }
  
  uint64_t testStage = context->GetAuxiliaryId();
  LOG(csmd, debug) << "CSM_INFRASTRUCTURE_TEST_MASTER: Entering handler. TEST_STAGE = " << testStage;
  
  switch (context->GetAuxiliaryId())
  {
    case INITIAL_TEST:
    {
      if ( !isNetworkEvent(aEvent) ) break;
      csm::network::Message msg = GetNetworkMessage(aEvent);
      if( msg.GetCommandType() != CSM_infrastructure_test )
      {
        // create network error event and exit
        LOG( csmd, warning ) << "CSM_INFRASTRUCTURE_TEST_MASTER: Entering INITIAL state with an invalid command: " << msg.GetCommandType()
            << ". Could be a late-arrival from a previous test after context got deleted. Will not generate a response.";
        break;
      }

      HealthCheckData data;
      data = CreateHCDAndSetLDaemon(msg);
      data._master = HealthNodeInfo( CSMDaemonRole_to_string( CSM_DAEMON_ROLE_MASTER ),
                                     csm::daemon::Configuration::Instance()->GetHostname(),
                                     std::string(CSM_VERSION, 7),
                                     0, true );
      data._master.SetDaemonID( csm::daemon::Configuration::Instance()->GetDaemonState()->GetDaemonID() );
      
#ifdef WITH_MASTER_LOAD_STATS
      if (GetDaemonState()) data._event_load = GetDaemonState()->GetEventLoad();
#endif
      LOG( csmd, debug ) << "CSM_INFRASTRUCTURE_TEST_MASTER: configured " << GetSecurityLevel()
          << ". Calling user:group "
          << " ("<< msg.GetUserID() << ":" << msg.GetGroupID() << ").";
      LOG( csmd, debug ) << "CSM_INFRASTRUCTURE_TEST_MASTER: Point of Entry marked request to compare user data: " << msg.GetPrivateCheck();

      // example check for other handler implementations
      if( msg.PrivateRequiresUserCompare() )
      {
        LOG( csmd, warning ) << "CSM_INFRASTRUCTURE_TEST_MASTER: Calling user:group "
          << " ("<< msg.GetUserID() << ":" << msg.GetGroupID()
          << ") has NON-PRIVILEGED access while API configured PRIVATE. Defined action for this API is: PERMISSION DENIED error.";

        postEventList.push_back( CreateErrorEvent( GetCmdErr(EPERM), "PERMISSION DENIED", *( context->GetReqEvent() ) ) );
        return;
      }

      data._user_access_test = true;   // successful configuration as public indicates that the authentication works


      data._db_free_pool_size =  GetNumOfFreeDBConnections();
      data._db_locked_pool_size = GetNumOfLockedDBConnections();
      context->_cached_data = data;

      // set up the timer test
      context->SetAuxiliaryId(TIMER);
      postEventList.push_back( CreateTimerEvent( 50, context, TIMER ) );
      context->_timer_reference = std::chrono::system_clock::now() + std::chrono::milliseconds( 50 );
      
      break;
    }
    case TIMER:
    {
      if ( !isTimerEvent(aEvent) ) break;

      csm::daemon::TimerContent timer_data = dynamic_cast<const csm::daemon::TimerEvent*>(&aEvent)->GetContent();
      
      // make sure the actual timer expiration is about the order of the expected expiration
      int64_t remainingMicro = timer_data.RemainingMicros();
      context->_cached_data._timer_test = ( timer_data.GetEndTime() < context->_timer_reference );
      context->_cached_data._timer_test &= ( remainingMicro > -500000 );
      context->_cached_data._timer_test &= ( timer_data.GetTargetStateId() == TIMER );

      LOG(csmd, debug) << "CSM_INFRASTRUCTURE_TEST_MASTER: TimerEvent interval " << timer_data.GetTimerInterval()
        << " timer triggered correct=" << context->_cached_data._timer_test
        << " remainMuSec=" << remainingMicro
        << " (" << timer_data.GetEndTime().time_since_epoch().count()
        << " < " << context->_timer_reference.time_since_epoch().count() << ")";

      csm::db::DBConnection_sptr dbConn = AcquireDBConnection();
      if( dbConn == nullptr )
      {
        LOG(csmd, info) << "CSM_INFRASTRUCTURE_TEST_MASTER: Couldn't acquire db connection. Too busy? "
            << "Will go without dedicated connection, but test will fail.";
        context->SetErrorMsg("DB test: cannot acquire db connection. Too busy?");

        // skip the db-response state, there will be nothing to do without db connection
        context->SetAuxiliaryId(SKIP_DBRESP);
        postEventList.push_back( CreateTimerEvent( 1, context, SKIP_DBRESP ) );
        break;
      }
      else
      {
        // set connection and next state
        context->SetDBConnection( dbConn );
        context->SetAuxiliaryId(DBRESP);

        // create a timeout event in case the db doesn't respond.
        postEventList.push_back( CreateTimerEvent( 2000, context, DBRESP ) );

        // Fetch rows from pg_database, the system catalog of databases
        postEventList.push_back( CreateDBReqEvent(std::string("select * from pg_database;"),
                                                  aEvent.GetEventContext(),
                                                  dbConn ) );
        break;
      }
    }
    
    case DBRESP:
    {
      if( !isDBRespEvent( aEvent )  && !isTimerEvent( aEvent ) ) break;

      int errcode;
      std::string errmsg;
      bool ret = false;

      if( isTimerEvent( aEvent ) )
      {
        if( dynamic_cast<const csm::daemon::TimerEvent*>(&aEvent)->GetContent().GetTargetStateId() != DBRESP )
        {
          LOG( csmd, debug ) << "CSM_INFRASTRUCTURE_TEST_MASTER: TimerEvent from different state. Ignoring.";
          break;
        }
        LOG( csmd, info ) << "CSM_INFRASTRUCTURE_TEST_MASTER: Hit DB-request timeout.";
        context->_cached_data._db_query_test = false;
        ReleaseDBConnection( context->GetDBConnection() );
        break;
      }
      else
      {
        ret = InspectDBResult(aEvent, errcode, errmsg);
      }

      if (ret)
      {
        context->_cached_data._db_query_test = true;
        
        // retrieve the tuples using the APIs provided in db.
        // make sure these APIs are memory-leak free!!!
        csm::db::DBRespContent dbResp = GetDBRespContent(aEvent);
        csm::db::DBResult_sptr dbRes = dbResp.GetDBResult();
        std::vector<csm::db::DBTuple *> tuples;
        if (GetTuplesFromDBResult(dbRes, tuples))
        {
          for (uint32_t i=0;i<tuples.size();i++) csm::db::DB_TupleFree(tuples[i]);
        }

        csm::db::DBConnection_sptr dbConn = dbResp.GetDBConnection();

        if( dbConn != context->GetDBConnection() )
        {
          context->_cached_data._db_query_test = false;
          LOG( csmd, info ) << "CSM_INFRASTRUCTURE_TEST_MASTER: reserved DBConnection changed during request processing.";
        }

        if( dbConn != nullptr )
          ReleaseDBConnection( dbConn );
        else
        {
          context->_cached_data._db_query_test = false;
          LOG( csmd, info ) << "CSM_INFRASTRUCTURE_TEST_MASTER: reserved DBConnection lost during request processing.";
        }
      }
      else
      {
        ReleaseDBConnection( context->GetDBConnection() );
        context->_cached_data._db_query_test = false;
        LOG(csmd, info) << "CSM_INFRASTRUCTURE_TEST_MASTER: DBResponse fails. errmsg=" << errmsg;
      }
    }
    // no break in regular processing, this label is only for db errors
    case SKIP_DBRESP:
    {
      // starting to set up the flow test
      context->SetAuxiliaryId(FLOW);
      
      if ( !isNetworkEvent(*(context->GetReqEvent())) )
      {
        LOG(csmd, info) << "CSM_INFRASTRUCTURE_TEST_MASTER: Expecting a NetworkEvent in GetReqEvent. Stopping";
        break;
      }
      
      // start up a timer for the complex flow test
      //postEventList.push_back( CreateTimerEvent( _master_timeout, context ) );
      
      // the original Network Inbound request
      csm::daemon::NetworkEvent *ev = (csm::daemon::NetworkEvent *) context->GetReqEvent();
      if( ev == nullptr )
      {
        LOG(csmd, warning) << "CSM_INFRASTRUCTURE_TEST_MASTER: BUG: orig request not stored in event context. Can't continue and need to rely on timeout.";
        break;
      }
      csm::daemon::NetworkEvent *copyOrigEvent = CreateNetworkEvent(ev->GetContent(), context);
      
      // now we can start the complex flow test...
      // Don't post the event here. Otherwise, it will go out of the network. We just need to
      // trigger the test by calling FlowTest()
      
      // need to set up a user timer here because we expect multiple responses from Aggregators
      // as we don't know how many are there, we simply are setting a timeout
      postEventList.push_back( CreateTimerEvent(_master_timeout, context, FLOW ) );
      
      FlowTest( *copyOrigEvent, postEventList );
      delete copyOrigEvent;
      if( context->GetState() != DONE_STATE )
      {
        break;
      }
      // keep going if the flow test is immediately done (e.g. no known nodes)
    }

    case FLOW:
    {
      // no harm to call twice if previous state already got to done-state
      FlowTest(aEvent, postEventList);
      if (context->GetState() == DONE_STATE)
      {
        LOG(csmd, debug) << "CSM_INFRASTRUCTURE_TEST_MASTER: Done with FLOW test";
        
        // can clear postEventList if it's in DONE_STATE
        postEventList.clear();
        
        // network channel test has 2 phases
        // test will cause a failed recv on vchannel but return via regular event with FAIL_VCHAN state to continue
        postEventList.push_back( CreateTimerEvent( 3000, context, FAIL_VCHAN ) );
        context->_cached_data._net_vchannel_test = NetworkVChannelTest( 0, context );
        context->SetAuxiliaryId( FAIL_VCHAN );
        
#ifndef CSM_INFRASTRUCTURE_TEST_PARALLEL
        break;
#endif
      }
      // allow to come back case FLOW again when more than 1 Agg
      else break;
    }
    
    case FAIL_VCHAN:
    {
      bool skip_test = false;
      if( isTimerEvent( aEvent ) )
      {
        if( dynamic_cast<const csm::daemon::TimerEvent*>(&aEvent)->GetContent().GetTargetStateId() != FAIL_VCHAN )
        {
          LOG( csmd, debug ) << "CSM_INFRASTRUCTURE_TEST_MASTER: TimerEvent from different state. Ignoring.";
          return;
        }

        LOG(csmd, debug ) << "CSM_INFRASTRUCTURE_TEST_MASTER: VCHAN test timed out.";
        context->_cached_data._net_vchannel_test = false;
        skip_test = true;
      }
      // we'll get here by an error path test with send/recv via network channel when recv called after channel released...
      if( ! skip_test )
        context->_cached_data._net_vchannel_test &= NetworkVChannelTest( 1, context );

      int activeCompute = context->_cached_data.GetActiveAgents();
      if ( activeCompute <= 0)
      {
        DaemonIDUniqueTest( context );
        context->SetAuxiliaryId(END_TEST);
        GenerateReply(context, postEventList);

        LOG(csmd, debug) << "CSM_INFRASTRUCTURE_TEST_MASTER: End of All Tests. Skipping MTC test. Compute nodes = " << activeCompute;
      }
      else
      {
        LOG( csmd, debug ) << "CSM_INFRASTRUCTURE_TEST_MASTER: Starting multi-cast test phase for " << activeCompute << " compute nodes.";
        // set up the next test
        context->SetAuxiliaryId(MTC);
        context->SetState(INITIAL_STATE);

        // need to set up a user timer here because we send out one msg but expect 1+ replies
        postEventList.push_back( CreateTimerEvent(_master_timeout, context, MTC) );
        GenerateMTCMessage(context, postEventList);
      }
      
      // need to break here
      break;
    }
    case MTC:
    {
      RcvMessagesFromAgg(aEvent, postEventList);
      
      if (context->GetState() != DONE_STATE) break;

      context->SetAuxiliaryId(END_TEST);
        
      DaemonIDUniqueTest( context );
      GenerateReply(context, postEventList);
      
      LOG(csmd, debug) << "CSM_INFRASTRUCTURE_TEST_MASTER: End of All Tests";
      
      break;
    }
    case END_TEST:
    {
      if( ! isTimerEvent( aEvent ) )
      {
        LOG( csmd, warning ) << "CSM_INFRASTRUCTURE_TEST_MASTER: Hit by non-timer event after completion.";
      }
      else
      {
        LOG( csmd, debug ) << "CSM_INFRASTRUCTURE_TEST_MASTER: Triggered by expected timer after completion.";
      }
      break;
    }
    default:
      LOG(csmd, warning ) << "CSM_INFRASTRUCTURE_TEST_MASTER: At UNKNOWN TEST_SETUP...";
      break;
  }
  LOG(csmd, debug) << "CSM_INFRASTRUCTURE_TEST_MASTER: Leaving handler. TEST_STAGE = " << testStage;
}


bool CSM_INFRASTRUCTURE_TEST_MASTER::NetworkVChannelTest( const uint64_t stage,
                                                          const EventContextTestMaster_sptr context )
{
  bool ret = true;
  LOG( csmd, debug ) << "CSM_INFRASTRUCTURE_TEST_MASTER: NetworkVChannelTest: Starting phase " << stage << " ...";

  csm::daemon::VirtualNetworkChannelPool *Pool = csm::daemon::Configuration::Instance()->GetNetConnectionPool();
  if( Pool == nullptr )
  {
    LOG( csmd, info ) << "CSM_INFRASTRUCTURE_TEST_MASTER: No network channel pool available. Can't continue test.";
    return false;
  }

  switch( stage )
  {
    case 0:
    {
#ifndef CSM_INFRASTRUCTURE_TEST_PARALLEL
      unsigned previous = Pool->GetNumOfFreeNetworkChannels();
      // only test if this is a non-parallel version of the test
      if( previous == 0 )
      {
        LOG( csmd, info ) << "CSM_INFRASTRUCTURE_TEST_MASTER: NetworkVChannelTest: Pool depleted";
        return false;
      }
#endif

      csm::daemon::VirtualNetworkChannel_sptr VChan = Pool->AcquireNetworkChannel();
#ifdef CSM_INFRASTRUCTURE_TEST_PARALLEL
      // retry to get a channel - since we're competing for available channels
      while( VChan == nullptr )
      {
        sched_yield();
        VChan = Pool->AcquireNetworkChannel();
      }
#endif
      if( VChan == nullptr )
      {
        LOG( csmd, debug ) << "CSM_INFRASTRUCTURE_TEST_MASTER: NetworkVChannelTest: Failed to acquire connection.";
        return false;
      }

      csm::network::AddressAbstract_sptr addr = std::make_shared<csm::network::AddressAbstract>(csm::network::ABSTRACT_ADDRESS_SELF);
      csm::network::Message msg;
      msg.Init( CSM_CMD_ECHO, 0, CSM_PRIORITY_DEFAULT, 0, 0x1234, 0x4321, geteuid(), getegid(), "HELLO TO MYSELF.");
      csm::network::MessageAndAddress_sptr msgAddr = std::make_shared<csm::network::MessageAndAddress>( msg, addr );
      csm::network::MessageAndAddress recvMsgAddr;
      uint64_t randomId = random();
      EventContextTestMaster_sptr ctx = std::make_shared<EventContextTestMaster>(this, randomId);

      ssize_t rc = VChan->Send( msgAddr, ctx );

      // VChan->Send() returns payload length, since we set up a msg with payload, it cannot return 0
      if( rc <= 0 )
      {
        LOG( csmd, info ) << "CSM_INFRASTRUCTURE_TEST_MASTER: NetworkVChannelTest: VChan->Send() failed: rc=" << rc;
        ret = false;
      }
      else
      {
        csm::daemon::EventContext_sptr cctx = std::dynamic_pointer_cast<csm::daemon::EventContext>( ctx );
        rc = VChan->Recv( recvMsgAddr,
                          cctx,
                          1000000 );
        if( rc <= 0 )
        {
          LOG( csmd, info ) << "CSM_INFRASTRUCTURE_TEST_MASTER: NetworkVChannelTest: VChan->Recv() failed: rc=" << rc;
          ret = false;
        }
        else
        {
          if( ! (recvMsgAddr._Msg == msgAddr->_Msg) )
          {
            LOG( csmd, info ) << "CSM_INFRASTRUCTURE_TEST_MASTER: NetworkVChannelTest: Failed because outbound and inbound msg differ!";
            ret = false;
          }
          if( ctx == nullptr )
          {
            LOG( csmd, info ) << "CSM_INFRASTRUCTURE_TEST_MASTER: NetworkVChannelTest: Failed because inbound context is null!";
            ret = false;
          }
          else if ( ctx->GetAuxiliaryId() != randomId )
          {
            LOG( csmd, info ) << "CSM_INFRASTRUCTURE_TEST_MASTER: NetworkVChannelTest: Failed because outbound and inbound context differ!";
            ret = false;
          }
        }
      }

      if( ! ret )
        break;

#ifndef CSM_INFRASTRUCTURE_TEST_PARALLEL
      // another send: this time with the actual context of the infrastructure test because the recv is forced to fail
      // and we come back via regular event path, testing the vchannel error handling path
      rc = VChan->Send( msgAddr, context );
      if( rc <= 0 )
      {
        LOG( csmd, info ) << "CSM_INFRASTRUCTURE_TEST_MASTER: NetworkVChannelTest: 2nd VChan->Send() failed: rc=" << rc;
        ret = false;
      }
      else
      {
        Pool->ReleaseNetworkChannel( VChan );
        if( Pool->GetNumOfFreeNetworkChannels() != previous )
        {
          LOG( csmd, info ) << "CSM_INFRASTRUCTURE_TEST_MASTER: NetworkVChannelTest: Failed to returned connection.";
          ret = false;
        }
        else
        {
          csm::daemon::EventContext_sptr cctx = std::dynamic_pointer_cast<csm::daemon::EventContext>( ctx );
          rc = VChan->Recv( recvMsgAddr,
                            cctx,
                            1000000 );
          if( rc > 0 )
          {
            LOG( csmd, info ) << "CSM_INFRASTRUCTURE_TEST_MASTER: NetworkVChannelTest: Error: Successful recv after channel released!";
            ret = false;
          }
          else
          {
            LOG( csmd, info ) << "CSM_INFRASTRUCTURE_TEST_MASTER: NetworkVChannelTest: Expected fail of recv after channel released";
            ret = true;
          }
        }
      }
#else
      // during parallel test, we cannot yet test the release+recv() test to return via event path because
      // the channel might be reserved again by another thread and the recv would succeed
      // + steal the data from the other thread
      Pool->ReleaseNetworkChannel( VChan );
#endif
      break;
    }
    case 1:
#ifndef CSM_INFRASTRUCTURE_TEST_PARALLEL
      LOG( csmd, info ) << "CSM_INFRASTRUCTURE_TEST_MASTER: Successfully returned to VChannel test via regular event.";
#endif
      ret = true;
      break;
    default:
      LOG( csmd, info ) << "CSM_INFRASTRUCTURE_TEST_MASTER: unexpected stage in NetworkVChannelTest.";
      ret = false;
  }
  LOG( csmd, debug ) << "CSM_INFRASTRUCTURE_TEST_MASTER: NetworkVChannelTest: Finish phase " << stage << " status=" << ret;
  return ret;
}



void CSM_INFRASTRUCTURE_TEST_MASTER::GenerateReply(EventContextTestMaster_sptr context,
                        std::vector<csm::daemon::CoreEvent*>& postEventList )
{
  csm::daemon::NetworkEvent *reqEvent = (csm::daemon::NetworkEvent *) context->GetReqEvent();
  
  std::string payload = CSMI_BASE::ConvertToBytes<HealthCheckData>( context->_cached_data );
  
  csm::daemon::NetworkEvent* event = CreateReplyNetworkEvent(payload.c_str(), payload.length(), *reqEvent, context);

  if (event)
  {
    LOG(csmd, debug) << "CSM_INFRASTRUCTURE_TEST_MASTER: Sending a reply back";
    postEventList.push_back(event);
  }
  else LOG(csmd, warning) << "CSM_INFRASTRUCTURE_TEST_MASTER: Fail to create a Reply NetworkEvent";
        
    
}

void CSM_INFRASTRUCTURE_TEST_MASTER::GenerateMTCMessage(EventContextTestMaster_sptr context,
                        std::vector<csm::daemon::CoreEvent*>& postEventList )
{
  HealthCheckData data = context->_cached_data;

  int numCNs = 0;
  std::vector<std::string> cnList;
  numCNs = data.GetActiveAgents(cnList);
  
  context->SetNumCNs(numCNs);
  
  csm::network::Message mtc;
  bool valid = mtc.Init(CSM_TEST_MTC, 0, CSM_PRIORITY_DEFAULT, 0, MASTER, AGGREGATOR, geteuid(), getegid(), std::string(""));
  if (!valid)
  {
    return;
  }

  context->SetState(WAIT_STATE);
  csm::daemon::NetworkEvent *event = CreateMulticastEvent(mtc, cnList, context);
  if (event)
  {
    LOG(csmd, debug) << "CSM_INFRASTRUCTURE_TEST_MASTER: Sending a MTC message with " << numCNs << " nodes";
    postEventList.push_back(event);
  }
}

void CSM_INFRASTRUCTURE_TEST_MASTER::RcvMessagesFromAgg(const csm::daemon::CoreEvent &aEvent,
                std::vector<csm::daemon::CoreEvent*>& postEventList )
{
  EventContextTestMaster_sptr context = std::dynamic_pointer_cast<EventContextTestMaster> (aEvent.GetEventContext());
  
  if (isTimerEvent(aEvent))
  {
    if( dynamic_cast<const csm::daemon::TimerEvent*>(&aEvent)->GetContent().GetTargetStateId() != MTC )
    {
      LOG( csmd, debug ) << "CSM_INFRASTRUCTURE_TEST_MASTER: TimerEvent from different state. Ignoring.";
      return;
    }
    
    LOG(csmd, warning) << "CSM_INFRASTRUCTURE_TEST_MASTER: Hit the user-defined timeout for MTC test! timer = " << GetTimerInterval(aEvent);
    context->SetState(DONE_STATE);
    return;
  }
  
  if ( !isNetworkEvent(aEvent) ) return;
  
  
  if (GetNetworkMessage(aEvent).GetErr())
  {
    LOG(csmd, warning) << "CSM_INFRASTRUCTURE_TEST_MASTER: Hit the ack timeout or encountered network errors!";
    context->SetState(DONE_STATE);
    return;
  }
  
  // todo: potential data race condition. multple replies associated with same context!
  context->IncrementRecvCNs();
  LOG(csmd, debug) << "CSM_INFRASTRUCTURE_TEST_MASTER: Get a reply from a compute node! # of rcv msg=" << context->GetRecvCNs();
    
  if (context->GetNumCNs() ==context->GetRecvCNs())
  {
    context->_cached_data._mtc_test = true;
    context->SetState(DONE_STATE);
    
    LOG(csmd, debug) << "CSM_INFRASTRUCTURE_TEST_MASTER: Get all expected replies from compute nodes! Done with MTC!";
  }
  
}

void CSM_INFRASTRUCTURE_TEST_MASTER::UpdateHealthInfo( std::vector<csm::network::AddressCode> &remaining, EventContextTestMaster_sptr context )
{
  csm::daemon::DaemonStateMaster *dState = dynamic_cast<csm::daemon::DaemonStateMaster*>( GetDaemonState() );

  // then add the list of known disconnected nodes
  csm::daemon::AddressListType disconnected;
  dState->GetAllEPs( disconnected, csm::daemon::ConnectionType::ANY, false );
  for( auto it : disconnected )
  {
    remaining.push_back( dState->GenerateNodeID( it ) );
  }

  // update the node information of the health-check data
  if( remaining.size() > 0 )
  {
    std::lock_guard<std::mutex> guard( context->_Lock );
    for( auto it : remaining )
    {
      csm::daemon::ConnectedNodeStatus *nodeInfo = dState->GetNodeInfo( it );
      if( nodeInfo == nullptr )
        continue;

      LOG( csmd, debug ) << "UpdateNodeStatus: addr=" << nodeInfo->_NodeAddr->Dump()
          << " Type=" << nodeInfo->_NodeAddr->GetAddrType()
          << " ID=" << nodeInfo->_NodeID
          << " code=" << GetDaemonState()->GenerateNodeID( nodeInfo->_NodeAddr );
      nodeInfo->_NodeMode = csm::daemon::RUN_MODE::DISCONNECTED;

      std::string nodeID = nodeInfo->_NodeID;
      if( nodeID.empty() )
        nodeID = nodeInfo->_NodeAddr->Dump();

      switch( nodeInfo->_NodeAddr->GetAddrType() )
      {
        case csm::network::AddressType::CSM_NETWORK_TYPE_AGGREGATOR:
        {
          AggInfo_sptr agg = boost::make_shared<AggInfo>( nodeID,
                                                          "n/a",
                                                          nodeInfo->_Bounced,
                                                          false );
          context->_cached_data._agg_info.push_back( agg );
          break;
        }
        case csm::network::AddressType::CSM_NETWORK_TYPE_UTILITY:
        {
          UtilInfo_sptr util = boost::make_shared<UtilInfo>( nodeID,
                                                             "n/a",
                                                             nodeInfo->_Bounced,
                                                             false );
          context->_cached_data._util_info.push_back( util );
          break;
        }
        default:
          LOG( csmd, warning ) << "Found an unresponsive node that's neither Utility nor Aggregator type.";
      }

    }
  }
}


/*
 *  FLOW TEST
 */
void CSM_INFRASTRUCTURE_TEST_MASTER::FlowTest( const csm::daemon::CoreEvent &aEvent,
                                               std::vector<csm::daemon::CoreEvent*>& postEventList )
{
  EventContextTestMaster_sptr context = std::dynamic_pointer_cast<EventContextTestMaster> (aEvent.GetEventContext());
  if( context == nullptr )
  {
    LOG( csmd, debug ) << "CSM_INFRASTRUCTURE_TEST_MASTER: FlowTest timer: empty context.";
    return;
  }

  csm::daemon::DaemonStateMaster *dState = dynamic_cast<csm::daemon::DaemonStateMaster*>( _handlerOptions.GetDaemonState() );
  
  // wait for some reasonable time for Agg
  if (isTimerEvent(aEvent))
  {
    if( dynamic_cast<const csm::daemon::TimerEvent*>(&aEvent)->GetContent().GetTargetStateId() != FLOW )
    {
      LOG( csmd, debug ) << "CSM_INFRASTRUCTURE_TEST_MASTER: TimerEvent from different state. Ignoring.";
      return;
    }

    if( context->GetState() != WAIT_STATE )
    {
      LOG(csmd, info) << "CSM_INFRASTRUCTURE_TEST_MASTER: FlowTest timer, we have moved on because all daemons responded already... timer = " << GetTimerInterval(aEvent);
      return;
    }

    LOG(csmd, debug) << "CSM_INFRASTRUCTURE_TEST_MASTER: Hit the user-defined timeout for FlowTest! timer = " << GetTimerInterval(aEvent);

    // get list of non-responding nodes
    std::vector<csm::network::AddressCode> remaining;
    std::set_union( context->GetRemainingUtility().begin(), context->GetRemainingUtility().end(),
                    context->GetRemainingAggregator().begin(), context->GetRemainingAggregator().end(),
                    std::back_inserter( remaining ) );

    UpdateHealthInfo( remaining, context );

    context->SetState(DONE_STATE);
    return;
  }
  
  if ( !isNetworkEvent(aEvent) ) return;

  switch ( context->GetState() )
  {
    case INITIAL_STATE:
    {
      csm::daemon::NetworkEvent *event;

      // Got the message from utility. Forward the message to MQTT/aggregator using CSM/MASTER/QUERY topic
      // The context should have the Address Info (i.e. CSM/IP/REQ) of the original request
      context->SetState(WAIT_STATE);
      
      // add the earlier health check in the payload
      csm::network::Message msg = GetNetworkMessage(aEvent);
      msg.SetData( CSMI_BASE::ConvertToBytes<HealthCheckData>(context->_cached_data) );

      // generate a message to all known utility nodes:
      msg.SetMessageID(0);  // make sure this is a new message because it might go back to a Utility where it came from
      msg.CheckSumUpdate();

      csm::daemon::AddressListType nodeList;
      std::set< csm::network::AddressCode > utilNodeSet;
      std::set< csm::network::AddressCode > aggNodeSet;
      unsigned utilNodeCount = 0;
      unsigned aggNodeCount = 0;

      dState->GetAllEPs( nodeList, csm::daemon::ConnectionType::ANY, true );

      LOG( csmd, info ) << "CSM_INFRASTRUCTURE_TEST_MASTER: Contacting " << nodeList.size() << " nodes (util and aggr)";
      for( auto it : nodeList )
      {
        LOG( csmd, debug ) << "CSM_INFRASTRUCTURE_TEST_MASTER: Node=" << it->Dump();

        // only process utility and aggregator nodes here
        if( it->GetAddrType() == csm::network::CSM_NETWORK_TYPE_UTILITY )
        {
          utilNodeSet.insert( dState->GenerateNodeID( it ) );
          ++utilNodeCount;

          if ( (event = CreateNetworkEvent( msg, it, context )) != nullptr )
          {
            LOG(csmd, debug) << "CSM_INFRASTRUCTURE_TEST_MASTER: Request to Utility";
            postEventList.push_back( event );
          }
          else LOG(csmd, warning) << "CSM_INFRASTRUCTURE_TEST_MASTER: Fail to create a NetworkEvent for utility";
        }
        if( it->GetAddrType() == csm::network::CSM_NETWORK_TYPE_AGGREGATOR )
        {
          aggNodeSet.insert( dState->GenerateNodeID( it ) );
          ++aggNodeCount;
        }
      }
      if( utilNodeSet.size() != utilNodeCount )
      {
        LOG(csmd, warning) << "CSM_INFRASTRUCTURE_TEST_MASTER: Inconsistent state: Utility node set and list size don't match.";
        context->SetErrorMsg( std::string("Inconsistent state: Utility node set and list size don't match.") );
        postEventList.clear();
        postEventList.push_back( CreateErrorEvent( EINVAL, "Inconsistent state: Utility node set and list size don't match.", aEvent ) );

        return;
      }
      context->InitUtilList( utilNodeSet );
      if( aggNodeCount > 0 )
      {
        if ( (event = CreateNetworkEvent( msg, _AbstractAggregator,
                                          context )) != nullptr )
        {
          LOG(csmd, debug) << "CSM_INFRASTRUCTURE_TEST_MASTER: Forward a request to Aggregator";
          postEventList.push_back( event );
        }
        else LOG(csmd, warning) << "CSM_INFRASTRUCTURE_TEST_MASTER: Fail to create a NetworkEvent for aggregator";
      }
      context->InitAggList( aggNodeSet );
      LOG( csmd, debug ) << "CSM_INFRASTRUCTURE_TEST_MASTER: Expecting " << utilNodeSet.size() << " Utility and " << aggNodeSet.size() << " Aggregator daemons to respond.";

      if( aggNodeCount + utilNodeCount == 0 )
      {
        // add the disconnected nodes to the health-check info and finish with the FLOW test
        std::vector<csm::network::AddressCode> remaining;
        UpdateHealthInfo( remaining, context );
        context->SetState(DONE_STATE);
        LOG( csmd, debug ) << "CSM_INFRASTRUCTURE_TEST_MASTER: No connected utility or aggregator daemons. FLOW test done.";
      }

      break;
    }
    
    case WAIT_STATE:
    {
      csm::network::Message msg = GetNetworkMessage(aEvent);
      csm::network::Address_sptr addr = GetNetworkAddress(aEvent);

      if (msg.GetErr())
      {
        // either ECOMM or ETIMEOUT
        csmi_err_t *err = csmi_err_unpack(msg.GetData().c_str(), msg.GetDataLen());

        if (err)
        {
          LOG(csmd, warning) << "CSM_INFRASTRUCTURE_TEST_MASTER: Get an Error NetworkEvent. errmsg=" << err->errmsg;
          context->SetErrorMsg( std::string(err->errmsg) );
          csmi_err_free(err);
        }
        else
        {
          context->SetErrorMsg( std::string("Failed to retrieve the error payload" ) );
          LOG(csmd, warning) << "CSM_INFRASTRUCTURE_TEST_MASTER: Fail to unpack a Error NetworkEvent";
        }
      }
      else
      {
        // ATTN: potential data race condition. multple replies associated with same context!
        // make sure the context stays consistent in multi-threaded env

        bool all_aggregators_done = ( context->GetRemainingAggregator().size() == 0);
        bool all_utitity_done = ( context->GetRemainingUtility().size() == 0 );

        // deserialize the received data
        HealthCheckData data;
        CSMI_BASE::ConvertToClass<HealthCheckData>( GetNetworkMessage(aEvent).GetData(), data );
        LOG(csmd, debug) << "CSM_INFRASTRUCTURE_TEST_MASTER: Inbound lists: agg=" << data._agg_info.size() << " util=" << data._util_info.size();

        // if it's from aggregator:
        if( data._agg_info.size() > 0)
        {
          std::vector<AggInfo_sptr> aggInfoList = data._agg_info;

          if( aggInfoList.size() > 1 )
          {
            LOG(csmd, warning) << "CSM_INFRASTRUCTURE_TEST_MASTER: _agg_info from Aggregator should be just one element";
            context->SetErrorMsg( std::string("MASTER: Aggregator info response should be just one element") );
          }
          for( auto agg : aggInfoList )
          {
            LOG(csmd, debug) << "CSM_INFRASTRUCTURE_TEST_MASTER: Got AggInfo from Node " << addr->Dump();
            csm::daemon::ConnectedNodeStatus *nodeInfo = dState->GetNodeInfo( addr );
            if( nodeInfo == nullptr )
              break;

            agg->SetBounced( nodeInfo->_Bounced );
            {
              std::lock_guard<std::mutex> guard( context->_Lock );
              context->_cached_data._agg_info.push_back( agg );
            }
            try
            {
              int remaining = context->ReceivedAggInfo( dState->GenerateNodeID( addr ));
              if( remaining == 0 )
              {
                LOG( csmd, debug ) << "CSM_INFRASTRUCTURE_TEST_MASTER: All Aggregator daemons responded.";
                all_aggregators_done = true;
              }
              else
              {
                LOG( csmd, debug ) << "CSM_INFRASTRUCTURE_TEST_MASTER: Remaining aggergators to respond: " << remaining;
              }
            }
            catch( std::out_of_range &e )
            {
              LOG( csmd, warning ) << "CSM_INFRASTRUCTURE_TEST_MASTER: Response from unknown aggregator node: " << addr->Dump();
              context->SetErrorMsg( std::string("MASTER: Response from unknown aggregator node: ") + addr->Dump() );
            }
          }
        }

        // if it's from utility:
        if( data._util_info.size() > 0 )
        {
          std::vector<UtilInfo_sptr> utilInfoList = data._util_info;

          for( auto util : utilInfoList )
          {
            LOG(csmd, debug) << "CSM_INFRASTRUCTURE_TEST_MASTER: Got UtilInfo from Node " << addr->Dump();
            csm::daemon::ConnectedNodeStatus *nodeInfo = dState->GetNodeInfo( addr );
            if( nodeInfo == nullptr )
              break;

            util->SetBounced( nodeInfo->_Bounced );
            {
              std::lock_guard<std::mutex> guard( context->_Lock );
              context->_cached_data._util_info.push_back( util );
            }
            try
            {
              int remaining = context->ReceivedUtilInfo( dState->GenerateNodeID( addr ));
              if( remaining == 0 )
              {
                LOG( csmd, debug ) << "CSM_INFRASTRUCTURE_TEST_MASTER: All Utility daemons responded.";
                all_utitity_done = true;
              }
              else
              {
                LOG( csmd, debug ) << "CSM_INFRASTRUCTURE_TEST_MASTER: Remaining utility nodes to respond: " << remaining;
              }
            }
            catch( std::out_of_range &e )
            {
              LOG( csmd, warning ) << "CSM_INFRASTRUCTURE_TEST_MASTER: Response from unknown utility node: " << addr->Dump();
              context->SetErrorMsg( std::string("MASTER: Response from unknown utility node: ") + addr->Dump() );
            }
          }
          if( utilInfoList.size() > 1 )
          {
            LOG(csmd, warning) << "CSM_INFRASTRUCTURE_TEST_MASTER: _util_info from Utility should be just one element. Received: " << utilInfoList.size();
            context->SetErrorMsg( std::string("MASTER: Utility info response should be just one element") );
          }

        }

        if(( all_aggregators_done ) && ( all_utitity_done ))
        {
          std::vector<csm::network::AddressCode> remaining;
          UpdateHealthInfo( remaining, context );
          context->SetState(DONE_STATE);
          LOG(csmd, debug) << "CSM_INFRASTRUCTURE_TEST_MASTER: Flow test: All replies received.";
        }
      }

      break;
    }
    default:
      break;
  }
}

typedef std::pair<uint64_t, std::string> daemonID_name_pair_t;

void CSM_INFRASTRUCTURE_TEST_MASTER::DaemonIDUniqueTest( EventContextTestMaster_sptr context )
{
  LOG( csmd, debug ) << "CSM_INFRASTRUCTURE_TEST: DaemonIDUniqueTest";
  std::unordered_set<uint64_t> daemonIDs;
  std::pair< std::unordered_set<uint64_t>::iterator, bool > in;

  uint64_t id = context->_cached_data._master.GetDaemonID();
  if( id != 0 )
  {
    in = daemonIDs.insert( id );
    context->_cached_data._unique_daemon_id_test &= ( in.second == true );
    if( in.second == false )
    {
      context->SetErrorMsg( std::string("Duplicate DaemonID found on host: ")+context->_cached_data._master.GetID() );

    }
  }

  for( auto agg : context->_cached_data._agg_info )
  {
    id = agg->GetDaemonID();
    if( id != 0 )
    {
      in = daemonIDs.insert( id );
      context->_cached_data._unique_daemon_id_test &= ( in.second == true );
      if( in.second == false )
        context->SetErrorMsg( std::string("Duplicate DaemonID found on host: ")+agg->GetID() );
    }
    for( auto compute : agg->_PrimaryComputes )
    {
      id = compute->GetDaemonID();
      if( id != 0 )
      {
        in = daemonIDs.insert( id );
        context->_cached_data._unique_daemon_id_test &= ( in.second == true );
        if( in.second == false )
          context->SetErrorMsg( std::string("Duplicate DaemonID found on host: ")+compute->GetID() );
      }
    }
  }
  for( auto util : context->_cached_data._util_info )
  {
    id = util->GetDaemonID();
    if( id != 0 )
    {
      in = daemonIDs.insert( id );
      context->_cached_data._unique_daemon_id_test &= ( in.second == true );
      if( in.second == false )
        context->SetErrorMsg( std::string("Duplicate DaemonID found on host: ")+util->GetID() );
    }
  }
}
