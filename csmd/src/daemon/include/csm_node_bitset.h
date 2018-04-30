/*================================================================================

    csmd/src/daemon/include/csm_node_bitset.h

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
/* csm_node_bitset.h
 *
 ******************************************/

#ifndef CSMD_SRC_DAEMON_INCLUDE_CSM_NODE_BITSET_H_
#define CSMD_SRC_DAEMON_INCLUDE_CSM_NODE_BITSET_H_

#include <bitset>
#include <vector>
#include <string>
#include <algorithm>
#include <functional>

#include <boost/crc.hpp>

#include "csmnet/src/CPP/address.h"

namespace csm {
namespace daemon {

#define BITSET_WIDTH (8192)

typedef std::vector<std::string> NodeNameList_t;

// abstract base nodeSet class to hold number of collisions and entries
class NodeSet
{
protected:
  int _Collisions;
  int _Entries;
public:
  NodeSet()
  : _Collisions(0), _Entries(0)
  {}

  NodeSet( const NodeNameList_t &i_Nodes )
  : _Collisions(0), _Entries( i_Nodes.size() )
  {}

  virtual ~NodeSet()
  {}

  virtual void CreateFromNodeList( const NodeNameList_t &i_Nodes ) = 0;

  inline int GetCollisions() const { return _Collisions; }
  inline int GetEntries() const { return _Entries; }

protected:
  // does a simple hash of the input string
  static uint64_t hashingString( const std::string &i_Name )
  {
    std::hash<std::string> str_hash;
    return (uint64_t)str_hash( i_Name );
  }

  // does a simple hash of the second half of the string only
  static uint64_t hashingString2( const std::string &i_Name )
  {
    std::hash<std::string> str_hash;
    return (uint64_t)str_hash( i_Name.substr( i_Name.length()/2, std::string::npos ) );
//         + (uint64_t)str_hash( i_Name.substr( 0, i_Name.length()/2 ) );
  }

  // generates the CRC64 hash of the string
  static uint64_t hashingCRC64( const std::string &i_Name )
  {
    boost::crc_optimal<64, 0x42F0E1EBA9EA3693, 0, 0, false, false> crc;
    uint64_t *ptr = (uint64_t*)i_Name.c_str();

    crc.process_bytes( ptr, i_Name.length() );
    return crc.checksum();
  }

  // generates the CRC64 hash of the second half of the string
  static uint64_t hashingCRC64_2( const std::string &i_Name )
  {
    boost::crc_optimal<64, 0x42F0E1EBA9EA3693, 0, 0, false, false> crc;
    std::string half = i_Name.substr( i_Name.length()/2, std::string::npos );
    uint16_t *ptr = (uint16_t*)half.c_str();

    crc.process_bytes( ptr, half.length() );
    return crc.checksum();
  }

  // generates the CRC16 of the string
  static uint64_t hashingCRC16( const std::string &i_Name )
  {
    boost::crc_optimal<16, 0x2F15, 0, 0, false, false> crc;
    uint16_t *ptr = (uint16_t*)i_Name.c_str();

    crc.process_bytes( ptr, i_Name.length() );
    return crc.checksum();
  }

  // generates the CRC16 of the second half of the string
  static uint64_t hashingCRC16_2( const std::string &i_Name )
  {
    boost::crc_optimal<16, 0x2F15, 0, 0, false, false> crc;
    std::string half = i_Name.substr( i_Name.length()/2, std::string::npos );
    uint16_t *ptr = (uint16_t*)half.c_str();

    crc.process_bytes( ptr, half.length() );
    return crc.checksum();
  }

  // creates a hash by simple shift and add/xor operation
  static uint64_t hashingPlusShift( const std::string &i_Name )
  {
    uint64_t sum = 0;
    for( uint64_t n=0; n<i_Name.length(); ++n )
    {
      sum = (sum << 2);
      sum += (uint64_t)i_Name[n];
    }

    return sum;
  }

  // creates a hash by simple shift and add/xor operation of the second half of the strirg
  static uint64_t hashingPlusShift2( const std::string &i_Name )
  {
    uint64_t sum = 0;
    for( uint64_t n=i_Name.length()/2; n<i_Name.length(); ++n )
    {
      sum = (sum << 2);
      sum ^= (uint64_t)i_Name[n];
    }

    return sum;
  }

  // interprets the last N chars as a number (very specific, only for testing after adjusting the hostnames)
  static uint64_t hashingTopology( const std::string &i_Name )
  {
    std::string nm = i_Name.substr( 7, std::string::npos );

    return atoll( nm.c_str() );
  }

  // extracts all numerical chars from the string and creates a base11 number of them
  static uint64_t hashingNumber( const std::string &i_Name )
  {
    uint64_t sum = 0;
    for( auto c: i_Name )
      if(( c >='0' ) && ( c<='9' ))
        sum = sum * 11 + ( c - '0' );

//    std::cout << "[" << i_Name << ":" << sum << "]";
    return sum;
  }

};

// node bitset that uses a single hash function
class NodeBitSet : public NodeSet
{
  std::bitset<BITSET_WIDTH> _Bits;
  uint64_t (*hashfn)(const std::string &);

public:
  NodeBitSet()
  : NodeSet()
  {
    SetHashFunctions();
  }

