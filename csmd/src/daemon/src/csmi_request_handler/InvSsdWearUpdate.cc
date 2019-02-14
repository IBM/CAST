/*================================================================================
   
    csmd/src/daemon/src/csmi_request_handler/InvSsdWearUpdate.cc

  © Copyright IBM Corporation 2015-2019. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
 
================================================================================*/

#include "InvSsdWearUpdate.h"
#include "include/csm_event_type_definitions.h"

#ifndef logprefix
#define logprefix "SSD_WEAR"
#define logprefix_local
#endif
#include "csm_pretty_log.h"

#include "csmd/src/inv/include/inv_ssd_wear_serialization.h"
#include "csmi/include/csm_api_consts.h"

#include <sstream>

using std::string;
using std::to_string;
using std::endl;

void InvSsdWearUpdate::Process(const csm::daemon::CoreEvent &aEvent, std::vector<csm::daemon::CoreEvent*>& postEventList)
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

   LOG(csmd, debug) << _cmdName << "::Process: mystages = " << ctx->GetAuxiliaryId() << "....";
   if (ctx->GetAuxiliaryId() == INITIAL_STATE)
   {
      if ( !isNetworkEvent(aEvent) )
      {
         LOG(csmd, error) << _cmdName << "::Process: Expecting a NetworkEvent";
         return;
      }
      
      csm::network::Message msg = GetNetworkMessage(aEvent);
      ctx->SetAuxiliaryId(SEND_DB);
   }

   switch ( ctx->GetAuxiliaryId() )
   {
      case SEND_DB:
      {
         LOG(csmd, debug) << _cmdName << "::Process: at SEND_DB";
         bool create_db_success(false);

         ctx->SetAuxiliaryId(RCV_DB);
         create_db_success = CreateDbRequest(ctx, *(ctx->GetReqEvent()), postEventList); 
         if ( create_db_success == false )
         {
            ctx->SetAuxiliaryId(DONE);
            LOG(csmd, debug) << _cmdName << "::Process: at DONE";
            CSMLOG(csmd, info) << "EVENT COMPLETE";
         }

         break;
      }
          
      case RCV_DB:
      {
         if ( !isDBRespEvent(aEvent) )
         {
            LOG(csmd, error) << _cmdName << "::Process: Expecting a DBRespEvent";
            break;
         }
         LOG(csmd, debug) << _cmdName << "::Process at RCV_DB";

         int errcode;
         std::string errmsg;
         if ( !InspectDBResult(aEvent, errcode, errmsg) )
         {
            LOG(csmd, error) << _cmdName << "::Process: DB errcode=" << errcode << " " << errmsg;
         }
         else
         {
            LOG(csmd, debug) << _cmdName << "::Process: RCV_DB complete.";
         }
         CSMLOG(csmd, info) << "EVENT COMPLETE";
        
         ctx->SetAuxiliaryId(DONE);
         break;
      }

      default:
      {
         LOG(csmd, error) << _cmdName << "::Process: handler entered with context in invalid state: " << ctx->GetAuxiliaryId();
         break;
      }
   }
}

