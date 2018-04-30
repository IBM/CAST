/*================================================================================

    csmd/src/ras/src/RasEventThreshold.cc

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#include "../include/RasEventThreshold.h"
#include "csmi/include/csm_api_ras_keys.h"
#include <boost/regex.hpp>
#include <string_tokenizer.h>
#include <logging.h>

using namespace std;

/**
 * query the internal structs for the current threshold count 
 * for a message id and location... 
 * 
 * @param msg_id 
 * @param location 
 * 
 * @return int 
 */
int RasEventThreshold::GetThresholdCount(const std::string &msg_id, const std::string &location)
{
    string msgIdLocation = MsgIdLocation(msg_id,location);
    LocationMapIter liter = _locationMap.find(location);
    if (liter == _locationMap.end()) 
        return(0);
    return(liter->second);
}

/**
 * New thresholded ras event to the event with a 
 * threshold count and threshold period.... 
 *     if a egvent has a threshold count of 0 or 1 or a
 *     threshold period of blank or 0, then it is returned in
 *     postEvents immediatly.
 * 
 * @param rasEvent 
 * @param thresholdExceed -- [out] threshold was exceeded... 
 *                  threshold was exceeded..., or threshold = 0
 *                  or 1, or no location name...
 *                  all will cause the event to be returned
 *                  immediatly i trasEvent->..
 * @return -- true if threshold exceeded, false if not...
 */
bool RasEventThreshold::NewRasEvent(std::shared_ptr<RasEvent> rasEvent)
{
    boost::unique_lock<boost::mutex>  guard(_thresholdMutex);
    string msg_id;
    string location;
    int threshold_count;
    string threshold_period;
    int periodSecs;

    TimerExpired(); // check to see if any timers expried...

    msg_id = rasEvent->getValue(CSM_RAS_FKEY_MSG_ID);
    location = rasEvent->getValue(CSM_RAS_FKEY_LOCATION_NAME);
    threshold_count = atoi(rasEvent->getValue(CSM_RAS_FKEY_THRESHOLD_COUNT).c_str());
    threshold_period = rasEvent->getValue(CSM_RAS_FKEY_THRESHOLD_PERIOD);

   if ((msg_id == "") || (location == "")) {
        return(true);
    }
    if (threshold_count <= 1) {
        return(true);
    }
    periodSecs = GetThresholdPeriodSecs(threshold_period);
    if (periodSecs <= 0) {
        return(true);
    }

    // now we have something to consider... put it in the list of stuff in the pool...
    // keep a count of the ids...
    string msgIdLocation = MsgIdLocation(msg_id,location);
    LocationMapIter liter = _locationMap.find(msgIdLocation);
    int currCount;
    if (liter == _locationMap.end()) {
        _locationMap.insert(pair<string, unsigned long >(msgIdLocation, 1) );
        currCount = 1;
    }
    else {
        currCount = ++(liter->second);
    }

    time_t now;                             // pickup the epoc time...
    time(&now);
    time_t timeout = now + periodSecs;
    _timeToEventMap.insert(pair<time_t, std::shared_ptr<RasEvent> >(timeout, rasEvent) );

    if (currCount >= threshold_count) 
        return(true);
    else
        return(false);
}

/**
 * notify the ras event pool that it has an expried timer...
 * 
 * @param thresholdEvents -- [out] vector of expired ras events. 
 *                               which may or lmay not have been
 *                               suppressed
 * 
 * @return -- none.
 */
void RasEventThreshold::TimerExpired()
{
    time_t now;                             // pickup the epoc time...
    time(&now);


    std::vector<std::shared_ptr<RasEvent> > expiredEvents;

    TimeToEventMapIter iter;
    for (iter = _timeToEventMap.begin(); iter != _timeToEventMap.end(); iter++) {
        shared_ptr<RasEvent> re(iter->second);
        if (now < iter->first)
            break;
        expiredEvents.push_back(re);
    }
    _timeToEventMap.erase(_timeToEventMap.begin(), iter);        

    std::vector<std::shared_ptr<RasEvent> >::iterator eiter;
    for (eiter = expiredEvents.begin(); eiter != expiredEvents.end(); eiter++) {
        string msg_id;
        string location;
        shared_ptr<RasEvent> rasEvent(*eiter);


        msg_id = rasEvent->getValue(CSM_RAS_FKEY_MSG_ID);
        location = rasEvent->getValue(CSM_RAS_FKEY_LOCATION_NAME);
        string msgIdLocation = MsgIdLocation(msg_id,location);
        LocationMapIter liter = _locationMap.find(msgIdLocation);

        if (liter == _locationMap.end())    // nothign here, probably something wrong, but bug out...
            continue;
        if (liter->second > 0)
            liter->second--;

        if (liter->second <= 0)
            _locationMap.erase(liter);      // if our counter reached zero, ditch the value...
    }

    

    return;
}

/** 
 * GetThresholdPeriod 
 * parse and validate the threshold period field 
 * valid form is 
 *     nnn[HMS]
 *     where H == hours, M=minutes, S = secons.
 *     i.e. 100M is 100 minutes.
 * @param periodStr [in] -- string containing the period value. 
 * @param secs [out] -- number of seconds in string..
 * returns -- threshold period in seconds 
 *            == -1 if threshold is invalid 
 *         
 *  */ 
int RasEventThreshold::GetThresholdPeriodSecs(const std::string &periodStr)
{
    boost::regex ex("^([0-9]*)([HhMmSs]) *");
    boost::match_results<std::string::const_iterator> what;
    string interval;
    int secs;

    if(boost::regex_search(periodStr, what, ex)) {
        string s(what[1]);
        secs = atoi(s.c_str());
        interval = what[2];
        switch (tolower(interval[0])) {
            case 'h':
                secs = secs * 60 * 60;
                break;
            case 'm':
                secs = secs * 60;;
                break;
            case 's':
            default: 
                break;
        }
        return(secs);
    } else {
        return(-1);
    }

    return(-1);
}


