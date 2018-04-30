/*================================================================================
   
    csmd/src/daemon/src/csmi_request_handler/helpers/DataAggregators.cc

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
 
================================================================================*/

#include <iostream>    ///< IO stream for file operations.
#include <fstream>     ///< ofstream and ifstream used.
#include <regex>       ///< Regular exceptions 
#include <sys/types.h> ///< File Status types.
#include <sys/stat.h>  ///< File Status Functions.
#include <errno.h>     ///< Errno
#include <dirent.h>    ///< DIR and Directory sys calls.
#include "logging.h"   ///< CSM logging.
#include "csm_handler_exception.h"
#include "DataAggregators.h"
#include "OCCSensorData.h" // Sensor data utilities.

#define _HIDDEN_CHAR '.'

namespace csm {
namespace daemon {
namespace helper {

bool GetIBUsage(int64_t &ib_rx, int64_t &ib_tx)
{
    LOG(csmapi, trace) << "Enter GetIBUsage";

    // Constant values for defining the path of the ib usage data.
    static const std::string IB_DEVS( "/sys/class/infiniband/" );   
    static const std::string PORTS("/ports/");
    static const std::string COUNTERS("/counters/");
    // The rx and tx counters.
    static const std::string RECV  = "port_rcv_data";
    static const std::string TRANS = "port_xmit_data";
    
    // 0. Initialize the output.
    ib_rx = 0;
    ib_tx = 0;

    // 1. Open the device directory.
    errno = 0;
    DIR    *devDir = opendir(IB_DEVS.c_str());
    dirent *devDirDetails; 
    
    // If the device directory couldn't be opened, this node likely doesn't have OFED.
    // Set the return values to -1 to make th issue 
    if ( !devDir )
    {
        ib_rx = -1;
        ib_tx = -1;

        LOG(csmapi, trace) << "Exit GetIBUsage : OFED not detected.";
        return false;
    }
    
    // Use RAII to clean up the directory as the ifstreams can throw an error.
    std::shared_ptr<DIR> sharedPortsDir( devDir, closedir);

    // 1. Iterate over all the devices installed.
    while ( ( devDirDetails = readdir( devDir ) ) )
    {
        if (devDirDetails->d_name[0] == _HIDDEN_CHAR )
        {
            continue;
        }

        // Build up the ports string to get the list of ports.
        std::string portsStr = IB_DEVS + devDirDetails->d_name + PORTS;

        // 2. Open the ports directory of the device.
        errno = 0;
        DIR    *portsDir = opendir( portsStr.c_str());
        dirent *portsDirDetails;

        // If the portsDir was not opened, just continue.
        if ( !portsDir ) continue; // TODO cache failures?

        // Use RAII to clean up the directory as the ifstreams can throw an error.
        std::shared_ptr<DIR> sharedPortsDir( portsDir, closedir);

        // 3. Iterate over all the ports for the device.
        while ( ( portsDirDetails = readdir( portsDir ) ) )
        {
            if (portsDirDetails->d_name[0] == _HIDDEN_CHAR)
            {
                continue;
            }

            // Build the counter string (this is on a per directory basis).
            std::string counterStr = portsStr + portsDirDetails->d_name + COUNTERS;
            std::string line;

            // 5. Open a stream to the received data counter. 
            std::ifstream recvStream(counterStr + RECV);

            if ( recvStream.is_open() )
            {
                std::getline(recvStream, line);
                ib_rx +=  strtoll(line.c_str(), nullptr, 10);
            }

            // 6. Open a stream to the transmitted data counter.
            std::ifstream transStream(counterStr + TRANS);
            if ( transStream.is_open() )
            {
                std::getline(transStream, line);
                ib_tx += strtoll(line.c_str(), nullptr, 10);
            }
        }
        //closedir(portsDir);
    }

    LOG(csmapi, trace) << "Exit GetIBUsage";
    return true; // Even if none of the ports were available this is a success.

}

bool GetGPFSUsage(int64_t &gpfs_read, int64_t &gpfs_write)
{
    LOG(csmapi, trace) << "Enter GetGPFSUsage";

    // Success of the execution
    bool success = true;
    gpfs_read    = -1;
    gpfs_write   = -1;

    // Open a pipe to execute the gpfs command.
    std::array<char, 128> buffer;
    std::stringstream outputStream;
    FILE* pipe = popen( "/usr/lpp/mmfs/bin/mmfsadm eventsExporter mmpmon ns", "r" );
    
    // If the pipe was successfully opened execute.
    if ( pipe )
    {
        // Use a shared pointer, it should be safer for the pclose.
        std::shared_ptr<FILE> sharedPipe(pipe, pclose);

        // Build a string stream from the buffer.
        while(!feof(pipe))
        {
            if ( fgets( buffer.data(), 128, pipe) != nullptr)
                outputStream << buffer.data();
        }

        // Iterate over the stream to extract the usable values.
        std::string outputLine;
        while (std::getline(outputStream, outputLine, '\n'))
        {
            char *save_ptr;
            char *line_str = strdup(outputLine.c_str());
            char *val_str  = strtok_r(line_str, " ", &save_ptr);

            // First test that the string is either _r_ or _w_
            if ( val_str && strlen(val_str) >= 3                     &&
                    val_str[0] == '_'                                && 
                    ( outputLine[1] == 'r' || outputLine[1] == 'w' ) && 
                    outputLine[2] == '_' )
            {
                // Cache the type of op this is representing.
                bool isRead = outputLine[1] == 'r';
                bool scan   = true;

                // Seek the _b_ string.
                while( val_str != nullptr && scan)
                {
                    scan = strcmp(val_str, "_b_") != 0;
                    val_str = strtok_r(nullptr, " ", &save_ptr);
                }
                
                // If the previous step was a success place the next field in the correct 
                // attribute.
                if ( val_str != nullptr )
                {
                    if ( isRead )
                        gpfs_read  = strtoll(val_str, nullptr, 10);
                    else
                        gpfs_write = strtoll(val_str, nullptr, 10);
                }
                
            }

            free(line_str);
        }
    }
    else
    {
        success = false;
    }
    
    LOG(csmapi, trace) << "Exit GetGPFSUsage";

    return success;
}

bool GetOCCAccounting(int64_t &energy, int64_t &power_cap_hit, int64_t gpu_energy)
{
    // Generate the value map for the query.
    std::unordered_map<std::string, int64_t> valueMap = {
        {"PROCPWRTHROT", 0},
        {"PWRSYS"      , 0},
        {"PWRGPU"      , 0}
    };
    
    // Query and check for success.
    bool success = GetOCCSensorData( valueMap );

    // Extract values.
    power_cap_hit = valueMap["PROCPWRTHROT"];
    energy        = valueMap["PWRSYS"];
    gpu_energy    = valueMap["PWRGPU"];

    return success;
}

int32_t GetPowerCapacity()
{
    LOG(csmapi, trace) << "Enter GetPowerCapacity";
    static const std::string POWER_CAPACITY("/sys/firmware/opal/powercap/system-powercap/powercap-current");

    // TODO should this be -1 or zero - John Dunham
    int32_t powerCap = -1;

    std::string line;
    std::ifstream powerCapStream(POWER_CAPACITY);
    if ( powerCapStream.is_open() )
    {
        std::getline(powerCapStream, line);
        powerCap = strtol(line.c_str(), nullptr, 10);
    }

    LOG(csmapi, trace) << "Exit GetPowerCapacity";
    return powerCap;
}

int32_t GetPowerShiftRatio()
{
    LOG(csmapi, trace) << "Enter GetPowerDetails";

    static const std::string POWER_SHIFT_RATIO("/sys/firmware/opal/psr/");
    
    // 0. Initialize power shift ratio.
    int32_t powerShiftRatio = -1;

    // 1. Get the power shift ratio.
    errno = 0;
    DIR    *powerDir = opendir(POWER_SHIFT_RATIO.c_str());
    dirent *powerDirDetails;
    
    // If the PSR directory couldn't be opened, this node might not have a good firmware level.
    if ( !powerDir )
    {
        LOG(csmapi, warning) << "Exit GetPowerShiftRatio : Power Shift Ratio not found in " 
            << POWER_SHIFT_RATIO;
        return powerShiftRatio;
    }
    
    // Use RAII to clean up the directory as the ifstreams can throw an error.
    std::shared_ptr<DIR> sharedPowerDir( powerDir, closedir);

    // 2. Iterate over the psr dir and grab all of the numa modules.
    while( (powerDirDetails = readdir( powerDir ))  ) 
    {
        if ( powerDirDetails->d_name[0] == _HIDDEN_CHAR ) continue;
       
        std::string line;
        std::string psrFile =  POWER_SHIFT_RATIO + powerDirDetails->d_name;
        std::ifstream psrStream(psrFile);

        // If the file could be opened attempt to parse.
        if ( psrStream.is_open() )
        {
            std::getline(psrStream, line);
            int32_t psr = strtol(line.c_str(), nullptr, 10);

            // If the power shift ratio hasn't been set, set it.
            // Else if the new psr differs log a warning.
            if ( powerShiftRatio < 0 ) 
            {
                powerShiftRatio = psr;
            }
            else if ( psr != powerShiftRatio )
            {
                LOG(csmapi, warning) << " Power shift ratio mismatch across sockets.";
            }
        }
        else
        {
            LOG(csmapi, error) << "Couldn't open " << psrFile;
        }
    }

    LOG(csmapi, trace) << "Exit GetPowerDetails";
    return powerShiftRatio;
}


} // End namespace helpers
} // End namespace daemon
} // End namespace csm

