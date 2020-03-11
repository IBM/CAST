/*******************************************************************************
 |    nodecontroller.h
 |
 |  � Copyright IBM Corporation 2015,2016. All Rights Reserved
 |
 |    This program is licensed under the terms of the Eclipse Public License
 |    v1.0 as published by the Eclipse Foundation and available at
 |    http://www.eclipse.org/legal/epl-v10.html
 |
 |    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
 |    restricted by GSA ADP Schedule Contract with IBM Corp.
 *******************************************************************************/

#ifndef D_NODECONTROLLER_CSM_H
#define D_NODECONTROLLER_CSM_H

#include <string>
#include "nodecontroller.h"
#include "csm_api_burst_buffer.h"
#include "csm_api_ras.h"
#include "csm_api_common.h"
#include <dlfcn.h>
#include "csm_api_workload_manager.h"
#include "csmi/include/csm_api_macros.h"

//dlsym function types for csm
typedef int (*csm_init_lib_vers_t)(int64_t version_id);
typedef int (*csm_term_lib_t)();
typedef void (*csm_api_object_clear_t)(csm_api_object *csm_obj);
typedef void (*csm_api_object_destroy_t)(csm_api_object *csm_obj);

typedef int (*csm_bb_vg_create_t)(csm_api_object **handle, csm_bb_vg_create_input_t* input);
typedef int (*csm_bb_lv_create_t)(csm_api_object **handle, csm_bb_lv_create_input_t* input);
typedef int (*csm_bb_lv_delete_t)(csm_api_object **handle, csm_bb_lv_delete_input_t* input);
typedef int (*csm_bb_lv_update_t)(csm_api_object **handle, csm_bb_lv_update_input_t* input);


typedef int (*csm_bb_cmd_t)(csm_api_object **handle, csm_bb_cmd_input_t* input, csm_bb_cmd_output_t** output);
typedef int (*csm_allocation_query_t)(csm_api_object **handle,
                         csm_allocation_query_input_t *input,
                         csm_allocation_query_output_t **output);

typedef int (*csm_allocation_query_active_all_t)(csm_api_object **handle, 
   csm_allocation_query_active_all_input_t *input,
   csm_allocation_query_active_all_output_t **output);


typedef int (*csm_ras_event_create_t)(csm_api_object **csm_obj,
                         const char *msg_id, 
                         const char *time_stamp,
                         const char *location_name,
                         const char *raw_data,
                         const char *kvcsv);

class NodeController_CSM : public NodeController
{
  private:
    csm_init_lib_vers_t      _csm_init_lib_vers_func;
    csm_term_lib_t           _csm_term_lib_func;
    csm_api_object_clear_t   _csm_api_object_clear_func;
    csm_api_object_destroy_t _csm_api_object_destroy_func;
    csm_bb_vg_create_t       _csm_bb_vg_create_func;
    csm_bb_lv_create_t       _csm_bb_lv_create_func;
    csm_bb_lv_delete_t       _csm_bb_lv_delete_func;
    csm_bb_lv_update_t       _csm_bb_lv_update_func;
    csm_bb_cmd_t             _csm_bb_cmd_func;
    csm_ras_event_create_t   _csm_ras_event_create_func;
    csm_allocation_query_t   _csm_allocation_query_func;
    csm_allocation_query_active_all_t _csm_allocation_query_active_all_func;
    csm_api_object* csmhandle;
    std::string myhostname;
    std::map< std::pair<uint64_t,uint32_t>, csmi_allocation_t* > job2allocationmap;

    int convertState(enum LVState state, char& statechar);
    int getAllocationInfo(csmi_allocation_t& allocation);
    int getcsmSymbols(const std::string& controllerPath);


  public:
    NodeController_CSM();
    virtual ~NodeController_CSM();

    virtual int gethostname(std::string& pHostName);
    virtual int gethostlist(std::string& hostlist);
    virtual int lvcreate(const std::string& lvname, enum LVState state, size_t current_size,
                         const std::string& mountpath, const std::string& fstype);
    virtual int lvremove(const std::string& lvname, const BBUsage_t& usage);
    virtual int lvupdate(const std::string& lvname, enum LVState state, size_t current_size);
    virtual int postRAS(const TSHandler& tsthis);
    virtual int bbcmd(std::vector<std::uint32_t> ranklist,
                      std::vector<std::string> nodelist,
                      std::string executable,
                      std::vector<std::string> arguments,
                      boost::property_tree::ptree& output,
                      bool nodebcast);
};

#endif
