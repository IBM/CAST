/*================================================================================

    csmi/src/common/include/csmi_cmds.h

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#ifndef CSMI_CMDS_H_
#define CSMI_CMDS_H_

// todo: (this is just the temporary home of the enum) needs to be moved/integrated into csmi component

/** @defgroup csm_csmi_cmds_t csm_csmi_cmds_t*/
/**@ingroup csm_csmi_cmds_t
 * @brief An enumerated type describing the commands for csm.
 */
enum csm_csmi_cmds_t {
  #define cmd(n) CSM_CMD_##n,
  #include "csmi/src/common/include/csmi_cmds_def.h"
  #undef cmd

  #define cmd(n) n,
  #include "csmi/src/common/include/csm_cmds_internal_def.h"
  #undef cmd
  
  CSM_CMD_MAX,
  CSM_CMD_INVALID
};

#ifdef __cplusplus
extern "C" {
#endif
/** @ingroup csm_csmi_cmds_t
 * @brief Maps the commands to their equivalent strings.
 * Defined in csmi_cmds.c.
 */
extern const char* csmi_cmds_t_strs [];
#define csmi_cmds_to_str( cmd ) ( (cmd) < CSM_CMD_MAX ? csmi_cmds_t_strs[ (cmd) ] : csmi_cmds_t_strs[ CSM_CMD_INVALID ] )
#define csmi_cmd_is_valid( cmd ) ( ( (cmd) < CSM_CMD_MAX_REGULAR ) || ( ( (cmd) >= CSM_FIRST_INTERNAL) && ( (cmd) < CSM_CMD_MAX )) )


#ifdef __cplusplus
}
#endif


typedef enum csm_csmi_cmds_t csmi_cmd_t;

#define UNKNOWN_LOC   0
#define LOCAL_DAEMON  1
#define COMPUTE_AGENT 2
#define AGGREGATOR    3
#define MASTER        4

#endif /* CSMI_CMDS_H_ */
