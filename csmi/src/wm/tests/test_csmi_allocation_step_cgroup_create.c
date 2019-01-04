/*================================================================================

    csmi/src/wm/tests/test_csmi_allocation_step_cgroup_create.c

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "csmi/include/csm_api_workload_manager.h"
#include <getopt.h>

#include <assert.h>
struct option longopts[] = {
	{"help",             no_argument,       0, 'h'},
	{"allocation_id", required_argument, 0, 'a'},
	{"step_id",       required_argument, 0, 's'}
};

int csmi_client(int argc, char *argv[])
{
    int                opt;
    int              retval = 0;
    uint64_t  allocation_id = 0;
    char*              step = 0;
    int                indexptr = 0;

    //pid_t            pid = -1;
    csm_api_object *csm_obj = NULL;
    csm_allocation_step_cgroup_create_input_t* cgroup_args = NULL;
    
    while ((opt = getopt_long(argc, argv, "hs:a:", longopts, &indexptr)) != -1)
    {
        switch (opt)
        {
            case 'h':
                puts("-a: allocation id, -s: step cgroup name");
                return 1;
                break;
            case 'a':
                allocation_id =  atoi(optarg);
                break;
            case 's':
                step = strdup(optarg);
                break;
            default:
                return 1;
        }
    }
    
    csm_init_struct_ptr( csm_allocation_step_cgroup_create_input_t, cgroup_args );

    cgroup_args->allocation_id = -1;
    //cgroup_args->pid           = pid;
    cgroup_args->num_components= 1;
    
    cgroup_args->components = calloc( 0, cgroup_args->num_components * sizeof(csm_allocation_step_cgroup_create_input_t*));

    cgroup_args->components[0] = malloc(sizeof(csm_allocation_step_cgroup_create_input_t));
    cgroup_args->components[0]->type = CG_MEMORY;

    cgroup_args->components[0]->num_params = 1;
    cgroup_args->components[0]->params = malloc(sizeof(char*) * cgroup_args->components[0]->num_params);
    cgroup_args->components[0]->values = malloc(sizeof(char*) * cgroup_args->components[0]->num_params);
    
    cgroup_args->components[0]->params[0] = strdup("cgroup.clone_children");
    cgroup_args->components[0]->values[0] = strdup("1");


    assert (csm_init_lib() == 0);
    free(step);

    printf("\ncsmi_allocation_step_cgroup_create:\n");

    // ========================================================================
    
    //// Update the parent.
    //retval = csm_allocation_step_cgroup_create( &csm_obj, cgroup_args );

    //if (retval != 0) 
    //    printf("%s FAILED: errcode=%d errmsg=\"%s\"\n", 
    //        argv[0], csm_api_object_errcode_get(csm_obj),
    //        csm_api_object_errmsg_get(csm_obj));
    //csm_api_object_destroy(csm_obj);

    // ========================================================================
    cgroup_args->allocation_id = allocation_id;

    //// Update the parent.
    //retval = csm_allocation_step_cgroup_create( &csm_obj, cgroup_args );

    //if (retval != 0) 
    //    printf("%s FAILED: errcode=%d errmsg=\"%s\"\n", 
    //        argv[0], csm_api_object_errcode_get(csm_obj),
    //        csm_api_object_errmsg_get(csm_obj));
    //csm_api_object_destroy(csm_obj);

    // ========================================================================

    // Now update the child.
    cgroup_args->cgroup_name   = strdup(step);
    retval = csm_allocation_step_cgroup_create( &csm_obj, cgroup_args );

    if (retval != 0) 
        printf("%s FAILED: errcode=%d errmsg=\"%s\"\n", 
            argv[0], csm_api_object_errcode_get(csm_obj),
            csm_api_object_errmsg_get(csm_obj));
    // ========================================================================
    
    if (step) free(step);

    // It's the csmi library's responsibility to free internal space
    csm_free_struct_ptr(csm_allocation_step_cgroup_create_input_t, cgroup_args );

    assert( csm_term_lib() == 0);

    return retval;
}
