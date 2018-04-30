/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/CSMIRasEventCreate.cc

  © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
// implement the CSM api ras event create command...
//

#include <syslog.h>
#include <string>       // std::string
#include <iostream>     // std::cout
#include <sstream> 
#include <memory>  
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/predicate.hpp>

#include "csm_daemon_config.h"
#include "CSMIRasEventCreate.h"
#include <csm_api_ras.h>

#include <csmd/src/ras/include/RasEvent.h>
#include <csmd/src/ras/include/RasMaster.h>
#include <string_tokenizer.h>

using namespace std;

std::string bool_to_string(bool b)
{
    std::stringstream string_str;
    string_str << b;
    return string_str.str();
}

bool string_to_bool(std::string s)
{
    if (s == "t")
    {
        return true;
    }
    else if (s == "f")
    {
        return false;
    }
    else
    {
        LOG(csmras, error) << "Unexpected boolean string value: " << s << ", setting to false";
        return false;
    }
}

CSMIRasEventCreate::CSMIRasEventCreate(csm::daemon::HandlerOptions &options) : 
    CSMI_BASE(CSM_CMD_ras_event_create, options ),
    _csmDaemonConfig(nullptr)
{
    _initialized  = false;
}

/**
 * send ras event information to syslog...
 * 
 * @param rasEvent 
 */
void CSMIRasEventCreate::logSyslog(RasEvent &rasEvent)
{
    // Disabled for Beta 1
    return;

    LOG(csmras, trace) << "Enter " << __PRETTY_FUNCTION__;
    
    std::string jsonData=string("CSMRAS:") + rasEvent.getJsonData();
    // keep it simple for now..
    syslog(LOG_INFO, "%s", jsonData.c_str());

}

/**
 * onRasPoolExit
 *  
 * Processing of ras events when they exit the ras pool... 
 * 
 * @param rasEvent 
 */
void CSMIRasEventCreate::onRasPoolExit(RasEvent &rasEvent,
                                       const csm::daemon::CoreEvent &aEvent, 
                                       std::vector<csm::daemon::CoreEvent*>& postEventList)
{
    LOG(csmras, info) << "CSMIRasEventCreate::Process" << "onRasPoolExit = " << rasEvent.msg_id();

    string suppressed = rasEvent.getValue(CSM_RAS_FKEY_SUPPRESSED);
    string ctxIdStr = rasEvent.getValue(CSM_RAS_FKEY_CTXID);       // get the remembered context id...
    uint64_t ctxId = atoll(ctxIdStr.c_str());
    bool db_write_resp_pending(false);

    bool threshold_hit(false);
    threshold_hit = g_ras_master.handleRasEventThreshold(rasEvent);

    if ( (threshold_hit) &&            // Only take action if the event has hit threshold
         (!(suppressed == "1")) )      // Only take action and record events that aren't suppressed
    {
       string control_action = rasEvent.getValue(CSM_RAS_FKEY_CONTROL_ACTION);

       // Check to see if there is a control_action that needs to be run 
       if ((control_action.size() != 0) && !boost::iequals(control_action, CSM_RAS_NONE)) 
       {
          _handleAction.handle(rasEvent);     // TO DO, move this somewhere else, after processing the ras pool...
       }   
       
       // Add all events that are enabled and have hit threshold to the csm_ras_event_action table   
       csm::daemon::EventContext_sptr context(new csm::daemon::EventContext(this, ctxId, CopyEvent(aEvent)));

       shared_ptr<CSMIRasEventCreateContext> rctx = _contextMap[ctxId];
       assert(rctx);                     // todo, some better error checking...

       string sqlStmt = getRasCreateSql(rasEvent);
       rctx->_state = CSMIRasEventCreateContext::WRITE_RAS_EVENT;      // remember we did this so we dispose of things on completion...
          
       csm::db::DBReqContent dbcontent(sqlStmt);
       csm::daemon::DBReqEvent *dbevent = new csm::daemon::DBReqEvent(dbcontent, csm::daemon::EVENT_TYPE_DB_Request, context);
       postEventList.push_back(dbevent);
       db_write_resp_pending = true; 
       
       logSyslog(rasEvent);
    }
       
    // Write to RAS EVENT LOG
    _rasEventLog.log(rasEvent.getJsonData());
     
    if (!db_write_resp_pending)
    {
       LOG(csmras, debug) << "RAS EVENT COMPLETE   " << rasEvent.getLogString();
    }
}

