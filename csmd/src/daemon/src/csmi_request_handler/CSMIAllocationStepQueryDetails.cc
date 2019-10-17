/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/CSMIAllocationStepQueryDetails.cc

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
/*
* Author: Nick Buonarota
* Email: nbuonar@us.ibm.com
*/

//C and C++ includes
#include <stdio.h>
#include <inttypes.h>
//CSM API includes
#include "CSMIAllocationStepQueryDetails.h"
#include "csmi_stateful_db/CSMIStatefulDBRecvSend.h"

//CSM Infrastructure Includes
#include "include/csm_event_type_definitions.h"

#define STATE_NAME "CSMIAllocationStepQueryDetails:"

// Use this to make changing struct names easier.
#define INPUT_STRUCT csm_allocation_step_query_details_input_t
#define OUTPUT_STRUCT csm_allocation_step_query_details_output_t
#define STEP_STRUCT csmi_allocation_step_t
#define HISTORY_STRUCT csmi_allocation_step_history_t

//========================================================================================
#define EXTRA_STATES 1

CSMIAllocationStepQueryDetails::CSMIAllocationStepQueryDetails(csm::daemon::HandlerOptions& options) :
    CSMIStatefulDB(CSM_CMD_allocation_step_query_details, options, STATEFUL_DB_DONE + EXTRA_STATES)
{
    const uint32_t final_state = STATEFUL_DB_DONE + EXTRA_STATES;
    uint32_t current_state = STATEFUL_DB_RECV_DB;
    uint32_t next_state = current_state + 1;

    SetState( current_state,
        new StatefulDBRecvSend<CreateResponsePayload>(
            next_state,
            final_state,
            final_state ) );
}
#undef EXTRA_STATES

bool CSMIAllocationStepQueryDetails::RetrieveDataForPrivateCheck(
        const std::string& arguments, 
        const uint32_t len, 
        csm::db::DBReqContent **dbPayload,
        csm::daemon::EventContextHandlerState_sptr& ctx )
{
    //TODO: Add in history table check as well. 
    LOG( csmapi, trace ) << STATE_NAME ":RetrieveDataForPrivateCheck: Enter";
    
    // Unpack the buffer.
    INPUT_STRUCT* input = nullptr;

    if( csm_deserialize_struct(INPUT_STRUCT, &input, arguments.c_str(), len) == 0 )
    {
        std::string stmt = "SELECT user_id "
                "FROM csm_allocation "
                "WHERE allocation_id = $1::bigint "
            "UNION ALL "
                "SELECT user_id "
                "FROM csm_allocation_history "
                "WHERE allocation_id = $1::bigint";

        const int paramCount = 1;
        csm::db::DBReqContent *dbReq = new csm::db::DBReqContent( stmt, paramCount );
        dbReq->AddNumericParam<int64_t>( input->allocation_id );
        *dbPayload = dbReq;

        ctx->SetErrorMessage("Allocation ID: " +  std::to_string(input->allocation_id) + ";");
        csm_free_struct_ptr(INPUT_STRUCT, input);

        LOG( csmapi, trace ) << STATE_NAME ":RetrieveDataForPrivateCheck: Parameterized SQL: " 
            << stmt;
    }
    else
    {
        LOG( csmapi, error ) << STATE_NAME ":RetrieveDataForPrivateCheck: Deserialization failed";
        LOG( csmapi, trace  ) << STATE_NAME ":RetrieveDataForPrivateCheck: Exit";
        
        ctx->SetErrorCode( CSMERR_MSG_UNPACK_ERROR);
        ctx->SetErrorMessage("Unable to build the private check query, "
            "struct could not be deserialized");
        return false;
    }

    LOG( csmapi, trace ) << STATE_NAME ":RetrieveDataForPrivateCheck: Exit";

    return true;
}

