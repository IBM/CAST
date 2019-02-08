/*******************************************************************************
 |    BBTagInfo.h
 |
 |  � Copyright IBM Corporation 2015,2016. All Rights Reserved
 |
 |    This program is licensed under the terms of the Eclipse Public License
 |    v1.0 as published by the Eclipse Foundation and available at
 |    http://www.eclipse.org/legal/epl-v10.html
 |
 |    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
 |    restricted by GSA ADP Schedule Contract with IBM Corp.
 *******************************************************************************/

#ifndef BB_LVLOOKUP_H_
#define BB_LVLOOKUP_H_

#include <map>
#include <string>
#include <vector>

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

using namespace std;

#include "bbinternal.h"
#include "LVExtent.h"

/*******************************************************************************
 | Forward declarations
 *******************************************************************************/
class Extent;
class filehandle;


// Implemented in serial.cc
extern string getDeviceBySerial(string serial);
extern string getSerialByDevice(string device);
extern string getNVMeByIndex(uint32_t index);
extern string getNVMeDeviceInfo(string device, string key);
extern vector<string> getDeviceSerials();
extern void findSerials(void);
extern int removeDeviceSerial(string serial);
extern string getKeyByHostname(string hostname);

extern int convertLVMSizeToBytes(const string& size, ssize_t& bytes);
extern int getVGSize(const string& vg, ssize_t& vgfree, ssize_t& vgtotal);

//
// LVLookup class
//

class LVLookup
{
  public:
    LVLookup() {};

    int build(const string& pVolumeGroup, const string& pLogicalVolumeName);

    int build(const filehandle& fileh, const string& vg);

    /**
     \brief Return data pertinent to the logical volume

     \param[out] pFirstByte First byte of the logical volume
     \param[out] pSize Size of the logical volume
     \return error indicator
     */
    int getData(uint64_t& pFirstByte, size_t& pSize);

    /**
     \brief Fetches all the segment information for all Logical Volume(s)

     \param[in] vg Burst buffer volume group
     \return error indicator
     */
    int getLVExtents(const string& vg);

    int translate(Extent& input, vector<Extent>& list);

  private:
    /**
     \brief Fetch the Logical Volume name for a given file
     Obtains the logical volume name from the file system and stores it in the private member 'lvname'

     \param[in] filename Path to the file
     \return error indicator
     */
    int getLVName(const filehandle& fileh);

    inline void setLVName(const char* pName)
    {
        lvname = pName;
    }

    inline void setLVName(const string& pName)
    {
        lvname = pName;
    }

    string lvname;
    map< string, vector<LVExtent> > LVMapping;
};

#endif /* BB_LVLOOKUP_H_ */

