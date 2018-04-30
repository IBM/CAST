/*================================================================================

    csmd/src/daemon/tests/csm_api_acl_test.cc

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#include "include/csm_api_acl.h"
#include "csmi/src/common/include/csmi_cmds.h"

#include <iostream>

int main(int argc, char *argv[])
{

  if (argc < 2)
  {
    std::cerr << "Usage: " << argv[0] << " api_acl_file" << std::endl;
    exit(-1);
  }
  
  std::string file(argv[1]);
  
  csm::daemon::CSMIAuthList_sptr auth_list = std::make_shared<csm::daemon::CSMIAuthList>(file);
  
  // first, test the API group
  assert(auth_list->GetSecurityLevel(CSM_infrastructure_test) == csm::daemon::PUBLIC);
  assert(auth_list->GetSecurityLevel("csm_infrastructure_test") == csm::daemon::PUBLIC);
  assert(auth_list->GetSecurityLevel(CSM_CMD_allocation_query_details) == csm::daemon::PRIVATE);
  assert(auth_list->GetSecurityLevel("csm_allocation_query_details") == csm::daemon::PRIVATE);
  assert(auth_list->GetSecurityLevel((csmi_cmd_t) 100) == csm::daemon::PRIVILEGED);
  
  // now test the user/group id
  uid_t root_uid = getpwnam("root")->pw_uid;
  gid_t root_gid = getgrnam("root")->gr_gid;
  assert(auth_list->HasPrivilegedAccess(root_uid, root_gid) == true);
  
  struct passwd *uentry = getpwnam(getenv("USER"));
  struct group *gentry = getgrnam("users");
  if (uentry && gentry)
  {
    assert(auth_list->HasPrivilegedAccess(uentry->pw_uid, root_gid) == true);
    assert(auth_list->HasPrivilegedAccess(root_uid, gentry->gr_gid) == true);
    std::cout << "Group:users User:" << getenv("USER") << " HasPrivilegedAccess = " <<
      auth_list->HasPrivilegedAccess(uentry->pw_uid,gentry->gr_gid) << std::endl;
  }
}
