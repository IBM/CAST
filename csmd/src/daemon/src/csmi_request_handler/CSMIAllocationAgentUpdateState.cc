/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/CSMIAllocationAgentUpdateState.cc

  © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
 
================================================================================*/

#include "CSMIAllocationAgentUpdateState.h"
#include "helpers/csm_handler_exception.h"
#include "helpers/cgroup.h"
#include "helpers/DataAggregators.h"
#include "helpers/AgentHandler.h"
#include "csmd/src/inv/include/inv_dcgm_access.h"

#include "csmi/include/csm_api_consts.h"
#include "csmi/include/csm_api.h"
#include <syslog.h>
#include <iostream>    ///< IO stream for file operations.
#include <fstream>     ///< ofstream and ifstream used.
#include <algorithm>

#define PROLOG_RAS(message)  this->PushRASEvent(ctx, postEventList, message,\
    respPayload->hostname, ctx->GetErrorMessage(), "rc=" + std::to_string(ctx->GetErrorCode()));

const char* CSM_ACTIVELIST = "/etc/pam.d/csm/activelist";
const char* CSM_ACTIVELIST_SWAP = "/etc/pam.d/csm/activelist.swp";

#define STATE_NAME "AllocationAgentUpdateState:"

bool AllocationAgentUpdateState::HandleNetworkMessage(
    const csm::network::MessageAndAddress content,
    std::vector<csm::daemon::CoreEvent*>& postEventList,
    csm::daemon::EventContextHandlerState_sptr& ctx ) 
{
    LOG( csmapi, trace ) << STATE_NAME ":HandleNetworkMessage: Enter";

    // Return status of this function.
    bool success = false;
    
    // EARLY RETURN This needs to invoke the error handler NOT through the aggregator.
    if ( content.GetAddr()->GetAddrType() != csm::network::CSM_NETWORK_TYPE_PTP )
    {
        //LOG(csmapi,error) << STATE_NAME " Expecting a PTP connection.";
        ctx->SetErrorCode(CSMERR_BAD_ADDR_TYPE);
        ctx->SetErrorMessage("Message: This API may not be called from a Compute Node");
        return false;
    }

    // EARLY RETURN
    if ( content._Msg.GetDataLen() == 0  )
    {
        ctx->SetErrorCode(CSMERR_PAYLOAD_EMPTY);
        ctx->SetErrorMessage("Message: Payload received was empty on " + 
            csm::daemon::Configuration::Instance()->GetHostname());
        return false;
    }

    // Receive the payload after we know this is a valid invocation.
    csmi_allocation_mcast_payload_request_t *allocation;
    
    // EARLY RETURN
    if( csm_deserialize_struct( csmi_allocation_mcast_payload_request_t, &allocation, 
            content._Msg.GetData().c_str(), content._Msg.GetDataLen() ) != 0 )
    {
        ctx->SetErrorCode(CSMERR_MSG_UNPACK_ERROR);
        ctx->SetErrorMessage("Message: Unable to unpack allocation struct");
        return false;
    }

    // Generate the response.
    csmi_allocation_mcast_payload_response_t *response;
    csm_init_struct_ptr(csmi_allocation_mcast_payload_response_t, response);
    
    // Allocate memory for the detailed per gpu data
    csm_init_struct_ptr(csmi_allocation_gpu_metrics_t, response->gpu_metrics);
    response->gpu_metrics->num_gpus = 0;
    response->gpu_metrics->num_cpus = 0;

    // setup the response.
    response->hostname = 
        strdup( csm::daemon::Configuration::Instance()->GetHostname().c_str() );
    response->create = allocation->create;
    
    // The create flag was set initialize the node.
    // Else, this is an instruction to revert the node.
    if ( allocation->create )
    {
        success = InitNode( allocation, response, postEventList, ctx ); 
    }
    else
    {
        success = RevertNode( allocation, response, postEventList, ctx );
    }
    
    // If the initialization or reversion was successful reply to the master daemon.
    if ( success || response->error_code != CSMI_SUCCESS )
    {
        // Return the results to the Master via the Aggregator.
        char *buffer          = nullptr;
        uint32_t bufferLength = 0;
        csm_serialize_struct( csmi_allocation_mcast_payload_response_t, response, 
            &buffer, &bufferLength );

        if( buffer )
        {
            // Push the reply through the aggregator.
            this->PushReply(
                buffer,
                bufferLength,
                ctx,
                postEventList,
                true);

            free( buffer );

            LOG(csmapi,info) << ctx->GetCommandName() << ctx <<
                "Allocation ID: "      << allocation->allocation_id << 
                "; Primary Job Id: "   << allocation->primary_job_id << 
                "; Secondary Job Id: " << allocation->secondary_job_id <<
                "; Error Code: "       << response->error_code << 
                "; Error Message: "    << response->error_message << 
                "; Message: Agent completed;";
        }
        else
        {
            ctx->SetErrorCode(CSMERR_MSG_PACK_ERROR);
            ctx->SetErrorMessage("Message: Unable pack response to the Master Daemon;");
            success = false; 
        }
    }

    // Clean up the struct.
    csm_free_struct_ptr(csmi_allocation_mcast_payload_request_t , allocation );
    csm_free_struct_ptr(csmi_allocation_mcast_payload_response_t, response   );

    LOG( csmapi, trace ) << STATE_NAME ":HandleNetworkMessage: Exit";

    return success;
}

