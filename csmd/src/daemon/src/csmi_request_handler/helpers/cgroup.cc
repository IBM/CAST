/*================================================================================
   
    csmd/src/daemon/src/csmi_request_handler/helpers/cgroup.cc

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
 
================================================================================*/

#include <iostream>    ///< IO stream for file operations.
#include <fstream>     ///< ofstream and ifstream used.
#include <system_error>///< For processing system errors.
#include <sys/types.h> ///< File Status types.
#include <sys/stat.h>  ///< File Status Functions.
#include <sys/mount.h> ///< Mount function
#include <unistd.h>    ///< Write/sync function
#include <fcntl.h>     ///< Open function
#include <errno.h>     ///< Errno
#include <signal.h>    ///< Kill System call.
#include <dirent.h>    ///< DIR and Directory sys calls.
#include "logging.h"   ///< CSM logging.
#include "cgroup.h" 
#include "csm_handler_exception.h"
#include "csmi/include/csm_api_macros.h" // csm_get_enum_from_stringget_enum_from_string

// TODO Might want to migrate to consts.
///< Syntactic sugar for directory flags.
#define _DIRECTORY true
#define _FILE false

///< Parameters to the cgroup mount command.
#define _CGROUP_MOUNT "none"
#define _CGROUP_TYPE  "cgroup"
#define _CGROUP_OPTS  0

///< Cgroup mkdir permissions.
#define _CGROUP_PERM 755

///< Task helpers.
#define _CGROUP_TASK "/tasks"
#define _TASK_KILL SIGKILL
#define _KILL_ATTEMPTS 5

///< File helpers
#define _DIR_DELIM "/"
#define _HIDDEN_CHAR '.'

///< Controller tools.
#define _RANGE_DELIM "-"
#define _GROUP_DELIM ","

///< Logging prefix.
#define _LOG_PREFIX "CGroup::"

#define KB_TO_B(kB) kB * 1024

/// Enables a check for development enviroment.
//#define VM_DEVELOPMENT 0

