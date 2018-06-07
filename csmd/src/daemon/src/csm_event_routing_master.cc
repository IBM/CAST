/*================================================================================

    csmd/src/daemon/src/csm_event_routing_master.cc

  © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#include "include/csm_event_routing.h"

#include "inv_master_handler.h"

namespace csm {
namespace daemon {

EventRoutingMaster::EventRoutingMaster()
{
    RegisterHandlers();
}

void EventRoutingMaster::RegisterHandlers()
{
   Register( CSM_CMD_ERROR, _ErrorEventHandler );

   Register<CSMIAllocationCreate_Master>(CSM_CMD_allocation_create);
   Register<CSMIAllocationDelete_Master>(CSM_CMD_allocation_delete);
   Register<CSMIJSRUNCMD_Master>(CSM_CMD_jsrun_cmd);
   Register<CSMIAllocationQuery>(CSM_CMD_allocation_query);
   Register<CSMIAllocationQueryDetails>(CSM_CMD_allocation_query_details);
   Register<CSMIAllocationResourcesQuery>(CSM_CMD_allocation_resources_query);
   Register<CSMIAllocationQueryActiveAll>(CSM_CMD_allocation_query_active_all);
   Register<CSMIAllocationStepBegin>(CSM_CMD_allocation_step_begin);
   //Register<CSMIAllocationStepCGROUPCreate_Master>(CSM_CMD_allocation_step_cgroup_create);
   Register<CSMIAllocationStepEnd>(CSM_CMD_allocation_step_end);
   //Register<CSMIAllocationStepCGROUPDelete_Master>(CSM_CMD_allocation_step_cgroup_delete);
   Register<CSMIAllocationStepQuery>(CSM_CMD_allocation_step_query);
   Register<CSMIAllocationStepQueryActiveAll>(CSM_CMD_allocation_step_query_active_all);
   Register<CSMIAllocationStepQueryDetails>(CSM_CMD_allocation_step_query_details);
   Register<CSMIAllocationUpdateHistory>(CSM_CMD_allocation_update_history);
   Register<CSMIAllocationUpdateState>(CSM_CMD_allocation_update_state);
   Register<CSMINodeResourcesQuery>(CSM_CMD_node_resources_query);
   Register<CSMINodeResourcesQueryAll>(CSM_CMD_node_resources_query_all);
   Register<CSMIBBCMD_Master>(CSM_CMD_bb_cmd);
   Register<CSMIBBLVCreate>(CSM_CMD_bb_lv_create);
   Register<CSMIBBLVDelete>(CSM_CMD_bb_lv_delete);
   Register<CSMIBBLVQuery>(CSM_CMD_bb_lv_query);
   Register<CSMIBBLVUpdate>(CSM_CMD_bb_lv_update);
   Register<CSMIBBVGCreate>(CSM_CMD_bb_vg_create);
   Register<CSMIBBVGDelete>(CSM_CMD_bb_vg_delete);
   Register<CSMIBBVGQuery>(CSM_CMD_bb_vg_query);
   Register<CSMIClusterQueryState>(CSM_CMD_cluster_query_state);
   Register<CSMIDiagResultCreate>(CSM_CMD_diag_result_create);
   Register<CSMIDiagRunBegin>(CSM_CMD_diag_run_begin);
   Register<CSMIDiagRunEnd>(CSM_CMD_diag_run_end);
   Register<CSMIDiagRunQuery>(CSM_CMD_diag_run_query);
   Register<CSMIDiagRunQueryDetails>(CSM_CMD_diag_run_query_details);
   Register<CSMIIbCableInventoryCollection>(CSM_CMD_ib_cable_inventory_collection);
   Register<CSMIIbCableQuery>(CSM_CMD_ib_cable_query);
   Register<CSMIIbCableQueryHistory>(CSM_CMD_ib_cable_query_history);
   Register<CSMIIbCableUpdate>(CSM_CMD_ib_cable_update);
   Register<CSMINodeAttributesQuery>(CSM_CMD_node_attributes_query);
   Register<CSMINodeAttributesQueryDetails>(CSM_CMD_node_attributes_query_details);
   Register<CSMINodeAttributesQueryHistory>(CSM_CMD_node_attributes_query_history);
   Register<CSMINodeAttributesUpdate>(CSM_CMD_node_attributes_update);
   Register<CSMINodeQueryStateHistory>(CSM_CMD_node_query_state_history);
   Register<CSMINodeDelete>(CSM_CMD_node_delete);
   Register<CSMIRasEventCreate>(CSM_CMD_ras_event_create);
   Register<CSMIRasEventQuery>(CSM_CMD_ras_event_query);
   Register<CSMIRasMsgTypeCreate>(CSM_CMD_ras_msg_type_create);
   Register<CSMIRasMsgTypeUpdate>(CSM_CMD_ras_msg_type_update);
   Register<CSMIRasMsgTypeDelete>(CSM_CMD_ras_msg_type_delete);
   Register<CSMIRasMsgTypeQuery>(CSM_CMD_ras_msg_type_query);
   Register<CSMIRasEventQueryAllocation>(CSM_CMD_ras_event_query_allocation);
   Register<CSMISwitchAttributesQuery>(CSM_CMD_switch_attributes_query);
   Register<CSMISwitchAttributesQueryDetails>(CSM_CMD_switch_attributes_query_details);
   Register<CSMISwitchAttributesQueryHistory>(CSM_CMD_switch_attributes_query_history);
   Register<CSMISwitchAttributesUpdate>(CSM_CMD_switch_attributes_update);
   Register<CSMISwitchInventoryCollection>(CSM_CMD_switch_inventory_collection);
   Register<CSMISwitchChildrenInventoryCollection>(CSM_CMD_switch_children_inventory_collection);
   //Register<AllocationInst>(csm_allocation_get);

   // for csm infrastructure testing
   CSMI_BASE_sptr infraTest = Register<CSM_INFRASTRUCTURE_TEST_MASTER>(CSM_infrastructure_test);
   Register(CSM_TEST_MTC, infraTest);
   Register<CSMI_ECHO_HANDLER>( CSM_CMD_ECHO );

   Register<CSM_ERROR_CASE_HANDLER>(CSM_error_inject);

   Register<CSM_CTRL_CMD_HANDLER>(CSM_CTRL_cmd);
   
   AddInitEventHandler( createInstance_sptr<CSMI_MASTER_INIT_HANDLER>() );
   
   // Internal node inventory functions
   Register<InvGetNodeInventory>(CSM_CMD_INV_get_node_inventory);
   AddInitEventHandler( createInstance_sptr<INV_MASTER_HANDLER>() );

   // Environmental data handling
   Register<CSM_ENVDATA_HANDLER>( CSM_environmental_data );

#ifdef WITH_MASTER_LOAD_STATS
   AddInitEventHandler( createInstance_sptr<CSM_DAEMON_CLOCK>() );
#endif
}
 
EventRoutingMaster::~EventRoutingMaster()
{
  _csmi_map.clear();
}

}  // namespace daemon
} // namespace csm
