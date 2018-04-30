/*================================================================================

    csmd/src/daemon/tests/csm_item_scheduler_test.cc

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#include <logging.h>
#include "include/item_scheduler.h"

#define ITEMS ( 4 )

int main( int argc, char **argv )
{
  int rc = 0;

  csm::daemon::ItemScheduler sched = csm::daemon::ItemScheduler( 100 );

  uint32_t step[ ITEMS ];
  uint32_t offset[ ITEMS ];

  step[0] = 4;
  step[1] = 10;
  step[2] = 51;
  step[3] = 106;

  for( int i=0; i < ITEMS; ++i )
  {
    offset[i] = 0;
    sched.AddItem(i, step[i], offset[i] );
  }

  uint64_t empty = 0;
  uint64_t tick = 0;
  sched.RRForward();
  while( tick < 10000 )
  {
    csm::daemon::BucketEntry *item = sched.GetNext();
    if( item == nullptr )
    {
      sched.RRForward();
      ++empty;
      ++tick;
    }
    else
    {
      LOG( csmd, always ) << "Step: " << tick % 100 << ": Scheduling item: " << item->_Identifier;
      empty = 0;
    }
  }

  for( int i=0; i < ITEMS; ++i )
  {
    sched.DeleteItem( i );
  }

  return rc;
}

