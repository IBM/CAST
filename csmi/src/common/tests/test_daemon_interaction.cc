/*================================================================================

    csmi/src/common/tests/test_daemon_interaction.cc

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <iostream>

#include "logging.h"
#include "csmi/include/csm_api_common.h"

enum test_mode_t
{
  TEST_MODE_NOTHING,
  TEST_MODE_CONCURRENCY,
  TEST_MODE_CONNECT_SLEEP,
};

typedef struct
{
  test_mode_t _mode;
  int _concurrency;
  int _sleep_time;
  int _iterations;
}  test_config_t;

typedef struct
{
  int _rc;
  int _thread_id;
  int _sleep_us;
  int _iterations;
} init_term_config_t;

void usage( const char* text, const int rc )
{
  std::cerr << text << std::endl << " Usage:  [-h]   print help" << std::endl
      << "   [-m <c|s>]           test mode" << std::endl
      << "   [-P <parallelism>]   number of concurrent actions" << std::endl
      << "   [-d <delay>]         random delays in microsecs" << std::endl
      << "   [-n <iterations>]    number of iterations per concurrent task" << std::endl
      << std::endl;
  exit( rc );
}


test_config_t* parse_options( int argc, char **argv)
{
  test_config_t *conf = new test_config_t;
  int opt;

  conf->_mode = TEST_MODE_CONCURRENCY;
  conf->_concurrency = 1;
  conf->_sleep_time = 0;
  conf->_iterations = 1;

  while ((opt = getopt(argc, argv, "hd:m:n:P:")) != -1) {
    switch( opt ) {
      case 'h':
        usage("Usage: ", 0);
        break;
      case 'd':
        conf->_sleep_time = atol( optarg );
        break;
      case 'm':
      {
        switch( optarg[ 0 ] )
        {
          case 'C':
          case 'c':
            conf->_mode = TEST_MODE_CONCURRENCY;
            break;
          default:
            usage( "Unknown test mode.", 1 );
            break;
        }
        break;
      }
      case 'n':
        conf->_iterations = atol( optarg );
        break;
      case 'P':
        conf->_concurrency = atol( optarg );
        break;
    }
  }
  return conf;
}

int init_term( init_term_config_t *i_Config )
{
  int thrID = i_Config->_thread_id;
  int sleeptime = i_Config->_sleep_us >> 1;
  int rc = 0;

  int loop = i_Config->_iterations;

  while(( rc == 0 )&&(--loop >= 0))
  {
    rc = csm_init_lib();
    if( rc == 0 )
    {
      if( sleeptime )
        usleep( random() % sleeptime + sleeptime );
      rc = csm_term_lib();
    }
    if( sleeptime )
      usleep( random() % sleeptime + sleeptime );
  }
  i_Config->_rc = rc;
  std::cout << " Thread " << thrID << " exit with rc=" << rc << std::endl;
  return rc;
}

int ConcurrentConnectTest( test_config_t *i_Config )
{
  int rc = 0;

  pid_t thread[ i_Config->_concurrency ];
  init_term_config_t thr_in_out[ i_Config->_concurrency ];
  for( int t=0; t<i_Config->_concurrency; ++t )
  {
    thr_in_out[t]._thread_id = t;
    thr_in_out[t]._sleep_us = i_Config->_sleep_time;
    thr_in_out[t]._iterations = i_Config->_iterations;
    thr_in_out[t]._rc = 0;

    pid_t pid = fork();
    if(pid > 0)
      thread[ t ] = pid;
    else
      exit( init_term( &thr_in_out[t] ) );
  }

  for( int t=0; t<i_Config->_concurrency; ++t )
  {
    waitpid( thread[t], &thr_in_out[t]._rc, 0 );
    rc += WEXITSTATUS( thr_in_out[t]._rc );
  }

  std::cout << " ConcurrentConnectTest exits with rc=" << rc << std::endl;
  return rc;
}

int main(int argc, char *argv[])
{
  int rc = 0;

  test_config_t *conf = parse_options( argc, argv );

  switch( conf->_mode )
  {
    case TEST_MODE_CONCURRENCY:
      rc += ConcurrentConnectTest( conf );
      break;
    default:
      std::cout << "Unrecognized test mode." << std::endl;
      break;
  }

  std::cout << " test_daemon_interaction exits with rc=" << rc << std::endl;
  return rc;
}

