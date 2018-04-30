/*================================================================================

    csmi/src/common/src/csmi_cmds.c

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#include "csmi/src/common/include/csmi_cmds.h"
#include "csmnet/include/csm_timing.h"

#define STR1(x) #x
#define STR(x) STR1(x)

const char* csmi_cmds_t_strs[] = {
  #define cmd(n) STR(CSM_CMD_##n),
  #include "csmi/src/common/include/csmi_cmds_def.h"
  #undef cmd
  
  #define cmd(n) STR(n),
  #include "csmi/src/common/include/csm_cmds_internal_def.h"
  #undef cmd

  "CSM_CMD_MAX",
  "CSM_CMD_INVALID"
};

int csmi_cmd_timeouts[] = {
  #define cmd(n) CSM_RECV_TIMEOUT_SECONDS,
  #include "csmi/src/common/include/csmi_cmds_def.h"
  #undef cmd

  #define cmd(n) CSM_RECV_TIMEOUT_SECONDS,
  #include "csmi/src/common/include/csm_cmds_internal_def.h"
  #undef cmd

    CSM_RECV_TIMEOUT_SECONDS, // "CSM_CMD_MAX"
    CSM_RECV_TIMEOUT_SECONDS  //"CSM_CMD_INVALID"
};

#undef STR
#undef STR1
