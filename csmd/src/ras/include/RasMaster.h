/*================================================================================

    csmd/src/ras/include/RasMaster.h

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#ifndef __RASMASTER_H__
#define __RASMASTER_H__

#include "RasEvent.h"

#include <string>
#include <map>
#include <chrono>

#include <boost/thread.hpp>

using std::string;
using std::map;

typedef std::chrono::time_point<std::chrono::steady_clock> time_point_t;

class ThresholdData
{
public:
   ThresholdData();

   int32_t count;                     // Current count of events for the active period
   time_point_t expiration_time;      // Expiration time of the active threshold period
};

class RasMaster
{
public:
   RasMaster();

   /**
    * Check to see if the event has hit treshold and increment the threshold count in the event. 
    * 
    * @param io_event The ras event to process.
    * @return true if event has hit threshold, otherwise false.
    * @post The threshold_count of io_event will be set. 
    */
   bool handleRasEventThreshold(RasEvent &io_event);

private:

   /**
    * Generate a string containing the msg_id and location
    */
   string msgIdLocation(const string &msg_id, const string &location);

   boost::mutex m_threshold_mutex;
   std::map<std::string, ThresholdData> m_threshold_map;
   typedef std::map<std::string, ThresholdData>::iterator ThresholdMapItr_t;
};

// Single global shared instance
extern RasMaster g_ras_master;

#endif