std::string CSMIRasEventCreate::trim(const std::string& str)
{
    LOG(csmras, trace) << "Enter " << __PRETTY_FUNCTION__;
    
    if (str.size() == 0)
        return(str);
    size_t first = str.find_first_not_of(' ');
    size_t last = str.find_last_not_of(' ');
    return str.substr(first, (last-first+1));
}

void CSMIRasEventCreate::returnErrorMsg(csm::network::Address_sptr addr,
                                        csm::network::Message& inMsg,
                                        int errcode,
                                        const std::string &errmsg,
                                        std::vector<csm::daemon::CoreEvent*>& postEventList)
{
    LOG(csmras, trace) << "Enter " << __PRETTY_FUNCTION__;
    
    char *buf=nullptr;
    uint32_t bufLen = 0;
    buf = csmi_err_pack(errcode, errmsg.c_str(), &bufLen);
    uint8_t flags = CSM_HEADER_RESP_BIT | CSM_HEADER_ERR_BIT;

    csm::network::Message rspMsg;
    CreateNetworkMessage(inMsg, buf, bufLen, flags, rspMsg);
    if (buf) free(buf);

    csm::network::Address_sptr rspAddress( CreateReplyAddress(addr.get()));
    if (rspAddress) {
      csm::network::MessageAndAddress netcontent( rspMsg, rspAddress );
      csm::daemon::NetworkEvent *netEvent = new csm::daemon::NetworkEvent(netcontent,
                                                csm::daemon::EVENT_TYPE_NETWORK, nullptr);
      postEventList.push_back(netEvent);
    }
    

}

RasRc CSMIRasEventCreate::decodeRasEvent(csm::network::MessageAndAddress content, 
                                         RasEvent &rasEvent)
{
    LOG(csmras, trace) << "Enter " << __PRETTY_FUNCTION__;
    
    string msgData = content._Msg.GetData();
    csm_ras_event_create_input_t *rargs = nullptr;
    csm_deserialize_struct(csm_ras_event_create_input_t, &rargs, msgData.c_str(), content._Msg.GetDataLen());

    if ( rargs == nullptr )
    {
        LOG(csmras,error) << "error from _argUnpackFunc " << _className;
        return(CSMERR_MSG_PACK_ERROR);
    }

    // msg_id is required, return an error if it isn't set
    if ((rargs->msg_id == NULL) || (strlen(rargs->msg_id) == 0))
    {
        LOG(csmras, error) << "Required parameter msg_id is not set." << endl;
        csm_free_struct_ptr(csm_ras_event_create_input_t, rargs);
        return(CSMERR_MISSING_PARAM);
    }
   
    // Treat time_stamp as optional and generate one now if it wasn't supplied 
    // Example format:
    // 2016-05-12 15:12:11.799506
    if ((rargs->time_stamp == NULL) || (strlen(rargs->time_stamp) == 0))
    {
        char time_stamp[80];
        char time_stamp_with_usec[80];

        struct timeval now_tv;
        time_t rawtime;
        struct tm *info;

        gettimeofday(&now_tv, NULL);
        rawtime = now_tv.tv_sec;
        info = localtime( &rawtime );

        strftime(time_stamp, 80, "%Y-%m-%d %H:%M:%S", info);    
        snprintf(time_stamp_with_usec, 80, "%s.%06lu", time_stamp, now_tv.tv_usec);
    
        LOG(csmras, info) << "Optional parameter time_stamp is not set, setting to:" << time_stamp_with_usec;
        rasEvent.setValue(CSM_RAS_FKEY_TIME_STAMP, time_stamp_with_usec);
    }
    else
    {
        rasEvent.setValue(CSM_RAS_FKEY_TIME_STAMP, rargs->time_stamp);
    }

    rasEvent.setValue(CSM_RAS_FKEY_MSG_ID, rargs->msg_id);
    string location_name("");
    string raw_data("");
    string kvcsv("");
    if (rargs->location_name) location_name=rargs->location_name;
    if (rargs->raw_data) raw_data = rargs->raw_data;
    if (rargs->kvcsv) kvcsv = rargs->kvcsv;
    rasEvent.setValue(CSM_RAS_FKEY_LOCATION_NAME, location_name);
    rasEvent.setValue(CSM_RAS_FKEY_RAW_DATA, raw_data);
    rasEvent.setValue(CSM_RAS_FKEY_KVCSV, kvcsv);

    // In addition to passing the whole kvcsv string, also set the individual key value pairs...
    if (rargs->kvcsv) 
    {
        // key value format is:
        // key=value,key=value,key=value
        StringTokenizer kvtokens;
        kvtokens.tokenize(rargs->kvcsv,",");
        for (unsigned n = 0; n < kvtokens.size(); n++) 
        {
            StringTokenizer kv;
            kv.tokenize(kvtokens[n],"=");
            if (kv.size() >= 2)  
            {
                rasEvent.setValue(kv[0], kv[1]);
            }
        }
    }

    csm_free_struct_ptr(csm_ras_event_create_input_t, rargs);

    return(RasRc(CSMI_SUCCESS));
}

