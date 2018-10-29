/*================================================================================
   
    csmi/src/common/src/csmi_common_serial.c

    © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html
    
    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
    
================================================================================*/
#include "csmi/include/csmi_type_common.h"

#undef STRUCT_DEF
#include "csmi/src/common/include/csm_serialization_x_macros.h"
#include "csmi/src/common/include/csmi_common_type_internal.h"
const char* csmi_cmd_err_t_strs [] = {"NO ERROR","An undefined error was detected.","A help functionality caused an early exit.","CSM Library not initialized","No results received","Timeout","Message Id mismatched","CSMI CMD mismatched","Missing required parameter","Invalid parameter or value",
"Ras handler exception","CSMI CMD Unknown To Daemon","Send or Recv Error","Memory error","Not defined error","PubSub error","CSMI permission denied","Script failure error","Can not connect to daemon","Can not disconnect from daemon",
"The Payload of a message was unexpectedly empty","Handler received incorrect event type","The Address type of a network message was unexpected","Indicates that a CGroup couldn't be deleted","Database error; can't connect","Database error; table in bad state","Number of deleted records is less than expected","Number of updated records is less than expected","Message packing error","Message unpack error",
"The String Buffer of a csmi_sendrecv_cmd message was unexpectedly empty","The Return Buffer of a csmi_sendrecv_cmd message was unexpectedly empty","The Return Buffer of a csmi_sendrecv_cmd message was unknown or corrupted","It was not possible to create the multicast message","Errors were found with the responses from the compute daemons","API has a bad permission level.","A generic error occurred modifying cgroups.","The handler context was lost in some way.","An invalid value was written to a cgroup parameter.","An illegal resource request occurred for the cgroup parameter.  ",
"Burst Buffer Command encountered a generic error.","The allocation jumped from staging-in to staging-out.","The allocation state transition was to the same state.","The allocation state transition failed to complete.","The allocation delete was performed in an illegal state.","Indicates JSRUN could not be started by the CSM infrastructure.","Nodes specified for allocation create were not in the database.","Nodes specified for allocation create were in use by other allocations.","Allocation create had nodes that were not available.","Allocation create had bad allocation flags.",
"Allocation couldn't find the allocation.",""};

const char* csmi_node_type_t_strs [] = {"no type","management","service","login","workload-manager","launch","compute","utility","aggregator",""};

const char* csmi_node_state_t_strs [] = {"undefined","DISCOVERED","IN_SERVICE","OUT_OF_SERVICE","ADMIN_RESERVED","SOFT_FAILURE","MAINTENANCE","CSM_DATABASE_NULL",""};

const char* csmi_ras_severity_t_strs [] = {"undefined","INFO","WARNING","FATAL",""};

