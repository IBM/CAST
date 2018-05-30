/*================================================================================

    csmi/src/wm/tests/test_csmi_sample.c

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

/// The API Header for workload management, contains the function, enum and function definitions.
#include "csmi/include/csm_api_workload_manager.h"

#include <time.h>
#include <sys/time.h>

#define HOSTNAME_1 "c931f02p18-vm05"
#define HOSTNAME_2 "c931f02p18-vm06"

int csmi_client(int argc, char *argv[])
{
    csm_init_lib(); ///< Initializes the connection to the Backend.

    /// ========================================================================================
    /// =                                csm_allocation_create                                 =
    /// ========================================================================================
    /// Example of an API invocation that takes a struct malloc'd by the user as input and reuses 
    /// the struct for output. The user is repsonsible for struct destruction in this case. 
    /// 'csm_api_object_destroy' must be invoked to clean up the csm_obj. 

    csmi_allocation_t *allocation; ///< A Struct for holding an allocation. csmi/include/csm_apis.h
    csm_api_object   *csm_obj;    ///< An object for holding error codes and returned data.

    /*
     * The following macro is defined in csmi/include/csm_api_helper_macros.h.
     * It will allocate the struct on the heap and then set the contents of the 
     * struct to the default values. These defaults are null pointers for 
     * components of pointer types (ie strings).
     * Using this macro is not required, but recommended if using the free macro.
     */
    csm_init_struct_ptr(csmi_allocation_t, allocation); 
    
    /* 
     * Initialize the _metadata field to be the API version code
     * in /include/csm_api_version.h. If the struct is initialized with the above 
     * csm_init_struct_ptr this step is automatically performed.
     */
    csm_init_struct_versioning(allocation);

    /// Set the contents of the allocation for a creation.
    /// Mandatory for allocation creation. 
    allocation->primary_job_id = 1;     

    /// Mandatory for allocation creation. 
    allocation->secondary_job_id = 0;   
    
    /// Mandatory for allocation creation. strdups are needed for the csm_free_struct_ptr macro.
    allocation->state  = CSM_RUNNING;

    /// This is set by csm_init_struct_ptr, but if the macro is not invoked this must be manually set.
    allocation->shared = 0;             

    /// As the compute_nodes member is allocated through a malloc the API user is responsible for 
    /// freeing it. If csm_init_struct_ptr or csm_init_struct was used, csm_free_struct_ptr or csm_free_struct 
    /// will release anything malloc'd to this member.
    allocation->num_nodes = 2;
    allocation->compute_nodes = 
        (char **) malloc(sizeof(char *) * allocation->num_nodes);
    allocation->compute_nodes[0] = strdup(HOSTNAME_1);
    allocation->compute_nodes[1] = strdup(HOSTNAME_2);

    /// Sets up the time.
    time_t             t;
    struct tm         *tm;
    char               tbuf[32];
    time(&t);
    tm = localtime(&t);
    strftime(tbuf, sizeof(tbuf), "%F %T", tm);
    allocation->job_submit_time = strdup(tbuf);

    /// Invokes the csm_allocation_create csm api.
    int ret_val = csm_allocation_create( &csm_obj, allocation ); 
    
    /// If the return value was not zero an error event was recieved.
    if( ret_val )
    {
        /// Print out the error code and error message 
        printf("%s FAILED: errcode=%d errmsg=\"%s\"\n", 
            argv[0], 
            csm_api_object_errcode_get(csm_obj),    /// Syntax for retrieving the error code.
            csm_api_object_errmsg_get(csm_obj) );   /// Syntax for retrieving the error message.

        /// Destroy the csm_api object now that we've extracted what we need.
        csm_api_object_destroy(csm_obj);

        /// Free the allocation struct and any malloc'd members.
        /// If this is invoked csm_init_struct_ptr should have been called first.
        csm_free_struct_ptr( csmi_allocation_t, allocation );

        /// Exit early, because the other portions of this code are impossible to execute without 
        /// a successful create.
        return ret_val;
    }

    /// Destroy the csm_api object to prevent leaks.
    csm_api_object_destroy(csm_obj);

    /// Outputs the allocation id to verify that the create populated the allocation struct 
    /// (if this is zero something has gone wrong).
    printf("\tallocation_id: %" PRIu64 "\n", allocation->allocation_id);


    /// ========================================================================================
    /// =                                csm_allocation_query                                  =
    /// ========================================================================================
    /// Example of an API invocation that takes a pointer to a struct which is populated by the 
    /// API. The API is responsible for destroying this struct through 'csm_api_object_destroy'.
    

    /// A separate pointer to supply to the allocation query, this allocation will be malloc'd 
    /// and populated by the C API.
    csm_allocation_query_output_t *alloc_query_out; 

    csm_allocation_query_input_t input;
    input.allocation_id    = allocation->allocation_id,    
    input.primary_job_id   = allocation->primary_job_id,   
    input.secondary_job_id = allocation->secondary_job_id,  

    
    /// Invoke the csm_allocation_query api.
    ret_val = csm_allocation_query( 
        &csm_obj, 
        &input,
        &alloc_query_out );

    /// Print the results of the query, good or bad.
    if ( !ret_val ) 
    {
        /// Output the contents of the query.
        uint32_t i;
        csmi_allocation_t *allocation_query = alloc_query_out->allocation;

        printf("\nallocation_id: %" PRIu64 "\n", allocation_query->allocation_id);
        printf("num_nodes: %" PRIu32 "\n",       allocation_query->num_nodes);
        printf("user_name: %s\n",                allocation_query->user_name);
        printf("state: %s\n",                    csm_get_string_from_enum(csmi_state_t,allocation_query->state));
        printf("job_submit_time: %s\n\n",        allocation_query->job_submit_time);
        for (i = 0; i < allocation_query->num_nodes; i++) 
            printf("- compute_nodes: %s\n", allocation_query->compute_nodes[i]);
    }
    else
        printf("%s FAILED: errcode=%d errmsg=\"%s\"\n", 
            argv[0], 
            csm_api_object_errcode_get(csm_obj),
            csm_api_object_errmsg_get(csm_obj) );
        

    /// If a csm_api mallocs a struct invoke destroy to clean it up.
    /// This should free the contents of alloc_query_out and the other contents of the csm_obj.
    csm_api_object_destroy(csm_obj); ///< Destroy the allocation generated by the query. 

    /// ========================================================================================
    /// =                               csm_allocation_delete                                  =
    /// ========================================================================================
    /// Example of an API invocation that performs no mallocs.'csm_api_object_destroy' must be 
    /// invoked to clean up the csm_obj. 
    
    csm_allocation_delete_input_t delete_input;
    delete_input.allocation_id = allocation->allocation_id;

    /// Invokes the csm_allocation_delete api. 
    /// This API generates no structs, nor does it modify any.
    ret_val = csm_allocation_delete( &csm_obj, &delete_input );

    /// If the allocation was deleted notify the user.
    /// Else print the error received.
    if( !ret_val )
        printf("\tallocation_id: %" PRIu64 " was Deleted\n", delete_input.allocation_id);
    else
        printf("%s FAILED: errcode=%d errmsg=\"%s\"\n", 
            argv[0], 
            csm_api_object_errcode_get(csm_obj),
            csm_api_object_errmsg_get(csm_obj) );

    /// Cleanup the csm_api object. This needs to be invoked regardless of whether the API 
    /// performs any mallocs.
    csm_api_object_destroy(csm_obj);

    /// ========================================================================================
    /// =                                       Cleanup                                        =
    /// ========================================================================================

    /// The API user is responsible for cleaning up their structs.
    /// Any members that were malloc'd will need to be freed by the programmer.
    /// csm_free_struct_ptr should be used if csm_init_struct_ptr was used. 
    /// A similar parity exists between csm_free_struct and csm_init_struct. 
    csm_free_struct_ptr( csmi_allocation_t, allocation );

    csm_term_lib(); ///< Terminate the connection to the Backend.

    return ret_val;
}

