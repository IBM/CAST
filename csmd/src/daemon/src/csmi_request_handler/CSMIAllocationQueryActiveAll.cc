/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/CSMIAllocationQueryActiveAll.cc
    
    © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#include "CSMIAllocationQueryActiveAll.h"

#define STATE_NAME "CSMIAllocationQueryActiveAll:"
// Use this to make changing struct names easier.
#define INPUT_STRUCT 
#define OUTPUT_STRUCT csm_allocation_query_active_all_output_t
#define ALLOC_STRUCT csmi_allocation_t



bool CSMIAllocationQueryActiveAll::CreatePayload(
        const std::string& arguments,
        const uint32_t len,
        csm::db::DBReqContent **dbPayload,
        csm::daemon::EventContextHandlerState_sptr& ctx ) 
{
    LOG( csmapi, trace ) << STATE_NAME ":CreatePayload: Enter";
    
    std::string stmt= "SELECT a.allocation_id, a.primary_job_id, a.secondary_job_id,"
        " a.ssd_file_system_name, a.launch_node_name, a.user_flags,"
        " a.system_flags, a.ssd_min, a.ssd_max, a.num_nodes, a.num_processors,"
        " a.num_gpus, a.projected_memory, a.state,a.type, a.job_type, a.user_name, a.user_id,"
        " a.user_group_id, a.user_script, a.begin_time,"
        " a.account, a.comment, a.job_name,"
        " a.job_submit_time, a.queue, a.requeue, a.time_limit, a.wc_key, a.isolated_cores,"
        " array_to_string(array_agg(an.node_name), ',') as nodes  FROM csm_allocation a"
        " JOIN csm_allocation_node an ON a.allocation_id = an.allocation_id GROUP BY"
        " a.allocation_id";
    
    const int paramCount = 0;
    csm::db::DBReqContent *dbReq = new csm::db::DBReqContent( stmt, paramCount );
    *dbPayload = dbReq;

    LOG( csmapi, trace ) << STATE_NAME ":CreatePayload: Parameterized SQL: " << stmt;
    
    LOG( csmapi, trace ) << STATE_NAME ":CreatePayload: Exit";
    return true;
}

bool CSMIAllocationQueryActiveAll::CreateByteArray(
        const std::vector<csm::db::DBTuple *>&tuples,
        char **buf, uint32_t &bufLen,
        csm::daemon::EventContextHandlerState_sptr& ctx ) 
{
	LOG( csmapi, trace ) << STATE_NAME ":CreateByteArray: Enter";

    *buf = nullptr;
    bufLen = 0;

    uint32_t numRecords = tuples.size();

    if ( numRecords > 0 )
    {
        OUTPUT_STRUCT output;
        csm_init_struct_versioning(&output);
        output.num_allocations = numRecords;
        output.allocations = (ALLOC_STRUCT**) malloc( numRecords * sizeof(ALLOC_STRUCT*));
    
        for ( uint32_t i = 0; i < numRecords; ++i )
        {
            CreateOutputStruct( tuples[i], &output.allocations[i] );
        }

        csm_serialize_struct( OUTPUT_STRUCT, &output, buf, &bufLen);
	    
        csm_free_struct( OUTPUT_STRUCT, output );
    }

    LOG( csmapi, trace ) << STATE_NAME ":CreateByteArray: Exit";
    return true;
}

void CSMIAllocationQueryActiveAll::CreateOutputStruct(
    csm::db::DBTuple * const & fields,
    ALLOC_STRUCT **output )
{
    LOG( csmapi, debug ) << STATE_NAME ":CreateOutputStruct: Enter";

    // Check to verify that the number of fields matches, else exit.
    if ( fields->nfields != 31 )
    {
        *output = nullptr;
        return;
    }

    // convert from DB tuple results to c data structure and add node list

    ALLOC_STRUCT *o = nullptr;
    csm_init_struct_ptr(csmi_allocation_t, o);

    o->allocation_id        = strtoll(fields->data[0], nullptr, 10);
    o->primary_job_id       = strtoll(fields->data[1], nullptr, 10);
    o->secondary_job_id     = strtol(fields->data[2], nullptr, 10);
    o->ssd_file_system_name = strdup(fields->data[3]);
    o->launch_node_name     = strdup(fields->data[4]);
    o->user_flags           = strdup(fields->data[5]);
    o->system_flags         = strdup(fields->data[6]);
    o->ssd_max             = strtoll(fields->data[7], nullptr, 10);
    o->ssd_min             = strtoll(fields->data[8], nullptr, 10);
    o->num_nodes            = strtol(fields->data[9], nullptr, 10);
    o->num_processors       = strtol(fields->data[10], nullptr, 10);
    o->num_gpus             = strtol(fields->data[11], nullptr, 10);
    o->projected_memory     = strtol(fields->data[12], nullptr, 10);
    o->state                = (csmi_state_t)csm_get_enum_from_string(csmi_state_t, fields->data[13]);
    o->type                 = (csmi_allocation_type_t)csm_get_enum_from_string(csmi_allocation_type_t, fields->data[14]);
    o->job_type             = (csmi_job_type_t)csm_get_enum_from_string(csmi_job_type_t, fields->data[15]);
    o->user_name            = strdup(fields->data[16]);
    o->user_id              = strtol(fields->data[17], nullptr, 10);
    o->user_group_id        = strtol(fields->data[18], nullptr, 10);
    o->user_script          = strdup(fields->data[19]);
    o->begin_time           = strdup(fields->data[20]);
    o->account              = strdup(fields->data[21]);
    o->comment              = strdup(fields->data[22]);
    o->job_name             = strdup(fields->data[23]);
    o->job_submit_time      = strdup(fields->data[24]);
    o->queue                = strdup(fields->data[25]);
    o->requeue              = strdup(fields->data[26]);
    o->time_limit           = strtoll(fields->data[27], nullptr, 10);
    o->wc_key               = strdup(fields->data[28]);
    o->isolated_cores       = strtol(fields->data[29], nullptr, 10);
                                                  
    // If more than one node was found scan the compute nodes.
    if ( o->num_nodes > 0 ) 
    {
        o->compute_nodes = (char **)malloc(sizeof(char *) * o->num_nodes);
        uint32_t i = 0;
        char *saveptr;
        char *nodeStr = strtok_r(fields->data[30], ",", &saveptr);
        
        while (nodeStr != NULL && i < o->num_nodes ) 
        {
            o->compute_nodes[i++] = strdup(nodeStr);
            nodeStr = strtok_r(NULL, ",", &saveptr);
        }

        while ( i < o->num_nodes )
        {
            o->compute_nodes[i++] = strdup("N/A");
        }
    }

    *output = o;
    LOG( csmapi, debug ) << STATE_NAME ":CreateOutputStruct: Exit";
}

