/*================================================================================

    csmd/src/daemon/tests/csm_event_test.cc

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
/* csm_event_test.cc
 *
 ******************************************/

#include "include/csm_core_event.h"

int main( int argc, char **argv )
{
  int rc = 0;

  csm::daemon::CPUInfo data;
  data.mCPUCount = 12;
  data.mFrequency = 5432;
  csm::daemon::CPUInfoEvent *cpuev = new csm::daemon::CPUInfoEvent( data,
                                                                    csm::daemon::EVENT_TYPE_SENSOR_Data,
                                                                    nullptr );
  csm::daemon::CPUInfo retrieved = cpuev->GetContent();

  std::cout << "CPUData: " << data << " @ " << (void*)&data<< std::endl;
  std::cout << "Eventdata: " << retrieved << std::endl;

  if( data.mCPUCount != retrieved.mCPUCount )
  {
    std::cerr << "CPUCount mismatch" << std::endl;
    rc += 1;
  }
  if( data.mFrequency != retrieved.mFrequency )
  {
    std::cerr << "Frequency mismatch" << std::endl;
    rc += 1;
  }

  return rc;
}