bool AllocationAgentUpdateState::InitNode( 
    csmi_allocation_mcast_payload_request_t *payload,
    csmi_allocation_mcast_payload_response_t *respPayload,
    std::vector<csm::daemon::CoreEvent*>& postEventList,
    csm::daemon::EventContextHandlerState_sptr& ctx )
{ 
    LOG( csmapi, trace ) << STATE_NAME ":InitNode: Enter ";

    // Track the revert status.
    bool success = true;
     
    // First notify the start of the job.
    //openlog("csm", LOG_CONS | LOG_NDELAY | LOG_PERROR | LOG_PID, LOG_USER);
    openlog("csmd", LOG_NDELAY , LOG_USER);
    syslog(LOG_INFO, "Allocation %lu create: primary_job_id=%lu secondary_job_id=%u",
           payload->allocation_id, payload->primary_job_id, payload->secondary_job_id);
    closelog();
  
    LOG(csmapi, info) << ctx << "Allocation ID: " << payload->allocation_id << 
        "; Primary Job Id: " << payload->primary_job_id << 
        "; Secondary Job Id: " << payload->secondary_job_id <<
        "; Message: Allocation enter running;";
    
    // 0. Get the data before any operations.
    // TODO Should this trigger an allocation failure on failure?
    DataAggregators(respPayload);

    // TODO should this fail the allocation?
    csm::daemon::INV_DCGM_ACCESS::GetInstance()->StartAllocationStats(payload->allocation_id);

    // 1. Create the cgroup for the allocation.
    #if !defined CSM_MULTI_COMPUTE_PER_NODE
    try
    {
        LOG( csmapi, info ) <<  ctx << "Allocation ID: " << payload->allocation_id <<
            "; Message: Opening cgroups;";
        
        csm::daemon::helper::CGroup cgroup = 
                csm::daemon::helper::CGroup(payload->allocation_id);

        if ( cgroup.IsEnabled() )
        {
            // If the payload was not for a shared allocation
            if (payload->shared != CSM_TRUE)
            {
                // Clear any remaining cgroups, if isolate cores is set to zero remove the system cgroup.
                cgroup.ClearCGroups( payload->isolated_cores == 0 );
                cgroup.SetupCGroups( payload->isolated_cores, payload->smt_mode);
            }
            else 
            {
                cgroup.SetupCGroups( 0 );
                // TODO num processors and num gpus.
                cgroup.ConfigSharedCGroup( payload->projected_memory, payload->num_gpus, payload->num_processors );
            }
        }

        LOG( csmapi, info ) <<  ctx << "Allocation ID: " << payload->allocation_id <<
            "; Message: Opened cgroups;";
    }
    catch(const csm::daemon::helper::CSMHandlerException& e)
    {
        // TODO is there a way to get a more granular error code?
        std::string error = "Message: ";
        error.append(e.what());
        ctx->SetErrorMessage(error);
        ctx->SetErrorCode(CSMERR_CGROUP_FAIL);
        
        // Push a RAS Event to flag the node.
        this->PushRASEvent(ctx, postEventList, CSM_RAS_MSG_ID_CGROUP_CREATE_FAILURE,
            respPayload->hostname, ctx->GetErrorMessage(), 
            "rc=" + std::to_string(ctx->GetErrorCode()));
        
        return false; // TODO Is this the correct behavior?
    }
    catch(const std::exception& e)
    {
        std::string error = "Message: ";
        error.append(e.what());
        ctx->SetErrorMessage(error);
        ctx->SetErrorCode(CSMERR_CGROUP_FAIL);
        
        // Push a RAS Event to flag the node.
        this->PushRASEvent(ctx, postEventList, CSM_RAS_MSG_ID_CGROUP_CREATE_FAILURE,
            respPayload->hostname, ctx->GetErrorMessage(), 
            "rc=" + std::to_string(ctx->GetErrorCode()));

        return false; // TODO Is this the correct behavior?
    }
    #endif
    
    // 2. Register the allocation.
    // TODO Error code for registration failing?
    RegisterAllocation(payload->allocation_id, payload->user_name);
   
    // 3. Export Environment Variables.
    csm_export_env( 
        payload->allocation_id, 
        payload->primary_job_id, 
        payload->secondary_job_id, 
        payload->user_name )

    // 4. Run the Prolog.
    // EARLY RETURN!
    
    // TODO Test for epilog.
    LOG( csmapi, info ) <<  ctx << "Allocation ID: " << payload->allocation_id <<
        "; user_flags: " << payload->user_flags <<
        "; system_flags: " << payload->system_flags << 
        "; Message: Prolog start;";

    success = csm::daemon::helper::ExecutePrivileged(
                payload->user_flags, payload->system_flags, ctx, true, false );

    if ( success )
    {
        LOG( csmapi, info ) << ctx << "Allocation ID: " << payload->allocation_id <<
            "; user_flags: " << payload->user_flags <<
            "; system_flags: " << payload->system_flags << 
            "; Message: Prolog success;";
    }
    else 
    {
        switch (ctx->GetErrorCode())
        {
            case CSMERR_EPILOG_PROLOG_COLLISION:
                PROLOG_RAS(CSM_RAS_EPILOG_PROLOG_COLLISION)
                break;
            case CSMERR_EPILOG_EPILOG_COLLISION:
                PROLOG_RAS(CSM_RAS_EPILOG_EPILOG_COLLISION)
                break;
            case CSMERR_PROLOG_EPILOG_COLLISION:
                PROLOG_RAS(CSM_RAS_PROLOG_EPILOG_COLLISION)
                break;
            case CSMERR_PROLOG_PROLOG_COLLISION:
                PROLOG_RAS(CSM_RAS_PROLOG_PROLOG_COLLISION)
                break;
            default:
                break;
        }

        LOG( csmapi, error ) << ctx << "Allocation ID: " << payload->allocation_id <<
            "; user_flags: " << payload->user_flags <<
            "; system_flags: " << payload->system_flags << 
            "; Message: Prolog failure;";

        respPayload->error_code = (csmi_cmd_err_t)ctx->GetErrorCode();
        respPayload->error_message = strdup(ctx->GetErrorMessage().c_str());
    }
    
    // 5. Get the data that can be set in the prolog.
    // TODO should a failure trigger an error code?
    AfterPrologDataAggregators(respPayload);

    // Set the Daemon Run Mode interaction.
    postEventList.push_back( csm::daemon::helper::CreateJobStartSystemEvent(ctx) );

    LOG( csmapi, trace ) << STATE_NAME ":InitNode: Exit";
    
    return success;
}

