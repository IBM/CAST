/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/CSMISoftFailureRecovery.cc
    
    © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#include "CSMISoftFailureRecovery.h"

#include "CSMISoftFailureRecoveryAgentState.h"
#include "csmi_mcast/CSMIMcastResponder.h"
#include "csmi_mcast/CSMIMcastSpawner.h"

#include "csmi/include/csm_api.h"
#include "csmi/src/wm/include/csmi_wm_type_internal.h"
#include "include/csm_event_type_definitions.h"
#include "csmi/src/wm/include/csmi_wm_internal.h"
#include "csmi/src/common/include/csmi_api_internal.h"
#include "csmi/src/common/include/csmi_json.h"

#include "csmi_stateful_db/CSMIStatefulDBRecvSend.h"

#define STATE_NAME "CSMISoftFailureRecovery"

// Use this to make changing struct names easier.
#define INPUT_STRUCT   csm_soft_failure_recovery_input_t
#define MCAST_STRUCT   csmi_soft_failure_recovery_context_t
#define OUTPUT_STRUCT 
#define CMD_ID CSM_CMD_soft_failure_recovery

#define MCAST_PROPS_PAYLOAD CSMIMcastSoftFailureRecovery
#define EXTRA_STATES 3
CSMISoftFailureRecovery_Master::CSMISoftFailureRecovery_Master(csm::daemon::HandlerOptions& options) :
    CSMIStatefulDB(CMD_ID, options, STATEFUL_DB_DONE + EXTRA_STATES)
{
    // State id for the multicast spawner.    
    const int MCAST_SPAWN    = STATEFUL_DB_RECV_DB;     // 2
    const int MCAST_RESPONSE = STATEFUL_DB_RECV_DB + 1; // 3
    const int FIX_NODES      = STATEFUL_DB_RECV_DB + 2; // 4 
    const int FAIL_NODES     = STATEFUL_DB_RECV_DB + 3; // 5  NOTE: This is recv_db

    const int FINAL          = STATEFUL_DB_DONE + EXTRA_STATES;

    const int MASTER_TIMEOUT = csm_get_master_timeout(CMD_ID);

    SetState( MCAST_SPAWN, 
        new McastSpawner< MCAST_PROPS_PAYLOAD,
                          ParseInfoQuery,
                          FixRepairedNodes,
                          CreateByteArray >(
            MCAST_RESPONSE,                    // Success State 
            FIX_NODES,                       // Failure State
            FINAL,                             // Final State
            csm::daemon::helper::BAD_STATE,    // Timeout State 
            MASTER_TIMEOUT));                  // Timeout Time

    SetState( MCAST_RESPONSE,
        new McastResponder< MCAST_PROPS_PAYLOAD,
                            PayloadConstructor<MCAST_PROPS_PAYLOAD>,
                            FixRepairedNodes,                         // Called when all responses are successful.                            
                            FixRepairedNodes,                         // Called for failure.                                                 
                            csm::mcast::nodes::ParseResponseSoftFailure,// Performs a parse of the responses.                                  
                           false>(                                      // Specifies that a second multicast shouldn't be attempted in a failure. 
           FIX_NODES,               // Success State 
           FIX_NODES,               // Failure State
           FINAL,                     // Final State
           FIX_NODES,               // Timeout State
           MASTER_TIMEOUT));          // Timeout Time

    SetState( FIX_NODES,
        new StatefulDBRecvSend<CreateHardFailures>(
            FAIL_NODES,
            FAIL_NODES,
            FINAL));
}                           

                            
bool CSMISoftFailureRecovery_Master::CreatePayload(
    const std::string& arguments,
    const uint32_t len,
    csm::db::DBReqContent **dbPayload,
    csm::daemon::EventContextHandlerState_sptr ctx )
{
    LOG(csmapi,trace) << STATE_NAME ":CreatePayload: Enter"; 

    const int paramCount = 0;
    std::string paramStmt("SELECT node_name FROM csm_node WHERE state='SOFT_FAILURE' AND "
        "type='compute' and node_name NOT IN (SELECT node_name from csm_allocation_node)");

    csm::db::DBReqContent *dbReq = new csm::db::DBReqContent( paramStmt, paramCount );
    *dbPayload = dbReq;
    
    MCAST_PROPS_PAYLOAD* payload = new MCAST_PROPS_PAYLOAD( CMD_ID, new csmi_soft_failure_recovery_context_t()
        , false, false, 
         CSM_RAS_MSG_ID_ALLOCATION_TIMEOUT); // TODO Replace msg id
     ctx->SetDataDestructor( []( void* data ){ delete (MCAST_PROPS_PAYLOAD*)data;});
     ctx->SetUserData( payload );
        
     LOG(csmapi,info) << ctx << payload->GenerateIdentifierString()
         << "; Message: Looking for nodes in the soft failure state.";

    LOG(csmapi,trace) << STATE_NAME ":CreatePayload: Exit"; 

    return true;
}

