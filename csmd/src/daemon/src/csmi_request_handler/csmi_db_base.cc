/*================================================================================
   
    csmd/src/daemon/src/csmi_request_handler/csmi_db_base.cc

  © Copyright IBM Corporation 2015-2019. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
 
================================================================================*/
#include "csmi_db_base.h"
#include "include/csm_event_type_definitions.h"


csm::daemon::NetworkEvent* CSMI_DB_BASE::CheckDBConnAndCreateSqlStmt(const csm::daemon::CoreEvent &aEvent,
                                                        std::string &sqlStmt, bool isPrivateCheck,
                                                        bool compareDataForPrivateCheckRes)
{
  csm::daemon::NetworkEvent *ev = (csm::daemon::NetworkEvent *)&aEvent;
  csm::network::MessageAndAddress reqContent = ev->GetContent();
    
  // decode the Data (arguments) and construct the sql stmt for node_attributes_query
  int errcode=csm_enum_max(csmi_cmd_err_t);
  std::string errmsg;

  bool isSuccess=HasValidDBConn();
  if ( !isSuccess ) {
    errcode = CSMERR_DAEMON_DB_ERR;
    errmsg.append("CSM ERROR - Database Error in Local Daemon");
  } else {
    if (isPrivateCheck)
    {
      isSuccess = RetrieveDataForPrivateCheck(reqContent._Msg.GetData(), reqContent._Msg.GetDataLen(),
                               sqlStmt, errcode, errmsg);
    }
    else
    {
      isSuccess = CreateSqlStmt(reqContent._Msg.GetData(), reqContent._Msg.GetDataLen(), sqlStmt,
                               errcode, errmsg, compareDataForPrivateCheckRes);
    }
    
  }

  csm::daemon::NetworkEvent* errEvent = nullptr;
  if (!isSuccess) // no db conn or no valid sql stmt
  {
    if (isPrivateCheck && errcode == csm_enum_max(csmi_cmd_err_t))
    {
     errEvent = CreateErrorEvent( CSMERR_PERM, "PERMISSION DENIED", aEvent );
    }
    else
    {
      errEvent = CreateErrorEvent(errcode, errmsg, aEvent);
    }
  }
  
  return errEvent;
}


csm::daemon::NetworkEvent *CSMI_DB_BASE::CreateNetworkEventFromDBRespEvent(
                        const csm::daemon::CoreEvent &dbRespEvent,
                        bool compareDataForPrivateCheckRes)
{
  csm::daemon::DBRespEvent *dbevent = (csm::daemon::DBRespEvent *) &dbRespEvent;
  csm::db::DBRespContent dbResp = dbevent->GetContent();
  csm::daemon::CoreEvent *reqEvent = dbevent->GetEventContext()->GetReqEvent();
  if( reqEvent == nullptr )
  {
    LOG( csmapi, error ) << _cmdName << "::CreateNetworkEventFromDBRespEvent(): DBResponse::_ReqEvent is null.";
    return nullptr;
  }
  if ( reqEvent && !isNetworkEvent(*reqEvent) )
  {
    LOG(csmapi, error) << _cmdName << "::CreateNetworkEventFromDBRespEvent(): Expected NetworkEvent in the DBResponse::_ReqEvent";
    delete reqEvent;
    return nullptr;
  }
  csm::db::DBResult_sptr dbRes = dbResp.GetDBResult();

  // convert the db response to byte array
  std::vector<csm::db::DBTuple *> tuples;
  // tuples will be empty if error in DBResult

  //dbRes has to be a non-null ptr and contains valid db result here
  if ( !GetTuplesFromDBResult(dbRes, tuples) )
    LOG(csmapi, error) << "CSMI_DB_BASE::CreateNetworkEventFromDBRespEvent: GetTuplesFromDBResult returned false";

  // will call CreateByteArray anyway even if the tuples.size == 0
  // The CreateByteArray() has to make a call whether it still needs to put data
  // in the payload when tuples.size == 0
  char *buf=nullptr;
  uint32_t bufLen=0;

	// size_t answer = dbRes->GetNumOfAffectedRows();
	// LOG(csmapi, error) << "===================================================================";
	// puts("############################################################################");
	// printf("answer: %lu \n", answer);
	// if(answer == 0){
		// puts("it's zero");
	// }

  int errcode = CreateByteArray(tuples, &buf, bufLen, compareDataForPrivateCheckRes);

  for (uint32_t i=0;i<tuples.size();i++) csm::db::DB_TupleFree(tuples[i]);

  csm::daemon::NetworkEvent *replyEvent = CreateReplyNetworkEvent(buf, bufLen, *reqEvent, nullptr, errcode != CSMI_SUCCESS);

  LOG(csmapi, debug) << "CSMI_DB_BASE::CreateNetworkEventFromDBRespEvent(): NumTuples = " << tuples.size() << " errcode = " << errcode;

  // free memory
  if (buf) free(buf);

  // to-confirm: reqEvent is a shared pointer. No need to touch it..
  //delete reqEvent;

  return replyEvent;
}

