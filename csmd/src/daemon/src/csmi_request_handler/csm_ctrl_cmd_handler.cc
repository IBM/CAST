/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/csm_ctrl_cmd_handler.cc

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#include <sys/resource.h>

#include "csm_ctrl_cmd_handler.h"

#include "logging.h"

#define SET_SEVERITY_LEVEL(subcomp, level) \
        if (level == utility::bluecoral_sevs::off) ::setLoggingLevel(subcomp, off); \
        else if (level == utility::bluecoral_sevs::trace) ::setLoggingLevel(subcomp, trace); \
        else if (level == utility::bluecoral_sevs::debug) ::setLoggingLevel(subcomp, debug); \
        else if (level == utility::bluecoral_sevs::info) ::setLoggingLevel(subcomp, info); \
        else if (level == utility::bluecoral_sevs::warning) ::setLoggingLevel(subcomp, warning); \
        else if (level == utility::bluecoral_sevs::error) ::setLoggingLevel(subcomp, error); \
        else if (level == utility::bluecoral_sevs::critical) ::setLoggingLevel(subcomp, critical); \
        else if (level == utility::bluecoral_sevs::always) ::setLoggingLevel(subcomp, always); \
        else if (level == utility::bluecoral_sevs::disable) ::setLoggingLevel(subcomp, disable);

void CSM_CTRL_CMD_HANDLER::Process( const csm::daemon::CoreEvent &aEvent,
                std::vector<csm::daemon::CoreEvent*>& postEventList )
{
  if (!isNetworkEvent(aEvent))
  {
    LOG(csmd, error) << "CSM_CTRL_CMD_HANDLER: Expecting a NetworkEvent...";
    return;
  }
  
  LOG(csmd, debug) << "CSM_CTRL_CMD_HANDLER::Process...";

  // get the options from clients' request
  csm::network::Message msg = GetNetworkMessage(aEvent);
  CtrlCmdOption option;
  CSMI_BASE::ConvertToClass<CtrlCmdOption>(msg.GetData(), option);
  
  if (option.get_log_csmd() != utility::bluecoral_sevs::NUM_SEVERITIES)
  {
    SET_SEVERITY_LEVEL(csmd, option.get_log_csmd());
  }
  if (option.get_log_csmdb() != utility::bluecoral_sevs::NUM_SEVERITIES)
  {
    SET_SEVERITY_LEVEL(csmdb, option.get_log_csmdb());
  }
  if (option.get_log_csmnet() != utility::bluecoral_sevs::NUM_SEVERITIES)
  {
    SET_SEVERITY_LEVEL(csmnet, option.get_log_csmnet());
  }
  if (option.get_log_csmras() != utility::bluecoral_sevs::NUM_SEVERITIES)
  {
    SET_SEVERITY_LEVEL(csmras, option.get_log_csmras());
  }
  if (option.get_log_csmapi() != utility::bluecoral_sevs::NUM_SEVERITIES)
  {
    SET_SEVERITY_LEVEL(csmapi, option.get_log_csmapi());
  }
  
  // process the options which would require to generate the response.
  // The response is simply a string
  std::string reply_payload;
  if (option.get_dump_perf_data())
  {
    reply_payload.append( GetPerfData() );
  }

  if (option.get_dump_mem_usage())
  {
    reply_payload.append( GetMemUsage() );
    reply_payload.append( GetDaemonState()->DumpMapSize() );
  }

  if( option.get_agg_reset() )
  {
    LOG( csmd, info ) << "Resetting Primary aggregator on user request.";
    if( _handlerOptions.GetRole() != CSM_DAEMON_ROLE_AGENT )
      reply_payload.append("Primary aggregator reset only applicable to compute daemons.\n");
    else
    {
      csm::daemon::DaemonStateAgent *dsa = dynamic_cast<csm::daemon::DaemonStateAgent*>( _handlerOptions.GetDaemonState() );
      if( dsa != nullptr )
      {
        csm::daemon::SystemContent content(csm::daemon::SystemContent::RESET_AGG);
        postEventList.push_back( new csm::daemon::SystemEvent( content, csm::daemon::EVENT_TYPE_SYSTEM, nullptr ) );
        reply_payload.append("Successfully created trigger for daemon to reset primary aggregator. Please check the daemon log.\n");
      }
    }
  }

  msg.SetData(reply_payload );
  msg.SetResp();
  msg.CheckSumUpdate();

  postEventList.push_back( CreateNetworkEvent(msg, GetNetworkAddress(aEvent)) );
}

std::string CSM_CTRL_CMD_HANDLER::GetMemUsage()
{
  struct rusage RU;
  double max_memory;
  
  std::stringstream ss;
  ss << "Memory Usage:\nmax resident set size    = ";
  getrusage(RUSAGE_SELF, &RU);
  max_memory = ((double) RU.ru_maxrss)/1024.0;
  ss << max_memory;
  ss << " MBytes\n";
  return ss.str();

}

std::string CSM_CTRL_CMD_HANDLER::GetPerfData()
{
  std::stringstream ss;
  ss << "Call Count:" << "\n";
  for (auto it = _PublicAPIHandlers.begin(); it != _PublicAPIHandlers.end(); it++)
  {
    CSMI_BASE_sptr hdl = *it;
    if ( hdl->GetNewRequestCount() > 0 )
      ss << "\t" << hdl->getCmdName() << " = " << hdl->GetNewRequestCount() << "\n";
  }
  return ss.str();
}
