/*================================================================================

    csmi/src/inv/tests/test_csmi_node_attributes_query_history.c

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
#include <stdint.h>
#include <inttypes.h>

#include "csmi/include/csm_api_inventory.h"

/*For printing through the infrastructure*/
#include "csmutil/include/csmutil_logging.h"

#include <assert.h>

int csmi_client(int argc, char *argv[])
{
	csmutil_logging_level_set("trace");
	
	csmutil_logging(trace, "%s-%d:", __FILE__, __LINE__);
	csmutil_logging(trace, "  Begin code in test_csmi_node_attributes_query_history");
	
	/*Helper variables*/
	/*For for loops*/
	//uint32_t i = 0;
	
	//csm_api_object *csm_obj = NULL;

	assert (csm_init_lib() == 0);
	
	//todo: test case

	assert( csm_term_lib() == 0);
	
	csmutil_logging(trace, "%s-%d:", __FILE__, __LINE__);
	csmutil_logging(trace, "  End code in test_csmi_node_attributes_query_history");
	
	return 0;
}