// Generate SQL to read back the data for this msg_id from the csm_ras_type table and
// get the current state for the node matching location_name, if applicable
// This information should be returned in a single row containing all of the csm_ras_type data followed by the node state
std::string CSMIRasEventCreate::getMsgTypeSql(const std::string &msg_id, const std::string &location_name)
{
    ostringstream sqlStmt;
   
    // Get the RAS MSG TYPE settings 
    // This should return 0 or 1 rows
    sqlStmt << "WITH T1 AS (SELECT "
            << "msg_id, " 
            << "severity, "
            << "message, "
            << "description, "
            << "control_action, "
            << "threshold_count, "
            << "threshold_period, "
            << "enabled, "
            << "set_state, "
            << "visible_to_users, "
            << "NULL AS state, "
            << "ROW_NUMBER() OVER (ORDER BY (SELECT 1)) AS ID "
            << "FROM csm_ras_type WHERE " 
            << "msg_id = '" << msg_id << "'"
            << "),";
   
    // Get the node state 
    sqlStmt << " T2 AS (SELECT "
            << "NULL AS msg_id, " 
            << "NULL AS severity, "
            << "NULL AS message, "
            << "NULL AS description, "
            << "NULL AS control_action, "
            << "NULL AS threshold_count, "
            << "NULL AS threshold_period, "
            << "NULL AS enabled, "
            << "NULL AS set_state, "
            << "NULL AS visible_to_users, "
            << "state, "
            << "ROW_NUMBER() OVER (ORDER BY (SELECT 1)) AS ID "
            << "FROM csm_node WHERE " 
            << "node_name = '" << location_name << "'"
            << ")";

    // Join the two queries
    sqlStmt << " SELECT "
            << "T1.msg_id," 
            << "T1.severity, "
            << "T1.message, "
            << "T1.description, "
            << "T1.control_action, "
            << "T1.threshold_count, "
            << "T1.threshold_period, "
            << "T1.enabled, "
            << "T1.set_state, "
            << "T1.visible_to_users, "
            << "T2.state "
            << "FROM T1 LEFT JOIN T2 ON(T1.ID=T2.ID)"
            << ";";
    
    LOG(csmras, debug) << "CSMIRasEventCreate::getMsgTypeSql: " << sqlStmt.str();

    return(sqlStmt.str());
}