bool CSMIAllocationStepQueryDetails::CompareDataForPrivateCheck(
        const std::vector<csm::db::DBTuple *>& tuples,
        const csm::network::Message &msg,
        csm::daemon::EventContextHandlerState_sptr& ctx)
{
    LOG( csmapi, trace ) << STATE_NAME ":CompareDataForPrivateCheck: Enter";
    
    bool success = false;

    if ( tuples.size() == 1 )
    {
        uint32_t userID = strtoul(tuples[0]->data[0], nullptr, 10);
        success = (userID == msg.GetUserID());

        // If the success failed report it.
        if (success)
        {
            ctx->SetErrorMessage("");
        }
        else
        {
            std::string error = "Allocation is owned by user id ";
            error.append(std::to_string(userID)).append(", user id ");
            error.append(std::to_string(msg.GetUserID())).append(" does not have permission to "
                "access step details;");

            ctx->AppendErrorMessage(error);
        }
    }
    else
    {
        ctx->SetErrorCode(CSMERR_DB_ERROR);
        ctx->AppendErrorMessage("Database check failed for user id " + std::to_string(msg.GetUserID()) + ";" );
    }
    
    LOG( csmapi, trace ) << STATE_NAME ":CompareDataForPrivateCheck: Exit";

    return success;
}

bool CSMIAllocationStepQueryDetails::CreatePayload(
        const std::string& arguments,
        const uint32_t len,
        csm::db::DBReqContent **dbPayload,
        csm::daemon::EventContextHandlerState_sptr& ctx )
{
    // TODO: Old log. Should this be a trace only?
    LOG(csmapi, trace) << STATE_NAME ":CreatePayload: Enter";
    
    // TODO should this live in StatefulDBRecvPrivate?
        
    // Unpack the buffer.
    INPUT_STRUCT* input = nullptr;
    
    if ( csm_deserialize_struct(INPUT_STRUCT, &input, arguments.c_str(), len) == 0 )
    {
        csm::db::DBReqContent *dbReq;

        int paramCount = 1;

        std::string stmt = "(WITH s AS ("
            "SELECT "
                "s.step_id, "
                "s.allocation_id, "
                "CAST(NULL AS DATE) as archive_history_time, "
                "s.argument as argument, "
                "s.begin_time, "
                "CAST(NULL AS TEXT) as cpu_stats, "
                "CAST(NULL AS DATE) as end_time, "
                "s.environment_variable as environment_variable, "
                "CAST(NULL AS TEXT) as error_message, s.executable as executable, "
                "CAST(NULL AS INT) as exit_status, CAST(NULL AS TEXT) as gpu_stats, "   
                "CAST(NULL AS TEXT) as io_stats, "    
                "CAST(NULL AS BIGINT) as max_memory, "  
                "CAST(NULL AS TEXT) as memory_stats, "
                "s.num_gpus as num_gpus, "
                "s.projected_memory as projected_memory, "
                "s.num_nodes as num_nodes, "
                "s.num_processors as num_processors, "
                "s.num_tasks as num_tasks, "
                "s.status as status, "
                "CAST(NULL AS TEXT) as omp_thread_limit, "
                "CAST(NULL AS DOUBLE PRECISION) as total_s_time, "
                "CAST(NULL AS DOUBLE PRECISION) as total_u_time, "
                "s.user_flags as user_flags, s.working_directory as working_directory "
            "FROM csm_step AS s "
            "WHERE s.allocation_id = $1::bigint ";

        if(input->step_id >= 0)
        {
            stmt.append("AND s.step_id = $2::bigint ");
            paramCount++;
        }

        stmt.append( "ORDER BY s.allocation_id, s.step_id) "
            "SELECT * FROM s UNION ALL ( "
                "SELECT "
                    "sh.step_id, "
                    "sh.allocation_id, "
                    "sh.archive_history_time, "
                    "sh.argument as argument, "
                    "sh.begin_time, "
                    "sh.cpu_stats as cpu_stats, "
                    "sh.end_time, "
                    "sh.environment_variable as environment_variable, "
                    "sh.error_message as error_message, sh.executable as executable, "
                    "sh.exit_status as exit_status, sh.gpu_stats as gpu_stats, "
                    "sh.io_stats as io_stats, "
                    "sh.max_memory as max_memory, "
                    "sh.memory_stats as memory_stats, "
                    "sh.num_gpus as num_gpus, "
                    "sh.projected_memory as projected_memory, sh.num_nodes as num_nodes, "
                    "sh.num_processors as num_processors, sh.num_tasks as num_tasks, "
                    "sh.status as status, "
                    "sh.omp_thread_limit,"
                    "sh.total_s_time as total_s_time, "
                    "sh.total_u_time as total_u_time, "
                    "sh.user_flags as user_flags, sh.working_directory as working_directory "
                "FROM csm_step_history AS sh "
                "WHERE sh.allocation_id = $1::bigint ");
        
        if(input->step_id >= 0) stmt.append("AND sh.step_id = $2::bigint ");

        stmt.append("ORDER BY sh.allocation_id, sh.step_id )) "
            "ORDER BY step_id ASC NULLS LAST"); 
                    
        dbReq = new csm::db::DBReqContent( stmt, paramCount );
        dbReq->AddNumericParam<int64_t>( input->allocation_id );
        if(input->step_id >= 0) dbReq->AddNumericParam<int64_t>( input->step_id );

        *dbPayload = dbReq;
        csm_free_struct_ptr(INPUT_STRUCT, input);

        LOG( csmapi, trace ) << STATE_NAME ":CreatePayload: Parameterized SQL: " << stmt;
    }
    else
    {
        LOG( csmapi, error ) << STATE_NAME ":CreatePayload: argUnpackFunc failed...";
        LOG( csmapi, trace  ) << STATE_NAME ":CreatePayload: Exit";
        
        ctx->SetErrorCode( CSMERR_MSG_UNPACK_ERROR);
        ctx->SetErrorMessage("Unable to build the query, struct could not be deserialized");
        return false;
    }

    LOG( csmapi, trace ) << STATE_NAME ":CreatePayload: Exit";

    return true;
}

