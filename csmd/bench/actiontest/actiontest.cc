/*================================================================================

    csmd/bench/actiontest/actiontest.cc

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/


/**
*    prototype test program to develop the code to run an external script
*    capture its stdout and stderror, route its output to a file
*    or stream and timeout if the script takes too long...
* 
*/


#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h> 
#include <string.h>
#include <fcntl.h>

#include <ctime>
#include <iostream>
#include <string>


class RasRunActionScript 
{
public:
    RasRunActionScript() :
        _dirname(""),
        _timeout(30) {};        // thirty second default timeout.


    int Run(const std::string &script, const std::string &arg);
    
    void setTimeout(unsigned secs) {
        _timeout = secs;
    };

    bool fileExists(const std::string &file);
    bool isFileExe(const std::string &file);

    void setLogFile(const std::string &logfile) {
        _logfile = logfile; 
    };
    std::string getErrStr() {
        return(_errstr);
    };


protected:

    void writeScrLogfile(int fd, const char *data, size_t len);
    void flushScrLogfile(int fd);

    std::string getTimeStamp();
    std::string _dirname;       // directory name to find the script in...
    unsigned _timeout;          // timeout in seconds, 0 is no timeout.
    std::string _errstr;        // last error string...
    std::string _logfile;       // file to log output to...

    std::string _currline;      // current line under construction
private:

};

using namespace std;


std::string RasRunActionScript::getTimeStamp() {
    std::time_t t = std::time(NULL);
    char mbstr[100];
    std::strftime(mbstr, sizeof(mbstr), "%F %T", std::localtime(&t));

    return(mbstr);
}

void RasRunActionScript::writeScrLogfile(int fd, const char *data, size_t len)
{
    if (fd < 0)
        return;

    const char *p = data;
    const char *ep = data + len;      // end of the line...

    while (p < ep ) {
        const char *np = (const char *)memchr(const_cast<char*>(p), '\n', ep-p);
        if (np == NULL) {
            if (p < ep)                     // last received without a cr...
                _currline.assign(p, ep-p);
            break;
        }
        // np points to the \n" character...
        if (np-p) {
            if (_currline.size()) {
                write(fd, _currline.c_str(), _currline.size());
                _currline.erase();
            }
            string ts = string("[") + getTimeStamp() + string("] ");
            write(fd, ts.c_str(), ts.size());
            write(fd, p, np-p+1);
        }
        p = np+1;
    }
    // turn these into buffered writes with timestamps at the beginning of each line...
    
}
void RasRunActionScript::flushScrLogfile(int fd) 
{
    if (_currline.size()) {
        string ts = string("[") + getTimeStamp() + string("] ");
        write(fd, ts.c_str(), ts.size());
        write(fd, _currline.c_str(), _currline.size());
    }
}


/**
 * Check if a file is executable by this user..
 * 
 * @param file 
 * 
 * @return int 
 */
bool RasRunActionScript::isFileExe(const std::string &file)
{
    return !access(file.c_str(), X_OK);
}
bool RasRunActionScript::fileExists(const std::string &file)
{
    struct stat  st;
    if (stat(file.c_str(), &st) < 0)
         return false;
    return(true);

}

