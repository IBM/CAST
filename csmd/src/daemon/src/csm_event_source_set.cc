/*================================================================================

    csmd/src/daemon/src/csm_event_source_set.cc

  © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#include <stdlib.h>
#include <algorithm>

#include <logging.h>
#include "csm_daemon_config.h"
#include "csm_network_config.h"
#include "csm_network_header.h"
#include "../include/csm_event_source_set.h"

namespace csm {
namespace daemon {

EventSourceSet::EventSourceSet()
{
  csm::daemon::Configuration *config = csm::daemon::Configuration::Instance();
  mActiveSetIndex = 0;

  mSources.clear();
  mCurrentSource = mSources.begin();
  mActiveSources[0].clear();
  mActiveSources[1].clear();

  std::vector<int> BucketIntervals;
  config->GetBucketIntervals( BucketIntervals );
  uint64_t MaxInterval = 128;
  for( auto it: BucketIntervals )
    MaxInterval = std::max(MaxInterval, (uint64_t)it);
  MaxInterval = std::max( MaxInterval, (uint64_t)DEFAULT_NETWORK_SRC_INTERVAL );
  MaxInterval = std::max( MaxInterval, (uint64_t)DEFAULT_TIMER_SRC_INTERVAL );
  if( config->GetRole() == CSM_DAEMON_ROLE_MASTER )
    MaxInterval = std::max( MaxInterval, (uint64_t)DEFAULT_DB_SRC_INTERVAL );

  RecurringTasks RT = config->GetRecurringTasks();
  if( RT.IsEnabled() )
    MaxInterval = std::max( MaxInterval, (uint64_t)RT.GetMinInterval() );

  mBucketScheduler = new ItemScheduler( MaxInterval );

  for( uint64_t i=0; i < BucketIntervals.size(); ++i )
    mBucketScheduler->AddItem( i, BucketIntervals[ i ], 0 );

  mBucketScheduler->AddItem( NETWORK_SRC_ID, DEFAULT_NETWORK_SRC_INTERVAL, 0 );
  mBucketScheduler->AddItem( TIMER_SRC_ID, DEFAULT_TIMER_SRC_INTERVAL, 0 );
  if( config->GetRole() == CSM_DAEMON_ROLE_MASTER )
    mBucketScheduler->AddItem( DB_SRC_ID, DEFAULT_DB_SRC_INTERVAL, 0 );

  if( RT.IsEnabled() )
    mBucketScheduler->AddItem( INTERVAL_SRC_ID, RT.GetMinInterval(), 0 );
  mCurrentWindow = -1;

  mBucketScheduler->RRForward();

  // create the initial active bucket list
  BucketEntry *bucket = nullptr;;
  mActiveBucketList.clear();
  while( (bucket = mBucketScheduler->GetNext()) != nullptr )
    mActiveBucketList.push_back( bucket->_Identifier );
  LOG(csmd, debug) << "New BucketList size=" << mActiveBucketList.size();

  mScheduledSources = 0;
  mOneShotSources = 0;
}

EventSourceSet::~EventSourceSet()
{
  // destructor of mBucketScheduler auto-removes the added items
  mSources.clear();
  delete mBucketScheduler;
}

csm::daemon::EventSource* csm::daemon::EventSourceSet::GetNextSource()
{
  // todo: observe priorities when selecting the next event source
  // got to the end of the current source set, wipe and switch the active set
  if( mCurrentSource == mActiveSources[ mActiveSetIndex ].end() )
  {
    mActiveSources[ mActiveSetIndex ].clear();
    mActiveSetIndex = ( mActiveSetIndex + 1 ) % 2;
    mCurrentSource = mActiveSources[ mActiveSetIndex ].begin();
#ifdef DETAIL_EVENT_SOURCE_DEBUG
    LOG(csmd, trace) << "Flipping ActiveSource Set to idx=" << mActiveSetIndex
        << " Scheduled=" << mScheduledSources
        << " Sources=" << mActiveSources[ mActiveSetIndex ].size();
#endif
    if( ( mScheduledSources > mActiveSources[ mActiveSetIndex ].size() ) ||
        ( mScheduledSources < mActiveSources[ mActiveSetIndex ].size() - mOneShotSources ))
      throw csm::daemon::Exception("BUG: Failed to schedule all event sources.");
    mScheduledSources = 0;
  }

  if( mCurrentSource == mActiveSources[ mActiveSetIndex ].end() )
    return nullptr;
  else
  {
    LOG( csmd, trace ) << "Scheduling Event Source: " << (*mCurrentSource)->GetIdentifier();
    csm::daemon::EventSource *nextSource = *mCurrentSource;
    ++mCurrentSource;
    if( ! nextSource->OncePerWindow() )
    {
      ++mScheduledSources;
      mActiveSources[ (mActiveSetIndex + 1) % 2 ].insert( nextSource );
    }
    else
    {
      LOG( csmd, trace ) << "Event Source " << nextSource->GetIdentifier() << " is a one-shot source.";
    }
    return nextSource;
  }
}


csm::daemon::CoreEvent* EventSourceSet::Fetch( const int i_JitterWindow )
{
  // path split for non-compute nodes? Make sure to cover env-collection...
  if( i_JitterWindow != mCurrentWindow )
  {
    LOG(csmd, trace) << "EventSourceSet::Fetch with new window";
    mCurrentWindow = i_JitterWindow;
    mBucketScheduler->RRForward();

    // recreate the active bucket list
    BucketEntry *bucket = nullptr;;
    mActiveBucketList.clear();
    mActiveSources[ mActiveSetIndex ].clear();
    while( (bucket = mBucketScheduler->GetNext()) != nullptr )
    {
      mActiveBucketList.push_back( bucket->_Identifier );
      for( auto it : mSources )
      {
        if( ( it->GetIdentifier() == bucket->_Identifier ) ||
            (( bucket->_Identifier < NETWORK_SRC_ID ) && ( it->GetIdentifier() == ENVIRONMENT_SRC_ID )) )
        {
          mActiveSources[ mActiveSetIndex ].insert( it );
          break;
        }
      }
    }
    mCurrentSource = mActiveSources[ mActiveSetIndex ].begin();
    mScheduledSources = 0;

    LOG(csmd, trace) << "SCHED: BucketListItems=" << mActiveBucketList.size()
        << " ActiveSources=" << mActiveSources[ mActiveSetIndex ].size()
        << " TotalSources=" << mSources.size();
  }

  // check all active sources and return the first available event
  csm::daemon::CoreEvent *event = nullptr;
  int oldIndex = mActiveSetIndex;
  do
  {
    csm::daemon::EventSource* EP = GetNextSource();
    if( EP )
    {
      event = EP->GetEvent( mActiveBucketList );
    }
  } while(( event == nullptr ) && ( oldIndex == mActiveSetIndex ));
  return event;
}

// adds a new event source to a bucket
int EventSourceSet::Add( const csm::daemon::EventSource *aSource,
                         const uint64_t aIdentifier,
                         const uint64_t aBucketID,
                         const uint64_t aInterval )
{
  // priority will be mostly ignored for now
  try
  {
    std::pair<csm::daemon::SourceSetType::iterator, bool> retval = mSources.insert( (csm::daemon::EventSource*)aSource );
    if( retval.second == false )
      return 1;
    else
    {
      LOG( csmd, debug ) << "Adding Event Source: " << (*retval.first)->GetIdentifier() << " oneshot=" << (*retval.first)->OncePerWindow();
      mActiveSources[ mActiveSetIndex^1 ].clear();
      mActiveSources[ mActiveSetIndex ] = mSources;
      mCurrentSource = mActiveSources[ mActiveSetIndex ].begin();
      if( aSource->OncePerWindow() )
        ++mOneShotSources;
      return 0;
    }
  }
  catch (...)
  {
    throw csm::daemon::EventSourceException();
  }
}
// removes an event source from a given priority list
int EventSourceSet::Remove( csm::daemon::EventSource * const aSource, const uint8_t aPriority )
{
  if( aPriority > CSM_NETWORK_MAX_PRIORITY )
    throw csm::daemon::EventSourceException();

  // priority will be mostly ignored for now
  try
  {
    mSources.erase( aSource );
    mActiveSources[ mActiveSetIndex^1 ].clear();
    mActiveSources[ mActiveSetIndex ] = mSources;
    mCurrentSource = mActiveSources[ mActiveSetIndex ].begin();
    if( aSource->OncePerWindow() )
      --mOneShotSources;
    if( mOneShotSources < 0 )
      throw csm::daemon::Exception("BUG: Event source counter underflow.");
  }
  catch (...)
  {
    throw csm::daemon::EventSourceException();
  }
  return 0;
}

}  // namespace daemon
} // namespace csm

