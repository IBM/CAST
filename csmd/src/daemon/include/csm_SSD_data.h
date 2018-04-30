/*================================================================================

    csmd/src/daemon/include/csm_SSD_data.h

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
/* csm_SSD_data.h ***************************************************************/

#ifndef CSMD_SRC_DAEMON_INCLUDE_CSM_SSD_DATA_H_
#define CSMD_SRC_DAEMON_INCLUDE_CSM_SSD_DATA_H_

#include "logging.h"

#include <boost/serialization/serialization.hpp>
#include <boost/serialization/vector.hpp>

#include <sstream>
#include <string>
#include <vector>

class CSM_SSD_Data
{

 public:

  CSM_SSD_Data(){

   // allocation of the vectors

  }
  
  ~CSM_SSD_Data(){

   // clearing of the vectors
  
  }

  template<class Archive>
  void serialize(Archive &archive, const unsigned int version)
  {

  }

 private:

  
};

#endif