bool CSMIAllocationStepQueryDetails::CreateResponsePayload(
    const std::vector<csm::db::DBTuple *>&tuples,
    csm::db::DBReqContent **dbPayload,
    csm::daemon::EventContextHandlerState_sptr& ctx)
{
    //i think
    uint32_t step_count = tuples.size();

    if ( step_count > 0 )
    {
        OUTPUT_STRUCT *output = nullptr;
        csm_init_struct_ptr(OUTPUT_STRUCT, output);
        output->num_steps = step_count;

        output->steps = (STEP_STRUCT**) malloc( step_count * sizeof(STEP_STRUCT*));

        // Build the array.
        for ( uint32_t step = 0; step < step_count; ++step )
        {
            CreateStepStruct( tuples[step], &(output->steps[step]) );
        }

        ctx->SetDataDestructor( []( void* data ){  
                free_csm_allocation_step_query_details_output_t( (OUTPUT_STRUCT*) data);
                free(data);
                data=nullptr;
        });
        ctx->SetUserData( output );

        // Build the query for the node names.
        int paramCount  = 1;
        int64_t allocation_id = output->steps[0]->allocation_id;
        int64_t step_id = step_count ==1 ? output->steps[0]->step_id : -1;

        std::string stmt = 
               "SELECT "
                   "sn.step_id, "
                   "array_to_string(array_agg(sn.node_name),',') AS nodelist "
               "FROM csm_step_node sn "
               "WHERE sn.allocation_id = $1::bigint ";
        if(step_id >= 0)
        {
            paramCount++;
            stmt.append("AND sn.step_id = $2::bigint ");
        }

        stmt.append("GROUP BY sn.step_id "
               "UNION "
               "SELECT "
                   "sn.step_id, "
                   "array_to_string(array_agg(sn.node_name),',') AS nodelist "
               "FROM csm_step_node_history sn "
               "WHERE sn.allocation_id = $1::bigint ");
        if(step_id >= 0) stmt.append("AND sn.step_id = $2::bigint ");

        stmt.append("GROUP BY sn.step_id " 
               "ORDER BY step_id ASC") ;

        
        csm::db::DBReqContent * dbReq = new csm::db::DBReqContent( stmt, paramCount );
        dbReq->AddNumericParam<int64_t>( allocation_id );
        if(step_id >= 0) dbReq->AddNumericParam<int64_t>( step_id );

        *dbPayload = dbReq;
    }
    else
    {
        ctx->SetErrorCode(CSMI_NO_RESULTS);
        ctx->SetErrorMessage("No steps found matching the supplied criteria.");
        return false;
    }

    
    return true;
}

