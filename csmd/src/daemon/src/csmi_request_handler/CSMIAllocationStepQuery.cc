/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/CSMIAllocationStepQuery.cc

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

/* Header for this file. */
#include "CSMIAllocationStepQuery.h"

#include "csmi/include/csmi_type_wm_funct.h"
#include "csmi/include/csmi_type_wm.h"

#include "include/csm_event_type_definitions.h"

#include <inttypes.h>
#include <stdio.h>

#define STATE_NAME "CSMIAllocationStepQuery"

// Use this to make changing struct names easier.
#define INPUT_STRUCT csm_allocation_step_query_input_t
#define OUTPUT_STRUCT csm_allocation_step_query_output_t
#define STEP_STRUCT csmi_allocation_step_t
#define HISTORY_STRUCT csmi_allocation_step_history_t

bool CSMIAllocationStepQuery::CreatePayload(
        const std::string& arguments,
        const uint32_t len,
        csm::db::DBReqContent **dbPayload,
        csm::daemon::EventContextHandlerState_sptr ctx ) 
{
    LOG(csmapi, trace) << STATE_NAME ":CreatePayload: Enter";
	
	// Unpack the buffer.
	INPUT_STRUCT* input = nullptr; 
    
    if( csm_deserialize_struct( INPUT_STRUCT, &input, arguments.c_str(), len ) == 0 )
    {
        int paramCount = 1;

        std::string stmt  = "SELECT "
                "step_id as step_id, allocation_id as allocation_id, "
                "null as archive_history_time, argument as argument, "         
                "begin_time as begin_time, null as cpu_stats, "            
                "null as end_time, environment_variable as environment_variable, " 
                "null as error_message, executable as executable, "
                "null as exit_status, null as gpu_stats, "
                "null as io_stats, "
                "null as max_memory, "
                "null as memory_stats, "
                "num_gpus as num_gpus, "         
                "projected_memory as projected_memory, num_nodes as num_nodes, "
                "num_processors as num_processors, num_tasks as num_tasks, "
                "status as status,null as omp_thread_limit,"
                "null as total_s_time, null as total_u_time, "
                "user_flags as user_flags, working_directory as working_directory "
            "FROM csm_step "
            "WHERE allocation_id = $1::bigint ";

        if(input->step_id >= 0)
        {
            paramCount++;
            stmt.append("AND step_id = $2::bigint ");
        }
        
        stmt.append("UNION ALL "
            "SELECT "
                "step_id as step_id, allocation_id as allocation_id,"
                "null as archive_history_time, argument as argument,"
                "begin_time as begin_time, cpu_stats as cpu_stats,"
                "end_time as end_time, environment_variable as environment_variable,"
                "error_message as error_message, executable as executable,"
                "exit_status as exit_status, gpu_stats as gpu_stats,"
                "io_stats as io_stats,"
                "max_memory as max_memory,"
                "memory_stats as memory_stats,"
                "num_gpus as num_gpus,"
                "projected_memory as projected_memory, num_nodes as num_nodes,"
                "num_processors as num_processors, num_tasks as num_tasks,"
                "status as status, omp_thread_limit as omp_thread_limit,"
                "total_s_time as total_s_time, total_u_time as total_u_time, "
                "user_flags as user_flags, working_directory as working_directory "
            "FROM csm_step_history WHERE "
                "allocation_id = $1::bigint " );

        if(input->step_id >= 0) stmt.append("AND step_id = $2::bigint ");

        stmt.append("ORDER BY step_id ASC NULLS LAST");

        csm::db::DBReqContent * dbReq = new csm::db::DBReqContent( stmt, paramCount );
        dbReq->AddNumericParam<int64_t>( input->allocation_id );
        if (input->step_id >= 0) dbReq->AddNumericParam<int64_t>( input->step_id );

        *dbPayload = dbReq;
	    csm_free_struct_ptr(INPUT_STRUCT, input);
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

bool CSMIAllocationStepQuery::CreateByteArray(
        const std::vector<csm::db::DBTuple *>&tuples,
        char **buf, uint32_t &bufLen,
        csm::daemon::EventContextHandlerState_sptr ctx )
{
    LOG( csmapi, trace ) << STATE_NAME ":CreateByteArray: Enter";
    
    // Init the buffer
    *buf = nullptr;
    bufLen = 0;

    // Allocate the array to be serialized.
    uint32_t step_count = tuples.size();

    if ( step_count > 0 )
    {
        OUTPUT_STRUCT output;
        csm_init_struct_versioning(&output);
        output.num_steps = step_count;
        output.steps = (STEP_STRUCT**) malloc( step_count * sizeof(STEP_STRUCT*));

        // Build the array.
        for ( uint32_t step = 0; step < step_count; ++step )
        {
            CreateStepStruct( tuples[step], &output.steps[step] );
        }

        // Package the data for transport.
        csm_serialize_struct( OUTPUT_STRUCT, &output, buf, &bufLen );
        csm_free_struct(OUTPUT_STRUCT,output);
    }

    LOG( csmapi, trace ) << STATE_NAME ":CreateByteArray: Exit";
    
    return true;
}


void CSMIAllocationStepQuery::CreateStepStruct(
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

	STEP_STRUCT *s    = nullptr;
    csm_init_struct_ptr(STEP_STRUCT, s);
	
    s->step_id              = strtoll(fields->data[0], nullptr, 10);
    s->allocation_id        = strtoll(fields->data[1], nullptr, 10);
	s->argument             = strdup (fields->data[3]);
	s->begin_time           = strdup (fields->data[4]);
	s->environment_variable = strdup (fields->data[7]);
	s->executable           = strdup (fields->data[9]);
	s->num_gpus             = strtol (fields->data[15], nullptr, 10);
	s->projected_memory     = strtol (fields->data[16], nullptr, 10);
	s->num_nodes            = strtol (fields->data[17], nullptr, 10);
	s->num_processors       = strtol (fields->data[18], nullptr, 10);
	s->num_tasks            = strtol (fields->data[19], nullptr, 10);
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
	    h->exit_status          = atoi   (fields->data[10]);
	    h->gpu_stats            = strdup (fields->data[11]);
	    h->io_stats             = strdup (fields->data[12]);
	    h->max_memory           = strtoll (fields->data[13], nullptr, 10);
	    h->memory_stats         = strdup (fields->data[14]);
	    h->omp_thread_limit    = strdup (fields->data[21]);
	    h->total_s_time         = strtod(fields->data[22], nullptr);
	    h->total_u_time         = strtod(fields->data[23], nullptr);

        s->history = h;
    }
	
	*step = s;

    LOG( csmapi, debug ) << STATE_NAME ":CreateStepStruct: Exit";
}

