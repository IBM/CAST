/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/CSMIAllocationStepQueryActiveAll.cc

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

#include "CSMIAllocationStepQueryActiveAll.h"
#define STATE_NAME "CSMIAllocationStepQueryActiveAll:"

// Use this to make changing struct names easier.
#define INPUT_STRUCT csm_allocation_step_query_active_all_input_t
#define OUTPUT_STRUCT csm_allocation_step_query_active_all_output_t
#define STEP_STRUCT csmi_allocation_step_t

bool CSMIAllocationStepQueryActiveAll::CreatePayload(
    const std::string& arguments,
    const uint32_t len,
    csm::db::DBReqContent **dbPayload,
    csm::daemon::EventContextHandlerState_sptr& ctx )
{
    LOG( csmapi, trace ) << STATE_NAME ":CreatePayload: Enter";

    // Deserialize the struct.
    INPUT_STRUCT* input = nullptr;

    // If the struct was found formulate the query.
	if( csm_deserialize_struct( INPUT_STRUCT, &input, arguments.c_str(), len ) == 0 )
	{
        std::string stmt = "SELECT "
                "step_id, allocation_id, begin_time, status, executable, working_directory, "
                "argument, environment_variable, num_nodes, num_processors, "
                "num_gpus, projected_memory, num_tasks, user_flags "
            "FROM csm_step "
            "WHERE allocation_id = $1::bigint "
            "ORDER BY step_id ASC NULLS LAST";

        const int paramCount = 1;
        csm::db::DBReqContent *dbReq = new csm::db::DBReqContent( stmt, paramCount );
        dbReq->AddNumericParam<>( input->allocation_id );
        *dbPayload = dbReq;

		csm_free_struct_ptr( INPUT_STRUCT, input );

        LOG( csmapi, trace ) << STATE_NAME ":CreatePayload: Parameterized SQL: " << stmt;
	}
    else
    {
        LOG( csmapi, error ) << STATE_NAME ":CreatePayload: argUnpackFunc failed...";
        LOG( csmapi, trace  ) << STATE_NAME ":CreatePayload: Exit";

        ctx->SetErrorCode(CSMERR_MSG_UNPACK_ERROR);
        ctx->SetErrorMessage("Unable to build the query, struct could not be deserialized");
        return false;
    }
	
    LOG( csmapi, trace ) << STATE_NAME ":CreatePayload: Exit";

	return true;
}

bool CSMIAllocationStepQueryActiveAll::CreateByteArray(
    const std::vector<csm::db::DBTuple *>&tuples,
    char **buf, uint32_t &bufLen,
    csm::daemon::EventContextHandlerState_sptr& ctx ) 
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
        output.steps     = (STEP_STRUCT**) malloc( 
            step_count * sizeof(STEP_STRUCT*));

        // Build the array.
        for ( uint32_t step = 0; step < step_count; ++step )
        {
            CreateStepStruct( tuples[step], &output.steps[step] );
        }

        // Package the data for transport.
        csm_serialize_struct( OUTPUT_STRUCT, &output, buf, &bufLen );

        csm_free_struct(OUTPUT_STRUCT, output );

    }
    LOG( csmapi, trace ) << STATE_NAME ":CreateByteArray: Exit";
    
    return true;
}

void CSMIAllocationStepQueryActiveAll::CreateStepStruct(
    csm::db::DBTuple * const & fields,
    STEP_STRUCT **step )
{
    LOG( csmapi, debug ) << STATE_NAME ":CreateStepStruct: Enter";

    if ( fields->nfields != 14 )
    {
        *step = nullptr;
        return;
    }

    STEP_STRUCT *s = nullptr;
    csm_init_struct_ptr( STEP_STRUCT, s );

    s->step_id              = atol(fields->data[0]);
    s->allocation_id        = atol(fields->data[1]);
    s->begin_time           = strdup(fields->data[2]);
	s->status               = (csmi_step_status_t)csm_get_enum_from_string(csmi_step_status_t, fields->data[3]);
    s->executable           = strdup(fields->data[4]);
    s->working_directory    = strdup(fields->data[5]);
    s->argument             = strdup(fields->data[6]);
    s->environment_variable = strdup(fields->data[7]);
    s->num_nodes            = atol(fields->data[8 ]);
    if( s->num_nodes < 0 ) s->num_nodes = 0; // Zero for serialization.
    s->num_processors       = atol(fields->data[9 ]);
    s->num_gpus             = atol(fields->data[10]);
    s->projected_memory     = atol(fields->data[11]);
    s->num_tasks            = atol(fields->data[12]);
    s->user_flags         = strdup(fields->data[13]);

    *step = s; 

    LOG( csmapi, debug ) << STATE_NAME ":CreateStepStruct: Exit";
}