void CSMI_DB_BASE::Process( const csm::daemon::CoreEvent &aEvent, std::vector<csm::daemon::CoreEvent*>& postEventList )
{
  // set up the context
  EventContextDBBase_sptr ctx;
  csm::daemon::EventContext_sptr gen_ctx = aEvent.GetEventContext();
  if( ! gen_ctx )
  {
    ctx = std::make_shared<EventContextDBBase>(this, INITIAL_STATE, CopyEvent(aEvent));
    if( ! ctx )
      throw csm::daemon::Exception("ERROR: Context type cannot be created.");
  }
  else
  {
    ctx = std::dynamic_pointer_cast<EventContextDBBase> (aEvent.GetEventContext() );
    if( ! ctx )
      throw csm::daemon::Exception("ERROR: Context type cannot be dyn-casted to EventContextDBBase.");
  }

  LOG(csmd, info) << _cmdName << "::Process(): mystages = " << ctx->GetAuxiliaryId() << "....";
  if (ctx->GetAuxiliaryId() == INITIAL_STATE)
  {
      if ( !isNetworkEvent(aEvent))
      {
        LOG(csmd, error) << _cmdName << "::Process(): Expecting a NetworkEvent";
        return;
      }
      
      csm::network::Message msg = GetNetworkMessage(aEvent);
      if ( msg.PrivateRequiresUserCompare() )
      {
        ctx->SetAuxiliaryId(SEND_DB_PRIVATE_CHECK);
      }
      else
      {
        ctx->SetAuxiliaryId(SEND_DB);
      }
  }
  

  bool compareDataForPrivateCheckRes = true;
  switch ( ctx->GetAuxiliaryId() )
  {
    case SEND_DB_PRIVATE_CHECK:
    {
      LOG(csmapi, debug) << _cmdName << " at SEND_DB_PRIVATE_CHECK";
      
      std::string stmt;
      csm::daemon::NetworkEvent* reply = CheckDBConnAndCreateSqlStmt(aEvent, stmt, true );
      // if reply is null, it means we have a valid sql stmt in stmt
      // if reply is not null, send the error network reply event to the requester
      if ( reply == nullptr)
      {
        ctx->SetAuxiliaryId(RCV_DB_PRIVATE_CHECK);
        postEventList.push_back( CreateDBReqEvent(stmt, ctx) );
      }
      else
      {
        ctx->SetAuxiliaryId(DONE);
        postEventList.push_back(reply);
      }
      break;
    }
    case RCV_DB_PRIVATE_CHECK:
    {
      if ( !isDBRespEvent(aEvent) )
      {
        LOG(csmd, error) << _cmdName << "::Process(): Expecting a DBRespEvent";
        break;
      }
      LOG(csmapi, debug) << _cmdName << " at RCV_DB_PRIVATE_CHECK";

      int errcode;
      std::string errmsg;
      if ( !InspectDBResult(aEvent, errcode, errmsg) )
      {
        ctx->SetAuxiliaryId(DONE);
        if (ctx->GetReqEvent()) postEventList.push_back( CreateErrorEvent(errcode, errmsg, *(ctx->GetReqEvent())) );
        LOG(csmd, error) << _cmdName << "::Process(): DB errcode=" << errcode << " " << errmsg;
        // has to break here as error occurs
        break;
      }
      else
      {
        csm::db::DBRespContent dbResp = GetDBRespContent(aEvent);
        compareDataForPrivateCheckRes = CompareDataForPrivateCheck( dbResp, GetNetworkMessage( *(ctx->GetReqEvent()) ) );
        ctx->SetCompareDataForPrivateCheck(compareDataForPrivateCheckRes);
        // no break here. continue to transit to SEND_DB
      }
    }
    case SEND_DB:
    {
      LOG(csmapi, debug) << _cmdName << " at SEND_DB";

      std::string stmt;
      csm::daemon::NetworkEvent* reply = CheckDBConnAndCreateSqlStmt(*(ctx->GetReqEvent()), stmt, false, compareDataForPrivateCheckRes);
      // if reply is null, it means we have a valid sql stmt in stmt
      // if reply is not null, send the error network reply event to the requester
      if ( reply == nullptr)
      {
        ctx->SetAuxiliaryId(RCV_DB);
        postEventList.push_back( CreateDBReqEvent(stmt, ctx) );
      }
      else
      {
        ctx->SetAuxiliaryId(DONE);
        postEventList.push_back(reply);
      }
        
      break;
    }
          
    case RCV_DB:
    {
      if ( !isDBRespEvent(aEvent) )
      {
        LOG(csmd, error) << _cmdName << "::Process(): Expecting a DBRespEvent";
        break;
      }
      LOG(csmapi, debug) << _cmdName << " at RCV_DB; compareDataForPrivateCheckRes = " << ctx->GetCompareDataForPrivateCheck() ;

      int errcode;
      std::string errmsg;
      if ( !InspectDBResult(aEvent, errcode, errmsg) )
      {
        if (ctx->GetReqEvent()) postEventList.push_back( CreateErrorEvent(errcode, errmsg, *(ctx->GetReqEvent())) );
      
        LOG(csmd, error) << _cmdName << "::Process(): DB errcode=" << errcode << " " << errmsg;
      }
      else
      {
        postEventList.push_back( CreateNetworkEventFromDBRespEvent(aEvent, ctx->GetCompareDataForPrivateCheck()) );
      }
        
      ctx->SetAuxiliaryId(DONE);
      break;
    }

    
    default:
    {
      LOG(csmd, error ) << "DB_BASE API handler entered with context in invalid state: " << ctx->GetAuxiliaryId();
      break;
    }
      
  }

}

