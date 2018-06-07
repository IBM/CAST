/*================================================================================

    csmi/src/inv/tests/test_csmi_node_attributes_update.c

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
	puts("############");
	puts("#Begin code inside 'csmi_client' of 'test_csmi_node_attributes_update.c'");
	
#if 0
	/*Comment out these variables for now because they are not used and it throws an error in the compile.*/
	
	int retval;
	//uint32_t i;
	//int nodeCount;
	//csm_node_attributes_update_t *nodeAttributes;
	//char **nodeList;
	csm_api_object *csm_obj = NULL;

	assert (csm_init_lib() == 0);

	/******************
	** test 1        **
	******************/
	/*
	puts("############");
	puts("###TEST 1###");
	puts("############");
	
	retval = csm_node_attributes_update_get_all(&csm_obj, &nodeCount, &nodeAttributes);
	printf("csm_node_attributes_update_get_all:\n");
	for (i=0; retval == 0 && i<nodeCount; i++) {
		printf("\tname:%s cpus:%d\n", nodeAttributes[i].nodeName, nodeAttributes[i].numCpus);
	}

	if (retval != 0) printf("\terrcode=%d errmsg=\"%s\"\n", csm_api_object_errcode_get(csm_obj), csm_api_object_errmsg_get(csm_obj));

	// it's the csmi library's responsility to free nodeAttributes and other internal space
	csm_api_object_destroy(csm_obj);
	*/

	/******************
	** test 2        **
	******************/
	/*
	puts("############");
	puts("###TEST 2###");
	puts("############");
	printf("\ncsm_node_attributes_update_get:\n");
	nodeList = (char **) malloc(sizeof(char *)*1);
	nodeList[0] = strdup("node2");
	retval = csm_node_attributes_update_get(&csm_obj, 1, nodeList, &nodeCount, &nodeAttributes);

	for (i=0; retval == 0 && i<nodeCount; i++) {
		printf("\tname:%s cpus:%d\n", nodeAttributes[i].nodeName, nodeAttributes[i].numCpus);
	}

	if (retval != 0) printf("\terrcode=%d errmsg=\"%s\"\n", csm_api_object_errcode_get(csm_obj), csm_api_object_errmsg_get(csm_obj));

	// it's the csmi library's responsility to free nodeAttributes and other internal space
	csm_api_object_destroy(csm_obj);

	// it's the csmi client's responsibility to free nodeList
	for (i=0; i<1;i++) free(nodeList[i]);
	free(nodeList);
	*/

	/******************
	** test 3        **
	******************/
	/*
	puts("############");
	puts("###TEST 3###");
	puts("############");
   
	// test the corner cases
	printf("\ncsm_node_attributes_update_get (unknown node name): <= shall return empty\n");

	nodeList = (char **) malloc(sizeof(char *)*1);
	nodeList[0] = strdup("node3");
	retval = csm_node_attributes_update_get(&csm_obj, 1, nodeList, &nodeCount, &nodeAttributes);

	for (i=0; retval == 0 && i<nodeCount; i++) {
		printf("\tname:%s cpus:%d\n", nodeAttributes[i].nodeName, nodeAttributes[i].numCpus);
	} 
	if (retval != 0) printf("\terrcode=%d errmsg=\"%s\"\n", csm_api_object_errcode_get(csm_obj), csm_api_object_errmsg_get(csm_obj));

	if (nodeCount <= 0) {
		printf("\tEmpty from DB!\n");
	}

	// it's the csmi library's responsility to free nodeAttributes and other internal space
	csm_api_object_destroy(csm_obj);

	// it's the csmi client's responsibility to free nodeList
	for (i=0; i<1;i++) free(nodeList[i]);
	free(nodeList);
	*/

	/******************
	** test 4        **
	******************/
	/*
	puts("############");
	puts("###TEST 4###");
	puts("############");
	// test the corner cases
	printf("\ncsm_node_attributes_get (invalid node selection): <= shall select all by default\n");
	retval = csm_node_attributes_update_get(&csm_obj, 0, NULL, &nodeCount, &nodeAttributes);
	for (i=0; retval == 0 &&  i<nodeCount; i++) {
		printf("\tname:%s cpus:%d\n", nodeAttributes[i].nodeName, nodeAttributes[i].numCpus);
	}

	if (retval != 0) printf("\terrcode=%d errmsg=\"%s\"\n", csm_api_object_errcode_get(csm_obj), csm_api_object_errmsg_get(csm_obj));

	// it's the csmi library's responsility to free nodeAttributes and other internal space
	csm_api_object_destroy(csm_obj);
	*/
  
	puts("###########################################");
	puts("###TEST 1##################################");
	puts("###TESTING: csm_node_attributes_update()###");
	puts("###########################################");
	
	
	/*For now I manually add in another node to the 'csm_node' table.*/
	/* nodeName: n10 ready: n*/
	
	/*So if you go to mng node and run a 'SELECT * FROM csm_node;'*/
	/*Then you will see a record with: node_name: 'n10' ready: 'n' in the last entry*/
	
	/*If not, then update, delete, or add via sql to that table to set this test up.*/
	
	/*This test 5, will try to send a nodeUpdate struct through the api and update that entry.*/
	
	
	/*Create the test struct and populate with data*/
	csm_node_attributes_update_t myTestData;
	puts("#csm_node_attributes_update_t myTestData created");
	myTestData.nodeName = "n10";
	myTestData.ready = CSM_NODE_READY_YES;
	strncpy(myTestData.state, CSM_NODE_STATE_DISCOVERED,CSM_STATE_MAX);
	myTestData.state[CSM_STATE_MAX-1] = '\0';
	myTestData.physical_frame_location = "myFrame01";
	myTestData.physical_u_location = "myU01";
	myTestData.feature_1 = "myFeature01";
	myTestData.feature_2 = "myFeature02";
	myTestData.feature_3 = "myFeature03";
	myTestData.feature_4 = "myFeature04";
	myTestData.comment = "my Cool comment 01";
	puts("#myTestData set with nodeName n10, ready y, and state in service");
	
	/*Create the test struct and populate with data*/
	/*to test multiple structs*/
	csm_node_attributes_update_t myTestData2;
	puts("#csm_node_attributes_update_t myTestData2 created");
	myTestData2.nodeName = "n09";
	myTestData2.ready = CSM_NODE_READY_NO;
	strncpy(myTestData2.state, CSM_NODE_STATE_OUT_OF_SERVICE, CSM_STATE_MAX);
	myTestData2.state[CSM_STATE_MAX-1] = '\0';
	myTestData2.physical_frame_location = NULL;
	myTestData2.physical_u_location = NULL;
	myTestData2.feature_1 = NULL;
	myTestData2.feature_2 = NULL;
	myTestData2.feature_3 = NULL;
	myTestData2.feature_4 = NULL;
	myTestData2.comment = NULL;
	printf("#myTestData2.state: %.32s\n", myTestData2.state);
	puts("#myTestData2 set with nodeName n09, ready y, and state in service");
	
	/*create an array of structs*/
	/*place the test structs in this array*/
	csm_node_attributes_update_t *myTestNodeList[2];
	puts("#csm_node_attributes_update_t myTestNodeList created");
	myTestNodeList[0] = &myTestData;
	puts("#myTestData inserted into myTestNodeList");
	myTestNodeList[1] = &myTestData2;
	puts("#myTestData2 inserted into myTestNodeList");
	
	uint32_t nodeCount = 2;
	
	puts("#about to call csm_node_attributes_update");
	puts("#####");
	/*call node attributes update with our test array of node attribute structs*/
	retval = csm_node_attributes_update(&csm_obj, myTestNodeList, &nodeCount);
	puts("#####");
	puts("#called csm_node_attributes_update");
	
	if (retval != 0) printf("\terrcode=%d errmsg=\"%s\"\n", csm_api_object_errcode_get(csm_obj), csm_api_object_errmsg_get(csm_obj));
	
	/*When this is done, go back to mng node. And run a 'SELECT * FROM nodes;'*/
	/*At this point if everything went right. It should say 'y' for ready instead of 'n'.*/

	assert( csm_term_lib() == 0);
	puts("############");
	puts("#End code inside 'csmi_client' of 'test_csmi_node_attributes_update.c'");
	puts("############");
#endif
	return 0;
}
