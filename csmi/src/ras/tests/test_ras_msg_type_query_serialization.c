/*================================================================================

    csmi/src/ras/tests/test_ras_msg_type_query_serialization.c

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/


#include "csmi/include/csm_api_ras.h"
#include "csmi/src/common/include/csmi_serialization.h"
#include "../include/csmi_ras_msg_type_query_serialization.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>


static csmi_cmd_t test_cmd = CSM_CMD_ras_msg_type_query;


void test_ras_msg_type_query_arg(void) 
{

    csm_ras_msg_type_query_arg_t *argsIn = NULL;
    csm_ras_msg_type_query_arg_t *argsOut = NULL;
    csmi_cdata_t csmi_cdata;
    csmi_buf_t *packVal;
    csmi_cdata_t *unpackVal;
    packPrototype argPackFunc;
    unpackPrototype argUnpackFunc;


    argsIn = malloc(sizeof(*argsIn));
    memset(argsIn, 0, sizeof(*argsIn));
    argsIn->msg_id = "msg_id ";
    argsIn->suppress_ids = "suppress_ids";
    argsIn->severity = "severity";
    argsIn->control_action = "control_action";
    argsIn->message = "message";
    argsIn->limit = 100;
    argsIn->offset= 5;

    csmi_cdata.cdataLen = sizeof(*argsIn);
    csmi_cdata.cdata = argsIn;

    argPackFunc = csmi_argpack_get(test_cmd);
    argUnpackFunc = csmi_argunpack_get(test_cmd);
    assert(argPackFunc != NULL && argUnpackFunc != NULL);

    packVal = argPackFunc(test_cmd, &csmi_cdata);
    unpackVal = argUnpackFunc(test_cmd, packVal->buf, packVal->bufLen);
    argsOut = (csm_ras_msg_type_query_arg_t *) unpackVal->cdata;

    // check the results...
    assert(strcmp(argsIn->msg_id, argsOut->msg_id) == 0);
    assert(strcmp(argsIn->suppress_ids, argsOut->suppress_ids) == 0);
    assert(strcmp(argsIn->severity, argsOut->severity) == 0);
    assert(strcmp(argsIn->control_action, argsOut->control_action) == 0);
    assert(strcmp(argsIn->message, argsOut->message) == 0);

    assert(argsIn->limit == argsOut->limit);
    assert(argsIn->offset == argsOut->offset);

    // dispose of memory...
    free(packVal->buf);
    free(packVal);

   
    free(unpackVal->cdata);
    free(unpackVal);

    free(argsIn);


}


void test_ras_msg_type_query(void) 
{

    csm_ras_msg_type_vector_t *argsIn = NULL;
    csm_ras_msg_type_vector_t *argsOut = NULL;
    csmi_cdata_t csmi_cdata;
    csmi_buf_t *packVal;
    csmi_cdata_t *unpackVal;
    packPrototype packFunc;
    unpackPrototype unpackFunc;
    unsigned num_ras_msg_types;
    unsigned n;
    csm_ras_msg_type_t *ceventIn;


    num_ras_msg_types = 10;

#if 0
    const char *msg_id;
    const char *severity; 
    const char *time_stamp;
    const char *location_name;
    unsigned int processor;
    unsigned int count;
    const char *control_action;
    const char *message;
    const char *raw_data;
#endif

    argsIn = malloc(sizeof(*argsIn)+sizeof(argsIn->msg_types[0])*num_ras_msg_types);
    argsIn->num_ras_msg_types = num_ras_msg_types;
    for (n = 0, ceventIn = argsIn->msg_types; n < num_ras_msg_types; n++, ceventIn++) {
        ceventIn->msg_id = "msg_id";
        ceventIn->min_time_in_pool= 100+n;
        ceventIn->suppress_ids = "suppress_ids";
        ceventIn->severity = "severity";
        ceventIn->message = "message";
        ceventIn->decoder = "decoder";
        ceventIn->control_action = "control_action";
        ceventIn->description = "description";
        ceventIn->threshold_count = 200+n;
        ceventIn->threshold_period = "1H";
        ceventIn->relevant_diags = "relevant_diags";

    }

    csmi_cdata.cdataLen = sizeof(*argsIn);
    csmi_cdata.cdata = argsIn;

    packFunc = csmi_pack_get(test_cmd);
    unpackFunc = csmi_unpack_get(test_cmd);
    assert(packFunc != NULL && unpackFunc != NULL);

    packVal = packFunc(test_cmd, &csmi_cdata);
    unpackVal = unpackFunc(test_cmd, packVal->buf, packVal->bufLen);
    argsOut = (csm_ras_msg_type_vector_t *) unpackVal->cdata;

    // check the results...
    assert(argsIn->num_ras_msg_types == argsOut->num_ras_msg_types);
    for (n = 0; n < num_ras_msg_types; n++) {
        csm_ras_msg_type_t *ceventIn = argsIn->msg_types+ n;
        csm_ras_msg_type_t *ceventOut = argsOut->msg_types + n;

        assert(strcmp(ceventIn->msg_id, ceventOut->msg_id) == 0);
        assert(ceventIn->min_time_in_pool == ceventOut->min_time_in_pool);
        assert(strcmp(ceventIn->suppress_ids, ceventOut->suppress_ids) == 0);
        assert(strcmp(ceventIn->severity, ceventOut->severity) == 0);
        assert(strcmp(ceventIn->message, ceventOut->message) == 0);
        assert(strcmp(ceventIn->decoder, ceventOut->decoder) == 0);
        assert(strcmp(ceventIn->control_action, ceventOut->control_action) == 0);
        assert(strcmp(ceventIn->description, ceventOut->description) == 0);
        assert(ceventIn->threshold_count == ceventOut->threshold_count);;
        assert(strcmp(ceventIn->threshold_period,ceventOut->threshold_period) == 0);
        assert(strcmp(ceventIn->relevant_diags, ceventOut->relevant_diags) == 0);
    }

    // dispose of memory...
    free(packVal->buf);
    free(packVal);


    free(unpackVal->cdata);
    free(unpackVal);

    free(argsIn);


}


int main(int argc, char *argv[]) {
  // need to initialize the pack/unpack functions
  csmi_cmd_hdl_init();

  test_ras_msg_type_query_arg();
  test_ras_msg_type_query();
  printf("TEST PASSED\n");
  return 0;
}



