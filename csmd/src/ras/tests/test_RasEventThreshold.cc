/*================================================================================

    csmd/src/ras/tests/test_RasEventThreshold.cc

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#include <iostream>
#include <boost/lexical_cast.hpp>
#include <csmd/src/ras/include/RasEventThreshold.h>
#include <csm_api_ras.h>
#include <csmd/src/ras/include/RasEvent.h>



using namespace std;

class TestRasEventThreshold {
public:
    int main(int argc, char **argv);

protected:
    std::shared_ptr<RasEvent> createRasEvent(const std::string &msg_id, 
                                          const std::string &location,
                                          int threshold_count,
                                          const std::string &threshold_period);

private:
    RasEventThreshold _rasEventThreshold;

};

int TestRasEventThreshold::main(int argc, char **argv)
{
    int tc = _rasEventThreshold.GetThresholdCount("a","b");      // place holder
    cout << "tc = " << tc << endl;

    // check threshold parsing
    bool threshEx;
    string test01 = "test01.cat01.test01";
    string loc001 = "loc001";
    string loc002 = "loc002";
    std::shared_ptr<RasEvent> rasEvent;

#if 0

    // we expect 0 for this...
    threshEx = _rasEventThreshold.NewRasEvent(createRasEvent(test01, loc001, 1, ""));
    assert(threshEx);
    assert(_rasEventThreshold.GetNumTimers() == 0);
    assert(_rasEventThreshold.GetLocationMapCount() == 0);
    
    // no period, but positive threshold, also immediately triggers, as we can never have two or more in zero time.
    threshEx = _rasEventThreshold.NewRasEvent(createRasEvent(test01, loc001, 2, ""));
    assert(threshEx);
    assert(_rasEventThreshold.GetNumTimers() == 0);
    assert(_rasEventThreshold.GetLocationMapCount() == 0);

    threshEx = _rasEventThreshold.NewRasEvent(createRasEvent(test01, loc001, 2, "2s"));
    assert(!threshEx);
    assert(_rasEventThreshold.GetNumTimers() == 1);
    assert(_rasEventThreshold.GetLocationMapCount() == 1);

    threshEx = _rasEventThreshold.NewRasEvent(createRasEvent(test01, loc001, 2, "2s"));
    assert(threshEx);
    assert(_rasEventThreshold.GetNumTimers() == 2);
    assert(_rasEventThreshold.GetLocationMapCount() == 1);

    threshEx = _rasEventThreshold.NewRasEvent(createRasEvent(test01, loc002, 2, "2s"));
    assert(!threshEx);
    assert(_rasEventThreshold.GetNumTimers() == 3);
    assert(_rasEventThreshold.GetLocationMapCount() == 2);

    sleep(1);
    threshEx = _rasEventThreshold.NewRasEvent(createRasEvent(test01, loc002, 2, "2s"));
    assert(threshEx);
    assert(_rasEventThreshold.GetNumTimers() == 4);
    assert(_rasEventThreshold.GetLocationMapCount() == 2);


    cout << "sleeping 3 seconds" << endl;
    sleep(3);

    // all timers should have expired, and the events should be cleared out...
    _rasEventThreshold.TimerExpired();
    assert(_rasEventThreshold.GetNumTimers() == 0);
    assert(_rasEventThreshold.GetLocationMapCount() == 0);
#endif

    // now test rolling it..
    threshEx = _rasEventThreshold.NewRasEvent(createRasEvent(test01, loc001, 3, "2s"));
    assert(!threshEx);
    assert(_rasEventThreshold.GetNumTimers() == 1);
    assert(_rasEventThreshold.GetLocationMapCount() == 1);
    sleep(1);

    assert(_rasEventThreshold.GetNumTimers() == 1);
    assert(_rasEventThreshold.GetLocationMapCount() == 1);

    threshEx = _rasEventThreshold.NewRasEvent(createRasEvent(test01, loc001, 3, "2s"));
    assert(!threshEx);
    assert(_rasEventThreshold.GetNumTimers() == 2);
    assert(_rasEventThreshold.GetLocationMapCount() == 1);
    sleep(1);
    usleep(10000);

    threshEx = _rasEventThreshold.NewRasEvent(createRasEvent(test01, loc001, 3, "2s"));
    assert(!threshEx);
    assert(_rasEventThreshold.GetNumTimers() == 2);
    assert(_rasEventThreshold.GetLocationMapCount() == 1);
    sleep(1);
    usleep(10000);

    sleep(1);
    usleep(10000);
    threshEx = _rasEventThreshold.NewRasEvent(createRasEvent(test01, loc001, 0, "0s"));
    assert(threshEx);
    assert(_rasEventThreshold.GetNumTimers() == 0);
    assert(_rasEventThreshold.GetLocationMapCount() == 0);


    cout << "TEST PASSED" << endl;
    return(0);
}

std::shared_ptr<RasEvent> TestRasEventThreshold::createRasEvent(const std::string &msg_id, 
                                      const std::string &location,
                                      int threshold_count,
                                      const std::string &threshold_period)
{
    std::shared_ptr<RasEvent> rasEvent(new RasEvent);
    rasEvent->setValue(CSM_RAS_FKEY_MSG_ID, msg_id);
    rasEvent->setValue(CSM_RAS_FKEY_LOCATION_NAME, location);
    rasEvent->setValue(CSM_RAS_FKEY_THRESHOLD_COUNT, boost::lexical_cast<std::string>(threshold_count));
    rasEvent->setValue(CSM_RAS_FKEY_THRESHOLD_PERIOD, threshold_period);
    return(rasEvent);

}

int main(int argc, char **argv)
{
    TestRasEventThreshold rt;
    return rt.main(argc, argv);
}


