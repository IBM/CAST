/*================================================================================
   
    csmd/src/daemon/src/csmi_request_handler/helpers/Agent.cc

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
 
================================================================================*/

#include <stdio.h>
#include <fcntl.h>     ///< Open function
#include <string>
#include <dirent.h>    ///< DIR and Directory sys calls.
#include "logging.h"   ///< CSM logging.
#include <pwd.h>
#include <grp.h> 
#include <sys/types.h>
#include <sys/wait.h>
#include "cgroup.h"

namespace csm {
namespace daemon {
namespace helper {

#define _DIR_DELIM "/"
#define _HIDDEN_CHAR '.'
#define _LOG_FD 3 ///< The File descriptor for logging.

// FD 
#define _STDIN  0
#define _STDOUT 1
#define _STDERR 2

static const std::string PROC_DIR = "/proc/";
static const std::string FD_DIR = "/fd/";

bool ClearFileDescriptors( pid_t pid)
{
    // Build the proc path.
    std::string procPath(PROC_DIR);
    procPath.append(std::to_string(pid)).append(FD_DIR);
    
    // Open the proc directories.
    DIR    *procDir = opendir(procPath.c_str());
    dirent *procDetails;

    // Verify the proc dir opened.
    if ( !procDir )
    {
        return false;
    }

    std::shared_ptr<DIR> procDirShared(procDir, closedir);
    while ( ( procDetails = readdir( procDir ) ) )
    {
        if (procDetails->d_name[0] != _HIDDEN_CHAR)
        {
            int fd = atoi(procDetails->d_name);
            if( fd >= _LOG_FD )
            {
                close(fd);
            }
        }
    }
    return true;
    
}

void ProcessExit(int sig)
{
    errno=0;
    while(waitpid((pid_t)(-1), 0, WNOHANG) > 0) {}
}

int ForkAndExecCapture( char * const argv[], char **output, uid_t user_id, int timeout_seconds)
{
    #define FROM_CHILD 0
    #define TO_PARENT 1
    #define BUFFER_SIZE 1024
    int uni_pipe[2];
    int status = 0;
    int rc;
    pipe(uni_pipe);
    pid_t execPid = fork();
    
    // Clear old fd first.
    if ( execPid == 0  )
    {
        // Get this pid to be sure the correct fd is closed.
        //pid_t forkPid = getpid();
        //ClearFileDescriptors(forkPid);
        // Set the userid.
        passwd *pw = getpwuid(user_id);
        if (pw)
        {
            #define MaxGroups 1024  // LDAP limit is slightly less than 1024
            int ngroups = MaxGroups;
            gid_t groups[MaxGroups];
            
            if( getgrouplist(pw->pw_name, pw->pw_gid, groups, &ngroups) != -1)
            {
                rc = setregid(pw->pw_gid, pw->pw_gid);
                if(rc)
                {
                    // \todo bail out due to failure
                    _Exit(-1);
                }
                rc = setgroups(ngroups, groups);
                if(rc)
                {
                    // \todo bail out due to failure
                    _Exit(-1);
                }
            }
            rc = setreuid(user_id, user_id);  // must set UID last
            if(rc)
            {
                // \todo bail out due to failure
                _Exit(-1);
            }
        }
        else
        {
            // \todo pw lookup failure, exit
            _Exit(-1);
        }
    }
    
    // Dulplicate the file descriptors for processing.
    if ( execPid == 0  )
    {
        close(uni_pipe[FROM_CHILD]);
        dup2(uni_pipe[TO_PARENT], STDOUT_FILENO);
        dup2(uni_pipe[TO_PARENT], STDERR_FILENO);
        close(uni_pipe[TO_PARENT]);
        alarm(timeout_seconds);
        
        _Exit(execv(*argv, argv));
    }
    else // Save the output.
    {
        *output = nullptr;
        close(uni_pipe[TO_PARENT]);
        

        size_t buffer_read= 0,buffer_total= 1;
        char buffer[BUFFER_SIZE];
        std::string fd_out("");        

        // TODO might be some buffer weirdness here.
        while( (buffer_read = read( uni_pipe[FROM_CHILD], buffer, BUFFER_SIZE) ) )
        {
            buffer_total += buffer_read;
            fd_out.append(buffer, buffer_read);
        }
        close(uni_pipe[FROM_CHILD]);

        *output = (char*)malloc(buffer_total);
        strcpy(*output, fd_out.c_str());

        // Wait for either the timeout or the exec.
        int rc = waitpid(execPid, &status, 0);

        // Kill any return code to make sure it goes down.
        if (rc != -1) 
        {
            kill(rc, SIGKILL);
        }
    }

    return WEXITSTATUS(status);
}

int ForkAndExecAllocationCGroup(char * const argv[], uint64_t allocation_id, uid_t user_id)
{
    #define FROM_CHILD 0
    #define TO_PARENT 1
    #define BUFFER_SIZE 1024
    int status = 0;
    int rc;
    pid_t execPid = fork();
    
    if ( execPid == 0  )
    {
        execPid = fork();
        if(execPid != 0)
        {
            // Setup the cgroup.
            csm::daemon::helper::CGroup cgroup = csm::daemon::helper::CGroup( allocation_id );
            cgroup.MigratePid(execPid);
            _Exit(0);
        }

        // Set the userid.
        passwd *pw = getpwuid(user_id);
        if (pw)
        {
            #define MaxGroups 1024  // LDAP limit is slightly less than 1024
            int ngroups = MaxGroups;
            gid_t groups[MaxGroups];
            
            // TODO make this into a function.
            if( getgrouplist(pw->pw_name, pw->pw_gid, groups, &ngroups) != -1)
            {
                rc = setregid(pw->pw_gid, pw->pw_gid);
                if(rc)
                {
                    // \todo bail out due to failure
                    _Exit(-1);
                }
                rc = setgroups(ngroups, groups);
                if(rc)
                {
                    // \todo bail out due to failure
                    _Exit(-1);
                }
            }
            rc = setreuid(user_id, user_id);  // must set UID last
            if(rc)
            {
                // \todo bail out due to failure
                _Exit(-1);
            }


            // Wait on the PID migration then execute.
            try
            {
                csm::daemon::helper::CGroup cgroup = csm::daemon::helper::CGroup( allocation_id );
                
                LOG(csmapi, trace) << "Waiting on pid #" << getpid() << " migration allocation: " << allocation_id;
                if(cgroup.WaitPidMigration(getpid()) && argv)
                {
                    LOG(csmapi, trace) << "Pid #" << getpid() << " migrated allocation: " << allocation_id;
                    LOG(csmapi, trace) << "Executing \"" << *argv << "\"";
                   _Exit(execv(*argv, argv));
                    
                }
            }
            catch ( const std::exception& e )
            {
                _Exit(-1);
            }
        } 

        _Exit(0);
    }

    waitpid(execPid, &status, 0);

    return WEXITSTATUS(status);
        
}

int ForkAndExec( char * const argv[] )
{
    int status = 0;
    //signal(SIGCHLD, ProcessExit);        
    pid_t execPid = fork();

    if ( execPid > 0 )
    {
        //LOG( csmapi, warning ) << "Child PID: " << execPid ;
        if (waitpid(execPid, &status, 0) == -1) {
            ; // TODO additional error handle.
        }
    }
    else if ( execPid == 0  )
    {
        // Get this pid to be sure the correct fd is closed.
        pid_t forkPid = getpid();
        ClearFileDescriptors(forkPid);
        _Exit(execv(*argv, argv));
    }

    return WEXITSTATUS(status);
}


} // End namespace helpers
} // End namespace daemon
} // End namespace csm

