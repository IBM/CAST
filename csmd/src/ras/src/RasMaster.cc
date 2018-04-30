/*================================================================================

    csmd/src/ras/src/RasMaster.cc

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#include "../include/RasMaster.h"

#include "csmi/include/csm_api_ras_keys.h"

#include <stdint.h>

using namespace std::chrono; 

// Create the single global shared instance
RasMaster g_ras_master; 

ThresholdData::ThresholdData() :
   count(0),
   expiration_time()
{
}

RasMaster::RasMaster()
{
}

bool RasMaster::handleRasEventThreshold(RasEvent &io_event)
{
   // Handle the simple case where the threshold is disabled
   if ((io_event.getThresholdCount() < 2) || (io_event.getThresholdPeriod() < 1))
   {
      io_event.setCount(1);
      return true;
   }
   else
   {
      string msg_id = io_event.getValue(CSM_RAS_FKEY_MSG_ID);
      string location = io_event.getValue(CSM_RAS_FKEY_LOCATION_NAME);
      string msg_id_location = msgIdLocation(msg_id,location);

      boost::unique_lock<boost::mutex> guard(m_threshold_mutex);

      ThresholdMapItr_t threshold_data_itr = m_threshold_map.find(msg_id_location);      
      if (threshold_data_itr == m_threshold_map.end())
      {
         // No existing data for this msg_id and location, insert some 
         ThresholdData threshold_data;
         threshold_data.count = 1;
         io_event.setCount(1);         
         threshold_data.expiration_time = steady_clock::now() + seconds(io_event.getThresholdPeriod());

         m_threshold_map.insert(std::pair<string,ThresholdData>(msg_id_location, threshold_data));
         return false;
      }
      else
      {
         // There is existing threshold_data for the msg_id and location, figure out if the threshold has been hit
         
         // Has the current period expired?
         if (threshold_data_itr->second.expiration_time > steady_clock::now())
         {
            // Current period hasn't expired, check to see if we have hit threshold
            threshold_data_itr->second.count++;
            io_event.setCount(threshold_data_itr->second.count);         

            if (io_event.getCount() >= io_event.getThresholdCount())
            {
               // Threshold hit, start a new period and return true
               threshold_data_itr->second.count = 0;
               threshold_data_itr->second.expiration_time = steady_clock::now() - seconds(1);  // Force the period to expire
               return true; 
            }
            else
            {
               return false;
            }
         }
         else
         {
            // Previous period expired without hitting threshold, start a new period
            threshold_data_itr->second.count = 1;
            io_event.setCount(threshold_data_itr->second.count);         
            threshold_data_itr->second.expiration_time = steady_clock::now() + seconds(io_event.getThresholdPeriod());
            return false;
         }
      }
   }
}

string RasMaster::msgIdLocation(const string &msg_id, const string &location) 
{
   return(msg_id + "@" + location);
}

