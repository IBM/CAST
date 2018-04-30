/*================================================================================

    csmd/src/daemon/src/csm_event_sources/csm_source_environmental.h

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#ifndef CSMD_SRC_DAEMON_SRC_CSM_EVENT_SOURCES_CSM_SOURCE_ENVIRONMENTAL_H_
#define CSMD_SRC_DAEMON_SRC_CSM_EVENT_SOURCES_CSM_SOURCE_ENVIRONMENTAL_H_

#include "csm_daemon_config.h"
#include "include/csm_bitmap.h"

#include "include/csm_event_source.h"

namespace csm {
namespace daemon {

class EventSourceEnvironmental: public csm::daemon::EventSource
{
private:
  std::vector< BitMap > _BucketList;
  
public:
  EventSourceEnvironmental( RetryBackOff *aRetryBackoff )
  : csm::daemon::EventSource( aRetryBackoff, ENVIRONMENT_SRC_ID, true )
  {
    csm::daemon::Configuration* config = csm::daemon::Configuration::Instance();
    _BucketList.resize( config->GetNumOfBuckets() );
    for (size_t i=0; i<_BucketList.size(); i++)
    {
      std::set< BucketItemType > list;
      config->GetBucketItems(i, list);
      _BucketList[i] = BitMap(list);
    }
    
  }
  
  virtual ~EventSourceEnvironmental() {}

  virtual csm::daemon::CoreEvent* GetEvent( const std::vector<uint64_t> &i_BucketIdList )
  {
    BitMap content;
    for (size_t i=0; i<i_BucketIdList.size(); i++)
    {
      uint64_t bucketId = i_BucketIdList[i];
      if ( bucketId >= _BucketList.size())
      {
        // skip the out of range bucket id
        //LOG(csmd, error) << "EventSourceEnvironment: The bucket id is out of range " << bucketId;
      }
      else
      {
        LOG(csmd, trace) << "EventSourceEnvironmental: Add the bucket id " << bucketId;
        content += _BucketList[ bucketId ];
      }
    }

    if ( !content.Empty() )
      return ( new EnvironmentalEvent(content, EVENT_TYPE_ENVIRONMENTAL) );
    else
      return nullptr;
  }

  virtual bool QueueEvent( const csm::daemon::CoreEvent *i_Event )
  {
    WakeUpMainLoop();
    return true;
  }
};

}  // namespace daemon
}  // namespace csm

#endif
