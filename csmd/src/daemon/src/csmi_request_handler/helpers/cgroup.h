/*================================================================================
   
    csmd/src/daemon/src/csmi_request_handler/helpers/cgroup.h

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
 
================================================================================*/

/**@file cgroup.h
 *
 * An interface to interact with linux cgroups. 
 *
 * @author John Dunham (jdunham@us.ibm.com)
 */

#ifndef _CGROUP_HELPERS_H_
#define _CGROUP_HELPERS_H_

//#include <systemd/sd-bus.h>
#include <string>
#include <inttypes.h>
#include "csmi/include/csmi_type_wm.h"
#include "csm_handler_exception.h"
#include <vector>

namespace csm {
namespace daemon {
namespace helper {

/**
 * @brief A class facilitating cgroup interactions.
 * @todo more granular error reporting?
 */
class CGroup
{
private: 
    static const char* SYSTEM_CGROUP;   ///< The name of the system cgroup.
    static const char* ALLOC_CGROUP;    ///< The name of the allocation cgroup.
    static const char* CONTROLLER_DIR;  ///< The location of the cgroup controller directory.
    static const char* CPUSET;       
    static const char* CPUACCT;       
    static const char* MEM;       
    static const char* DEVICES;       
    static const std::string CPUSET_DIR;
    static const std::string CPUACCT_DIR;
    static const std::string MEM_DIR;
    
    std::string _CGroupName;            ///< The name of the allocation/step cgroup.
    int16_t     _smtMode;               ///< The smt mode for the current cgroups.
    bool        _enabled;               ///< Flag to determine if the cgroup module is enabled.

public:
    /**
     *  @return True if the cgroup jitter mitigation is enabled.
     */
    bool IsEnabled() { return _enabled; }

    /**
     *  @brief Scans the cpuset
     */
    static bool RepairSMTChange();

    /** @brief Constructs the cgroup string for the allocation/step and verifies the cgroup can function.
     *
     * @param[in] allocationId The allocation associated with the control group.
     *
     * @throw CSMHandlerException If the control group root directory could not be found/wasn't a directory.
     */
    explicit CGroup( int64_t allocationId );
    
    /** @brief Deletes any cgroups associated with this object. 
     *
     * @note This should be used to remove the Allocation cgroup.
     *
     * @param[in] removeSystem If set this flag will remove the system cgroup as well.
     *
     * @throw CSMHandlerException If any of the cgroup deletes failed.
     */
    void DeleteCGroups( bool removeSystem = false);

    /** @brief Creates the cgroups for an allocation.
     *
     * @param[in] cores The number of cores to isolate in the system cgroup.
     * @param[in] smtMode The smt mode for the allocation cgroup.
     * @param[in] projectedMemory The amount of memory to allocate for the allocation cgroup.
     *                              This is in kB
     *
     * @throw CSMHandlerException If the core isolation failed.
     */
    void SetupCGroups( int64_t cores, int16_t smtMode=0 );
    
    /**@brief Creates a CGroup for the specified component.
     *
     * @param[in] cgroupName The name of the cgroup to create.
     * @param[in] component  A cgroup struct containing the controller type,
     *                         parameters and values.
     * @param[in] pid        A PID to place in the tasks file of the cgroup, optional.
     *
     * @throw CSMHandlerException If the cgroup could not be created.
     */
    void CreateCGroup( const char* cgroupName, 
                        csmi_cgroup_t* component, 
                        pid_t pid = 0 ) const ;


    /** @brief Deletes the specified cgroup.
     *
     * @param[in] controller     The controller to remove the cgroup from.
     * @param[in] stepCGroupName The child cgroup to remove, if not specified the 
     *                              allocation cgroup is removed.
     */
    void DeleteCGroup( csmi_cgroup_controller_t controller, const char* stepCGroupName = "" ) const;

    /** @brief Deletes the specified cgroup.
     * Deletes any controllers associated with the supplied group name from the 
     * list of supported controllers.
     *
     * @param[in] stepCGroupName The child cgroup to remove, if not specified the 
     *                              allocation cgroup is removed.
     */
    void DeleteCGroup( const char* stepCGroupName = "") const;


    /** @brief Migrates a pid to all of the available allocation CGroups.
     *
     * @param[in] pid The process id to be migrated.
     *
     * @throw CSMHandlerException If the parameter couldn't be written.
     */
    void MigratePid( pid_t pid ) const;


    /**
     * @brief Wait for the pid to appear in the task list of the allocation cgroup*
     * This function is to be used in conjunction with @ref MigratePid asynchronously.
     *
     * @param[in] pid The process id to to scan the tasks file for.
     * @param[in] sleepAttempts The number of times to wait for the pid write to be a success.
     * @param[in] sleepTime The time in seconds to wait between pid tests.
     *
     * @return True if the pid was successfully migrated.
     */
    bool WaitPidMigration(pid_t pid, uint32_t sleepAttempts=3, uint32_t sleepTime=1) const;