bool AllocationAgentUpdateState::RevertNode( 
        csmi_allocation_mcast_payload_request_t *payload,
        csmi_allocation_mcast_payload_response_t *respPayload,
    std::vector<csm::daemon::CoreEvent*>& postEventList,
    csm::daemon::EventContextHandlerState_sptr& ctx )
{
    LOG( csmapi, trace ) << STATE_NAME ":RevertNode: Enter";
    
    // Log that a "delete" operation is happening.
    openlog("csmd", LOG_NDELAY , LOG_USER);
    syslog(LOG_INFO, "Allocation %lu delete: primary_job_id=%lu secondary_job_id=%u",
           payload->allocation_id, payload->primary_job_id, payload->secondary_job_id);
    closelog();
    
    LOG(csmapi, info) << ctx << "Allocation ID: " << payload->allocation_id << 
        "; Primary Job Id: " << payload->primary_job_id << 
        "; Secondary Job Id: " << payload->secondary_job_id <<
        "; Message: Allocation leave running;";
    
    // Set the Daemon Run Mode interaction.
    postEventList.push_back( csm::daemon::helper::CreateJobEndSystemEvent(ctx) );
    
    // 0. Export Environment Variables.
    csm_export_env( 
        payload->allocation_id, 
        payload->primary_job_id, 
        payload->secondary_job_id, 
        payload->user_name)
    
    // 1. Executes epilog.
    LOG( csmapi, info ) <<  ctx << "Allocation ID: " << payload->allocation_id <<
        "; user_flags: " << payload->user_flags <<
        "; system_flags: " << payload->system_flags << 
        "; Message: Epilog start;";

    bool success = csm::daemon::helper::ExecutePrivileged(
                payload->user_flags, payload->system_flags, ctx, false, false );

    if ( success )
    {
        LOG( csmapi, info ) << ctx << "Allocation ID: " << payload->allocation_id <<
            "; user_flags: " << payload->user_flags <<
            "; system_flags: " << payload->system_flags << 
            "; Message: Epilog success;";
    }
    else 
    {

        switch (ctx->GetErrorCode())
        {
            case CSMERR_EPILOG_PROLOG_COLLISION:
                PROLOG_RAS(CSM_RAS_EPILOG_PROLOG_COLLISION)
                break;
            case CSMERR_EPILOG_EPILOG_COLLISION:
                PROLOG_RAS(CSM_RAS_EPILOG_EPILOG_COLLISION)
                break;
            case CSMERR_PROLOG_EPILOG_COLLISION:
                PROLOG_RAS(CSM_RAS_PROLOG_EPILOG_COLLISION)
                break;
            case CSMERR_PROLOG_PROLOG_COLLISION:
                PROLOG_RAS(CSM_RAS_PROLOG_PROLOG_COLLISION)
                break;
            default:
                break;
        }

        LOG( csmapi, error ) << ctx << "Allocation ID: " << payload->allocation_id <<
            "; user_flags: " << payload->user_flags <<
            "; system_flags: " << payload->system_flags << 
            "; Message: Epilog failure;";

        respPayload->error_code = (csmi_cmd_err_t)ctx->GetErrorCode();
        respPayload->error_message = strdup(ctx->GetErrorMessage().c_str());
    }

    // 2. Remove the allocation from the active list.
    // TODO Should a failure be reported?
    RemoveAllocation(payload->allocation_id);
    
    // 3. Remove the cgroup.
    #if !defined CSM_MULTI_COMPUTE_PER_NODE
    try
    {
        LOG( csmapi, info ) <<  ctx << "Allocation ID: " << payload->allocation_id <<
            "; Message: Closing cgroups;";

        csm::daemon::helper::CGroup cgroup = 
                csm::daemon::helper::CGroup(payload->allocation_id);

        if ( cgroup.IsEnabled() )
        {
            respPayload->cpu_usage = cgroup.GetCPUUsage();       // Agregate the cpu usage before leaving.
            respPayload->memory_max = cgroup.GetMemoryMaximum(); // Get the high water mark of the memory usage.

            // Get detailed cpu_usage for each physical core
            std::vector<int64_t> cpu_usage;
            cgroup.GetDetailedCPUUsage(cpu_usage);           
 
            respPayload->gpu_metrics->num_cpus = cpu_usage.size();

            // Allocate memory for the per cpu arrays
            respPayload->gpu_metrics->cpu_usage = (int64_t*)calloc(respPayload->gpu_metrics->num_cpus, sizeof(int64_t));

            // Copy cpu metrics into response payload
            std::copy(cpu_usage.begin(), cpu_usage.end(), respPayload->gpu_metrics->cpu_usage);  

            cgroup.DeleteCGroups( true ); // Delete the system cgroup as well.
        }

        LOG( csmapi, info ) <<  ctx << "Allocation ID: " << payload->allocation_id <<
            "; Message: Closed cgroups;";
    }
    catch(const csm::daemon::helper::CSMHandlerException& e)
    {
        std::string error = "Message: ";
        error.append(e.what());
        respPayload->error_code = CSMERR_CGROUP_FAIL;
        respPayload->error_message = strdup(error.c_str());
        
        // Push a RAS Event to flag the node.
        this->PushRASEvent(ctx, postEventList, CSM_RAS_MSG_ID_CGROUP_DELETE_FAILURE,
            respPayload->hostname, ctx->GetErrorMessage(), 
            "rc=" + std::to_string(ctx->GetErrorCode()));

        success = false; // TODO Is this the correct behavior?
    }
    catch(const std::exception& e)
    {
        std::string error = "Message: ";
        error.append(e.what());
        respPayload->error_code = CSMERR_CGROUP_FAIL;
        respPayload->error_message = strdup(error.c_str());
        
        // Push a RAS Event to flag the node.
        this->PushRASEvent(ctx, postEventList, CSM_RAS_MSG_ID_CGROUP_DELETE_FAILURE,
            respPayload->hostname, ctx->GetErrorMessage(), 
            "rc=" + std::to_string(ctx->GetErrorCode()));

        success = false; // TODO Is this the correct behavior?
    }
    #endif
    
    // 4. In a delete get a snapshot after everything.
    // If the runtime is present and less than the total uptime of the node gather the data.
    if ( payload->runtime > 0 && payload->runtime < csm::daemon::helper::GetUptime())
    {
        DataAggregators(respPayload);
        std::vector<int32_t> gpu_id;
        std::vector<int64_t> max_gpu_memory;
        std::vector<int64_t> gpu_usage;


        bool gpu_usage_success = csm::daemon::INV_DCGM_ACCESS::GetInstance()->StopAllocationStats(
            payload->allocation_id, respPayload->gpu_usage, gpu_id, max_gpu_memory, gpu_usage);
        if ( gpu_usage_success == false )
        {
            respPayload->gpu_usage = -1;
        }
        else
        {
            if ((gpu_id.size() == max_gpu_memory.size()) && (gpu_id.size() == gpu_usage.size()))
            {
                respPayload->gpu_metrics->num_gpus = gpu_id.size();

                // Allocate memory for the per gpu arrays
                respPayload->gpu_metrics->gpu_id         = (int32_t*)calloc(respPayload->gpu_metrics->num_gpus, sizeof(int32_t));
                respPayload->gpu_metrics->gpu_usage      = (int64_t*)calloc(respPayload->gpu_metrics->num_gpus, sizeof(int64_t));
                respPayload->gpu_metrics->max_gpu_memory = (int64_t*)calloc(respPayload->gpu_metrics->num_gpus, sizeof(int64_t));

                // Copy metrics into response payload
                std::copy(gpu_id.begin(), gpu_id.end(), respPayload->gpu_metrics->gpu_id);  
                std::copy(gpu_usage.begin(), gpu_usage.end(), respPayload->gpu_metrics->gpu_usage);  
                std::copy(max_gpu_memory.begin(), max_gpu_memory.end(), respPayload->gpu_metrics->max_gpu_memory);  
            }
            else
            {
                LOG( csmapi, warning ) <<  "Allocation ID: " << payload->allocation_id <<
                    "; Message: detected size discrepancy in gpu metric vector sizes;";
            }
        }
    }

    LOG( csmapi, trace ) << STATE_NAME ":RevertNode: Exit";
    return success;
}

