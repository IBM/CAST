/*================================================================================

    csmd/src/daemon/include/csm_bitmap.h

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#ifndef CSM_DAEMON_SRC_CSM_BITMAP_H_
#define CSM_DAEMON_SRC_CSM_BITMAP_H_
#include <climits>
#include <set>

#include "logging.h"

#include "csmd/include/csm_daemon_config.h"

namespace csm {
namespace daemon {

// now the number of bits in the bitmap is limited to 64
class BitMap
{
private:
  uint64_t _Mask;
  size_t _NumOfBits;

public:
  BitMap()
  :_Mask(0),
   _NumOfBits(0)
  {
    _NumOfBits = CHAR_BIT * sizeof(uint64_t);
    //LOG(csmd, info) << "Number of Bits: " << _NumOfBits;
  }

  BitMap(const BitMap &aSrc)
  : _Mask(aSrc._Mask),
    _NumOfBits(aSrc._NumOfBits)
  { }
  
  virtual ~BitMap() {}
  
  BitMap& operator+=(const BitMap& rhs)
  {
    this->_Mask |= rhs._Mask;
    return *this;
  }

  BitMap operator+(const BitMap& rhs)
  {
    BitMap res;
    res._Mask |= rhs._Mask;
    res._Mask |= this->_Mask;
    return res;
  }
    
  // generic APIs
  inline size_t GetNumOfBits() const { return _NumOfBits; }
  inline uint64_t GetMask() const { return _Mask; }
  
  inline void EmptySet()
  {
    _Mask = 0;
  }
  
  inline void FillSet()
  {
    for (size_t i=0; i<_NumOfBits; i++)
    {
      _Mask |= (0x1ULL << i);
    }
  }
  
  inline bool Empty() { return _Mask == 0; }
  
  inline void AddSet(size_t aVal)
  {
    if ( aVal >= _NumOfBits)
    {
      LOG(csmd, error) << "BucketItemBitMap: out of bound in AddSet()";
    }
    else _Mask |=  (0x1ULL << aVal);
  }

  inline void RemoveSet(size_t aVal)
  {
    if ( aVal >= _NumOfBits)
    {
      LOG(csmd, error) << "BucketItemBitMap: out of bound in RemoveSet()";
    }
    else _Mask &=  ~(0x1ULL << aVal);
  }
  
  inline bool IsMember(size_t aVal)
  {
    if ( aVal >= _NumOfBits)
    {
      LOG(csmd, error) << "BucketItemBitMap: out of bound in IsMember()";
      return false;
    }
    return ( ((_Mask >> aVal) & 0x1ULL) > 0);
  }
  
  inline void AddSet(std::set<size_t> aSet)
  {
    for (auto it=aSet.begin(); it!=aSet.end();it++)
      AddSet(*it);
  }

  inline void RemoveSet(std::set<size_t> aSet)
  {
    for (auto it=aSet.begin(); it!=aSet.end();it++)
      RemoveSet(*it);
  }
  
  inline size_t GetMembers()
  {
    size_t count = 0;
    for (size_t i=0; i<_NumOfBits; i++)
    {
      if (IsMember(i)) count++;
    }
    return count;
  }
  
  /////////////////////////////////////////////////////
  BitMap(const std::set< BucketItemType> &list)
  :BitMap()
  {
    AddSet(list);
  }
  
  //member functions for BucketItemType
  inline void AddSet(BucketItemType aType) { AddSet((size_t) aType); }
  inline void RemoveSet(BucketItemType aType) { RemoveSet((size_t) aType); }
  
  inline void AddSet(std::set<BucketItemType> aSet)
  {
    for (auto it=aSet.begin(); it!=aSet.end();it++)
      AddSet(*it);
  }

  inline void RemoveSet(std::set<BucketItemType> aSet)
  {
    for (auto it=aSet.begin(); it!=aSet.end();it++)
      RemoveSet(*it);
  }
    
  inline bool IsMember(BucketItemType aType) { return IsMember((size_t) aType); }
  
  inline void GetMembers(std::set< BucketItemType >& i_o_list)
  {
    i_o_list.clear();
    for (size_t i=0; i<_NumOfBits && i < (size_t) csm::daemon::NUM_BUCKET_ITEMS; i++)
    {
      if (IsMember(i)) i_o_list.insert((BucketItemType) i);
    }
  }

};

typedef CoreEventBase<csm::daemon::BitMap> EnvironmentalEvent;

}  // namespace daemon
}  // namespace csm

#endif
