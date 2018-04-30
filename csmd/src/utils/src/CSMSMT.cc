/*================================================================================
   
    csmd/src/cgroup-utils/CSMSMT.cc

    © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
 
================================================================================*/
#include "csmd/src/daemon/src/csmi_request_handler/helpers/Agent.h"
#include <getopt.h>

struct option longopts[] = {
    {"help",       no_argument, 0, 'h'},
    {"smt" , required_argument, 0, 's'},
    {0, 0, 0, 0}
};

static void help()
{
    puts( "Usage: csm_smt --smt=[smt-level]");
    puts( "A CSM passthrough for the `ppc64_cpu --smt=[smt-level]`");
    puts( "Fixes the cgroups for the SMT mode change.");
}

int main(int argc, char *argv[])
{
    
    int opt, indexptr = 0, smt=-1;
    char* arg_test = nullptr;

    while ((opt = getopt_long(argc, argv, "hs:", longopts, &indexptr)) != -1)
    {
        switch (opt)
        {
            case 'h':
                help();
                return 0;
            case 's':
                smt=strtol( optarg, &arg_test, 10 );

                if ( *arg_test )
                {
                    printf("Invalid value for smt (numeric expected): %s\n", optarg); 
                    help();
                    return 1;
                }

                break;
            default:
                help();
                return 0;
        }
    }

    if(smt < 0 )
    {
        puts("SMT value must be specified and greater than zero.");
        help();
        return 1;
    }
    
//    csm::daemon::helper::CGroup cgroup1 = csm::daemon::helper::CGroup( 1 );
//    cgroup1.SetupCGroups( 0 );
//    cgroup1.ConfigSharedCGroup( 100, 0, 4 );
//
//    csm::daemon::helper::CGroup cgroup2 = csm::daemon::helper::CGroup( 2 );
//    cgroup2.SetupCGroups( 0 );
//    cgroup2.ConfigSharedCGroup( 100, 0, 6 );
//
//    csm::daemon::helper::CGroup cgroup = csm::daemon::helper::CGroup( 3 );
//    cgroup.SetupCGroups( 0 );
//    cgroup.ConfigSharedCGroup( 100, 0, 10 );
    

    // Set the SMT level
    printf("Setting SMT level to %d.\n", smt);
    int rc = csm::daemon::helper::SetSMTLevelCSM( smt );

    if (rc != 0 )
    {
        puts("SMT mode set was a failure, verify a valid level was used.");
    }

    return rc;
}