int RasRunActionScript::Run(const std::string &script, const std::string &arg)
{
    string fname;
    int scrlogFd = -1;
    int retvalue = 0;       // csmi success??


    _errstr.erase();        // clear the error string...
    if (_dirname.size()) 
        fname = _dirname + "/";
    fname += script;

    if (!fileExists(fname)) {
        _errstr = fname + " not found";
        return(-1);
    }

    if (!isFileExe(fname)) {
        _errstr = fname + "not executable";
        return(-1);
    }


    if (_logfile.size()) {
        scrlogFd = open(_logfile.c_str(), O_WRONLY | O_APPEND | O_CREAT, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP);
        if (scrlogFd < 0) {
            _errstr = string("open ") + _logfile + " failed: " + strerror(errno);
            return(-1);
        }
    }

    int pipefd[2];
    pipe(pipefd);

    pid_t fpid;
    fpid =  fork();
    if (fpid == 0)
    {
        close(pipefd[0]);    // close reading end in the child
    
        // do both stdout and stderr...    
        dup2(pipefd[1], 1);  // send stdout to the pipe
        dup2(pipefd[1], 2);  // send stderr to the pipe
    
        close(pipefd[1]);    // this descriptor is no longer needed
        char *argvNew[5] = {NULL};
        //vector<char *> vectEnv;

        argvNew[0] = const_cast<char *>(fname.c_str());
        argvNew[1] = const_cast<char *>(arg.c_str());
        argvNew[2] = NULL;
        //
        // need exec with args and environment variables...
        //    todo, figure out how to get the take the current environment and add to it...
        execv(fname.c_str(), argvNew);
    }
    else
    {
        // parent
        fd_set pset;
        struct timeval timeout;
        close(pipefd[1]);  // close the write end of the pipe in the parent
               
        // set for non blocking io..
        int flags = fcntl(pipefd[0], F_GETFL, 0);
        fcntl(pipefd[0], F_SETFL, flags | O_NONBLOCK);               

        // Initialize the file descriptor set. 
        FD_ZERO(&pset);
        FD_SET(pipefd[0], &pset);
        
        // Initialize the timeout data structure. 
        timeout.tv_sec = _timeout;     
        timeout.tv_usec = 0;
        // this needs to have more logic, needs to not the time in the select, but the time in 
        //    the timeout value down.. each time the select returns...

        for (;;) {
            // select in this version of linux will adjust the timeout value down
            int ret = select(pipefd[0]+1, &pset, NULL, NULL, (_timeout > 0) ? &timeout : NULL);
            if (ret == -1 && errno == EINTR) // some signal
            { 
                int st;
                pid_t wpid = waitpid(fpid, &st, WNOHANG);
                if (wpid != 0) // some child signaled
                    break;
            } 
            else if (ret == -1) {
                _errstr = strerror(errno);
                break;
            }
            else if (ret > 0) {
                char buffer[1024];
                // something to read...
                for (;;) {
                    int readlen;
                    readlen = read(pipefd[0], buffer, sizeof(buffer)-1);
                    if (readlen <= 0)
                        break;
                    buffer[readlen] = 0;
                    writeScrLogfile(scrlogFd, buffer, readlen);
                }
                int st;
                pid_t wpid = waitpid(fpid, &st, WNOHANG);
                if (wpid != 0) // some child signaled
                    break;
            }
            else {
                kill(fpid, SIGABRT);
                _errstr = script + " timed out";
                retvalue = -1;
                break;
            }
        }
        flushScrLogfile(fpid);
        if (fpid > 0)
            close(fpid);
        // we need to better than this, with an asynchrnus select and potentially a timeout...
        //     
        close(pipefd[0]);  // close our pipe reader...
    }

    // check if we can find and execute this script first.  
    // we want our ras system to log the error we could not find the script..
    return(retvalue);
}

int main (int argc, char *argv[])
{
    RasRunActionScript scr;
    int rc;

    try {
        scr.setLogFile("action.log");
        scr.setTimeout(10);

        string rasEvent = "{\"msg_id\": \"test.testcat01.linkdown\" }";
        rc = scr.Run("boogers.sh", rasEvent);
        if (rc == 0) {
            cout << "expected failure of scr.Run(\"boogers.sh\") got success" << endl;
            return(1);
        }
    
        rc = scr.Run("stdout.sh", rasEvent);
        if (rc != 0) {
            cout << "scr.Run(\"stdout.sh\") failed: " << rc << " " << scr.getErrStr() << endl;
            return(1);
        }
    }
    catch (std::exception & e)
    {
        std::cerr << "Error running test " << e.what() << "\n";
        return -1;
    }

    cout << "TEST PASSED\n";
    return(0);

}





