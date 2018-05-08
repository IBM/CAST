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

enum ComputeNodeAction {
  NODE_ACTION_DOWN = 0,
  NODE_ACTION_UP = 1,
  NODE_ACTION_UNDEFINED = 2
};

typedef struct ComputeSetData
{
  std::string _name;
  uint64_t _connseq;
  ComputeNodeAction _action;

  ComputeSetData( const std::string name = "",
                  const uint64_t seq = 0,
                  const ComputeNodeAction a = NODE_ACTION_UNDEFINED )
  : _name( name ), _connseq( seq ), _action( a )
  {}


  template <class Archive>
  void serialize(Archive &ar, const unsigned int version)
  {
    ar & _name;
    ar & _connseq;
    ar & _action;
  }

} ComputeSetData_t;

static bool CompareComputeSetData( const ComputeSetData_t &a, const ComputeSetData_t &b )
{
  return ( a._name < b._name ) || (( a._name == b._name) && (a._connseq < b._connseq ));
}

typedef std::vector<ComputeSetData_t> ComputeSetUpdates_t;

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
  ComputeSetUpdates_t _updates; // changes to the existing set
  bool _active;  // determines whether this set of nodes is active or not
  mutable std::mutex _lock;

public:
  ComputeSet()
  : _addrs(), _updates(),_active( false )
  {}

  ComputeSet( const ComputeNodeList_t &nodes )
  : _updates(), _active( false ), _lock()
  {
    SetAddrList( nodes );
  }

  ComputeSet( const ComputeSet &in )
  : _addrs( in._addrs ),
    _updates( in._updates ),
    _active( in._active ),
    _lock()
  {
    if( ! std::is_sorted( _addrs.begin(), _addrs.end() ) )
      std::sort( _addrs.begin(), _addrs.end() );

    if(( ! _updates.empty() ) && ( ! std::is_sorted( _updates.begin(), _updates.end(), CompareComputeSetData )) )
      std::sort( _updates.begin(), _updates.end(), CompareComputeSetData );
  }

  ComputeSet& operator=( const ComputeSet &in )
  {
    std::lock_guard<std::mutex> guard( _lock );

    _addrs = in._addrs;
    _updates = in._updates;
    _active = in._active;

    if( ! std::is_sorted( _addrs.begin(), _addrs.end() ) )
      std::sort( _addrs.begin(), _addrs.end() );

    if(( ! _updates.empty() ) && ( ! std::is_sorted( _updates.begin(), _updates.end(), CompareComputeSetData )) )
      std::sort( _updates.begin(), _updates.end(), CompareComputeSetData );

    return *this;
  }


  ~ComputeSet()
  {
    Clear();
  }

  bool HasNode( const std::string lookup ) const
  {
    std::lock_guard<std::mutex> guard( _lock );

    // check if the node appears in the update queue as an insert or delete
    // start search from the back, we're only interested in the last action
    for( auto it = _updates.crbegin(); it != _updates.crend(); ++it )
    {
      // stop the lookup if we're past the point (not part of the updates)
      if( it->_name < lookup )
        break;
      // found the entry, return what the last action was
      if( it->_name == lookup )
        return ( it->_action == NODE_ACTION_UP );
    }
    // if node is not in updates, do search and return what's in the addr list
    return std::binary_search( _addrs.begin(), _addrs.end(), lookup );
  }

  void SetAddrList( const ComputeNodeList_t &addrs )
  {
    std::lock_guard<std::mutex> guard( _lock );

    // assemble the addr list and sort by key
    _addrs = addrs;
    std::sort( _addrs.begin(), _addrs.end() );

    _updates.clear();
  }

  ComputeNodeList_t GetAddrList() const
  {
    return _addrs;
  }

  ComputeSetUpdates_t GetUpdateList() const
  {
    return _updates;
  }

  ComputeNodeList_t GenerateAddrList()
  {
    Commit();
    return _addrs;
  }

  size_t GetSize() const
  {
    size_t n = _addrs.size();
    for( auto it : _updates )
      if( it._action == NODE_ACTION_UP ) ++n;
      else --n;

    return n;
  }

  size_t GetUncommittedUpdates() const { return _updates.size(); }

  void SetActive( const bool active ) { _active = active; }
  bool GetActive( ) const { return _active; }

  bool empty() const
  { return ( _addrs.empty() & _updates.empty() ); }

  void Clear()
  {
    std::lock_guard<std::mutex> guard( _lock );
    _addrs.clear();
    _updates.clear();
  }

  bool AddNode( const std::string node, const uint64_t sequence = 0 )
  {
    std::lock_guard<std::mutex> guard( _lock );

    if( node.empty() ) return false;
    bool had_uncommitted_delete = false;

    // prevent double insertion
    for( auto it = _updates.crbegin(); it != _updates.crend(); ++it )
    {
      if( it->_name < node ) break;
      if(( it->_name == node ) && ( it->_connseq <= sequence ))
      {
        CSMLOG( csmd, debug ) << "AddNode: Found node " << node << " last activity: "
            << ( it->_action == NODE_ACTION_UP ? "ADD" : "DEL");
        if( it->_action == NODE_ACTION_UP )
          return false;
        else
        {
          had_uncommitted_delete = true;
          break; // if last action was not UP, then we can break here and update the status
        }
      }
    }

    // don't attempt to insert if it's already there
    if(( had_uncommitted_delete ) || ( ! std::binary_search( _addrs.begin(), _addrs.end(), node ) ))
    {
      _updates.push_back( ComputeSetData_t(node, sequence, NODE_ACTION_UP ) );
      CSMLOG( csmd, debug ) << "Inserting node " << node << " to compute set";
      return true;
    }
    return had_uncommitted_delete;
  }

  bool DelNode( const std::string node, const uint64_t sequence = 0 )
  {
    std::lock_guard<std::mutex> guard( _lock );

    if( node.empty() ) return false;
    if( _addrs.empty() && _updates.empty() ) return false;

    bool had_uncommitted_insert = false;

    // prevent double insertion
    for( auto it = _updates.crbegin(); it != _updates.crend(); ++it )
    {
      if( it->_name < node ) break;
      if(( it->_name == node ) && ( it->_connseq <= sequence ))
      {
        CSMLOG( csmd, debug ) << "DelNode: Found node " << node << " last activity: "
            << ( it->_action == NODE_ACTION_UP ? "ADD" : "DEL");
        if( it->_action == NODE_ACTION_DOWN )
          return false;
        else
        {
          had_uncommitted_insert = true;
          break; // if last action was not DOWN, then we can break here and update the status
        }
      }

    }

    // only record the deletion if it either has been inserted or is available in the committed list
    if(( had_uncommitted_insert) || ( std::binary_search( _addrs.begin(), _addrs.end(), node ) ))
    {
      _updates.push_back( ComputeSetData_t(node, sequence, NODE_ACTION_DOWN ) );
      CSMLOG( csmd, debug ) << "Deleting node " << node << " from compute set";
      return true;
    }
    return had_uncommitted_insert;
  }

  void Commit( const SetCommitType commitMask = SET_COMMIT_BOTH )
  {
    std::lock_guard<std::mutex> guard( _lock );
    if( _updates.empty() ) return;
    size_t updsize = _updates.size();
    std::map<std::string, int> node_map;
    ComputeNodeList_t to_delete;

    for( auto it : _updates )
    {
      if( it._action == NODE_ACTION_UP ) node_map[ it._name ] = 1;
      if( it._action == NODE_ACTION_DOWN ) node_map[ it._name ] = -1;
    }

    for( auto it : node_map )
    {
      if( it.second == 0 ) continue;
      if(( it.second > 0 ) && ( commitMask & SET_COMMIT_INSERTED ))
        _addrs.push_back( it.first );
      if(( it.second < 0 ) && ( commitMask & SET_COMMIT_DELETED ))
        to_delete.push_back( it.first ); // assemble deletion list to perform set-differenc
    }

    // sort the updated node list
    if( ! std::is_sorted( _addrs.begin(), _addrs.end() ) )
      std::sort( _addrs.begin(), _addrs.end() );

    // perform the difference op to remove deletede nodes
    if(( ! to_delete.empty() ) && ( commitMask & SET_COMMIT_DELETED ))
    {
      if( ! std::is_sorted( to_delete.begin(), to_delete.end() ) )
        std::sort( to_delete.begin(), to_delete.end() );
      _addrs = DifferenceNodes( to_delete );
    }

    CSMLOG( csmd, debug ) << " Committed changes(mask:" << commitMask << "): updates=" << updsize
        << " totalsize=" << GetSize();

    _updates.clear();

  }

  void Update( const ComputeSetUpdates_t &upd )
  {
    if( _updates.empty() ) _updates = upd;
    else
    {
      for( auto it : upd )
        _updates.push_back( it );
    }
    Commit();
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
    ret.append("U:");
    for( auto it : _updates )
    {
      ret.append( it._name );
      ret.append(".");
      ret.append( it._action == NODE_ACTION_UP ? "i;" : "d;" );
    }
    return ret;
  }

