/*================================================================================

    csmd/src/daemon/include/csm_GPU_double_data.h

  © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
/* csm_GPU_double_data.h ***************************************************************/

#ifndef CSMD_SRC_DAEMON_INCLUDE_CSM_GPU_DOUBLE_DATA_H_
#define CSMD_SRC_DAEMON_INCLUDE_CSM_GPU_DOUBLE_DATA_H_

#include "csmd/src/inv/include/inv_dcgm_access.h"

#include "logging.h"

#include <boost/serialization/serialization.hpp>
#include <boost/serialization/vector.hpp>

#include <sstream>
#include <string>
#include <vector>

class CSM_GPU_Double_Data
{

 public:

  CSM_GPU_Double_Data()
  {

   // allocation of the vectors
   vector_double_dcgm_field_values.resize( 0 );

  }

  CSM_GPU_Double_Data( const CSM_GPU_Double_Data& GPU_Double_Data_To_Copy  )
  {

   // allocation of the vectors
   vector_double_dcgm_field_values.resize( 0 );

   // copy of the vectors
   vector_double_dcgm_field_values = GPU_Double_Data_To_Copy.vector_double_dcgm_field_values;
//Return_Double_DCGM_Vector();

  }

  ~CSM_GPU_Double_Data()
  {

   // clearing of the vectors
   vector_double_dcgm_field_values.clear();

  }
  
  CSM_GPU_Double_Data& operator = ( const CSM_GPU_Double_Data& GPU_Double_Data_To_Copy )
  {
  
   // allocation of the vectors
   vector_double_dcgm_field_values.resize( 0 );

   // copy of the vectors
   vector_double_dcgm_field_values = GPU_Double_Data_To_Copy.vector_double_dcgm_field_values;

   return *this; 

  }

  template <class Archive>
  void serialize(Archive &archive, const unsigned int version)
  {
    archive & vector_double_dcgm_field_values; 
  }
  
  void Get_Double_DCGM_Field_Values()
  {
    csm::daemon::INV_DCGM_ACCESS::GetInstance()->Get_Double_DCGM_Field_Values( vector_double_dcgm_field_values );
  }

  void Print_Double_DCGM_Field_Values()
  {
    LOG(csmenv, debug) << "Printing Double GPU Field Values: ";
    for (unsigned int i = 0; i < vector_double_dcgm_field_values.size(); i++ )
    {
      LOG(csmenv, debug) << "Double GPU field value: " <<  vector_double_dcgm_field_values.at(i);
    }
  }

  std::vector<double> Return_Double_DCGM_Vector(){ return vector_double_dcgm_field_values; }

 private:

  std::vector<double> vector_double_dcgm_field_values;
  
};

#endif
