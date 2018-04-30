/*================================================================================

    csmd/src/daemon/include/csm_event_source_set.h

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
/* csm_event_sources.h
 *
 ******************************************/

#ifndef CSM_DAEMON_SRC_CSM_EVENT_SOURCE_SET_H_
#define CSM_DAEMON_SRC_CSM_EVENT_SOURCE_SET_H_

// implements a set of event sources with priorities
// uses polymorphic eventsources to enable fetching from various types of sources
// provides a fetch routine to return a CSMCoreEvent (or a limited list)

#include <set>
#include <deque>
#include <string>
#include <inttypes.h>

#include "include/csm_core_event.h"
#include "include/csm_event_source.h"
#include "include/item_scheduler.h"

#define DEFAULT_NETWORK_SRC_INTERVAL ( 1 )
#define DEFAULT_TIMER_SRC_INTERVAL ( 1 )
#define DEFAULT_DB_SRC_INTERVAL ( 1 )


namespace csm {
namespace daemon {

typedef std::set< csm::daemon::EventSource* > SourceSetType;

typedef std::exception EventSourceException;

class EventSourceSet
{
  // \todo: one set per priority
  SourceSetType mSources;
  SourceSetType mActiveSources[2];  // double buffering of active sources
  int mActiveSetIndex;

  ItemScheduler *mBucketScheduler;
  std::vector<uint64_t> mActiveBucketList;
  SourceSetType::iterator mCurrentSource;
  int mCurrentWindow;
  unsigned mScheduledSources;
  int mOneShotSources;

public:
  EventSourceSet();
  virtual ~EventSourceSet();
  csm::daemon::CoreEvent* Fetch( const int i_JitterWindow );

  // adds a new event source with priority
  int Add( const csm::daemon::EventSource *aSource,
           const uint64_t aIdentifier,
           const uint64_t aBucketID = 0,
           const uint64_t aInterval = 1);

  // removes an event processor from a given priority list
  // priority currently ignored until priorities implemented
  int Remove( const csm::daemon::EventSource *aSource,
              const uint8_t aPriority );

  // removes an event processor by name from a given priority list
  // priority currently ignored until priorities implemented
  int Remove( const std::string aName,
              const uint8_t aPriority );

private:
  csm::daemon::EventSource* GetNextSource();
};

}  // namespace daemon
} // namespace csm

#endif /* CSM_DAEMON_SRC_CSM_EVENT_SOURCE_SET_H_ */
