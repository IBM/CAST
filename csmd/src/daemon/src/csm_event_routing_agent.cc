/*================================================================================

    csmd/src/daemon/src/csm_event_routing_agent.cc

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
/* csm_event_routing_agent.cc
 *
 ******************************************/

#include "include/csm_event_routing.h"
#include "csmi_request_handler/csmi_init_handler_base.h"
#include "inv_agent_handler.h"  // Handler for internal inventory functions

namespace csm {
namespace daemon {

void EventRoutingAgent::RegisterHandlers()
{
   // set up the default event handler
   // note: no need to register cmd with EventHandler if using the default handler
   _DefaultEventHandler = createInstance_sptr<CSMI_FORWARD_HANDLER>();
   Register( CSM_CMD_ERROR, _ErrorEventHandler);
   
   Register<CSMIAllocationCreate_Agent>(CSM_CMD_allocation_create);
   Register<CSMIAllocationDelete_Agent>(CSM_CMD_allocation_delete);
   Register<CSMIAllocationStepBegin_Agent>(CSM_CMD_allocation_step_begin);
   Register<CSMIAllocationStepEnd_Agent>(CSM_CMD_allocation_step_end);
   Register<CSMIBBCMD_Agent>(CSM_CMD_bb_cmd);
   Register<CSMIJSRUNCMD_Agent>(CSM_CMD_jsrun_cmd);
   Register<CSMIAllocationUpdateState_Agent>(CSM_CMD_allocation_update_state);
   Register<CSMIAllocationStepCGROUPCreate>(CSM_CMD_allocation_step_cgroup_create);
   Register<CSMIAllocationStepCGROUPDelete>(CSM_CMD_allocation_step_cgroup_delete);
   Register<CSMICGROUPLogin>(CSM_CMD_cgroup_login);
      
   //CSM_CMD_allocation_step_end
   //for csm infrastructure testing
   Register<CSM_INFRASTRUCTURE_TEST_AGENT>(CSM_infrastructure_test);
   Register<CSMI_ECHO_HANDLER>(CSM_TEST_MTC);
   Register<CSM_CTRL_CMD_HANDLER>(CSM_CTRL_cmd);

   AddInitEventHandler(createInstance_sptr<CSMI_INIT_HANDLER_BASE>());

   // Register handler for interal inventory functions
   CSMI_BASE_sptr inv_agent_handler = Register<INV_AGENT_HANDLER>(CSM_CMD_INV_get_node_inventory);
   CSMI_BASE_sptr conn_ctrl_handler = Register<CONN_CTRL_HANDLER>(CSM_CMD_CONNECTION_CTRL);

   // Register init handler for the internal inventory functions
   AddInitEventHandler(inv_agent_handler);
   AddInitEventHandler(conn_ctrl_handler);
   
#ifdef WITH_MASTER_LOAD_STATS
   AddInitEventHandler( createInstance_sptr<CSM_DAEMON_CLOCK>() );
#endif

   _EnvironmentHandler = createInstance_sptr<CSM_ENVIRONMENTAL>();
}

EventRoutingAgent::EventRoutingAgent()
{

    RegisterHandlers();
}


EventRoutingAgent::~EventRoutingAgent()
{ }

}  // namespace daemon
} // namespace csm
