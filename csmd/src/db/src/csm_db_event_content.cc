/*================================================================================

    csmd/src/db/src/csm_db_event_content.cc

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#include "include/csm_core_event.h"           //: DBResult_sptr (XXX I think -John Dunham)
#include "csmd/src/db/include/csm_db_event_content.h"
#include "csmd/src/db/include/DBConnection.h" //: DBConn source.
#include "logging.h"                          //: For logging funtionality.
#include <libpq-fe.h>                         //: Postgresql header.
#include <boost/locale.hpp>

namespace csm {
namespace db {

// FIXME why is this called ~3 times?
DBReqContent::DBReqContent( const DBReqContent &obj ):
    DBContent(obj),
    _SqlStmt(obj._SqlStmt),
    _ParamIndex(obj._ParamIndex)
{
    if (obj._NumParams == 0 )
    {
        _NumParams    = 0;
        _ParamValues  = nullptr;
        _ParamSizes   = nullptr;
        _ParamFormats = nullptr;
        return;
    }

    // Param Values, Number of parameters, Param formats.
    if( obj._ParamValues != nullptr )
    {
        _NumParams = obj._NumParams;

        if ( obj._ParamSizes != nullptr )
        {
            _ParamValues = (char **)malloc( obj._NumParams * sizeof(char*) );
            _ParamSizes =    (int *)malloc( obj._NumParams * sizeof(int) );
        
            for ( int i = 0; i < _NumParams; ++i )
            {
            
                _ParamSizes[i]  = obj._ParamSizes[i];
                _ParamValues[i] = (char*)malloc( obj._ParamSizes[i] );
                memcpy( _ParamValues[i], obj._ParamValues[i], _ParamSizes[i] );
            }
        }
        else
        {
            _NumParams  = 0;
            _ParamValues = nullptr;
            _ParamSizes  = nullptr;
        }
    }

    // Formats
    if ( obj._ParamFormats != nullptr )
    {
        _ParamFormats = (int*) malloc(_NumParams * sizeof(int));
        memcpy( _ParamFormats, obj._ParamFormats, _NumParams * sizeof(int) );
    }
    else
        _ParamFormats = nullptr;

}

void DBReqContent::AddCharacterParam( char state )
{
    if ( _NumParams != 0  && _ParamIndex < _NumParams )
    {
        _ParamValues[_ParamIndex] =(char*) malloc(sizeof(char));
        memset( _ParamValues[_ParamIndex], state, sizeof(char));

        _ParamFormats[_ParamIndex] = 1;
        _ParamSizes[_ParamIndex]   = sizeof(char);
        _ParamIndex++;
    }
}

void DBReqContent::AddTextParam( const char* text )
{
    if ( text && _NumParams != 0  && _ParamIndex < _NumParams )
    {
        // If the text param was null calloc a size 1 string.
        std::string utf8Text = boost::locale::conv::utf_to_utf<char>(text);
        _ParamValues[_ParamIndex]  = (char*)strdup( utf8Text.c_str() );

        _ParamFormats[_ParamIndex] = 0; // Text field.
        _ParamSizes[_ParamIndex]   = strlen(_ParamValues[_ParamIndex]) + 1;
        _ParamIndex++;
    }
}

void DBReqContent::AddTextParam( const char* text, int size )
{
    if ( _NumParams != 0  && _ParamIndex < _NumParams )
    {
        if (text)
        {
            // Get the fixed string as a null terminated string.
            int tempSize = text[size-1] == 0 ? size : size + 1;
            char* tempText  = (char*) calloc(tempSize, sizeof(char));
            memcpy( tempText, text, tempSize );

            // Convert to utf8.
            std::string utf8Text = boost::locale::conv::utf_to_utf<char>(text);
            free(tempText);
            
            // Allocate the target point grab enough from the encoded string to fill.
            _ParamValues[_ParamIndex] =  (char*) calloc(size, sizeof(char));
            tempSize = size > 0 && (uint64_t) size > utf8Text.length() ? utf8Text.length() : size;
            memcpy(_ParamValues[_ParamIndex], utf8Text.c_str(), tempSize); 
        }
        else
        {
            // Empty string.
            _ParamValues[_ParamIndex] = (char*) calloc(size, sizeof(char));
        }

        _ParamFormats[_ParamIndex] = 0; // Text field.
        _ParamSizes[_ParamIndex]   = size;
        _ParamIndex++;
    }
}

void DBReqContent::AddTextArrayParam( const char* const * elements, uint32_t size )
{
    if ( _NumParams != 0  && _ParamIndex < _NumParams )
    {
        std::string arrayString = "{";

        if( elements )
        {
            for ( uint32_t i = 0; i < size; ++i)
            {
                if(elements[i] != nullptr )
                    arrayString.append(elements[i]).append(",");
            }
        }

        if( arrayString.length() > 1)
            arrayString[arrayString.length()-1] = '}';
        else
            arrayString.append("}");

        // Clean up the string for utf-8
        std::string utf8Text = boost::locale::conv::utf_to_utf<char>(arrayString);
        _ParamValues[_ParamIndex]  = (char*)strdup( utf8Text.c_str() );

        //_ParamValues[_ParamIndex]  = (char*)strdup(arrayString.c_str() );
        _ParamFormats[_ParamIndex] = 0; // Text field.
        _ParamSizes[_ParamIndex]   = strlen(_ParamValues[_ParamIndex])  + 1;
        _ParamIndex++;
    }
}

DBReqContent::~DBReqContent()
{
    if ( _ParamValues != nullptr )
    {
        for (int i = 0; i < _NumParams; i++)
        {
            if (_ParamValues[i] != nullptr )
            {
                free(_ParamValues[i]);
                _ParamValues[i] = nullptr;
            }
        }
        free(_ParamValues);
        _ParamValues = nullptr;
    }
    
    if ( _ParamFormats != nullptr )
    {
        free(_ParamFormats);
        _ParamFormats = nullptr;
    }

    if ( _ParamSizes != nullptr )
    {
        free(_ParamSizes);
        _ParamSizes = nullptr;
    }
}

} //namespace db
} //namespace csm

