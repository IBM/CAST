/*================================================================================

    csmd/src/daemon/tests/csm_retry_backoff_test.cc

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#include <unistd.h>
#include <iostream>
#include <thread>

// testing the wake-up and wait mechanism of retry backoff for sleeptype conditional

#define SINGLE_THREAD
#define DUAL_THREAD

#include "logging.h"
#include "csmutil/include/csm_test_utils.h"
#include "include/csm_retry_backoff.h"
#include "include/window_timer.h"

std::atomic_int counter;
std::atomic_int exitCount;

csm::daemon::RetryBackOff *gRetry;

void produce( csm::daemon::RetryBackOff *pRetry, csm::daemon::RetryBackOff *cRetry )
{
  while( counter == -1 ) {}
  LOG( csmd, always ) << "Starting producer";
  while( exitCount > 0 )
  {
    gRetry->AgainOrWait( false );
    counter++;
    std::cout << "Producer:" << counter << std::endl;
#ifdef DUAL_THREAD
    if( counter != 1 )
      throw csm::daemon::Exception("Producer");
    pRetry->WakeUp();
    cRetry->AgainOrWait( false );
#else
    sleep( 3 );
#endif
  }
  pRetry->WakeUp();
}

void consume( csm::daemon::RetryBackOff *pRetry, csm::daemon::RetryBackOff *cRetry )
{
  while( counter == -1 ) {}
  LOG( csmd, always ) << "Starting consumer";
  while( exitCount > 0 )
  {
    gRetry->AgainOrWait( false );
    pRetry->AgainOrWait( false );
    counter--;
    std::cout << "Consumer:" << counter << std::endl;
    if(( counter != 0 ) && ( exitCount > 0 ))
      throw csm::daemon::Exception("Consumer");
    usleep( random() % 10000 );
    cRetry->WakeUp();
  }
  cRetry->WakeUp();
}

/* ticks at the frequency of the window interval to advance the jitter window index */
void TimerTicker( void )
{
  std::cout << "SCHED: Entering Window Tick Handler." << std::endl;
  gRetry->WakeUp();
}


int main( int argc, char **argv )
{
  int rc = 0;

  int sleepTime = 2;
  if( argc > 1 )
  {
    char *endptr;
    sleepTime = strtol(  argv[1], &endptr, 10 );
  }
  std::cout << "sleepTime = " << sleepTime << std::endl;

  counter = -1;
  exitCount = 1000000;

  gRetry = new csm::daemon::RetryBackOff( "Global",
                                          csm::daemon::RetryBackOff::SleepType::CONDITIONAL,
                                          csm::daemon::RetryBackOff::SleepType::CONDITIONAL,
                                          1, 20000, 0 );
  {
    csm::daemon::WindowTimer wtimer ( 100000, 10000, TimerTicker );

    csm::daemon::RetryBackOff produce_retry ("Producer",
                                             csm::daemon::RetryBackOff::SleepType::CONDITIONAL,  // set up to conditional wakeup during regular operation
                                             csm::daemon::RetryBackOff::SleepType::NOOP, // noop during job run, sleep is handled by jitterwindow function
                                             1, 10000, 0 );
    csm::daemon::RetryBackOff consume_retry ("Consumer",
                                             csm::daemon::RetryBackOff::SleepType::CONDITIONAL,  // set up to conditional wakeup during regular operation
                                             csm::daemon::RetryBackOff::SleepType::NOOP, // noop during job run, sleep is handled by jitterwindow function
                                             1, 10000, 0 );

#ifdef SINGLE_THREAD
    std::thread producer ( produce, &produce_retry, &consume_retry );
#ifdef DUAL_THREAD
    std::thread consumer ( consume, &produce_retry, &consume_retry );
#endif
#endif

    counter = 0;
    sleep( sleepTime );
    exitCount = 0;

#ifdef SINGLE_THREAD
#ifdef DUAL_THREAD
    consumer.join();
#endif
    producer.join();
#endif

  }

  delete gRetry;

  return rc;
}