    /**
     * @brief Performs the configuration step on the cgroups created by @ref SetupCGroups.
     * Restricts memory and number of processors.
     *
     * @TODO implement device logic.
     *
     * @param[in] projectedMemory The
     *
     */
    void ConfigSharedCGroup( int32_t projectedMemory, int32_t numGPUs, int32_t numProcessors );

    /** @brief Retrieve the CPU usage for the cgroup. Accesses cpuacct.usage.
     *
     * @param[in] stepCGroupName Optional, step cgroup.
     *
     * @note nanoseconds is max 292 years, we should be fine - John.
     * @return The cpu time in nano seconds used by the cgroup.
     */
    int64_t GetCPUUsage(const char* stepCGroupName = "") const;

    /** 
     * @brief Retrieve the CPU usage for the cgroup on a per physical core basis; returns the same number of entries independent of SMT mode. 
     *
     * @param[out] cpuUsage A vector with one entry per physical core containing the cpu time in nanoseconds for each core used by the cgroup. 
     * @param[in] stepCGroupName Optional, step cgroup.
     *
     * @note nanoseconds is max 292 years, we should be fine - John.
     * @return true if successful, false if unsuccessful
     */
    bool GetDetailedCPUUsage(std::vector<int64_t> &cpuUsage, const char* stepCGroupName = "") const;

    /** @brief Retrieve the maximum Memory usage for the cgroup. Accesses memory.max_usage_in_bytes.
     *
     * @param[in] stepCGroupName Optional, step cgroup.
     *
     * @return  The maximum memory usage in bytes.
     */
    int64_t GetMemoryMaximum(const char* stepCGroupName = "") const;

    /** 
     * @brief Removes all old cgroups in the cgroup directories.
     * @param[in] removeSystem If set this flag will remove the system cgroup as well.
     *
     * @throw CSMHandlerException If any of the cgroup deletes failed.
     */
    void ClearCGroups( bool removeSystem = false);


private:
    /** @brief Attempts to create a Control Group using the kernel files.
     *
     * @param[in] controller The controller for the cgroup (e.g. cpuset, memory, etc.).
     * @param[in] groupName The name of the group to add.
     *
     * @return The full control group directory (e.g. "/sys/fs/cgroup/cpuset/allocation_1").
     * 
     * @throw CSMHandlerException If the control group could not be mounted.
     */
    const std::string CreateCGroup(const char* controller, const std::string& groupName) const;

    /** @brief Attempts to delete a Control Group using the kernel files.
     *
     * @param[in] controller The controller for the cgroup (e.g. cpuset, memory, etc.).
     * @param[in] groupName The name of the group to remove.
     * @param[in] migrateTasksUp Migrates the tasks to the system cgroup.
     * 
     * @throw CSMHandlerException If the cgroup delete failed.
     */
    void DeleteCGroup(const char* controller, const std::string& groupName, 
        bool migrateTasksUp = false) const;

    /** @brief Iterates over the sub directories in the specified path, running DeleteCGroup on them.
     *
     * @param[in] controller The controller for the cgroup (e.g. cpuset, memory, etc.).
     * @param[in] groupName The name of the group to remove.
     * @param[in] groupPath The directory to delete.
     * @param[in] migrateTasksUp Migrates the tasks to the system cgroup.
     *
     * @throw CSMHandlerException If the cgroup delete failed in any of the child cgroups.
     */
    void DeleteChildren(const char* controller, const std::string& groupName, 
        const std::string& groupPath, bool migrateTasksUp = false) const;

    /** @brief Turns a CPU on or off depending on the supplied @p online parameter.
     *
     * @param[in] thread The thread to turn on or off.
     * @param[in] online {0,1} Whether or not the cpu should be turned on or off.
     *
     * @return The Errno of the operation.
     */
    static int CPUPower(const uint32_t thread, const char online);

    /**
     * @brief Applies the IRQ affinity using the irqbalance daemon in oneshot mode.
     *
     * @param[in] bannedCPUs A list of banned CPUs as hex strings, sets env `IRQBALANCE_BANNED_CPUS`.
     * @param[in] startIRQBalance If set true the IRQBalance daemon will be started, if set false
     *  it will stop the IRQBalance Daemon.
     *
     *
     * @return 0 on success, errno on failure.
     */
    static int IRQRebalance(const std::string bannedCPUs, bool startIRQBalance);