  NodeBitSet( const NodeNameList_t &i_Nodes )
  : NodeSet( i_Nodes )
  {
    SetHashFunctions();
    CreateFromNodeList( i_Nodes );
  }

  void SetHashFunctions()
  {
    hashfn = hashingNumber;
  }


  virtual ~NodeBitSet()
  {}

  inline unsigned Count() const
  {
    return _Bits.count();
  }

  inline unsigned Size() const
  {
    return _Bits.size();
  }

  bool MatchSet( const NodeBitSet &i_B ) const
  {
    NodeBitSet r = *this;
    r &= i_B;
    return r.Count() > 0;
  }

  unsigned MatchSet( const NodeNameList_t &i_NodeNames ) const
  {
    for( auto it : i_NodeNames )
    {
      unsigned idx = hashfn( it ) % BITSET_WIDTH;
      if( _Bits.test( idx  ) )
        return true;
    }
    return false;
  }

  NodeBitSet& operator&=( const NodeBitSet &i_B )
  {
    _Bits &= i_B._Bits;
    return *this;
  }

  NodeBitSet& operator=( const NodeBitSet &i_B )
  {
    _Bits = i_B._Bits;
    hashfn = i_B.hashfn;
    _Collisions = i_B._Collisions;
    _Entries = i_B._Entries;

    return *this;
  }

  virtual void CreateFromNodeList( const NodeNameList_t &i_Nodes )
  {
    _Entries = i_Nodes.size();
    _Collisions = 0;
    _Bits.reset();
    for( auto it : i_Nodes )
    {
      uint64_t idx = hashfn( it ) % BITSET_WIDTH;
//      std::cout << "; " << it->Dump() << ":" << idx;
      if( _Bits.test( idx ) )
        ++_Collisions;
      _Bits.set( idx );
    }
//    std::cout << std::endl;
  }

};


// bloom-filter bit set with multiple hash functions
// the number of hash functions used in the bloom filter
#define BF_K (2)

class NodeSetBloomFilter : public NodeSet
{
  std::bitset<BITSET_WIDTH> _Bits[BF_K];
  uint64_t (*hashfn[BF_K])(const std::string &);

public:
  NodeSetBloomFilter()
  : NodeSet()
  {
    SetHashFunctions();
  }

  NodeSetBloomFilter( const NodeNameList_t &i_Nodes )
  : NodeSet( i_Nodes )
  {
    SetHashFunctions();
    CreateFromNodeList( i_Nodes );
  }

  void SetHashFunctions()
  {
    hashfn[0] = hashingCRC64_2;
    hashfn[1] = hashingNumber;
  }

  virtual ~NodeSetBloomFilter()
  {}

  // counts the set bits
  // note that a bit only counts a set if it's set in all sets
  inline unsigned Count() const
  {
    size_t count = BITSET_WIDTH;
    for( int n=BF_K-1; n>=0; --n )
      count = std::min( count, _Bits[n].count() );
    return (unsigned)count;
  }

  virtual void CreateFromNodeList( const NodeNameList_t &i_Nodes )
  {
    _Entries = i_Nodes.size();
    _Collisions = 0;
    for( int k=BF_K-1; k>=0; --k )
      _Bits[k].reset();
    for( auto it : i_Nodes )
    {
//      std::cout << "; " << it->Dump();
      bool collide = true;
      uint64_t idx = 0;
      for( int k=BF_K-1; k>=0; --k )
      {
        idx = hashfn[k]( it ) % BITSET_WIDTH;
        collide &= ( _Bits[k].test( idx ) );
        _Bits[k].set( idx );
//        std::cout << ":" << idx;
      }
      if( collide )
        ++_Collisions;
    }
//    std::cout << std::endl;
  }

  bool MatchSet( const NodeSetBloomFilter &i_B ) const
  {
    NodeSetBloomFilter r = *this;
    r &= i_B;
    return (r.Count() > 0);
  }

  unsigned MatchSet( const NodeNameList_t &i_NodeNames ) const
  {
    for( auto it : i_NodeNames )
    {
      bool match = true;
      uint64_t idx = 0;
      for( int k=BF_K-1; (k>=0)&&(match); --k )
//      for( int k=BF_K-1; (k>=0); --k )
      {
        idx = hashfn[k]( it ) % BITSET_WIDTH;
        match &= ( _Bits[k].test( idx  ) );
      }

      if( match )
        return true;
    }
    return false;
  }

  NodeSetBloomFilter& operator&=( const NodeSetBloomFilter &i_B )
  {
    for( int k=BF_K-1; k>=0; --k )
      _Bits[k] &= i_B._Bits[k];
    return *this;
  }