bool CSMISoftFailureRecovery_Master::ParseInfoQuery( 
    csm::daemon::EventContextHandlerState_sptr ctx,
    const std::vector<csm::db::DBTuple *>& tuples, 
    MCAST_PROPS_PAYLOAD* mcastProps)
{
    LOG(csmapi,trace) <<  STATE_NAME ":ParseInfoQuery: Enter";

    MCAST_STRUCT* recovery = mcastProps->GetData();
    // TODO test recovery.

    size_t numNodes = tuples.size();

    recovery->num_nodes = (uint32_t)numNodes;
    recovery->compute_nodes = (char**)calloc(numNodes, sizeof(char*));
    LOG(csmapi,trace) << "Nodes initialized.";

    LOG(csmapi,info) << mcastProps->GenerateIdentifierString() <<
        "; Message: Found " << std::to_string(numNodes)  << " nodes in the SOFT_FAILURE state.";
    
    std::string nodeList("");

    for (size_t i =0; i < tuples.size(); ++i)
    {
        if ( tuples[i] && tuples[i]->nfields > 0 )
        {
            recovery->compute_nodes[i] = strdup(tuples[i]->data[0]);
            nodeList.append(recovery->compute_nodes[i]);
        }
        else
            recovery->compute_nodes[i] = strdup("");
    }

    // Log the mcast information.
    LOG(csmapi,info) << ctx << mcastProps->GenerateIdentifierString()
        << "; Message: Successfully retrieved nodes in soft failure, starting Mulitcast;";
    LOG(csmapi,info) << ctx << mcastProps->GenerateIdentifierString() 
        << "; Nodes: " << nodeList << ";";

    LOG(csmapi,trace) <<  STATE_NAME ":ParseInfoQuery: Exit";
    return true;
}
    
csm::db::DBReqContent* CSMISoftFailureRecovery_Master::FixRepairedNodes( 
    csm::daemon::EventContextHandlerState_sptr ctx,
    MCAST_PROPS_PAYLOAD* mcastProps)
{
    LOG(csmapi,trace) <<  STATE_NAME ":FixRepairedNodes: Enter";
    static std::map<std::string, short> _RetryMap;
    static std::mutex                   _RetryMutex;
    #define RETRY_LIMIT 10 // TODO Pull from daemon.

    // ========================================================================================
    std::vector<std::string> nodeVector = mcastProps->GenerateHostnameListing(true);
    std::string nodeStr = "{";

    std::string separator = "";
    for ( std::string node : nodeVector )
    {
        nodeStr.append(separator).append(node); 
        separator = ",";
    }
    nodeStr.append("}");
    LOG(csmapi, info) << nodeStr;
    // ========================================================================================
    
    // ========================================================================================
    std::vector<std::string> failVector = mcastProps->GenerateHostnameListing(false);
    std::map<std::string, short> tempMap;
    std::string failStr = "{";

    std::unique_lock<std::mutex>dataLock( _RetryMutex);
    separator = "";
    for ( std::string node : failVector )
    {
        short count = _RetryMap[node] + 1;

        if( count >= RETRY_LIMIT )
        {
            failStr.append(separator).append(node); 
            separator = ",";
        }
        else
        {
            tempMap[node] = count;
        }
    }
    failStr.append("}");

    // Replace the retry map and unclock access.
    _RetryMap = tempMap;
    dataLock.unlock();

    if ( mcastProps )
    {
        MCAST_STRUCT* recovery = mcastProps->GetData();
        recovery->hard_failure_nodes = strdup(failStr.c_str());
    }
    // ========================================================================================

    csm::db::DBReqContent *dbReq = nullptr;

    const int paramCount = 1; 
    std::string stmt = "UPDATE csm_node SET state='IN_SERVICE' "
        "WHERE node_name=ANY($1::text[]);";

    LOG(csmapi, info) << stmt;
    dbReq = new csm::db::DBReqContent( stmt, paramCount );
    dbReq->AddTextParam(nodeStr.c_str());

    LOG(csmapi,trace) <<  STATE_NAME ":FixRepairedNodes: Exit";
    return dbReq;
}