bool CSMIAllocationStepQueryDetails::CreateByteArray(
        const std::vector<csm::db::DBTuple *>&tuples,
        char **buf, uint32_t &bufLen,
        csm::daemon::EventContextHandlerState_sptr& ctx )
{   
    LOG( csmapi, trace ) << STATE_NAME ":CreateByteArray: Enter";
    
    // Init the buffer
    *buf = nullptr;
    bufLen = 0;

    OUTPUT_STRUCT *output= nullptr;
    std::unique_lock<std::mutex>dataLock = ctx->GetUserData<OUTPUT_STRUCT*>(&output);

    // tuples.size() is number of unique steps found in this query
    int32_t step_count = tuples.size();
    int32_t num_records_of_unique_steps = tuples.size();
    int32_t num_records_of_steps = output->num_steps;
    //printf("hi.\n");
    // output->num_steps is total records found in this query
    //printf("output->num_steps: %i\n", output->num_steps);
    //printf("step_count: %i\n", step_count);

    // This assumes a 1:1 parity between the first and second queries, some checks are made to ensure this.
    // NOTE: there is another wierd edge case here...
    // where if the total number of records returned matches the unique number of steps in the allocation. it will blow up. 
    // ie there are 3 unique steps in allocation 1
    // steps: 1,2,3
    // but there is also 3 step 1s
    if ( step_count > 0  && output->num_steps == step_count )
    {
        // printf("hello.\n");
        // printf("output->num_steps: %i\n", output->num_steps);
        // printf("step_count: %i\n", step_count);
        for (int32_t i = 0; i < step_count; ++i)
        {
            csm::db::DBTuple * const & fields = tuples[i];
            if (fields->nfields != 2 ) continue;
            
            // Aggregate the nodes.
            int64_t step_id = strtoll(fields->data[0], nullptr, 10);
            int32_t num_nodes = output->steps[i]->num_nodes;
            if (step_id == output->steps[i]->step_id && num_nodes > 0)
            {
                char** nodes = (char**)malloc( sizeof(char*) * output->steps[i]->num_nodes );

                int32_t node = 0;
                char *saveptr;
                char *nodeStr = strtok_r(fields->data[1], ",", &saveptr);

                while ( nodeStr != NULL && node < num_nodes )
                {
                    nodes[node++] = strdup(nodeStr);
                    nodeStr = strtok_r(NULL, ",", &saveptr);
                }
                while( node < num_nodes)
                {
                    nodes[i++] = strdup("N/A");
                }
                output->steps[i]->compute_nodes = nodes;
            }
        }
    }else if(step_count > 0  && output->num_steps > step_count)
    {
        //This is the edge case from the issue 770 

        //edge case of a situation that will never happen in production
        // the following code will prevent a seg fault but also display all the records returned from the database.
        // best of both worlds
        // doesnt hold records back from customer (duplicate steps on an allocation -- ie, 2 step #1s in the history table)
        // doesnt seg fault

        // printf("hello 2.\n");
        // printf("output->num_steps: %i\n", output->num_steps);
        // printf("num_records_of_unique_steps: %i\n", num_records_of_unique_steps);
        // printf("num_records_of_steps: %i\n", num_records_of_steps);
        // printf("step_count: %i\n", step_count);

        //set num steps  equal to the step count
        //step_count = output->num_steps;
        //don't do this in the good case, only the edge case. 

        // printf("output->num_steps: %i\n", output->num_steps);
        // printf("step_count: %i\n", step_count);

        // loop i for the unique number of steps found
        for (int32_t i = 0; i < num_records_of_unique_steps; ++i)
        {
            //printf("i: %i\n", i);
            csm::db::DBTuple * const & fields = tuples[i];
            if (fields->nfields != 2 ) continue;
            
            // Aggregate the nodes.
            int64_t step_id = strtoll(fields->data[0], nullptr, 10);
            //record number
            int32_t num_nodes = output->steps[i]->num_nodes;
            // printf("step_id: %li\n", step_id);
            // printf("num_nodes: %i\n", num_nodes);

            int32_t j = 0;
            //loop j for the total number of steps found, non unique
            for(j = 0; j < num_records_of_steps; j++)
            {
                // printf("j: %i\n", j);
                // printf("step_id: %li\n", step_id);
                // printf("output->steps[j]->step_id: %li\n", output->steps[j]->step_id);

                // i think this is the faulty line -- because it won't make the compute nodes if there is no match in the previous if
                if (step_id == output->steps[j]->step_id && num_nodes > 0)
                {
                    //printf("match. \n");
                    char** nodes = (char**)malloc( sizeof(char*) * output->steps[j]->num_nodes );

                    int32_t node = 0;
                    char *saveptr;
                    //names of the nodes
                    char *nodeStr = strtok_r(fields->data[1], ",", &saveptr);

                    while ( nodeStr != NULL && node < num_nodes )
                    {
                        nodes[node++] = strdup(nodeStr);
                        nodeStr = strtok_r(NULL, ",", &saveptr);
                    }
                    while( node < num_nodes)
                    {
                        nodes[i++] = strdup("N/A");
                    }
                    output->steps[j]->compute_nodes = nodes;
                }
            }
        }
    }elseif(step_count > 0  && output->num_steps < step_count)
    {
        //another case in 770
        // but where the step count is greater than num steps

        // doesnt show in normal database query

        // i expanded out to here as to not disturb the main fix.
        // the same fix seems to fix this error. 
        // but since its a different cause with the same effect. 
        // i decided to pull this out as if we maybe want to adress the true cause in the future, the seperation has already been done.

        // i beleive that "step count" may be related to the total number of steps found in an allocation. 
        // not necessarily the total number of steps found matching our current query. 

        // printf("hello 3.\n");
        // printf("output->num_steps: %i\n", output->num_steps);
        // printf("num_records_of_unique_steps: %i\n", num_records_of_unique_steps);
        // printf("num_records_of_steps: %i\n", num_records_of_steps);
        // printf("step_count: %i\n", step_count);

        //set num steps  equal to the step count
        //step_count = output->num_steps;
        //don't do this in the good case, only the edge case. 

        //printf("output->num_steps: %i\n", output->num_steps);
        //printf("step_count: %i\n", step_count);

        // loop i for the unique number of steps found

        //actually a change here.... only loong for num of steps found that match
        for (int32_t i = 0; i < num_records_of_steps; ++i)
        {
            //printf("i: %i\n", i);
            csm::db::DBTuple * const & fields = tuples[i];
            if (fields->nfields != 2 ) continue;
            
            // Aggregate the nodes.
            int64_t step_id = strtoll(fields->data[0], nullptr, 10);
            //record number
            int32_t num_nodes = output->steps[i]->num_nodes;
            //printf("step_id: %li\n", step_id);
            //printf("num_nodes: %i\n", num_nodes);

            int32_t j = 0;
            //loop j for the total number of steps found, non unique
            for(j = 0; j < num_records_of_steps; j++)
            {
                //printf("j: %i\n", j);
                //printf("step_id: %li\n", step_id);
                //printf("output->steps[j]->step_id: %li\n", output->steps[j]->step_id);

                // i think this is the faulty line -- because it won't make the compute nodes if there is no match in the previous if
                if (step_id == output->steps[j]->step_id && num_nodes > 0)
                {
                    //printf("match. \n");
                    char** nodes = (char**)malloc( sizeof(char*) * output->steps[j]->num_nodes );

                    int32_t node = 0;
                    char *saveptr;
                    //names of the nodes
                    char *nodeStr = strtok_r(fields->data[1], ",", &saveptr);

                    while ( nodeStr != NULL && node < num_nodes )
                    {
                        nodes[node++] = strdup(nodeStr);
                        nodeStr = strtok_r(NULL, ",", &saveptr);
                    }
                    while( node < num_nodes)
                    {
                        nodes[i++] = strdup("N/A");
                    }
                    output->steps[j]->compute_nodes = nodes;
                }
            }
        }
    }else{
        //test
        //printf("hello else.\n");

    }

    // Package the data for transport.
    csm_serialize_struct( OUTPUT_STRUCT, output, buf, &bufLen );
    dataLock.unlock();
    ctx->SetUserData(nullptr);

    LOG( csmapi, trace ) << STATE_NAME ":CreateByteArray: Enter";
    
    return true;
}

