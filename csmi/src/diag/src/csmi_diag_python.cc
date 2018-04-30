/*================================================================================

    csmi/src/diag/src/csmi_diag_python.cc

  © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#include <boost/python.hpp>

#include "csmi/src/common/include/csmi_python.h"
#include "csm_api_diagnostics.h"
using namespace boost::python;

//BOOST_PYTHON_MODULE(wm_structs)
BOOST_PYTHON_MODULE(lib_csm_diag_py)
{

    // STRUCTS_BEGIN
    enum_<csmi_diag_run_status_t>("csmi_diag_run_status_t")
		.value("DIAG_CANCELED",DIAG_CANCELED)
		.value("DIAG_COMPLETED",DIAG_COMPLETED)
		.value("DIAG_RUNNING",DIAG_RUNNING)
		.value("DIAG_FAILED",DIAG_FAILED)
		.value("DIAG_COMPLETED_FAIL",DIAG_COMPLETED_FAIL);

    class_<csmi_diag_run_t,csmi_diag_run_t*>("diag_run_t")
		.add_property("run_id", &csmi_diag_run_t::run_id,&csmi_diag_run_t::run_id," Diagnostic run id. ")
		.add_property("allocation_id", &csmi_diag_run_t::allocation_id,&csmi_diag_run_t::allocation_id," Unique identifier of the  allocation that this diagnostic is being run under. ")
		STRING_PROPERTY(csmi_diag_run_t, char*, cmd_line, , NULL, )
		.add_property("inserted_ras", &csmi_diag_run_t::inserted_ras,&csmi_diag_run_t::inserted_ras," Inserted diagnostic ras events.  ")
		STRING_PROPERTY(csmi_diag_run_t, char*, begin_time, , NULL, )
		STRING_PROPERTY(csmi_diag_run_t, char*, end_time, , NULL, )
		STRING_PROPERTY(csmi_diag_run_t, char*, history_time, , NULL, )
		STRING_PROPERTY(csmi_diag_run_t, char*, log_dir, , NULL, );

    class_<csmi_diag_run_query_details_result_t,csmi_diag_run_query_details_result_t*>("diag_run_query_details_result_t")
		.add_property("run_id", &csmi_diag_run_query_details_result_t::run_id,&csmi_diag_run_query_details_result_t::run_id," Diagnostic run id. ")
		STRING_PROPERTY(csmi_diag_run_query_details_result_t, char*, history_time, , NULL, )
		STRING_PROPERTY(csmi_diag_run_query_details_result_t, char*, test_name, , NULL, )
		STRING_PROPERTY(csmi_diag_run_query_details_result_t, char*, node_name, , NULL, )
		STRING_PROPERTY(csmi_diag_run_query_details_result_t, char*, serial_number, , NULL, )
		STRING_PROPERTY(csmi_diag_run_query_details_result_t, char*, begin_time, , NULL, )
		STRING_PROPERTY(csmi_diag_run_query_details_result_t, char*, end_time, , NULL, )
		STRING_PROPERTY(csmi_diag_run_query_details_result_t, char*, log_file, , NULL, );

    class_<csm_diag_run_end_input_t,csm_diag_run_end_input_t*>("diag_run_end_input_t")
		.add_property("run_id", &csm_diag_run_end_input_t::run_id,&csm_diag_run_end_input_t::run_id," Diagnostic run id. ")
		.add_property("inserted_ras", &csm_diag_run_end_input_t::inserted_ras,&csm_diag_run_end_input_t::inserted_ras," Inserted diagnostic ras events.  ");

    class_<csm_diag_result_create_input_t,csm_diag_result_create_input_t*>("diag_result_create_input_t")
		.add_property("run_id", &csm_diag_result_create_input_t::run_id,&csm_diag_result_create_input_t::run_id," Diagnostic run id. ")
		STRING_PROPERTY(csm_diag_result_create_input_t, char*, test_name, , NULL, )
		STRING_PROPERTY(csm_diag_result_create_input_t, char*, node_name, , NULL, )
		STRING_PROPERTY(csm_diag_result_create_input_t, char*, serial_number, , NULL, )
		STRING_PROPERTY(csm_diag_result_create_input_t, char*, begin_time, , NULL, )
		STRING_PROPERTY(csm_diag_result_create_input_t, char*, log_file, , NULL, );

    class_<csm_diag_run_begin_input_t,csm_diag_run_begin_input_t*>("diag_run_begin_input_t")
		.add_property("run_id", &csm_diag_run_begin_input_t::run_id,&csm_diag_run_begin_input_t::run_id," Diagnostic run id. ")
		.add_property("allocation_id", &csm_diag_run_begin_input_t::allocation_id,&csm_diag_run_begin_input_t::allocation_id," Unique identifier of the  allocation that this diagnostic is being run under. ")
		STRING_PROPERTY(csm_diag_run_begin_input_t, char*, cmd_line, , NULL, )
		STRING_PROPERTY(csm_diag_run_begin_input_t, char*, log_dir, , NULL, );

    class_<csm_diag_run_query_input_t,csm_diag_run_query_input_t*>("diag_run_query_input_t")
		.add_property("status", &csm_diag_run_query_input_t::status,&csm_diag_run_query_input_t::status," Filter results to only include records that have a matching diagnostic status (NONE=0,CANCELED=1,COMPLETED=2,FAILED=4,RUNNING=8,ALL=15) ")
		.add_property("allocation_ids_count", &csm_diag_run_query_input_t::allocation_ids_count,&csm_diag_run_query_input_t::allocation_ids_count," Number of allocations to perform query on, size of @ref allocation_ids.")
		.add_property("run_ids_count", &csm_diag_run_query_input_t::run_ids_count,&csm_diag_run_query_input_t::run_ids_count," Number of run ids to perform query on, size of @ref run_ids.")
		.add_property("limit", &csm_diag_run_query_input_t::limit,&csm_diag_run_query_input_t::limit," SQL 'LIMIT' numeric value. API will ignore values less than 1.")
		.add_property("offset", &csm_diag_run_query_input_t::offset,&csm_diag_run_query_input_t::offset," SQL 'OFFSET' numeric value. API will ignore values less than 1.")
		.add_property("inserted_ras", &csm_diag_run_query_input_t::inserted_ras,&csm_diag_run_query_input_t::inserted_ras,"Filter results to only include records that inserted RAS events or only include records that did not insert RAS events. API doesn't query on @ref CSM_UNDEF_BOOL. ")
		STRING_PROPERTY(csm_diag_run_query_input_t, char*, begin_time_search_begin, , NULL, )
		STRING_PROPERTY(csm_diag_run_query_input_t, char*, begin_time_search_end, , NULL, )
		STRING_PROPERTY(csm_diag_run_query_input_t, char*, end_time_search_begin, , NULL, )
		STRING_PROPERTY(csm_diag_run_query_input_t, char*, end_time_search_end, , NULL, )
		ARRAY_PROPERTY(csm_diag_run_query_input_t, int64_t*, allocation_ids, allocation_ids_count, NULL, int64_t)
		ARRAY_PROPERTY(csm_diag_run_query_input_t, int64_t*, run_ids, run_ids_count, NULL, int64_t);

    class_<csm_diag_run_query_output_t,csm_diag_run_query_output_t*>("diag_run_query_output_t")
		.add_property("num_runs", &csm_diag_run_query_output_t::num_runs,&csm_diag_run_query_output_t::num_runs," Number of diagnostic runs retrieved, size of @ref runs. ")
		ARRAY_STRUCT_PROPERTY(csm_diag_run_query_output_t, csmi_diag_run_t**, runs, num_runs, NULL, csmi_diag_run_t);

    class_<csm_diag_run_query_details_input_t,csm_diag_run_query_details_input_t*>("diag_run_query_details_input_t")
		.add_property("run_id", &csm_diag_run_query_details_input_t::run_id,&csm_diag_run_query_details_input_t::run_id," A Diagnostic run id to search the database for. ");

    class_<csm_diag_run_query_details_output_t,csm_diag_run_query_details_output_t*>("diag_run_query_details_output_t")
		.add_property("num_details", &csm_diag_run_query_details_output_t::num_details,&csm_diag_run_query_details_output_t::num_details," The number of details objects retrieved, size of @ref details.")
		STRUCT_PROPERTY(csm_diag_run_query_details_output_t, csmi_diag_run_t*, run_data, , NULL, &csm_diag_run_query_details_output_t::run_data)
		ARRAY_STRUCT_PROPERTY(csm_diag_run_query_details_output_t, csmi_diag_run_query_details_result_t**, details, num_details, NULL, csmi_diag_run_query_details_result_t);

};
