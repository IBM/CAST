/*================================================================================
   
    csmd/src/daemon/src/csmi_request_handler/helpers/csm_handler_exception.h

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
 
================================================================================*/

#ifndef _CSM_HANDLER_EXCEPTION_H_
#define _CSM_HANDLER_EXCEPTION_H_

#include <stdexcept>
#include <string>
#include "csmi/include/csmi_type_common.h"

namespace csm {
namespace daemon {
namespace helper {

class CSMHandlerException : public std::runtime_error
{
private:
    csmi_cmd_err_t _ErrorCode; ///< The error code associated with this exception. @ref CSMERR_GENERIC is the default.

public:
    CSMHandlerException() : 
        runtime_error( "Missing Error;" ), 
        _ErrorCode( CSMERR_GENERIC )
    {}

    CSMHandlerException(const std::string& msg, const csmi_cmd_err_t errcode ) :
        runtime_error ( msg ),
        _ErrorCode( errcode )
    {}

    CSMHandlerException(const char*  msg, const csmi_cmd_err_t errcode ) :
        runtime_error ( msg ),
        _ErrorCode( errcode )
    {}

    /** @brief A getter for the exception's error code.
     * @return The error code associated with this error.
     */
    inline csmi_cmd_err_t type() const { return _ErrorCode; };

};

} // End namespace helpers
} // End namespace daemon
} // End namespace csm

#endif
