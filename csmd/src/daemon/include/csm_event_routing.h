/*================================================================================

    csmd/src/daemon/include/csm_event_routing.h

  © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#ifndef CSM_DAEMON_SRC_CSM_EVENT_ROUTING_H_
#define CSM_DAEMON_SRC_CSM_EVENT_ROUTING_H_

#include <map>
#include <netdb.h>

#include <include/csm_core_event.h>
#include "include/csm_event_type_definitions.h"

#include "csm_daemon_config.h"

#include "src/csmi_request_handler/csmi_base.h"
#include "src/csmi_request_handler/CSMIAllocationCreate.h"
#include "src/csmi_request_handler/CSMIAllocationDelete.h"
#include "src/csmi_request_handler/CSMIAllocationQuery.h"
#include "src/csmi_request_handler/CSMIAllocationQueryDetails.h"
#include "src/csmi_request_handler/CSMIAllocationResourcesQuery.h"
#include "src/csmi_request_handler/CSMIAllocationQueryActiveAll.h"
#include "src/csmi_request_handler/CSMIAllocationStepBegin.h"
#include "src/csmi_request_handler/CSMIAllocationStepCGROUPCreate.h"
#include "src/csmi_request_handler/CSMIAllocationStepCGROUPDelete.h"
#include "src/csmi_request_handler/CSMIAllocationStepEnd.h"
#include "src/csmi_request_handler/CSMIAllocationStepQuery.h"
#include "src/csmi_request_handler/CSMIAllocationStepQueryActiveAll.h"
#include "src/csmi_request_handler/CSMIAllocationStepQueryDetails.h"
#include "src/csmi_request_handler/CSMIAllocationUpdateHistory.h"
#include "src/csmi_request_handler/CSMIAllocationUpdateState.h"
#include "src/csmi_request_handler/CSMINodeResourcesQuery.h"
#include "src/csmi_request_handler/CSMINodeResourcesQueryAll.h"
#include "src/csmi_request_handler/CSMIBBCMD.h"
#include "src/csmi_request_handler/CSMIBBLVCreate.h"
#include "src/csmi_request_handler/CSMIBBLVDelete.h"
#include "src/csmi_request_handler/CSMIBBLVQuery.h"
#include "src/csmi_request_handler/CSMIBBLVUpdate.h"
#include "src/csmi_request_handler/CSMIBBVGCreate.h"
#include "src/csmi_request_handler/CSMIBBVGDelete.h"
#include "src/csmi_request_handler/CSMIBBVGQuery.h"
#include "src/csmi_request_handler/CSMICGROUPLogin.h"
#include "src/csmi_request_handler/CSMIDiagResultCreate.h"
#include "src/csmi_request_handler/CSMIDiagRunBegin.h"
#include "src/csmi_request_handler/CSMIDiagRunEnd.h"
#include "src/csmi_request_handler/CSMIDiagRunQuery.h"
#include "src/csmi_request_handler/CSMIDiagRunQueryDetails.h"
#include "src/csmi_request_handler/CSMIIbCableInventoryCollection.h"
#include "src/csmi_request_handler/CSMIIbCableQuery.h"
#include "src/csmi_request_handler/CSMIIbCableQueryHistory.h"
#include "src/csmi_request_handler/CSMIIbCableUpdate.h"
#include "src/csmi_request_handler/CSMINodeAttributesUpdate.h"
#include "src/csmi_request_handler/CSMINodeAttributesQuery.h"
#include "src/csmi_request_handler/CSMINodeAttributesQueryDetails.h"
#include "src/csmi_request_handler/CSMINodeAttributesQueryHistory.h"
#include "src/csmi_request_handler/CSMINodeQueryStateHistory.h"
#include "src/csmi_request_handler/CSMINodeDelete.h"
#include "src/csmi_request_handler/CSMIRasEventCreate.h"
#include "src/csmi_request_handler/CSMIRasEventQuery.h"
#include "src/csmi_request_handler/CSMIRasMsgTypeCreate.h"
#include "src/csmi_request_handler/CSMIRasMsgTypeUpdate.h"
#include "src/csmi_request_handler/CSMIRasMsgTypeDelete.h"
#include "src/csmi_request_handler/CSMIRasMsgTypeQuery.h"
#include "src/csmi_request_handler/CSMIRasEventQueryAllocation.h"
#include "src/csmi_request_handler/CSMISwitchAttributesQuery.h"
#include "src/csmi_request_handler/CSMISwitchAttributesQueryDetails.h"
#include "src/csmi_request_handler/CSMISwitchAttributesQueryHistory.h"
#include "src/csmi_request_handler/CSMISwitchAttributesUpdate.h"
#include "src/csmi_request_handler/CSMISwitchInventoryCollection.h"
#include "src/csmi_request_handler/CSMISwitchChildrenInventoryCollection.h"
#include "src/csmi_request_handler/csmi_forward_handler.h"
#include "src/csmi_request_handler/csmi_error_handler.h"
#include "src/csmi_request_handler/csmi_echo_handler.h"
#include "src/csmi_request_handler/csmi_master_init_handler.h"
#include "src/csmi_request_handler/csmi_agg_init_handler.h"
#include "src/csmi_request_handler/csmi_utility_init_handler.h"
#include "src/csmi_request_handler/csm_infrastructure_test.h"
#include "src/csmi_request_handler/csmi_agg_mtc_handler.h"
#include "src/csmi_request_handler/csm_ctrl_cmd_handler.h"
#include "src/csmi_request_handler/csm_conn_ctrl_handler.h"
#include "src/csmi_request_handler/csm_daemon_clock.h"
#include "src/csmi_request_handler/csm_environmental_handler.h"
#include "src/csmi_request_handler/csm_envdata_handler.h"
#include "src/csmi_request_handler/csm_error_case_handler.h"

#include "src/csmi_request_handler/csm_handler_options.h"

// Internal Inventory Functions
#include "src/csmi_request_handler/InvGetNodeInventory.h"

namespace csm {
namespace daemon {

class EventRouting
{
public:
  typedef std::map<csmi_cmd_t, CSMI_BASE_sptr > csmi_map_type;
  
protected:
  CSMI_BASE_sptr _DefaultEventHandler;
  CSMI_BASE_sptr _EnvironmentHandler;
  CSMI_BASE_sptr _ErrorEventHandler;

  csm::daemon::HandlerOptions _options;
  csmi_map_type _csmi_map;

public:
  EventRouting()
  {
    // get the pointer to the config instance
    _csmConfig = csm::daemon::Configuration::Instance();
    
    // need to set up the _options now before starting the registration process
    _options = csm::daemon::HandlerOptions();
    
    _ErrorEventHandler = createInstance_sptr<CSMI_ERROR_HANDLER>();
    _DefaultEventHandler = nullptr;
    _EnvironmentHandler = nullptr;

  }

  template<typename T>
  CSMI_BASE_sptr createInstance_sptr()
  { return CSMI_BASE_sptr(new T(_options)); }
    
  template <typename T>
  CSMI_BASE_sptr Register(csmi_cmd_t aCmd)
  {
    CSMI_BASE_sptr obj = createInstance_sptr<T>();
    _csmi_map[aCmd] = obj;
    return obj;
  }
  
  inline void Register(csmi_cmd_t aCmd, CSMI_BASE_sptr aObj)
  {
    _csmi_map[aCmd] = aObj;
  }
  
  void AddPublicAPIHandlers(csmi_cmd_t aCmd)
  {
    auto dst = _csmi_map.find(aCmd);
    if (dst == _csmi_map.end()) return;
    
    std::unordered_set<CSMI_BASE_sptr> list;
    for (auto it = _csmi_map.begin(); it != _csmi_map.end(); ++it)
      // \todo: later, we probably only care about the data for public API calls
      //if (it->first > CSM_CMD_UNDEFINED && it->first < CSM_FIRST_INTERNAL)
        list.insert(it->second);
    
    dst->second->AddPublicAPIHandlers(list);
  }
  
  CSMI_BASE *GetEventHandler(csmi_cmd_t cmd)
  {
    // check if the handler for the cmd is registered
    auto it = _csmi_map.find(cmd);
    if (it == _csmi_map.end()) return NULL;
    else return it->second.get();
  }
  
  inline CSMI_BASE *GetErrorEventHandler()
  {
    return ( (_ErrorEventHandler == nullptr) ? nullptr : _ErrorEventHandler.get() );
  }
  
  inline CSMI_BASE *GetDefaultEventHandler()
  {
    return ( (_DefaultEventHandler == nullptr) ? nullptr : _DefaultEventHandler.get() );
  }

  inline CSMI_BASE *GetEnvironmentalHandler()
  {
    return ( (_EnvironmentHandler == nullptr) ? nullptr : _EnvironmentHandler.get() );
  }
  
  inline void AddInitEventHandler(CSMI_BASE_sptr aHandler)
  {
    _InitHandlers.push_back(aHandler);
  }
  
  virtual int InitProcess(std::vector<csm::daemon::CoreEvent *>& postEventList)
  {
    for (auto initHandler : _InitHandlers)
    {
      csm::daemon::CoreEvent undefEvent = csm::daemon::CoreEvent(nullptr, EVENT_TYPE_INITIAL, nullptr);
      initHandler->Process(undefEvent, postEventList);
    }
    return 0;
  }

  virtual ~EventRouting()
  {}

  virtual csm::network::Address_sptr GetMasterAddress() const = 0;
  virtual csm::network::Address_sptr GetAggregatorAddress() const = 0;
  
private:
  
  virtual void RegisterHandlers() = 0;
  
  csm::daemon::Configuration *_csmConfig;
  std::vector< CSMI_BASE_sptr > _InitHandlers;
};

class EventRoutingMaster : public EventRouting
{
protected:

  void SetAllEventProcessors();
  
public:
  EventRoutingMaster();
  
  virtual ~EventRoutingMaster();
  
private:
  void RegisterHandlers();

  virtual csm::network::Address_sptr GetMasterAddress() const { return nullptr; }
  virtual csm::network::Address_sptr GetAggregatorAddress() const
  {
    return _Aggregator;
  }

  csm::network::Address_sptr _Aggregator;
};

class EventRoutingUtility : public EventRouting
{

protected:
  void SetAllEventProcessors();
  
public:
  EventRoutingUtility();

  virtual ~EventRoutingUtility() { }

private:
    void RegisterHandlers();

    virtual csm::network::Address_sptr GetMasterAddress() const
    {
      return _Master;
    }
    virtual csm::network::Address_sptr GetAggregatorAddress() const
    {
      return _Master;
    }

    csm::network::Address_sptr _Master;
};


class EventRoutingAgg : public EventRouting
{
protected:
  void SetAllEventProcessors();
  
public:
  EventRoutingAgg();
    
  virtual ~EventRoutingAgg() { }

  virtual csm::network::Address_sptr GetMasterAddress() const
  {
    return _Master;
  }
  virtual csm::network::Address_sptr GetAggregatorAddress() const { return nullptr; }

private:
    void RegisterHandlers();
    
    csm::network::Address_sptr _Master;
};

class EventRoutingAgent : public EventRouting
{

protected:
  void SetAllEventProcessors();
  
public:
  EventRoutingAgent();
  
  virtual ~EventRoutingAgent();

  virtual csm::network::Address_sptr GetMasterAddress() const
  {
    return _AggregatorA;
  }
  virtual csm::network::Address_sptr GetAggregatorAddress() const
  {
    return _AggregatorA;
  }

private:
  void RegisterHandlers();

  csm::network::AddressPTP_sptr _AggregatorA;
//  csm::network::AddressPTP _AggregatorB;
};


}  // namespace daemon
} // namespace csm


#endif