  NodeSetBloomFilter& operator=( const NodeSetBloomFilter &i_B )
  {
    for( int k=BF_K-1; k>=0; --k )
    {
      _Bits[k] = i_B._Bits[k];
      hashfn[k] = i_B.hashfn[k];
    }
    _Collisions = i_B._Collisions;
    _Entries = i_B._Entries;

    return *this;
  }

private:
};

/////////////////////////////////////////////////////////////////////////////////////////////

// bit filter that tries to use select and use the best 2 hash functions to work with
// idea was to make each aggregator decide which hash functions cause the fewest collisions
// didn't work as expected for apparent reason: what counts are the global collisions, not the local ones

#define BF_KM (5)

class NodeSetMultiFilter : public NodeSet
{
  std::bitset<BITSET_WIDTH> _Bits[BF_KM];
  uint64_t (*hashfn[BF_KM])(const std::string &);
  unsigned _fnA;
  unsigned _fnB;


public:
  NodeSetMultiFilter()
  : NodeSet()
  {
    SetHashFunctions();
    _fnA = 0;
    _fnB = 1;
  }

  NodeSetMultiFilter( const NodeNameList_t &i_Nodes )
  : NodeSet( i_Nodes )
  {
    SetHashFunctions();

    _fnA = BF_KM-1;
    _fnB = BF_KM-1;

    CreateFromNodeList( i_Nodes );
  }

  void SetHashFunctions()
  {
    hashfn[0] = hashingString;
    hashfn[1] = hashingCRC64;
    hashfn[2] = hashingCRC16;
    hashfn[3] = hashingPlusShift2;
    hashfn[4] = hashingTopology;
  }

  virtual ~NodeSetMultiFilter()
  {}

  inline unsigned Count() const
  {
    return std::min( _Bits[ _fnA ].count(), _Bits[ _fnB ].count() );
  }

  virtual void CreateFromNodeList( const NodeNameList_t &i_Nodes )
  {
    unsigned collisions[BF_KM];
    for( int n=BF_KM-1; n>=0; --n )
      collisions[n] = 0;

    _Entries = i_Nodes.size();
    _Collisions = 0;
    for( int k=BF_KM-1; k>=0; --k )
      _Bits[k].reset();
    for( auto it : i_Nodes )
    {
      bool collide = true;
      uint64_t idx = 0;
      for( int k=BF_KM-1; k>=0; --k )
      {
        idx = hashfn[k]( it ) % BITSET_WIDTH;
        collide &= ( _Bits[k].test( idx ) );
        if( _Bits[k].test( idx ) )
          ++(collisions[k]);
        _Bits[k].set( idx );
      }
      if( collide )
        ++_Collisions;
    }
    for( int k=BF_KM-1; k>=0; --k )
    {
//      std::cout << "Fnkt[" << k << "] with " << collisions[k] << "collisions" << std::endl;
      if( collisions[k] < collisions[ _fnA ] )
      {
        _fnB = _fnA;
        _fnA = k;
      }
    }
    for( int k=BF_KM-1; (k>=0); --k )
    {
      if( (unsigned)k == _fnA )
        continue;
      if( collisions[k] <= collisions[ _fnB ] )
        _fnB = k;
    }
//    std::cout << "Found: _fnA=" << _fnA << " (" << collisions[_fnA] << ") and _fnB=" << _fnB << " (" << collisions[_fnB] << ")" << std::endl;
    _Collisions = collisions[_fnA] + collisions[_fnB];
  }

  bool MatchSet( const NodeSetMultiFilter &i_B ) const
  {
    NodeSetMultiFilter r = *this;
    r &= i_B;
    return (r.Count() > 0);
  }

  unsigned MatchSet( const NodeNameList_t &i_NodeNames ) const
  {
    for( auto it : i_NodeNames )
    {
      bool match = true;
      uint64_t idx = 0;
      idx = hashfn[_fnA]( it ) % BITSET_WIDTH;
      match &= ( _Bits[_fnA].test( idx  ) );
      idx = hashfn[_fnB]( it ) % BITSET_WIDTH;
      match &= ( _Bits[_fnB].test( idx  ) );

      if( match )
        return true;
    }
    return false;
  }

  NodeSetMultiFilter& operator&=( const NodeSetMultiFilter &i_B )
  {
    for( int k=BF_KM-1; k>=0; --k )
      _Bits[k] &= i_B._Bits[k];
    return *this;
  }

  NodeSetMultiFilter& operator=( const NodeSetMultiFilter &i_B )
  {
    for( int k=BF_KM-1; k>=0; --k )
    {
      _Bits[k] = i_B._Bits[k];
      hashfn[k] = i_B.hashfn[k];
    }
    _Collisions = i_B._Collisions;
    _Entries = i_B._Entries;
    _fnA = i_B._fnA;
    _fnB = i_B._fnB;

    return *this;
  }

private:
};



}  // namespace daemon
} // namespace csm

#endif /* CSMD_SRC_DAEMON_INCLUDE_CSM_NODE_BITSET_H_ */
