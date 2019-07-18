/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/csm_environmental_handler.h

  © Copyright IBM Corporation 2015-2019. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#ifndef __CSM_DAEMON_SRC_CSM_DAEMON_ENVIRONMENTAL_HANDLER_H__
#define __CSM_DAEMON_SRC_CSM_DAEMON_ENVIRONMENTAL_HANDLER_H__

#include "csmi_base.h"
#include "csm_daemon_config.h"
#include "include/csm_bitmap.h"

#include "csmd/src/daemon/include/csm_environmental_data.h"

#include <memory>

class CSM_ENVIRONMENTAL: public CSMI_BASE {
private:
  csm::daemon::BucketItemType _startItem;
  csm::daemon::EventContext_sptr _context;
  csm::daemon::BitMap _pendingItems;
  CSM_Environmental_Data * Environmental_data;

public:
  CSM_ENVIRONMENTAL(csm::daemon::HandlerOptions& options)
  : CSMI_BASE(CSM_CMD_UNDEFINED, options),
    _startItem(),
    _context( nullptr ),
    _pendingItems(),
    Environmental_data( nullptr )
  {
    setCmdName(std::string("CSM_ENVIRONMENTAL"));
  }
  virtual ~CSM_ENVIRONMENTAL() {}
  virtual void Process( const csm::daemon::CoreEvent &aEvent,
                std::vector<csm::daemon::CoreEvent*>& postEventList );
  
private:
  inline csm::daemon::BucketItemType GetUserData()
  {
    if( _context == nullptr )
      return csm::daemon::BucketItemType::DEFAULT;
    return ( (csm::daemon::BucketItemType) *((int *) _context->GetUserData()) );
  }
  
  inline void UpdateUserData(csm::daemon::BucketItemType aType)
  {
    if( _context == nullptr )
      return;
    int *userData = (int *) _context->GetUserData();
    if (userData)
    {
      *userData = aType;
    }
  }
  
  inline csm::daemon::BucketItemType GetNextBucketItem(csm::daemon::BucketItemType aType)
  {
    return ( (csm::daemon::BucketItemType) ((aType + 1) % (int) csm::daemon::NUM_BUCKET_ITEMS) );
  }

private:

  bool BuildSsdWearUpdate(std::vector<csm::daemon::CoreEvent*>& postEventList);
};

class CSM_ENVIRONMENTAL_UTILITY : public CSM_ENVIRONMENTAL
{
public:
  CSM_ENVIRONMENTAL_UTILITY(csm::daemon::HandlerOptions& options)
  : CSM_ENVIRONMENTAL(options)
  {
  }
  virtual void Process( const csm::daemon::CoreEvent &aEvent,
                std::vector<csm::daemon::CoreEvent*>& postEventList )
  {
    return;
  }

};

#endif
