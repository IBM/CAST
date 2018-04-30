/*================================================================================

    csmd/src/daemon/include/csm_CPU_data.h

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
/* csm_CPU_data.h ***************************************************************/

#ifndef CSMD_SRC_DAEMON_INCLUDE_CSM_CPU_DATA_H_
#define CSMD_SRC_DAEMON_INCLUDE_CSM_CPU_DATA_H_

#include "logging.h"

#include <boost/serialization/serialization.hpp>
#include <boost/serialization/vector.hpp>

#include <sstream>
#include <string>
#include <vector>

class CSM_CPU_Data
{

 public:

  CSM_CPU_Data()
 {

   // allocation of the vectors
    _DummyTestInt = 0;
  }
  
  CSM_CPU_Data( const CSM_CPU_Data & in )
  {
    _DummyTestInt = in._DummyTestInt;
    vector_double = in.vector_double;
    vector_int64_t = in.vector_int64_t;
  }

  ~CSM_CPU_Data()
  {

   // clearing of the vectors
  
  }

  CSM_CPU_Data& operator=( const CSM_CPU_Data &in )
  {
    _DummyTestInt = in._DummyTestInt;
    vector_double = in.vector_double;
    vector_int64_t = in.vector_int64_t;
    return *this;
  }

  template<class Archive>
  void serialize(Archive &archive, const unsigned int version)
  {
    archive & _DummyTestInt;
  }

  void DummyReadCPU()
  {
    _DummyTestInt = random() % 1000;
  }
 private:

  std::vector<double>    vector_double;
  std::vector<int64_t>   vector_int64_t;
public:
  int _DummyTestInt;
};

#endif
