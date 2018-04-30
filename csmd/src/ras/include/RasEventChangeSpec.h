/*================================================================================

    csmd/src/ras/include/RasEventChangeSpec.h

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#ifndef __RASEVENTCHANGESPEC_H__
#define __RASEVENTCHANGESPEC_H__

#include <string>



/**
 * RasEventChangeSpec
 * 
 * Change specification for a ras event.
 * Given a message ID modify the key and value
 * 
 */
class RasEventChangeSpec
{
public:
    RasEventChangeSpec(const std::string &msg_id,
                       const std::string &key,
                       const std::string &value) :
        _msg_id(msg_id),
        _key(key),
        _value(value) {};

    const std::string &msg_id()  { return(_msg_id); };
    const std::string &key()    { return(_key); };
    const std::string &value()  { return(_value); };
protected:
    std::string _msg_id;
    std::string _key;
    std::string _value;

private:

};



#endif

