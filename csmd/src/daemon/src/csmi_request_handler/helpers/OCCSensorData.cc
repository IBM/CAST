/*================================================================================
   
    csmd/src/daemon/src/csmi_request_handler/helpers/OCCSensorData.cc

    © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
 
================================================================================*/

#include "OCCSensorData.h"
#include <fcntl.h>
#include <stdint.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <math.h>
#include <string.h>

#include <dirent.h>       // Provides scandir()
#include <glob.h>

#include <fstream>

#define OCC_INBAND_SENSORS "/sys/firmware/opal/exports/occ_inband_sensors"
#define TO_FP(f)    ((f >> 8) * pow(10, (f & 0xFF)))

namespace csm {
namespace daemon {
namespace helper {

/** 
 * @brief Reads the specified sensor and extracts its meaningful data.
 * @param[in] hb The data header to read from.
 * @param[in] offset The offset to perform the read in the data header.
 * @param[in] attr The attribute type, impacts the sensor field queried..
 *
 * @return The contents of the sensor, 0 in the case of a bad parameter.
 */
int64_t ReadSensor(struct occ_sensor_data_header *hb, uint32_t offset, int attr)
{
    struct occ_sensor_record *sping, *spong;
    struct occ_sensor_record *sensor = NULL;
    uint8_t *ping, *pong;
    
    // Read the Ping and pong 40kB buffers that contain the sensor data.
    ping = (uint8_t *)((uint64_t)hb + be32toh(hb->reading_ping_offset));
    pong = (uint8_t *)((uint64_t)hb + be32toh(hb->reading_pong_offset));
    // The struct portion of the buffer.
    sping = (struct occ_sensor_record *)((uint64_t)ping + offset);
    spong = (struct occ_sensor_record *)((uint64_t)pong + offset);
    
    // Determine the newest most up to date buffer.
    if (*ping && *pong) 
    {
        if (be64toh(sping->timestamp) > be64toh(spong->timestamp))
            sensor = sping;
        else
            sensor = spong;
    }
    else if (*ping && !*pong) 
    {
        sensor = sping;
    } 
    else if (!*ping && *pong) 
    {
        sensor = spong;
    } 
    else if (!*ping && !*pong) 
    {
        return 0;
    }
    
    // Extract the contents of the buffer.
    switch (attr)
    {
        case SENSOR_SAMPLE:
            return be16toh(sensor->sample);
        case SENSOR_ACCUMULATOR:
            return be64toh(sensor->accumulator);
        case SENSOR_CSM_MIN:
            return be16toh(sensor->csm_min);
        case SENSOR_CSM_MAX:
            return be16toh(sensor->csm_max);
        default:
            return 0;
    }
}

int64_t read_counter(struct occ_sensor_data_header *hb, uint32_t offset)
{
    struct occ_sensor_counter *sping, *spong;
    struct occ_sensor_counter *sensor = NULL;
    uint8_t *ping, *pong;

    ping = (uint8_t *)((uint64_t)hb + be32toh(hb->reading_ping_offset));
    pong = (uint8_t *)((uint64_t)hb + be32toh(hb->reading_pong_offset));
    sping = (struct occ_sensor_counter *)((uint64_t)ping + offset);
    spong = (struct occ_sensor_counter *)((uint64_t)pong + offset);

    // Determine the newest most up to date buffer.
    if (*ping && *pong) 
    {
        if (be64toh(sping->timestamp) > be64toh(spong->timestamp))
            sensor = sping;
        else
            sensor = spong;
    }
    else if (*ping && !*pong)
    {
        sensor = sping;
    }
    else if (!*ping && *pong)
    {
        sensor = spong;
    }
    else if (!*ping && !*pong)
    {
        return 0;
    }

    return be64toh(sensor->accumulator);
}


/** 
 *  @brief Searches the buffer for matches in the @p valueMap.
 *  When a hit is found add the sensor contents to the map.
 *  
 *  @param[in] buffer The buffer containing the sensor information.
 *  @param[in,out] valueMap The value map containing the sensors and sensor sums.
 */
void SeekSensors(const char *buffer, std::unordered_map<std::string,int64_t> &valueMap)
{
    struct occ_sensor_data_header *dataHeader;
    struct occ_sensor_name *sensorNames;

    // Retrieve some structs in the binary buffer data.
    dataHeader = (struct occ_sensor_data_header *) buffer;
    sensorNames = (struct occ_sensor_name *)((uint64_t)dataHeader + be32toh(dataHeader->names_offset));
    
    // Cache the number of sensors.
    int numSensors = be16toh(dataHeader->nr_sensors);

    // Iterate over the sensors, aggregating hits.
    for ( int i = 0; i < numSensors; ++i )
    {
        uint32_t offset =  be32toh(sensorNames[i].reading_offset);
        uint32_t freq = be32toh(sensorNames[i].freq); 

        if ((be16toh(sensorNames[i].type) == OCC_SENSOR_TYPE_POWER || 
                sensorNames[i].structure_type == OCC_SENSOR_READING_COUNTER ))
        {
            continue;
        }

        // Read the sensor if it was found in the value map.
        freq = TO_FP(freq);
        auto searchResult = valueMap.find(sensorNames[i].name);
        if ( searchResult != valueMap.end() )
        {
            searchResult->second += ReadSensor(dataHeader, offset, SENSOR_ACCUMULATOR);
        }
    }
}

bool GetOCCSensorData(std::unordered_map<std::string,int64_t> &valueMap)
{
    // Open the kernel file.
    int fd = open(OCC_INBAND_SENSORS, O_RDONLY);
    if (fd < 0)
    {
        //TODO Throw exception!
        return false;
    }
    
    // Allocate the buffer.
    char* buffer = (char*)malloc(OCC_SENSOR_DATA_BLOCK_SIZE);
    if ( !buffer )
    {
        // Close the file descriptor.
        close(fd);
        // TODO throw exception.
        return false;
    }
    
    // Initialize all values in the map to 0
    for (auto map_itr = valueMap.begin(); map_itr != valueMap.end(); map_itr++)
    {
        map_itr->second = 0;
    }

    // Initialize the chip for the search.
    int chipid = 0, rc = 0, bytes= 0; 

    // TODO is there a chance of this being a problem?
    while(true)
    {
        // Read the kernel file, populate the buffer.
        for ( rc = bytes = 0; bytes < OCC_SENSOR_DATA_BLOCK_SIZE; bytes +=rc )
        {
            rc = read(fd, buffer + bytes, OCC_SENSOR_DATA_BLOCK_SIZE - bytes);
            if ( !rc || rc < 0 )
                break;
        }

        // If the buffer is full process, otherwise exit early.
        if( bytes == OCC_SENSOR_DATA_BLOCK_SIZE )
        {
            // Track the chipid for completeness.
            chipid++;
            // Seek any sensors declared in the value map, summing.
            SeekSensors(buffer, valueMap);
            // Clear the buffer for the next step.
            memset(buffer, 0, OCC_SENSOR_DATA_BLOCK_SIZE);
            //// Seek the 
            //lseek(fd, chipid * OCC_SENSOR_DATA_BLOCK_SIZE, SEEK_CUR);
        }
        else
        {
            break;
        }
    }
    
    free(buffer);
    close(fd);
    return true;
}

/** 
 *  @brief Searches the buffer for matches in the @p inMap.
 *  When a hit is found add the sensor contents to the outValues as appropriate.
 *  
 *  @param[in] buffer The buffer containing the sensor information.
 *  @param[in] inMap The map containing the requested set of sensors to read.
 *  @param[out] outValues The value map containing the current values for the requested sensors.
 */
void SeekExtendedSensors(const char *buffer, const std::unordered_map<std::string,CsmOCCSensorRecord> & inMap,
    std::unordered_map<std::string,CsmOCCSensorRecord> &outMap)
{
    struct occ_sensor_data_header *dataHeader;
    struct occ_sensor_name *sensorNames;

    // Retrieve some structs in the binary buffer data.
    dataHeader = (struct occ_sensor_data_header *) buffer;
    sensorNames = (struct occ_sensor_name *)((uint64_t)dataHeader + be32toh(dataHeader->names_offset));
    
    // Cache the number of sensors.
    int numSensors = be16toh(dataHeader->nr_sensors);
    
    // Iterate over the sensors, aggregating hits.
    for (int i = 0; i < numSensors; ++i)
    {
        uint32_t offset = be32toh(sensorNames[i].reading_offset);
        uint32_t freq = be32toh(sensorNames[i].freq); 

        // Read the sensor if it was found in the value map.
        freq = TO_FP(freq);

        auto searchResult = inMap.find(sensorNames[i].name);
        if ( searchResult != inMap.end() )
        {
            if (sensorNames[i].structure_type == OCC_SENSOR_READING_FULL)
            {
                // Adjust the accumulator value based on the frequency, but guard against division by zero 
                int64_t accumulator = ReadSensor(dataHeader, offset, SENSOR_ACCUMULATOR);
                if (freq > 0)
                {
                    accumulator = (int64_t) (ReadSensor(dataHeader, offset, SENSOR_ACCUMULATOR) / freq);
                }
                else
                {
                   accumulator = 0;
                }
                
                outMap.insert
                ({ 
                    searchResult->first,
                    { 
                        ReadSensor(dataHeader, offset, SENSOR_SAMPLE),
                        ReadSensor(dataHeader, offset, SENSOR_CSM_MIN),
                        ReadSensor(dataHeader, offset, SENSOR_CSM_MAX),
                        accumulator
                    }
                });
            }
            else if (sensorNames[i].structure_type == OCC_SENSOR_READING_COUNTER)
            {
                outMap.insert
                ({ 
                    searchResult->first,
                    { 
                        0, 
                        0,
                        0,
                        read_counter(dataHeader, offset)
                    }
                });
            }
        }
    }
}

bool GetExtendedOCCSensorData( std::unordered_map<std::string,CsmOCCSensorRecord> &inMap,
    std::vector<std::unordered_map<std::string,CsmOCCSensorRecord>> &outValues)
{
    // Open the kernel file.
    int fd = open(OCC_INBAND_SENSORS, O_RDONLY);
    if (fd < 0)
    {
        //TODO Throw exception!
        return false;
    }
    
    // Allocate the buffer.
    char* buffer = (char*)malloc(OCC_SENSOR_DATA_BLOCK_SIZE);
    if ( !buffer )
    {
        // Close the file descriptor.
        close(fd);
        // TODO throw exception.
        return false;
    }
    
    // Initialize the chip for the search.
    int chipid = 0, rc = 0, bytes= 0; 

    // TODO is there a chance of this being a problem?
    while(true)
    {
        // Read the kernel file, populate the buffer.
        for ( rc = bytes = 0; bytes < OCC_SENSOR_DATA_BLOCK_SIZE; bytes +=rc )
        {
            rc = read(fd, buffer + bytes, OCC_SENSOR_DATA_BLOCK_SIZE - bytes);
            if ( !rc || rc < 0 )
                break;
        }

        // If the buffer is full process, otherwise exit early.
        if( bytes == OCC_SENSOR_DATA_BLOCK_SIZE )
        {
            outValues.push_back(std::unordered_map<std::string,CsmOCCSensorRecord>());

            // Seek any sensors declared in the inMap.
            SeekExtendedSensors(buffer, inMap, outValues[chipid]);
            
            // Clear the buffer for the next step.
            memset(buffer, 0, OCC_SENSOR_DATA_BLOCK_SIZE);
            
            chipid++;
        }
        else
        {
            break;
        }
    }
    
    free(buffer);
    close(fd);
    return true;
}

/**
 * @brief Resets the CSM min and max sensor values for all chips in the node.
 */
bool ResetOccCsmSensorMinMax()
{
    bool successful(true);

    // Scan the OCC sensor groups to find all OCC CSM sensor groups
    // Use globbing "*" for the occ-csm* portions of the directory names to find all OCC CSM sensor groups
    // ls -d /sys/firmware/opal/sensor_groups/occ-csm*/clear
    // /sys/firmware/opal/sensor_groups/occ-csm0/clear  
    // /sys/firmware/opal/sensor_groups/occ-csm8/clear
    const char OCC_CSM_GLOB_PATH[] = "/sys/firmware/opal/sensor_groups/occ-csm*/clear";

    int32_t globflags(0);
    glob_t csm_sensor_paths;
    int32_t rc(0);

    rc = glob(OCC_CSM_GLOB_PATH, globflags, nullptr, &csm_sensor_paths);
    if (rc == 0)
    {
        if (csm_sensor_paths.gl_pathc < 1)
        {
            // No OCC CSM sensor groups found
            successful = false;
        }

        // Reset each sensor group
        // echo 1 > /sys/firmware/opal/sensor_groups/occ-csm0/clear
        // echo 1 > /sys/firmware/opal/sensor_groups/occ-csm8/clear
        for (uint32_t i = 0; i < csm_sensor_paths.gl_pathc; i++)
        {
            std::ofstream clear_out(csm_sensor_paths.gl_pathv[i]);

            if (clear_out.is_open())
            {
                clear_out << "1" << std::endl;
            }
            else
            {
                // Failed to open the CSM sensor group clear file
                successful = false;
            }
        }
    }
    else
    {
        // glob returned an error when looking for the CSM sensor group paths 
        successful = false;
    }

    globfree(&csm_sensor_paths);

    return successful;
}

} // helper
} // daemon
} // csm
