/*================================================================================

    csmi/src/common/src/csmi_python.cc

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#include <boost/python/class.hpp>
#include <boost/python/enum.hpp>
//#include <boost/python.hpp>
#include <boost/python/module.hpp>
#include <boost/python/def.hpp>

#include "csmi/src/common/include/csmi_python.h"
#include "csmi_type_common.h"

void CSMIObj::DestroyCSMObj(int64_t oid)
{
    std::lock_guard<std::mutex> lockObjects(_ObjectsMutex);

    // Find the object, destroy if still present.
    auto obj = _Objects.find(oid);
    if ( obj != _Objects.end() )
    {
        csm_api_object_destroy((csm_api_object*) obj->second);
        _Objects.erase(obj);
    }
}

int64_t CSMIObj::StoreCSMObj(csm_api_object* obj, int64_t oid)
{
    // First test the object id, if not set generate one.
    int64_t finalOid = oid;
    if ( finalOid < 0 )
    {
        std::unique_lock<std::mutex> lockCounter(_ObjCounterMutex);
        finalOid = _ObjCounter++;
        lockCounter.unlock();
        }

    // Check to see if the OID was already assigned, if it has clear the old objects then store.
    std::lock_guard<std::mutex> lockObjects(_ObjectsMutex);
    auto oldObj = _Objects.find(finalOid);
    if ( oldObj != _Objects.end() )
    {
        csm_api_object_destroy((csm_api_object*) oldObj->second);
    }
    _Objects[finalOid] = obj;

    return finalOid;
}

CSMIObj::~CSMIObj()
{
    std::lock_guard<std::mutex> lockObjects(_ObjectsMutex);
    auto iterator = _Objects.begin();

    while ( iterator != _Objects.end())
    {
        csm_api_object_destroy((csm_api_object*) iterator->second);
        iterator++;
    }
}

int wrap_csm_init_lib()
{
    return csm_init_lib();
}

void wrap_csm_api_object_destroy(int64_t oid)
{
    CSMIObj::GetInstance().DestroyCSMObj(oid);    
}

BOOST_PYTHON_MODULE(lib_csm_py)
{
    using namespace boost::python;
    class_<csm_api_object>("api_object");

    // API object helpers
    //def("api_object_errcode_get",csm_api_object_errcode_get);
    //def("csm_api_object_errmsg_get",csm_api_object_errmsg_get);
    //def("api_object_traceid_get",csm_api_object_traceid_get);
    def("api_object_destroy", wrap_csm_api_object_destroy);
    
    // Init and terminate.
    def("init_lib",wrap_csm_init_lib);
    def("term_lib",csm_term_lib);
    // STRUCTS_BEGIN
    enum_<csmi_cmd_err_t>("csmi_cmd_err_t")
		.value("CSMI_SUCCESS",CSMI_SUCCESS)
		.value("CSMERR_GENERIC",CSMERR_GENERIC)
		.value("CSMI_HELP",CSMI_HELP)
		.value("CSMI_NOT_INITIALIZED",CSMI_NOT_INITIALIZED)
		.value("CSMI_NO_RESULTS",CSMI_NO_RESULTS)
		.value("CSMERR_TIMEOUT",CSMERR_TIMEOUT)
		.value("CSMERR_MSGID_MISMATCH",CSMERR_MSGID_MISMATCH)
		.value("CSMERR_CMD_MISMATCH",CSMERR_CMD_MISMATCH)
		.value("CSMERR_MISSING_PARAM",CSMERR_MISSING_PARAM)
		.value("CSMERR_INVALID_PARAM",CSMERR_INVALID_PARAM)
		.value("CSMERR_RAS_HANDLER_ERROR",CSMERR_RAS_HANDLER_ERROR)
		.value("CSMERR_CMD_UNKNOWN",CSMERR_CMD_UNKNOWN)
		.value("CSMERR_SENDRCV_ERROR",CSMERR_SENDRCV_ERROR)
		.value("CSMERR_MEM_ERROR",CSMERR_MEM_ERROR)
		.value("CSMERR_NOTDEF",CSMERR_NOTDEF)
		.value("CSMERR_PUBSUB",CSMERR_PUBSUB)
		.value("CSMERR_PERM",CSMERR_PERM)
		.value("CSMERR_SCRIPT_FAILURE",CSMERR_SCRIPT_FAILURE)
		.value("CSMERR_INIT_LIB_FAILED",CSMERR_INIT_LIB_FAILED)
		.value("CSMERR_TERM_LIB_FAILED",CSMERR_TERM_LIB_FAILED)
		.value("CSMERR_PAYLOAD_EMPTY",CSMERR_PAYLOAD_EMPTY)
		.value("CSMERR_BAD_EVENT_TYPE",CSMERR_BAD_EVENT_TYPE)
		.value("CSMERR_BAD_ADDR_TYPE",CSMERR_BAD_ADDR_TYPE)
		.value("CSMERR_CGROUP_DEL_ERROR",CSMERR_CGROUP_DEL_ERROR)
		.value("CSMERR_DAEMON_DB_ERR",CSMERR_DAEMON_DB_ERR)
		.value("CSMERR_DB_ERROR",CSMERR_DB_ERROR)
		.value("CSMERR_DEL_MISMATCH",CSMERR_DEL_MISMATCH)
		.value("CSMERR_UPDATE_MISMATCH",CSMERR_UPDATE_MISMATCH)
		.value("CSMERR_MSG_PACK_ERROR",CSMERR_MSG_PACK_ERROR)
		.value("CSMERR_MSG_UNPACK_ERROR",CSMERR_MSG_UNPACK_ERROR)
		.value("CSMERR_MSG_BUFFER_EMPTY",CSMERR_MSG_BUFFER_EMPTY)
		.value("CSMERR_MSG_RETURNBUFFER_EMPTY",CSMERR_MSG_RETURNBUFFER_EMPTY)
		.value("CSMERR_MSG_RETURNBUFFER_UNKNOWN_CORRUPTED",CSMERR_MSG_RETURNBUFFER_UNKNOWN_CORRUPTED)
		.value("CSMERR_MULTI_GEN_ERROR",CSMERR_MULTI_GEN_ERROR)
		.value("CSMERR_MULTI_RESP_ERROR",CSMERR_MULTI_RESP_ERROR)
		.value("CSMERR_EXEC_ILLEGAL",CSMERR_EXEC_ILLEGAL)
		.value("CSMERR_CGROUP_FAIL",CSMERR_CGROUP_FAIL)
		.value("CSMERR_CONTEXT_LOST",CSMERR_CONTEXT_LOST)
		.value("CSMERR_CGROUP_EINVAL",CSMERR_CGROUP_EINVAL)
		.value("CSMERR_CGROUP_EACCES",CSMERR_CGROUP_EACCES)
		.value("CSMERR_BB_CMD_ERROR",CSMERR_BB_CMD_ERROR)
		.value("CSM_STATE_JUMPED",CSM_STATE_JUMPED)
		.value("CSM_SAME_STATE_TRANSITION",CSM_SAME_STATE_TRANSITION)
		.value("CSMERR_STATE_CHANGE_FAILED",CSMERR_STATE_CHANGE_FAILED)
		.value("CSMERR_DELETE_STATE_BAD",CSMERR_DELETE_STATE_BAD)
		.value("CSMERR_JSRUN_CMD_ERROR",CSMERR_JSRUN_CMD_ERROR);

    enum_<csmi_node_type_t>("csmi_node_type_t")
		.value("CSM_NODE_NO_TYPE",CSM_NODE_NO_TYPE)
		.value("CSM_NODE_MANAGEMENT",CSM_NODE_MANAGEMENT)
		.value("CSM_NODE_SERVICE",CSM_NODE_SERVICE)
		.value("CSM_NODE_LOGIN",CSM_NODE_LOGIN)
		.value("CSM_NODE_WORKLOAD_MANAGER",CSM_NODE_WORKLOAD_MANAGER)
		.value("CSM_NODE_LAUNCH",CSM_NODE_LAUNCH)
		.value("CSM_NODE_COMPUTE",CSM_NODE_COMPUTE)
		.value("CSM_NODE_UTILITY",CSM_NODE_UTILITY)
		.value("CSM_NODE_AGGREGATOR",CSM_NODE_AGGREGATOR);

    enum_<csmi_node_state_t>("csmi_node_state_t")
		.value("CSM_NODE_NO_DEF",CSM_NODE_NO_DEF)
		.value("CSM_NODE_DISCOVERED",CSM_NODE_DISCOVERED)
		.value("CSM_NODE_IN_SERVICE",CSM_NODE_IN_SERVICE)
		.value("CSM_NODE_OUT_OF_SERVICE",CSM_NODE_OUT_OF_SERVICE)
		.value("CSM_NODE_SYS_ADMIN_RESERVED",CSM_NODE_SYS_ADMIN_RESERVED)
		.value("CSM_NODE_SOFT_FAILURE",CSM_NODE_SOFT_FAILURE)
		.value("CSM_NODE_MAINTENANCE",CSM_NODE_MAINTENANCE)
		.value("CSM_NODE_DATABASE_NULL",CSM_NODE_DATABASE_NULL);

    enum_<csmi_ras_severity_t>("csmi_ras_severity_t")
		.value("CSM_RAS_NO_SEV",CSM_RAS_NO_SEV)
		.value("CSM_RAS_INFO",CSM_RAS_INFO)
		.value("CSM_RAS_WARNING",CSM_RAS_WARNING)
		.value("CSM_RAS_FATAL",CSM_RAS_FATAL);

};
