/*================================================================================

    csmd/src/ras/src/RasEventPool.cc

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/




#include "../include/RasEventPool.h"
#include "csmi/include/csm_api_ras_keys.h"
#include <string_tokenizer.h>
#include <logging.h>



using namespace std;

bool RasEventPool::FatalErrorInPool(std::string locationName)
{
    LocationFatalIter fiter = _locationFatalMap.find(locationName);
    if (fiter == _locationFatalMap.end()) 
        return(false);
    return(fiter->second > 0);

}

void RasEventPool::AddExpiredEvent(std::vector<std::shared_ptr<RasEvent> > &expiredEvents, 
                                   std::shared_ptr<RasEvent> rasEvent)
{
    string locationName = rasEvent->getValue(CSM_RAS_FKEY_LOCATION_NAME);
    string severity = rasEvent->getValue(CSM_RAS_FKEY_SEVERITY);

    expiredEvents.push_back(rasEvent);      // no expire time, so return to act immediatly, after suppression...

    if (severity == CSM_RAS_SEV_FATAL_S) {
        unsigned long c;
        c = _locationFatalMap[locationName];
        if (c > 0)
            _locationFatalMap[locationName] = c-1;
    }

}


time_t RasEventPool::AddRasEvent(std::shared_ptr<RasEvent> rasEvent,
                                 std::vector<std::shared_ptr<RasEvent> > &expiredEvents)
{
    boost::unique_lock<boost::mutex>  guard(_poolMutex);
    bool newTimer = false;
#if 0
    typedef std::multimap<time_t, std::shared_ptr<RasEvent>>::iterator TimePoolIter;
    std::multimap<time_t, std::shared_ptr<RasEvent>> _timePool;                    // pool index by time.
    typedef std::multimap<std::string, std::shared_ptr<RasEvent> >::iterator LocationMapIter;
    std::multimap<std::string, std::shared_ptr<RasEvent> > _msgIdLocation;       // event indexed by msgid/location
#endif
    unsigned min_time_in_pool = atoi(rasEvent->getValue(CSM_RAS_FKEY_MIN_TIME_IN_POOL).c_str());
    string suppress_ids = rasEvent->getValue(CSM_RAS_FKEY_SUPPRESS_IDS);
    string locationName = rasEvent->getValue(CSM_RAS_FKEY_LOCATION_NAME);
    string msgId = rasEvent->getValue(CSM_RAS_FKEY_MSG_ID);
    string severity = rasEvent->getValue(CSM_RAS_FKEY_SEVERITY);

    // keep track of fatal errors....
    if (severity == CSM_RAS_SEV_FATAL_S) {
        LocationFatalIter fiter = _locationFatalMap.find(locationName);
        if (fiter == _locationFatalMap.end()) 
            _locationFatalMap.insert(pair<string, unsigned long >(locationName, 1) );
        else 
            fiter->second++;
    }



    StringTokenizer kvtokens;
    kvtokens.tokenize(suppress_ids);
    for (unsigned n = 0; n < kvtokens.size(); n++) {
        // first check to see if there are any events this should suppress...
        LocationMapIter miter = _locationMap.find(locationName);
        while (miter != _locationMap.end()) {
            shared_ptr<RasEvent> re(miter->second);
            if (locationName != re->getValue(CSM_RAS_FKEY_LOCATION_NAME))
                break;
            if (kvtokens[n] == re->getValue(CSM_RAS_FKEY_MSG_ID)) {
                re->setValue(CSM_RAS_FKEY_SUPPRESSED, "1");     // suppress this event...
                //LOG(csmd, info) << __FILE__ << "suppressing " << re->getValue(CSM_RAS_FKEY_MSG_ID);
            }
            string re_suppress_ids = re->getValue(CSM_RAS_FKEY_SUPPRESS_IDS);
            // should this event suppress the new one?
            StringTokenizer re_kvtokens;
            re_kvtokens.tokenize(re_suppress_ids);
            for (unsigned k = 0; k < re_kvtokens.size(); k++) {
                if (re_kvtokens[k] == msgId) {
                    rasEvent->setValue(CSM_RAS_FKEY_SUPPRESSED, "1");     // suppress we are inserting
                    //LOG(csmd, info) << __FILE__ << "suppressing " << msgId;
                    break;
                }
            }
            miter++;        // very nice, next....
        }
    }
    if (min_time_in_pool == 0) {
        AddExpiredEvent(expiredEvents,rasEvent);
    }
    else {
        TimePoolIter currTimer = _timePool.begin();     // first element in the list is the next timer...
        newTimer = (_timePool.size() == 0);      // do we need to ask for a new timer...
        time_t now;                             // pickup the epoc time...
        time(&now);
        time_t pooltime = now + min_time_in_pool;
        _timePool.insert(pair<time_t, std::shared_ptr<RasEvent> >(pooltime, rasEvent) );
        _locationMap.insert(pair<string, std::shared_ptr<RasEvent> >(locationName, rasEvent) );

        // don't be redundant, only return a new timer if we have added it to the front, 
        // if this is not new, then a timer call back is already on its way...
        if ((newTimer) && (currTimer != _timePool.begin())) {
            TimePoolIter miter = _timePool.begin();        // first element in the list is the next timer...
            return(miter->first);
        }
    }

    return(0);
}


/**
 * notify the ras event pool that it has an expried timer...
 * 
 * @param expiredEvents -- [out] vector of expired ras events. 
 *                               which may or lmay not have been
 *                               suppressed
 * 
 * @return -- timer event to set if not zero.  Absolute time 
 *         since eopc
 */
