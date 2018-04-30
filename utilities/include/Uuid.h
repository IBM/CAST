/*******************************************************************************
 |    Uuid.h
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

#ifndef COMMON_UUID_H_
#define COMMON_UUID_H_

#include <uuid/uuid.h>

/*******************************************************************************
 | Constants
 *******************************************************************************/
const size_t LENGTH_UUID_STR = 37;
const char HP_UUID[LENGTH_UUID_STR] = "ffffffff-ffff-ffff-ffff-ffffffffffff";

/*******************************************************************************
 | Classes
 *******************************************************************************/

class Uuid {
  public:
    Uuid() { clear(); }
    Uuid(const Uuid &src) { uuid_copy(uuid, src.uuid); }
    Uuid(const uuid_t src) { uuid_copy(uuid, src); }
    Uuid(const char* src) { copyFrom(src); return; }
    inline void clear() { uuid_clear(uuid); return; }
    inline int is_null() const { return uuid_is_null(uuid); }
    inline int copyFrom(const char* uuid_str) { return uuid_parse(uuid_str, uuid); }
    inline void copyTo(char* uuid_str) const { return uuid_unparse(uuid, uuid_str); }
    inline std::string str() const
    {
        char l_Temp[LENGTH_UUID_STR] = {'\0'};
        copyTo(l_Temp);

        return std::string(l_Temp);
    }
    inline void print() const
    {
        char l_Temp[LENGTH_UUID_STR] = {'\0'};
        copyTo(l_Temp);
        printf("Uuid.uuid = %s\n", l_Temp);

        return;
    }
    bool operator<(const Uuid &other) const { return uuid_compare(uuid, other.uuid) < 0; }
    bool operator>(const Uuid &other) const { return uuid_compare(uuid, other.uuid) > 0; }
    bool operator==(const Uuid &other) const { return uuid_compare(uuid, other.uuid) == 0; }
    bool operator!=(const Uuid &other) const { return !((*this) == other); }
    void operator=(const Uuid &other) { return uuid_copy(uuid, other.uuid); }
  private:
    uuid_t uuid;
};

inline std::ostream & operator<<(std::ostream &os, const Uuid& p)
{
    char l_Temp[LENGTH_UUID_STR] = {'\0'};
    p.copyTo(l_Temp);
    return os << l_Temp;
}

#endif /* COMMON_UUID_H_ */
