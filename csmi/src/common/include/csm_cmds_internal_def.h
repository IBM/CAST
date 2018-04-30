/*================================================================================

    csmi/src/common/include/csm_cmds_internal_def.h

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
//Internal CMDs
#define CSM_FIRST_INTERNAL CSM_CTRL_reconfig
cmd(CSM_CMD_API_DIVIDE) // Divider for filtration

cmd(CSM_CTRL_reconfig)  // 38
cmd(CSM_CTRL_cmd)
//below is for internal testing purpose
cmd(CSM_TEST_node_attributes_query)
// MTC for multi-cast testing
cmd(CSM_TEST_MTC)
cmd(CSM_infrastructure_test)  // 42
cmd(CSM_environmental_data)
cmd(CSM_error_inject)
cmd(CSM_DB_UNKNOWN)
cmd(CSM_DAEMON_UNKNOWN)
// for CSMI testing
cmd(CSM_CMD_ECHO)
// for heart beat msgs
cmd(CSM_CMD_HEARTBEAT)    // 47
cmd(CSM_CMD_STATUS)
cmd(CSM_CMD_CONNECTION_CTRL)
cmd(CSM_CMD_NODESET_UPDATE)
// use CSM_CMD_ERROR when the CommandType is unknown
cmd(CSM_CMD_ERROR)
