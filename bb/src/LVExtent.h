/*******************************************************************************
 |    BBTagInfo.h
 |
 |  © Copyright IBM Corporation 2015,2016. All Rights Reserved
 |
 |    This program is licensed under the terms of the Eclipse Public License
 |    v1.0 as published by the Eclipse Foundation and available at
 |    http://www.eclipse.org/legal/epl-v10.html
 |
 |    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
 |    restricted by GSA ADP Schedule Contract with IBM Corp.
 *******************************************************************************/

#ifndef BB_LVEXTENT_H_
#define BB_LVEXTENT_H_

#include <string>

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

using namespace std;

//
// LVLookup class
//

class LVExtent
{
  public:
    string   device;
    string   serial;
    uint64_t pstart;
    uint64_t lstart;
    uint64_t len;
};

#endif /* BB_LVEXTENT_H_ */

