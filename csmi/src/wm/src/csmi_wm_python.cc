/*================================================================================

    csmi/src/wm/src/csmi_wm_python.cc

  © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
//#include <boost/python/class.hpp>
#include <boost/python.hpp>
//#include <boost/python/module.hpp>
//#include <boost/python/def.hpp>

#include "csmi/src/common/include/csmi_python.h"
#include "csm_api_workload_manager.h"
using namespace boost::python;

tuple wrap_csm_allocation_create(
    csmi_allocation_t input)
{
    // Always sets the metadata.
    input._metadata=CSM_VERSION_ID;

    // Output objs
    csm_api_object * updated_handle;
    
    // Run the API
    int return_code = csm_allocation_create( (csm_api_object**)&updated_handle, &input );
    int64_t oid = CSMIObj::GetInstance().StoreCSMObj(updated_handle);
    
    return make_tuple(return_code, oid, input.allocation_id);
}

tuple wrap_csm_allocation_delete(
    csm_allocation_delete_input_t input)
{
    // Always sets the metadata.
    input._metadata=CSM_VERSION_ID;

    // Output objs
    csm_api_object * updated_handle;
    
    // Run the API
    int return_code = csm_allocation_delete( (csm_api_object**)&updated_handle, &input );
    int64_t oid = CSMIObj::GetInstance().StoreCSMObj(updated_handle);
    
    return make_tuple(return_code, oid);
}

tuple wrap_csm_allocation_query(
    csm_allocation_query_input_t input)
{
    // Always sets the metadata.
    input._metadata=CSM_VERSION_ID;

    // Output objs
    csm_api_object * updated_handle;
    csm_allocation_query_output_t * output= nullptr;
    
    // Run the API
    int return_code = csm_allocation_query( (csm_api_object**)&updated_handle, &input, &output);
    int64_t oid = CSMIObj::GetInstance().StoreCSMObj(updated_handle);
    
    return make_tuple(return_code, oid, *output);
}

tuple wrap_csm_allocation_query_active_all(
    csm_allocation_query_active_all_input_t input)
{
    // Always sets the metadata.
    input._metadata=CSM_VERSION_ID;

    // Output objs
    csm_api_object * updated_handle;
    csm_allocation_query_active_all_output_t * output= nullptr;
    
    // Run the API
    int return_code = csm_allocation_query_active_all( (csm_api_object**)&updated_handle, &input, &output);
    int64_t oid = CSMIObj::GetInstance().StoreCSMObj(updated_handle);
    
    return make_tuple(return_code, oid, *output);
}

tuple wrap_csm_allocation_query_details(
    csm_allocation_query_details_input_t input)
{
    // Always sets the metadata.
    input._metadata=CSM_VERSION_ID;

    // Output objs
    csm_api_object * updated_handle;
    csm_allocation_query_details_output_t * output= nullptr;
    
    // Run the API
    int return_code = csm_allocation_query_details( (csm_api_object**)&updated_handle, &input, &output);
    int64_t oid = CSMIObj::GetInstance().StoreCSMObj(updated_handle);
    
    return make_tuple(return_code, oid, *output);
}

tuple wrap_csm_allocation_update_history(
    csm_allocation_update_history_input_t input)
{
    // Always sets the metadata.
    input._metadata=CSM_VERSION_ID;

    // Output objs
    csm_api_object * updated_handle;

    // Run the API
    int return_code = csm_allocation_update_history( (csm_api_object**)&updated_handle, &input );
    int64_t oid = CSMIObj::GetInstance().StoreCSMObj(updated_handle);
    
    return make_tuple(return_code, oid);
}

tuple wrap_csm_allocation_step_begin(
    csm_allocation_step_begin_input_t input)
{
    // Always sets the metadata.
    input._metadata=CSM_VERSION_ID;

    // Output objs
    csm_api_object * updated_handle;

    // Run the API
    int return_code = csm_allocation_step_begin( (csm_api_object**)&updated_handle, &input );
    int64_t oid = CSMIObj::GetInstance().StoreCSMObj(updated_handle);
    
    return make_tuple(return_code, oid);
}

tuple wrap_csm_allocation_step_end(
    csm_allocation_step_end_input_t input)
{
    // Always sets the metadata.
    input._metadata=CSM_VERSION_ID;

    // Output objs
    csm_api_object * updated_handle;

    // Run the API
    int return_code = csm_allocation_step_end( (csm_api_object**)&updated_handle, &input );
    int64_t oid = CSMIObj::GetInstance().StoreCSMObj(updated_handle);
    
    return make_tuple(return_code, oid);
}

tuple wrap_csm_allocation_step_query(
    csm_allocation_step_query_input_t input)
{
    // Always sets the metadata.
    input._metadata=CSM_VERSION_ID;

    // Output objs
    csm_api_object * updated_handle;
    csm_allocation_step_query_output_t * output= nullptr;
    
    // Run the API
    int return_code = csm_allocation_step_query( (csm_api_object**)&updated_handle, &input, &output);
    int64_t oid = CSMIObj::GetInstance().StoreCSMObj(updated_handle);
    
    return make_tuple(return_code, oid, *output);
}

tuple wrap_csm_allocation_step_query_active_all(
    csm_allocation_step_query_active_all_input_t input)
{
    // Always sets the metadata.
    input._metadata=CSM_VERSION_ID;

    // Output objs
    csm_api_object * updated_handle;
    csm_allocation_step_query_active_all_output_t * output= nullptr;
    
    // Run the API
    int return_code = csm_allocation_step_query_active_all( (csm_api_object**)&updated_handle, &input, &output);
    int64_t oid = CSMIObj::GetInstance().StoreCSMObj(updated_handle);
    
    return make_tuple(return_code, oid, *output);
}

tuple wrap_csm_allocation_step_query_details(
    csm_allocation_step_query_details_input_t input)
{
    // Always sets the metadata.
    input._metadata=CSM_VERSION_ID;

    // Output objs
    csm_api_object * updated_handle;
    csm_allocation_step_query_details_output_t * output= nullptr;
    
    // Run the API
    int return_code = csm_allocation_step_query_details( (csm_api_object**)&updated_handle, &input, &output);
    int64_t oid = CSMIObj::GetInstance().StoreCSMObj(updated_handle);
    
    return make_tuple(return_code, oid, *output);
}

tuple wrap_csm_allocation_update_state(
    csm_allocation_update_state_input_t input)
{
    // Always sets the metadata.
    input._metadata=CSM_VERSION_ID;

    // Output objs
    csm_api_object * updated_handle;

    // Run the API
    int return_code = csm_allocation_update_state( (csm_api_object**)&updated_handle, &input );
    int64_t oid = CSMIObj::GetInstance().StoreCSMObj(updated_handle);
    
    return make_tuple(return_code, oid);
}

tuple wrap_csm_node_resources_query(
    csm_node_resources_query_input_t input)
{
    // Always sets the metadata.
    input._metadata=CSM_VERSION_ID;

    // Output objs
    csm_api_object * updated_handle;
    csm_node_resources_query_output_t * output= nullptr;
    
    // Run the API
    int return_code = csm_node_resources_query( (csm_api_object**)&updated_handle, &input, &output);
    int64_t oid = CSMIObj::GetInstance().StoreCSMObj(updated_handle);
    
    return make_tuple(return_code, oid, *output);
}

tuple wrap_csm_node_resources_query_all(
    csm_node_resources_query_all_input_t input)
{
    // Always sets the metadata.
    input._metadata=CSM_VERSION_ID;

    // Output objs
    csm_api_object * updated_handle;
    csm_node_resources_query_all_output_t * output= nullptr;
    
    // Run the API
    int return_code = csm_node_resources_query_all( (csm_api_object**)&updated_handle, &input, &output);
    int64_t oid = CSMIObj::GetInstance().StoreCSMObj(updated_handle);
    
    return make_tuple(return_code, oid, *output);
}

tuple wrap_csm_allocation_step_cgroup_create(
    csm_allocation_step_cgroup_create_input_t input)
{
    // Always sets the metadata.
    input._metadata=CSM_VERSION_ID;

    // Output objs
    csm_api_object * updated_handle;

    // Run the API
    int return_code = csm_allocation_step_cgroup_create( (csm_api_object**)&updated_handle, &input );
    int64_t oid = CSMIObj::GetInstance().StoreCSMObj(updated_handle);
    
    return make_tuple(return_code, oid);
}

tuple wrap_csm_allocation_step_cgroup_delete(
    csm_allocation_step_cgroup_delete_input_t input)
{
    // Always sets the metadata.
    input._metadata=CSM_VERSION_ID;

    // Output objs
    csm_api_object * updated_handle;

    // Run the API
    int return_code = csm_allocation_step_cgroup_delete( (csm_api_object**)&updated_handle, &input );
    int64_t oid = CSMIObj::GetInstance().StoreCSMObj(updated_handle);
    
    return make_tuple(return_code, oid);
}

tuple wrap_csm_cgroup_login(
    csm_cgroup_login_input_t input)
{
    // Always sets the metadata.
    input._metadata=CSM_VERSION_ID;

    // Output objs
    csm_api_object * updated_handle;

    // Run the API
    int return_code = csm_cgroup_login( (csm_api_object**)&updated_handle, &input );
    int64_t oid = CSMIObj::GetInstance().StoreCSMObj(updated_handle);
    
    return make_tuple(return_code, oid);
}

tuple wrap_csm_jsrun_cmd(
    csm_jsrun_cmd_input_t input)
{
    // Always sets the metadata.
    input._metadata=CSM_VERSION_ID;

    // Output objs
    csm_api_object * updated_handle;

    // Run the API
    int return_code = csm_jsrun_cmd( (csm_api_object**)&updated_handle, &input );
    int64_t oid = CSMIObj::GetInstance().StoreCSMObj(updated_handle);
    
    return make_tuple(return_code, oid);
}

//BOOST_PYTHON_MODULE(wm_structs)
BOOST_PYTHON_MODULE(lib_csm_wm_py)
{
    // Workload Management.
    def("allocation_create",
        wrap_csm_allocation_create,
        CSM_GEN_DOCSTRING("Creates an allocation in the CSM database.", ",<allocation_id>"));

    def("allocation_delete",
        wrap_csm_allocation_delete,
        CSM_GEN_DOCSTRING("Deletes an allocation from the CSM database, performs cleanup work on nodes.",""));

    def("allocation_query",
        wrap_csm_allocation_query,
        CSM_GEN_DOCSTRING("Queries the CSM database for a single allocation.",
            ",<csm_allocation_query_output_t>"));

    def("allocation_query_active_all",
        wrap_csm_allocation_query_active_all,
        CSM_GEN_DOCSTRING("Queries the CSM database for all active allocations.",
            ",<csm_allocation_query_active_all_output_t>"));

    def("allocation_query_details",
        wrap_csm_allocation_query_details,
        CSM_GEN_DOCSTRING("Queries the CSM database for a single allocation and private details.",
            ",<csm_allocation_query_details_output_t>"));
    
    def("allocation_update_history",
        wrap_csm_allocation_update_history,
        CSM_GEN_DOCSTRING("Updates the details of a historic allocation.",""));

    def("allocation_step_begin",
        wrap_csm_allocation_step_begin,
        CSM_GEN_DOCSTRING("Begins an allocation step for a running allocation.",""));

    def("allocation_step_end",
        wrap_csm_allocation_step_end,
        CSM_GEN_DOCSTRING("Ends an active allocation step.",""));

    def("allocation_step_query",
        wrap_csm_allocation_step_query,
        CSM_GEN_DOCSTRING("Queries the CSM database for allocation steps.",
            ",<csm_allocation_step_query_output_t>"));

    def("allocation_step_query_active_all",
        wrap_csm_allocation_step_query_active_all,
        CSM_GEN_DOCSTRING("Queries the CSM database for all active allocation steps.",
            ",<csm_allocation_step_query_active_all_output_t>"));

    def("allocation_step_query_details",
        wrap_csm_allocation_step_query_details,
        CSM_GEN_DOCSTRING("Queries the CSM database for allocation steps and private details.",
            ",<csm_allocation_step_query_details_output_t>"));

    def("allocation_update_state",
        wrap_csm_allocation_update_state,
        CSM_GEN_DOCSTRING("Updates the state of an active allocation.",""));

    def("node_resources_query",
        wrap_csm_node_resources_query,
        CSM_GEN_DOCSTRING("Queries the CSM database for a collection of nodes.",
            ",<csm_node_resources_query_output_t>"));

    def("node_resources_query_all",
        wrap_csm_node_resources_query_all,
        CSM_GEN_DOCSTRING("Queries the CSM database for all nodes present in the csm_node table.",
            ",<csm_node_resources_query_all_output_t>"));

    def("allocation_step_cgroup_create",
        wrap_csm_allocation_step_cgroup_create,
        CSM_GEN_DOCSTRING("Pass through mechanism for creating allocation cgroups.",""));

    def("allocation_step_cgroup_delete",
        wrap_csm_allocation_step_cgroup_delete,
        CSM_GEN_DOCSTRING("Pass through mechanism for deleting allocation cgroups.",""));

    def("cgroup_login",
        wrap_csm_cgroup_login,
        CSM_GEN_DOCSTRING("Mechanism to move a user into a cgroup.",""));


    def("jsrun_cmd",
        wrap_csm_jsrun_cmd,
        CSM_GEN_DOCSTRING("Executes a jsrun command as a remote user.",""));

    // STRUCTS_BEGIN
    enum_<csmi_state_t>("csmi_state_t")
		.value("CSM_STAGING_IN",CSM_STAGING_IN)
		.value("CSM_TO_RUNNING",CSM_TO_RUNNING)
		.value("CSM_RUNNING",CSM_RUNNING)
		.value("CSM_TO_STAGING_OUT",CSM_TO_STAGING_OUT)
		.value("CSM_STAGING_OUT",CSM_STAGING_OUT)
		.value("CSM_TO_COMPLETE",CSM_TO_COMPLETE)
		.value("CSM_COMPLETE",CSM_COMPLETE)
		.value("CSM_TO_FAILED",CSM_TO_FAILED)
		.value("CSM_FAILED",CSM_FAILED)
		.value("CSM_DELETING",CSM_DELETING)
		.value("CSM_DELETING_MCAST",CSM_DELETING_MCAST)
		.value("CSM_RUNNING_FAILED",CSM_RUNNING_FAILED)
		.value("CSM_STAGING_OUT_FAILED",CSM_STAGING_OUT_FAILED);

    enum_<csmi_cgroup_controller_t>("csmi_cgroup_controller_t")
		.value("CG_CPUSET",CG_CPUSET)
		.value("CG_MEMORY",CG_MEMORY)
		.value("CG_DEVICES",CG_DEVICES)
		.value("CG_CPUACCT",CG_CPUACCT);

    enum_<csmi_allocation_type_t>("csmi_allocation_type_t")
		.value("CSM_USER_MANAGED",CSM_USER_MANAGED)
		.value("CSM_JSM",CSM_JSM)
		.value("CSM_JSM_CGROUP_STEP",CSM_JSM_CGROUP_STEP)
		.value("CSM_DIAGNOSTICS",CSM_DIAGNOSTICS)
		.value("CSM_CGROUP_STEP",CSM_CGROUP_STEP);

    enum_<csmi_job_type_t>("csmi_job_type_t")
		.value("CSM_BATCH",CSM_BATCH)
		.value("CSM_INTERACTIVE",CSM_INTERACTIVE);

    enum_<csmi_step_status_t>("csmi_step_status_t")
		.value("CSM_STEP_RUNNING",CSM_STEP_RUNNING)
		.value("CSM_STEP_COMPLETED",CSM_STEP_COMPLETED)
		.value("CSM_STEP_KILLED",CSM_STEP_KILLED);

    class_<csmi_allocation_history_t,csmi_allocation_history_t*>("allocation_history_t")
		.add_property("exit_status", &csmi_allocation_history_t::exit_status,&csmi_allocation_history_t::exit_status," Allocation exit status. ")
		STRING_PROPERTY(csmi_allocation_history_t, char*, archive_history_time, , NULL, )
		STRING_PROPERTY(csmi_allocation_history_t, char*, end_time, , NULL, );

    class_<csmi_allocation_t,csmi_allocation_t*>("allocation_t")
		.add_property("allocation_id", &csmi_allocation_t::allocation_id,&csmi_allocation_t::allocation_id," The unique identifier for the allocation. ")
		.add_property("primary_job_id", &csmi_allocation_t::primary_job_id,&csmi_allocation_t::primary_job_id," Primary job id (for LSF this will be the LSF job ID). ")
		.add_property("ssd_min", &csmi_allocation_t::ssd_min,&csmi_allocation_t::ssd_min," Minimum ssd size for the allocation (bytes). ")
		.add_property("ssd_max", &csmi_allocation_t::ssd_max,&csmi_allocation_t::ssd_max," Maximum ssd size for the allocation (bytes).")
		.add_property("time_limit", &csmi_allocation_t::time_limit,&csmi_allocation_t::time_limit," The time limit requested or imposed on the job ")
		.add_property("secondary_job_id", &csmi_allocation_t::secondary_job_id,&csmi_allocation_t::secondary_job_id," Secondary job id (for LSF this will be the LSF job index for job arrays). ")
		.add_property("num_processors", &csmi_allocation_t::num_processors,&csmi_allocation_t::num_processors," Number of processors for use in the allocation (required). ")
		.add_property("num_gpus", &csmi_allocation_t::num_gpus,&csmi_allocation_t::num_gpus," Number of gpus to be used for the node (optional). ")
		.add_property("projected_memory", &csmi_allocation_t::projected_memory,&csmi_allocation_t::projected_memory," Projected memory usage on the node for the allocation. ")
		.add_property("user_id", &csmi_allocation_t::user_id,&csmi_allocation_t::user_id," User id of allocation owner. ")
		.add_property("user_group_id", &csmi_allocation_t::user_group_id,&csmi_allocation_t::user_group_id," User group of allocation owner. ")
		.add_property("isolated_cores", &csmi_allocation_t::isolated_cores,&csmi_allocation_t::isolated_cores," Specifies the number of cores to isolate in the system cgroup per socket: 0 indicates no core isolation, 1 indicates 1 per socket, etc. Current maximum of 4 per socket. ")
		.add_property("num_nodes", &csmi_allocation_t::num_nodes,&csmi_allocation_t::num_nodes," Number of nodes, size of @ref compute_nodes. ")
		.add_property("type", &csmi_allocation_t::type,&csmi_allocation_t::type," Type of allocation, refer to @ref csmi_allocation_type_t for details. ")
		.add_property("job_type", &csmi_allocation_t::job_type,&csmi_allocation_t::job_type," Type of job, refer to @ref csmi_job_type_t for details. ")
		.add_property("state", &csmi_allocation_t::state,&csmi_allocation_t::state," State of allocation, refer to @ref csmi_state_t for details.")
		.add_property("shared", &csmi_allocation_t::shared,&csmi_allocation_t::shared," Flag for creating a shared allocation.")
		STRING_PROPERTY(csmi_allocation_t, char*, ssd_file_system_name, , NULL, )
		STRING_PROPERTY(csmi_allocation_t, char*, launch_node_name, , NULL, )
		STRING_PROPERTY(csmi_allocation_t, char*, user_flags, , NULL, )
		STRING_PROPERTY(csmi_allocation_t, char*, system_flags, , NULL, )
		STRING_PROPERTY(csmi_allocation_t, char*, user_name, , NULL, )
		STRING_PROPERTY(csmi_allocation_t, char*, user_script, , NULL, )
		STRING_PROPERTY(csmi_allocation_t, char*, begin_time, , NULL, )
		STRING_PROPERTY(csmi_allocation_t, char*, account, , NULL, )
		STRING_PROPERTY(csmi_allocation_t, char*, comment, , NULL, )
		STRING_PROPERTY(csmi_allocation_t, char*, job_name, , NULL, )
		STRING_PROPERTY(csmi_allocation_t, char*, job_submit_time, , NULL, )
		STRING_PROPERTY(csmi_allocation_t, char*, queue, , NULL, )
		STRING_PROPERTY(csmi_allocation_t, char*, requeue, , NULL, )
		STRING_PROPERTY(csmi_allocation_t, char*, wc_key, , NULL, )
		ARRAY_STR_PROPERTY(csmi_allocation_t, char**, compute_nodes, num_nodes, NULL, )
		STRUCT_PROPERTY(csmi_allocation_t, csmi_allocation_history_t*, history, , NULL, &csmi_allocation_t::history)
		.add_property("smt_mode", &csmi_allocation_t::smt_mode,&csmi_allocation_t::smt_mode," The SMT Mode of the allocation. 0 - all cores, 1+ - smt_mode cores, <0 use system default. ")
		.add_property("core_blink", &csmi_allocation_t::core_blink,&csmi_allocation_t::core_blink," Flag for blinking allocation cores. ")		
    .add_property("num_allocations", &csmi_allocation_t::num_allocations,&csmi_allocation_t::num_allocations," Number of allocations found. ")
		ARRAY_STRUCT_PROPERTY(csmi_allocation_t, csmi_allocation_t**, allocations, num_allocations, NULL, csmi_allocation_t);

    class_<csmi_allocation_accounting_t,csmi_allocation_accounting_t*>("allocation_accounting_t")
		.add_property("ib_rx", &csmi_allocation_accounting_t::ib_rx,&csmi_allocation_accounting_t::ib_rx," Total count of Data Octets received on all Infiniband ports, 0 if the job is active ( multiply by 4 for bytes ).")
		.add_property("ib_tx", &csmi_allocation_accounting_t::ib_tx,&csmi_allocation_accounting_t::ib_tx," Total count of Data Octets transmitted on all Infiniband ports, 0 if the job is active ( multiply by 4 for bytes ).")
		.add_property("gpfs_read", &csmi_allocation_accounting_t::gpfs_read,&csmi_allocation_accounting_t::gpfs_read," Total counter for number of bytes read over network via gpfs, 0 if the job is active. Join on details. ")
		.add_property("gpfs_write", &csmi_allocation_accounting_t::gpfs_write,&csmi_allocation_accounting_t::gpfs_write," Total counter for number of bytes written over network via gpfs, 0 if the job is active. Join on details. ")
		.add_property("ssd_read", &csmi_allocation_accounting_t::ssd_read,&csmi_allocation_accounting_t::ssd_read," Total counter for number of bytes read from the ssds, 0 if the job is active. ")
		.add_property("ssd_write", &csmi_allocation_accounting_t::ssd_write,&csmi_allocation_accounting_t::ssd_write," Total counter for number of bytes written to the ssds, 0 if the job is active. ")
		.add_property("gpu_usage", &csmi_allocation_accounting_t::gpu_usage,&csmi_allocation_accounting_t::gpu_usage," Total usage of the GPU (TODO Change), 0 if the job is active. ")
		.add_property("energy_consumed", &csmi_allocation_accounting_t::energy_consumed,&csmi_allocation_accounting_t::energy_consumed," Energy consumed by the node. ")
		.add_property("cpu_usage", &csmi_allocation_accounting_t::cpu_usage,&csmi_allocation_accounting_t::cpu_usage," The cpu usage in nanoseconds.")
		.add_property("memory_usage_max", &csmi_allocation_accounting_t::memory_usage_max,&csmi_allocation_accounting_t::memory_usage_max," The maximum memory usage in bytes. ")
		.add_property("power_cap_hit", &csmi_allocation_accounting_t::power_cap_hit,&csmi_allocation_accounting_t::power_cap_hit," Counter indicating the number of times the power cap has been hit on the node.")
		.add_property("power_cap", &csmi_allocation_accounting_t::power_cap,&csmi_allocation_accounting_t::power_cap," Power Cap - measured in watts. ")
		.add_property("power_shifting_ratio", &csmi_allocation_accounting_t::power_shifting_ratio,&csmi_allocation_accounting_t::power_shifting_ratio," Power Shifting Ratio - a ratio [0-100] used by OCC for power capping. 0 is all CPU, 100 is all GPU. ")
		.add_property("gpu_energy", &csmi_allocation_accounting_t::gpu_energy,&csmi_allocation_accounting_t::gpu_energy," GPU usage in watts. ");

    class_<csmi_allocation_step_list_t,csmi_allocation_step_list_t*>("allocation_step_list_t")
		.add_property("step_id", &csmi_allocation_step_list_t::step_id,&csmi_allocation_step_list_t::step_id," The unique identifier for the step. ")
		.add_property("num_nodes", &csmi_allocation_step_list_t::num_nodes,&csmi_allocation_step_list_t::num_nodes," Number of nodes in the step, size of @ref compute_nodes. ")
		STRING_PROPERTY(csmi_allocation_step_list_t, char*, end_time, , NULL, )
		STRING_PROPERTY(csmi_allocation_step_list_t, char*, compute_nodes, , NULL, );

    class_<csmi_allocation_state_history_t,csmi_allocation_state_history_t*>("allocation_state_history_t")
		STRING_PROPERTY(csmi_allocation_state_history_t, char*, history_time, , NULL, )
		.add_property("state", &csmi_allocation_state_history_t::state,&csmi_allocation_state_history_t::state," State of allocation, refer to @ref csmi_state_t for details.");

    class_<csmi_allocation_details_t,csmi_allocation_details_t*>("allocation_details_t")
		.add_property("power_cap_hit", &csmi_allocation_details_t::power_cap_hit,&csmi_allocation_details_t::power_cap_hit," Counter indicating the number of times the power cap was hit by nodes in the allocation.")
		.add_property("ssd_read", &csmi_allocation_details_t::ssd_read,&csmi_allocation_details_t::ssd_read," SSD reads for this allocation (in Gigabytes). ")
		.add_property("ssd_write", &csmi_allocation_details_t::ssd_write,&csmi_allocation_details_t::ssd_write," SSD writes for this allocation (in Gigabytes) ")
		.add_property("num_steps", &csmi_allocation_details_t::num_steps,&csmi_allocation_details_t::num_steps," Number of steps in allocation, size of @ref steps. ")
		.add_property("num_nodes", &csmi_allocation_details_t::num_nodes,&csmi_allocation_details_t::num_nodes," Number of nodes in allocation, size of @ref node_accounting. ")
		ARRAY_STRUCT_PROPERTY(csmi_allocation_details_t, csmi_allocation_step_list_t**, steps, num_steps, NULL, csmi_allocation_step_list_t)
		ARRAY_STRUCT_PROPERTY(csmi_allocation_details_t, csmi_allocation_accounting_t**, node_accounting, num_nodes, NULL, csmi_allocation_accounting_t)
		.add_property("num_transitions", &csmi_allocation_details_t::num_transitions,&csmi_allocation_details_t::num_transitions," Number of transitions for the allocation. ")
		ARRAY_STRUCT_PROPERTY(csmi_allocation_details_t, csmi_allocation_state_history_t**, state_transitions, num_transitions, NULL, csmi_allocation_state_history_t);

    class_<csmi_allocation_step_history_t,csmi_allocation_step_history_t*>("allocation_step_history_t")
		.add_property("exit_status", &csmi_allocation_step_history_t::exit_status,&csmi_allocation_step_history_t::exit_status," step/s exit status. will be tracked and given to csm by job leader ")
		.add_property("total_u_time", &csmi_allocation_step_history_t::total_u_time,&csmi_allocation_step_history_t::total_u_time," Relates to the 'us' (user mode) value of %Cpu(s) of the 'top' Linux cmd. ")
		.add_property("total_s_time", &csmi_allocation_step_history_t::total_s_time,&csmi_allocation_step_history_t::total_s_time," Relates to the 'sy' (system mode) value of %Cpu(s) of the 'top' Linux cmd. ")
		.add_property("max_memory", &csmi_allocation_step_history_t::max_memory,&csmi_allocation_step_history_t::max_memory," The maximum memory usage of the step. ")
		STRING_PROPERTY(csmi_allocation_step_history_t, char*, error_message, , NULL, )
		STRING_PROPERTY(csmi_allocation_step_history_t, char*, omp_thread_limit, , NULL, )
		STRING_PROPERTY(csmi_allocation_step_history_t, char*, cpu_stats, , NULL, )
		STRING_PROPERTY(csmi_allocation_step_history_t, char*, gpu_stats, , NULL, )
		STRING_PROPERTY(csmi_allocation_step_history_t, char*, memory_stats, , NULL, )
		STRING_PROPERTY(csmi_allocation_step_history_t, char*, io_stats, , NULL, )
		STRING_PROPERTY(csmi_allocation_step_history_t, char*, end_time, , NULL, )
		STRING_PROPERTY(csmi_allocation_step_history_t, char*, archive_history_time, , NULL, );

    class_<csmi_allocation_step_t,csmi_allocation_step_t*>("allocation_step_t")
		.add_property("step_id", &csmi_allocation_step_t::step_id,&csmi_allocation_step_t::step_id," The identifier of the step, combined with @ref allocation_id to form a unique identifier. ")
		.add_property("allocation_id", &csmi_allocation_step_t::allocation_id,&csmi_allocation_step_t::allocation_id," Allocation that the step is a member of, when combined with @ref step_id represents a unique step.")
		.add_property("num_nodes", &csmi_allocation_step_t::num_nodes,&csmi_allocation_step_t::num_nodes," Number of nodes, size of @ref compute_nodes.")
		.add_property("num_processors", &csmi_allocation_step_t::num_processors,&csmi_allocation_step_t::num_processors," Number of CPUs assigned to a job. ")
		.add_property("num_gpus", &csmi_allocation_step_t::num_gpus,&csmi_allocation_step_t::num_gpus," Number of gpus. ")
		.add_property("projected_memory", &csmi_allocation_step_t::projected_memory,&csmi_allocation_step_t::projected_memory," Projected memory availablity for the step.")
		.add_property("num_tasks", &csmi_allocation_step_t::num_tasks,&csmi_allocation_step_t::num_tasks," Number of tasks in the step")
		.add_property("status", &csmi_allocation_step_t::status,&csmi_allocation_step_t::status," The status of the step, @ref csmi_step_status.def ")
		STRING_PROPERTY(csmi_allocation_step_t, char*, executable, , NULL, )
		STRING_PROPERTY(csmi_allocation_step_t, char*, working_directory, , NULL, )
		STRING_PROPERTY(csmi_allocation_step_t, char*, user_flags, , NULL, )
		STRING_PROPERTY(csmi_allocation_step_t, char*, argument, , NULL, )
		STRING_PROPERTY(csmi_allocation_step_t, char*, environment_variable, , NULL, )
		STRING_PROPERTY(csmi_allocation_step_t, char*, begin_time, , NULL, )
		ARRAY_STR_PROPERTY(csmi_allocation_step_t, char**, compute_nodes, num_nodes, NULL, )
		STRUCT_PROPERTY(csmi_allocation_step_t, csmi_allocation_step_history_t*, history, , NULL, &csmi_allocation_step_t::history);

    class_<csmi_ssd_resources_record_t,csmi_ssd_resources_record_t*>("ssd_resources_record_t")
		.add_property("wear_lifespan_used", &csmi_ssd_resources_record_t::wear_lifespan_used,&csmi_ssd_resources_record_t::wear_lifespan_used," Estimate of the amount of SSD life consumed (w.l.m. will use. valid range 0-255 percent) 0 = new, 100 = completely used, 100+ = over est life time. ")
		STRING_PROPERTY(csmi_ssd_resources_record_t, char*, serial_number, , NULL, )
		STRING_PROPERTY(csmi_ssd_resources_record_t, char*, update_time, , NULL, );

    class_<csmi_node_resources_record_t,csmi_node_resources_record_t*>("node_resources_record_t")
		.add_property("node_installed_memory", &csmi_node_resources_record_t::node_installed_memory,&csmi_node_resources_record_t::node_installed_memory," Amount of installed memory on this node (in kB). ")
		.add_property("vg_available_size", &csmi_node_resources_record_t::vg_available_size,&csmi_node_resources_record_t::vg_available_size," Available size remaining (in bytes) in the volume group on this node. ")
		.add_property("vg_total_size", &csmi_node_resources_record_t::vg_total_size,&csmi_node_resources_record_t::vg_total_size," Total size (in bytes) of the volume group on this node. ")
		.add_property("node_available_cores", &csmi_node_resources_record_t::node_available_cores,&csmi_node_resources_record_t::node_available_cores," Deprecated after CSM_VERSION_0_4_1. Number of physical cores on this node from all processors. ")
		.add_property("node_available_gpus", &csmi_node_resources_record_t::node_available_gpus,&csmi_node_resources_record_t::node_available_gpus," Deprecated after CSM_VERSION_0_4_1. Number of gpus available. ")
		.add_property("node_available_processors", &csmi_node_resources_record_t::node_available_processors,&csmi_node_resources_record_t::node_available_processors," Deprecated after CSM_VERSION_0_4_1. Number of processors on this node (processor sockets, non-uniform memory access (NUMA) nodes). ")
		.add_property("ssds_count", &csmi_node_resources_record_t::ssds_count,&csmi_node_resources_record_t::ssds_count," Number of elements in the ssds array. Size of @ref ssds. ")
		.add_property("node_type", &csmi_node_resources_record_t::node_type,&csmi_node_resources_record_t::node_type," The type of the node, see @ref csmi_node_type_t for details. ")
		.add_property("node_state", &csmi_node_resources_record_t::node_state,&csmi_node_resources_record_t::node_state," State of the node, see @ref csmi_node_state_t for details. ")
		.add_property("node_ready", &csmi_node_resources_record_t::node_ready,&csmi_node_resources_record_t::node_ready," After CSM_VERSION_0_4_1, this field is populated indirectly via 'node_state'. - Flag indicating whether or not the node is ready ( @ref csm_bool). ")
		STRING_PROPERTY(csmi_node_resources_record_t, char*, node_name, , NULL, )
		STRING_PROPERTY(csmi_node_resources_record_t, char*, node_update_time, , NULL, )
		STRING_PROPERTY(csmi_node_resources_record_t, char*, vg_update_time, , NULL, )
		ARRAY_STRUCT_PROPERTY(csmi_node_resources_record_t, csmi_ssd_resources_record_t**, ssds, ssds_count, NULL, csmi_ssd_resources_record_t)
		.add_property("node_discovered_cores", &csmi_node_resources_record_t::node_discovered_cores,&csmi_node_resources_record_t::node_discovered_cores," replacement for 'available_cores' after CSM_VERSION_0_4_1. Number of physical cores on this node from all processors. ")
		.add_property("node_discovered_gpus", &csmi_node_resources_record_t::node_discovered_gpus,&csmi_node_resources_record_t::node_discovered_gpus," replacement for 'available_gpus' after CSM_VERSION_0_4_1. Number of gpus on node. ")
		.add_property("node_discovered_sockets", &csmi_node_resources_record_t::node_discovered_sockets,&csmi_node_resources_record_t::node_discovered_sockets," replacement for 'available_processors' after CSM_VERSION_0_4_1. Number of processors on the node.");

    class_<csmi_cgroup_t,csmi_cgroup_t*>("cgroup_t")
		.add_property("num_params", &csmi_cgroup_t::num_params,&csmi_cgroup_t::num_params," The number of parameters/values to update. Size of @ref params, and @ref values. ")
		.add_property("type", &csmi_cgroup_t::type,&csmi_cgroup_t::type," The type of the cgroup controller to update. ")
		ARRAY_STR_PROPERTY(csmi_cgroup_t, char**, params, num_params, NULL, )
		ARRAY_STR_PROPERTY(csmi_cgroup_t, char**, values, num_params, NULL, );

    class_<csmi_allocation_resources_record_t,csmi_allocation_resources_record_t*>("allocation_resources_record_t")
		.add_property("ready", &csmi_allocation_resources_record_t::ready,&csmi_allocation_resources_record_t::ready," Flag indicating whether or not the node is ready ( @ref csm_bool). ")
		STRING_PROPERTY(csmi_allocation_resources_record_t, char*, node_name, , NULL, );

    class_<csm_allocation_query_details_input_t,csm_allocation_query_details_input_t*>("allocation_query_details_input_t")
		.add_property("allocation_id", &csm_allocation_query_details_input_t::allocation_id,&csm_allocation_query_details_input_t::allocation_id," The allocation id to search for.");

    class_<csm_allocation_query_details_output_t,csm_allocation_query_details_output_t*>("allocation_query_details_output_t")
		STRUCT_PROPERTY(csm_allocation_query_details_output_t, csmi_allocation_t*, allocation, , NULL, &csm_allocation_query_details_output_t::allocation)
		STRUCT_PROPERTY(csm_allocation_query_details_output_t, csmi_allocation_details_t*, allocation_details, , NULL, &csm_allocation_query_details_output_t::allocation_details);

    class_<csm_allocation_update_state_input_t,csm_allocation_update_state_input_t*>("allocation_update_state_input_t")
		.add_property("allocation_id", &csm_allocation_update_state_input_t::allocation_id,&csm_allocation_update_state_input_t::allocation_id," The identifier for the allocation to update. ")
		.add_property("exit_status", &csm_allocation_update_state_input_t::exit_status,&csm_allocation_update_state_input_t::exit_status," Allocation exit status, initializes to 0 ")
		.add_property("new_state", &csm_allocation_update_state_input_t::new_state,&csm_allocation_update_state_input_t::new_state," New Allocation state. Supported States: @ref csmi_state_t.RUNNING, @ref csmi_state_t.STAGING_OUT ");

    class_<csm_allocation_step_end_input_t,csm_allocation_step_end_input_t*>("allocation_step_end_input_t")
		.add_property("step_id", &csm_allocation_step_end_input_t::step_id,&csm_allocation_step_end_input_t::step_id," The identifier of the step, combined with @ref allocation_id to form a unique identifier.")
		.add_property("allocation_id", &csm_allocation_step_end_input_t::allocation_id,&csm_allocation_step_end_input_t::allocation_id," Allocation that the step is a member of, when combined with @ref step_id represents a unique step.")
		.add_property("status", &csm_allocation_step_end_input_t::status,&csm_allocation_step_end_input_t::status," The status of the step, @ref csmi_step_status_t ")
		STRUCT_PROPERTY(csm_allocation_step_end_input_t, csmi_allocation_step_history_t*, history, , NULL, &csm_allocation_step_end_input_t::history);

    class_<csm_allocation_step_query_input_t,csm_allocation_step_query_input_t*>("allocation_step_query_input_t")
		.add_property("allocation_id", &csm_allocation_step_query_input_t::allocation_id,&csm_allocation_step_query_input_t::allocation_id," Allocation that this step is a member of. If @ref step_id is < 0 all steps for the allocating will be retrieved. ")
		.add_property("step_id", &csm_allocation_step_query_input_t::step_id,&csm_allocation_step_query_input_t::step_id," The step id, combined with @ref allocation_id to uniquely identify a step in the database. Ignored if < 0. ")
		.add_property("limit", &csm_allocation_step_query_input_t::limit,&csm_allocation_step_query_input_t::limit," SQL 'LIMIT' numeric value. API will ignore values less than 1.")
		.add_property("offset", &csm_allocation_step_query_input_t::offset,&csm_allocation_step_query_input_t::offset," SQL 'OFFSET' numeric value. API will ignore values less than 1.");

    class_<csm_allocation_step_query_output_t,csm_allocation_step_query_output_t*>("allocation_step_query_output_t")
		.add_property("num_steps", &csm_allocation_step_query_output_t::num_steps,&csm_allocation_step_query_output_t::num_steps," The number of steps retrieved. ")
		ARRAY_STRUCT_PROPERTY(csm_allocation_step_query_output_t, csmi_allocation_step_t**, steps, num_steps, NULL, csmi_allocation_step_t);

    class_<csm_allocation_step_query_details_input_t,csm_allocation_step_query_details_input_t*>("allocation_step_query_details_input_t")
		.add_property("allocation_id", &csm_allocation_step_query_details_input_t::allocation_id,&csm_allocation_step_query_details_input_t::allocation_id," Allocation that this step is a member of. If @ref step_id is < 0 all steps for the allocation will be retrieved. ")
		.add_property("step_id", &csm_allocation_step_query_details_input_t::step_id,&csm_allocation_step_query_details_input_t::step_id," The step id, combined with @ref allocation_id to uniquely identify a step in the database. Ignored if < 0. ")
		.add_property("limit", &csm_allocation_step_query_details_input_t::limit,&csm_allocation_step_query_details_input_t::limit," SQL 'LIMIT' numeric value. API will ignore values less than 1.")
		.add_property("offset", &csm_allocation_step_query_details_input_t::offset,&csm_allocation_step_query_details_input_t::offset," SQL 'OFFSET' numeric value. API will ignore values less than 1.");

    class_<csm_allocation_step_query_active_all_input_t,csm_allocation_step_query_active_all_input_t*>("allocation_step_query_active_all_input_t")
		.add_property("allocation_id", &csm_allocation_step_query_active_all_input_t::allocation_id,&csm_allocation_step_query_active_all_input_t::allocation_id," The allocation to retrieve all active steps for. ")
		.add_property("limit", &csm_allocation_step_query_active_all_input_t::limit,&csm_allocation_step_query_active_all_input_t::limit," SQL 'LIMIT' numeric value. API will ignore values less than 1.")
		.add_property("offset", &csm_allocation_step_query_active_all_input_t::offset,&csm_allocation_step_query_active_all_input_t::offset," SQL 'OFFSET' numeric value. API will ignore values less than 1.");

    class_<csm_node_resources_query_input_t,csm_node_resources_query_input_t*>("node_resources_query_input_t")
		.add_property("limit", &csm_node_resources_query_input_t::limit,&csm_node_resources_query_input_t::limit," SQL 'LIMIT' numeric value. API will ignore values less than 1.")
		.add_property("offset", &csm_node_resources_query_input_t::offset,&csm_node_resources_query_input_t::offset," SQL 'OFFSET' numeric value. API will ignore values less than 1.")
		.add_property("node_names_count", &csm_node_resources_query_input_t::node_names_count,&csm_node_resources_query_input_t::node_names_count," Number of elements in the 'node_names' array. Must be greater than zero. Size of @ref node_names.")
		ARRAY_STR_PROPERTY(csm_node_resources_query_input_t, char**, node_names, node_names_count, NULL, );

    class_<csm_node_resources_query_output_t,csm_node_resources_query_output_t*>("node_resources_query_output_t")
		.add_property("results_count", &csm_node_resources_query_output_t::results_count,&csm_node_resources_query_output_t::results_count," Number of nodes recovered, size of @ref results. ")
		ARRAY_STRUCT_PROPERTY(csm_node_resources_query_output_t, csmi_node_resources_record_t**, results, results_count, NULL, csmi_node_resources_record_t);

    class_<csm_node_resources_query_all_input_t,csm_node_resources_query_all_input_t*>("node_resources_query_all_input_t")
		.add_property("limit", &csm_node_resources_query_all_input_t::limit,&csm_node_resources_query_all_input_t::limit," SQL 'LIMIT' numeric value. API will ignore values less than 1.")
		.add_property("offset", &csm_node_resources_query_all_input_t::offset,&csm_node_resources_query_all_input_t::offset," SQL 'OFFSET' numeric value. API will ignore values less than 1.");

    class_<csm_node_resources_query_all_output_t,csm_node_resources_query_all_output_t*>("node_resources_query_all_output_t")
		.add_property("results_count", &csm_node_resources_query_all_output_t::results_count,&csm_node_resources_query_all_output_t::results_count," Number of results recovered, size of @ref results.")
		ARRAY_STRUCT_PROPERTY(csm_node_resources_query_all_output_t, csmi_node_resources_record_t**, results, results_count, NULL, csmi_node_resources_record_t);

    class_<csm_allocation_step_cgroup_create_input_t,csm_allocation_step_cgroup_create_input_t*>("allocation_step_cgroup_create_input_t")
		.add_property("allocation_id", &csm_allocation_step_cgroup_create_input_t::allocation_id,&csm_allocation_step_cgroup_create_input_t::allocation_id," The allocation id of the allocation requesting a cgroup, if a cgroup for this allocation id doesn't exist this container will not be able to create the requested cgroup. If -1 is supplied operations will be performed on the base cgroup.")
		.add_property("num_components", &csm_allocation_step_cgroup_create_input_t::num_components,&csm_allocation_step_cgroup_create_input_t::num_components," The number of components [e.g. cpuset, devices, etc.] defined for this control group.")
		.add_property("pid", &csm_allocation_step_cgroup_create_input_t::pid,&csm_allocation_step_cgroup_create_input_t::pid," The process id to associate components with [this pid is placed in each cgroup's tasks file]")
		STRING_PROPERTY(csm_allocation_step_cgroup_create_input_t, char*, cgroup_name, , NULL, )
		ARRAY_STRUCT_PROPERTY(csm_allocation_step_cgroup_create_input_t, csmi_cgroup_t**, components, num_components, NULL, csmi_cgroup_t);

    class_<csm_allocation_step_cgroup_delete_input_t,csm_allocation_step_cgroup_delete_input_t*>("allocation_step_cgroup_delete_input_t")
		.add_property("allocation_id", &csm_allocation_step_cgroup_delete_input_t::allocation_id,&csm_allocation_step_cgroup_delete_input_t::allocation_id," The allocation id, representing parent of the cgroup to delete. ")
		.add_property("num_types", &csm_allocation_step_cgroup_delete_input_t::num_types,&csm_allocation_step_cgroup_delete_input_t::num_types," The number of controller types to run delete on, if 0 all control groups are deleted (make sure @ref controller_types is NULL). ")
		STRING_PROPERTY(csm_allocation_step_cgroup_delete_input_t, char*, cgroup_name, , NULL, )
		ARRAY_PROPERTY(csm_allocation_step_cgroup_delete_input_t, csmi_cgroup_controller_t*, controller_types, num_types, NULL, csmi_cgroup_controller_t);

    class_<csm_allocation_resources_query_input_t,csm_allocation_resources_query_input_t*>("allocation_resources_query_input_t")
		.add_property("allocation_id", &csm_allocation_resources_query_input_t::allocation_id,&csm_allocation_resources_query_input_t::allocation_id," Allocation id to search for. ")
		.add_property("limit", &csm_allocation_resources_query_input_t::limit,&csm_allocation_resources_query_input_t::limit," SQL 'LIMIT' numeric value. API will ignore values less than 1.")
		.add_property("offset", &csm_allocation_resources_query_input_t::offset,&csm_allocation_resources_query_input_t::offset," SQL 'OFFSET' numeric value. API will ignore values less than 1.");

    class_<csm_allocation_resources_query_output_t,csm_allocation_resources_query_output_t*>("allocation_resources_query_output_t")
		.add_property("results_count", &csm_allocation_resources_query_output_t::results_count,&csm_allocation_resources_query_output_t::results_count," Resources retrieved by the query, size of @ref results.")
		ARRAY_STRUCT_PROPERTY(csm_allocation_resources_query_output_t, csmi_allocation_resources_record_t**, results, results_count, NULL, csmi_allocation_resources_record_t);

    class_<csm_allocation_update_history_input_t,csm_allocation_update_history_input_t*>("allocation_update_history_input_t")
		.add_property("allocation_id", &csm_allocation_update_history_input_t::allocation_id,&csm_allocation_update_history_input_t::allocation_id," Allocation id to update the history of. ")
		.add_property("user_id", &csm_allocation_update_history_input_t::user_id,&csm_allocation_update_history_input_t::user_id," Optional, user id of allocation owner. Values < 0 will not be used.")
		.add_property("user_group_id", &csm_allocation_update_history_input_t::user_group_id,&csm_allocation_update_history_input_t::user_group_id," Optional, user group of allocation owner. Values < 0 will not be used.")
		STRING_PROPERTY(csm_allocation_update_history_input_t, char*, user_name, , NULL, )
		STRING_PROPERTY(csm_allocation_update_history_input_t, char*, account, , NULL, )
		STRING_PROPERTY(csm_allocation_update_history_input_t, char*, comment, , NULL, )
		STRING_PROPERTY(csm_allocation_update_history_input_t, char*, job_name, , NULL, )
		STRING_PROPERTY(csm_allocation_update_history_input_t, char*, reservation, , NULL, );

    class_<csm_allocation_query_input_t,csm_allocation_query_input_t*>("allocation_query_input_t")
		.add_property("allocation_id", &csm_allocation_query_input_t::allocation_id,&csm_allocation_query_input_t::allocation_id," The allocation id to search on, optional if @ref primary_job_id  and  @ref secondary_job_id are set. ")
		.add_property("primary_job_id", &csm_allocation_query_input_t::primary_job_id,&csm_allocation_query_input_t::primary_job_id," The primary job id of the query, ignored if @ref allocation_id was set.")
		.add_property("secondary_job_id", &csm_allocation_query_input_t::secondary_job_id,&csm_allocation_query_input_t::secondary_job_id," The secondary job id of the query, ignored if @ref allocation_id was set.");

    class_<csm_allocation_query_output_t,csm_allocation_query_output_t*>("allocation_query_output_t")
		STRUCT_PROPERTY(csm_allocation_query_output_t, csmi_allocation_t*, allocation, , NULL, &csm_allocation_query_output_t::allocation);

    class_<csm_allocation_query_active_all_input_t,csm_allocation_query_active_all_input_t*>("allocation_query_active_all_input_t")
		.add_property("limit", &csm_allocation_query_active_all_input_t::limit,&csm_allocation_query_active_all_input_t::limit," SQL 'LIMIT' numeric value. API will ignore values less than 1.")
		.add_property("offset", &csm_allocation_query_active_all_input_t::offset,&csm_allocation_query_active_all_input_t::offset," SQL 'OFFSET' numeric value. API will ignore values less than 1.");

    class_<csm_allocation_query_active_all_output_t,csm_allocation_query_active_all_output_t*>("allocation_query_active_all_output_t")
		.add_property("num_allocations", &csm_allocation_query_active_all_output_t::num_allocations,&csm_allocation_query_active_all_output_t::num_allocations," Number of allocations found. ")
		ARRAY_STRUCT_PROPERTY(csm_allocation_query_active_all_output_t, csmi_allocation_t**, allocations, num_allocations, NULL, csmi_allocation_t);

    class_<csm_allocation_delete_input_t,csm_allocation_delete_input_t*>("allocation_delete_input_t")
		.add_property("allocation_id", &csm_allocation_delete_input_t::allocation_id,&csm_allocation_delete_input_t::allocation_id," The allocation id to delete, set to zero if @ref primary_job_id is supplied.  ")
		.add_property("exit_status", &csm_allocation_delete_input_t::exit_status,&csm_allocation_delete_input_t::exit_status," Allocation exit status, initializes to 0 ")
		.add_property("primary_job_id", &csm_allocation_delete_input_t::primary_job_id,&csm_allocation_delete_input_t::primary_job_id," Primary Job ID, set to zero if @ref allocation_id is supplied. ")
		.add_property("secondary_job_id", &csm_allocation_delete_input_t::secondary_job_id,&csm_allocation_delete_input_t::secondary_job_id," Secondary Job ID, ignored if @ref primary_job_id is not supplied.");

    class_<csm_cgroup_login_input_t,csm_cgroup_login_input_t*>("cgroup_login_input_t")
		.add_property("allocation_id", &csm_cgroup_login_input_t::allocation_id,&csm_cgroup_login_input_t::allocation_id," Allocation ID to prefer (rejected if >0 and not found in daemon). ")
		.add_property("pid", &csm_cgroup_login_input_t::pid,&csm_cgroup_login_input_t::pid," The process id to push to the allocation cgroup. ")
		STRING_PROPERTY(csm_cgroup_login_input_t, char*, user_name, , NULL, )
		.add_property("migrate_pid", &csm_cgroup_login_input_t::migrate_pid,&csm_cgroup_login_input_t::migrate_pid," Flag to migrate pid in to cgroup if true. ");

    class_<csm_jsrun_cmd_input_t,csm_jsrun_cmd_input_t*>("jsrun_cmd_input_t")
		.add_property("allocation_id", &csm_jsrun_cmd_input_t::allocation_id,&csm_jsrun_cmd_input_t::allocation_id," The Allocation id for the JSM run. Exported to **CSM_ALLOCATION_ID**. ")
		STRING_PROPERTY(csm_jsrun_cmd_input_t, char*, kv_pairs, , NULL, )
		STRING_PROPERTY(csm_jsrun_cmd_input_t, char*, jsm_path, , NULL, );

    class_<csm_soft_failure_recovery_node_t,csm_soft_failure_recovery_node_t*>("soft_failure_recovery_node_t")
		.add_property("errcode", &csm_soft_failure_recovery_node_t::errcode,&csm_soft_failure_recovery_node_t::errcode,"int")
		STRING_PROPERTY(csm_soft_failure_recovery_node_t, char*, errmsg, , NULL, )
		STRING_PROPERTY(csm_soft_failure_recovery_node_t, char*, source, , NULL, );

    class_<csm_soft_failure_recovery_input_t,csm_soft_failure_recovery_input_t*>("soft_failure_recovery_input_t")
		.add_property("retry_count", &csm_soft_failure_recovery_input_t::retry_count,&csm_soft_failure_recovery_input_t::retry_count,"uint32_t");

    class_<csm_soft_failure_recovery_output_t,csm_soft_failure_recovery_output_t*>("soft_failure_recovery_output_t")
		.add_property("error_count", &csm_soft_failure_recovery_output_t::error_count,&csm_soft_failure_recovery_output_t::error_count,"uint32_t")
		ARRAY_STRUCT_PROPERTY(csm_soft_failure_recovery_output_t, csm_soft_failure_recovery_node_t**, node_errors, error_count, NULL, csm_soft_failure_recovery_node_t);

};
