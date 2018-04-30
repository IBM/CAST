/*================================================================================

    csmd/src/ras/src/RasEvent.cc

  © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/


#include "../include/RasEvent.h"
#include "csm_api_ras_keys.h"

using namespace std;

RasEvent::RasEvent() : 
   _handled(false),
   _threshold_count(0),
   _threshold_period(0),
   _count(0)
{
}

// RAS event initializer from json data..
RasEvent::RasEvent(const std::string &jsondata) : 
   _handled(false),
   _threshold_count(0),
   _threshold_period(0),
   _count(0)
{
    setFromJsonData(jsondata);
}

void RasEvent::setFromJsonData(const std::string &jsondata)
{
   stringstream ss;
   ss << jsondata;

   // exception handling??
   boost::property_tree::json_parser::read_json(ss, _event);

   // Set explicit integer fields from property tree
   if (_event.count(CSM_RAS_FKEY_THRESHOLD_COUNT) > 0)
   {
      _threshold_count = atoi(_event.get<string>(CSM_RAS_FKEY_THRESHOLD_COUNT).c_str());
   }
   
   if (_event.count(CSM_RAS_FKEY_THRESHOLD_PERIOD) > 0)
   {
      _threshold_period = atoi(_event.get<string>(CSM_RAS_FKEY_THRESHOLD_PERIOD).c_str());
   }
   
   if (_event.count(CSM_RAS_FKEY_COUNT) > 0)
   {
      _count = atoi(_event.get<string>(CSM_RAS_FKEY_COUNT).c_str());
   }
}

std::string RasEvent::getJsonData()
{
   // Add explicit integer fields to property_tree
   _event.put(CSM_RAS_FKEY_THRESHOLD_COUNT, to_string(_threshold_count)); 
   _event.put(CSM_RAS_FKEY_THRESHOLD_PERIOD, to_string(_threshold_period)); 
   _event.put(CSM_RAS_FKEY_COUNT, to_string(_count)); 

   ostringstream ss;

   boost::property_tree::json_parser::write_json(ss, _event, false);
   return(ss.str());
}

void RasEvent::setValue(const std::string &key, const std::string &value)
{
    _event.put(key,value);
}

std::string RasEvent::getValue(const std::string &key) const
{
    // figure out what to do with not found exception...
    if (_event.count(key) > 0)
        return(_event.get<string>(key)); 
    return(string(""));
}


std::string RasEvent::msg_id()
{
    return(getValue(CSM_RAS_FKEY_MSG_ID));
}

std::string RasEvent::getLogString() const
{
  return  "ctx:"  + getValue(CSM_RAS_FKEY_CTXID) +
          " ts:"  + getValue(CSM_RAS_FKEY_TIME_STAMP) +
          " loc:" + getValue(CSM_RAS_FKEY_LOCATION_NAME) +
          " msg:" + getValue(CSM_RAS_FKEY_MSG_ID);
}

bool RasEvent::hasValue(const std::string &key) const
{ 
   return (_event.count(key) > 0);
}

bool RasEvent::handled()
{ 
   return(_handled); 
}

void RasEvent::setHandled(bool v)
{ 
   _handled = v;
}

void RasEvent::setThresholdCount(int32_t threshold_count)
{
   _threshold_count = threshold_count;
}

int32_t RasEvent::getThresholdCount()
{
   return _threshold_count;
}

void RasEvent::setThresholdPeriod(int32_t threshold_period)
{
   _threshold_period = threshold_period;
}

int32_t RasEvent::getThresholdPeriod()
{
   return _threshold_period;
}

void RasEvent::setCount(int32_t count)
{
   _count = count;
}

int32_t RasEvent::getCount()
{
   return _count;
}