time_t RasEventPool::TimerExpired(std::vector<std::shared_ptr<RasEvent> > &expiredEvents)
{
    boost::unique_lock<boost::mutex>  guard(_poolMutex);
    //LOG(csmd, info) << __FILE__ << "TimerExpired + ";
    time_t now;                             // pickup the epoc time...
    time(&now);

    //LOG(csmd, info) << __FILE__ << "TimerExpired _timePool.size() = " << _timePool.size();

    TimePoolIter iter;
    for (iter = _timePool.begin(); iter != _timePool.end(); iter++) {
        shared_ptr<RasEvent> re(iter->second);
        if (now < iter->first)
            break;
        AddExpiredEvent(expiredEvents, re);
    }
    _timePool.erase(_timePool.begin(), iter);        
    //LOG(csmd, info) << __FILE__ << "TimerExpired _timePool.size() = " << _timePool.size();

    // these delete in order... remove these from the location map...
    for (std::vector<std::shared_ptr<RasEvent> >::iterator it = expiredEvents.begin();
         it != expiredEvents.end(); it++) {
        shared_ptr<RasEvent> re(*it);
        string locationName = re->getValue(CSM_RAS_FKEY_LOCATION_NAME);
        LocationMapIter liter = _locationMap.find(locationName);
        if (liter != _locationMap.end() || (liter->second->getValue(CSM_RAS_FKEY_LOCATION_NAME) != locationName)) {
            while (liter != _locationMap.end()) {
                if (liter->second == re) {
                    LocationMapIter t = liter++;
                    _locationMap.erase(t);      // erase behind us...
                    //LOG(csmd, info) << __FILE__ << ": erase " << locationName << ": " << re->getValue(CSM_RAS_FKEY_MSG_ID);
                } 
                else {
                    liter++;
                }
            }
        }
        else {
            LOG(csmd, error) << __FILE__ << ":" << __LINE__ << " " << "cannot find event in _locationMap: " << locationName;
        }
    }
    // do we need to return a new timer event.
    if ((expiredEvents.size()) && (_timePool.size())) {
        TimePoolIter miter = _timePool.begin();        // first element in the list is the next timer...
        return(miter->first);
    }
    return(0);
}



