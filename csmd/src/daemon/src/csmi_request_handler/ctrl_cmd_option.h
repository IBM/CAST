/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/ctrl_cmd_option.h

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#ifndef __CSM_DAEMON_SRC_CSM_CTRL_CMD_OPTION_H__
#define __CSM_DAEMON_SRC_CSM_CTRL_CMD_OPTION_H__

#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/split_member.hpp>
#include <iostream>
#include <sstream>

#include "logging.h"

class CtrlCmdOption
{
public:
  CtrlCmdOption()
  {
    _log_csmdb = utility::bluecoral_sevs::NUM_SEVERITIES;
    _log_csmnet = utility::bluecoral_sevs::NUM_SEVERITIES;
    _log_csmd = utility::bluecoral_sevs::NUM_SEVERITIES;
    _log_csmras = utility::bluecoral_sevs::NUM_SEVERITIES;
    _log_csmapi = utility::bluecoral_sevs::NUM_SEVERITIES;
    _dump_perf_data = false;
    _dump_mem_usage = false;
  }
  
private:
  friend class boost::serialization::access;
  
  template <class Archive>
  void serialize(Archive &ar, const unsigned int version)
  {
    ar & _log_csmdb;
    ar & _log_csmnet;
    ar & _log_csmd;
    ar & _log_csmras;
    ar & _log_csmapi;
    ar & _dump_perf_data;
    ar & _dump_mem_usage;
  }
  
public:
  void set_log_csmdb(utility::bluecoral_sevs i_level)
  {
    _log_csmdb = i_level;
  }
  utility::bluecoral_sevs get_log_csmdb() { return _log_csmdb; }
  
  void set_log_csmd(utility::bluecoral_sevs i_level)
  {
    _log_csmd = i_level;
  }
  utility::bluecoral_sevs get_log_csmd() { return _log_csmd; }

  void set_log_csmnet(utility::bluecoral_sevs i_level)
  {
    _log_csmnet = i_level;
  }
  utility::bluecoral_sevs get_log_csmnet() { return _log_csmnet; }
  
  void set_log_csmras(utility::bluecoral_sevs i_level)
  {
    _log_csmras = i_level;
  }
  utility::bluecoral_sevs get_log_csmras() { return _log_csmras; }
  
  void set_log_csmapi(utility::bluecoral_sevs i_level)
  {
    _log_csmapi = i_level;
  }
  utility::bluecoral_sevs get_log_csmapi() { return _log_csmapi; }
  
  void set_dump_perf_data() { _dump_perf_data = true;}
  bool get_dump_perf_data() { return _dump_perf_data;}
 
  void set_dump_mem_usage() { _dump_mem_usage = true;}
  bool get_dump_mem_usage() { return _dump_mem_usage;}
 
private:
  utility::bluecoral_sevs _log_csmdb;
  utility::bluecoral_sevs _log_csmnet;
  utility::bluecoral_sevs _log_csmd;
  utility::bluecoral_sevs _log_csmras;
  utility::bluecoral_sevs _log_csmapi;
  bool _dump_perf_data;
  bool _dump_mem_usage;

};

#endif

