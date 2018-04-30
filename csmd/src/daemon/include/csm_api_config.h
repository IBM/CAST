/*================================================================================

    csmd/src/daemon/include/csm_api_acl.h

  © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#ifndef CSM_DAEMON_SRC_CSM_API_TIMEOUTS_H_
#define CSM_DAEMON_SRC_CSM_API_TIMEOUTS_H_

#include <sys/types.h>
#include <pwd.h>
#include <grp.h>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include <boost/tokenizer.hpp>

#include <memory>
#include <map>
#include <climits>
#include <unordered_set>

#ifndef logprefix
#define logprefix "APICFG"
#define logprefix_local
#endif
#include "csm_pretty_log.h"

#include "csmi/src/common/include/csmi_cmds.h"
#include "csmi/src/common/include/csmi_serialization.h"
#include "csmnet/include/csm_timing.h"

namespace pt = boost::property_tree;

namespace  csm {
namespace daemon{

/*
 * API configuration (currently only timeouts)
 * Note that this class doesn't hold the timeouts itself
 * it only reads the timeouts and adjusts the C-code based
 * csm_get_timeout() function/timeout array
 * This was decided to avoid duplicate code,
 * since the C client lib needs those timeouts too
 */

class CSMAPIConfig
{
private:
  std::string _to_file;
  std::string _timeouts_serialized;

  void ParseTimeoutsFile(std::string i_file)
  {
    try
    {
      CSMLOG( csmd, info ) << "Reading API configuration file: " << i_file;
      pt::ptree auth_tree;
      pt::read_json(i_file, auth_tree);
      BOOST_FOREACH(pt::ptree::value_type &list, auth_tree)
      {
        std::string key = list.first.data();
        boost::algorithm::to_lower(key);

        csmi_cmd_t cmd = csmi_cmd_get( key.c_str() );
        if (cmd < CSM_CMD_MAX)
        {
          int timeout = std::stoi( list.second.data() );
          if( timeout >= CSM_RECV_TIMEOUT_MIN )
            csmi_cmd_timeouts[ cmd ] = timeout;
          else
          {
            CSMLOG( csmd, warning ) << "API=" << key << " keeping default " << CSM_RECV_TIMEOUT_SECONDS
              << ". Configured timeout out of range: " << timeout
              << " required >" << CSM_RECV_TIMEOUT_MIN;
          }
          CSMLOG( csmd, debug ) << "API=" << key << " TIMEOUT=" << csmi_cmd_timeouts[ cmd ];
        }
        else
        {
          CSMLOG( csmd, warning ) << "API=" << key << " NOT DEFINED!";
        }

#if 0
        if ( key == "privileged_user_id" )
        {
          // can be a single value or a list of uids
          std::vector< std::string > id_ary;
          if ( !list.second.data().empty() ) id_ary.push_back(list.second.data());
          else
          {
            BOOST_FOREACH(pt::ptree::value_type &id_list, list.second)
            {
              id_ary.push_back( id_list.second.data() );
            }
          }

          for (size_t i=0; i<id_ary.size(); i++)
          {
            std::string uid = id_ary[i];
            if ( struct passwd *uentry = getpwnam(uid.c_str()) ) AddPrivilegedUid( uentry->pw_uid);
            else
            {
              CSMLOG( csmd, warning ) << "privileged_user_id \"" << uid << "\" not valid. Ignore!";
            }
          }
        } // end if ( key == "privileged_user_id" )
        else if ( key == "privileged_group_id" )
        {
          // can be a single value or a list of groups
          std::vector< std::string > gid_ary;
          if ( !list.second.data().empty() ) gid_ary.push_back(list.second.data());
          else
          {
            BOOST_FOREACH(pt::ptree::value_type &gid_list, list.second)
            {
              gid_ary.push_back( gid_list.second.data()  );
            }
          }
          
          for (size_t i=0; i<gid_ary.size(); i++)
          {
            std::string gid = gid_ary[i];
            if ( struct group *gentry = getgrnam(gid.c_str()) ) AddPrivilegedGid( gentry->gr_gid );
            else
            {
              CSMLOG( csmd, warning ) << "privileged_group_id \"" << gid << "\" not valid. Ignore!";
            }
          }
        } // end  if ( key == "privileged_group_id" )
        else if ( (key == "public") || (key == "private") )
        {
          API_SEC_LEVEL grp = (key == "public")? PUBLIC:PRIVATE;
          BOOST_FOREACH(pt::ptree::value_type &api_list, list.second)
          {
            std::string api = api_list.second.data();
            boost::algorithm::to_lower(api);
            if (!api.empty())
            {
              csmi_cmd_t cmd = csmi_cmd_get(api.c_str());
              if (cmd < CSM_CMD_MAX)
              {
                _api_to_grp_map[cmd] = grp;
                CSMLOG( csmd, debug ) << "API=" << api << " GROUP=" << key;
              }
              else
              {
                CSMLOG( csmd, warning ) << "API=" << api << " NOT DEFINED!";
              }
            } // if api is not empty
          } // end BOOST_FOREACH in public | private
        } // end if key=public | private
        
#endif
      } // end BOOST_FOREACH
    } // end try
    catch (pt::json_parser_error& f)
    {
      CSMLOG( csmd, warning) << "file: " << f.what()
        << "Skip the rest of the lines in the file..."
        << "Unparsed APIs set to default timeout=" << CSM_RECV_TIMEOUT_SECONDS;
    }
    catch (std::exception &e)
    {
      CSMLOG( csmd, warning ) << "file: " << i_file << " " << e.what()
        << "Skip the rest of the lines in the file..."
        << "Unparsed APIs default timeout=" << CSM_RECV_TIMEOUT_SECONDS;
    }
  }
  
public:
  CSMAPIConfig(std::string i_file)
  {
    _to_file = i_file;

    if ( !i_file.empty() ) ParseTimeoutsFile(i_file);
    else
      CSMLOG( csmd, warning ) << "No API Configuration file defined (key: csm.api_configuration_file). All API settings will be default.";

    _timeouts_serialized.clear();
    for( int n=0; n<CSM_CMD_INVALID; ++n )
    {
      int timeout = csm_get_timeout(n) / 1000;
      if(( timeout >= CSM_RECV_TIMEOUT_MIN ) && ( timeout != CSM_RECV_TIMEOUT_SECONDS ))
      {
        _timeouts_serialized.append( std::to_string( n ) );
        _timeouts_serialized.append( ":" );
        _timeouts_serialized.append( std::to_string( timeout ) );
        _timeouts_serialized.append(";");
      }
      else if( timeout < CSM_RECV_TIMEOUT_MIN )
      {
        CSMLOG( csmd, warning ) << "Configured timeout (" << timeout
            << ") of " << csmi_cmds_t_strs[n] << " is shorter than the min " << CSM_RECV_TIMEOUT_MIN;
      }
    }
    CSMLOG( csmd, debug ) << "Serialized timeouts: " << _timeouts_serialized;
  }

  ~CSMAPIConfig()
  {  }

public:
  std::string GetSerializedTimeouts()
  {
    return _timeouts_serialized;
  }
};

typedef std::shared_ptr<CSMAPIConfig> CSMAPIConfig_sptr;


} // namespace daemon
} // namespace csm


#ifdef logprefix_local
#undef logprefix
#undef logprefix_local
#endif

#endif // CSM_DAEMON_SRC_CSM_API_TIMEOUTS_H_
