/*================================================================================

    csmd/src/ras/include/RasRc.h

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#ifndef __RAS_RC_H__
#define __RAS_RC_H__

#include <string>
/**
 * container class for Ras subsystem return codes. 
 *  
 * objects in the RAS subsystem return return codes in. 
 * return numeric codes are found in: 
 *    csmi/src/common/include/csmi_cmd_error.h 
 */




class RasRc  
{
public:
    RasRc(int rc) : _rc(rc) {
    };
    RasRc(int rc, const std::string &errstr) :
        _rc(rc),
        _errstr(errstr) {
    };
    RasRc& operator=(RasRc other)
    {
        _rc = other._rc;
        _errstr = other._errstr;
        return *this;
    }

    int _rc;
    std::string _errstr;

protected:

private:
    RasRc() : _rc(0) {};


};



#endif




