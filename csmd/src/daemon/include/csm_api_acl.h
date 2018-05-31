/*================================================================================

    csmd/src/daemon/include/csm_api_acl.h

  © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#ifndef CSM_DAEMON_SRC_CSM_API_ACL_H_
#define CSM_DAEMON_SRC_CSM_API_ACL_H_

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
#define logprefix "ACL"
#define logprefix_local
#endif
#include "csm_pretty_log.h"

#include "csmi/src/common/include/csmi_cmds.h"
#include "csmi/src/common/include/csmi_serialization.h"

namespace pt = boost::property_tree;

namespace  csm {
namespace daemon{

enum API_SEC_LEVEL
{
  PRIVILEGED,
  PRIVATE,
  PUBLIC
};

template<class stream>
static stream&
operator<<( stream &out, const csm::daemon::API_SEC_LEVEL &aSecLevel )
{
  switch( aSecLevel )
  {
    case PRIVILEGED:  out << "PRIVILEGED";  break;
    case PRIVATE:     out << "PRIVATE";     break;
    case PUBLIC:      out << "PUBLIC";      break;
    default:          out << "INVALID SECLEVEL"; break;
  }
  return (out);
}

class CSMIAuthList
{
private:
  std::string _auth_file;
  std::unordered_set< uid_t > _privileged_uids;
  std::unordered_set< gid_t > _privileged_gids;
  
  std::map<csmi_cmd_t, API_SEC_LEVEL> _api_to_grp_map;

  const static int _ngroups=20;
  gid_t *_groups;

  inline void AddPrivilegedUid(uid_t aUid)
  {
    _privileged_uids.insert(aUid);
  }
  
  inline void AddPrivilegedGid(gid_t aGid)
  {
    _privileged_gids.insert(aGid);
  }
  
  inline bool findUid(uid_t aUid) const
  {
    return ( _privileged_uids.find(aUid) != _privileged_uids.end() );
  }
  
  inline bool findGid(gid_t aGid) const
  {
    return ( _privileged_gids.find(aGid) != _privileged_gids.end() );
  }

  inline bool CheckGroups(gid_t *groups, int ngroups) const
  {
    //for (int i=0;i<ngroups;i++) {LOG(csmd, debug) << "group[" << i <<"]  = " << groups[i]; }
    for (int i=0; i<ngroups; i++)
      if ( findGid( groups[i] ) ) return true;
    return false;
  }
    
  void ParseACLFile(std::string i_file)
  {
    try
    {
      CSMLOG( csmd, info ) << "Reading API permissions from ACL file: " << i_file;
      pt::ptree auth_tree;
      pt::read_json(i_file, auth_tree);
      BOOST_FOREACH(pt::ptree::value_type &list, auth_tree)
      {
        std::string key = list.first.data();
        boost::algorithm::to_lower(key);
        
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
              if( csmi_cmd_is_valid( cmd ) )
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
        
      } // end BOOST_FOREACH
    } // end try
    catch (pt::json_parser_error& f)
    {
      CSMLOG( csmd, error ) << "file: " << i_file << " " << f.what();
      CSMLOG( csmd, error ) << "Skip the rest of the lines in the file...";
      CSMLOG( csmd, error ) << "Unparsed APIs considered PRIVILEGED!";
    }
    catch (std::exception &e)
    {
      CSMLOG( csmd, error ) << "file: " << i_file << " " << e.what();
      CSMLOG( csmd, error ) << "Skip the rest of the lines in the file...";
      CSMLOG( csmd, error ) << "Unparsed APIs considered PRIVILEGED!";
    }
  }
  
public:
  CSMIAuthList(std::string i_file)
  :_groups(nullptr)
  {
    _auth_file = i_file;
   
    _groups = (gid_t *) malloc(sizeof(gid_t)*_ngroups);

    // root always is privileged
    if ( struct passwd *uentry = getpwnam("root") ) AddPrivilegedUid(uentry->pw_uid);
    if ( struct group *gentry = getgrnam("root") ) AddPrivilegedGid(gentry->gr_gid);

    if ( !i_file.empty() ) ParseACLFile(i_file);
      
  }

  ~CSMIAuthList()
  {  if (_groups) free(_groups); }

  inline API_SEC_LEVEL GetSecurityLevel(csmi_cmd_t i_api_cmd) const
  {
    auto it = _api_to_grp_map.find(i_api_cmd);
    if ( it == _api_to_grp_map.end()) return PRIVILEGED;
    else return it->second;
  }
  
  // check if the i_uid/i_gid falls into the privileged user id/group
  inline bool HasPrivilegedAccess(uid_t i_uid, gid_t i_gid) const
  {
    // cases to check:
    // - root user is set and uid matches: true
    // - priv group is set and gid matches: true
    // - no more "*" since that would open the floodgates and grant everyone priv access
    if (findUid(i_uid) || findGid(i_gid)) return true;

    // if coming here, retrieve all the subgroups that the i_uid belongs...
    struct passwd *pw = getpwuid(i_uid);
    bool retval = false;
    if (pw)
    {
      int ngroups = _ngroups;
      if ( getgrouplist(pw->pw_name, pw->pw_gid, _groups, &ngroups) == -1 )
      {
        // in this case, we need to allocate a larger buffer to store the list
        gid_t* groups = (gid_t *) malloc(sizeof(gid_t)*ngroups);
        getgrouplist(pw->pw_name, pw->pw_gid, groups, &ngroups);
        retval = CheckGroups(groups, ngroups);
        free(groups);
        
      } else retval = CheckGroups(_groups, ngroups);
    }
    return retval;

  }
  
  /** brief Decide whether this API called by this user requires a check of user data or permission is granted
   *
   *  @param   cmd            command type, required to check the configured security level of the request
   *  @param   i_uid, i_gid   user and group ID of the caller to check for privileged access
   *
   *  @return  true      if calling handler needs to perform specific userID comparison to grant permission
   *           false     if no further action is needed and permission can be granted
   *
   *  @note This function excludes the case of a privileged API called by non-privileged user!
   *        It is not needed to perform this check because this case is already caught by the lower
   *        network layers.
   *        Explanation of cases:
   *
   *        API Seclevel:      Caller:        permission:
   *    -------------------------------------------------------
   *        public             any            granted
   *        private            admin          granted
   *        private            user           handler check required
   *        privileged         admin          granted
   *        privileged         user           denied (will never get here if network layer works!)
   *
   */
  inline bool PrivateRequiresUserCompare( const csmi_cmd_t i_cmd,
                                          const uid_t i_uid,
                                          const gid_t i_gid ) const
  {
    return ( ( GetSecurityLevel( i_cmd ) == csm::daemon::PRIVATE ) &&
        ( ! HasPrivilegedAccess( i_uid, i_gid ) ) );
  }

public:
  // for internal testing only
  inline API_SEC_LEVEL GetSecurityLevel(std::string i_api_name)
  {
    return GetSecurityLevel( csmi_cmd_get(i_api_name.c_str()) );
  }
  
};

typedef std::shared_ptr<CSMIAuthList> CSMIAuthList_sptr;

} // namespace daemon
} // namespace csm

#ifdef logprefix_local
#undef logprefix
#undef logprefix_local
#endif
#endif