void CSMIAllocationStepQueryDetails::CreateStepStruct(
        csm::db::DBTuple * const & fields,
        STEP_STRUCT **step )
{
    LOG( csmapi, debug ) << STATE_NAME ":CreateStepStruct: Enter";
    
    // Check to verify that the number of fields matches, else exit.
    if ( fields->nfields != 26 )
    {
        LOG(csmapi, error) << "Field Count mismatch " << fields->nfields; 
        *step = nullptr;
        return;
    }

    STEP_STRUCT *s = nullptr;
    csm_init_struct_ptr(STEP_STRUCT, s);

    s->step_id              = strtoll(fields->data[0], nullptr, 10);
    s->allocation_id        = strtoll(fields->data[1], nullptr, 10);
    s->argument             = strdup (fields->data[3]);
    s->begin_time           = strdup (fields->data[4]);
    s->environment_variable = strdup (fields->data[7]);
    s->executable           = strdup (fields->data[9]);
    s->num_gpus             = strtol(fields->data[15], nullptr, 10);
    s->projected_memory     = strtol(fields->data[16], nullptr, 10);
    s->num_nodes            = strtol(fields->data[17], nullptr, 10);
    if ( s->num_nodes < 0 ) s->num_nodes = 0; // Zero for serialization's sake.
    s->num_processors       = strtol(fields->data[18], nullptr, 10);
    s->num_tasks            = strtol(fields->data[19], nullptr, 10);
    s->status               = (csmi_step_status_t)csm_get_enum_from_string(csmi_step_status_t, fields->data[20]);
    s->user_flags           = strdup (fields->data[24]);
    s->working_directory    = strdup (fields->data[25]);
    

    if ( fields->data[6][0] )
    {
        HISTORY_STRUCT *h = (HISTORY_STRUCT*) malloc(sizeof(HISTORY_STRUCT));
        h->archive_history_time = strdup (fields->data[2]);
        h->cpu_stats            = strdup (fields->data[5]);
        h->end_time             = strdup (fields->data[6]);
        h->error_message        = strdup (fields->data[8]);
        h->exit_status          = strtol(fields->data[10], nullptr, 10);
        h->gpu_stats            = strdup (fields->data[11]);
        h->io_stats             = strdup (fields->data[12]);
        h->max_memory           = strtoll (fields->data[13], nullptr, 10);
        h->memory_stats         = strdup (fields->data[14]);
        h->omp_thread_limit     = strdup (fields->data[21]);
        h->total_s_time         = strtod (fields->data[22], nullptr);
        h->total_u_time         = strtod (fields->data[23], nullptr);
                                                       
        s->history  = h;
    }
    
    //// Parse the nodes from csv
    //if ( s->num_nodes > 0 )
    //{
    //    s->compute_nodes = (char**)malloc( sizeof(char*) * s->num_nodes);

    //    int32_t i = 0;
    //    char *saveptr;
    //    char *nodeStr = strtok_r(fields->data[26], ",", &saveptr);

    //    while (nodeStr != NULL && i < s->num_nodes)
    //    {
    //        s->compute_nodes[i++] = strdup(nodeStr);
    //        nodeStr = strtok_r(NULL, ",", &saveptr);
    //    }

    //    while ( i < s->num_nodes )
    //    {
    //        s->compute_nodes[i++] = strdup("N/A");
    //    }
    //}

    *step = s;

    LOG( csmapi, debug ) << STATE_NAME ":CreateStepStruct: Exit";
}