RasRc CSMIRasEventCreate::getMsgTypeFromDbRec(csm::db::DBResult_sptr dbRes,
                                              RasMessageTypeRec &rec,
                                              std::string &node_state)
{
    LOG(csmras, trace) << "Enter " << __PRETTY_FUNCTION__;
    
    RasRc rc = RasRc(CSMI_SUCCESS);
    node_state="";    
    
    std::vector<csm::db::DBTuple *> tuples;
    GetTuplesFromDBResult(dbRes, tuples);
    // tuples will be empty if error in DBResult
    uint32_t nrows = tuples.size();
    if (dbRes == NULL || nrows == 0) 
    {
        return(RasRc(CSMERR_DB_ERROR, "RAS Event could not be created in database (invalid msg_id)"));
    }
   
#ifdef REMOVED
    for (uint32_t i = 0; i < tuples.size(); i++)
    {
        for (int32_t j = 0; j < tuples[i]->nfields; j++)
        {
            LOG(csmras, debug) << "tuple " << i << " field " << j << ": " << tuples[i]->data[j] << endl;
        }
    }
#endif

    // tuples should return 1 row in the good case
    // expected data is either:
    // 0 rows -> either an error occurred or the msg_id is invalid
    // 1 row with 10 msg_type fields and NULL state field -> msg_id is valid, location_name does not match any nodes in csm_node table
    // 1 row with 10 msg_type fields and valid state field -> msg_id is valid, location_name matched a node in the csm_node table
    if (nrows > 1)
    {
       rc = RasRc(CSMERR_DB_ERROR, "invalid row count");
    }
    else
    {
        csm::db::DBTuple* fields = tuples[0];
        if (fields->nfields == 11) 
        {
            rec._msg_id            = fields->data[0];
            rec._severity          = trim(fields->data[1]);
            rec._message           = fields->data[2];
            rec._description       = fields->data[3];
            rec._control_action    = fields->data[4];
            rec._threshold_count   = atoi(fields->data[5]);
            rec._threshold_period  = atoi(fields->data[6]);
            rec._enabled           = string_to_bool(fields->data[7]);
            rec._set_state         = fields->data[8];
            rec._visible_to_users  = string_to_bool(fields->data[9]);
            node_state             = fields->data[10];
        }
        else
        {
            rc = RasRc(CSMERR_DB_ERROR, "invalid field count");
        }
    }

    for (uint32_t i=0;i<tuples.size();i++) csm::db::DB_TupleFree(tuples[i]);
    return rc;
}

string CSMIRasEventCreate::getRasCreateSql(RasEvent &rasEvent)
{
    LOG(csmras, trace) << "Enter " << __PRETTY_FUNCTION__;

    vector<string> fields;
    if (rasEvent.hasValue(CSM_RAS_FKEY_MSG_ID))          fields.push_back(CSM_RAS_FKEY_MSG_ID);
    if (rasEvent.hasValue(CSM_RAS_FKEY_TIME_STAMP))      fields.push_back(CSM_RAS_FKEY_TIME_STAMP);
    if (rasEvent.hasValue(CSM_RAS_FKEY_LOCATION_NAME))   fields.push_back(CSM_RAS_FKEY_LOCATION_NAME);
    fields.push_back(CSM_RAS_FKEY_COUNT);
    if (rasEvent.hasValue(CSM_RAS_FKEY_MESSAGE))         fields.push_back(CSM_RAS_FKEY_MESSAGE);
    if (rasEvent.hasValue(CSM_RAS_FKEY_KVCSV))           fields.push_back(CSM_RAS_FKEY_KVCSV);
    if (rasEvent.hasValue(CSM_RAS_FKEY_RAW_DATA))        fields.push_back(CSM_RAS_FKEY_RAW_DATA);

    string dbstr;
    dbstr = "INSERT INTO csm_ras_event_action (";
    
    // Need to associate to the correct msg_id_seq from the csm_ras_type_audit table
    dbstr += "msg_id_seq,";
    dbstr += "master_time_stamp,";
    
    for (unsigned n = 0; n < fields.size(); n++) {
        if (n > 0) dbstr += ",";
        dbstr += fields[n];
    }
    
    dbstr += ") VALUES (";
    
    // Need to associate to the correct msg_id_seq from the csm_ras_type_audit table
    dbstr += "(select msg_id_seq from csm_ras_type_audit where msg_id = '";
    dbstr += rasEvent.getValue(CSM_RAS_FKEY_MSG_ID);
    dbstr += "' order by change_time desc limit 1),";
    
    // master_time_stamp
    dbstr += "now(),";
    
    for (unsigned n = 0; n < fields.size(); n++) 
    {
        if (n > 0) dbstr += ",";
        if (fields[n] == CSM_RAS_FKEY_COUNT) 
        {
            //dbstr += "to_number('" + to_string(rasEvent.getCount()) + "','9999')";
            dbstr += "'" + to_string(rasEvent.getCount()) + "'";
        } 
        else 
        {
            dbstr += "'" + rasEvent.getValue(fields[n]) + "'";
        }
    }
    dbstr += ")";

    // Add sections to SQL if set_state is not null
    string set_state = rasEvent.getValue(CSM_RAS_FKEY_SET_STATE);
    // Only change the node state if set_state=SOFT_FAILURE
    if (set_state == CSM_NODE_STATE_SOFT_FAILURE)
    {
        // Only change the node state if set_state=SOFT_FAILURE and
        // the node state is currently DISCOVERED or IN_SERVICE
        LOG(csmras, debug) << "Generating SQL for set_state=" << set_state << endl;
        dbstr += ";UPDATE csm_node SET state='" + set_state + "' where node_name='";
        dbstr += rasEvent.getValue(CSM_RAS_FKEY_LOCATION_NAME);
        dbstr += "'";
        dbstr += " AND state IN ('" + (string) CSM_NODE_STATE_DISCOVERED + "','" + (string) CSM_NODE_STATE_IN_SERVICE + "')";
    }
    else if (set_state == "")
    {
        // Nothing to do
    }
    else
    {
        LOG(csmras, warning) << "Ignoring invalid state request: set_state=" << set_state << endl;
    }

    LOG(csmras, debug) << "CSMIRasEventCreate::getRasCreateSql: " << dbstr;

    return(dbstr);
}