bool InvSsdWearUpdate::CreateDbRequest(const EventContextDBBase_sptr& ctx, const csm::daemon::CoreEvent& aEvent, 
   std::vector<csm::daemon::CoreEvent*>& postEventList)
{
   CSMLOG(csmd, debug) << "Enter " << __PRETTY_FUNCTION__;
   csm::daemon::NetworkEvent *ev = (csm::daemon::NetworkEvent *)&aEvent;
   csm::network::MessageAndAddress req_content = ev->GetContent();
    
   if ( HasValidDBConn() == false )
   {
      CSMLOG(csmd, error) << "CreateDbRequest: No valid DB connections available.";
      return false;
   }

   // Try to unpack the csm_ssd_wear_t update
   csm_ssd_wear_t ssd_wear;
   uint32_t bytes_unpacked;

   bytes_unpacked = ssd_wear_unpack(req_content._Msg.GetData(), ssd_wear);

   if ( bytes_unpacked == 0 ) 
   {
      CSMLOG(csmd, error) << "CreateDbRequest: failed to unpack ssd wear update message.";
      return false;
   }
   else
   {
      const string node_name(ssd_wear.node_name);
         
      CSMLOG(csmd, info) << "NEW EVENT: Received ssd wear update from " << node_name << ", discovered_ssds=" << ssd_wear.discovered_ssds;
      
      if (ssd_wear.discovered_ssds < 1)
      {
         // No wear data to update 
         return false;
      }   

      static const string SSD_TABLE_NAME("csm_ssd");
      std::ostringstream stmt_out;
      uint32_t param(0);         // Count of SQL parameters    
     
      for ( uint32_t i = 0; i < ssd_wear.discovered_ssds; i++ )
      {
         const string SSD_WEAR = "SSD_WEAR: " + node_name + "-ssd" + to_string(i) + " sn:" + string(ssd_wear.ssd[i].serial_number);
         CSMLOGp(csmd, info, SSD_WEAR) << "wear_lifespan_used: " << ssd_wear.ssd[i].wear_lifespan_used
                                       << " wear_percent_spares_remaining: " << ssd_wear.ssd[i].wear_percent_spares_remaining;
         CSMLOGp(csmd, info, SSD_WEAR) << "wear_total_bytes_written: " << ssd_wear.ssd[i].wear_total_bytes_written
                                       << " wear_total_bytes_read: " << ssd_wear.ssd[i].wear_total_bytes_read;
         
         // With the current architecture, only one SSD is expected per compute node
         // The DB prepared statement syntax also limits the DB Req to a single SQL statement
         // Attempting to update more than one row in this DB Req results in the following error:
         // DB errcode=25 cannot insert multiple commands into a prepared statement
         // Given the current requirements, assume only one SSD per update, but truncate the update here
         // with an appropriate warning message in case future systems contain multiple SSDs
         // A different approach will be needed to support multiple updates if they are required in the future.
         if (i == 0)
         {
            // Build the update statement 
            stmt_out << "UPDATE " << SSD_TABLE_NAME << " SET ";
            stmt_out << "wear_lifespan_used="            << "$" << ++param << "::integer, ";
            stmt_out << "wear_total_bytes_written="      << "$" << ++param << "::bigint, ";
            stmt_out << "wear_total_bytes_read="         << "$" << ++param << "::bigint, ";
            stmt_out << "wear_percent_spares_remaining=" << "$" << ++param << "::integer";
            stmt_out << " WHERE node_name="              << "$" << ++param << "::text "; 
            stmt_out << "AND serial_number="             << "$" << ++param << "::text;" << endl;
         }
         else
         {
            CSMLOGp(csmd, warning, SSD_WEAR) << "discarding update to unexpected SSD!"; 
         }
      }
      
      std::string sql_stmt = stmt_out.str();
      CSMLOG(csmd, debug) << "sql_stmt=" << sql_stmt;
   
      // Create the DB request 
      csm::db::DBReqContent db_content(sql_stmt, param);

      // Add SQL params
      for ( uint32_t i = 0; i < ssd_wear.discovered_ssds; i++ )
      {  
         // Limit DB Req to one SSD update
         // See discussion in previous loop above 
         if (i == 0)
         {
            db_content.AddNumericParam<int32_t>(ssd_wear.ssd[i].wear_lifespan_used);
            db_content.AddNumericParam<int64_t>(ssd_wear.ssd[i].wear_total_bytes_written);
            db_content.AddNumericParam<int64_t>(ssd_wear.ssd[i].wear_total_bytes_read);
            db_content.AddNumericParam<int32_t>(ssd_wear.ssd[i].wear_percent_spares_remaining);
            db_content.AddTextParam(ssd_wear.node_name);
            db_content.AddTextParam(ssd_wear.ssd[i].serial_number);
         }
      }
        
      csm::daemon::DBReqEvent* dbevent = new csm::daemon::DBReqEvent(db_content, csm::daemon::EVENT_TYPE_DB_Request, ctx);
      postEventList.push_back(dbevent);
    
      return true;
   }
}

bool InvSsdWearUpdate::CreateSqlStmt(const std::string& arguments, const uint32_t len,
                std::string &stmt, int &errcode, std::string &errmsg, bool compareDataForPrivateCheckRes)
{
   // This function is not in use, but is required by csmi_db_base
   CSMLOG(csmd, warning) << "Entering " << __PRETTY_FUNCTION__ << " unexpectedly!";
   return false; 
}

// will return csmi defined error code
int InvSsdWearUpdate::CreateByteArray(std::vector<csm::db::DBTuple *>&tuples,
                                          char **buf, uint32_t &bufLen, bool compareDataForPrivateCheckRes)
{
   // This function is not in use, but is required by csmi_db_base
   CSMLOG(csmd, warning) << "Entering " << __PRETTY_FUNCTION__ << " unexpectedly!";
    
   int errcode = CSMI_SUCCESS;
   return errcode;
}
