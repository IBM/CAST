/*================================================================================

    csmi/src/common/src/csmi_api_internal.c

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#include "csmi/src/common/include/csmi_api_internal.h"
#include "csmi/src/common/include/csm_serialization_x_macros.h"
#include "csmutil/include/csmutil_logging.h"
#include "csmi/include/csm_api_common.h"
#include <string.h>

csm_api_object * csm_api_object_new(csmi_cmd_t cmd, freePrototype free_func)
{
   /* Initialize an object and handle to Null. */
   csm_api_object *csm_obj=NULL;
   csmi_api_internal *csmi_hdl=NULL;

   /* EARLY RETURN: If memory can't be allocated for either the csm_obj or handle.
    * csm_obj is not initialized, the hdl is.
    */
   if ( (csm_obj = malloc(sizeof(csm_api_object))) == NULL) return NULL;
   
   if ( (csm_obj->hdl = calloc(1, sizeof(csmi_api_internal))) == NULL) {
     free(csm_obj);
     return NULL;
   }

   csmi_hdl = (csmi_api_internal *) csm_obj->hdl;

   /* Assign the supplied command and free function to the handle.*/
   csmi_hdl->cmd = cmd;
   csmi_hdl->csmi_free_func = free_func;

   // FIXME Should the Error Code get set here?
   // initialize errmsg for CSMI_SUCCESS
   csmi_hdl->errmsg = strdup(csm_get_string_from_enum(csmi_cmd_err_t,CSMI_SUCCESS));
 
   return csm_obj;
}

void csm_api_object_set_retdata(csm_api_object *csm_obj, int ret_data_size, void *ret_data)
{
  csmi_api_internal *csmi_hdl;

  if (csm_obj == NULL || csm_obj->hdl == NULL) {
    csmutil_logging(error, "csmi_api_object not valid");
    return;
  }
  csmi_hdl = (csmi_api_internal *) csm_obj->hdl;
  csmi_hdl->ret_cdata_size = ret_data_size;
  csmi_hdl->ret_cdata = ret_data;
}

void csmi_api_handler_destroy(csmi_api_internal *csmi_hdl)
{
  if (csmi_hdl == NULL) {
    csmutil_logging(warning, "csmi_hdl not valid");
    return;
  }
  if (csmi_hdl->errmsg != NULL) free(csmi_hdl->errmsg);

  free(csmi_hdl);
}

void csm_api_object_errcode_set(csm_api_object *csm_obj, int errcode)
{
  if (csm_obj == NULL || csm_obj->hdl == NULL) {
    csmutil_logging(error, "csmi_api_object not valid");
    return;
  }
   csmi_api_internal *csmi_hdl;
   csmi_hdl = (csmi_api_internal *) csm_obj->hdl;
   csmi_hdl->errcode = errcode;
}

void csm_api_object_errmsg_set(csm_api_object *csm_obj, char *msg)
{
  if (csm_obj == NULL || csm_obj->hdl == NULL) {
    csmutil_logging(error, "csmi_api_object not valid");
    return;
  }
   csmi_api_internal *csmi_hdl;
   csmi_hdl = (csmi_api_internal *) csm_obj->hdl;

   // free the outdated errmsg
   if (csmi_hdl->errmsg) free(csmi_hdl->errmsg);
   csmi_hdl->errmsg = msg;
}

void csm_api_object_trace_set(csm_api_object *csm_obj, uint32_t traceId)
{
  if (csm_obj == NULL || csm_obj->hdl == NULL) {
    csmutil_logging(error, "csmi_api_object not valid");
    return;
  }
  
  // Set the new trace id.
  csmi_api_internal *csmi_hdl;
  csmi_hdl = (csmi_api_internal *) csm_obj->hdl;
  csmi_hdl->traceid = traceId;
}

