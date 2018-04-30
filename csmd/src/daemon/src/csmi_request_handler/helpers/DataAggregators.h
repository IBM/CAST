/*================================================================================
   
    csmd/src/daemon/src/csmi_request_handler/helpers/DataAggregators.h

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
 
================================================================================*/

/**@file DataAggregators.h
 *
 * A collection of functions to aggregate metrics on a compute node.
 *
 * @author John Dunham (jdunham@us.ibm.com)
 */

#ifndef _CSM_DATA_AGGREGATORS_H_
#define _CSM_DATA_AGGREGATORS_H_

//#include <systemd/sd-bus.h>
#include <string>
#include <inttypes.h>

namespace csm {
namespace daemon {
namespace helper {

/** @brief Retrieve the Infiniband usage at the time of invocation.
 *
 * @param[out] ib_rx Count of data octets received on all infiniband ports.
 * @param[out] ib_tx Count of data octets transmitted on all infiniband ports.
 *
 * @returns True  - If the /sys/class/infiniband/ directory was defined (indicating MOFED).
 * @returns False - If the /sys/class/infiniband/ directory was missing (indicating no MOFED).
 *              @p ib_rx and @p ib_tx are set to -1.
 */
bool GetIBUsage(int64_t &ib_rx, int64_t &ib_tx);

/** @brief Retrieve the GPFS I/O Usgage at the time of invocation.
 *
 * @param[out] gpfs_read The network bytes read counter.
 * @param[out] gpfs_write The network byes written counter.
 *
 * @return True  - GPFS was running and the query succeeded.
 * @return False - GPFS was not reachable, 
 *          @p gpfs_read and @p gpfs_write were set to -1.
 */
bool GetGPFSUsage(int64_t &gpfs_read, int64_t &gpfs_write);

/** @brief Retrieves OCC accounting data from the OCC sensor kernel utility.
 * @param[out] energy The energy usage in watts.
 * @param[out] power_cap_hit The number of throttled watts.
 * @param[out] gpu_energy The gpu energy usage in watts.
 *
 * @return True  - The data was read successfully.
 * @return False - The data could not be read.
 */
bool GetOCCAccounting(int64_t &energy, int64_t &power_cap_hit, int64_t gpu_energy );

/** @brief Retrieves the power capacity of the node using the OCC /sys/fs utility.
 * This function queries the OCC file to determine the power cap.
 *
 * @return -1 : The capacity could not be found
 * @return >0 : The power capacity of the node.
 */
int32_t GetPowerCapacity();

/** @brief Retrieves the power shift ratio of the node using the OCC /sys/fs utility.
 *
 * @warning If the node has multiple NUMA nodes and the psr is different a warning will be
 *      issued and the first found power shift ratio will be used in the database.
 *
 * @return -1 : No psr was found.
 * @return [0-100] : The power shift ratio. 0 - favor gpu; 100 - favor cpu.
 */
int32_t GetPowerShiftRatio();

} // End namespace helpers
} // End namespace daemon
} // End namespace csm

#endif
