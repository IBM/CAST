/*================================================================================
   
    csmd/src/daemon/src/csmi_request_handler/helpers/AgentHandler.cc

  © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
 
================================================================================*/

#include "AgentHandler.h"
#include <dirent.h>
#include <string>
#include <cctype>

#define PROC         "/proc/"
#define CSM_P        "/opt/ibm/csm/prologs/privileged_"
#define CSM_P_LEN    strlen(CSM_P)


namespace csm {
namespace daemon {
namespace helper {

int64_t GetNumericEnvironmentVariable(const std::string& filePath, const std::string& variableName )
{
    int64_t value = 0; // The contents of the numeric environment variable.
    size_t varLen = variableName.size(); // Length of the variable being searched on.

    // Open a connection to the environment file, split the search on null characters.
    std::ifstream sourceStream( filePath );
    std::string line;
    while( getline(sourceStream, line, '\0') )
    {
        if( line.compare(0, varLen, variableName) == 0 )
        {
            // Extract the 
            std::string valStr = line.substr(line.find('=') + 1, std::string::npos); 
            value=strtoll(valStr.c_str(), nullptr, 10);
            break;
        }
    }

    return value;
}

int ScanForPrivleged(bool isProlog, int64_t allocationId, bool isShared)
{
    // Open the proc dir for searching.
    DIR * cDir = opendir(PROC);
    dirent *dirDetails;
    
    //pid_t prologPid = 0;
    //pid_t epilogPid = 0;
    //int64_t processAllocation = 0;
    int errorCode = 0;

    // This should never get hit.
    if ( !cDir )
        return errorCode;

    std::shared_ptr<DIR> cDirShared(cDir, closedir );
    
    while ( ( dirDetails = readdir( cDir ) )  )
    {
        // Search for process ids.
        if ( dirDetails->d_type == DT_DIR && isdigit(dirDetails->d_name[0]))
        {
            std::string procDir(PROC);
            procDir.append(dirDetails->d_name);


            // Command file.
            std::string cmd(procDir);
            cmd.append("/cmdline");

            // Read the commandline
            std::ifstream sourceStream( cmd );
            std::string line;
            while( getline(sourceStream, line, '\0') )
            {
                if ( line.compare(0, CSM_P_LEN,  CSM_P) == 0 )
                {
                    // If a shared allocation search the environment for the allocation id.
                    if(isShared) 
                    {
                        std::string environ(procDir);
                        environ.append("/environ");

                        int64_t procAID = GetNumericEnvironmentVariable(environ, 
                            CSM_TYPE_ALLOCATION_ID);

                        // Continue if the allocation id isn't colliding in the shared case.
                        if ( procAID != allocationId )
                            continue;

                    }

                    bool pidIsProlog=line.compare( 0, std::string::npos, CSM_P_PROLOG) == 0;
                    char state = ( ( (isProlog & 1) << 1) |  (pidIsProlog & 1) );

                    switch (state)
                    {
                        case '\1': // epilog, prolog
                        {
                            // TODO Enhance this code in the future.
                            // Attempt to kill the prolog, if the kill fails return an error.
                            errno = 0;
                            long pid = atol(dirDetails->d_name);                
                            LOG(csmapi, debug) << "Killing prolog, pid: " << pid;

                            kill(pid,SIGKILL);
                            if( errno != 0 )
                                errorCode = CSMERR_EPILOG_PROLOG_COLLISION;
                            break;
                        }
                        // These cases are considered failures.
                        case '\0': // epilog, epilog
                            errorCode = CSMERR_EPILOG_EPILOG_COLLISION;
                            break;
                        case '\2': // prolog, epilog
                            errorCode = CSMERR_PROLOG_EPILOG_COLLISION;
                            break;
                        default: // prolog, prolog
                            errorCode = CSMERR_PROLOG_PROLOG_COLLISION;
                            break;
                    }

                    // Exit command scan.
                    break;
                }
            }
        }
    }
    return errorCode;
}

} // End namespace helpers
} // End namespace daemon
} // End namespace csm

