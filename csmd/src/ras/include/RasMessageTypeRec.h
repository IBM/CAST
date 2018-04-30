/*================================================================================

    csmd/src/ras/include/RasMessageTypeRec.h

  © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#ifndef __RASMESSAGETYPEREC_H__
#define __RASMESSAGETYPEREC_H__

#include <ostream>

//
// temporary filler class to get data from the db.
//
struct RasMessageTypeRec
{
    std::string _msg_id;
    std::string _severity;
    std::string _message;
    std::string _description;
    std::string _control_action;
    int         _threshold_count;
    int         _threshold_period;
    bool        _enabled;
    std::string _set_state;
    bool        _visible_to_users;
protected:
    // static std::string _componentAsString[];
private:
    friend std::ostream& operator<< (std::ostream& o, const RasMessageTypeRec& r);

};


inline std::ostream& operator<< (std::ostream& o, const RasMessageTypeRec& r)
{
    using namespace std;
    o   << "_msg_id=" << r._msg_id << ",\n"
        << "_severity=" << r._severity << ",\n"
        << "_message=" << r._message << ",\n"
        << "_description=" << r._description << ",\n"
        << "_control_action=" << r._control_action << ",\n"
        << "_threshold_count=" << r._threshold_count << ",\n"
        << "_threshold_period=" << r._threshold_period << ",\n"
        << "_enabled=" << r._enabled << ",\n"
        << "_set_state=" << r._set_state << ",\n"
        << "_visible_to_users=" << r._visible_to_users << " "
        << endl;
    return o;
}




#endif
