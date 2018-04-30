/*================================================================================

    csmd/src/daemon/include/csm_environmental_data.h

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    /U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#ifndef CSMD_SRC_DAEMON_INCLUDE_CSM_ENVIRONMENTAL_DATA_H_
#define CSMD_SRC_DAEMON_INCLUDE_CSM_ENVIRONMENTAL_DATA_H_

#include <bitset>
#include "csm_CPU_data.h"
#include "csm_GPU_double_data.h"
#include "csm_GPU_long_data.h"
#include "csm_GPU_double_labels.h"
#include "csm_GPU_long_labels.h"

class CSM_Environmental_Data
{

  typedef enum
  {

    CPU_DATA_BIT,
    CPU_LABEL_BIT,

    GPU_DOUBLE_DATA_BIT,
    GPU_LONG_DATA_BIT,

    GPU_DOUBLE_LABEL_BIT,
    GPU_LONG_LABEL_BIT,

    SSD_DATA_BIT,
    SSD_LABEL_BIT,

    MAX_DATA_BIT

  } BitDefinitions;

public:

  CSM_Environmental_Data()
  {
    _Data_Mask.reset();
  }
  
  CSM_Environmental_Data( const CSM_Environmental_Data& in )
  : _Data_Mask( in._Data_Mask ),
    _CPU_Data( in._CPU_Data ),
    _GPU_Double_Data( in._GPU_Double_Data ),
    _GPU_Long_Data( in._GPU_Long_Data ),
    _GPU_Double_Label_Data( in._GPU_Double_Label_Data ),
    _GPU_Long_Label_Data( in._GPU_Long_Label_Data )
  {}

  ~CSM_Environmental_Data()
  {
  }

  inline void Get_GPU_Double_DCGM_Field_Values_And_Set_Bit() { _GPU_Double_Data.Get_Double_DCGM_Field_Values(); _Data_Mask.set( GPU_DOUBLE_DATA_BIT ); }
  inline void Get_GPU_Long_DCGM_Field_Values_And_Set_Bit() {       _GPU_Long_Data.Get_Long_DCGM_Field_Values(); _Data_Mask.set( GPU_LONG_DATA_BIT ); }

  inline void Get_GPU_Double_DCGM_Field_String_Identifiers_And_Set_Bit(){ _GPU_Double_Label_Data.Get_Double_DCGM_Field_String_Identifiers(); _Data_Mask.set( GPU_DOUBLE_LABEL_BIT ); }
  inline void Get_GPU_Long_DCGM_Field_String_Identifiers_And_Set_Bit(){       _GPU_Long_Label_Data.Get_Long_DCGM_Field_String_Identifiers(); _Data_Mask.set( GPU_LONG_LABEL_BIT ); }

  inline void Print_GPU_Double_DCGM_Field_Values(){                         _GPU_Double_Data.Print_Double_DCGM_Field_Values(); }
  inline void Print_GPU_Long_DCGM_Field_Values(){                               _GPU_Long_Data.Print_Long_DCGM_Field_Values(); }

  inline void Print_GPU_Double_DCGM_Field_String_Identifiers(){ _GPU_Double_Label_Data.Print_Double_DCGM_Field_String_Identifiers(); }
  inline void Print_GPU_Long_DCGM_Field_String_Identifiers(){       _GPU_Long_Label_Data.Print_Long_DCGM_Field_String_Identifiers(); }

  void Print(){

    LOG( csmd, debug ) << " ENVDATA: BitSet:" << _Data_Mask.to_string();

    if( _Data_Mask.test( GPU_DOUBLE_LABEL_BIT ) ){ Print_GPU_Double_DCGM_Field_String_Identifiers(); }
    if( _Data_Mask.test( GPU_DOUBLE_DATA_BIT ) ){ Print_GPU_Double_DCGM_Field_Values(); }

    if( _Data_Mask.test( GPU_LONG_LABEL_BIT ) ){ Print_GPU_Long_DCGM_Field_String_Identifiers(); }
    if( _Data_Mask.test( GPU_LONG_DATA_BIT ) ){ Print_GPU_Long_DCGM_Field_Values(); }

    /*
    if( _Data_Mask.test( CPU_DATA_BIT ) ){ LOG( csmd, debug ) << "       DummyInt = " << _CPUData._DummyTestInt; }
    */

  }

  inline CSM_CPU_Data& Return_CPU_Data_Object(){ return (this->_CPU_Data); }

  inline CSM_GPU_Double_Data& Return_GPU_Double_Data_Object(){ return (this->_GPU_Double_Data); }
  inline CSM_GPU_Long_Data& Return_GPU_Long_Data_Object(){ return (this->_GPU_Long_Data); }
  inline CSM_GPU_Double_Label_Data& Return_GPU_Double_Label_Data_Object(){ return (this->_GPU_Double_Label_Data); }
  inline CSM_GPU_Long_Label_Data& Return_GPU_Long_Label_Data_Object(){ return (this->_GPU_Long_Label_Data); }

  void Set_Data( const CSM_GPU_Double_Data& GPU_Double_Data_To_Copy )
  {
    _GPU_Double_Data = GPU_Double_Data_To_Copy;
   _Data_Mask.set( GPU_DOUBLE_DATA_BIT );
  }

  void Set_Data( const CSM_GPU_Long_Data& GPU_Long_Data_To_Copy )
  {
    _GPU_Long_Data = GPU_Long_Data_To_Copy;
   _Data_Mask.set( GPU_LONG_DATA_BIT );
  }

  void Set_Data( const CSM_GPU_Double_Label_Data& GPU_Double_Label_Data_To_Copy )
  {
    _GPU_Double_Label_Data = GPU_Double_Label_Data_To_Copy;
   _Data_Mask.set( GPU_DOUBLE_LABEL_BIT );
  }

  void Set_Data( const CSM_GPU_Long_Label_Data& GPU_Long_Label_Data_To_Copy )
  {
    _GPU_Long_Label_Data = GPU_Long_Label_Data_To_Copy;
   _Data_Mask.set( GPU_LONG_LABEL_BIT );
  }

  void Set_Data( const CSM_CPU_Data& CPU_data_to_copy )
  {
    _CPU_Data = CPU_data_to_copy;
   _Data_Mask.set( CPU_DATA_BIT );
  }

  CSM_Environmental_Data& operator=( const CSM_Environmental_Data& in )
  {
    _Data_Mask = in._Data_Mask;
    _GPU_Double_Data = in._GPU_Double_Data;
    _GPU_Long_Data = in._GPU_Long_Data;
    _GPU_Double_Label_Data = in._GPU_Double_Label_Data;
    _GPU_Long_Label_Data = in._GPU_Long_Label_Data;
    _CPU_Data = in._CPU_Data;
    return *this;
  }

  // operator to only update the items that are present in the input
  CSM_Environmental_Data& operator|=( const CSM_Environmental_Data& in )
  {
    _Data_Mask |= in._Data_Mask;

    if( in._Data_Mask.test( GPU_DOUBLE_DATA_BIT ) )
      _GPU_Double_Data = in._GPU_Double_Data;

    if( in._Data_Mask.test( GPU_LONG_DATA_BIT ) )
      _GPU_Long_Data = in._GPU_Long_Data;

   if( in._Data_Mask.test( GPU_DOUBLE_LABEL_BIT ) )
      _GPU_Double_Label_Data = in._GPU_Double_Label_Data;

    if( in._Data_Mask.test( GPU_LONG_LABEL_BIT ) )
      _GPU_Long_Label_Data = in._GPU_Long_Label_Data;

    if( in._Data_Mask.test( CPU_DATA_BIT ) )
      _CPU_Data = in._CPU_Data;

    return *this;
  }

  inline bool HasData() const
  {
    return _Data_Mask.any();
  }