private:
  friend class boost::serialization::access;

  template <class Archive>
  void serialize(Archive &ar, const unsigned int version)
  {
    std::lock_guard<std::mutex> guard( _lock );
    ar & _updates;
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

  size_t Updates( const ComputeSetUpdates_t &upd, const bool down )
  {
    for( auto it : upd )
    {
      if( _refs[ it._name ] > 0 )
        _refs.at( it._name )--;

      if( _refs[ it._name ] == 0 )
        _events.push_back( ComputeActionEntry_t( it._name,
                                                 (down ? COMPUTE_DOWN : COMPUTE_LOST_CONNECTION ) ) );
      if( _refs[ it._name ] == 1 )
        _events.push_back( ComputeActionEntry_t( it._name, COMPUTE_LOST_REDUNDANCY ) );

      CSMLOG( csmd, debug ) << (it._action == NODE_ACTION_UP ? "UP" : "DOWN" ) << "counting " << it._name
          << " refs=" << std::to_string( _refs[ it._name ] );
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
              const ComputeSetUpdates_t &upd )
  {
    if( addr == nullptr ) return -EINVAL;

    std::lock_guard<std::mutex> guard( _lock );
    CSMLOG( csmd, debug ) << "Updating AGG: " << addr->Dump()  << " updates:" << upd.size();

    if( ! upd.empty() )
      _refcount.Updates( upd, true );

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
