/*================================================================================

    csmi/src/bb/src/csmi_bb_python.cc

  © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#include <boost/python.hpp>

#include "csmi/src/common/include/csmi_python.h"
#include "csm_api_burst_buffer.h"
using namespace boost::python;

tuple wrap_csm_bb_cmd(
    csm_bb_cmd_input_t input)
{
    // Always sets the metadata.
    input._metadata=CSM_VERSION_ID;

    // Output objs
    csm_api_object * updated_handle;
    csm_bb_cmd_output_t * output= nullptr;

    // Run the API
    int return_code = csm_bb_cmd( (csm_api_object**)&updated_handle, &input, &output);
    int64_t oid = CSMIObj::GetInstance().StoreCSMObj(updated_handle);

    return make_tuple(return_code, oid, *output);
}

tuple wrap_csm_bb_lv_create(
    csm_bb_lv_create_input_t input)
{
    // Always sets the metadata.
    input._metadata=CSM_VERSION_ID;

    // Output objs
    csm_api_object * updated_handle;

    // Run the API
    int return_code = csm_bb_lv_create( (csm_api_object**)&updated_handle, &input);
    int64_t oid = CSMIObj::GetInstance().StoreCSMObj(updated_handle);

    return make_tuple(return_code, oid);
}

tuple wrap_csm_bb_lv_delete(
    csm_bb_lv_delete_input_t input)
{
    // Always sets the metadata.
    input._metadata=CSM_VERSION_ID;

    // Output objs
    csm_api_object * updated_handle;

    // Run the API
    int return_code = csm_bb_lv_delete( (csm_api_object**)&updated_handle, &input);
    int64_t oid = CSMIObj::GetInstance().StoreCSMObj(updated_handle);

    return make_tuple(return_code, oid);
}

tuple wrap_csm_bb_lv_query(
    csm_bb_lv_query_input_t input)
{
    // Always sets the metadata.
    input._metadata=CSM_VERSION_ID;

    // Output objs
    csm_api_object * updated_handle;
    csm_bb_lv_query_output_t * output= nullptr;

    // Run the API
    int return_code = csm_bb_lv_query( (csm_api_object**)&updated_handle, &input, &output);
    int64_t oid = CSMIObj::GetInstance().StoreCSMObj(updated_handle);

    return make_tuple(return_code, oid, *output);
}

tuple wrap_csm_bb_lv_update(
    csm_bb_lv_update_input_t input)
{
    // Always sets the metadata.
    input._metadata=CSM_VERSION_ID;

    // Output objs
    csm_api_object * updated_handle;

    // Run the API
    int return_code = csm_bb_lv_update( (csm_api_object**)&updated_handle, &input);
    int64_t oid = CSMIObj::GetInstance().StoreCSMObj(updated_handle);

    return make_tuple(return_code, oid);
}

tuple wrap_csm_bb_vg_create(
    csm_bb_vg_create_input_t input)
{
    // Always sets the metadata.
    input._metadata=CSM_VERSION_ID;

    // Output objs
    csm_api_object * updated_handle;

    // Run the API
    int return_code = csm_bb_vg_create( (csm_api_object**)&updated_handle, &input);
    int64_t oid = CSMIObj::GetInstance().StoreCSMObj(updated_handle);

    return make_tuple(return_code, oid);
}

tuple wrap_csm_bb_vg_delete(
    csm_bb_vg_delete_input_t input)
{
    // Always sets the metadata.
    input._metadata=CSM_VERSION_ID;

    // Output objs
    csm_api_object * updated_handle;
    csm_bb_vg_delete_output_t * output= nullptr;

    // Run the API
    int return_code = csm_bb_vg_delete( (csm_api_object**)&updated_handle, &input, &output);
    int64_t oid = CSMIObj::GetInstance().StoreCSMObj(updated_handle);

    return make_tuple(return_code, oid, *output);
}

tuple wrap_csm_bb_vg_query(
    csm_bb_vg_query_input_t input)
{
    // Always sets the metadata.
    input._metadata=CSM_VERSION_ID;

    // Output objs
    csm_api_object * updated_handle;
    csm_bb_vg_query_output_t * output= nullptr;

    // Run the API
    int return_code = csm_bb_vg_query( (csm_api_object**)&updated_handle, &input, &output);
    int64_t oid = CSMIObj::GetInstance().StoreCSMObj(updated_handle);

    return make_tuple(return_code, oid, *output);
}

//BOOST_PYTHON_MODULE(wm_structs)
BOOST_PYTHON_MODULE(lib_csm_bb_py)
{
    // Burst Buffer
    def("cmd",
        wrap_csm_bb_cmd,
        CSM_GEN_DOCSTRING("Issues a multicast request to execute burst buffer commands remotely.",
            ",<csm_bb_cmd_output_t>"));
    
    def("lv_create",
        wrap_csm_bb_lv_create,
        CSM_GEN_DOCSTRING("Creates an entry in the CSM database for a logical volume.",""));
    
    def("lv_delete",
        wrap_csm_bb_lv_delete,
        CSM_GEN_DOCSTRING("Deletes a logical volume entry in the CSM database.",""));
    
    def("lv_query",
        wrap_csm_bb_lv_query,
        CSM_GEN_DOCSTRING("Queries a logical volume from the CSM database.",",<csm_bb_lv_query_output_t>"));
        
    def("lv_update",
        wrap_csm_bb_lv_update,
        CSM_GEN_DOCSTRING("Updates a logical volume entry in the CSM database.",""));
        
    def("vg_create",
        wrap_csm_bb_vg_create,
        CSM_GEN_DOCSTRING("Creates an entry in the CSM database for a volume group.",""));
        
    def("vg_delete",
        wrap_csm_bb_vg_delete,
        CSM_GEN_DOCSTRING("Deletes a volume group entry in the CSM database.",
            ",<csm_bb_vg_delete_output_t>"));
        
    def("vg_query",
        wrap_csm_bb_vg_query,
        CSM_GEN_DOCSTRING("Queries a volume group entry from the CSM database.",
            ",<csm_bb_vg_query_output_t>"));
    
    // STRUCTS_BEGIN
    class_<csmi_vg_record_t,csmi_vg_record_t*>("vg_record_t")
		.add_property("total_size", &csmi_vg_record_t::total_size,&csmi_vg_record_t::total_size," Volume group size, in bytes.")
		.add_property("available_size", &csmi_vg_record_t::available_size,&csmi_vg_record_t::available_size," Bytes available in volume group, subset of @ref total_size. ")
		.add_property("scheduler", &csmi_vg_record_t::scheduler,&csmi_vg_record_t::scheduler," Tells CSM whether or not this is the volume group for the scheduler. Defaults to false. ( @ref csm_bool). ")
		STRING_PROPERTY(csmi_vg_record_t, char*, vg_name, , NULL, )
		STRING_PROPERTY(csmi_vg_record_t, char*, node_name, , NULL, );

    class_<csmi_lv_record_t,csmi_lv_record_t*>("lv_record_t")
		.add_property("allocation_id", &csmi_lv_record_t::allocation_id,&csmi_lv_record_t::allocation_id," Unique identifier for an allocation associated with the logical volume. ")
		.add_property("current_size", &csmi_lv_record_t::current_size,&csmi_lv_record_t::current_size," Current size (in bytes). ")
		.add_property("max_size", &csmi_lv_record_t::max_size,&csmi_lv_record_t::max_size," Max size (in bytes) achieved at run time.")
		.add_property("state", &csmi_lv_record_t::state,&csmi_lv_record_t::state," State of the logical volume - [C]reated, [M]ounted, [S]hrinking, [R]emoved ")
		STRING_PROPERTY(csmi_lv_record_t, char*, logical_volume_name, , NULL, )
		STRING_PROPERTY(csmi_lv_record_t, char*, vg_name, , NULL, )
		STRING_PROPERTY(csmi_lv_record_t, char*, node_name, , NULL, )
		STRING_PROPERTY(csmi_lv_record_t, char*, file_system_mount, , NULL, )
		STRING_PROPERTY(csmi_lv_record_t, char*, file_system_type, , NULL, )
		STRING_PROPERTY(csmi_lv_record_t, char*, begin_time, , NULL, )
		STRING_PROPERTY(csmi_lv_record_t, char*, updated_time, , NULL, );

    class_<csmi_bb_vg_ssd_info_t,csmi_bb_vg_ssd_info_t*>("bb_vg_ssd_info_t")
		.add_property("ssd_allocation", &csmi_bb_vg_ssd_info_t::ssd_allocation,&csmi_bb_vg_ssd_info_t::ssd_allocation," The amount of space (in bytes) that this ssd contributes to the volume group. Can not be less than zero. The total sum of these fields should equal 'total_size' in the @ref csm_bb_vg_create_input_t struct. API will check this and throw error if mismatched. ")
		STRING_PROPERTY(csmi_bb_vg_ssd_info_t, char*, ssd_serial_number, , NULL, );

    class_<csm_bb_cmd_input_t,csm_bb_cmd_input_t*>("bb_cmd_input_t")
		.add_property("node_names_count", &csm_bb_cmd_input_t::node_names_count,&csm_bb_cmd_input_t::node_names_count," Number of  nodes to issue the burst buffer command on, size of @ref node_names. ")
		STRING_PROPERTY(csm_bb_cmd_input_t, char*, command_arguments, , NULL, )
		ARRAY_STR_PROPERTY(csm_bb_cmd_input_t, char**, node_names, node_names_count, NULL, );

    class_<csm_bb_cmd_output_t,csm_bb_cmd_output_t*>("bb_cmd_output_t")
		STRING_PROPERTY(csm_bb_cmd_output_t, char*, command_output, , NULL, );

    class_<csm_bb_lv_create_input_t,csm_bb_lv_create_input_t*>("bb_lv_create_input_t")
		.add_property("allocation_id", &csm_bb_lv_create_input_t::allocation_id,&csm_bb_lv_create_input_t::allocation_id," Unique identifier of an allocation. ")
		.add_property("current_size", &csm_bb_lv_create_input_t::current_size,&csm_bb_lv_create_input_t::current_size," Current logical volume size (bytes). ")
		.add_property("state", &csm_bb_lv_create_input_t::state,&csm_bb_lv_create_input_t::state," State of the logical volume - [C]reated, [M]ounted, [S]hrinking, or [R]emoved. ")
		STRING_PROPERTY(csm_bb_lv_create_input_t, char*, file_system_mount, , NULL, )
		STRING_PROPERTY(csm_bb_lv_create_input_t, char*, file_system_type, , NULL, )
		STRING_PROPERTY(csm_bb_lv_create_input_t, char*, logical_volume_name, , NULL, )
		STRING_PROPERTY(csm_bb_lv_create_input_t, char*, node_name, , NULL, )
		STRING_PROPERTY(csm_bb_lv_create_input_t, char*, vg_name, , NULL, );

    class_<csm_bb_lv_delete_input_t,csm_bb_lv_delete_input_t*>("bb_lv_delete_input_t")
		.add_property("allocation_id", &csm_bb_lv_delete_input_t::allocation_id,&csm_bb_lv_delete_input_t::allocation_id," Unique identifier of an allocation. ")
		.add_property("num_bytes_read", &csm_bb_lv_delete_input_t::num_bytes_read,&csm_bb_lv_delete_input_t::num_bytes_read," Number of bytes read during the life of this partition. - OPTIONAL - defaults to '-1' if not provided. values less than 0 will be inserted into csm database as NULL.")
		.add_property("num_bytes_written", &csm_bb_lv_delete_input_t::num_bytes_written,&csm_bb_lv_delete_input_t::num_bytes_written," Number of bytes written during the life of this partition. - OPTIONAL - defaults to '-1' if not provided. values less than 0 will be inserted into csm database as NULL.")
		STRING_PROPERTY(csm_bb_lv_delete_input_t, char*, logical_volume_name, , NULL, )
		STRING_PROPERTY(csm_bb_lv_delete_input_t, char*, node_name, , NULL, )
		.add_property("num_reads", &csm_bb_lv_delete_input_t::num_reads,&csm_bb_lv_delete_input_t::num_reads," Number of reads during the life of this partition. - OPTIONAL - defaults to '-1' if not provided. values less than 0 will be inserted into csm database as NULL.")
		.add_property("num_writes", &csm_bb_lv_delete_input_t::num_writes,&csm_bb_lv_delete_input_t::num_writes," Number of writes during the life of this partition. - OPTIONAL - defaults to '-1' if not provided. values less than 0 will be inserted into csm database as NULL.");

    class_<csm_bb_lv_query_input_t,csm_bb_lv_query_input_t*>("bb_lv_query_input_t")
		.add_property("limit", &csm_bb_lv_query_input_t::limit,&csm_bb_lv_query_input_t::limit," SQL 'LIMIT' numeric value. API will ignore values less than 1.")
		.add_property("offset", &csm_bb_lv_query_input_t::offset,&csm_bb_lv_query_input_t::offset," SQL 'OFFSET' numeric value. API will ignore values less than 1.")
		.add_property("allocation_ids_count", &csm_bb_lv_query_input_t::allocation_ids_count,&csm_bb_lv_query_input_t::allocation_ids_count," Number of allocations to perform query on, size of @ref allocation_ids. ")
		.add_property("logical_volume_names_count", &csm_bb_lv_query_input_t::logical_volume_names_count,&csm_bb_lv_query_input_t::logical_volume_names_count," Number of logical volumes to perform query on, size of @ref logical_volume_names. ")
		.add_property("node_names_count", &csm_bb_lv_query_input_t::node_names_count,&csm_bb_lv_query_input_t::node_names_count," Number of nodes to perform the query on, size of @ref node_names.")
		ARRAY_PROPERTY(csm_bb_lv_query_input_t, int64_t*, allocation_ids, allocation_ids_count, NULL, int64_t)
		ARRAY_STR_PROPERTY(csm_bb_lv_query_input_t, char**, logical_volume_names, logical_volume_names_count, NULL, )
		ARRAY_STR_PROPERTY(csm_bb_lv_query_input_t, char**, node_names, node_names_count, NULL, );

    class_<csm_bb_lv_query_output_t,csm_bb_lv_query_output_t*>("bb_lv_query_output_t")
		.add_property("results_count", &csm_bb_lv_query_output_t::results_count,&csm_bb_lv_query_output_t::results_count," Number of logical volumes retrieved, size of @ref results. ")
		ARRAY_STRUCT_PROPERTY(csm_bb_lv_query_output_t, csmi_lv_record_t**, results, results_count, NULL, csmi_lv_record_t);

    class_<csm_bb_lv_update_input_t,csm_bb_lv_update_input_t*>("bb_lv_update_input_t")
		.add_property("allocation_id", &csm_bb_lv_update_input_t::allocation_id,&csm_bb_lv_update_input_t::allocation_id," Unique identifier for this allocation. Can not be less than zero. ")
		.add_property("current_size", &csm_bb_lv_update_input_t::current_size,&csm_bb_lv_update_input_t::current_size," Current size (in bytes) Can not be less than zero. ")
		.add_property("state", &csm_bb_lv_update_input_t::state,&csm_bb_lv_update_input_t::state," State of the logical volume - [C]reated, [M]ounted, [S]hrinking, or [R]emoved. If left NULL, then API will fail.")
		STRING_PROPERTY(csm_bb_lv_update_input_t, char*, logical_volume_name, , NULL, )
		STRING_PROPERTY(csm_bb_lv_update_input_t, char*, node_name, , NULL, );

    class_<csm_bb_vg_create_input_t,csm_bb_vg_create_input_t*>("bb_vg_create_input_t")
		.add_property("available_size", &csm_bb_vg_create_input_t::available_size,&csm_bb_vg_create_input_t::available_size," Available size remaining in this volume group. Can not be greater than 'total_size'. Values less than zero are not valid. ")
		.add_property("total_size", &csm_bb_vg_create_input_t::total_size,&csm_bb_vg_create_input_t::total_size," Total size of this volume group. Values less than zero are not valid. ")
		.add_property("ssd_info_count", &csm_bb_vg_create_input_t::ssd_info_count,&csm_bb_vg_create_input_t::ssd_info_count," Number of entries in the ssd_info list, size of @ref ssd_info. ")
		.add_property("scheduler", &csm_bb_vg_create_input_t::scheduler,&csm_bb_vg_create_input_t::scheduler," Tells CSM whether or not this is the volume group for the scheduler. Defaults to false. ( @ref csm_bool). ")
		STRING_PROPERTY(csm_bb_vg_create_input_t, char*, node_name, , NULL, )
		STRING_PROPERTY(csm_bb_vg_create_input_t, char*, vg_name, , NULL, )
		ARRAY_STRUCT_PROPERTY(csm_bb_vg_create_input_t, csmi_bb_vg_ssd_info_t**, ssd_info, ssd_info_count, NULL, csmi_bb_vg_ssd_info_t);

    class_<csm_bb_vg_delete_input_t,csm_bb_vg_delete_input_t*>("bb_vg_delete_input_t")
		STRING_PROPERTY(csm_bb_vg_delete_input_t, char*, node_name, , NULL, )
		STRING_PROPERTY(csm_bb_vg_delete_input_t, char*, vg_name, , NULL, );

    class_<csm_bb_vg_delete_output_t,csm_bb_vg_delete_output_t*>("bb_vg_delete_output_t")
		.add_property("failure_count", &csm_bb_vg_delete_output_t::failure_count,&csm_bb_vg_delete_output_t::failure_count," Number of volume groups that failed to be deleted. Size of @ref failure_vg_names. ")
		ARRAY_STR_PROPERTY(csm_bb_vg_delete_output_t, char**, failure_vg_names, failure_count, NULL, );

    class_<csm_bb_vg_query_input_t,csm_bb_vg_query_input_t*>("bb_vg_query_input_t")
		.add_property("limit", &csm_bb_vg_query_input_t::limit,&csm_bb_vg_query_input_t::limit," SQL 'LIMIT' numeric value. API will ignore values less than 1.")
		.add_property("offset", &csm_bb_vg_query_input_t::offset,&csm_bb_vg_query_input_t::offset," SQL 'OFFSET' numeric value. API will ignore values less than 1.")
		.add_property("vg_names_count", &csm_bb_vg_query_input_t::vg_names_count,&csm_bb_vg_query_input_t::vg_names_count," Number of volume names to filter on, size of @ref vg_names.")
		.add_property("node_names_count", &csm_bb_vg_query_input_t::node_names_count,&csm_bb_vg_query_input_t::node_names_count," Number of node names to filter on, size of @ref node_names.")
		ARRAY_STR_PROPERTY(csm_bb_vg_query_input_t, char**, vg_names, vg_names_count, NULL, )
		ARRAY_STR_PROPERTY(csm_bb_vg_query_input_t, char**, node_names, node_names_count, NULL, );

    class_<csm_bb_vg_query_output_t,csm_bb_vg_query_output_t*>("bb_vg_query_output_t")
		.add_property("results_count", &csm_bb_vg_query_output_t::results_count,&csm_bb_vg_query_output_t::results_count," The number of records retrieved, size of @ref results. ")
		ARRAY_STRUCT_PROPERTY(csm_bb_vg_query_output_t, csmi_vg_record_t**, results, results_count, NULL, csmi_vg_record_t);

};
