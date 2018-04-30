/*================================================================================

    csmd/src/ras/include/RasEventHandlerVarSub.h

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#ifndef __RASEVENT_VARSUBSTITUTE_H_
#define __RASEVENT_VARSUBSTITUTE_H_

#include "RasEventHandler.h"

#include <set>
#include <string>
#include <boost/regex.hpp>


class RasEventHandlerVarSub : public RasEventHandler
{
public:
    virtual RasEvent& handle(RasEvent& event);
    virtual const std::string& name() { return _name; }
    RasEventHandlerVarSub();
    virtual ~RasEventHandlerVarSub();
protected:
    // regex expressions for locating variable substutitions...
    boost::regex _var_regex;

private:
    std::string _name;
    std::set<std::string> _std_keys;
};

#endif /*VARSUBSTITUTE_H_*/