    /** @brief Write the supplied value to the specified parameter for the controller.
     *
     * @todo update controller strings?
     *
     * @param[in] parameter The parameter to write to (e.g. "/cpuset.cpus"), assumed to have a 
     *       leading '/' character.
     * @param[in] groupController The controller containing the parameter 
     *      (e.g. "/sys/fs/cgroup/cpuset/allocation_1")
     * @param[in] value The value to write to the parameter (typically either an int or char*).
     * @param[in] valueLen The length of the written value.
     *
     * @throw CSMHandlerException If the parameter path couldn't be open or written to.
     */
    static void WriteToParameter( const char* parameter, const std::string& groupController,
        const void* value, const size_t valueLen );

    /**
     * @brief Reads a numeric value from the supplied component.
     *
     * @param[in] component The component to extract the numeric value from.
     * 
     * @return The numeric value of the specified component.
     */
    int64_t ReadNumeric( const std::string component ) const;
    
    /** 
     * @brief Reads a string value from the supplied component.
     *
     * @param[in] component The component to extract the string value from.
     * 
     * @return The string value of the specified component.
     */
    static std::string ReadString( const std::string component );

    /** @brief Moves the task pids in the source group to the target group.
     *
     * @note This is distinct from @ref CopyParameter, because tasks need to use the *write* function.
     *
     * @param[in] sourceGroup The source control group.
     * @param[in] targetGroup The target control group.
     * @return The total number of tasks that the migrate function was run against.
     *
     * @throw CSMHandlerException If the task migration failed.
     */
    uint64_t MigrateTasks( const std::string& sourceGroup, const std::string& targetGroup ) const;

    /** @brief Kills all of the tasks in the @p controlGroup tasks file.
     *
     * @note SIGKILL is used in killing the tasks.
     *
     * @param[in] controlGroup The control group to kill the tasks in.
     * @param[in] printPids If set this function will print the pids as an error log.
     * @return The total number of tasks that the kill function was run against.
     *
     * @throw CSMHandlerException If the tasks could not be killed.
     */
    uint64_t KillTasks( const std::string& controlGroup, bool printPids = false ) const;

    /** @brief Copies the contents of a parameter from one group to another.
     *
     * @todo Profile this vs. @ref MigrateTasks
     *
     * @param[in] parameter The parameter to copy from the source to target group.
     * @param[in] sourceGroup The source control group.
     * @param[in] targetGroup The target control group. 
     *
     * @throw CSMHandlerException If the parameter copy failed.
     */
    void CopyParameter(  const char* parameter, const std::string& sourceGroup, 
        const std::string& targetGroup ) const;

    /** @brief Determine if a file exists and is of the correct type (e.g. file or directory)
     *
     *  @param[in] path The fully qualified path to the file ( or directory ).
     *  @param[in] isDir (Optional) Specifies whether this file should be checked for being a dir.
     *
     *  @return True if the file exists, and is of the correct type (dir or not dir).
     */
    bool CheckFile( const char* path, bool isDir = false ) const;

    /** @brief Get the blink settings from the existing cgroups.
     *
     * @param[in] cpuCount The CPU count for the node (for all sockets).
     * @param[in] maxSMT The maximum SMT for the node (effectively the threads per core).
     * @param[in] coreIsolation The core isolation factor set by the user (per socket).
     *
     * @return A numeric value indicating one of three behaviors:
     *  - -1 : The allocation cgroup cores need to be blinked to clear old processes.
     *  -  0 : No blinking should occur.
     *  - >0 : The System Cgroup needs blinking, returns the previous isolation factor.
     */
    int32_t GetBlinkSettings(int32_t cpuCount, int32_t maxSMT, int32_t coreIsolation) const;

    /**
     * @brief Builds the core isolation strings for the allocation and system cgroups.
     *
     * @param[in] cores The number of cores to isolate for the system cgroup.
     *
     * @param[out] sysCores The output string for the system cgroup.
     * @param[out] groupCores The output string for the allocation cgroup.
     */
    void GetCoreIsolation(int64_t cores, std::string &sysCores, std::string &groupCores);

public:
    /** @brief Retrieve the node's cpu counts.
     *
     * @todo This should be made more robust, remove pipe dependency.
     *
     * @param[out] threads The number of configured cpus.
     * @param[out] sockets The number of configured CPU sockets.
     * @param[out] threadsPerCore The number of threads per CPU core.
     * @param[out] coresPerSocket The number of cores per CPU socket.
     *
     * @return True if the CPU information was grabbed successfully.
     */
    static bool GetCPUs( int32_t &threads, int32_t &sockets, 
        int32_t &threadsPerCore, int32_t &coresPerSocket);
};

} // End namespace helpers
} // End namespace daemon
} // End namespace csm

#endif
