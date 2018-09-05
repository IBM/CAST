/*================================================================================

    csmd/src/daemon/src/csm_event_routing_agg.cc

  © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#include "include/csm_event_routing.h"

#include "inv_aggregator_handler.h"  // Handler for internal inventory functions

namespace csm {
namespace daemon {
  
void EventRoutingAgg::RegisterHandlers()
{
   // set up the default event handler
   // note: no need to register cmd with EventHandler if using the default handler
   _DefaultEventHandler = createInstance_sptr<CSMI_FORWARD_HANDLER>();
   Register( CSM_CMD_ERROR, _ErrorEventHandler );
   
   // set up the special event handler for each command
   
   //Multi-cast handlers need to be registered here:
   CSMI_BASE_sptr mtcHandler = Register<CSMI_AGG_MTC_HANDLER>(CSM_CMD_allocation_create);
   Register(CSM_CMD_allocation_delete,       mtcHandler);
   Register(CSM_CMD_allocation_update_state, mtcHandler);
   Register(CSM_CMD_allocation_step_begin,   mtcHandler);
   Register(CSM_CMD_allocation_step_end,     mtcHandler);
   Register(CSM_CMD_bb_cmd,                  mtcHandler);
   Register(CSM_CMD_jsrun_cmd,               mtcHandler);

   // Environmental data handling
   Register<CSM_ENVDATA_HANDLER>( CSM_environmental_data );

   // for csm infrastructure testing
   Register<CSM_INFRASTRUCTURE_TEST_AGG>(CSM_infrastructure_test);
   Register(CSM_TEST_MTC, mtcHandler);
   Register<CSM_CTRL_CMD_HANDLER>(CSM_CTRL_cmd);
   
   AddInitEventHandler( createInstance_sptr<CSMI_AGG_INIT_HANDLER>() );
 
   // Register handler for interal inventory functions
   CSMI_BASE_sptr inv_aggregator_handler = Register<INV_AGGREGATOR_HANDLER>(CSM_CMD_INV_get_node_inventory);

   // Register init handler for collecting the local inventory during initialization
   AddInitEventHandler(inv_aggregator_handler);

   _IntervalHandler = createInstance_sptr<CSM_INTERVAL_HANDLER>();

#ifdef WITH_MASTER_LOAD_STATS
   AddInitEventHandler( createInstance_sptr<CSM_DAEMON_CLOCK>() );
#endif 
}

EventRoutingAgg::EventRoutingAgg()
  {
    //_RewriteFn.SetMultiCastSuffix( std::string( "QUERY" ) );
    //_RewriteFn.SetMultiCastReply( GetMQTTReqTopic() );
    
    RegisterHandlers();
}

}  // namespace daemon
} // namespace csm