private:
   friend class boost::serialization::access;

   template <class Archive>
  void serialize(Archive &archive, const unsigned int version)
  {
     // serialize the bitset as a sting
     std::string dmString = _Data_Mask.to_string();
     archive & dmString;

     // update from the string for the deserialization path
     _Data_Mask = std::bitset<MAX_DATA_BIT>( dmString );

     // check and archive the content
     if( _Data_Mask.test( GPU_DOUBLE_DATA_BIT ) )
       archive & _GPU_Double_Data;

    if( _Data_Mask.test( GPU_LONG_DATA_BIT ) )
       archive & _GPU_Long_Data;

    if( _Data_Mask.test( GPU_DOUBLE_LABEL_BIT ) )
       archive & _GPU_Double_Label_Data;

    if( _Data_Mask.test( GPU_LONG_LABEL_BIT ) )
       archive & _GPU_Long_Label_Data;

     if( _Data_Mask.test( CPU_DATA_BIT ) )
       archive & _CPU_Data;
  }

 private:

  std::bitset<MAX_DATA_BIT> _Data_Mask;

  CSM_CPU_Data _CPU_Data;

  CSM_GPU_Double_Data _GPU_Double_Data;
  CSM_GPU_Long_Data _GPU_Long_Data;
  
  CSM_GPU_Double_Label_Data _GPU_Double_Label_Data;
  CSM_GPU_Long_Label_Data _GPU_Long_Label_Data;

};

#endif
