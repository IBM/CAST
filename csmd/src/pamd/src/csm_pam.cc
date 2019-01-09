/*================================================================================

    csmd/src/pamd/src/csm_pam.cc

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#define PAM_SM_SESSION

#include <security/pam_modules.h>
#include <string.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <sys/types.h>
#include <pwd.h>
#include <unistd.h>
#include "csmi/include/csm_api_workload_manager.h"
#include "csmutil/include/csmutil_logging.h"

#define WHITELIST "/etc/pam.d/csm/whitelist"
#define NO_CG "CSM_NO_CGROUP"
#define ALLOC "CSM_ALLOCATION_ID"
#define SUPER_USER "root"
#define DELIM ";"

// FORMAT
int check_users(const char* userName, char migrate_pid)
{
    // 0. Check if root (early return).
    if ( strcmp(SUPER_USER,userName) == 0 )
        return PAM_SUCCESS;

    // Disable logging.
    csmutil_logging_level_set((char*)"off");

    // 1. Check the active list if the NO_CG flag is not set.
    csm_init_lib();
    csm_api_object   *csm_obj = NULL;

    // Construct payload.
    csm_cgroup_login_input_t input;
    input.user_name     = strdup(userName);
    input.pid           = getpid();
    input.allocation_id = -1;
    input.migrate_pid = migrate_pid;

    // Execute login attempt.
    int errCode = csm_cgroup_login(&csm_obj, &input);

    // Cleanup
    free(input.user_name);
    csm_api_object_destroy(csm_obj);
    csm_term_lib();
    
    // If the login was a success return as a success.
    if (errCode == CSMI_SUCCESS)
        return PAM_SUCCESS;

    // 2. Load the whitelist users IFF the user 
    std::ifstream whitelistStream(WHITELIST);
    if( whitelistStream.is_open() )
    {
        std::string user ( userName );
        std::string line;
        while ( std::getline( whitelistStream, line ) )
        {
            // If the user is in the whitelist dump them to the base cgroup.
            if ( user.compare(line) == 0 )
                return PAM_SUCCESS;
        }
    }

    return PAM_PERM_DENIED;
}

PAM_EXTERN int pam_sm_open_session( pam_handle_t *pamh, int flags, int argc, const char **argv)
{
    const char* user_name;
    pam_get_user(pamh, &user_name, "Username: ");
    check_users(user_name, 1);
    return PAM_SUCCESS;
}

PAM_EXTERN int pam_sm_close_session( pam_handle_t *pamh, int flags, int argc, const char **argv)
{
    return PAM_SUCCESS;
}

PAM_EXTERN int pam_sm_acct_mgmt( pam_handle_t *pamh, int flags, int argc, const char **argv)
{
    const char* user_name;
    pam_get_user(pamh, &user_name, "Username: ");
    return check_users(user_name, 0);
}
