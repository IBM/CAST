/*================================================================================
   
    csmd/src/daemon/src/csmi_request_handler/helpers/OCCSensorData.h

    © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
 
================================================================================*/

/**@file OCCSensorData.h
 *
 * Structs for extracting OCC sensor data.
 */

#ifndef _OCC_SENSOR_DATA_H_ 
#define _OCC_SENSOR_DATA_H_ 

// Max maxmimum values.
#define MAX_OCCS               8
#define MAX_CHARS_SENSOR_NAME 16
#define MAX_CHARS_SENSOR_UNIT  4

// Offset to the OCC Binary data block.
#define OCC_SENSOR_DATA_BLOCK_OFFSET 0x00580000
#define OCC_SENSOR_DATA_BLOCK_SIZE   0x00025800

#include  <vector>
#include  <unordered_map>
namespace csm {
namespace daemon {
namespace helper {

// ==============================================
// Enum Definitions.
// ==============================================
enum occ_sensor_type {
    OCC_SENSOR_TYPE_GENERIC        = 0x0001,
    OCC_SENSOR_TYPE_CURRENT        = 0x0002,
    OCC_SENSOR_TYPE_VOLTAGE        = 0x0004,
    OCC_SENSOR_TYPE_TEMPERATURE    = 0x0008,
    OCC_SENSOR_TYPE_UTILIZATION    = 0x0010,
    OCC_SENSOR_TYPE_TIME           = 0x0020,
    OCC_SENSOR_TYPE_FREQUENCY      = 0x0040,
    OCC_SENSOR_TYPE_POWER          = 0x0080,
    OCC_SENSOR_TYPE_PERFORMANCE    = 0x0200,
};

enum occ_sensor_location {
    OCC_SENSOR_LOC_SYSTEM       = 0x0001,
    OCC_SENSOR_LOC_PROCESSOR    = 0x0002,
    OCC_SENSOR_LOC_PARTITION    = 0x0004,
    OCC_SENSOR_LOC_MEMORY       = 0x0008,
    OCC_SENSOR_LOC_VRM          = 0x0010,
    OCC_SENSOR_LOC_OCC          = 0x0020,
    OCC_SENSOR_LOC_CORE         = 0x0040,
    OCC_SENSOR_LOC_GPU          = 0x0080,
    OCC_SENSOR_LOC_QUAD         = 0x0100,
};

enum sensor_struct_type {
    OCC_SENSOR_READING_FULL       = 0x01,
    OCC_SENSOR_READING_COUNTER    = 0x02,
};

enum sensor_attr {
    SENSOR_SAMPLE,
    SENSOR_ACCUMULATOR,
    SENSOR_CSM_MIN,
    SENSOR_CSM_MAX
};

/** @brief A collection of noteworthy sensor ids.
 */
enum sensor_id{
    PWRSYS       = 158, // Power Usage System
    PROCPWRTHROT = 301  // Power Cap
};
// ==============================================

// ==============================================
// Struct Definitions.
// ==============================================

/**
 * struct occ_sensor_data_header -    Sensor Data Header Block
 */
struct occ_sensor_data_header {
    uint8_t  valid;               /**< When the value is 0x01 it indicates that this header block 
                                        and the sensor names buffer are ready.*/
    uint8_t  version;             ///< Format version of this block.
    uint16_t nr_sensors;          ///< Number of sensors in names, ping and pong buffer.
    uint8_t  reading_version;     ///< Format version of the Ping/Pong buffer.
    uint8_t  pad[3];              
    uint32_t names_offset;        ///< Offset to the location of names buffer.
    uint8_t  names_version;       ///< Format version of names buffer.
    uint8_t  name_length;         ///< Length of each sensor in names buffer.
    uint16_t reserved;      
    uint32_t reading_ping_offset; ///< Offset to the location of Ping buffer. 
    uint32_t reading_pong_offset; ///< Offset to the location of Ping buffer
} __attribute__((__packed__));

/**
 * struct occ_sensor_name -        Format of Sensor Name
 */
struct occ_sensor_name {
    char name[MAX_CHARS_SENSOR_NAME];  ///< Sensor name.
    char units[MAX_CHARS_SENSOR_UNIT]; ///< Sensor units of measurement.
    uint16_t gsid;                      ///< Global sensor id (OCC).
    uint32_t freq;                      ///< Update frequenc.
    uint32_t scale_factor;              ///< Scaling factor.
    uint16_t type;                      ///< Sensor type as defined in 'enum occ_sensor_type'.
    uint16_t location;                  ///< Sensor location as defined in 'enum occ_sensor_location'.
    uint8_t structure_type;             /**< Indicates type of data structure used for the sensor 
                                                readings in the ping and pong buffers for this 
                                                sensor as defined in 'enum sensor_struct_type'. */
    uint32_t reading_offset;            /**< Offset from the start of the ping/pong reading buffers 
                                                for this sensor. */
    uint8_t sensor_data;                ///< Sensor specific info.
    uint8_t pad[8];
} __attribute__((__packed__));

/**
 * struct occ_sensor_record -        Sensor Reading Full
 */
struct occ_sensor_record {
    uint16_t gsid;              ///< Global sensor id (OCC)
    uint64_t timestamp;         ///< Time base counter value while updating the sensor
    uint16_t sample;            ///< Latest sample of this sensor.
    uint16_t sample_min;        ///< Minimum value since last OCC reset.
    uint16_t sample_max;        ///< Maximum value since last OCC reset
    uint16_t csm_min;           ///< Minimum value since last reset request by CSM (CORAL).
    uint16_t csm_max;           ///< Maximum value since last reset request by CSM (CORAL).
    uint16_t profiler_min;      ///< Minimum value since last reset request by profiler (CORAL).
    uint16_t profiler_max;      ///< Maximum value since last reset request by profiler (CORAL).
    uint16_t job_scheduler_min; ///< Minimum value since last reset request by job scheduler(CORAL).
    uint16_t job_scheduler_max; ///< Maximum value since last reset request by job scheduler (CORAL).
    uint64_t accumulator;       ///< Accumulator for this sensor.
    uint32_t update_tag;        ///< Count of the number of ticks that have passed between updates.
    uint8_t pad[8];
} __attribute__((__packed__));


/**
 * struct occ_sensor_counter -        Sensor Reading Counter
 */
struct occ_sensor_counter {
    uint16_t gsid;        ///< Global sensor id (OCC).
    uint64_t timestamp;   ///< Time base counter value while updating the sensor.
    uint64_t accumulator; ///< Accumulator/Counter.
    uint8_t sample;       ///< Latest sample of this sensor (0/1).
    uint8_t pad[5];
} __attribute__((__packed__));
// ==============================================
// Function Definitions.
// ==============================================


/** @brief Queries all of the detected sensor blocks for the key values in the @p valueMap.
 * The results of the query are stored as a sum in the @p valueMap.
 *
 * @param[in,out] valueMap A mapping of the sensor blocks to output values. Contents of output
 *  values will be summed with matches in the sensor blocks.
 *
 *  @return True if sensors could be read.
 */
bool GetOCCSensorData(std::unordered_map<std::string,int64_t> &valueMap);

struct CsmOCCSensorRecord
{
    int64_t sample;            ///< Latest sample of this sensor.
    int64_t csm_min;           ///< Minimum value since last reset request by CSM (CORAL).
    int64_t csm_max;           ///< Maximum value since last reset request by CSM (CORAL).
    int64_t accumulator;       ///< Accumulator for this sensor.
};

/** 
 *  @brief Queries all of the detected sensor blocks for the key values in the @p inMap.
 *  Stores the sample, csm_min, csm_max, and accumulator values for the sensors in the outValues, if applicable. 
 *  Values are reported on a per chip basis without accumulating values across chips.
 *  Node level values are reported as part of chip index 0 in the output data.
 *  Values are only populated into the csm_min, csm_max, and accumulator for sensors that support them. 
 *  
 *  @param[in] inMap The value map containing the requested list of sensors.
 *  @param[out] outValues A vector of maps, indexed by chip, containing the current values for the sensor. 
 */
bool GetExtendedOCCSensorData( std::unordered_map<std::string,CsmOCCSensorRecord> &inMap, 
    std::vector<std::unordered_map<std::string,CsmOCCSensorRecord>> &outValues);

} // helper
} // daemon
} // csm

#endif
