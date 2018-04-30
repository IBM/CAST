/*================================================================================

    csmd/src/ras/include/RasEventLog.h

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#ifndef RAS_EVENT_LOG_H_
#define RAS_EVENT_LOG_H_

#include <string>
#include <iomanip>
#include <fstream>

using std::endl;
using std::string;
using std::ofstream;

#define RAS_EVENT_LOG_FILE "/var/log/ibm/csm/csm_ras_events.log"
#define RAS_EVENT_LOG_FILE_PREVIOUS "/var/log/ibm/csm/csm_ras_events.log.1"
#define RAS_EVENT_LOG_MAX_LINES 50000 

class RasEventLog
{
public:

   RasEventLog();

   void log(string message);

private:
   int init();
   
   void rotateFiles();
   
   bool _initialized;
   bool _file_is_open;
   ofstream _file_out;
   uint32_t _line_count;

};

#endif /* RAS_EVENT_LOG_H_ */
