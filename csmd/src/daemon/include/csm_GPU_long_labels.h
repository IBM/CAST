/*================================================================================

    csmd/src/daemon/include/csm_GPU_long_labels.h

  © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
/* csm_GPU_long_labels.h ***************************************************************/

#ifndef CSMD_SRC_DAEMON_INCLUDE_CSM_GPU_LONG_LABELS_H_
#define CSMD_SRC_DAEMON_INCLUDE_CSM_GPU_LONG_LABELS_H_

#include "csmd/src/inv/include/inv_dcgm_access.h"

#include "logging.h"

#include <boost/serialization/serialization.hpp>
#include <boost/serialization/vector.hpp>

#include <sstream>
#include <string>
#include <vector>

class CSM_GPU_Long_Label_Data
{

 public:

  CSM_GPU_Long_Label_Data()
  {

   // allocation of the vectors
   vector_labels_for_long_dcgm_fields.resize( 0 );

  }

  CSM_GPU_Long_Label_Data( const CSM_GPU_Long_Label_Data& GPU_Long_Label_Data_To_Copy  )
  {

   // allocation of the vectors
   vector_labels_for_long_dcgm_fields.resize( 0 );

   // copy of the vectors
   vector_labels_for_long_dcgm_fields = GPU_Long_Label_Data_To_Copy.vector_labels_for_long_dcgm_fields;
//Return_Long_DCGM_Field_String_Identifiers();

  }

  ~CSM_GPU_Long_Label_Data()
  {

   // clearing of the vectors
   vector_labels_for_long_dcgm_fields.clear();

  }
  
  CSM_GPU_Long_Label_Data& operator = ( const CSM_GPU_Long_Label_Data& GPU_Long_Label_Data_To_Copy )
  {
  
   // allocation of the vectors
   vector_labels_for_long_dcgm_fields.resize( 0 );

   // copy of the vectors
   vector_labels_for_long_dcgm_fields = GPU_Long_Label_Data_To_Copy.vector_labels_for_long_dcgm_fields;
//Return_Long_DCGM_Field_String_Identifiers();

  return *this;

  }

  template <class Archive>
  void serialize(Archive &archive, const unsigned int version)
  {
    archive & vector_labels_for_long_dcgm_fields;
  }
  
  void Get_Long_DCGM_Field_String_Identifiers()
  {
    csm::daemon::INV_DCGM_ACCESS::GetInstance()->Get_Long_DCGM_Field_String_Identifiers( vector_labels_for_long_dcgm_fields );
  }

  void  Print_Long_DCGM_Field_String_Identifiers()
  {
    LOG(csmenv, debug) << "Printing Int64 GPU Field String Identifiers: ";
    for (unsigned int i = 0; i <  vector_labels_for_long_dcgm_fields.size(); i++ )
    {
      LOG(csmenv, debug) << "Int64 GPU field String Identifier: " <<  vector_labels_for_long_dcgm_fields.at(i);
    }
  }

  std::vector<std::string> Return_Long_DCGM_Field_String_Identifiers(){ return vector_labels_for_long_dcgm_fields; }

 private:

  std::vector<std::string> vector_labels_for_long_dcgm_fields;
  
};

#endif