/**
 * Process the CSMRasEventCreate event. 
 *  
 * Potentially a three step setup for the work. 
 * 1.  optinally get the meta data, if it is not already 
 *      cached.. 
 * 2.  process the ras handler chain. 
 * 3.  write the ras data to the db.. 
 * 
 * 
 * @param aEvent 
 * @param postEventList 
 */
void CSMIRasEventCreate::Process( const csm::daemon::CoreEvent &aEvent, 
                                  std::vector<csm::daemon::CoreEvent*>& postEventList )
{
    LOG(csmras, trace) << "Enter " << __PRETTY_FUNCTION__;

    if (!_initialized) {
        init();
    }
    if (!HasValidDBConn()) {
        csm::daemon::NetworkEvent *ev = (csm::daemon::NetworkEvent *)&aEvent;
        csm::network::MessageAndAddress content = ev->GetContent();
        if (! content._Msg.GetInt() )
            returnErrorMsg(content.GetAddr(), content._Msg, CSMERR_DAEMON_DB_ERR,
                           std::string("CSM ERROR - Database Error in Local Daemon"),
                           postEventList);
        return;
    }

    boost::unique_lock<boost::mutex>  guard(_ctxMutex);

    // step one, decode the message, and
    // setup a db request to get the message id.
    //      keep a place holder to cache this request..
    if( aEvent.HasSameContentTypeAs( typeid( csm::daemon::EventContentContainer<csm::network::MessageAndAddress> ) ) )
    {
        csm::daemon::NetworkEvent *ev = (csm::daemon::NetworkEvent *)&aEvent;
        csm::network::MessageAndAddress content = ev->GetContent();
        shared_ptr<RasEvent > rasEvent(new RasEvent());
        RasRc rasRc = decodeRasEvent(content, *rasEvent);
        if (rasRc._rc != CSMI_SUCCESS) {
            if (! content._Msg.GetInt() )
                returnErrorMsg(content.GetAddr(), content._Msg, rasRc._rc, rasRc._errstr, postEventList);
            return;
        }

        //
        // create a network event with both a ContextUuid and the 
        // original event content, type and source...
        string sqlStmt = getMsgTypeSql(rasEvent->getValue(CSM_RAS_FKEY_MSG_ID), rasEvent->getValue(CSM_RAS_FKEY_LOCATION_NAME));

        csm::daemon::EventContext_sptr context(new csm::daemon::EventContext(this, CreateCtxAuxId(), CopyEvent(aEvent)));

        shared_ptr<CSMIRasEventCreateContext> rctx(new CSMIRasEventCreateContext());
        _contextMap[context->GetAuxiliaryId()] = rctx;
        rasEvent->setValue(CSM_RAS_FKEY_CTXID,  boost::lexical_cast<std::string>(context->GetAuxiliaryId()));     // remember this.

        rctx->_rasEvent = rasEvent;
        rctx->_state = CSMIRasEventCreateContext::READ_EVENT_TYPE;

        LOG(csmras, info) << "NEW RAS EVENT        " << rctx->_rasEvent->getLogString();

        csm::db::DBReqContent dbcontent(sqlStmt);
        csm::daemon::DBReqEvent *dbevent = new csm::daemon::DBReqEvent(dbcontent, csm::daemon::EVENT_TYPE_DB_Request, context);
        postEventList.push_back(dbevent);
        return;
    }
    else if( aEvent.HasSameContentTypeAs( typeid( csm::daemon::EventContentContainer<csm::db::DBRespContent> ) ) )
    {
        //
        // we will get two of these, one to get the message response, and the second to get the
        // reply to the caller once the ras event is squared away..
        //
        
        csm::daemon::DBRespEvent *dbevent = (csm::daemon::DBRespEvent *) &aEvent;
        csm::db::DBRespContent content = dbevent->GetContent();
        // get the earlier request in the context
        csm::daemon::CoreEvent *reqEvent = dbevent->GetEventContext()->GetReqEvent();

        bool isMsgAndAddress = 
            reqEvent->HasSameContentTypeAs( typeid( csm::daemon::EventContentContainer<csm::network::MessageAndAddress> ) ) ;

        if (content.GetContentType() == csm::db::DBContentType::CSM_DB_REQ)
        {
          // unexpected behavior
          LOG(csmras, error) << _cmdName << "::Process(): Unexpected DB Request Event";
        } // if DB_REQ
        //else if( reqEvent->HasSameContentTypeAs( typeid( csm::daemon::EventContentContainer<csm::network::MessageAndAddress> ) ) )
        {
            //shared_ptr<csm::daemon::HandlerContext> ctx = content.GetContext();
            csm::daemon::EventContext_sptr ctx = dbevent->GetEventContext();
            assert(ctx);
            shared_ptr<CSMIRasEventCreateContext> rctx = _contextMap[ctx->GetAuxiliaryId()];
            assert(rctx);                     // todo, some better error checking...
        

            // CSM_DB_RESP here && the earlier request is a network event
            //char *buf=nullptr;
            //uint32_t bufLen=0;
            //int errcode = CSMI_SUCCESS;
            csm::db::DBRespContent dbResp = *(csm::db::DBRespContent*) &content;
                
            if (rctx->_state == CSMIRasEventCreateContext::READ_EVENT_TYPE) 
            {
                LOG(csmras, debug) << "RAS EVENT DB RD RESP " << rctx->_rasEvent->getLogString(); 
                
                assert(isMsgAndAddress);        // read event should always be in response to a regular network event.
                RasEvent &rasEvent = *rctx->_rasEvent;
                // retrieve the db response
                csm::db::DBResult_sptr dbRes = dbResp.GetDBResult();

                RasRc rasRc(CSMI_SUCCESS);
                if (dbRes == nullptr)
                {
                    rasRc._rc = CSMERR_DB_ERROR;
                    rasRc._errstr = "No Database Connection in Local Daemon";
                }
                else if (dbRes->GetResStatus() != csm::db::DB_SUCCESS)
                {
                    rasRc._rc = CSMERR_DB_ERROR;
                    rasRc._errstr = dbRes->GetErrMsg();
                }
                RasMessageTypeRec rec;
                string node_state("");
                if (rasRc._rc == CSMI_SUCCESS) {
                    rasRc = getMsgTypeFromDbRec(dbRes, rec, node_state);
                }
    
                if (rasRc._rc == CSMI_SUCCESS) {
                    // transfer rec fields to RasEvent...
                    rasEvent.setValue(CSM_RAS_FKEY_MSG_ID,   rec._msg_id);
                    rasEvent.setValue(CSM_RAS_FKEY_SEVERITY,   rec._severity);
                    rasEvent.setValue(CSM_RAS_FKEY_MESSAGE,    rec._message);
                    rasEvent.setValue(CSM_RAS_FKEY_CONTROL_ACTION,  rec._control_action);
                    rasEvent.setValue(CSM_RAS_FKEY_DESCRIPTION,  rec._description);
                    
                    rasEvent.setThresholdCount(rec._threshold_count);
                    rasEvent.setThresholdPeriod(rec._threshold_period);
                    
                    rasEvent.setValue(CSM_RAS_FKEY_ENABLED, bool_to_string(rec._enabled));
                    rasEvent.setValue(CSM_RAS_FKEY_SET_STATE,  rec._set_state);
                    rasEvent.setValue(CSM_RAS_FKEY_VISIBLE_TO_USERS,  bool_to_string(rec._visible_to_users));
 
                    // next, do the ras handler chain...
                    
                    // create the ras event sql...
                    //rasRc = _rasHandlerChain->handle(rasEvent);
                   
                    // RAS events will be suppressed for nodes in these states: 
                    // CSM_NODE_STATE_OUT_OF_SERVICE, CSM_NODE_STATE_MAINTENANCE
                    bool suppressed(false);
                    if ((node_state == CSM_NODE_STATE_OUT_OF_SERVICE) || (node_state == CSM_NODE_STATE_MAINTENANCE))
                    {
                        suppressed=true;
                    }

                    // Only continue to process the event if it is enabled in the csm_ras_type table
                    // and the event has not been suppressed due to the node_state 
                    if (rec._enabled && !suppressed)
                    {
                        _handleVarSub.handle(rasEvent);
                        onRasPoolExit(rasEvent, aEvent, postEventList);
                    }
                    else if (suppressed)
                    {
                        LOG(csmras, debug) << "RAS EVENT SUPPRESSED " << rctx->_rasEvent->getLogString(); 
                    }
                    else
                    {
                        LOG(csmras, debug) << "RAS EVENT DISABLED   " << rctx->_rasEvent->getLogString(); 
                    }
                }

                if (rasRc._rc != CSMI_SUCCESS) {
                    csm::daemon::NetworkEvent *ev = (csm::daemon::NetworkEvent *)reqEvent;
                    csm::network::MessageAndAddress c = ev->GetContent();
                    LOG(csmras,error) << "CreateRasEvent error " << rec._msg_id << ": " << rasRc._errstr;
                    if (! c._Msg.GetInt() )
                        returnErrorMsg(c.GetAddr(), c._Msg, rasRc._rc, rasRc._errstr, postEventList);

                    return;     // end of the line...
                }

                // finally, ready to construct a network event
                int errcode = CSMI_SUCCESS;

                csm::daemon::NetworkEvent *ev = (csm::daemon::NetworkEvent *)reqEvent;
                csm::network::MessageAndAddress content = ev->GetContent();
                // check to see if this is daemon to daemon or back to to the CSMapi.
                //     only reply if the return is for CSMapi.
                if (! content._Msg.GetInt() ) 
                {
                    char *buf=nullptr;      // return null response...
                    uint32_t bufLen=0;
                    uint8_t flags = CSM_HEADER_RESP_BIT;
                    if (errcode != CSMI_SUCCESS) flags |= CSM_HEADER_ERR_BIT;;

                    csm::network::Message rspMsg;
                    CreateNetworkMessage(content._Msg, buf, bufLen, flags, rspMsg);
                    csm::network::Address_sptr rspAddress = CreateReplyAddress(content.GetAddr().get());

                    if (rspAddress) {
                      csm::network::MessageAndAddress netcontent( rspMsg, rspAddress );
                      csm::daemon::NetworkEvent *netEvent = new csm::daemon::NetworkEvent(netcontent,
                                                              csm::daemon::EVENT_TYPE_NETWORK);
                      postEventList.push_back(netEvent);
                      //delete rspAddress;
                    }
                }

            }
            else if (rctx->_state == CSMIRasEventCreateContext::WRITE_RAS_EVENT) 
            {
                LOG(csmras, debug) << "RAS EVENT DB WR RESP " << rctx->_rasEvent->getLogString(); 
                LOG(csmras, debug) << "RAS EVENT COMPLETE   " << rctx->_rasEvent->getLogString(); 
            }
            else 
            {
                LOG(csmras, warning) << "RAS EVENT unexpected branch taken, rctx->_state=" << rctx->_state; 

                // TODO: detect and log db errors here...
                // remove the rtx reference...
                _contextMap.erase(ctx->GetAuxiliaryId());

            }
        }
    }
    else if (isTimerEvent(aEvent)) {
        std::vector<std::shared_ptr<RasEvent> > expiredEvents;
        time_t t = _rasEventPool.TimerExpired(expiredEvents);

        for (std::vector<std::shared_ptr<RasEvent> >::iterator it = expiredEvents.begin();
              it != expiredEvents.end(); ++it ) {
            
            LOG(csmras,warning) << "RAS EVENT TIMER      " << (*it)->getLogString();
            
            onRasPoolExit(*(*it), aEvent, postEventList);
        }

        if (t > 0) {
            // time is in absolute epoc time, convert to delta from now..
            time_t now;                             // pickup the epoc time...
            time(&now);
            uint64_t deltaTime = 1000;  // minimum delta time (1 second)...
            if (now < t) {
                deltaTime = (t-now)*1000;       // convert to milliseconds..
            }
            LOG(csmras,debug) << "CreateTimerEvent now:" << now << " deltaTime:" << deltaTime;
            postEventList.push_back( CreateTimerEvent( deltaTime, this) );

        }
    }
}

