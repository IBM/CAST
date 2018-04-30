/*================================================================================

    csmi/src/common/include/csmi_cmds_def.h

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
// NOTES: An API name has to have csm_ as its prefix. In this file, only the string after the prefix
//        needs to be used. For example, for the csm_allcation_query api, add an entry
//        cmd(allocation_query) here.
cmd(UNDEFINED)
//Burst Buffer
cmd(bb_cmd)//1
cmd(bb_lv_create)
cmd(bb_lv_delete)
cmd(bb_lv_query)
cmd(bb_lv_update)
cmd(bb_vg_create)
cmd(bb_vg_delete)
cmd(bb_vg_query)
// Diagnostic
cmd(diag_result_create)// 8
cmd(diag_run_begin)
cmd(diag_run_end)
cmd(diag_run_query)
cmd(diag_run_query_details)
// Workload Management
cmd(allocation_step_cgroup_delete)//14
cmd(allocation_step_cgroup_create)
cmd(allocation_update_state)
cmd(allocation_query_active_all)
cmd(allocation_create) 
cmd(allocation_delete)//19
cmd(allocation_query)
cmd(allocation_query_details)
cmd(allocation_step_begin)
cmd(allocation_step_end)
cmd(allocation_step_query)//24
cmd(allocation_step_query_active_all)
cmd(allocation_step_query_details)
cmd(node_resources_query)
cmd(node_resources_query_all)
// RAS
cmd(ras_event_create)   // 29
cmd(ras_subscribe)
cmd(ras_unsubscribe)
cmd(ras_sub_event)
cmd(ras_event_query)
cmd(ras_msg_type_create)
cmd(ras_msg_type_update)//35
cmd(ras_msg_type_get)
cmd(ras_msg_type_delete)
cmd(ras_msg_type_query)
// Inventory
cmd(ib_cable_query)  // 39
cmd(ib_cable_query_history)
cmd(ib_cable_update)
cmd(node_attributes_update)  
cmd(node_attributes_query)
cmd(node_attributes_query_details)
cmd(node_attributes_query_history)
cmd(node_query_state_history)
cmd(node_delete)
cmd(switch_attributes_query)
cmd(switch_attributes_query_details)
cmd(switch_attributes_query_history)
cmd(switch_attributes_update)
// Internal Inventory Functions
cmd(INV_get_node_inventory)//50
cmd(ib_cable_inventory_collection)
cmd(switch_inventory_collection)
cmd(switch_children_inventory_collection)

// PRPQ
cmd(allocation_resources_query)
cmd(allocation_update_history)
cmd(ras_event_query_allocation)
cmd(cgroup_login)
cmd(jsrun_cmd)

