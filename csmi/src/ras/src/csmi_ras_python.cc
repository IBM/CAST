/*================================================================================

    csmi/src/ras/src/csmi_ras_python.cc

  © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#include <boost/python.hpp>

#include "csmi/src/common/include/csmi_python.h"
#include "csm_api_ras.h"
using namespace boost::python;


tuple wrap_csm_ras_event_create(
    const char *msg_id,
    const char *time_stamp,
    const char *location_name,
    const char *raw_data,
    const char *kvcsv)
{
    csm_api_object * updated_handle;

    int return_code = csm_ras_event_create( (csm_api_object**)&updated_handle, msg_id,
        time_stamp, location_name, raw_data, kvcsv );
    int64_t oid = CSMIObj::GetInstance().StoreCSMObj(updated_handle);
    
    return make_tuple(return_code, oid);
}
    
//BOOST_PYTHON_MODULE(wm_structs)
BOOST_PYTHON_MODULE(lib_csm_ras_py)
{
    def( "ras_event_create", 
        wrap_csm_ras_event_create,
        CSM_GEN_DOCSTRING("Creates a RAS event in the CSM infrastructure.\nInput:\n\t<msg_id, time_stamp, location_name, raw_data, kvcsv>",""));

    // STRUCTS_BEGIN
    enum_<RasSeverity>("RasSeverity")
		.value("CSM_RAS_SEV_INFO",CSM_RAS_SEV_INFO)
		.value("CSM_RAS_SEV_WARN",CSM_RAS_SEV_WARN)
		.value("CSM_RAS_SEV_ERROR",CSM_RAS_SEV_ERROR)
		.value("CSM_RAS_SEV_FATAL",CSM_RAS_SEV_FATAL);

    class_<csmi_ras_type_record_t,csmi_ras_type_record_t*>("ras_type_record_t")
		.add_property("threshold_count", &csmi_ras_type_record_t::threshold_count,&csmi_ras_type_record_t::threshold_count," Number of times this event has to occur during the 'threshold_period' before taking action on the RAS event. ")
		.add_property("threshold_period", &csmi_ras_type_record_t::threshold_period,&csmi_ras_type_record_t::threshold_period," Period in seconds over which to count the 'threshold_count'. ")
		.add_property("enabled", &csmi_ras_type_record_t::enabled,&csmi_ras_type_record_t::enabled," (bool) Events will be processed if enabled=true and suppressed if enabled=false. ")
		.add_property("visible_to_users", &csmi_ras_type_record_t::visible_to_users,&csmi_ras_type_record_t::visible_to_users," (bool) When visible_to_users=true, RAS events of this type will be returned in the response to csm_ras_event_query_allocation. ")
		.add_property("set_not_ready", &csmi_ras_type_record_t::set_not_ready,&csmi_ras_type_record_t::set_not_ready," Deprecated after CSM_VERSION_0_4_1. (bool) When set_not_ready=true, resources associated with the event will be set to ready=n when the event hits threshold. ")
		.add_property("set_ready", &csmi_ras_type_record_t::set_ready,&csmi_ras_type_record_t::set_ready," Deprecated after CSM_VERSION_0_4_1. (bool) When set_ready=true, resources associated with the event will be set to ready=y when the event hits threshold. ")
		STRING_PROPERTY(csmi_ras_type_record_t, char*, msg_id, , NULL, )
		STRING_PROPERTY(csmi_ras_type_record_t, char*, control_action, , NULL, )
		STRING_PROPERTY(csmi_ras_type_record_t, char*, description, , NULL, )
		STRING_PROPERTY(csmi_ras_type_record_t, char*, message, , NULL, )
		.add_property("severity", &csmi_ras_type_record_t::severity,&csmi_ras_type_record_t::severity," Severity of the RAS event. INFO/WARNING/FATAL. ")
		.add_property("set_state", &csmi_ras_type_record_t::set_state,&csmi_ras_type_record_t::set_state," replacement for 'set_ready' and 'set_not_ready' after CSM_VERSION_0_4_1. resources associated with the event will be set to this node state when the event hits threshold. ");

    class_<csm_ras_event_create_input_t,csm_ras_event_create_input_t*>("ras_event_create_input_t")
		STRING_PROPERTY(csm_ras_event_create_input_t, char*, msg_id, , NULL, )
		STRING_PROPERTY(csm_ras_event_create_input_t, char*, time_stamp, , NULL, )
		STRING_PROPERTY(csm_ras_event_create_input_t, char*, location_name, , NULL, )
		STRING_PROPERTY(csm_ras_event_create_input_t, char*, raw_data, , NULL, )
		STRING_PROPERTY(csm_ras_event_create_input_t, char*, kvcsv, , NULL, );

    class_<csmi_ras_event_action_record_t,csmi_ras_event_action_record_t*>("ras_event_action_record_t")
		.add_property("rec_id", &csmi_ras_event_action_record_t::rec_id,&csmi_ras_event_action_record_t::rec_id," The record id of the RAS evnet. ")
		.add_property("count", &csmi_ras_event_action_record_t::count,&csmi_ras_event_action_record_t::count," The number of times the event occured. ")
		.add_property("msg_id_seq", &csmi_ras_event_action_record_t::msg_id_seq,&csmi_ras_event_action_record_t::msg_id_seq," The sequence number indexing the csm_ras_type_audit table.")
		STRING_PROPERTY(csmi_ras_event_action_record_t, char*, archive_history_time, , NULL, )
		STRING_PROPERTY(csmi_ras_event_action_record_t, char*, location_name, , NULL, )
		STRING_PROPERTY(csmi_ras_event_action_record_t, char*, message, , NULL, )
		STRING_PROPERTY(csmi_ras_event_action_record_t, char*, msg_id, , NULL, )
		STRING_PROPERTY(csmi_ras_event_action_record_t, char*, raw_data, , NULL, )
		STRING_PROPERTY(csmi_ras_event_action_record_t, char*, time_stamp, , NULL, );

    class_<csmi_ras_event_action_t,csmi_ras_event_action_t*>("ras_event_action_t")
		.add_property("rec_id", &csmi_ras_event_action_t::rec_id,&csmi_ras_event_action_t::rec_id," The record id of the RAS event. ")
		.add_property("msg_id_seq", &csmi_ras_event_action_t::msg_id_seq,&csmi_ras_event_action_t::msg_id_seq," The sequence number indexing the csm_ras_type_audit table.")
		.add_property("count", &csmi_ras_event_action_t::count,&csmi_ras_event_action_t::count," The number of times the event occurred. ")
		STRING_PROPERTY(csmi_ras_event_action_t, char*, msg_id, , NULL, )
		STRING_PROPERTY(csmi_ras_event_action_t, char*, time_stamp, , NULL, )
		STRING_PROPERTY(csmi_ras_event_action_t, char*, location_name, , NULL, )
		STRING_PROPERTY(csmi_ras_event_action_t, char*, message, , NULL, )
		STRING_PROPERTY(csmi_ras_event_action_t, char*, raw_data, , NULL, )
		STRING_PROPERTY(csmi_ras_event_action_t, char*, archive_history_time, , NULL, );

    class_<csmi_ras_event_t,csmi_ras_event_t*>("ras_event_t")
		.add_property("min_time_in_pool", &csmi_ras_event_t::min_time_in_pool,&csmi_ras_event_t::min_time_in_pool," The minimum time the event is available in the RAS event pool. ")
		.add_property("processor", &csmi_ras_event_t::processor,&csmi_ras_event_t::processor," @todo Post-PRPQ: What is the processor? ")
		.add_property("count", &csmi_ras_event_t::count,&csmi_ras_event_t::count," @todo Post-PRPQ: What is the count of?")
		STRING_PROPERTY(csmi_ras_event_t, char*, msg_id, , NULL, )
		STRING_PROPERTY(csmi_ras_event_t, char*, suppress_ids, , NULL, )
		STRING_PROPERTY(csmi_ras_event_t, char*, severity, , NULL, )
		STRING_PROPERTY(csmi_ras_event_t, char*, time_stamp, , NULL, )
		STRING_PROPERTY(csmi_ras_event_t, char*, location_name, , NULL, )
		STRING_PROPERTY(csmi_ras_event_t, char*, control_action, , NULL, )
		STRING_PROPERTY(csmi_ras_event_t, char*, message, , NULL, )
		STRING_PROPERTY(csmi_ras_event_t, char*, raw_data, , NULL, )
		STRING_PROPERTY(csmi_ras_event_t, char*, kvcsv, , NULL, )
		STRING_PROPERTY(csmi_ras_event_t, char*, master_time_stamp, , NULL, )
		.add_property("rec_id", &csmi_ras_event_t::rec_id,&csmi_ras_event_t::rec_id," unique identifier for this specific ras event. ");

    class_<csmi_ras_event_vector_t,csmi_ras_event_vector_t*>("ras_event_vector_t")
		.add_property("num_ras_events", &csmi_ras_event_vector_t::num_ras_events,&csmi_ras_event_vector_t::num_ras_events," The number of RAS events, size of @ref events. ")
		ARRAY_STRUCT_PROPERTY(csmi_ras_event_vector_t, csmi_ras_event_t**, events, num_ras_events, NULL, csmi_ras_event_t);

    class_<csm_ras_event_query_input_t,csm_ras_event_query_input_t*>("ras_event_query_input_t")
		.add_property("limit", &csm_ras_event_query_input_t::limit,&csm_ras_event_query_input_t::limit," SQL 'LIMIT' numeric value. API will ignore values less than 1.")
		.add_property("offset", &csm_ras_event_query_input_t::offset,&csm_ras_event_query_input_t::offset," SQL 'OFFSET' numeric value. API will ignore values less than 1. ")
		.add_property("severity", &csm_ras_event_query_input_t::severity,&csm_ras_event_query_input_t::severity," Severity of the RAS event. INFO/WARNING/FATAL, optional. ")
		.add_property("order_by", &csm_ras_event_query_input_t::order_by,&csm_ras_event_query_input_t::order_by," Used to alter 'ORDER BY'. API will ignore NULL values. Default to 'ORDER BY rec_id ASC NULLS LAST'. VALID VALUES: [a] = 'ORDER BY rec_id ASC NULLS LAST', [b] =  'ORDER BY rec_id DESC NULLS LAST', [c] = 'ORDER BY time_stamp ASC NULLS LAST', [d] =  'ORDER BY time_stamp DESC NULLS LAST', [e] = 'ORDER BY master_time_stamp ASC NULLS LAST', [f] =  'ORDER BY master_time_stamp DESC NULLS LAST', [g] = 'ORDER BY location_name ASC NULLS LAST', [h] =  'ORDER BY location_name DESC NULLS LAST', [i] = 'ORDER BY msg_id ASC NULLS LAST', [j] =  'ORDER BY msg_id DESC NULLS LAST', [k] = 'ORDER BY severity ASC NULLS LAST', [l] =  'ORDER BY severity DESC NULLS LAST'. ")
		STRING_PROPERTY(csm_ras_event_query_input_t, char*, msg_id, , NULL, )
		STRING_PROPERTY(csm_ras_event_query_input_t, char*, start_time_stamp, , NULL, )
		STRING_PROPERTY(csm_ras_event_query_input_t, char*, end_time_stamp, , NULL, )
		STRING_PROPERTY(csm_ras_event_query_input_t, char*, location_name, , NULL, )
		STRING_PROPERTY(csm_ras_event_query_input_t, char*, control_action, , NULL, )
		STRING_PROPERTY(csm_ras_event_query_input_t, char*, message, , NULL, )
		STRING_PROPERTY(csm_ras_event_query_input_t, char*, master_time_stamp_search_begin, , NULL, )
		STRING_PROPERTY(csm_ras_event_query_input_t, char*, master_time_stamp_search_end, , NULL, )
		.add_property("rec_id", &csm_ras_event_query_input_t::rec_id,&csm_ras_event_query_input_t::rec_id," Query by rec_id. This is a unique identifier for this specific ras event. API will ignore values less than 1. optional. ");

    class_<csm_ras_event_query_output_t,csm_ras_event_query_output_t*>("ras_event_query_output_t")
		.add_property("results_count", &csm_ras_event_query_output_t::results_count,&csm_ras_event_query_output_t::results_count," Number of elements in the 'results' array. ")
		ARRAY_STRUCT_PROPERTY(csm_ras_event_query_output_t, csmi_ras_event_t**, results, results_count, NULL, csmi_ras_event_t);

    class_<csm_ras_event_query_allocation_input_t,csm_ras_event_query_allocation_input_t*>("ras_event_query_allocation_input_t")
		.add_property("allocation_id", &csm_ras_event_query_allocation_input_t::allocation_id,&csm_ras_event_query_allocation_input_t::allocation_id," The allocation id to search for associated RAS events. ")
		.add_property("limit", &csm_ras_event_query_allocation_input_t::limit,&csm_ras_event_query_allocation_input_t::limit," SQL 'LIMIT' numeric value. API will ignore values less than 1.")
		.add_property("offset", &csm_ras_event_query_allocation_input_t::offset,&csm_ras_event_query_allocation_input_t::offset," SQL 'OFFSET' numeric value. API will ignore values less than 1.");

    class_<csm_ras_event_query_allocation_output_t,csm_ras_event_query_allocation_output_t*>("ras_event_query_allocation_output_t")
		.add_property("num_events", &csm_ras_event_query_allocation_output_t::num_events,&csm_ras_event_query_allocation_output_t::num_events," Number of events retrieved by the query, size of @ref events. ")
		ARRAY_STRUCT_PROPERTY(csm_ras_event_query_allocation_output_t, csmi_ras_event_action_t**, events, num_events, NULL, csmi_ras_event_action_t);

    class_<csm_ras_msg_type_create_input_t,csm_ras_msg_type_create_input_t*>("ras_msg_type_create_input_t")
		.add_property("threshold_count", &csm_ras_msg_type_create_input_t::threshold_count,&csm_ras_msg_type_create_input_t::threshold_count," Number of times this event has to occur during the 'threshold_period' before taking action on the RAS event. ")
		.add_property("threshold_period", &csm_ras_msg_type_create_input_t::threshold_period,&csm_ras_msg_type_create_input_t::threshold_period," Period in seconds over which to count the 'threshold_count'. ")
		.add_property("visible_to_users", &csm_ras_msg_type_create_input_t::visible_to_users,&csm_ras_msg_type_create_input_t::visible_to_users," (bool) When visible_to_users=true, RAS events of this type will be returned in the response to csm_ras_event_query_allocation. ")
		.add_property("enabled", &csm_ras_msg_type_create_input_t::enabled,&csm_ras_msg_type_create_input_t::enabled," Events will be processed if enabled=true and suppressed if enabled=false ")
		.add_property("set_ready", &csm_ras_msg_type_create_input_t::set_ready,&csm_ras_msg_type_create_input_t::set_ready," Deprecated after CSM_VERSION_0_4_1. (bool) When set_ready=true, resources associated with the event will be set to ready=y when the event hits threshold. ")
		.add_property("set_not_ready", &csm_ras_msg_type_create_input_t::set_not_ready,&csm_ras_msg_type_create_input_t::set_not_ready," Deprecated after CSM_VERSION_0_4_1. (bool) When set_not_ready=true, resources associated with the event will be set to ready=n when the event hits threshold. ")
		STRING_PROPERTY(csm_ras_msg_type_create_input_t, char*, control_action, , NULL, )
		STRING_PROPERTY(csm_ras_msg_type_create_input_t, char*, description, , NULL, )
		STRING_PROPERTY(csm_ras_msg_type_create_input_t, char*, msg_id, , NULL, )
		STRING_PROPERTY(csm_ras_msg_type_create_input_t, char*, message, , NULL, )
		.add_property("severity", &csm_ras_msg_type_create_input_t::severity,&csm_ras_msg_type_create_input_t::severity," Severity of the RAS event. INFO/WARNING/FATAL. default to INFO ")
		.add_property("set_state", &csm_ras_msg_type_create_input_t::set_state,&csm_ras_msg_type_create_input_t::set_state," replacement for 'set_ready' and 'set_not_ready' after CSM_VERSION_0_4_1. resources associated with the event will be set to this node state when the event hits threshold. ");

    class_<csm_ras_msg_type_create_output_t,csm_ras_msg_type_create_output_t*>("ras_msg_type_create_output_t")
		.add_property("insert_successful", &csm_ras_msg_type_create_output_t::insert_successful,&csm_ras_msg_type_create_output_t::insert_successful," The RAS creation was successful.")
		STRING_PROPERTY(csm_ras_msg_type_create_output_t, char*, msg_id, , NULL, );

    class_<csm_ras_msg_type_delete_input_t,csm_ras_msg_type_delete_input_t*>("ras_msg_type_delete_input_t")
		.add_property("msg_ids_count", &csm_ras_msg_type_delete_input_t::msg_ids_count,&csm_ras_msg_type_delete_input_t::msg_ids_count," Number of messages to delete, size of @ref msg_ids. ")
		ARRAY_STR_PROPERTY(csm_ras_msg_type_delete_input_t, char**, msg_ids, msg_ids_count, NULL, );

    class_<csm_ras_msg_type_delete_output_t,csm_ras_msg_type_delete_output_t*>("ras_msg_type_delete_output_t")
		.add_property("expected_number_of_deleted_msg_ids", &csm_ras_msg_type_delete_output_t::expected_number_of_deleted_msg_ids,&csm_ras_msg_type_delete_output_t::expected_number_of_deleted_msg_ids," Number of msg ids that were attempted to be deleted. ")
		.add_property("not_deleted_msg_ids_count", &csm_ras_msg_type_delete_output_t::not_deleted_msg_ids_count,&csm_ras_msg_type_delete_output_t::not_deleted_msg_ids_count," Number of messages that failed to be deleted, size of @ref not_deleted_msg_ids. ")
		.add_property("deleted_msg_ids_count", &csm_ras_msg_type_delete_output_t::deleted_msg_ids_count,&csm_ras_msg_type_delete_output_t::deleted_msg_ids_count," Number of messages that were successfully deleted, size of @ref deleted_msg_ids.")
		ARRAY_STR_PROPERTY(csm_ras_msg_type_delete_output_t, char**, not_deleted_msg_ids, not_deleted_msg_ids_count, NULL, )
		ARRAY_STR_PROPERTY(csm_ras_msg_type_delete_output_t, char**, deleted_msg_ids, deleted_msg_ids_count, NULL, );

    class_<csm_ras_msg_type_update_input_t,csm_ras_msg_type_update_input_t*>("ras_msg_type_update_input_t")
		.add_property("threshold_count", &csm_ras_msg_type_update_input_t::threshold_count,&csm_ras_msg_type_update_input_t::threshold_count," Number of times this event has to occur during the 'threshold_period' before taking action on the RAS event. API will ignore NULL values.")
		.add_property("threshold_period", &csm_ras_msg_type_update_input_t::threshold_period,&csm_ras_msg_type_update_input_t::threshold_period," Period in seconds over which to count the 'threshold_count'. API will ignore NULL values.")
		.add_property("visible_to_users", &csm_ras_msg_type_update_input_t::visible_to_users,&csm_ras_msg_type_update_input_t::visible_to_users," (bool) When visible_to_users=true, RAS events of this type will be returned in the response to csm_ras_event_query_allocation. API will ignore @ref CSM_UNDEF_BOOL.")
		.add_property("set_not_ready", &csm_ras_msg_type_update_input_t::set_not_ready,&csm_ras_msg_type_update_input_t::set_not_ready," Deprecated after CSM_VERSION_0_4_1. (bool) When set_not_ready=true, resources associated with the event will be set to ready=n when the event hits threshold. API will ignore @ref CSM_UNDEF_BOOL.")
		.add_property("set_ready", &csm_ras_msg_type_update_input_t::set_ready,&csm_ras_msg_type_update_input_t::set_ready," Deprecated after CSM_VERSION_0_4_1. (bool)  When set_ready=true, resources associated with the event will be set to ready=y when the event hits threshold. API will ignore @ref CSM_UNDEF_BOOL.")
		.add_property("enabled", &csm_ras_msg_type_update_input_t::enabled,&csm_ras_msg_type_update_input_t::enabled," (bool)  Events will be processed if enabled=true and suppressed if enabled=false. API will ignore @ref CSM_UNDEF_BOOL.")
		STRING_PROPERTY(csm_ras_msg_type_update_input_t, char*, control_action, , NULL, )
		STRING_PROPERTY(csm_ras_msg_type_update_input_t, char*, description, , NULL, )
		STRING_PROPERTY(csm_ras_msg_type_update_input_t, char*, msg_id, , NULL, )
		STRING_PROPERTY(csm_ras_msg_type_update_input_t, char*, message, , NULL, )
		.add_property("severity", &csm_ras_msg_type_update_input_t::severity,&csm_ras_msg_type_update_input_t::severity," Severity of the RAS event. INFO/WARNING/FATAL. ")
		.add_property("set_state", &csm_ras_msg_type_update_input_t::set_state,&csm_ras_msg_type_update_input_t::set_state," replacement for 'set_ready' and 'set_not_ready' after CSM_VERSION_0_4_1. resources associated with the event will be set to this node state when the event hits threshold. ");

    class_<csm_ras_msg_type_update_output_t,csm_ras_msg_type_update_output_t*>("ras_msg_type_update_output_t")
		.add_property("update_successful", &csm_ras_msg_type_update_output_t::update_successful,&csm_ras_msg_type_update_output_t::update_successful," The RAS update was successful. ")
		STRING_PROPERTY(csm_ras_msg_type_update_output_t, char*, msg_id, , NULL, );

    class_<csm_ras_msg_type_query_input_t,csm_ras_msg_type_query_input_t*>("ras_msg_type_query_input_t")
		.add_property("limit", &csm_ras_msg_type_query_input_t::limit,&csm_ras_msg_type_query_input_t::limit," SQL 'LIMIT' numeric value. API will ignore values less than 1.")
		.add_property("offset", &csm_ras_msg_type_query_input_t::offset,&csm_ras_msg_type_query_input_t::offset," SQL 'OFFSET' numeric value. API will ignore values less than 1.")
		.add_property("severity", &csm_ras_msg_type_query_input_t::severity,&csm_ras_msg_type_query_input_t::severity," Severity of the RAS event. INFO/WARNING/FATAL. optional. ")
		STRING_PROPERTY(csm_ras_msg_type_query_input_t, char*, control_action, , NULL, )
		STRING_PROPERTY(csm_ras_msg_type_query_input_t, char*, msg_id, , NULL, )
		STRING_PROPERTY(csm_ras_msg_type_query_input_t, char*, message, , NULL, );

    class_<csm_ras_msg_type_query_output_t,csm_ras_msg_type_query_output_t*>("ras_msg_type_query_output_t")
		.add_property("results_count", &csm_ras_msg_type_query_output_t::results_count,&csm_ras_msg_type_query_output_t::results_count," The number of records retrieved by the query, size of @ref results. ")
		ARRAY_STRUCT_PROPERTY(csm_ras_msg_type_query_output_t, csmi_ras_type_record_t**, results, results_count, NULL, csmi_ras_type_record_t);

    class_<csm_ras_subscribe_input_t,csm_ras_subscribe_input_t*>("ras_subscribe_input_t")
		STRING_PROPERTY(csm_ras_subscribe_input_t, char*, topic, , NULL, );

    class_<csm_ras_unsubscribe_input_t,csm_ras_unsubscribe_input_t*>("ras_unsubscribe_input_t")
		STRING_PROPERTY(csm_ras_unsubscribe_input_t, char*, topic, , NULL, );

};
