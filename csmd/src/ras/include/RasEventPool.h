/*================================================================================

    csmd/src/ras/include/RasEventPool.h

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#ifndef __RASEVENT_POOL_H__
#define __RASEVENT_POOL_H__

#include <boost/thread.hpp>
#include "RasRc.h"
#include "RasEvent.h"

#include "time.h"
#include <string>
#include <vector>
#include <map>
#include <set>


class RasEventPool
{
public:
    /**
     * Add a Ras Event to the event pool.... 
     * 
     * @param rasEvent 
     * @param expiredEvents -- [out] vector of expired ras events. 
     *                               which may or lmay not have been
     *                               suppressed
     * @return -- timer event to set if not zero.  Absolute time 
     *         since eopc
     */
     time_t AddRasEvent(std::shared_ptr<RasEvent> rasEvent,std::vector<std::shared_ptr<RasEvent> > &expiredEvents);

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
    time_t TimerExpired(std::vector<std::shared_ptr<RasEvent> > &expiredEvents);

    

    /**
     * check if there is already a fatal error in the pool
     * 
     * 
     * @param locationName 
     * 
     * @return bool 
     */
    bool FatalErrorInPool(std::string locationName);
protected:
    /**
     * add a an expired to expired event queue...
     * 
     * 
     * @param expiredEvents 
     * @param rasEvent 
     */
    void AddExpiredEvent(std::vector<std::shared_ptr<RasEvent> > &expiredEvents, 
                         std::shared_ptr<RasEvent> rasEvent);
private:

    boost::mutex _poolMutex;
    typedef std::multimap<time_t, std::shared_ptr<RasEvent>>::iterator TimePoolIter;
    std::multimap<time_t, std::shared_ptr<RasEvent>> _timePool;          // pool index by time.
    typedef std::multimap<std::string, std::shared_ptr<RasEvent> >::iterator LocationMapIter;
    std::multimap<std::string, std::shared_ptr<RasEvent> > _locationMap;       // event indexed by msgid/location
    // map of fatal error count for a location, increment on insert, decrement on exit...
    typedef std::map<std::string, unsigned long >::iterator LocationFatalIter;
    std::map<std::string, unsigned long > _locationFatalMap;


};

#endif




