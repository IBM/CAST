/*================================================================================

    csmd/src/ras/include/RasEvent.h

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#ifndef __RASEVENT_H__
#define __RASEVENT_H__

#include <stdio.h>
#include <errno.h>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/foreach.hpp>
#include <string>
#include <stdint.h>

class RasEvent
{
public:
   RasEvent();

   RasEvent(const std::string &jsondata);

   /**
    * Set ras event from json data. 
    * 
    * @param jsondata 
    *         throws exception boost::property_tree::ptree_error
    *         if there is a json format problem.
    */
   void setFromJsonData(const std::string &jsondata);

   std::string msg_id();

   // List of event severities, severity is added from the message template...
   enum Severity {
       INFO,   // designates informational messages that highlight the progress of system software.
       WARN,   // designates potentially harmful situations like an error threshold being exceeded.or a redundant component failed
       FATAL,  // designates severe error events.
       UNKNOWN // the severity is not known
   };

   /**
    * Take the event and and put it in json data format. 
    * @param none. 
    * @returns string of json data. 
    */
   std::string getJsonData();

   /**
    * set a key value pair in the ras data.
    * 
    * @param key 
    * @param value 
    */
   void setValue(const std::string &key, const std::string &value);

   std::string getValue(const std::string &key) const;

   std::string getLogString() const;

   /**
    * Check if the RasEvent has a value for the specified key.
    * 
    * @param key 
    * 
    * @return bool 
    */
   bool hasValue(const std::string &key) const; 

   bool handled();
   
   void setHandled(bool v);
   
   void setThresholdCount(int32_t threshold_count);

   int32_t getThresholdCount();
 
   void setThresholdPeriod(int32_t threshold_period);

   int32_t getThresholdPeriod();

   void setCount(int32_t count);

   int32_t getCount();

protected:
   boost::property_tree::ptree _event;
   bool _handled;

private:
   int32_t _threshold_count;
   int32_t _threshold_period;
   int32_t _count;
};

#endif