void AllocationAgentUpdateState::DataAggregators(
    csmi_allocation_mcast_payload_response_t *payload)
{
    try
    {
        // Grab any cromulent OCC Accounting.
        csm::daemon::helper::GetOCCAccounting( payload->energy, payload->pc_hit, payload->gpu_energy);
        csm::daemon::helper::GetIBUsage  ( payload->ib_rx,     payload->ib_tx      );
        csm::daemon::helper::GetGPFSUsage( payload->gpfs_read, payload->gpfs_write );
    }
    catch( const std::exception &e )
    {
        LOG( csmapi, error ) << STATE_NAME ":DataAggregators Unhandled Exception: " << e.what();
    }
}

void AllocationAgentUpdateState::AfterPrologDataAggregators(
    csmi_allocation_mcast_payload_response_t *payload)
{
    try
    {
        payload->power_cap = csm::daemon::helper::GetPowerCapacity();
        payload->ps_ratio  = csm::daemon::helper::GetPowerShiftRatio();
    }
    catch( const std::exception &e )
    {
        LOG( csmapi, error ) << 
            STATE_NAME ":AfterPrologDataAggregators Unhandled Exception: " << e.what();
    }
}

void AllocationAgentUpdateState::HandleError(
    csm::daemon::EventContextHandlerState_sptr& ctx,
    const csm::daemon::CoreEvent &aEvent,
    std::vector<csm::daemon::CoreEvent*>& postEventList,
    bool byAggregator )
{ 
    // Append the hostname to the start of the error message.
    // FIXME Workaround for Beta 1
    // ============================================================================
    ctx->SetErrorMessage(csm::daemon::Configuration::Instance()->GetHostname() + "; " + ctx->GetErrorMessage());
    // ============================================================================ 

        // FIXME Temporary fix for beta 1!
    // If this was a CSMERR_BAD_ADDR_TYPE don't talk through the aggregator.
    // Otherwise return the error through the aggregator.
    if ( ctx->GetErrorCode() == CSMERR_BAD_ADDR_TYPE )
    {
        CSMIHandlerState::DefaultHandleError( ctx, aEvent, postEventList, false );
    }
    else
    {
        CSMIHandlerState::DefaultHandleError( ctx, aEvent, postEventList, true );
    }
} 

