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
        default:
            return 0;
    }
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
        // TODO throw exception.
        return false;
    }

    // Initiailize the chip for the search.
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

} // helper
} // daemon
} // csm

