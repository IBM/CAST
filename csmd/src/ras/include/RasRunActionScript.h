/*================================================================================

    csmd/src/ras/include/RasRunActionScript.h

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#ifndef __RAS_RUN_ACTION_SCRIPT_H__
#define __RAS_RUN_ACTION_SCRIPT_H__
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
    
    void setScriptDir(std::string &scriptDir) {
        _dirname = scriptDir; 
    };
    void setTimeout(unsigned secs) {
        _timeout = secs;
    };


    void setLogFile(const std::string &logfile) {
        _logfile = logfile; 
    };
    std::string getErrStr() {
        return(_errstr);
    };


protected:
    bool fileExists(const std::string &file);
    bool isFileExe(const std::string &file);

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


#endif

