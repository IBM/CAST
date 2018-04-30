/*================================================================================

    csmi/src/inv/tests/test_csmi_cmd_line.c

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
#include "csmi/include/csm_api_inventory.h"

#include <assert.h>


int csmi_client(int argc, char *argv[])
{
	
#if 0
	
	int retval;
	csm_api_object *csm_obj = NULL;

	assert(csm_init_lib() == 0);
	
	uint32_t nodeCount = 0;
	
	puts("What is the node count? (aka. How many nodes?)");
	scanf("%i", &nodeCount);
	
	csm_node_attributes_update_t **nodeAttributeList;
	nodeAttributeList = calloc(nodeCount, sizeof(csm_node_attributes_update_t*));
	
	char nodeName[80];
	char ready;
	char state[CSM_STATE_MAX];
	
	uint32_t i = 0;
	
	for(i = 0; i < nodeCount; i++)
	{
		getchar();
		puts("WAITING ON INPUT: nodeName,ready,state");
		scanf("%79[^,],%c,%31[^\n]", nodeName, &ready, state);
		nodeAttributeList[i] = create(nodeName, ready, state);
		printf("nodeAttributeList[%i]->nodeName: %s\n", i, nodeAttributeList[i]->nodeName);
		printf("nodeAttributeList[%i]->ready: %c\n", i, nodeAttributeList[i]->ready);
		printf("nodeAttributeList[%i]->state: %s\n", i, nodeAttributeList[i]->state);
	}

	retval = csm_node_attributes_update(&csm_obj, nodeAttributeList, &nodeCount);
	
	for(i = 0; i < nodeCount; i++)
	{
		release(nodeAttributeList[i]);
	}
	free(nodeAttributeList);
	
	if (retval != 0){
		printf("\terrcode=%d errmsg=\"%s\"\n", csm_api_object_errcode_get(csm_obj), csm_api_object_errmsg_get(csm_obj));
	}

	assert(csm_term_lib() == 0);
#endif
	
	return 0;
}
