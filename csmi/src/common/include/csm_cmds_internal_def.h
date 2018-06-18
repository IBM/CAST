/*================================================================================

    csmi/src/common/include/csm_cmds_internal_def.h

  © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
//Internal CMDs
#define CSM_FIRST_INTERNAL CSM_CTRL_reconfig

cmd(CSM_CTRL_reconfig) // (201)
cmd(CSM_CTRL_cmd)
cmd(CSM_infrastructure_test)
cmd(CSM_TEST_MTC) // MTC for multi-cast testing
cmd(CSM_error_inject) // (205) injecting errors into the infrastructure for testing
cmd(CSM_environmental_data_obsolete) // Obsolete env data prototype message from compute to agg, replaced by CSM_environmental_data
cmd(CSM_CMD_ECHO)  // for testing of comm and handling
cmd(CSM_CMD_HEARTBEAT) // keep-alive messages between peers
cmd(CSM_CMD_STATUS) // connection creation/teardown signalling
cmd(CSM_CMD_CONNECTION_CTRL) // (210) connection status changes between compute and agg
cmd(CSM_CMD_NODESET_UPDATE) // compute node set update messages between agg and master
cmd(CSM_CMD_ERROR) // use CSM_CMD_ERROR when the CommandType is unknown
cmd(CSM_environmental_data) // env data from compute to agg
