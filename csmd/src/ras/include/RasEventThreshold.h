/*================================================================================

    csmd/src/ras/include/RasEventThreshold.h

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#ifndef __RASEVENT_THRESHOLD_H__
#define __RASEVENT_THRESHOLD_H__


/**
 * Class to manage ras events with a threshold.
 */

#include <boost/thread.hpp>
#include "RasRc.h"
#include "RasEvent.h"

#include "time.h"
#include <string>
#include <vector>
#include <map>
#include <set>


class RasEventThreshold
{
public:
    /**
     * New thresholded ras event to the event with a 
     * threshold count and threshold period.... 
     *     if a egvent has a threshold count of 0 or 1 or a
     *     threshold period of blank or 0, then it is returned in
     *     postEvents immediatly.
     * 
     * @param rasEvent 
     * @return thresholdExceed -- [out] threshold was exceeded... 
     */
     bool NewRasEvent(std::shared_ptr<RasEvent>);


    /** 
     * GetThresholdPeriodSecs
     *   parse and validate the threshold period 
     *     field valid form is nnn[HMS] where H == hours, M=minutes,
     *     S = secons. i.e. 100M is 100 minutes.
     * @param periodStr [in] -- string containing the period value. 
     * @param secs [out] -- number of seconds in string..
     * returns -- threshold period in seconds 
     *            == -1 if threshold is invalid 
     *         
     */ 
    static int GetThresholdPeriodSecs(const std::string &periodStr);



    // test telemetry stuff, don't use for regular programs... TESTING ONLY...
    /**
     * query the internal structs for the current threshold count 
     * for a message id and location... 
     * 
     * @param msg_id 
     * @param location 
     * 
     * @return int 
     */
    int GetThresholdCount(const std::string &msg_id, const std::string &location);

    int GetNumTimers() { return(_timeToEventMap.size()); };
    int GetLocationMapCount() { return(_locationMap.size()); };





protected:

    /**
     * notify the ras threshold class
     * 
     * @param thresholdEvents -- [out] vector of expired ras events. 
     *                               which may or lmay not have been
     *                               suppressed
     * 
     * @return -- none
     */
    void TimerExpired();

    std::string MsgIdLocation(const std::string &msg_id, const std::string &location) {
        return(msg_id + "@" + location);
    }

    typedef std::multimap<time_t, std::shared_ptr<RasEvent>>::iterator TimeToEventMapIter;
    std::multimap<time_t, std::shared_ptr<RasEvent>> _timeToEventMap;          // time events, ordered time list.
    // map of location#msgid to current ras event count...  
    //    represents total number of events in the threshold interval..
    typedef std::map<std::string, unsigned long >::iterator LocationMapIter;
    std::map<std::string, unsigned long > _locationMap;       // event indexed by location/msgid

    boost::mutex _thresholdMutex;

private:

};


#endif