namespace csm {
namespace daemon {
namespace helper {

static const char ENABLE_CONTROLLER         = '1'; ///< Syntactic Sugar
static const char DISABLE_CONTROLLER        = '0'; ///< Syntactic Sugar

// Constant definitions.
#define CSM_SYSTEM "csm_system"
const char* CGroup::SYSTEM_CGROUP    = "/" CSM_SYSTEM "/";
#define ALLOC "allocation_"
#define ALLOC_LEN strlen(ALLOC)
const char* CGroup::ALLOC_CGROUP     = "/" ALLOC;
#define ALLOC_CGROUP_LEN strlen(CGroup::ALLOC_CGROUP)
const char* CGroup::CONTROLLER_DIR   = "/sys/fs/cgroup/"; 


const char* CGroup::CPUSET           = csmi_cgroup_controller_t_strs[ CG_CPUSET ]; 
const char* CGroup::CPUACCT          = csmi_cgroup_controller_t_strs[ CG_CPUACCT ]; 
const char* CGroup::MEM              = csmi_cgroup_controller_t_strs[ CG_MEMORY ];
const char* CGroup::DEVICES          = csmi_cgroup_controller_t_strs[ CG_DEVICES ];

const std::string CGroup::CPUSET_DIR = std::string(CGroup::CONTROLLER_DIR).append(CGroup::CPUSET).append("/");
const std::string CGroup::CPUACCT_DIR = std::string(CGroup::CONTROLLER_DIR).append(CGroup::CPUACCT).append("/");
const std::string CGroup::MEM_DIR = std::string(CGroup::CONTROLLER_DIR).append(CGroup::MEM).append("/");

bool CGroup::RepairSMTChange() 
{
    const char* CPUS  = "cpuset.cpus";

    int32_t threads, sockets, threadsPerCore, coresPerSocket;
    // Get the CPUS and do a sanity check.
    if ( CGroup::GetCPUs( threads, sockets, threadsPerCore, coresPerSocket ) )
    {
        dirent *dirDetails;                                  // Output struct for directory contents.
        DIR *sysDir =  opendir(CGroup::CPUSET_DIR.c_str());  // Open the directory to search for subdirs.
        
        // If the system directory could not be retrieved throw an exception.
        if ( !sysDir )
        {
            std::string error = "CGroup directory was not mounted: ";
            error.append(CGroup::CPUSET_DIR).append(" errno message: ").append(strerror(errno));

            throw CSMHandlerException( error, CSMERR_CGROUP_FAIL );
        }

        // Construct a shared pointer to let RAII handle the close in the event of a thrown error.
        std::shared_ptr<DIR> sysDirShared(sysDir, closedir);
            
        threadsPerCore--;
        // Build the path for processing.
        std::string dirPath = CGroup::CPUSET_DIR + _DIR_DELIM;
        while ( ( dirDetails = readdir( sysDir ) ) ) // While this assignment is successful.
        {
            // Only seek allocation cgroups.
            if ( !strncmp( dirDetails->d_name, ALLOC,  ALLOC_LEN) ||
                 !strcmp( dirDetails->d_name, CSM_SYSTEM) )
            {
                // Strings for cgroup operations.
                std::string updatedCores = "";
                std::string path( dirPath );
                path.append( dirDetails->d_name ).append("/").append(CPUS);
                
                // Prep the string parse.
                char* coresReserved = strdup(ReadString(path).c_str());
                int32_t core = 0;
                char* pEnd = coresReserved;
                
                // Decrement to help with the offset math.
                do
                {
                    core =strtol( pEnd, &pEnd, 10 );

                    if (  *(pEnd) == '-' )
                    {
                        pEnd++;
                        strtol( pEnd, &pEnd, 10 );
                    }

                    updatedCores.append(std::to_string(core)).append("-")
                        .append(std::to_string(core + threadsPerCore)).append(",");

                }  while ( *(pEnd++) );

                free(coresReserved);

                WriteToParameter("", path, updatedCores.c_str(), updatedCores.size());
            }
        }
    } 
    else
    {
        return false;
    }

    return true;
}

CGroup::CGroup( int64_t allocationId ):
       _CGroupName(ALLOC_CGROUP)
{
    // Build the core cgroup name.
    if ( allocationId >= 0 )
        _CGroupName.append(std::to_string(allocationId)).append(_DIR_DELIM);
    else
        _CGroupName = "";
    
    // If the controller directory was mising throw an exception.
    if ( !CheckFile( CGroup::CONTROLLER_DIR, _DIRECTORY ) )
    {
        std::string error(CGroup::CONTROLLER_DIR);
        error.append(" was not a valid directory.");

        throw CSMHandlerException( error, CSMERR_CGROUP_FAIL );
    }
}

void CGroup::DeleteCGroups( bool removeSystem )
{
    LOG( csmapi, trace ) << _LOG_PREFIX "DeleteCGroups Enter";
    
    // Walk the list of cgroups that could be created and attempt to delete them.
    for ( uint32_t controller = CG_CPUSET; 
            controller < csm_enum_max(csmi_cgroup_controller_t);
            ++controller )
    {
        DeleteCGroup( csmi_cgroup_controller_t_strs[controller], _CGroupName);

        // Only delete the system cgroup if this is not a step cgroup.
        // NEVER remove system cgroup.
        //if( removeSystem )
        //    DeleteCGroup( csmi_cgroup_controller_t_strs[controller], 
        //                    CGroup::SYSTEM_CGROUP, true);

    }

    LOG( csmapi, trace ) << _LOG_PREFIX "DeleteCGroups Exit";
}

void CGroup::SetupCGroups(int64_t cores)
{
    LOG( csmapi, trace ) << _LOG_PREFIX "SetupCGroups Enter";
    static_assert(CG_CPUSET == 0, "CG_CPUSET must have a value of 0.");

    // First create the control groups.
    const std::string groupCpuset = CreateCGroup( CGroup::CPUSET, _CGroupName );
    
    // Create the cgroups for all of the remaining controllers.
    for( uint32_t controller = CG_CPUSET + 1; 
            controller < csm_enum_max(csmi_cgroup_controller_t);
            ++controller )
    {
        CreateCGroup(csmi_cgroup_controller_t_strs[controller], _CGroupName);
    }

    //==============================================================================
    // Calculate core isolation.
    std::string sysCores   = "";
    std::string groupCores = "";
    GetCoreIsolation(cores, sysCores, groupCores);
    
    // Convert the projected memory
    //std::string allocProjected = std::to_string(KB_TO_B(projectedMemory));
   
    //==============================================================================

    // Define the cpuset parameters as a constant to make it easier to update.
    const char* CPU_EXCLUSIVE = "cpuset.cpu_exclusive";
    const char* MEM_EXCLUSIVE = "cpuset.mem_exclusive";
    const char* MEM_MIGRATE   = "cpuset.memory_migrate";
    const char* CPUS          = "cpuset.cpus";
    const char* MEMS          = "cpuset.mems";

    //const char* MEM_LIMIT     = "memory.limit_in_bytes";

    // Disable the exclusivity of the base cupset.
    WriteToParameter( CPU_EXCLUSIVE, CGroup::CPUSET_DIR, &DISABLE_CONTROLLER, sizeof(DISABLE_CONTROLLER));
    WriteToParameter( MEM_EXCLUSIVE, CGroup::CPUSET_DIR, &DISABLE_CONTROLLER, sizeof(DISABLE_CONTROLLER));
    WriteToParameter( MEM_MIGRATE,   CGroup::CPUSET_DIR, &ENABLE_CONTROLLER,  sizeof(ENABLE_CONTROLLER));

    // Write the new value to the group controller.
    WriteToParameter(        CPUS, groupCpuset, groupCores.c_str(), groupCores.size());
    WriteToParameter( MEM_MIGRATE, groupCpuset, &ENABLE_CONTROLLER, sizeof(ENABLE_CONTROLLER));
    
    CopyParameter( MEMS, CGroup::CPUSET_DIR, groupCpuset );

    //WriteToParameter( MEM_LIMIT, CGroup::MEM_DIR, allocProjected.c_str(), allocProjected.size() );
    // Generate the system cpuset if the cores are greater than zero.
    if ( cores > 0)
    {
        const std::string sysCpuset = CreateCGroup( CGroup::CPUSET, CGroup::SYSTEM_CGROUP );

        for( uint32_t controller = CG_CPUSET + 1;
            controller < csm_enum_max(csmi_cgroup_controller_t);
            ++controller )
        {
            CreateCGroup(csmi_cgroup_controller_t_strs[controller], CGroup::SYSTEM_CGROUP);
        }

        WriteToParameter(        CPUS, sysCpuset,  sysCores.c_str(),  sysCores.size() );
        WriteToParameter( MEM_MIGRATE, sysCpuset, &ENABLE_CONTROLLER, sizeof(ENABLE_CONTROLLER));

        CopyParameter( MEMS, CGroup::CPUSET_DIR, sysCpuset );
        MigrateTasks( CGroup::CPUSET_DIR, sysCpuset );

        // TODO Compute System CGroup limit.
    }

    LOG( csmapi, trace ) << _LOG_PREFIX "SetupCGroups Exit";
}

void CGroup::CreateCGroup( const char* cgroupName, 
                            csmi_cgroup_t* component, 
                            pid_t pid ) const
{
    LOG( csmapi, trace ) << _LOG_PREFIX "CreateCGroup Enter";
    
    if ( component == nullptr )
    {
        // Build the error for reporting the failure.
        std::string error("CreateCGroup; Component was not specified, unable to create cgroup ");
        error.append(cgroupName);

        throw CSMHandlerException( error, CSMERR_CGROUP_FAIL );
    }

    // Get the appropriate controller.
    const char* controller = csm_get_string_from_enum( csmi_cgroup_controller_t, component->type );
    
    // TODO Is there a better way to organize this?
    // --------------------------------------------------------

    // Construct the base cgroup path.
    std::string allocationCGroup(CONTROLLER_DIR);
    allocationCGroup.append(controller).append(_CGroupName);

    // EARLY RETURN
    // If the base cgroup doesn't exist, throw an error.
    if ( !CheckFile( allocationCGroup.c_str(), _DIRECTORY ) )
    {
        std::string error("CreateCGroup; Parent cgroup ");
        error.append(allocationCGroup).append(" was not mounted");

        throw CSMHandlerException( error, CSMERR_CGROUP_FAIL );
    }
    
    // --------------------------------------------------------

    // Build the step cgroup for CreateCgroup.
    std::string stepCGroup(_CGroupName);
    stepCGroup.append(cgroupName).append(_DIR_DELIM);

    // Create the specific cgroup. Has an early return if the cgroup already exists.
    stepCGroup = CreateCGroup( controller, stepCGroup);
    // --------------------------------------------------------

    // Write all of the values to the specified parameters.
    for( uint32_t i = 0; i < component->num_params; ++i )
    {
        WriteToParameter( component->params[i], stepCGroup,
                            component->values[i], strlen(component->values[i]) );
    }

    // If the pid was specified write to the tasks file.
    if ( pid != 0 )
    {
        // TODO reduce the number of calls.
        std::string pidStr = std::to_string(pid);
        WriteToParameter( "tasks", stepCGroup, pidStr.c_str(), pidStr.size() );
    }

    LOG( csmapi, trace ) << _LOG_PREFIX "CreateCGroup Exit";
}

void CGroup::DeleteCGroup( 
    csmi_cgroup_controller_t controller,
    const char* stepCGroupName ) const
{
    std::string cgroup(_CGroupName);
    cgroup.append(stepCGroupName);

    DeleteCGroup( csm_get_string_from_enum( csmi_cgroup_controller_t, controller), cgroup );
}

void CGroup::DeleteCGroup( 
    const char* stepCGroupName ) const
{
    std::string cgroup(_CGroupName);
    cgroup.append(stepCGroupName);

    for ( uint32_t controller = CG_CPUSET; 
            controller < csm_enum_max(csmi_cgroup_controller_t);
            ++controller )
    {
        DeleteCGroup( csmi_cgroup_controller_t_strs[controller], cgroup);
    }
}

void CGroup::MigratePid( pid_t pid ) const
{
    std::string pidStr = std::to_string(pid);

    for ( uint32_t controller = CG_CPUSET; 
            controller < csm_enum_max(csmi_cgroup_controller_t);
            ++controller )
    {
        // 0. Controller.
        std::string controllerPath(CONTROLLER_DIR);
        controllerPath.append(csmi_cgroup_controller_t_strs[controller]);
        controllerPath.append(_CGroupName);

        // 1. Pid
        WriteToParameter( "tasks", controllerPath, pidStr.c_str(), pidStr.size() );
    }
    
}

void CGroup::ConfigSharedCGroup( int32_t projectedMemory, int32_t numGPUs, int32_t numProcessors ) 
{
    std::string memCGroup(CONTROLLER_DIR);
    memCGroup.append(MEM).append(_CGroupName);

    std::string devCGroup(CONTROLLER_DIR);
    devCGroup.append(DEVICES).append(_CGroupName);

    std::string cpuCGroup(CONTROLLER_DIR);
    cpuCGroup.append(CPUSET).append(_CGroupName);

    std::string cpusetRoot(CONTROLLER_DIR);
    cpusetRoot.append(CPUSET);

    // Set the projected memory
    const char* MEM_LIMIT     = "memory.limit_in_bytes";
    std::string allocProjected = std::to_string(KB_TO_B(projectedMemory));
    WriteToParameter( MEM_LIMIT, memCGroup, allocProjected.c_str(), allocProjected.size() );
    // =========================================================================================== 
    
    // Restrict the GPUs
    // XXX Should CSM really be responsible for this?
    //const char*  BLOCK_GPU    = "c 195:* rwm";
    //const size_t BLOCK_LEN    = strlen(BLOCK_GPU);
    //const char* DEV_DENY      = "devices.deny";
    //const char* DEV_ALLOW     = "devices.allow";
    //WriteToParameter( DEV_DENY, devCGroup, BLOCK_GPU, BLOCK_LEN);
    // XXX Should we enable GPUs after this?
    // =========================================================================================== 

    // TODO move to a function?
    // Restrict the CPUS
    const char* CPUS  = "cpuset.cpus";

    int32_t threads, sockets, threadsPerCore, coresPerSocket;
    // Get the CPUS and do a sanity check.
    if ( GetCPUs( threads, sockets, threadsPerCore, coresPerSocket ) )
    {
        const int32_t cores = coresPerSocket * sockets; //threads / threadsPerCore;
        const int32_t maxSMT = threads / cores;

        bool availableCores[cores];
        for(int32_t i = 0; i < cores; ++i) availableCores[i] = true;
        
    
        DIR *sysDir =  opendir(cpusetRoot.c_str());  // Open the directory to search for subdirs.
        dirent *dirDetails;                         // Output struct for directory contents.
        
        // If the system directory could not be retrieved throw an exception.
        if ( !sysDir )
        {
            std::string error = "Unable to open cpuset directory: " + 
                cpusetRoot + " errno message: ";
            error.append(strerror(errno));

            LOG( csmapi, error ) << _LOG_PREFIX << error;
            throw CSMHandlerException( error, CSMERR_CGROUP_FAIL );
        }

        // Construct a shared pointer to let RAII handle the close in the event of a thrown error.
        std::shared_ptr<DIR> sysDirShared(sysDir, closedir);
        
        // Clear the cpuset
        WriteToParameter(CPUS, cpuCGroup, " ", 1);

        // Build the path for processing.
        std::string dirPath = cpusetRoot + _DIR_DELIM;
        while ( ( dirDetails = readdir( sysDir ) ) ) // While this assignment is successful.
        {
            // Only seek allocation cgroups.
            if ( !strncmp( dirDetails->d_name, ALLOC,  ALLOC_LEN) )
            {
                std::string path( dirPath );
                path.append( dirDetails->d_name ).append("/").append(CPUS);

                // Prep the string parse.
                char* coresReserved = strdup(ReadString(path).c_str());

                int32_t coreIdx = 0, endIdx  = 0;
                char* pEnd = coresReserved;

                do
                {
                    endIdx = coreIdx = (strtol( pEnd, &pEnd, 10 ) / maxSMT);

                    if (  *(pEnd) == '-' )
                    {
                        pEnd++;
                        endIdx = (strtol( pEnd, &pEnd, 10 ) / maxSMT);
                    }

                    for ( ; coreIdx <= endIdx && coreIdx < cores; ++coreIdx ) 
                    {
                        availableCores[coreIdx] = false;
                    }
                }  while ( *(pEnd++) );

                free(coresReserved);
            }
        }

        std::string cpuStr = "";
        threadsPerCore--;
        for ( int i = 0, assigned = 0, core =0;
                i < cores && assigned < numProcessors; ++i )
        {
            if ( availableCores[i] )
            {
                assigned++;
                core = i * maxSMT;
                cpuStr.append(std::to_string(core)).append("-")
                    .append(std::to_string(core + threadsPerCore)).append(",");

            }
        }

        WriteToParameter(CPUS, cpuCGroup, cpuStr.c_str(), cpuStr.size());
    } 
    else
    {
        // TODO Should this throw an exception?
        WriteToParameter(CPUS, cpuCGroup, " ", 1);
    }
}

int64_t CGroup::GetCPUUsage(const char* stepCGroupName) const
{
    // Build the usage path.
    const char* USAGE = "/cpuacct.usage";
    std::string usagePath(CGroup::CPUACCT_DIR);
    usagePath.append(_CGroupName).append(stepCGroupName).append(USAGE);

    return ReadNumeric( usagePath );
}

int64_t CGroup::GetMemoryMaximum(const char* stepCGroupName) const
{
    // Build the usage path.
    const char* USAGE = "/memory.max_usage_in_bytes";
    std::string usagePath(CGroup::MEM_DIR);
    usagePath.append(_CGroupName).append(stepCGroupName).append(USAGE);

    return ReadNumeric( usagePath );
}

//===================================================
//Private Functions
//===================================================
const std::string CGroup::CreateCGroup( 
    const char* controller, 
    const std::string& groupName ) const
{
    LOG( csmapi, trace ) << _LOG_PREFIX "CreateCGroup Enter";

    // Construct the base cgroup path.
    std::string controllerPath(CONTROLLER_DIR);
    controllerPath.append(controller);
    const char* basePath = controllerPath.c_str(); 

    // Construct the specialized cgroup path.
    std::string groupPath(controllerPath + groupName);
    const char* path = groupPath.c_str();    

    // EARLY RETURN
    // If the targeted path exists, return early.
    if ( CheckFile( path, _DIRECTORY ) ) return groupPath;

    // Attempt to make the cgroup, if it fails try to mount the controller, 
    // if the cgroup is still not mounting fail.
    bool retry = false;
    do 
    {
        // Verify that the controller directory exists.
        // If not, fail this execution.
        if( CheckFile(basePath, _DIRECTORY) ) 
        {
            errno = 0;
            int errCode = mkdir( path, _CGROUP_PERM );

            // If the directory was made, no retries are needed.
            // Else if there have been no retry attempts attempt to mount the cgroup.
            if( errCode == 0 ) retry = false;
            else if ( !retry )
            {
                // Attempt to mount the controller.
                errno = 0;
                retry = mount( _CGROUP_MOUNT,     basePath, 
                                _CGROUP_TYPE, _CGROUP_OPTS, controller ) == 0;

                // if the mount failed, log the failure and set the path to null.
                if( !retry )
                {
                    // Build the error for reporting the failure.
                    std::string error("Unable to mount cgroup '");
                    error.append(controller).append("' to ").append(basePath);
                    error.append(" errno message: ").append(strerror(errno));

                    //LOG( csmapi, error ) << error;
                    throw CSMHandlerException( error, CSMERR_CGROUP_FAIL );
                }
            }
        }
        else
        {
            // Build the error for reporting the failure.
            std::string error("Controller ");
            error.append(controller).append(" could not be mounted, the base path '");
            error.append(basePath).append(" was not a directory.");
            error.append(" errno message: ").append(strerror(errno));

            throw CSMHandlerException( error, CSMERR_CGROUP_FAIL );
        }
    } while( retry );

    LOG( csmapi, trace ) << _LOG_PREFIX "CreateCGroup Exit";
    return groupPath;
}

void CGroup::DeleteCGroup( 
    const char* controller, 
    const std::string& groupName, 
    bool migrateTasksUp ) const
{
    LOG( csmapi, trace ) << _LOG_PREFIX "DeleteCGroup Enter";

    // Build the controller path before the specialized group. 
    std::string controllerPath(CONTROLLER_DIR);
    controllerPath.append(controller);

    // The cgroup being deleted.
    std::string groupPath(controllerPath);
    groupPath.append(groupName);

    // EARLY RETURN : If the cgroup doesn't exists, don't continue.
    if( !CheckFile( groupPath.c_str( ), _DIRECTORY )) 
        return;

    // These both throw failure exceptions.
    // If the migrate flag is set, migrate the tasks to the root controller.
    // Else kill the tasks (This attempts to kill the tasks 5 times).
    if ( migrateTasksUp )
    {
        MigrateTasks( groupPath, controllerPath );
    }
    else
    {
        // Attempt to kill all the tasks _KILL_ATTEMPTS times.
        // If any tasks are left at the end throw an exception.
        int killAttempts = -1;
        int tasksFound   = 0;
        do
        {
            tasksFound = KillTasks( groupPath );
            killAttempts++;            
        }
        while ( tasksFound > 0 && killAttempts < _KILL_ATTEMPTS);

        // If the cgroup fails to kill all of the tasks throw an exception.
        if (tasksFound > 0 )
        {
            std::string error = 
                "DeleteCGroup; Unable to remove " + std::to_string(tasksFound) +
                " tasks after " +  std::to_string(killAttempts) + 
                " attempts in the " + groupPath + " cgroup.";
            throw CSMHandlerException( error, CSMERR_CGROUP_FAIL );
        }
    }
    
    // If this point is reached no critical errors occured in kill tasks or migrate, if
    // we can't remove the directory and there are no subdirs something went wrong and an 
    // exception should be thrown.
    errno = 0;
    if( rmdir(groupPath.c_str()) != 0 ) 
    {
        // 1. Determine if there are any other directories present. 
        DeleteChildren(controller, groupName, groupPath, migrateTasksUp); 

        // 2. If this is reached, all the sub dirs were processed.
        errno = 0;
        bool cgroup_removed = rmdir(groupPath.c_str()) == 0;
        if( !cgroup_removed )
        {
            // FIXME ThiS is a major slow down point!
            // EBUSY 
            LOG( csmapi, warning ) << _LOG_PREFIX "DeleteCGroup: cgroup directory removal failed, "
                "sleeping for 1 second and retrying. Directory : " << groupPath;

            // First try to sync the file system.
            errno = 0;
            std::string taskFile = groupPath + _CGROUP_TASK;
            int fileDescriptor = open( taskFile.c_str(),  O_WRONLY | O_CLOEXEC );

            if( fileDescriptor > 0 )
            {
                syncfs( fileDescriptor );
                close( fileDescriptor );
            }

            // FIXME Add a more robust fix to this block of code.
            // Then Sleep the rmdir for a second to let the task migration/kill go through.
            // This is not in the critical path, so a second slow down is not terrible.
            sleep(1);
            errno = 0;
            cgroup_removed = rmdir(groupPath.c_str()) == 0;

            // If the cgroup is still not removed throw an exception.
            if( !cgroup_removed )
            {
                std::string error = "DeleteCGroup; Unable to delete cgroup "
                    "(subdirectories were cleared): " + groupPath + " errno message: ";
                error.append(strerror(errno));

                LOG( csmapi, error ) << _LOG_PREFIX << error;
                throw CSMHandlerException( error, CSMERR_CGROUP_FAIL );
            }
        }
    }
    
    // If this is reached the cgroup can be assumed to be deleted.
    LOG( csmapi, trace ) << _LOG_PREFIX "DeleteCGroup Exit; " << groupPath << " removed.";
}

void CGroup::DeleteChildren( 
    const char* controller, 
    const std::string& groupName, 
    const std::string& groupPath, 
    bool migrateTasksUp ) const
{
    DIR *sysDir =  opendir(groupPath.c_str());  // Open the directory to search for subdirs.
    dirent *dirDetails;                         // Output struct for directory contents.
    
    // If the system directory could not be retrieved throw an exception.
    if ( !sysDir )
    {
        std::string error = "DeleteCGroup; Unable to delete cgroup: " + 
            groupPath + " errno message: ";
        error.append(strerror(errno));

        LOG( csmapi, error ) << _LOG_PREFIX << error;
        throw CSMHandlerException( error, CSMERR_CGROUP_FAIL );
    }

    // Construct a shared pointer to let RAII handle the close in the event of a thrown error.
    std::shared_ptr<DIR> sysDirShared(sysDir, closedir);
        
    // Build the path for processing.
    std::string dirPath = groupPath + _DIR_DELIM;
    while ( ( dirDetails = readdir( sysDir ) ) ) // While this assignment is successful.
    {
        // Ignore all hidden files.
        if ( dirDetails->d_name[0] != _HIDDEN_CHAR  )
        {
            switch(dirDetails->d_type) 
            {
                case DT_DIR: // This is a valid directory.
                {
                    // Recurse
                    DeleteCGroup(
                        controller,
                        groupName + _DIR_DELIM + dirDetails->d_name,
                        migrateTasksUp);
                    break;
                }
                case DT_UNKNOWN: // This might be a directory.
                {
                    // Verify the file is a directory.
                    std::string file = groupPath + _DIR_DELIM + dirDetails->d_name;
                    if (CheckFile( file.c_str( ), _DIRECTORY ))
                        DeleteCGroup(
                            controller,
                            groupName + _DIR_DELIM + dirDetails->d_name,
                            migrateTasksUp);
                    break;
                }
                default:
                    // Do nothing
                    break;
            }
        }
    }
}

void CGroup::WriteToParameter( 
    const char* parameter, 
    const std::string& groupController, 
    const void* value, 
    const size_t valueLen )
{
    //LOG( csmapi, trace ) << _LOG_PREFIX "WriteToParameter Enter";

    // Build the file name
    std::string parameterPath(groupController);
    parameterPath.append(parameter);
   
    // Open the parameter file.
    errno=0;
    int fileDescriptor = open( parameterPath.c_str(),  O_WRONLY | O_CLOEXEC );
    if( fileDescriptor >= 0 )
    {
        errno=0;
        write( fileDescriptor, value, valueLen );
        int errorCode = errno;
        close( fileDescriptor );
        
        // Build a verbose error for the user.
        if ( errorCode != 0 )
        {
            csmi_cmd_err_t err = CSMERR_CGROUP_FAIL;

            std::string error("Unable to write to parameter ");
            error.append(parameterPath);

            if( value ) 
                error.append("; Value : ").append(std::string((const char*)value, valueLen));
            
            error.append("; Error : ").append(strerror(errorCode)); 
            switch (errorCode){
                case EINVAL:
                    error.append("; This error typically means the value written to the"
                        " cgroup parameter was invalid.");
                    err = CSMERR_CGROUP_EINVAL;
                    break;
                case EACCES:
                    error.append("; This error typically means the resources being requested are "
                        "invalid.");
                    err = CSMERR_CGROUP_EACCES;
                    break;
            }
            //LOG( csmapi, error ) << _LOG_PREFIX << error;

            throw CSMHandlerException( error, err );
        }
    }
    else
    {
        std::string error = "WriteToParameter; Error unable to open parameter: " +
            parameterPath + "; Error: ";
        error.append(strerror(errno));

        //LOG( csmapi, error ) << _LOG_PREFIX << error;
        throw CSMHandlerException( error, CSMERR_CGROUP_FAIL );
    }

    //LOG( csmapi, trace ) << _LOG_PREFIX "WriteToParameter Exit";
}

int64_t CGroup::ReadNumeric( const std::string component ) const 
{
    int64_t value = -1;
    try
    { 
        std::ifstream sourceStream(component);
        if ( sourceStream.is_open() )
        {
            std::string line;
            std::getline (sourceStream, line);
            value= strtoll(line.c_str(), nullptr, 10);
        }
    }
    catch (const std::system_error& e)     // Wrap the errors in the CSMHandlerException.
    {
        LOG( csmapi, warning) << "Read error detected when gathering numeric value " << component;
    }

    return value;
}

std::string CGroup::ReadString( const std::string component ) 
{
    std::string value = "";
    try
    { 
        std::ifstream sourceStream(component);
        if ( sourceStream.is_open() )
        {
            std::getline (sourceStream, value);
        }
    }
    catch (const std::system_error& e)     // Wrap the errors in the CSMHandlerException.
    {
        LOG( csmapi, warning) << "Read error detected when gathering string value " << component;
    }

    return value;
}


uint64_t CGroup::MigrateTasks( 
    const std::string& sourceGroup, 
    const std::string& targetGroup ) const
{
    LOG( csmapi, trace ) << _LOG_PREFIX "MigrateTasks Enter";

    // Build the tasks strings.
    std::string sourceTasks(sourceGroup);
    sourceTasks.append(_CGROUP_TASK);

    std::string targetTasks(targetGroup);
    targetTasks.append(_CGROUP_TASK); 

    // The number of tasks migrated.
    uint64_t numMigrated = 0;
    uint64_t failedMigration = 0;

    // The file descriptor describing the target task directory.
    int fileDescriptor = -1;
    
    // Open a connection.
    try 
    {
        std::ifstream sourceStream(sourceTasks);
        if ( ! sourceStream.is_open() )
        {
            std::string error = "MigrateTasks; Could not open target file: " + 
                sourceTasks + " errno message: ";
            error.append(strerror(errno));

            LOG( csmapi, error ) << _LOG_PREFIX << error;
            throw CSMHandlerException(error, CSMERR_CGROUP_FAIL);
        }

        // Open the file descriptor.
        fileDescriptor = open( targetTasks.c_str(),  O_WRONLY | O_CLOEXEC );
        if( fileDescriptor < 0 )
        { 
            std::string error = "MigrateTasks; Could not open target file descriptor: " + 
                targetTasks + " errno message: ";
            error.append(strerror(errno));

            LOG( csmapi, error ) << _LOG_PREFIX << error;
            throw CSMHandlerException(error, CSMERR_CGROUP_FAIL);
        }

        // The write function appears to be the only way to transfer the tasks.
        std::string line;
        while ( std::getline (sourceStream, line) )
        {
            errno=0;
            write( fileDescriptor, line.c_str(), line.size() );

            // TODO Do something with this failure.
            // If the task migrated, increment the numMigrated.
            if( errno == 0 ) 
                numMigrated++;
            else
            {
                failedMigration++;
                LOG( csmapi, trace ) << "pid " << line << 
                    " could not be transfered: " << strerror(errno);
            }
        }
        syncfs(fileDescriptor);

        close( fileDescriptor );
    } 
    catch (const std::system_error& e)     // Wrap the errors in the CSMHandlerException.
    {
        std::string error = "MigrateTasks; Error detected in transfer of pids from " +
            sourceTasks + " to " + targetTasks + " errno message: ";
        error.append(strerror(errno)).append(" exception message: ").append(e.what());

        // If the targetTasks file was opened, make sure it was closed before throwing the 
        // customized exception.
        if ( fileDescriptor >= 0 )
        {
            close( fileDescriptor );
        }

        LOG( csmapi, error ) << _LOG_PREFIX << error;
        throw CSMHandlerException( error, CSMERR_CGROUP_FAIL );
    }

    LOG( csmapi, trace ) << _LOG_PREFIX "MigrateTasks Exit";

    return failedMigration;
}

uint64_t CGroup::KillTasks( const std::string& controlGroup ) const
{
    LOG( csmapi, trace ) << _LOG_PREFIX "KillTasks Enter";

    // Build the taskfile string.
    std::string taskFile(controlGroup);
    taskFile.append(_CGROUP_TASK);

    // The number of tasks in the file.
    uint64_t numTasks = 0;

    // Open a connection.
    try 
    {
        std::ifstream sourceStream(taskFile);
        if ( ! sourceStream.is_open() )
        {
            std::string error = "KillTasks; Could not open target file: " + taskFile +
                " errno message: ";
            error.append(strerror(errno));

            LOG( csmapi, error ) << _LOG_PREFIX << error;
            throw CSMHandlerException(error, CSMERR_CGROUP_FAIL);
        }
        
        // The write function appears to be the only way to transfer the tasks.
        std::string line;
        while ( std::getline (sourceStream, line) )
        {
            long pid = stol(line);

            errno = 0;
            kill(pid,_TASK_KILL);

            // TODO Do something with this failure.
            if( errno != 0 )
            {
                LOG( csmapi, warning ) << "pid " << line << 
                    " could not be killed: " << strerror(errno);
            }
            numTasks++;
        }
        
        // XXX This makes it work, but we need to stress test it.
        int fileDescriptor = open( taskFile.c_str(),  O_WRONLY | O_CLOEXEC );
        if( fileDescriptor > 0 )
        {
            syncfs( fileDescriptor );
            close( fileDescriptor );
        }
    } 
    catch (const std::system_error& e)     // Wrap the errors in the CSMHandlerException.
    {
        std::string error = "KillTasks; Error detected in killing pids in " +
            taskFile + " errno message: ";
        error.append(strerror(errno)).append(" exception message: ").append(e.what());

        LOG( csmapi, error ) << _LOG_PREFIX << error;
        throw CSMHandlerException( error, CSMERR_CGROUP_FAIL );
    }

    LOG( csmapi, trace ) << _LOG_PREFIX "KillTasks Exit";

    return numTasks;
}

void CGroup::CopyParameter( const char* parameter, const std::string& sourceGroup, 
    const std::string& targetGroup ) const
{
    LOG( csmapi, trace ) << _LOG_PREFIX "CopyParameter Enter";

    // Build the tasks strings.
    std::string sourceTasks(sourceGroup);
    sourceTasks.append(parameter);

    std::string targetTasks(targetGroup);
    targetTasks.append(parameter);
    
    // Open a connection to the files and copy source to target.
    try 
    {
        std::ifstream sourceStream(sourceTasks);
        std::ofstream targetStream(targetTasks);

        targetStream << sourceStream.rdbuf();

    } 
    catch (const std::exception& e) // Wrap the errors in the CSMHandlerException.
    {
        std::string error = "CopyParameter; Error detected in copying the content of " +
            sourceTasks + " to " + targetTasks + " errno message: ";
        error.append(strerror(errno)).append(" exception message: ").append(e.what());

        LOG( csmapi, error ) << _LOG_PREFIX << error;
        throw CSMHandlerException( error, CSMERR_CGROUP_FAIL );
    }

    LOG( csmapi, trace ) << _LOG_PREFIX "CopyParameter Exit";
}

bool CGroup::CheckFile( const char* path, bool isDir ) const
{
    struct stat fileDetails;
    // Perform some boolean logic, if stat fails, the lazy evaluation will stop the check.
    // Then perform a negated xor checking the file mode against is dir.
    return (stat( path, &fileDetails ) == 0) && 
         (!( ( (fileDetails.st_mode & S_IFDIR) > 0 ) ^ isDir) );
}

bool CGroup::GetCPUs( int32_t &threads, int32_t &sockets, 
    int32_t &threadsPerCore, int32_t &coresPerSocket)
{
    // Success of the execution
    bool success = true;
    threads = -1;
    sockets = -1;
    threadsPerCore = -1;
    coresPerSocket = -1;

    // Open a pipe to execute the gpfs command.
    std::array<char, 128> buffer;
    std::stringstream outputStream;
    FILE* pipe = popen( "lscpu", "r" );
    
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
            // TODO is there a better way?
            int limit = outputLine.find_first_of(':');
            if (limit > 1 && 
                ( outputLine[0] == 'C' || outputLine[0] == 'S' || outputLine[0] == 'T' ))
            {
                switch(outputLine[1])
                {
                    case 'o':
                        if ( outputLine.find("Socket(s):") != std::string::npos)
                        {
                            sockets = strtol(outputLine.substr(limit+1).c_str(), nullptr,10);
                        }
                        else if ( outputLine.find("Core(s) per socket:") != std::string::npos)
                        {
                           coresPerSocket = strtol(outputLine.substr(limit+1).c_str(), nullptr,10); 
                        }
                        break;
                    case 'P':
                        if ( outputLine.find("CPU(s):") != std::string::npos)
                        {
                            threads = strtol(outputLine.substr(limit+1).c_str(),nullptr,10); 
                        }
                        break;
                    case 'h':
                        if ( outputLine.find("Thread(s) per core:") != std::string::npos)
                        {
                            threadsPerCore = strtol(outputLine.substr(limit+1).c_str(),nullptr,10);
                        }
                        break;
                }
            }
        }
    }
    else
    {
        success = false;
    }
    return success;
}

void CGroup::GetCoreIsolation( int64_t cores, std::string &sysCores, std::string &groupCores)
{
    int32_t threads, sockets, threadsPerCore, coresPerSocket;
    // Get the CPUS and do a sanity check.
    if ( GetCPUs( threads, sockets, threadsPerCore, coresPerSocket ) && 
            threads > 0        && 
            sockets > 0        && 
            threadsPerCore > 0 && 
            coresPerSocket > 0 &&
            (threads % sockets) == 0)
    {
        // Assemble the thread grouping 
        #define assembleGroup( string )                                        \
            string.append(std::to_string(groupStart)).append(DELIM);           \
            if ( groupStart != thread - 1 )                                    \
            {                                                                  \
                string.append(std::to_string(thread - 1)).append(_GROUP_DELIM);\
            }

#ifdef VM_DEVELOPMENT
        sockets        = 1;
        threadsPerCore = 1;
        coresPerSocket = 4;
        const int32_t threadsPerCoreMax    = 1;
        const int32_t threadsPerCoreOffset = 0;
        const char* DELIM = _RANGE_DELIM;
#else
        // Determine the delimiter based on the number of PerSocket/hreads per core.
        const char* DELIM = threadsPerCore > 2 ? _RANGE_DELIM : _GROUP_DELIM;
        // Maximum number of logical cores per core.
        const int32_t threadsPerCoreMax    = (threads / (sockets * coresPerSocket));
        // Difference between the maximum and actual thread count per core.
        const int32_t threadsPerCoreOffset = (threads / (sockets * coresPerSocket)) - threadsPerCore;
#endif
        // If SMT is not fully enabled (e.g. SMT=4 on the GTW), set the range to be disjointed. 
        const bool rangeDisjointed = threadsPerCoreMax != threadsPerCore;
        // Start of the system cgroup on the socket.
        const int32_t SYSTEM_CORE_START = coresPerSocket - cores;

        // For each socket build the system and allocation groups.
        for( int32_t socket =0, thread = 0, groupStart = thread; 
                socket < sockets; ++socket )
        {
            int32_t core = 0;
            int32_t continuousRangeStart = groupStart;

            // Then isolate the allocation cgroup.
            for(; core < SYSTEM_CORE_START; ++core ) 
            {
                thread += threadsPerCore;
                if ( rangeDisjointed )
                {
                    assembleGroup(groupCores);
                }
                thread +=threadsPerCoreOffset;
                groupStart = thread;
            }
            // If the range was not disjointed assemble the range one time.
            if ( !rangeDisjointed )
            {
                groupStart = continuousRangeStart;
                assembleGroup(groupCores);
            }

            // Start the continuous range.
            continuousRangeStart = groupStart = thread;
            // First the socket should get the system cgroup.
            for(; core < coresPerSocket; ++core ) 
            {
                thread += threadsPerCore;
                if ( rangeDisjointed )
                {
                    assembleGroup(sysCores);
                }
                thread += threadsPerCoreOffset;
                groupStart = thread;
            } 
            // If the range was not disjointed assemble the range one time.
            if ( !rangeDisjointed )
            {
                groupStart = continuousRangeStart;
                assembleGroup(sysCores);
            }
            groupStart = thread;
        }
    }
    else
    {
        // Build the error for reporting the failure.
        std::string error("CreateCGroup; CPU data was invalid, couldn't create cgroup." );
        throw CSMHandlerException( error, CSMERR_CGROUP_FAIL );
    }

    // Clear the last character.
    groupCores.back() = ' ';
    sysCores.back()   = ' ';
}

} // End namespace helpers
} // End namespace daemon
} // End namespace csm

