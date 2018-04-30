/*******************************************************************************
 |    LVKey.h
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

#ifndef BB_LVKEY_H_
#define BB_LVKEY_H_

#include "Uuid.h"
#include <string.h>

typedef std::pair<std::string, Uuid> LVKey;

inline std::ostream & operator<<(std::ostream &os, const LVKey& p)
{
    return os << "LVKey(" << (p.first.empty() ? "None" : p.first) << "," << p.second << ")";
}

#endif /* BB_LVKEY_H_ */
