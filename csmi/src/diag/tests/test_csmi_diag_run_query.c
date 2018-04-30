/*================================================================================

    csmi/src/diag/tests/test_csmi_diag_run_query.c

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
/*
* Author: Nick Buonarota
* Email: nbuonar@us.ibm.com
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "csmi/include/csm_api_diagnostics.h"

#include <assert.h>

int csmi_client(int argc, char *argv[])
{
	// puts("############");
	// puts("#Begin code inside 'csmi_client' of 'test_csmi_diag_run_query.c'");
	
	// /*Set up test data*/
	// uint64_t run_id = 1;
	// csmi_diag_run_t runData;
	// uint32_t dataCount = 0;
	
	// puts("#about to call csm_diag_run_query");
	// puts("#####");
	// /*create csm object*/
	// csm_api_object *csm_obj = NULL;
	// /*establish connection*/
	// assert(csm_init_lib() == 0);
	// /*Calling the actual API*/
	// int returnValue = 0;
	// returnValue = csm_diag_run_query(&csm_obj, run_id, &runData, &dataCount);
	// if(returnValue != 0){
		// printf("\terrcode=%d errmsg=\"%s\"\n", csm_api_object_errcode_get(csm_obj), csm_api_object_errmsg_get(csm_obj));
	// }
	// /* terminate connection */
	// assert( csm_term_lib() == 0);
	// puts("#####");
	// puts("#called csm_diag_run_query");
	
	// puts("#####");
	// puts("# About to print out runData contents:");
	// printf("# runData.history_time: %s \n", runData.history_time);
	// printf("# runData.run_id: %lu \n", runData.run_id);
	// printf("# runData.allocation_id: %lu \n", runData.allocation_id);
	// printf("# runData.begin_time: %s \n", runData.begin_time);
	// printf("# runData.end_time: %s \n", runData.end_time);
	// printf("# runData.diag_status: %s \n", runData.diag_status);
	// printf("# runData.inserted_ras: %c \n", runData.inserted_ras);
	// printf("# runData.log_dir: %s \n", runData.log_dir);
	// printf("# runData.cmd_line: %s \n", runData.cmd_line);
	
	// puts("############");
	// puts("#End code inside 'csmi_client' of 'test_csmi_diag_run_query.c'");
	// puts("############");
	
	return 0;
}
