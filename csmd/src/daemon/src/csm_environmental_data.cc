/*================================================================================

    csmd/src/daemon/src/csm_environmental_data.cc

  © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    /U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#include "include/csm_environmental_data.h"

CSM_Environmental_Data::CSM_Environmental_Data()
{
  _Data_Mask.reset();
}
  
CSM_Environmental_Data::CSM_Environmental_Data( const CSM_Environmental_Data& in ) : 
  _Data_Mask( in._Data_Mask ),
  _CPU_Data( in._CPU_Data ),
  _GPU_Double_Data( in._GPU_Double_Data ),
  _GPU_Long_Data( in._GPU_Long_Data ),
  _GPU_Double_Label_Data( in._GPU_Double_Label_Data ),
  _GPU_Long_Label_Data( in._GPU_Long_Label_Data )
{
}

CSM_Environmental_Data::~CSM_Environmental_Data()
{
}

void CSM_Environmental_Data::Get_GPU_Double_DCGM_Field_Values_And_Set_Bit() 
{ 
  _GPU_Double_Data.Get_Double_DCGM_Field_Values();
  _Data_Mask.set( GPU_DOUBLE_DATA_BIT ); 
}

void CSM_Environmental_Data::Get_GPU_Long_DCGM_Field_Values_And_Set_Bit() 
{
  _GPU_Long_Data.Get_Long_DCGM_Field_Values();
  _Data_Mask.set( GPU_LONG_DATA_BIT );
}

void CSM_Environmental_Data::Get_GPU_Double_DCGM_Field_String_Identifiers_And_Set_Bit()
{ 
  _GPU_Double_Label_Data.Get_Double_DCGM_Field_String_Identifiers();
  _Data_Mask.set( GPU_DOUBLE_LABEL_BIT ); 
}

void CSM_Environmental_Data::Get_GPU_Long_DCGM_Field_String_Identifiers_And_Set_Bit()
{
  _GPU_Long_Label_Data.Get_Long_DCGM_Field_String_Identifiers();
  _Data_Mask.set( GPU_LONG_LABEL_BIT );
}

void CSM_Environmental_Data::Print_GPU_Double_DCGM_Field_Values()
{
  _GPU_Double_Data.Print_Double_DCGM_Field_Values();
}

void CSM_Environmental_Data::Print_GPU_Long_DCGM_Field_Values()
{
  _GPU_Long_Data.Print_Long_DCGM_Field_Values();
}

void CSM_Environmental_Data::Print_GPU_Double_DCGM_Field_String_Identifiers()
{
  _GPU_Double_Label_Data.Print_Double_DCGM_Field_String_Identifiers();
}

void CSM_Environmental_Data::Print_GPU_Long_DCGM_Field_String_Identifiers()
{
  _GPU_Long_Label_Data.Print_Long_DCGM_Field_String_Identifiers();
}

void CSM_Environmental_Data::Print()
{
  LOG( csmenv, debug ) << " ENVDATA: BitSet:" << _Data_Mask.to_string();

  if( _Data_Mask.test( GPU_DOUBLE_LABEL_BIT ) ){ Print_GPU_Double_DCGM_Field_String_Identifiers(); }
  if( _Data_Mask.test( GPU_DOUBLE_DATA_BIT ) ){ Print_GPU_Double_DCGM_Field_Values(); }

  if( _Data_Mask.test( GPU_LONG_LABEL_BIT ) ){ Print_GPU_Long_DCGM_Field_String_Identifiers(); }
  if( _Data_Mask.test( GPU_LONG_DATA_BIT ) ){ Print_GPU_Long_DCGM_Field_Values(); }

  /*
  if( _Data_Mask.test( CPU_DATA_BIT ) ){ LOG( csmd, debug ) << "       DummyInt = " << _CPUData._DummyTestInt; }
  */
}

CSM_CPU_Data& CSM_Environmental_Data::Return_CPU_Data_Object()
{ 
  return (this->_CPU_Data);
}

CSM_GPU_Double_Data& CSM_Environmental_Data::Return_GPU_Double_Data_Object()
{
  return (this->_GPU_Double_Data);
}

CSM_GPU_Long_Data& CSM_Environmental_Data::Return_GPU_Long_Data_Object()
{ 
  return (this->_GPU_Long_Data); 
}

CSM_GPU_Double_Label_Data& CSM_Environmental_Data::Return_GPU_Double_Label_Data_Object()
{ 
  return (this->_GPU_Double_Label_Data);
}

CSM_GPU_Long_Label_Data& CSM_Environmental_Data::Return_GPU_Long_Label_Data_Object()
{
  return (this->_GPU_Long_Label_Data);
}

void CSM_Environmental_Data::Set_Data( const CSM_GPU_Double_Data& GPU_Double_Data_To_Copy )
{
  _GPU_Double_Data = GPU_Double_Data_To_Copy;
  _Data_Mask.set( GPU_DOUBLE_DATA_BIT );
}

void CSM_Environmental_Data::Set_Data( const CSM_GPU_Long_Data& GPU_Long_Data_To_Copy )
{
  _GPU_Long_Data = GPU_Long_Data_To_Copy;
  _Data_Mask.set( GPU_LONG_DATA_BIT );
}

void CSM_Environmental_Data::Set_Data( const CSM_GPU_Double_Label_Data& GPU_Double_Label_Data_To_Copy )
{
  _GPU_Double_Label_Data = GPU_Double_Label_Data_To_Copy;
  _Data_Mask.set( GPU_DOUBLE_LABEL_BIT );
}

void CSM_Environmental_Data::Set_Data( const CSM_GPU_Long_Label_Data& GPU_Long_Label_Data_To_Copy )
{
  _GPU_Long_Label_Data = GPU_Long_Label_Data_To_Copy;
  _Data_Mask.set( GPU_LONG_LABEL_BIT );
}

void CSM_Environmental_Data::Set_Data( const CSM_CPU_Data& CPU_data_to_copy )
{
  _CPU_Data = CPU_data_to_copy;
  _Data_Mask.set( CPU_DATA_BIT );
}

std::string CSM_Environmental_Data::Get_Json_String()
{
  return "Test string";
}

CSM_Environmental_Data& CSM_Environmental_Data::operator=( const CSM_Environmental_Data& in )
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
CSM_Environmental_Data& CSM_Environmental_Data::operator|=( const CSM_Environmental_Data& in )
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

bool CSM_Environmental_Data::HasData() const
{
  return _Data_Mask.any();
}
