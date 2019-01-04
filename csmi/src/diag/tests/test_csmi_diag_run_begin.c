/*================================================================================

    csmi/src/diag/tests/test_csmi_diag_run_begin.c

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
    /*
	puts("############");
	puts("#Begin code inside 'csmi_client' of 'test_csmi_diag_run_begin.c'");
	
	csm_api_object *csm_obj = NULL;
	assert(csm_init_lib() == 0);
	
	//Set up test data
	csm_diag_run_begin_input_t runBeginData;
	runBeginData.run_id = 1;
	runBeginData.allocation_id = 0;
	runBeginData.log_dir = "~/myCoolFiles/log.txt";
	runBeginData.cmd_line = "myProgram.py";
	
	puts("#about to call csm_diag_run_begin");
	puts("#####");
	//Calling the actual API
	int returnValue = 0;
	returnValue = csm_diag_run_begin(&csm_obj, &runBeginData);
	if(returnValue != 0){
		printf("\terrcode=%d errmsg=\"%s\"\n", csm_api_object_errcode_get(csm_obj), csm_api_object_errmsg_get(csm_obj));
	}
		
	puts("#####");
	puts("#called csm_diag_run_begin");
	
	assert( csm_term_lib() == 0);
	puts("############");
	puts("#End code inside 'csmi_client' of 'test_csmi_diag_run_begin.c'");
	puts("############");
	
	return 0;
    */
}
