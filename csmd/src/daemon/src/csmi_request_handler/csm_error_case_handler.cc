/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/csm_error_case_handler.h

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/


#include "csm_error_case_handler.h"


void CSM_ERROR_CASE_HANDLER::Process(const csm::daemon::CoreEvent &aEvent, std::vector<csm::daemon::CoreEvent*>& postEventList)
{
  std::shared_ptr<EventContextErrorInjection> context;
  csm::daemon::EventContext_sptr gen_ctx = aEvent.GetEventContext();
  if( ! gen_ctx )
  {
    context = std::make_shared<EventContextErrorInjection>(this, INIT, CopyEvent(aEvent));
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
    context = std::dynamic_pointer_cast<EventContextErrorInjection> ( gen_ctx );
    if( ! context )
    {
      postEventList.push_back( CreateErrorEvent(EINVAL,
                                                "ERROR: Invalid context type. Expected EventContextErrorInjection.",
                                                aEvent ) );
      return;
    }
  }
  switch(context->GetAuxiliaryId())
  {
    case INIT:
      if ( isNetworkEvent(aEvent) )
      {
        csm::network::Message msg = GetNetworkMessage(aEvent);

        if( msg.GetErr() )
        {
          LOG( csmd, debug ) << "ERROR INJECTION: detected network problem.";
          postEventList.push_back( CreateErrorEvent(EINVAL,
                                                    "ERROR: Network error detected.",
                                                    aEvent ) );
          break;
        }
        ErrorInjectionData options;
        // catch the largemsg test from client to daemon
        if(( msg.GetDataLen() > 6 ) && ( std::string( msg.GetData(), 0, 6 ) == "NODATA" ))
        {
          LOG( csmd, debug ) << "ERROR INJECTION: Received indicator for LARGEMSG test from client-to-daemon. Length=" << msg.GetDataLen();
          options.SetIntArg( 2 );
          options.SetMode( ErrorInjectionData::LARGEMSG );
          options.SetDrain( false );
        }
        else
          CSMI_BASE::ConvertToClass<ErrorInjectionData>( msg.GetData(), options );

        context->cached_data = options;

        switch( options.GetMode() )
        {
          case ErrorInjectionData::TIMEOUT:
            LOG(csmd,debug) << "ERROR INJECTION: Timeout test... " << options.GetIntArg() << "s";
            context->SetAuxiliaryId(TIMEOUT_RESPONSE);
            postEventList.push_back( CreateTimerEvent( options.GetIntArg() * 1000, context ) );
            break;

          case ErrorInjectionData::DBLOCK:
          {
            LOG(csmd,debug) << "ERROR INJECTION: Locking " << options.GetIntArg() << " connections...";
            csm::db::DBConnection_sptr conn;
            for(int64_t i = 0; i < options.GetIntArg(); ++i)
            {
              conn = AcquireDBConnection();
              if(conn)
              {
                context->conns.push_back(conn);
              }
              else
              {
                LOG(csmd,debug) << "Could not acquire " << i << "th connection";
              }
            }

            if(context->conns.empty())
            {
              context->SetAuxiliaryId(RELEASECONNS);
              postEventList.push_back(CreateTimerEvent(1, context, RELEASECONNS));
            }
            else if(options.drain)
            {
              // send db requests every 500ms and check the state of the connection
              context->SetAuxiliaryId(DBREQ);
              postEventList.push_back(CreateTimerEvent(WAIT, context, DBREQ));
            }
            else
            {
              context->SetAuxiliaryId(RELEASECONNS);
              postEventList.push_back(CreateTimerEvent( 8000, context, RELEASECONNS ));
            }
            break;
          }
          case ErrorInjectionData::LARGEMSG:
          {
            LOG( csmd, debug ) << "ERROR INJECTION: Generating random response message of length: " << options.GetIntArg();
            if( options.GetIntArg() <= 0 )
            {
              postEventList.push_back( CreateErrorEvent(EINVAL,
                                                        "ERROR: Error injection requested negative (or zero) length message size: " + std::to_string( options.GetIntArg() ),
                                                        aEvent ) );
              break;
            }
            std::string largedata( options.GetIntArg(), 0);
            for( uint64_t n = 0; n < (uint64_t)options.GetIntArg(); ++n )
              largedata[n] = (char)(random() % 26) + 97;

            csm::network::Message *resp = new csm::network::Message( msg );
            resp->SetData( largedata );

            postEventList.push_back( CreateNetworkEvent( *resp,
                                                         GetNetworkAddress( aEvent ),
                                                         context )
                                   );
            delete resp;
            break;
          }

          case ErrorInjectionData::LOOP:
          {
            LOG( csmd, debug ) << "ERROR INJECTION: Generating random response message of length: " << options.GetIntArg();
            if( options.GetIntArg() <= 0 )
            {
              postEventList.push_back( CreateErrorEvent(EINVAL,
                                                        "ERROR: Error injection requested negative (or zero) length message size: " + std::to_string( options.GetIntArg() ),
                                                        aEvent ) );
              break;
            }
            size_t len = random() % 1048576 + 1;
            std::string largedata( len, 0);
            for( uint64_t n = 0; n < len; ++n )
              largedata[n] = (char)(random() % 26) + 97;

            csm::network::Message *resp = new csm::network::Message( msg );
            resp->SetData( largedata );

            postEventList.push_back( CreateNetworkEvent( *resp,
                                                         GetNetworkAddress( aEvent ),
                                                         context )
                                   );
            delete resp;
            break;
          }

          default:
            postEventList.push_back( CreateErrorEvent(EINVAL,
                                                      "ERROR: Unknown Error injection type.",
                                                      aEvent ) );
            break;
        }
      }
      break;
    case TIMEOUT_RESPONSE:
      postEventList.push_back( CreateTimeoutNetworkResponse( aEvent, CSMERR_TIMEOUT, "Request timer triggered." ) );
      break;
    case RELEASECONNS:
    {
      if(!context->conns.empty())
      {
        LOG(csmd,debug) << "ERROR INJECTION: Freeing " << context->conns.size() << " connections";
      }
      for(auto& conn : context->conns)
      {
        ReleaseDBConnection(conn);
      }
      context->conns.clear();

      // put this here since there only two exit paths and the other is supposed to timeout
      std::string payload = ConvertToBytes<ErrorInjectionData>(context->cached_data);
      postEventList.push_back(CreateReplyNetworkEvent(payload.c_str(), payload.length(),
                              *(context->GetReqEvent()), context));
      break;
    }

    case DBREQ:
      // make one request
      LOG(csmd,debug) << "ERROR INJECTION: " << context->conns.size() << " db requests left";
      context->dbconn = context->conns.back();
      context->conns.pop_back();

      postEventList.push_back(CreateDBReqEvent("", context, context->dbconn ));

      // in case of no response
      postEventList.push_back( CreateTimerEvent( 5000, context, DBRESP ) );
      context->SetAuxiliaryId(DBRESP);
      break;

    case DBRESP: // check dbconn and wait to make the next request
      if(!isDBRespEvent(aEvent) && !isTimerEvent(aEvent)) break;

      if(isDBRespEvent(aEvent))
      {
        csm::db::DBRespContent dbResp = GetDBRespContent(aEvent);
        csm::db::DBConnection_sptr dbconn = dbResp.GetDBConnection();
        if(nullptr == dbconn)
        {
          LOG(csmd, warning) << "ERROR INJECTION: Reserved DBConnection was lost during request processing";
        }
        else if(dbconn != context->dbconn)
        {
          LOG(csmd, warning) << "ERROR INJECTION: Reserved DBConnection changed during request processing";
        }
        else if(dbconn == context->dbconn)
        {
          LOG(csmd, debug) << "ERROR INJECTION: Reserved DBConnection is the same";
        }
        ReleaseDBConnection(context->dbconn);
        if(context->conns.empty())
        {
          context->SetAuxiliaryId(RELEASECONNS);
          postEventList.push_back(CreateTimerEvent(1, context, RELEASECONNS));
        }
        else
        {
          context->SetAuxiliaryId(DBREQ);
          postEventList.push_back(CreateTimerEvent(WAIT, context, DBREQ));
        }
      }
      else // no db response
      {
        LOG(csmd, error) << "ERROR INJECTION: Hit DB-request timeout";
        ReleaseDBConnection(context->dbconn);
        context->SetAuxiliaryId(RELEASECONNS);
        postEventList.push_back(CreateTimerEvent(1, context, RELEASECONNS));
      }
      break;
    }
}

