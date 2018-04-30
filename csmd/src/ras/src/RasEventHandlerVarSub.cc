/*================================================================================

    csmd/src/ras/src/RasEventHandlerVarSub.cc

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#include "csm_api_ras.h"
#include "../include/RasEventHandlerVarSub.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <stdio.h>
#include <map>
#include <boost/regex.hpp>


//#include "RasLog.h"
using namespace std;

//#ifdef __linux
//LOG_DECLARE_FILE("ras");
//#endif

RasEvent& RasEventHandlerVarSub::handle(RasEvent& event)
{

  //LOG_TRACE_MSG("RasEventHandlerVarSub handling event id=" << event.getDetail(RasEvent::MSG_ID).c_str() << " msg=" <<  event.getDetail(RasEvent::MESSAGE).c_str());

    // this needs a rewrite, in bluegene, when the meta data is processed the variables can be located
    //    but with this CORAL scheme we have to do it much later in the process...
    // 
    // scan the "message" field for $(xxx) and extract the each of the variable names.
    // for each of these names,  {
    //     if (event field name with same value) {
    //         substutite value.
    //     }
    // }
    //
    string msg = event.getValue(CSM_RAS_FKEY_MESSAGE);
    boost::sregex_token_iterator iter(msg.begin(), msg.end(), _var_regex, 0);   // extract $(xxxx) values...
    boost::sregex_token_iterator end;
    vector<string> vars;
    for( ; iter != end; ++iter ) {
        string s = (*iter);
        string s1 = s.substr(2, s.size()-3);
        vars.push_back(s1);
    }

    std::string getValue(const std::string &key);
    unsigned nsubs = 0;
    // do the substutition for each var...  We allow any ras field name except MESSAGE, to prevent recursion...
    for (unsigned i = 0; i < vars.size(); ++i) {
        if (vars[i] == CSM_RAS_FKEY_MESSAGE)
            continue;
        if (!event.hasValue(vars[i]))
            continue;
        string var_value = event.getValue(vars[i]);
        string var = "$(" + vars[i] + ")";
        string::size_type v_index = msg.find(var, 0);
        if (v_index != string::npos) { 
            // replace the variable with its value
            msg.replace(v_index, var.size(), var_value);
            nsubs++;
        }
    }
    if (nsubs) {        // if we substutited, then put the message back...
        event.setValue(CSM_RAS_FKEY_MESSAGE, msg);
    }


  return event;
}

RasEventHandlerVarSub::RasEventHandlerVarSub() :  
    _var_regex("\\$\\(.*?\\)"),     // look for $(xxxx)....
    _name("RasEventHandlerVarSub"), 
    _std_keys()

{

}

RasEventHandlerVarSub::~RasEventHandlerVarSub()
{
}


