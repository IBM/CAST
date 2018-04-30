/*================================================================================

    csmd/src/daemon/tests/csm_timer_queue_test.cc

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#include <stdlib.h>
#include <string>

#include <logging.h>
#include "csm_test_utils.h"
#include "csm_daemon_config.h"
#include "include/csm_timer_event.h"
#include "src/csm_event_sinks/csm_sink_timer.h"


int main( int argc, char **argv )
{
  int rc = 0;

  csm::daemon::RetryBackOff *wakeup = new csm::daemon::RetryBackOff( );
  csm::daemon::EventSinkTimer *sink = new csm::daemon::EventSinkTimer( wakeup );

  csm::daemon::TimerContent content1( 200 );
  csm::daemon::TimerContent content2( 100 );
  csm::daemon::TimerEvent *ev1 = new csm::daemon::TimerEvent( content1, csm::daemon::EVENT_TYPE_TIMER, nullptr );
  csm::daemon::TimerEvent *ev2 = new csm::daemon::TimerEvent( content2, csm::daemon::EVENT_TYPE_TIMER, nullptr );
  csm::daemon::CoreEvent *res;

  rc += TESTFAIL( ev1, nullptr );
  rc += TESTFAIL( ev2, nullptr );

  sink->PostEvent( *ev1 );
  sink->PostEvent( *ev2 );

  res = sink->FetchEvent();

  rc += TESTFAIL( res, nullptr );

  int64_t remain1 = ev1->GetContent().RemainingMicros();
  int64_t remainR = dynamic_cast<csm::daemon::TimerEvent*>( res )->GetContent().RemainingMicros();

  rc += TEST( remainR < remain1, true );

  delete ev1;
  delete sink;
  delete wakeup;

  LOG(csmd, always) << "Test complete rc=" << rc;
  return rc;
}

