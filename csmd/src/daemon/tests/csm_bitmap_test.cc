/*================================================================================

    csmd/src/daemon/tests/csm_bitmap_test.cc

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#include "include/csm_bitmap.h"

#include "logging.h"

#include <set>
#include <iostream>

int main(int argc, char *argv[])
{
  csm::daemon::BitMap bitmap;
  bitmap.EmptySet();
  assert(bitmap.GetMembers() == 0);
  bitmap.FillSet();
  assert(bitmap.GetMembers() == bitmap.GetNumOfBits());
  bitmap.EmptySet();
  bitmap.AddSet(bitmap.GetNumOfBits()-1);
  for (size_t i=0; i<bitmap.GetNumOfBits(); i++)
  {
    if (i == bitmap.GetNumOfBits()-1)
      assert(bitmap.IsMember(i));
    else
      assert( !bitmap.IsMember(i) );
  }
  
  bitmap.EmptySet();
  bitmap.AddSet(bitmap.GetNumOfBits()/2);
  for (size_t i=0; i<bitmap.GetNumOfBits(); i++)
  {
    if (i == bitmap.GetNumOfBits()/2)
      assert(bitmap.IsMember(i));
    else
      assert( !bitmap.IsMember(i) );
  }
  bitmap.AddSet(bitmap.GetNumOfBits()-1);
  
  LOG(csmd, info) << "bitmap = 0x" << std::hex << bitmap.GetMask();
  
  bitmap.RemoveSet(bitmap.GetNumOfBits()/2);
  assert( !bitmap.IsMember(bitmap.GetNumOfBits()/2) );
  assert(bitmap.IsMember(bitmap.GetNumOfBits()-1));
  
  bitmap.AddSet(bitmap.GetNumOfBits());
  assert( !bitmap.IsMember(bitmap.GetNumOfBits()) );

}
