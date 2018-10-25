/*================================================================================

    csmi/src/inv/src/csmi_inv_python.cc

  © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#include <boost/python.hpp>

#include "csmi/src/common/include/csmi_python.h"
#include "csm_api_inventory.h"
using namespace boost::python;

tuple wrap_csm_node_attributes_query(
    csm_node_attributes_query_input_t input)
{
    // Always sets the metadata.
    input._metadata=CSM_VERSION_ID;

    // Output objs
    csm_api_object * updated_handle;
    csm_node_attributes_query_output_t * output= nullptr;

    // Run the API
    int return_code = csm_node_attributes_query( (csm_api_object**)&updated_handle, &input, &output);
    int64_t oid = CSMIObj::GetInstance().StoreCSMObj(updated_handle);

    return make_tuple(return_code, oid, *output);
}

tuple wrap_csm_node_attributes_query_details(
    csm_node_attributes_query_details_input_t input)
{
    // Always sets the metadata.
    input._metadata=CSM_VERSION_ID;

    // Output objs
    csm_api_object * updated_handle;
    csm_node_attributes_query_details_output_t * output= nullptr;

    // Run the API
    int return_code = csm_node_attributes_query_details( (csm_api_object**)&updated_handle, &input, &output);
    int64_t oid = CSMIObj::GetInstance().StoreCSMObj(updated_handle);

    return make_tuple(return_code, oid, *output);
}

tuple wrap_csm_node_attributes_query_history(
    csm_node_attributes_query_history_input_t input)
{
    // Always sets the metadata.
    input._metadata=CSM_VERSION_ID;

    // Output objs
    csm_api_object * updated_handle;
    csm_node_attributes_query_history_output_t * output= nullptr;

    // Run the API
    int return_code = csm_node_attributes_query_history( (csm_api_object**)&updated_handle, &input, &output);
    int64_t oid = CSMIObj::GetInstance().StoreCSMObj(updated_handle);

    return make_tuple(return_code, oid, *output);
}

tuple wrap_csm_node_query_state_history(
    csm_node_query_state_history_input_t input)
{
    // Always sets the metadata.
    input._metadata=CSM_VERSION_ID;

    // Output objs
    csm_api_object * updated_handle;
    csm_node_query_state_history_output_t * output= nullptr;

    // Run the API
    int return_code = csm_node_query_state_history( (csm_api_object**)&updated_handle, &input, &output);
    int64_t oid = CSMIObj::GetInstance().StoreCSMObj(updated_handle);

    return make_tuple(return_code, oid, *output);
}

tuple wrap_csm_node_attributes_update(
    csm_node_attributes_update_input_t input)
{
    // Always sets the metadata.
    input._metadata=CSM_VERSION_ID;

    // Output objs
    csm_api_object * updated_handle;
    csm_node_attributes_update_output_t * output= nullptr;

    // Run the API
    int return_code = csm_node_attributes_update( (csm_api_object**)&updated_handle, &input, &output);
    int64_t oid = CSMIObj::GetInstance().StoreCSMObj(updated_handle);

    return make_tuple(return_code, oid, *output);
}

tuple wrap_csm_node_delete(
    csm_node_delete_input_t input)
{
    // Always sets the metadata.
    input._metadata=CSM_VERSION_ID;

    // Output objs
    csm_api_object * updated_handle;
    csm_node_delete_output_t * output= nullptr;

    // Run the API
    int return_code = csm_node_delete( (csm_api_object**)&updated_handle, &input, &output);
    int64_t oid = CSMIObj::GetInstance().StoreCSMObj(updated_handle);

    return make_tuple(return_code, oid, *output);
}

tuple wrap_csm_switch_attributes_query(
    csm_switch_attributes_query_input_t input)
{
    // Always sets the metadata.
    input._metadata=CSM_VERSION_ID;

    // Output objs
    csm_api_object * updated_handle;
    csm_switch_attributes_query_output_t * output= nullptr;

    // Run the API
    int return_code = csm_switch_attributes_query( (csm_api_object**)&updated_handle, &input, &output);
    int64_t oid = CSMIObj::GetInstance().StoreCSMObj(updated_handle);

    return make_tuple(return_code, oid, *output);
}

tuple wrap_csm_switch_attributes_query_details(
    csm_switch_attributes_query_details_input_t input)
{
    // Always sets the metadata.
    input._metadata=CSM_VERSION_ID;

    // Output objs
    csm_api_object * updated_handle;
    csm_switch_attributes_query_details_output_t * output= nullptr;

    // Run the API
    int return_code = csm_switch_attributes_query_details( (csm_api_object**)&updated_handle, &input, &output);
    int64_t oid = CSMIObj::GetInstance().StoreCSMObj(updated_handle);

    return make_tuple(return_code, oid, *output);
}

tuple wrap_csm_switch_attributes_query_history(
    csm_switch_attributes_query_history_input_t input)
{
    // Always sets the metadata.
    input._metadata=CSM_VERSION_ID;

    // Output objs
    csm_api_object * updated_handle;
    csm_switch_attributes_query_history_output_t * output= nullptr;

    // Run the API
    int return_code = csm_switch_attributes_query_history( (csm_api_object**)&updated_handle, &input, &output);
    int64_t oid = CSMIObj::GetInstance().StoreCSMObj(updated_handle);

    return make_tuple(return_code, oid, *output);
}

tuple wrap_csm_switch_attributes_update(
    csm_switch_attributes_update_input_t input)
{
    // Always sets the metadata.
    input._metadata=CSM_VERSION_ID;

    // Output objs
    csm_api_object * updated_handle;
    csm_switch_attributes_update_output_t * output= nullptr;

    // Run the API
    int return_code = csm_switch_attributes_update( (csm_api_object**)&updated_handle, &input, &output);
    int64_t oid = CSMIObj::GetInstance().StoreCSMObj(updated_handle);

    return make_tuple(return_code, oid, *output);
}

tuple wrap_csm_switch_inventory_collection(
    csm_switch_inventory_collection_input_t input)
{
    // Always sets the metadata.
    input._metadata=CSM_VERSION_ID;

    // Output objs
    csm_api_object * updated_handle;
    csm_switch_inventory_collection_output_t * output= nullptr;

    // Run the API
    int return_code = csm_switch_inventory_collection( (csm_api_object**)&updated_handle, &input, &output);
    int64_t oid = CSMIObj::GetInstance().StoreCSMObj(updated_handle);

    return make_tuple(return_code, oid, *output);
}

tuple wrap_csm_switch_children_inventory_collection(
    csm_switch_inventory_collection_input_t input)
{
    // Always sets the metadata.
    input._metadata=CSM_VERSION_ID;

    // Output objs
    csm_api_object * updated_handle;
    csm_switch_children_inventory_collection_output_t * output= nullptr;

    // Run the API
    int return_code = csm_switch_children_inventory_collection( (csm_api_object**)&updated_handle, &input, &output);
    int64_t oid = CSMIObj::GetInstance().StoreCSMObj(updated_handle);

    return make_tuple(return_code, oid, *output);
}

tuple wrap_csm_ib_cable_inventory_collection(
    csm_ib_cable_inventory_collection_input_t input)
{
    // Always sets the metadata.
    input._metadata=CSM_VERSION_ID;

    // Output objs
    csm_api_object * updated_handle;
    csm_ib_cable_inventory_collection_output_t * output= nullptr;

    // Run the API
    int return_code = csm_ib_cable_inventory_collection( (csm_api_object**)&updated_handle, &input, &output);
    int64_t oid = CSMIObj::GetInstance().StoreCSMObj(updated_handle);

    return make_tuple(return_code, oid, *output);
}

tuple wrap_csm_ib_cable_query(
    csm_ib_cable_query_input_t input)
{
    // Always sets the metadata.
    input._metadata=CSM_VERSION_ID;

    // Output objs
    csm_api_object * updated_handle;
    csm_ib_cable_query_output_t* output= nullptr;

    // Run the API
    int return_code = csm_ib_cable_query( (csm_api_object**)&updated_handle, &input, &output);
    int64_t oid = CSMIObj::GetInstance().StoreCSMObj(updated_handle);

    return make_tuple(return_code, oid, *output);
}

tuple wrap_csm_ib_cable_query_history(
    csm_ib_cable_query_history_input_t input)
{
    // Always sets the metadata.
    input._metadata=CSM_VERSION_ID;

    // Output objs
    csm_api_object * updated_handle;
    csm_ib_cable_query_history_output_t* output= nullptr;

    // Run the API
    int return_code = csm_ib_cable_query_history( (csm_api_object**)&updated_handle, &input, &output);
    int64_t oid = CSMIObj::GetInstance().StoreCSMObj(updated_handle);

    return make_tuple(return_code, oid, *output);
}

tuple wrap_csm_ib_cable_update(
    csm_ib_cable_update_input_t input)
{
    // Always sets the metadata.
    input._metadata=CSM_VERSION_ID;

    // Output objs
    csm_api_object * updated_handle;
    csm_ib_cable_update_output_t* output= nullptr;

    // Run the API
    int return_code = csm_ib_cable_update( (csm_api_object**)&updated_handle, &input, &output);
    int64_t oid = CSMIObj::GetInstance().StoreCSMObj(updated_handle);

    return make_tuple(return_code, oid, *output);
}


//BOOST_PYTHON_MODULE(wm_structs)
BOOST_PYTHON_MODULE(lib_csm_inv_py)
{
    def("node_attributes_query",
        wrap_csm_node_attributes_query,
        CSM_GEN_DOCSTRING("Queries the database for node attributes.",
            ",<csm_node_attributes_query_output_t>"));

    def("node_attributes_query_details",
        wrap_csm_node_attributes_query_details,
        CSM_GEN_DOCSTRING("Queries the database for specific details about a node.",
            ",<csm_node_attributes_query_details_output_t>"));

    def("node_attributes_query_history",
        wrap_csm_node_attributes_query_history,
        CSM_GEN_DOCSTRING("Queries the database for historic information about a node.",
            ",<csm_node_attributes_query_history_output_t>"));

    def("node_query_state_history",
        wrap_csm_node_query_state_history,
        CSM_GEN_DOCSTRING("Queries the database for node state changes.",
            ",<csm_node_query_state_history_output_t>"));

    def("node_attributes_update",
        wrap_csm_node_attributes_update,
        CSM_GEN_DOCSTRING("Updates the attributes of the specified node.",
            ",<csm_node_attributes_update_output_t>"));

    def("node_delete",
        wrap_csm_node_delete,
        CSM_GEN_DOCSTRING("Removes a node from the CSM database.",
            ",<csm_node_delete_output_t>"));

    def("switch_attributes_query",
        wrap_csm_switch_attributes_query,
        CSM_GEN_DOCSTRING("Queries the database for the specified switch's attributes.",
            ",<csm_switch_attributes_query_output_t>"));

    def("switch_attributes_query_details",
        wrap_csm_switch_attributes_query_details,
        CSM_GEN_DOCSTRING("Queries the database for the specified switch's detailed attributes.",
            ",<csm_switch_attributes_query_details_output_t>"));

    def("switch_attributes_query_history",
        wrap_csm_switch_attributes_query_history,
        CSM_GEN_DOCSTRING("Queries the database for the specified switch's historic attributes.",
            ",<csm_switch_attributes_query_history_output_t>"));

    def("switch_attributes_update",
        wrap_csm_switch_attributes_update,
        CSM_GEN_DOCSTRING("Updates a switch record in the CSM database.",
            ",<csm_switch_attributes_update_output_t>"));

    def("switch_inventory_collection",
        wrap_csm_switch_inventory_collection,
        CSM_GEN_DOCSTRING("Used to update the switch inventory in the CSM database.",
            ",<csm_switch_inventory_collection_output_t>"));

    def("switch_children_inventory_collection",
        wrap_csm_switch_children_inventory_collection,
        CSM_GEN_DOCSTRING("Used to update the switch inventory in the CSM database.",
            ",<csm_switch_children_inventory_collection_output_t>"));

    def("ib_cable_inventory_collection",
        wrap_csm_ib_cable_inventory_collection,
        CSM_GEN_DOCSTRING("Used to update the ib cable inventory in the CSM database.",
            ",<csm_ib_cable_inventory_collection_output_t>"));

    def("ib_cable_query",
        wrap_csm_ib_cable_query,
        CSM_GEN_DOCSTRING("Queries the CSM database for information about Infiniband Cables.",
            ",<csm_ib_cable_query_output_t>"));

    def("ib_cable_query_history",
        wrap_csm_ib_cable_query_history,
        CSM_GEN_DOCSTRING("Queries the CSM database for historic information about Infiniband Cables.",
            ",<csm_ib_cable_query_history_output_t>"));

    def("ib_cable_update",
        wrap_csm_ib_cable_update,
        CSM_GEN_DOCSTRING("Updates an Infiniband Cable record in the CSM Database.",
            ",<csm_ib_cable_update_output_t>"));

    // STRUCTS_BEGIN
    enum_<csmi_node_alteration_t>("csmi_node_alteration_t")
		.value("CSM_NODE_ALTERATION_NO_DEF",CSM_NODE_ALTERATION_NO_DEF)
		.value("CSM_NODE_ALTERATION_DATABASE_NULL",CSM_NODE_ALTERATION_DATABASE_NULL)
		.value("CSM_NODE_ALTERATION_CSM_API",CSM_NODE_ALTERATION_CSM_API)
		.value("CSM_NODE_ALTERATION_RAS_EVENT",CSM_NODE_ALTERATION_RAS_EVENT)
		.value("CSM_NODE_ALTERATION_SYS_ADMIN",CSM_NODE_ALTERATION_SYS_ADMIN)
		.value("CSM_NODE_ALTERATION_CSM_INVENTORY",CSM_NODE_ALTERATION_CSM_INVENTORY);

    class_<csmi_dimm_record_t,csmi_dimm_record_t*>("dimm_record_t")
		STRING_PROPERTY(csmi_dimm_record_t, char*, serial_number, , NULL, )
		STRING_PROPERTY(csmi_dimm_record_t, char*, node_name, , NULL, )
		STRING_PROPERTY(csmi_dimm_record_t, char*, physical_location, , NULL, )
		.add_property("size", &csmi_dimm_record_t::size,&csmi_dimm_record_t::size," The size of the memory DIMM, in GB. ")
		.add_property("status", &csmi_dimm_record_t::status,&csmi_dimm_record_t::status," Deprecated after CSM_VERSION_0_4_1. Status of the DIMM - [a]ctive, [d]econfigured, [f]ailure. ");

    class_<csmi_gpu_record_t,csmi_gpu_record_t*>("gpu_record_t")
		.add_property("gpu_id", &csmi_gpu_record_t::gpu_id,&csmi_gpu_record_t::gpu_id," Gpu identification number. ")
		.add_property("hbm_memory", &csmi_gpu_record_t::hbm_memory,&csmi_gpu_record_t::hbm_memory," High bandwidth memory: amount of available memory on this gpu (in kB). ")
		STRING_PROPERTY(csmi_gpu_record_t, char*, node_name, , NULL, )
		STRING_PROPERTY(csmi_gpu_record_t, char*, device_name, , NULL, )
		STRING_PROPERTY(csmi_gpu_record_t, char*, inforom_image_version, , NULL, )
		STRING_PROPERTY(csmi_gpu_record_t, char*, pci_bus_id, , NULL, )
		STRING_PROPERTY(csmi_gpu_record_t, char*, serial_number, , NULL, )
		STRING_PROPERTY(csmi_gpu_record_t, char*, status, , NULL, )
		STRING_PROPERTY(csmi_gpu_record_t, char*, uuid, , NULL, )
		STRING_PROPERTY(csmi_gpu_record_t, char*, vbios, , NULL, );

    class_<csmi_hca_record_t,csmi_hca_record_t*>("hca_record_t")
		STRING_PROPERTY(csmi_hca_record_t, char*, serial_number, , NULL, )
		STRING_PROPERTY(csmi_hca_record_t, char*, node_name, , NULL, )
		STRING_PROPERTY(csmi_hca_record_t, char*, board_id, , NULL, )
		STRING_PROPERTY(csmi_hca_record_t, char*, device_name, , NULL, )
		STRING_PROPERTY(csmi_hca_record_t, char*, fw_ver, , NULL, )
		STRING_PROPERTY(csmi_hca_record_t, char*, guid, , NULL, )
		STRING_PROPERTY(csmi_hca_record_t, char*, hw_rev, , NULL, )
		STRING_PROPERTY(csmi_hca_record_t, char*, part_number, , NULL, )
		STRING_PROPERTY(csmi_hca_record_t, char*, pci_bus_id, , NULL, );

    class_<csmi_ib_cable_record_t,csmi_ib_cable_record_t*>("ib_cable_record_t")
		STRING_PROPERTY(csmi_ib_cable_record_t, char*, serial_number, , NULL, )
		STRING_PROPERTY(csmi_ib_cable_record_t, char*, discovery_time, , NULL, )
		STRING_PROPERTY(csmi_ib_cable_record_t, char*, collection_time, , NULL, )
		STRING_PROPERTY(csmi_ib_cable_record_t, char*, comment, , NULL, )
		STRING_PROPERTY(csmi_ib_cable_record_t, char*, guid_s1, , NULL, )
		STRING_PROPERTY(csmi_ib_cable_record_t, char*, guid_s2, , NULL, )
		STRING_PROPERTY(csmi_ib_cable_record_t, char*, identifier, , NULL, )
		STRING_PROPERTY(csmi_ib_cable_record_t, char*, length, , NULL, )
		STRING_PROPERTY(csmi_ib_cable_record_t, char*, name, , NULL, )
		STRING_PROPERTY(csmi_ib_cable_record_t, char*, part_number, , NULL, )
		STRING_PROPERTY(csmi_ib_cable_record_t, char*, port_s1, , NULL, )
		STRING_PROPERTY(csmi_ib_cable_record_t, char*, port_s2, , NULL, )
		STRING_PROPERTY(csmi_ib_cable_record_t, char*, revision, , NULL, )
		STRING_PROPERTY(csmi_ib_cable_record_t, char*, severity, , NULL, )
		STRING_PROPERTY(csmi_ib_cable_record_t, char*, type, , NULL, )
		STRING_PROPERTY(csmi_ib_cable_record_t, char*, width, , NULL, );

    class_<csmi_ib_cable_history_record_t,csmi_ib_cable_history_record_t*>("ib_cable_history_record_t")
		STRING_PROPERTY(csmi_ib_cable_history_record_t, char*, history_time, , NULL, )
		STRING_PROPERTY(csmi_ib_cable_history_record_t, char*, serial_number, , NULL, )
		STRING_PROPERTY(csmi_ib_cable_history_record_t, char*, discovery_time, , NULL, )
		STRING_PROPERTY(csmi_ib_cable_history_record_t, char*, collection_time, , NULL, )
		STRING_PROPERTY(csmi_ib_cable_history_record_t, char*, comment, , NULL, )
		STRING_PROPERTY(csmi_ib_cable_history_record_t, char*, guid_s1, , NULL, )
		STRING_PROPERTY(csmi_ib_cable_history_record_t, char*, guid_s2, , NULL, )
		STRING_PROPERTY(csmi_ib_cable_history_record_t, char*, identifier, , NULL, )
		STRING_PROPERTY(csmi_ib_cable_history_record_t, char*, length, , NULL, )
		STRING_PROPERTY(csmi_ib_cable_history_record_t, char*, name, , NULL, )
		STRING_PROPERTY(csmi_ib_cable_history_record_t, char*, part_number, , NULL, )
		STRING_PROPERTY(csmi_ib_cable_history_record_t, char*, port_s1, , NULL, )
		STRING_PROPERTY(csmi_ib_cable_history_record_t, char*, port_s2, , NULL, )
		STRING_PROPERTY(csmi_ib_cable_history_record_t, char*, revision, , NULL, )
		STRING_PROPERTY(csmi_ib_cable_history_record_t, char*, severity, , NULL, )
		STRING_PROPERTY(csmi_ib_cable_history_record_t, char*, type, , NULL, )
		STRING_PROPERTY(csmi_ib_cable_history_record_t, char*, width, , NULL, );

    class_<csmi_node_attributes_record_t,csmi_node_attributes_record_t*>("node_attributes_record_t")
		.add_property("available_cores", &csmi_node_attributes_record_t::available_cores,&csmi_node_attributes_record_t::available_cores," Deprecated after CSM_VERSION_0_4_1. Number of physical cores on this node from all processors. ")
		.add_property("available_gpus", &csmi_node_attributes_record_t::available_gpus,&csmi_node_attributes_record_t::available_gpus," Deprecated after CSM_VERSION_0_4_1. Number of gpus available. ")
		.add_property("available_processors", &csmi_node_attributes_record_t::available_processors,&csmi_node_attributes_record_t::available_processors," Deprecated after CSM_VERSION_0_4_1. Number of processors available on the node.")
		.add_property("discovered_hcas", &csmi_node_attributes_record_t::discovered_hcas,&csmi_node_attributes_record_t::discovered_hcas," Number of IB HCAs discovered on this node during the most recent inventory collection. ")
		.add_property("hard_power_cap", &csmi_node_attributes_record_t::hard_power_cap,&csmi_node_attributes_record_t::hard_power_cap," The hard power capacity for the node, the node may not exceed this power capacity. ")
		.add_property("installed_memory", &csmi_node_attributes_record_t::installed_memory,&csmi_node_attributes_record_t::installed_memory," Amount of installed memory on this node in kB. ")
		.add_property("installed_swap", &csmi_node_attributes_record_t::installed_swap,&csmi_node_attributes_record_t::installed_swap," Amount of available swap space on this node in kB. ")
		.add_property("state", &csmi_node_attributes_record_t::state,&csmi_node_attributes_record_t::state," Deprecated after CSM_VERSION_0_4_1. State of the node, see @ref csmi_node_state_t for details. ")
		.add_property("type", &csmi_node_attributes_record_t::type,&csmi_node_attributes_record_t::type," The type of the node, see @ref csmi_node_state_t for details.")
		.add_property("ready", &csmi_node_attributes_record_t::ready,&csmi_node_attributes_record_t::ready," Deprecated after CSM_VERSION_0_4_1. The ready state of the node, used for workload management. ")
		STRING_PROPERTY(csmi_node_attributes_record_t, char*, node_name, , NULL, )
		STRING_PROPERTY(csmi_node_attributes_record_t, char*, comment, , NULL, )
		STRING_PROPERTY(csmi_node_attributes_record_t, char*, feature_1, , NULL, )
		STRING_PROPERTY(csmi_node_attributes_record_t, char*, feature_2, , NULL, )
		STRING_PROPERTY(csmi_node_attributes_record_t, char*, feature_3, , NULL, )
		STRING_PROPERTY(csmi_node_attributes_record_t, char*, feature_4, , NULL, )
		STRING_PROPERTY(csmi_node_attributes_record_t, char*, kernel_release, , NULL, )
		STRING_PROPERTY(csmi_node_attributes_record_t, char*, kernel_version, , NULL, )
		STRING_PROPERTY(csmi_node_attributes_record_t, char*, machine_model, , NULL, )
		STRING_PROPERTY(csmi_node_attributes_record_t, char*, os_image_name, , NULL, )
		STRING_PROPERTY(csmi_node_attributes_record_t, char*, os_image_uuid, , NULL, )
		STRING_PROPERTY(csmi_node_attributes_record_t, char*, physical_frame_location, , NULL, )
		STRING_PROPERTY(csmi_node_attributes_record_t, char*, physical_u_location, , NULL, )
		STRING_PROPERTY(csmi_node_attributes_record_t, char*, primary_agg, , NULL, )
		STRING_PROPERTY(csmi_node_attributes_record_t, char*, secondary_agg, , NULL, )
		STRING_PROPERTY(csmi_node_attributes_record_t, char*, serial_number, , NULL, )
		STRING_PROPERTY(csmi_node_attributes_record_t, char*, discovery_time, , NULL, )
		STRING_PROPERTY(csmi_node_attributes_record_t, char*, update_time, , NULL, )
		.add_property("discovered_cores", &csmi_node_attributes_record_t::discovered_cores,&csmi_node_attributes_record_t::discovered_cores," replacement for 'available_cores' after CSM_VERSION_0_4_1. Number of physical cores on this node from all processors. ")
		.add_property("discovered_gpus", &csmi_node_attributes_record_t::discovered_gpus,&csmi_node_attributes_record_t::discovered_gpus," replacement for 'available_gpus' after CSM_VERSION_0_4_1. Number of gpus on node. ")
		.add_property("discovered_sockets", &csmi_node_attributes_record_t::discovered_sockets,&csmi_node_attributes_record_t::discovered_sockets," replacement for 'available_processors' after CSM_VERSION_0_4_1. Number of processors on the node.")
		STRING_PROPERTY(csmi_node_attributes_record_t, char*, collection_time, , NULL, )
		.add_property("discovered_dimms", &csmi_node_attributes_record_t::discovered_dimms,&csmi_node_attributes_record_t::discovered_dimms," Number of dimms discovered via inventory on this node. ")
		.add_property("discovered_ssds", &csmi_node_attributes_record_t::discovered_ssds,&csmi_node_attributes_record_t::discovered_ssds," Number of ssds discovered via inventory on this node. ");

    class_<csmi_node_attributes_history_record_t,csmi_node_attributes_history_record_t*>("node_attributes_history_record_t")
		.add_property("available_cores", &csmi_node_attributes_history_record_t::available_cores,&csmi_node_attributes_history_record_t::available_cores," Deprecated after CSM_VERSION_0_4_1. Number of physical cores on this node from all processors. ")
		.add_property("available_gpus", &csmi_node_attributes_history_record_t::available_gpus,&csmi_node_attributes_history_record_t::available_gpus," Deprecated after CSM_VERSION_0_4_1. Number of gpus available. ")
		.add_property("available_processors", &csmi_node_attributes_history_record_t::available_processors,&csmi_node_attributes_history_record_t::available_processors," Deprecated after CSM_VERSION_0_4_1. Number of processors available on the node. ")
		.add_property("discovered_hcas", &csmi_node_attributes_history_record_t::discovered_hcas,&csmi_node_attributes_history_record_t::discovered_hcas," Number of IB HCAs discovered on this node during the most recent inventory collection. ")
		.add_property("hard_power_cap", &csmi_node_attributes_history_record_t::hard_power_cap,&csmi_node_attributes_history_record_t::hard_power_cap," The hard power capacity for the node, the node may not exceed this power capacity. ")
		.add_property("installed_memory", &csmi_node_attributes_history_record_t::installed_memory,&csmi_node_attributes_history_record_t::installed_memory," Amount of installed memory on this node in kB. ")
		.add_property("installed_swap", &csmi_node_attributes_history_record_t::installed_swap,&csmi_node_attributes_history_record_t::installed_swap," Amount of available swap space on this node in kB. ")
		.add_property("state", &csmi_node_attributes_history_record_t::state,&csmi_node_attributes_history_record_t::state," State of the node, see @ref csmi_node_state_t for details. ")
		.add_property("type", &csmi_node_attributes_history_record_t::type,&csmi_node_attributes_history_record_t::type,"The type of the node, see @ref csmi_node_state_t for details.")
		.add_property("ready", &csmi_node_attributes_history_record_t::ready,&csmi_node_attributes_history_record_t::ready," Deprecated after CSM_VERSION_0_4_1. The ready state of the node, used for workload management. ")
		STRING_PROPERTY(csmi_node_attributes_history_record_t, char*, node_name, , NULL, )
		STRING_PROPERTY(csmi_node_attributes_history_record_t, char*, comment, , NULL, )
		STRING_PROPERTY(csmi_node_attributes_history_record_t, char*, feature_1, , NULL, )
		STRING_PROPERTY(csmi_node_attributes_history_record_t, char*, feature_2, , NULL, )
		STRING_PROPERTY(csmi_node_attributes_history_record_t, char*, feature_3, , NULL, )
		STRING_PROPERTY(csmi_node_attributes_history_record_t, char*, feature_4, , NULL, )
		STRING_PROPERTY(csmi_node_attributes_history_record_t, char*, kernel_release, , NULL, )
		STRING_PROPERTY(csmi_node_attributes_history_record_t, char*, kernel_version, , NULL, )
		STRING_PROPERTY(csmi_node_attributes_history_record_t, char*, machine_model, , NULL, )
		STRING_PROPERTY(csmi_node_attributes_history_record_t, char*, os_image_name, , NULL, )
		STRING_PROPERTY(csmi_node_attributes_history_record_t, char*, os_image_uuid, , NULL, )
		STRING_PROPERTY(csmi_node_attributes_history_record_t, char*, physical_frame_location, , NULL, )
		STRING_PROPERTY(csmi_node_attributes_history_record_t, char*, physical_u_location, , NULL, )
		STRING_PROPERTY(csmi_node_attributes_history_record_t, char*, primary_agg, , NULL, )
		STRING_PROPERTY(csmi_node_attributes_history_record_t, char*, secondary_agg, , NULL, )
		STRING_PROPERTY(csmi_node_attributes_history_record_t, char*, serial_number, , NULL, )
		STRING_PROPERTY(csmi_node_attributes_history_record_t, char*, discovery_time, , NULL, )
		STRING_PROPERTY(csmi_node_attributes_history_record_t, char*, update_time, , NULL, )
		STRING_PROPERTY(csmi_node_attributes_history_record_t, char*, history_time, , NULL, )
		.add_property("discovered_cores", &csmi_node_attributes_history_record_t::discovered_cores,&csmi_node_attributes_history_record_t::discovered_cores," replacement for 'available_cores' after CSM_VERSION_0_4_1. Number of physical cores on this node from all processors. ")
		.add_property("discovered_gpus", &csmi_node_attributes_history_record_t::discovered_gpus,&csmi_node_attributes_history_record_t::discovered_gpus," replacement for 'available_gpus' after CSM_VERSION_0_4_1. Number of gpus on node. ")
		.add_property("discovered_sockets", &csmi_node_attributes_history_record_t::discovered_sockets,&csmi_node_attributes_history_record_t::discovered_sockets," replacement for 'available_processors' after CSM_VERSION_0_4_1. Number of processors on the node.")
		STRING_PROPERTY(csmi_node_attributes_history_record_t, char*, collection_time, , NULL, )
		.add_property("discovered_dimms", &csmi_node_attributes_history_record_t::discovered_dimms,&csmi_node_attributes_history_record_t::discovered_dimms," Number of dimms discovered via inventory on this node. ")
		.add_property("discovered_ssds", &csmi_node_attributes_history_record_t::discovered_ssds,&csmi_node_attributes_history_record_t::discovered_ssds," Number of ssds discovered via inventory on this node. ");

    class_<csmi_node_query_state_history_record_t,csmi_node_query_state_history_record_t*>("node_query_state_history_record_t")
		STRING_PROPERTY(csmi_node_query_state_history_record_t, char*, history_time, , NULL, )
		.add_property("state", &csmi_node_query_state_history_record_t::state,&csmi_node_query_state_history_record_t::state," State of the node, see @ref csmi_node_state_t for details. ")
		.add_property("alteration", &csmi_node_query_state_history_record_t::alteration,&csmi_node_query_state_history_record_t::alteration," Reason for the state change. ")
		STRING_PROPERTY(csmi_node_query_state_history_record_t, char*, ras_rec_id, , NULL, )
		STRING_PROPERTY(csmi_node_query_state_history_record_t, char*, ras_msg_id, , NULL, );

    class_<csmi_processor_record_t,csmi_processor_record_t*>("processor_record_t")
		.add_property("available_cores", &csmi_processor_record_t::available_cores,&csmi_processor_record_t::available_cores," Deprecated after CSM_VERSION_0_4_1. The number of physical cores available on this processor.")
		.add_property("socket", &csmi_processor_record_t::socket,&csmi_processor_record_t::socket," Deprecated after CSM_VERSION_0_4_1. The socket number of the processor. ")
		.add_property("status", &csmi_processor_record_t::status,&csmi_processor_record_t::status," Deprecated after CSM_VERSION_0_4_1. Status of the processor - [a]ctive, [d]econfigured, [f]ailure ")
		STRING_PROPERTY(csmi_processor_record_t, char*, serial_number, , NULL, )
		STRING_PROPERTY(csmi_processor_record_t, char*, node_name, , NULL, )
		.add_property("discovered_cores", &csmi_processor_record_t::discovered_cores,&csmi_processor_record_t::discovered_cores," replacement for 'available_cores' after CSM_VERSION_0_4_1. Number of physical cores on this node from all processors. ")
		STRING_PROPERTY(csmi_processor_record_t, char*, physical_location, , NULL, );

    class_<csmi_ssd_record_t,csmi_ssd_record_t*>("ssd_record_t")
		.add_property("total_size", &csmi_ssd_record_t::total_size,&csmi_ssd_record_t::total_size," Deprecated after CSM_VERSION_0_4_1. Physical total capacity (volume group total capacity). ")
		.add_property("size", &csmi_ssd_record_t::size,&csmi_ssd_record_t::size," Total capacity (in bytes) of this ssd. ")
		.add_property("wear_total_bytes_read", &csmi_ssd_record_t::wear_total_bytes_read,&csmi_ssd_record_t::wear_total_bytes_read," Number of bytes read from the SSD over the life of the device. ")
		.add_property("wear_total_bytes_written", &csmi_ssd_record_t::wear_total_bytes_written,&csmi_ssd_record_t::wear_total_bytes_written," Number of bytes written to the SSD over the life of the device. ")
		.add_property("wear_lifespan_used", &csmi_ssd_record_t::wear_lifespan_used,&csmi_ssd_record_t::wear_lifespan_used," Estimate of the amount of SSD life consumed (w.l.m. will use. valid range 0-255 percent) 0 = new, 100 = completely used, 100+ = over est life time. ")
		.add_property("wear_percent_spares_remaining", &csmi_ssd_record_t::wear_percent_spares_remaining,&csmi_ssd_record_t::wear_percent_spares_remaining," Amount of SSD capacity over-provisioning that remains.")
		.add_property("status", &csmi_ssd_record_t::status,&csmi_ssd_record_t::status," Deprecated after CSM_VERSION_0_4_1. The state of the ssd - [a]ctive, [d]econfigured, [f]ailure ")
		STRING_PROPERTY(csmi_ssd_record_t, char*, serial_number, , NULL, )
		STRING_PROPERTY(csmi_ssd_record_t, char*, node_name, , NULL, )
		STRING_PROPERTY(csmi_ssd_record_t, char*, discovery_time, , NULL, )
		STRING_PROPERTY(csmi_ssd_record_t, char*, update_time, , NULL, )
		STRING_PROPERTY(csmi_ssd_record_t, char*, device_name, , NULL, )
		STRING_PROPERTY(csmi_ssd_record_t, char*, pci_bus_id, , NULL, )
		STRING_PROPERTY(csmi_ssd_record_t, char*, fw_ver, , NULL, );

    class_<csmi_switch_record_t,csmi_switch_record_t*>("switch_record_t")
		STRING_PROPERTY(csmi_switch_record_t, char*, switch_name, , NULL, )
		STRING_PROPERTY(csmi_switch_record_t, char*, serial_number, , NULL, )
		STRING_PROPERTY(csmi_switch_record_t, char*, discovery_time, , NULL, )
		STRING_PROPERTY(csmi_switch_record_t, char*, collection_time, , NULL, )
		STRING_PROPERTY(csmi_switch_record_t, char*, comment, , NULL, )
		STRING_PROPERTY(csmi_switch_record_t, char*, description, , NULL, )
		STRING_PROPERTY(csmi_switch_record_t, char*, fw_version, , NULL, )
		STRING_PROPERTY(csmi_switch_record_t, char*, gu_id, , NULL, )
		.add_property("has_ufm_agent", &csmi_switch_record_t::has_ufm_agent,&csmi_switch_record_t::has_ufm_agent," indicate if system (Switch or Host) is running a UFM Agent ")
		STRING_PROPERTY(csmi_switch_record_t, char*, hw_version, , NULL, )
		STRING_PROPERTY(csmi_switch_record_t, char*, ip, , NULL, )
		STRING_PROPERTY(csmi_switch_record_t, char*, model, , NULL, )
		.add_property("num_modules", &csmi_switch_record_t::num_modules,&csmi_switch_record_t::num_modules," number of modules attached to this switch. This is the number of expected records in the csm_switch inventory table associated with this switch name. ")
		STRING_PROPERTY(csmi_switch_record_t, char*, physical_frame_location, , NULL, )
		STRING_PROPERTY(csmi_switch_record_t, char*, physical_u_location, , NULL, )
		STRING_PROPERTY(csmi_switch_record_t, char*, ps_id, , NULL, )
		STRING_PROPERTY(csmi_switch_record_t, char*, role, , NULL, )
		STRING_PROPERTY(csmi_switch_record_t, char*, server_operation_mode, , NULL, )
		STRING_PROPERTY(csmi_switch_record_t, char*, sm_mode, , NULL, )
		STRING_PROPERTY(csmi_switch_record_t, char*, state, , NULL, )
		STRING_PROPERTY(csmi_switch_record_t, char*, sw_version, , NULL, )
		STRING_PROPERTY(csmi_switch_record_t, char*, system_guid, , NULL, )
		STRING_PROPERTY(csmi_switch_record_t, char*, system_name, , NULL, )
		.add_property("total_alarms", &csmi_switch_record_t::total_alarms,&csmi_switch_record_t::total_alarms," total number of alarms which are currently exist on the system ")
		STRING_PROPERTY(csmi_switch_record_t, char*, type, , NULL, )
		STRING_PROPERTY(csmi_switch_record_t, char*, vendor, , NULL, );

    class_<csmi_switch_inventory_record_t,csmi_switch_inventory_record_t*>("switch_inventory_record_t")
		STRING_PROPERTY(csmi_switch_inventory_record_t, char*, name, , NULL, )
		STRING_PROPERTY(csmi_switch_inventory_record_t, char*, host_system_guid, , NULL, )
		STRING_PROPERTY(csmi_switch_inventory_record_t, char*, discovery_time, , NULL, )
		STRING_PROPERTY(csmi_switch_inventory_record_t, char*, collection_time, , NULL, )
		STRING_PROPERTY(csmi_switch_inventory_record_t, char*, comment, , NULL, )
		STRING_PROPERTY(csmi_switch_inventory_record_t, char*, description, , NULL, )
		STRING_PROPERTY(csmi_switch_inventory_record_t, char*, device_name, , NULL, )
		STRING_PROPERTY(csmi_switch_inventory_record_t, char*, device_type, , NULL, )
		STRING_PROPERTY(csmi_switch_inventory_record_t, char*, hw_version, , NULL, )
		.add_property("max_ib_ports", &csmi_switch_inventory_record_t::max_ib_ports,&csmi_switch_inventory_record_t::max_ib_ports," maximum number of external ports of this module. ")
		.add_property("module_index", &csmi_switch_inventory_record_t::module_index,&csmi_switch_inventory_record_t::module_index," index of module. Each module type has separate index: FAN1,FAN2,FAN3...S1,PS2 ")
		.add_property("number_of_chips", &csmi_switch_inventory_record_t::number_of_chips,&csmi_switch_inventory_record_t::number_of_chips," number of chips which are contained in this module. (relevant only for line / spine modules, for all other modules number_of_chips=0) ")
		STRING_PROPERTY(csmi_switch_inventory_record_t, char*, path, , NULL, )
		STRING_PROPERTY(csmi_switch_inventory_record_t, char*, serial_number, , NULL, )
		STRING_PROPERTY(csmi_switch_inventory_record_t, char*, severity, , NULL, )
		STRING_PROPERTY(csmi_switch_inventory_record_t, char*, status, , NULL, );

    class_<csmi_switch_ports_record_t,csmi_switch_ports_record_t*>("switch_ports_record_t")
		STRING_PROPERTY(csmi_switch_ports_record_t, char*, name, , NULL, )
		STRING_PROPERTY(csmi_switch_ports_record_t, char*, parent, , NULL, )
		STRING_PROPERTY(csmi_switch_ports_record_t, char*, discovery_time, , NULL, )
		STRING_PROPERTY(csmi_switch_ports_record_t, char*, collection_time, , NULL, )
		STRING_PROPERTY(csmi_switch_ports_record_t, char*, active_speed, , NULL, )
		STRING_PROPERTY(csmi_switch_ports_record_t, char*, comment, , NULL, )
		STRING_PROPERTY(csmi_switch_ports_record_t, char*, description, , NULL, )
		STRING_PROPERTY(csmi_switch_ports_record_t, char*, enabled_speed, , NULL, )
		STRING_PROPERTY(csmi_switch_ports_record_t, char*, external_number, , NULL, )
		STRING_PROPERTY(csmi_switch_ports_record_t, char*, guid, , NULL, )
		STRING_PROPERTY(csmi_switch_ports_record_t, char*, lid, , NULL, )
		STRING_PROPERTY(csmi_switch_ports_record_t, char*, max_supported_speed, , NULL, )
		STRING_PROPERTY(csmi_switch_ports_record_t, char*, logical_state, , NULL, )
		STRING_PROPERTY(csmi_switch_ports_record_t, char*, mirror, , NULL, )
		STRING_PROPERTY(csmi_switch_ports_record_t, char*, mirror_traffic, , NULL, )
		STRING_PROPERTY(csmi_switch_ports_record_t, char*, module, , NULL, )
		STRING_PROPERTY(csmi_switch_ports_record_t, char*, mtu, , NULL, )
		STRING_PROPERTY(csmi_switch_ports_record_t, char*, number, , NULL, )
		STRING_PROPERTY(csmi_switch_ports_record_t, char*, physical_state, , NULL, )
		STRING_PROPERTY(csmi_switch_ports_record_t, char*, peer, , NULL, )
		STRING_PROPERTY(csmi_switch_ports_record_t, char*, severity, , NULL, )
		STRING_PROPERTY(csmi_switch_ports_record_t, char*, supported_speed, , NULL, )
		STRING_PROPERTY(csmi_switch_ports_record_t, char*, system_guid, , NULL, )
		STRING_PROPERTY(csmi_switch_ports_record_t, char*, tier, , NULL, )
		STRING_PROPERTY(csmi_switch_ports_record_t, char*, width_active, , NULL, )
		STRING_PROPERTY(csmi_switch_ports_record_t, char*, width_enabled, , NULL, )
		STRING_PROPERTY(csmi_switch_ports_record_t, char*, width_supported, , NULL, );

    class_<csmi_switch_details_t,csmi_switch_details_t*>("switch_details_t")
		STRUCT_PROPERTY(csmi_switch_details_t, csmi_switch_record_t*, switch_data, , NULL, &csmi_switch_details_t::switch_data)
		.add_property("inventory_count", &csmi_switch_details_t::inventory_count,&csmi_switch_details_t::inventory_count," Number of elements in the 'inventory' array. ")
		ARRAY_STRUCT_PROPERTY(csmi_switch_details_t, csmi_switch_inventory_record_t**, inventory, inventory_count, NULL, csmi_switch_inventory_record_t)
		.add_property("ports_count", &csmi_switch_details_t::ports_count,&csmi_switch_details_t::ports_count," Deprecated after CSM_VERSION_0_4_1. Number of elements in the 'ports' array. ")
		ARRAY_STRUCT_PROPERTY(csmi_switch_details_t, csmi_switch_ports_record_t**, ports, ports_count, NULL, csmi_switch_ports_record_t);

    class_<csmi_switch_history_record_t,csmi_switch_history_record_t*>("switch_history_record_t")
		STRING_PROPERTY(csmi_switch_history_record_t, char*, history_time, , NULL, )
		STRING_PROPERTY(csmi_switch_history_record_t, char*, switch_name, , NULL, )
		STRING_PROPERTY(csmi_switch_history_record_t, char*, serial_number, , NULL, )
		STRING_PROPERTY(csmi_switch_history_record_t, char*, discovery_time, , NULL, )
		STRING_PROPERTY(csmi_switch_history_record_t, char*, collection_time, , NULL, )
		STRING_PROPERTY(csmi_switch_history_record_t, char*, comment, , NULL, )
		STRING_PROPERTY(csmi_switch_history_record_t, char*, description, , NULL, )
		STRING_PROPERTY(csmi_switch_history_record_t, char*, fw_version, , NULL, )
		STRING_PROPERTY(csmi_switch_history_record_t, char*, gu_id, , NULL, )
		.add_property("has_ufm_agent", &csmi_switch_history_record_t::has_ufm_agent,&csmi_switch_history_record_t::has_ufm_agent," indicate if system (Switch or Host) is running a UFM Agent ")
		STRING_PROPERTY(csmi_switch_history_record_t, char*, hw_version, , NULL, )
		STRING_PROPERTY(csmi_switch_history_record_t, char*, ip, , NULL, )
		STRING_PROPERTY(csmi_switch_history_record_t, char*, model, , NULL, )
		.add_property("num_modules", &csmi_switch_history_record_t::num_modules,&csmi_switch_history_record_t::num_modules," number of modules attached to this switch. This is the number of expected records in the csm_switch inventory table associated with this switch name. ")
		STRING_PROPERTY(csmi_switch_history_record_t, char*, physical_frame_location, , NULL, )
		STRING_PROPERTY(csmi_switch_history_record_t, char*, physical_u_location, , NULL, )
		STRING_PROPERTY(csmi_switch_history_record_t, char*, ps_id, , NULL, )
		STRING_PROPERTY(csmi_switch_history_record_t, char*, role, , NULL, )
		STRING_PROPERTY(csmi_switch_history_record_t, char*, server_operation_mode, , NULL, )
		STRING_PROPERTY(csmi_switch_history_record_t, char*, sm_mode, , NULL, )
		STRING_PROPERTY(csmi_switch_history_record_t, char*, state, , NULL, )
		STRING_PROPERTY(csmi_switch_history_record_t, char*, sw_version, , NULL, )
		STRING_PROPERTY(csmi_switch_history_record_t, char*, system_guid, , NULL, )
		STRING_PROPERTY(csmi_switch_history_record_t, char*, system_name, , NULL, )
		.add_property("total_alarms", &csmi_switch_history_record_t::total_alarms,&csmi_switch_history_record_t::total_alarms," total number of alarms which are currently exist on the system ")
		STRING_PROPERTY(csmi_switch_history_record_t, char*, type, , NULL, )
		STRING_PROPERTY(csmi_switch_history_record_t, char*, vendor, , NULL, )
		STRING_PROPERTY(csmi_switch_history_record_t, char*, operation, , NULL, )
		STRING_PROPERTY(csmi_switch_history_record_t, char*, archive_history_time, , NULL, );

    class_<csmi_node_env_data_t,csmi_node_env_data_t*>("node_env_data_t")
		.add_property("field_01", &csmi_node_env_data_t::field_01,&csmi_node_env_data_t::field_01," Placeholder value. ");

    class_<csmi_switch_env_data_t,csmi_switch_env_data_t*>("switch_env_data_t")
		.add_property("field_01", &csmi_switch_env_data_t::field_01,&csmi_switch_env_data_t::field_01," Unused. ");

    class_<csmi_fabric_topology_t,csmi_fabric_topology_t*>("fabric_topology_t")
		.add_property("field_01", &csmi_fabric_topology_t::field_01,&csmi_fabric_topology_t::field_01," ");

    class_<csmi_node_details_t,csmi_node_details_t*>("node_details_t")
		.add_property("dimms_count", &csmi_node_details_t::dimms_count,&csmi_node_details_t::dimms_count," Number of dimms associated with @ref node, size of @ref dimms. ")
		.add_property("gpus_count", &csmi_node_details_t::gpus_count,&csmi_node_details_t::gpus_count," Number of gpus associated with @ref node, size of @ref gpus.")
		.add_property("hcas_count", &csmi_node_details_t::hcas_count,&csmi_node_details_t::hcas_count," Number of hcas associated with @ref node, size of @ref hcas.")
		.add_property("processors_count", &csmi_node_details_t::processors_count,&csmi_node_details_t::processors_count," Number of processors associated with @ref node, size of @ref processors.")
		.add_property("ssds_count", &csmi_node_details_t::ssds_count,&csmi_node_details_t::ssds_count," Number of ssds associated with @ref node, size of @ref ssds. ")
		STRUCT_PROPERTY(csmi_node_details_t, csmi_node_attributes_record_t*, node, , NULL, &csmi_node_details_t::node)
		ARRAY_STRUCT_PROPERTY(csmi_node_details_t, csmi_dimm_record_t**, dimms, dimms_count, NULL, csmi_dimm_record_t)
		ARRAY_STRUCT_PROPERTY(csmi_node_details_t, csmi_gpu_record_t**, gpus, gpus_count, NULL, csmi_gpu_record_t)
		ARRAY_STRUCT_PROPERTY(csmi_node_details_t, csmi_hca_record_t**, hcas, hcas_count, NULL, csmi_hca_record_t)
		ARRAY_STRUCT_PROPERTY(csmi_node_details_t, csmi_processor_record_t**, processors, processors_count, NULL, csmi_processor_record_t)
		ARRAY_STRUCT_PROPERTY(csmi_node_details_t, csmi_ssd_record_t**, ssds, ssds_count, NULL, csmi_ssd_record_t);

    class_<csmi_cluster_query_state_record_t,csmi_cluster_query_state_record_t*>("cluster_query_state_record_t")
		STRING_PROPERTY(csmi_cluster_query_state_record_t, char*, node_name, , NULL, )
		STRING_PROPERTY(csmi_cluster_query_state_record_t, char*, collection_time, , NULL, )
		STRING_PROPERTY(csmi_cluster_query_state_record_t, char*, update_time, , NULL, )
		.add_property("state", &csmi_cluster_query_state_record_t::state,&csmi_cluster_query_state_record_t::state," Deprecated after CSM_VERSION_0_4_1. State of the node, see @ref csmi_node_state_t for details. ")
		.add_property("type", &csmi_cluster_query_state_record_t::type,&csmi_cluster_query_state_record_t::type," The type of the node, see @ref csmi_node_state_t for details.")
		.add_property("num_allocs", &csmi_cluster_query_state_record_t::num_allocs,&csmi_cluster_query_state_record_t::num_allocs," Number of allocations that this node is participating in. also the length member for the following arrays ")
		ARRAY_PROPERTY(csmi_cluster_query_state_record_t, int64_t*, allocs, num_allocs, NULL, int64_t)
		ARRAY_STR_PROPERTY(csmi_cluster_query_state_record_t, char**, states, num_allocs, NULL, )
		ARRAY_STR_PROPERTY(csmi_cluster_query_state_record_t, char**, shared, num_allocs, NULL, );

    class_<csmi_node_find_job_record_t,csmi_node_find_job_record_t*>("node_find_job_record_t")
		STRING_PROPERTY(csmi_node_find_job_record_t, char*, node_name, , NULL, )
		.add_property("allocation_id", &csmi_node_find_job_record_t::allocation_id,&csmi_node_find_job_record_t::allocation_id," The allocation_id of the job.")
		.add_property("primary_job_id", &csmi_node_find_job_record_t::primary_job_id,&csmi_node_find_job_record_t::primary_job_id," The primary_job_id of the job.")
		STRING_PROPERTY(csmi_node_find_job_record_t, char*, user_name, , NULL, )
		.add_property("num_nodes", &csmi_node_find_job_record_t::num_nodes,&csmi_node_find_job_record_t::num_nodes," The number of nodes participating in this job.")
		STRING_PROPERTY(csmi_node_find_job_record_t, char*, begin_time, , NULL, )
		STRING_PROPERTY(csmi_node_find_job_record_t, char*, end_time, , NULL, );

    class_<csm_ib_cable_inventory_collection_input_t,csm_ib_cable_inventory_collection_input_t*>("ib_cable_inventory_collection_input_t")
		.add_property("inventory_count", &csm_ib_cable_inventory_collection_input_t::inventory_count,&csm_ib_cable_inventory_collection_input_t::inventory_count," Number of ib cable records, size of @ref inventory. ")
		ARRAY_STRUCT_PROPERTY(csm_ib_cable_inventory_collection_input_t, csmi_ib_cable_record_t**, inventory, inventory_count, NULL, csmi_ib_cable_record_t);

    class_<csm_ib_cable_inventory_collection_output_t,csm_ib_cable_inventory_collection_output_t*>("ib_cable_inventory_collection_output_t")
		.add_property("insert_count", &csm_ib_cable_inventory_collection_output_t::insert_count,&csm_ib_cable_inventory_collection_output_t::insert_count," number of new records inserted into the database. ")
		.add_property("update_count", &csm_ib_cable_inventory_collection_output_t::update_count,&csm_ib_cable_inventory_collection_output_t::update_count," number of old records updated in the database. ")
		.add_property("delete_count", &csm_ib_cable_inventory_collection_output_t::delete_count,&csm_ib_cable_inventory_collection_output_t::delete_count," number of old records removed from the database. ");

    class_<csm_ib_cable_query_input_t,csm_ib_cable_query_input_t*>("ib_cable_query_input_t")
		.add_property("limit", &csm_ib_cable_query_input_t::limit,&csm_ib_cable_query_input_t::limit," SQL 'LIMIT' numeric value. API will ignore values less than 1.")
		.add_property("offset", &csm_ib_cable_query_input_t::offset,&csm_ib_cable_query_input_t::offset," SQL 'OFFSET' numeric value. API will ignore values less than 1.")
		.add_property("serial_numbers_count", &csm_ib_cable_query_input_t::serial_numbers_count,&csm_ib_cable_query_input_t::serial_numbers_count," Number of serial numbers to query on, size of @ref serial_numbers. ")
		ARRAY_STR_PROPERTY(csm_ib_cable_query_input_t, char**, serial_numbers, serial_numbers_count, NULL, );

    class_<csm_ib_cable_query_output_t,csm_ib_cable_query_output_t*>("ib_cable_query_output_t")
		.add_property("results_count", &csm_ib_cable_query_output_t::results_count,&csm_ib_cable_query_output_t::results_count," Number of records retrieved by the query, size of @ref results.")
		ARRAY_STRUCT_PROPERTY(csm_ib_cable_query_output_t, csmi_ib_cable_record_t**, results, results_count, NULL, csmi_ib_cable_record_t);

    class_<csm_ib_cable_query_history_input_t,csm_ib_cable_query_history_input_t*>("ib_cable_query_history_input_t")
		.add_property("limit", &csm_ib_cable_query_history_input_t::limit,&csm_ib_cable_query_history_input_t::limit," SQL 'LIMIT' numeric value. API will ignore values less than 1.")
		.add_property("offset", &csm_ib_cable_query_history_input_t::offset,&csm_ib_cable_query_history_input_t::offset," SQL 'OFFSET' numeric value. API will ignore values less than 1.")
		STRING_PROPERTY(csm_ib_cable_query_history_input_t, char*, serial_number, , NULL, );

    class_<csm_ib_cable_query_history_output_t,csm_ib_cable_query_history_output_t*>("ib_cable_query_history_output_t")
		.add_property("results_count", &csm_ib_cable_query_history_output_t::results_count,&csm_ib_cable_query_history_output_t::results_count," Number of records retrieved, size of @ref results. ")
		ARRAY_STRUCT_PROPERTY(csm_ib_cable_query_history_output_t, csmi_ib_cable_history_record_t**, results, results_count, NULL, csmi_ib_cable_history_record_t);

    class_<csm_ib_cable_update_input_t,csm_ib_cable_update_input_t*>("ib_cable_update_input_t")
		.add_property("serial_numbers_count", &csm_ib_cable_update_input_t::serial_numbers_count,&csm_ib_cable_update_input_t::serial_numbers_count," Number of serial numbers to update, size of @ref serial_numbers.")
		ARRAY_STR_PROPERTY(csm_ib_cable_update_input_t, char**, serial_numbers, serial_numbers_count, NULL, )
		STRING_PROPERTY(csm_ib_cable_update_input_t, char*, comment, , NULL, )
		STRING_PROPERTY(csm_ib_cable_update_input_t, char*, guid_s1, , NULL, )
		STRING_PROPERTY(csm_ib_cable_update_input_t, char*, guid_s2, , NULL, )
		STRING_PROPERTY(csm_ib_cable_update_input_t, char*, port_s1, , NULL, )
		STRING_PROPERTY(csm_ib_cable_update_input_t, char*, port_s2, , NULL, );

    class_<csm_ib_cable_update_output_t,csm_ib_cable_update_output_t*>("ib_cable_update_output_t")
		.add_property("failure_count", &csm_ib_cable_update_output_t::failure_count,&csm_ib_cable_update_output_t::failure_count," The number of ib cables that failed to be updated, size of @ref failure_ib_cables.")
		ARRAY_STR_PROPERTY(csm_ib_cable_update_output_t, char**, failure_ib_cables, failure_count, NULL, );

    class_<csm_node_attributes_query_input_t,csm_node_attributes_query_input_t*>("node_attributes_query_input_t")
		.add_property("limit", &csm_node_attributes_query_input_t::limit,&csm_node_attributes_query_input_t::limit," SQL 'LIMIT' numeric value. API will ignore values less than 1.")
		.add_property("offset", &csm_node_attributes_query_input_t::offset,&csm_node_attributes_query_input_t::offset," SQL 'OFFSET' numeric value. API will ignore values less than 1.")
		.add_property("node_names_count", &csm_node_attributes_query_input_t::node_names_count,&csm_node_attributes_query_input_t::node_names_count," Number of names to query, size of @ref node_names. ")
		.add_property("ready", &csm_node_attributes_query_input_t::ready,&csm_node_attributes_query_input_t::ready," Deprecated after CSM_VERSION_0_4_1. Query the 'ready' field in the database. API will ignore @ref CSM_UNDEF_BOOL for this field.  Valid values: 0,1,2. API checks for invalid values and fails if invalid values are passed. Database description: Is the node ready for workload manager? ('y' or 'n') ")
		ARRAY_STR_PROPERTY(csm_node_attributes_query_input_t, char**, node_names, node_names_count, NULL, )
		STRING_PROPERTY(csm_node_attributes_query_input_t, char*, comment, , NULL, )
		.add_property("type", &csm_node_attributes_query_input_t::type,&csm_node_attributes_query_input_t::type," Query the 'type' field in the database. API will ignore @ref csmi_node_state_t::CSM_NODE_NO_TYPE values for this fields, see @ref csmi_node_state_t for details.")
		.add_property("state", &csm_node_attributes_query_input_t::state,&csm_node_attributes_query_input_t::state," replacement for 'ready' after CSM_VERSION_0_4_1. Query the 'state' field in the database. API will ignore @ref CSM_NODE_NO_DEF for this field. API checks for invalid values and fails if invalid values are passed. State of the node, see @ref csmi_node_state_t for details. ");

    class_<csm_node_attributes_query_output_t,csm_node_attributes_query_output_t*>("node_attributes_query_output_t")
		.add_property("results_count", &csm_node_attributes_query_output_t::results_count,&csm_node_attributes_query_output_t::results_count," Number of node records retrieved, size of @ref results. ")
		ARRAY_STRUCT_PROPERTY(csm_node_attributes_query_output_t, csmi_node_attributes_record_t**, results, results_count, NULL, csmi_node_attributes_record_t);

    class_<csm_node_attributes_query_details_input_t,csm_node_attributes_query_details_input_t*>("node_attributes_query_details_input_t")
		STRING_PROPERTY(csm_node_attributes_query_details_input_t, char*, node_name, , NULL, );

    class_<csm_node_attributes_query_details_output_t,csm_node_attributes_query_details_output_t*>("node_attributes_query_details_output_t")
		.add_property("result_count", &csm_node_attributes_query_details_output_t::result_count,&csm_node_attributes_query_details_output_t::result_count," Number of node records, size of @ref result. ")
		ARRAY_STRUCT_PROPERTY(csm_node_attributes_query_details_output_t, csmi_node_details_t**, result, result_count, NULL, csmi_node_details_t);

    class_<csm_node_attributes_query_history_input_t,csm_node_attributes_query_history_input_t*>("node_attributes_query_history_input_t")
		.add_property("limit", &csm_node_attributes_query_history_input_t::limit,&csm_node_attributes_query_history_input_t::limit," SQL 'LIMIT' numeric value. API will ignore values less than 1.")
		.add_property("offset", &csm_node_attributes_query_history_input_t::offset,&csm_node_attributes_query_history_input_t::offset," SQL 'OFFSET' numeric value. API will ignore values less than 1.")
		STRING_PROPERTY(csm_node_attributes_query_history_input_t, char*, node_name, , NULL, )
		.add_property("order_by", &csm_node_attributes_query_history_input_t::order_by,&csm_node_attributes_query_history_input_t::order_by," Used to alter 'ORDER BY'. API will ignore NULL values. Default to 'ORDER BY history_time DESC NULLS LAST'. VALID VALUES: [d] =  'ORDER BY history_time DESC NULLS LAST', [a] = 'ORDER BY history_time ASC NULLS LAST'. @TODO should this be a boolean?");

    class_<csm_node_attributes_query_history_output_t,csm_node_attributes_query_history_output_t*>("node_attributes_query_history_output_t")
		.add_property("results_count", &csm_node_attributes_query_history_output_t::results_count,&csm_node_attributes_query_history_output_t::results_count," Number of history records retrieved by the query, size of @ref results. ")
		ARRAY_STRUCT_PROPERTY(csm_node_attributes_query_history_output_t, csmi_node_attributes_history_record_t**, results, results_count, NULL, csmi_node_attributes_history_record_t);

    class_<csm_node_query_state_history_input_t,csm_node_query_state_history_input_t*>("node_query_state_history_input_t")
		.add_property("limit", &csm_node_query_state_history_input_t::limit,&csm_node_query_state_history_input_t::limit," SQL 'LIMIT' numeric value. API will ignore values less than 1.")
		.add_property("offset", &csm_node_query_state_history_input_t::offset,&csm_node_query_state_history_input_t::offset," SQL 'OFFSET' numeric value. API will ignore values less than 1.")
		STRING_PROPERTY(csm_node_query_state_history_input_t, char*, node_name, , NULL, )
		.add_property("order_by", &csm_node_query_state_history_input_t::order_by,&csm_node_query_state_history_input_t::order_by," Used to alter 'ORDER BY'. API will ignore NULL values. Default to 'ORDER BY history_time DESC NULLS LAST'. VALID VALUES: [d] =  'ORDER BY history_time DESC NULLS LAST', [a] = 'ORDER BY history_time ASC NULLS LAST'. @TODO should this be a boolean?");

    class_<csm_node_query_state_history_output_t,csm_node_query_state_history_output_t*>("node_query_state_history_output_t")
		STRING_PROPERTY(csm_node_query_state_history_output_t, char*, node_name, , NULL, )
		.add_property("results_count", &csm_node_query_state_history_output_t::results_count,&csm_node_query_state_history_output_t::results_count," Number of history records retrieved by the query, size of @ref results. ")
		ARRAY_STRUCT_PROPERTY(csm_node_query_state_history_output_t, csmi_node_query_state_history_record_t**, results, results_count, NULL, csmi_node_query_state_history_record_t);

    class_<csm_node_attributes_update_input_t,csm_node_attributes_update_input_t*>("node_attributes_update_input_t")
		.add_property("node_names_count", &csm_node_attributes_update_input_t::node_names_count,&csm_node_attributes_update_input_t::node_names_count," The number of nodes to update, size of @ref node_names. ")
		.add_property("state", &csm_node_attributes_update_input_t::state,&csm_node_attributes_update_input_t::state," State of the node, see @ref csmi_node_state_t for details. ")
		.add_property("ready", &csm_node_attributes_update_input_t::ready,&csm_node_attributes_update_input_t::ready," Deprecated after CSM_VERSION_0_4_1. Query the 'ready' field in the database. API will ignore @ref CSM_UNDEF_BOOL for this field.  Valid values: 0,1,2. API checks for invalid values and fails if invalid values are passed. Database description: Is the node ready for workload manager? ('y' or 'n') ")
		STRING_PROPERTY(csm_node_attributes_update_input_t, char*, comment, , NULL, )
		STRING_PROPERTY(csm_node_attributes_update_input_t, char*, feature_1, , NULL, )
		STRING_PROPERTY(csm_node_attributes_update_input_t, char*, feature_2, , NULL, )
		STRING_PROPERTY(csm_node_attributes_update_input_t, char*, feature_3, , NULL, )
		STRING_PROPERTY(csm_node_attributes_update_input_t, char*, feature_4, , NULL, )
		STRING_PROPERTY(csm_node_attributes_update_input_t, char*, physical_frame_location, , NULL, )
		STRING_PROPERTY(csm_node_attributes_update_input_t, char*, physical_u_location, , NULL, )
		ARRAY_STR_PROPERTY(csm_node_attributes_update_input_t, char**, node_names, node_names_count, NULL, );

    class_<csm_node_attributes_update_output_t,csm_node_attributes_update_output_t*>("node_attributes_update_output_t")
		.add_property("failure_count", &csm_node_attributes_update_output_t::failure_count,&csm_node_attributes_update_output_t::failure_count," The number of nodes which failed to be updated, size of @ref failure_node_names. ")
		ARRAY_STR_PROPERTY(csm_node_attributes_update_output_t, char**, failure_node_names, failure_count, NULL, );

    class_<csm_node_delete_input_t,csm_node_delete_input_t*>("node_delete_input_t")
		.add_property("node_names_count", &csm_node_delete_input_t::node_names_count,&csm_node_delete_input_t::node_names_count," Number of nodes to be deleted, size of @ref node_names. ")
		ARRAY_STR_PROPERTY(csm_node_delete_input_t, char**, node_names, node_names_count, NULL, );

    class_<csm_node_delete_output_t,csm_node_delete_output_t*>("node_delete_output_t")
		.add_property("failure_count", &csm_node_delete_output_t::failure_count,&csm_node_delete_output_t::failure_count," The number of nodes which failed to be deleted, size of @ref failure_node_names. ")
		ARRAY_STR_PROPERTY(csm_node_delete_output_t, char**, failure_node_names, failure_count, NULL, );

    class_<csm_node_find_job_input_t,csm_node_find_job_input_t*>("node_find_job_input_t")
		.add_property("limit", &csm_node_find_job_input_t::limit,&csm_node_find_job_input_t::limit," SQL 'LIMIT' numeric value. API will ignore values less than 1.")
		.add_property("offset", &csm_node_find_job_input_t::offset,&csm_node_find_job_input_t::offset," SQL 'OFFSET' numeric value. API will ignore values less than 1.")
		.add_property("order_by", &csm_node_find_job_input_t::order_by,&csm_node_find_job_input_t::order_by," Used to alter 'ORDER BY'. API will ignore NULL values. Default to 'ORDER BY node_name, allocation_id ASC NULLS LAST'. VALID VALUES: [a] = 'ORDER BY node_name, allocation_id ASC NULLS LAST'\", [b] = 'ORDER BY node_name, allocation_id DESC NULLS LAST'\", [c] = 'ORDER BY allocation_id ASC NULLS LAST'\", [d] = 'ORDER BY allocation_id DESC NULLS LAST'\", [e] = 'ORDER BY primary_job_id ASC NULLS LAST'\", [f] = 'ORDER BY primary_job_id DESC NULLS LAST'\", [g] = 'ORDER BY user_name ASC NULLS LAST'\", [h] = 'ORDER BY user_name DESC NULLS LAST'\", [i] = 'ORDER BY num_nodes ASC NULLS LAST'\", [j] = 'ORDER BY num_nodes DESC NULLS LAST'\", [k] = 'ORDER BY begin_time ASC NULLS LAST'\", [l] = 'ORDER BY begin_time DESC NULLS LAST'\", [m] = 'ORDER BY end_time ASC NULLS LAST'\", [n] = 'ORDER BY end_time DESC NULLS LAST'\" ")
		.add_property("node_names_count", &csm_node_find_job_input_t::node_names_count,&csm_node_find_job_input_t::node_names_count," Number of names to query, size of @ref node_names. ")
		ARRAY_STR_PROPERTY(csm_node_find_job_input_t, char**, node_names, node_names_count, NULL, )
		STRING_PROPERTY(csm_node_find_job_input_t, char*, begin_time_search_begin, , NULL, )
		STRING_PROPERTY(csm_node_find_job_input_t, char*, begin_time_search_end, , NULL, )
		STRING_PROPERTY(csm_node_find_job_input_t, char*, end_time_search_begin, , NULL, )
		STRING_PROPERTY(csm_node_find_job_input_t, char*, end_time_search_end, , NULL, )
		STRING_PROPERTY(csm_node_find_job_input_t, char*, midpoint, , NULL, )
		STRING_PROPERTY(csm_node_find_job_input_t, char*, midpoint_interval, , NULL, )
		STRING_PROPERTY(csm_node_find_job_input_t, char*, search_range_begin, , NULL, )
		STRING_PROPERTY(csm_node_find_job_input_t, char*, search_range_end, , NULL, )
		STRING_PROPERTY(csm_node_find_job_input_t, char*, user_name, , NULL, );

    class_<csm_node_find_job_output_t,csm_node_find_job_output_t*>("node_find_job_output_t")
		.add_property("results_count", &csm_node_find_job_output_t::results_count,&csm_node_find_job_output_t::results_count," Number of database records retrieved, size of @ref results. ")
		ARRAY_STRUCT_PROPERTY(csm_node_find_job_output_t, csmi_node_find_job_record_t**, results, results_count, NULL, csmi_node_find_job_record_t);

    class_<csm_switch_attributes_query_input_t,csm_switch_attributes_query_input_t*>("switch_attributes_query_input_t")
		.add_property("limit", &csm_switch_attributes_query_input_t::limit,&csm_switch_attributes_query_input_t::limit," SQL 'LIMIT' numeric value. API will ignore values less than 1.")
		.add_property("offset", &csm_switch_attributes_query_input_t::offset,&csm_switch_attributes_query_input_t::offset," SQL 'OFFSET' numeric value. API will ignore values less than 1.")
		.add_property("switch_names_count", &csm_switch_attributes_query_input_t::switch_names_count,&csm_switch_attributes_query_input_t::switch_names_count," Number of switches being queried, size of @ref switch_names. ")
		STRING_PROPERTY(csm_switch_attributes_query_input_t, char*, state, , NULL, )
		ARRAY_STR_PROPERTY(csm_switch_attributes_query_input_t, char**, switch_names, switch_names_count, NULL, )
		STRING_PROPERTY(csm_switch_attributes_query_input_t, char*, serial_number, , NULL, )
		.add_property("order_by", &csm_switch_attributes_query_input_t::order_by,&csm_switch_attributes_query_input_t::order_by," Used to alter 'ORDER BY'. API will ignore NULL values. Default to 'ORDER BY switch_name ASC NULLS LAST'. VALID VALUES: [a] = 'ORDER BY switch_name ASC NULLS LAST', [b] =  'ORDER BY switch_name DESC NULLS LAST' ");

    class_<csm_switch_attributes_query_output_t,csm_switch_attributes_query_output_t*>("switch_attributes_query_output_t")
		.add_property("results_count", &csm_switch_attributes_query_output_t::results_count,&csm_switch_attributes_query_output_t::results_count," Number of switch records, size of @ref results. ")
		ARRAY_STRUCT_PROPERTY(csm_switch_attributes_query_output_t, csmi_switch_record_t**, results, results_count, NULL, csmi_switch_record_t);

    class_<csm_switch_attributes_query_details_input_t,csm_switch_attributes_query_details_input_t*>("switch_attributes_query_details_input_t")
		STRING_PROPERTY(csm_switch_attributes_query_details_input_t, char*, switch_name, , NULL, );

    class_<csm_switch_attributes_query_details_output_t,csm_switch_attributes_query_details_output_t*>("switch_attributes_query_details_output_t")
		.add_property("result_count", &csm_switch_attributes_query_details_output_t::result_count,&csm_switch_attributes_query_details_output_t::result_count," Number of records returned from the SQL query, and length of the result array. only values this should return are 1 and 0. 1 = success, 0 = no records match. ")
		ARRAY_STRUCT_PROPERTY(csm_switch_attributes_query_details_output_t, csmi_switch_details_t**, result, result_count, NULL, csmi_switch_details_t);

    class_<csm_switch_attributes_query_history_input_t,csm_switch_attributes_query_history_input_t*>("switch_attributes_query_history_input_t")
		.add_property("limit", &csm_switch_attributes_query_history_input_t::limit,&csm_switch_attributes_query_history_input_t::limit," SQL 'LIMIT' numeric value. API will ignore values less than 1.")
		.add_property("offset", &csm_switch_attributes_query_history_input_t::offset,&csm_switch_attributes_query_history_input_t::offset," SQL 'OFFSET' numeric value. API will ignore values less than 1.")
		STRING_PROPERTY(csm_switch_attributes_query_history_input_t, char*, switch_name, , NULL, );

    class_<csm_switch_attributes_query_history_output_t,csm_switch_attributes_query_history_output_t*>("switch_attributes_query_history_output_t")
		.add_property("results_count", &csm_switch_attributes_query_history_output_t::results_count,&csm_switch_attributes_query_history_output_t::results_count," Number of records retrieved by the query, size of @ref results. ")
		ARRAY_STRUCT_PROPERTY(csm_switch_attributes_query_history_output_t, csmi_switch_history_record_t**, results, results_count, NULL, csmi_switch_history_record_t);

    class_<csm_switch_attributes_update_input_t,csm_switch_attributes_update_input_t*>("switch_attributes_update_input_t")
		.add_property("switch_names_count", &csm_switch_attributes_update_input_t::switch_names_count,&csm_switch_attributes_update_input_t::switch_names_count," Number of switch records to update, size of @ref switch_names. ")
		STRING_PROPERTY(csm_switch_attributes_update_input_t, char*, comment, , NULL, )
		STRING_PROPERTY(csm_switch_attributes_update_input_t, char*, physical_frame_location, , NULL, )
		STRING_PROPERTY(csm_switch_attributes_update_input_t, char*, physical_u_location, , NULL, )
		STRING_PROPERTY(csm_switch_attributes_update_input_t, char*, state, , NULL, )
		ARRAY_STR_PROPERTY(csm_switch_attributes_update_input_t, char**, switch_names, switch_names_count, NULL, );

    class_<csm_switch_attributes_update_output_t,csm_switch_attributes_update_output_t*>("switch_attributes_update_output_t")
		.add_property("failure_count", &csm_switch_attributes_update_output_t::failure_count,&csm_switch_attributes_update_output_t::failure_count," The number of failures which occurred in the update, size of @ref failure_switches. ")
		ARRAY_STR_PROPERTY(csm_switch_attributes_update_output_t, char**, failure_switches, failure_count, NULL, );

    class_<csm_switch_inventory_collection_input_t,csm_switch_inventory_collection_input_t*>("switch_inventory_collection_input_t")
		.add_property("inventory_count", &csm_switch_inventory_collection_input_t::inventory_count,&csm_switch_inventory_collection_input_t::inventory_count," The number of switch records to insert, size of @ref inventory. ")
		ARRAY_STRUCT_PROPERTY(csm_switch_inventory_collection_input_t, csmi_switch_details_t**, inventory, inventory_count, NULL, csmi_switch_details_t);

    class_<csm_switch_inventory_collection_output_t,csm_switch_inventory_collection_output_t*>("switch_inventory_collection_output_t")
		.add_property("TBD", &csm_switch_inventory_collection_output_t::TBD,&csm_switch_inventory_collection_output_t::TBD," TBD. ")
		.add_property("insert_count", &csm_switch_inventory_collection_output_t::insert_count,&csm_switch_inventory_collection_output_t::insert_count," number of new records inserted into the database. ")
		.add_property("update_count", &csm_switch_inventory_collection_output_t::update_count,&csm_switch_inventory_collection_output_t::update_count," number of old records updated in the database. ")
		.add_property("delete_count", &csm_switch_inventory_collection_output_t::delete_count,&csm_switch_inventory_collection_output_t::delete_count," number of old records removed from the database. ")
		.add_property("delete_module_count", &csm_switch_inventory_collection_output_t::delete_module_count,&csm_switch_inventory_collection_output_t::delete_module_count," number of old module records removed from the database because of removed switches. ");

    class_<csm_switch_children_inventory_collection_input_t,csm_switch_children_inventory_collection_input_t*>("switch_children_inventory_collection_input_t")
		.add_property("inventory_count", &csm_switch_children_inventory_collection_input_t::inventory_count,&csm_switch_children_inventory_collection_input_t::inventory_count," Number of elements in the 'inventory' array. ")
		ARRAY_STRUCT_PROPERTY(csm_switch_children_inventory_collection_input_t, csmi_switch_details_t**, inventory, inventory_count, NULL, csmi_switch_details_t);

    class_<csm_switch_children_inventory_collection_output_t,csm_switch_children_inventory_collection_output_t*>("switch_children_inventory_collection_output_t")
		.add_property("insert_count", &csm_switch_children_inventory_collection_output_t::insert_count,&csm_switch_children_inventory_collection_output_t::insert_count," number of new records inserted into the database. ")
		.add_property("update_count", &csm_switch_children_inventory_collection_output_t::update_count,&csm_switch_children_inventory_collection_output_t::update_count," number of old records updated in the database. ")
		.add_property("delete_count", &csm_switch_children_inventory_collection_output_t::delete_count,&csm_switch_children_inventory_collection_output_t::delete_count," number of old records removed from the database. ");

    class_<csm_cluster_query_state_input_t,csm_cluster_query_state_input_t*>("cluster_query_state_input_t")
		.add_property("limit", &csm_cluster_query_state_input_t::limit,&csm_cluster_query_state_input_t::limit," SQL 'LIMIT' numeric value. API will ignore values less than 1.")
		.add_property("num_allocs", &csm_cluster_query_state_input_t::num_allocs,&csm_cluster_query_state_input_t::num_allocs," Filter query by the 'num_allocs' field in the database.. API will ignore values less than 0.")
		.add_property("offset", &csm_cluster_query_state_input_t::offset,&csm_cluster_query_state_input_t::offset," SQL 'OFFSET' numeric value. API will ignore values less than 1.")
		.add_property("order_by", &csm_cluster_query_state_input_t::order_by,&csm_cluster_query_state_input_t::order_by," Used to alter 'ORDER BY'. API will ignore NULL values. Default to 'ORDER BY node_name ASC NULLS LAST'. VALID VALUES: [a] = 'ORDER BY node_name ASC NULLS LAST', [b] =  'ORDER BY node_name DESC NULLS LAST', [c] = 'ORDER BY state ASC NULLS LAST', [d] =  'ORDER BY state DESC NULLS LAST', [e] = 'ORDER BY type ASC NULLS LAST', [f] =  'ORDER BY type DESC NULLS LAST', [g] = 'ORDER BY num_allocs ASC NULLS LAST', [h] =  'ORDER BY num_allocs DESC NULLS LAST'. ")
		.add_property("state", &csm_cluster_query_state_input_t::state,&csm_cluster_query_state_input_t::state," Query the 'state' field in the database. API will ignore @ref csmi_node_state_t::CSM_NODE_NO_DEF values for this fields, see @ref csmi_node_state_t for details.")
		.add_property("type", &csm_cluster_query_state_input_t::type,&csm_cluster_query_state_input_t::type," Query the 'type' field in the database. API will ignore @ref csmi_node_type_t::CSM_NODE_NO_TYPE values for this fields, see @ref csmi_node_type_t for details.");

    class_<csm_cluster_query_state_output_t,csm_cluster_query_state_output_t*>("cluster_query_state_output_t")
		.add_property("results_count", &csm_cluster_query_state_output_t::results_count,&csm_cluster_query_state_output_t::results_count," Number of records retrieved, size of @ref results. ")
		ARRAY_STRUCT_PROPERTY(csm_cluster_query_state_output_t, csmi_cluster_query_state_record_t**, results, results_count, NULL, csmi_cluster_query_state_record_t);

};