int AllocationAgentUpdateState::RegisterAllocation( int64_t allocationId, const char* username, bool shared )
{ 
    LOG( csmapi, trace ) << STATE_NAME ":RegisterAllocation; Enter";

    // Lock the activeListMutex
    std::lock_guard<std::mutex> lock(_ActiveListMutex);

    // Initialize the strings and codes.
    int errorCode = 0;
    std::string allocationString(username);
    allocationString.append(";").append(std::to_string(allocationId)).append("\n");

    std::ofstream activelistStream(CSM_ACTIVELIST);
    try
    {
        activelistStream << allocationString;
    
        // Close the stream so it can be swapped.
        activelistStream.close();

    } catch (const std::ifstream::failure& e){
        LOG( csmapi, error ) <<  "Allocation ID: "
            << std::to_string(allocationId) << 
            "; Message: Unable to register allocation with daemon; Extended Message: "
            << e.what();
        errorCode = 1;
    }

    LOG( csmapi, trace ) << STATE_NAME ":RegisterAllocation; Exit";
    
    return errorCode;
}

int AllocationAgentUpdateState::RemoveAllocation( int64_t allocationId )
{
    LOG( csmapi, trace ) << STATE_NAME ":RemoveAllocation; Enter";

    // Lock the activeListMutex
    std::lock_guard<std::mutex> lock(_ActiveListMutex);

    // Error code for tracking.
    int errorCode = 0;
    std::string aId = std::to_string(allocationId);

    std::ifstream activelistStream(CSM_ACTIVELIST);
    std::ofstream activelistSwapStream(CSM_ACTIVELIST_SWAP);

    try
    {
        // If the stream was open continue.
        if( activelistStream.is_open() && activelistSwapStream.is_open())
        {
            // Only write the still active nodes to the swap file.
            std::string line;
            while ( std::getline( activelistStream, line ) )
            {
                int delim = line.find(";") + 1;
                if ( line.compare(delim, std::string::npos, aId ) != 0 )
                {
                    activelistSwapStream << line << "\n";
                }
            }

            // Close the stream so it can be swapped.
            activelistStream.close();
            activelistSwapStream.close();
            
            // Swap the temporary list with the official one.
            std::remove(CSM_ACTIVELIST);
            if ( std::rename(CSM_ACTIVELIST_SWAP, CSM_ACTIVELIST) )
                LOG( csmapi, warning ) <<  "Allocation ID: " << std::to_string(allocationId)
                << "; Message: Activelist couldn't be swapped;";
        }
        else
        {
            LOG( csmapi, warning ) <<  "Allocation ID: "
                << std::to_string(allocationId) << 
                "; Message: Unable to remove allocation with daemon;";
        }

    } catch (const std::ifstream::failure& e){
        LOG( csmapi, error ) <<  "Allocation ID: "
            << std::to_string(allocationId) << 
            "; Message: Unable to remove allocation with daemon; Extended Message: "
            << e.what();
        errorCode = 1;
    }

    LOG( csmapi, trace ) << STATE_NAME ":RemoveAllocation; Exit";

    return errorCode;
}