bool CSMISoftFailureRecovery_Master::CreateHardFailures(
    const std::vector<csm::db::DBTuple *>&tuples,
    csm::db::DBReqContent **dbPayload,
    csm::daemon::EventContextHandlerState_sptr ctx)
{
    LOG(csmapi,trace) <<  STATE_NAME ":CreateHardFailures: Enter";
    bool success = false;
    MCAST_PROPS_PAYLOAD* mcastProps = nullptr;
    std::unique_lock<std::mutex>dataLock =
        ctx->GetUserData<MCAST_PROPS_PAYLOAD*>(&mcastProps);
    
    if ( mcastProps )
    {
        MCAST_STRUCT* recovery = mcastProps->GetData();
        const int paramCount = 1; 
        std::string stmt =  "UPDATE csm_node SET state='HARD_FAILURE' "
            "WHERE node_name=ANY($1::text[]);";

        csm::db::DBReqContent *dbReq = new csm::db::DBReqContent( stmt, paramCount );
        dbReq->AddTextParam(recovery->hard_failure_nodes);

        *dbPayload = dbReq;
        success=true;
    }

    dataLock.unlock();
    LOG(csmapi,trace) <<  STATE_NAME ":CreateHardFailures: Exit";
    return success;
}

bool CSMISoftFailureRecovery_Master::CreateResponsePayload(
    const std::vector<csm::db::DBTuple *>&tuples,
    csm::db::DBReqContent **dbPayload,
    csm::daemon::EventContextHandlerState_sptr ctx)
{
    LOG(csmapi,trace) <<  STATE_NAME ":CreateResponsePayload: Enter";
    MCAST_PROPS_PAYLOAD* mcastProps = nullptr;
    std::unique_lock<std::mutex>dataLock =
        ctx->GetUserData<MCAST_PROPS_PAYLOAD*>(&mcastProps);

    *dbPayload = FixRepairedNodes(ctx, mcastProps);
    dataLock.unlock();

    LOG(csmapi,trace) <<  STATE_NAME ":CreateResponsePayload: Exit";
    return *dbPayload != nullptr;
}

bool CSMISoftFailureRecovery_Master::CreateByteArray(
        const std::vector<csm::db::DBTuple *>&tuples,
        char **buf, uint32_t &bufLen,
        csm::daemon::EventContextHandlerState_sptr ctx )
{
    return CreateByteArray(buf, bufLen, ctx);
}

bool CSMISoftFailureRecovery_Master::CreateByteArray(
    char **buf, uint32_t &bufLen,
    csm::daemon::EventContextHandlerState_sptr ctx )
{
    LOG(csmapi,trace) <<  STATE_NAME ":CreateByteArray: Enter";

    bool success = ctx->GetErrorCode() == CSMI_SUCCESS;
    // Get the allocation data.
    MCAST_PROPS_PAYLOAD* mcastProps = nullptr;
    std::unique_lock<std::mutex>dataLock =
        ctx->GetUserData<MCAST_PROPS_PAYLOAD*>(&mcastProps);

    *buf = nullptr;
    bufLen= 0;
    if ( mcastProps )
    {
        LOG(csmapi,info) << ctx->GetCommandName() << ctx
            << mcastProps->GenerateIdentifierString()
            << "; Message: Recovery completed;";
    }

    dataLock.unlock();

    LOG(csmapi,trace) <<  STATE_NAME ":CreateByteArray: Exit";

    return success;
}

CSMISoftFailureRecovery_Agent::CSMISoftFailureRecovery_Agent( csm::daemon::HandlerOptions& options ) : 
    CSMIStateful( CMD_ID, options )
{
    // Set the start state for the machine.
    SetInitialState( 0 );

    // Add the states and their Transitions.
    ResizeStates( 1 );

    SetState( 0,
        new SoftFailureRecoveryAgentState(
            1,
            1,
            1));
}

