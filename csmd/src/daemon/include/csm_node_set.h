/*================================================================================

    csmd/src/daemon/include/csm_node_set.h

  © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#ifndef CSMD_SRC_DAEMON_INCLUDE_CSM_NODE_SET_H_
#define CSMD_SRC_DAEMON_INCLUDE_CSM_NODE_SET_H_

#ifndef logprefix
#define logprefix "NODESET"
#define cleanlogprefix
#endif

#include <mutex>
#include <string>
#include <unordered_set>
#include <map>
#include <deque>
#include <algorithm>

#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/split_member.hpp>

#include "csm_pretty_log.h"
#include "csmnet/src/CPP/address.h"
#include "include/run_mode.h"

namespace csm {
namespace daemon {

typedef std::vector<std::string> ComputeNodeList_t;

class ComputeSet
{
public:
  enum SetCommitType {
    SET_COMMIT_NONE = 0,
    SET_COMMIT_INSERTED = 1,
    SET_COMMIT_DELETED = 2,
    SET_COMMIT_BOTH = 3
  };
private:
  ComputeNodeList_t _addrs;   // committed list of addresses
  ComputeNodeList_t _inserted;  // any uncommitted/newly added addresses
  ComputeNodeList_t _deleted;  // any uncommitted/removed addresses
  bool _active;  // determines whether this set of nodes is active or not
  mutable std::mutex _lock;

public:
  ComputeSet()
  : _addrs(), _inserted(), _deleted(), _active( false )
  {}

  ComputeSet( const ComputeNodeList_t &nodes )
  : _inserted(), _deleted(), _active( false ), _lock()
  {
    SetAddrList( nodes );
  }

  ComputeSet( const ComputeSet &in )
  : _addrs( in._addrs ),
    _inserted( in._inserted ), _deleted( in._deleted ),
    _active( in._active ),
    _lock()
  {
    if( ! std::is_sorted( _addrs.begin(), _addrs.end() ) )
      std::sort( _addrs.begin(), _addrs.end() );

    if(( ! _inserted.empty() ) && ( ! std::is_sorted( _inserted.begin(), _inserted.end() )) )
      std::sort( _inserted.begin(), _inserted.end() );

    if(( ! _deleted.empty() ) && ( ! std::is_sorted( _deleted.begin(), _deleted.end() )) )
      std::sort( _deleted.begin(), _deleted.end() );
  }

  ComputeSet& operator=( const ComputeSet &in )
  {
    std::lock_guard<std::mutex> guard( _lock );

    _addrs = in._addrs;
    _inserted = in._inserted;
    _deleted = in._deleted;
    _active = in._active;

    if( ! std::is_sorted( _addrs.begin(), _addrs.end() ) )
      std::sort( _addrs.begin(), _addrs.end() );

    if(( ! _inserted.empty() ) && ( ! std::is_sorted( _inserted.begin(), _inserted.end() )) )
      std::sort( _inserted.begin(), _inserted.end() );

    if(( ! _deleted.empty() ) && ( ! std::is_sorted( _deleted.begin(), _deleted.end() )) )
      std::sort( _deleted.begin(), _deleted.end() );

    return *this;
  }


  ~ComputeSet()
  {
    Clear();
  }

  bool HasNode( const std::string lookup ) const
  {
    std::lock_guard<std::mutex> guard( _lock );
    bool ret = std::binary_search( _addrs.begin(), _addrs.end(), lookup );

    if( ret )
    {
      // if it was in the addr list, check if it might be an uncommitted delete
      bool deleted = ( ! _deleted.empty() ) && (std::binary_search( _deleted.begin(), _deleted.end(), lookup ));
      ret &= ( ! deleted );
    }
    else
    {
      // if it was not in the addr list, check if it might be an uncommitted insert
      ret |= ( ( ! _inserted.empty() ) && std::binary_search( _inserted.begin(), _inserted.end(), lookup ) );
    }

    return ret;
  }

  void SetAddrList( const ComputeNodeList_t &addrs )
  {
    std::lock_guard<std::mutex> guard( _lock );

    // assemble the addr list and sort by key
    _addrs = addrs;
    std::sort( _addrs.begin(), _addrs.end() );

    _inserted.clear();
    _deleted.clear();
  }

  ComputeNodeList_t GetAddrList() const
  {
    return _addrs;
  }

  ComputeNodeList_t GetInsertList() const
  {
    return _inserted;
  }

  ComputeNodeList_t GetDeleteList() const
  {
    return _deleted;
  }

  ComputeNodeList_t GenerateAddrList()
  {
    Commit();
    return _addrs;
  }

  ComputeNodeList_t GenerateInsertList() const
  {
    return _inserted;
  }

  ComputeNodeList_t GenerateDeleteList() const
  {
    return _deleted;
  }

  size_t GetSize() const
  {
    return _addrs.size() + _inserted.size() - _deleted.size();
  }

  size_t GetUncommittedInsert() const { return _inserted.size(); }
  size_t GetUncommittedDelete() const { return _deleted.size(); }

  void SetActive( const bool active ) { _active = active; }
  bool GetActive( ) const { return _active; }

  bool empty() const
  { return ( GetSize() == 0 ); }

  void Clear()
  {
    std::lock_guard<std::mutex> guard( _lock );
    _addrs.clear();
    _inserted.clear();
    _deleted.clear();
  }

  // adding a node checks if the node might be in the uncommitted deleted list
  // if it's there, then remove it
  // finally append it to the uncommitted inserted list
  bool AddNode( const std::string node )
  {
    std::lock_guard<std::mutex> guard( _lock );

    if( node.empty() ) return false;
    bool had_uncommitted_delete = false;

    // prevent double insertion
    if( std::binary_search( _inserted.begin(), _inserted.end(), node ))
      return false;

    // if it's listed in _delete set, then remove it from there
    for( ComputeNodeList_t::iterator del = _deleted.begin();
        del != _deleted.end();
        ++del )
    {
      if( *del == node )
      {
        _deleted.erase( del );
        had_uncommitted_delete = true;
        break;
      }
    }

    // don't attempt to insert if it's already there
    // search based on the address key because the pointer might be different
    if( ! std::binary_search( _addrs.begin(), _addrs.end(), node ) )
    {
      _inserted.push_back( node );
      std::sort( _inserted.begin(), _inserted.end() );
      CSMLOG( csmd, debug ) << "Inserting node " << node << " to compute set";
      return true;
    }
    return had_uncommitted_delete;
  }

  // deleting a node checks if the node might be in the uncommitted insertion list
  // if it's in there, then remove it
  // finally append it to the uncommitted deletion list
  bool DelNode( const std::string node )
  {
    std::lock_guard<std::mutex> guard( _lock );

    if( node.empty() ) return false;
    if( _addrs.empty() && _inserted.empty() ) return false;

    bool had_uncommitted_insert = false;

    // prevent double deletion
    if( std::binary_search( _deleted.begin(), _deleted.end(), node ) )
      return false;

    // check if it's in the inserted list
    for( ComputeNodeList_t::iterator ins = _inserted.begin();
        ins != _inserted.end();
        ++ins )
    {
      if( *ins == node )
      {
        _inserted.erase( ins );
        had_uncommitted_insert = true;
        CSMLOG( csmd, debug ) << "Deleting node " << node << " from compute set";
        break;
      }
    }

    // before adding it to the delete set, make sure the item actually exists
    if( std::binary_search( _addrs.begin(), _addrs.end(), node ) )
    {
      _deleted.push_back( node );
      std::sort( _deleted.begin(), _deleted.end() );
      return true;
    }

    return had_uncommitted_insert;
  }

  void Commit( const SetCommitType commitMask = SET_COMMIT_BOTH )
  {
    std::lock_guard<std::mutex> guard( _lock );
    size_t delsize = _deleted.size();
    size_t inssize = _inserted.size();

    if(( inssize > 0 ) && ( commitMask & SET_COMMIT_INSERTED ))
    {
      _addrs = UnionNodes( _inserted );
      _inserted.clear();
    }

    if(( delsize > 0 ) && ( commitMask & SET_COMMIT_DELETED ))
    {
      _addrs = DifferenceNodes( _deleted );
      _deleted.clear();
    }

    CSMLOG( csmd, debug ) << " Committed changes(mask:" << commitMask << "): insertions=" << inssize
        << " deletions=" << delsize << " totalsize=" << GetSize() << " (" << _inserted.size() << ":" << _deleted.size() << ")";
  }

  void InterSect( const ComputeSet &in )
  {
    Commit();
    std::lock_guard<std::mutex> guard( _lock );
    _addrs = InterSectNodes( in.GetAddrList() );
  }

  void Union( const ComputeSet &in )
  {
    Commit();
    std::lock_guard<std::mutex> guard( _lock );
    _addrs = UnionNodes( in.GetAddrList() );
  }

  void Difference( const ComputeSet &in )
  {
    Commit();
    std::lock_guard<std::mutex> guard( _lock );
    _addrs = DifferenceNodes( in.GetAddrList() );
  }

  // address list generator functions - will create a new addrKeyList_t
  ComputeNodeList_t InterSectNodes( const ComputeNodeList_t &in ) const
  {
    ComputeNodeList_t isct( std::min( _addrs.size(), in.size() ) );
    ComputeNodeList_t::iterator it;
    it = std::set_intersection( _addrs.begin(), _addrs.end(),
                                in.begin(), in.end(),
                                isct.begin() );
    isct.resize( it - isct.begin() );
    return isct;
  }

  ComputeNodeList_t UnionNodes( const ComputeNodeList_t &in ) const
  {
    ComputeNodeList_t un( in.size() + _addrs.size() );
    ComputeNodeList_t::iterator it;
    it = std::set_union( _addrs.begin(), _addrs.end(),
                    in.begin(), in.end(),
                    un.begin() );
    un.resize( it - un.begin() );
    return un;
  }

  ComputeNodeList_t DifferenceNodes( const ComputeNodeList_t &in ) const
  {
    ComputeNodeList_t diff( _addrs.size() );
    ComputeNodeList_t::iterator it;
    it = std::set_difference( _addrs.begin(), _addrs.end(),
                         in.begin(), in.end(),
                         diff.begin() );
    diff.resize( it - diff.begin() );
    return diff;
  }

  static std::string ConvertDiffToBytes( const ComputeSet &cs )
  {
    std::stringstream ss;
    boost::archive::text_oarchive oa(ss);
    oa << cs;
    return ss.str();
  }

  static void ConvertDiffToClass(const std::string &i_payload, ComputeSet &cs )
  {
    std::stringstream ss;
    ss << i_payload;
    boost::archive::text_iarchive ia(ss);
    ia >> cs;
  }

  std::string Dump() const
  {
    std::lock_guard<std::mutex> guard( _lock );
    std::string ret = "N:";
    for( auto it : _addrs ) { ret.append( it ); ret.append(";"); }
    ret.append("I:");
    for( auto it : _inserted ) { ret.append( it ); ret.append(";"); }
    ret.append("D:");
    for( auto it : _deleted ) { ret.append( it ); ret.append(";"); }
    return ret;
  }

private:
  friend class boost::serialization::access;

  template <class Archive>
  void serialize(Archive &ar, const unsigned int version)
  {
    std::lock_guard<std::mutex> guard( _lock );
    ar & _inserted;
    ar & _deleted;
  }

};

typedef enum
{
  COMPUTE_DOWN = 0,
  COMPUTE_UP = 1,
  COMPUTE_FULL_REDUNDANCY = 2,
  COMPUTE_LOST_REDUNDANCY = 3,
  COMPUTE_LOST_CONNECTION = 4,
  COMPUTE_ACTION_UNDEF = 5
} ComputeAction_t;


typedef std::pair<const csm::network::AddressCode, const ComputeSet> ComputeSetEntry;
typedef std::map<const csm::network::AddressCode, ComputeSet> AggregatorNodeSet_t;
typedef std::pair<std::string, ComputeAction_t> ComputeActionEntry_t;
typedef std::deque<ComputeActionEntry_t> ComputeActionList_t;

class ComputeReferenceCounter_t
{
public:
  std::map<const std::string, uint8_t> _refs;
  ComputeActionList_t _events;

  size_t Up( const ComputeNodeList_t &in )
  {
    for( auto it : in )
    {
      if( _refs.find( it ) == _refs.end() )
        _refs[ it ] = 0;

      if( _refs[ it ] < 2 )
        _refs.at( it )++;

      // now, if the refcount bumped up from 0 to 1, we add it to the "RAS-up" candidate list
      if( _refs[ it ] == 1 )
        _events.push_back( ComputeActionEntry_t( it, COMPUTE_UP ) );
      if( _refs[ it ] == 2 )
        _events.push_back( ComputeActionEntry_t( it, COMPUTE_FULL_REDUNDANCY ) );

      CSMLOG( csmd, debug ) << "UP: counting " << it << " refs=" << std::to_string( _refs[ it ] );
    }
    return _events.size();
  }
  size_t Down( const ComputeNodeList_t &in, const bool down )
  {
    for( auto it : in )
    {
      if( _refs[ it ] > 0 )
        _refs.at( it )--;

      if( _refs[ it ] == 0 )
        _events.push_back( ComputeActionEntry_t( it,
                                                 (down ? COMPUTE_DOWN : COMPUTE_LOST_CONNECTION ) ) );
      if( _refs[ it ] == 1 )
        _events.push_back( ComputeActionEntry_t( it, COMPUTE_LOST_REDUNDANCY ) );

      CSMLOG( csmd, debug ) << "DOWN: counting " << it << " refs=" << std::to_string( _refs[ it ] );
    }
    return _events.size();
  }
  uint8_t operator[]( const std::string &node ) const
  {
    try
    {
      return _refs.at( node );
    }
    catch( ... )
    {
      return 0;
    }
  }
  ComputeActionEntry_t GetNextEvent()
  {
    if( _events.empty() )
      return ComputeActionEntry_t( "", COMPUTE_ACTION_UNDEF );

    ComputeActionEntry_t ret = _events.front();
    _events.pop_front();
    return ret;
  }
};


// handling of a set of aggregator info
// search for node ranges and subsets of nodes
// implements foreach functionality for a set of aggregators
class AggregatorSet
{
  ComputeReferenceCounter_t _refcount;
  AggregatorNodeSet_t _aggrs;
  mutable std::mutex _lock;

public:
  AggregatorSet()
  : _aggrs()
  {}

  AggregatorSet( const AggregatorSet &in )
  : _aggrs( in._aggrs )
  {}

  ~AggregatorSet()
  {
    _aggrs.clear();
  }

public:
  int Add( const csm::network::Address_sptr addr,
           const ComputeSet &in )
  {
    if( addr == nullptr )
      return 0;

    std::lock_guard<std::mutex> guard( _lock );
    _aggrs[ addr->MakeKey() ] = in;
    _aggrs[ addr->MakeKey() ].Commit();
    return 0;
  }
  int Del( const csm::network::Address_sptr addr )
  {
    if( addr == nullptr )
      return 0;

    std::lock_guard<std::mutex> guard( _lock );
    AggregatorNodeSet_t::iterator loc = _aggrs.find( addr->MakeKey() );
    if( loc != _aggrs.end() )
      _aggrs.erase( loc );
    else
      return -ENOENT;
    return 0;
  }
  int Replace( const csm::network::Address_sptr addr,
               const ComputeSet &in )
  {
    return Add( addr, in ); // current impl of Add replaces the set anyway
  }

  int Update( const csm::network::Address_sptr addr,
              const ComputeNodeList_t &ins, const ComputeNodeList_t &del )
  {
    if( addr == nullptr ) return -EINVAL;

    std::lock_guard<std::mutex> guard( _lock );
    CSMLOG( csmd, debug ) << "Updating AGG: " << addr->Dump()  << " i:" << ins.size() << " d:" << del.size();
    if( ! ins.empty() )
      _refcount.Up( ins );
    if( ! del.empty() )
      _refcount.Down( del, true );

    _aggrs[ addr->MakeKey() ].Union( ins );
    _aggrs[ addr->MakeKey() ].Difference( del );
    CSMLOG( csmd, info ) << "Nodes of "  << addr->Dump()  << ": " << _aggrs[ addr->MakeKey() ].GetSize();
    CSMLOG( csmd, trace ) << "Nodes of "  << addr->Dump()  << ": " << _aggrs[ addr->MakeKey() ].Dump();
    return 0;
  }

  void Disconnect( const csm::network::Address_sptr addr )
  {
    if( addr == nullptr ) return;
    std::lock_guard<std::mutex> guard( _lock );
    _aggrs[ addr->MakeKey() ].SetActive( false );
    _refcount.Down( _aggrs[ addr->MakeKey() ].GetAddrList(), false );
  }
  void Connect( const csm::network::Address_sptr addr )
  {
    if( addr == nullptr ) return;
    std::lock_guard<std::mutex> guard( _lock );
    _aggrs[ addr->MakeKey() ].SetActive( true );
    _refcount.Up( _aggrs[ addr->MakeKey() ].GetAddrList() );
  }

  bool IsNodeConnected( const std::string &node ) const
  {
    std::lock_guard<std::mutex> guard( _lock );
    uint8_t refcount = _refcount[ node ];
    bool connected = ( refcount > 0 );
    for( auto it : _aggrs )
    {
      // only if that aggregator is active
      if( ! it.second.GetActive() )
        continue;
      if( it.second.HasNode( node ) )
      {
        if( connected )
          return true;
        else
          throw csm::daemon::Exception("BUG: mismatch between CN Refcount " + std::to_string(refcount) + " and ComputeSet" );
      }
    }
    if( refcount == 0 )
      return false;
    else
      throw csm::daemon::Exception("BUG: mismatch between CN Refcount " + std::to_string(refcount) + " and ComputeSet" );
  }

  ComputeNodeList_t GetAggrDisconnectedNodes( const csm::network::Address_sptr downAggrAddr ) const
  {
    if( downAggrAddr == nullptr )
      return ComputeNodeList_t();
    csm::network::AddressCode downAggrAddrKey = downAggrAddr->MakeKey();

    std::lock_guard<std::mutex> guard( _lock );
    ComputeSet aggNodes = _aggrs.at( downAggrAddrKey );

    // remove all nodes from this aggregator's set that are covered by other aggregators
    for( auto it : _aggrs )
    {
      // skip the aggregator with the given address
      if(( ! it.second.GetActive() ) || ( downAggrAddrKey == it.first ))
        continue;

      aggNodes.Difference( it.second );
    }

    return aggNodes.GenerateAddrList();
  }

  inline ComputeActionEntry_t GetNextEvent()
  {
    return _refcount.GetNextEvent();
  }

};

}   // daemon
}  // csm

#ifdef cleanlogprefix
#undef logprefix
#undef cleanlogprefix
#endif

#endif /* CSMD_SRC_DAEMON_INCLUDE_CSM_NODE_SET_H_ */