int CSMIRasEventCreate::GetRespData(const std::string& argument,
                                    const uint32_t &argLen, 
                                    char **buf,
                                    uint32_t& bufLen)
{
    return(CSMI_SUCCESS);
}





// singleton initializer, call on the first call to get resp data...
void CSMIRasEventCreate::init() {
    LOG(csmras, trace) << "Enter " << __PRETTY_FUNCTION__;
    
    if (_initialized)
        return;
    _initialized = true;
    _rasHandlerChain.reset(new RasEventHandlerChain());

    // pass the configuration information to the handler chain, so it can 
    _csmDaemonConfig = csm::daemon::Configuration::Instance();
    boost::property_tree::ptree *csmConfig = _csmDaemonConfig->GetProperties();
    _rasHandlerChain->setCsmConfig(csmConfig);

    if (_csmDaemonConfig) {
        std::string scriptDir = csmConfig->get<string>("csm.ras.action.scriptdir", std::string( "./default_scriptdir" ) );
        std::string fatalScr = csmConfig->get<string>("csm.ras.action.fatal_event", std::string( "./fatal_event.sh" ) );
        std::string logDir = csmConfig->get<string>("csm.ras.action.logdir", std::string( "./defaultlog_dir" ) );
        unsigned maxActions = csmConfig->get<unsigned>("csm.ras.action.maxactions", 100);
        unsigned actionTimeout = csmConfig->get<unsigned>("csm.ras.action.timeout", 20);
        #if 0
        LOG(csmras, info) << "RasEventHandlerChain scriptDir = " << scriptDir;
        LOG(csmras, info) << "RasEventHandlerChain logDir = " << logDir;
        LOG(csmras, info) << "RasEventHandlerChain maxActions = " << maxActions;
        LOG(csmras, info) << "RasEventHandlerChain actionTimeout = " << actionTimeout;
        #endif
        _handleAction.setScriptDir(scriptDir);
        _handleAction.setFatalScript(fatalScr);
        _handleAction.setLogDir(logDir);
        _handleAction.setMaxActions(maxActions);
        _handleAction.setActionTimeout(actionTimeout);

    }
    else {
        LOG(csmras, error) << "CSMIRasEventCreate::init csmConfig";
    }



}