#if 0 //!!! OBSOLETE FUNCTION !!!
void CSMI_DB_BASE::Process( const csm::daemon::CoreEvent &aEvent, std::vector<csm::daemon::CoreEvent*>& postEventList )
{
  enum mystages
  {
    send_db_req = 0,
    receive_db_reply = 1,
    
  };
  LOG(csmd, info) << _cmdName << "::Process()....";
  
  // set up the context
  csm::daemon::EventContext_sptr ctx = aEvent.GetEventContext();
  // set the AuxiliaryId to 0
  if (ctx == nullptr) ctx = CreateContext(aEvent, this, 0);

  csm::daemon::NetworkEvent *reply = nullptr;
  csm::daemon::DBReqEvent *dbreply = nullptr;
  switch (ctx->GetAuxiliaryId())
  {
    case send_db_req:
    {
      if ( !isNetworkEvent(aEvent))
      {
        LOG(csmd, error) << _cmdName << "::Process(): Expecting a NetworkEvent";
        break;
      }
    
      std::string stmt;
      reply = CheckDBConnAndCreateSqlStmt(aEvent, stmt);
      // if reply is null, it means we have a valid sql stmt in stmt
      // if reply is not null, send the error network reply event to the requester
      if ( reply == nullptr)
      {
        ctx->SetAuxiliaryId(receive_db_reply);
        dbreply = CreateDBReqEvent(stmt, ctx);
      }
      break;
    }
    case receive_db_reply:
    {
      if ( !isDBRespEvent(aEvent) )
      {
        LOG(csmd, error) << _cmdName << "::Process(): Expecting a DBRespEvent";
        break;
      }
    
      int errcode;
      std::string errmsg;
      if ( !InspectDBResult(aEvent, errcode, errmsg) )
      {
        if (ctx->GetReqEvent()) reply = CreateErrorEvent(errcode, errmsg, *(ctx->GetReqEvent()));
      
        LOG(csmd, error) << _cmdName << "::Process(): DB errcode=" << errcode << " " << errmsg;
      }
      else
        reply = CreateNetworkEventFromDBRespEvent(aEvent);
      break;
    }
    default:
      LOG(csmapi, error) << _cmdName << "::Process(): Unexpected stage";
      break;
  }
  
  if (reply || dbreply)
  {
    if (reply)
      postEventList.push_back(reply);
    if (dbreply)
      postEventList.push_back(dbreply);
  }
  else LOG(csmapi, error) << _cmdName << "::Process(): Fail to post a Event";
}
#endif
