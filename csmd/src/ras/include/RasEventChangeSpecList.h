/*================================================================================

    csmd/src/ras/include/RasEventChangeSpecList.h

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#ifndef __RasEventChangeSpecList_H__
#define __RasEventChangeSpecList_H__


#include <string>
#include <vector>
#include <map>
#include "RasEventChangeSpec.h"


/**
 * container class to hold ras filter specifications
 */

// todo rename to RasEventFiterSpecs and 
class RasEventChangeSpecList
{
public:

    /**
     * Read the event filter from a file...
     * 
     * @param fname 
     * @param envMap 
     * 
     * @return int 
     */
    int readEventFilter(const std::string &env, const std::string &fname);

    typedef std::vector<RasEventChangeSpec> RasEventSpecVect;

    RasEventSpecVect::iterator begin() { return(_specs.begin()); };
    RasEventSpecVect::iterator end()   { return(_specs.end()); };
    unsigned size() { return(_specs.size()); };



protected:

    RasEventSpecVect _specs;

    void addEventSpec(const std::string &msgId,
                      const std::string &key,
                      const std::string &value);

private:

};


#endif

