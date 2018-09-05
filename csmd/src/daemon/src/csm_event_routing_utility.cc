/*================================================================================
   
    csmd/src/daemon/src/csm_event_routing_utility.cc

  © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
 
================================================================================*/

#include "csmi/src/common/include/csmi_cmds.h"
#include "include/csm_event_routing.h"

#include "inv_agent_handler.h"  // Handler for internal inventory functions

namespace csm {
namespace daemon {

void EventRoutingUtility::RegisterHandlers()
{
   // set up the default event handler
   // note: no need to register cmd with EventHandler if using the default handler
   _DefaultEventHandler = createInstance_sptr<CSMI_FORWARD_HANDLER>();
   Register( CSM_CMD_ERROR, _ErrorEventHandler );
   
   // set up the special event handler for each command
   
   //Register(CSM_CMD_bb_cmd, bbCMD);
   //Register<CSMIBBCMD_Utility>(CSM_CMD_bb_cmd);
   
   CSMI_BASE_sptr fwdHandler = Register<CSMI_FORWARD_HANDLER>(CSM_CMD_allocation_create);
   Register(CSM_CMD_allocation_delete,       fwdHandler);
   Register(CSM_CMD_allocation_update_state, fwdHandler);
   Register(CSM_CMD_allocation_step_begin,   fwdHandler);
   Register(CSM_CMD_allocation_step_end,     fwdHandler);
   Register(CSM_CMD_bb_cmd,                  fwdHandler);
   
   //for csm internal testing
   Register<CSM_INFRASTRUCTURE_TEST_UTILITY>(CSM_infrastructure_test);
   Register<CSM_CTRL_CMD_HANDLER>(CSM_CTRL_cmd);
   
   AddInitEventHandler( createInstance_sptr<CSMI_UTILITY_INIT_HANDLER>() );

   // Register handler for interal inventory functions
   CSMI_BASE_sptr inv_agent_handler = Register<INV_AGENT_HANDLER>(CSM_CMD_INV_get_node_inventory);

   // Register init handler for the internal inventory functions
   AddInitEventHandler(inv_agent_handler);
   
   _EnvironmentHandler = createInstance_sptr<CSM_ENVIRONMENTAL_UTILITY>();
   _IntervalHandler = createInstance_sptr<CSM_INTERVAL_HANDLER>();

#ifdef WITH_MASTER_LOAD_STATS
   AddInitEventHandler ( createInstance_sptr<CSM_DAEMON_CLOCK>() );
#endif
} 

EventRoutingUtility::EventRoutingUtility()
{
  RegisterHandlers();
}

}  // namespace daemon
} // namespace csm
