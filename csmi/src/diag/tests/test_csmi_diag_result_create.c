/*================================================================================

    csmi/src/diag/tests/test_csmi_diag_result_create.c

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
	puts("############");
	puts("#Begin code inside 'csmi_client' of 'test_csmi_diag_result_create.c'");
	
	csm_api_object *csm_obj = NULL;
	assert(csm_init_lib() == 0);
	
	/*Set up test data*/
	csm_diag_result_create_input_t myResultData;
	myResultData.run_id = 1;
	myResultData.test_name = "My test case.";
	myResultData.node_name = "myNode";
	myResultData.serial_number = "mySerialNumber";
	myResultData.begin_time = "1999-01-27 13:37:33.134317";
	strncpy(myResultData.status, "success",16);
	myResultData.status[15] = '\0';
	myResultData.log_file = "myLogFile";
	
	puts("#about to call csm_diag_result_create");
	puts("#####");
	/*Calling the actual API*/
	int returnValue = 0;
	returnValue = csm_diag_result_create(&csm_obj, &myResultData);
	if(returnValue != 0){
		printf("\terrcode=%d errmsg=\"%s\"\n", csm_api_object_errcode_get(csm_obj), csm_api_object_errmsg_get(csm_obj));
	}
		
	puts("#####");
	puts("#called csm_diag_result_create");
	
	assert( csm_term_lib() == 0);
	puts("############");
	puts("#End code inside 'csmi_client' of 'test_csmi_diag_result_create.c'");
	puts("############");
	
	return 0;
}
