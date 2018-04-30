/*================================================================================

    csmd/src/daemon/tests/csm_nodeset_test.cc

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
/* csm_nodeset_test.cc
 *
 ******************************************/

#include "logging.h"
#include <string>
#include <set>
#include <chrono>

#include "csmnet/src/CPP/address.h"
#include "include/csm_node_bitset.h"
#include "include/csm_node_set.h"

#include "csmutil/include/csm_test_utils.h"

#define NODE_COUNT_MAX (4608)
//#define NODE_COUNT_MAX (100)
#define AGG_COUNT (16)
#define TEST_COUNT (10000)

#define RANDOM_MAGIC (0xFFFFFFFF)

//typedef csm::daemon::NodeBitSet BitFilter_t;
typedef csm::daemon::NodeSetBloomFilter BitFilter_t;
//typedef csm::daemon::NodeSetMultiFilter BitFilter_t;

typedef struct
{
  double _avg;
  unsigned _msgs[ TEST_COUNT ];
  unsigned _min;
  unsigned _max;
  uint64_t _mic;
  unsigned _idx;

  void reset()
  {
    _avg = 0.0;
    memset( _msgs, 0, TEST_COUNT * sizeof(unsigned) );
    _min = INT_MAX;
    _max = 0;
    _mic = 0;
    _idx = 0;
  }
  void update( const unsigned c, const uint64_t mic )
  {
    _avg += c;
    _msgs[ _idx ] = c;
    _mic += mic;
    if( _min > c ) _min = c;
    if( _max < c ) _max = c;
    _idx = ( _idx + 1 ) % TEST_COUNT;
  }
} Stats_t;

std::string RandomString( const uint64_t number = RANDOM_MAGIC )
{
  std::string out = "ce";
  int len = 4;

  if( number == RANDOM_MAGIC )
  {
    char c[2];
    c[1] = '\0';
    for( int n=0; n<len; ++n )
    {
      c[0] = (char)( random() % 10 + 48 );
      out.append( c, 1 );
    }
  }
  else
  {
#define RACKSIZE (18)
#define COLUMS (12)
    uint64_t r = number / (RACKSIZE*COLUMS);
    uint64_t c = ( number / RACKSIZE ) % COLUMS;
    uint64_t n = number % RACKSIZE + 1;

    len = 2;

    out.append("-r");
    out.append( std::to_string(r) );
    out.append("c");
    out.append( std::to_string(c) );
    out.append("n");

    std::string nm = std::to_string(n);
    for( unsigned i=0; i<len-nm.length(); ++i )
      out.append("0");
    out.append( nm );
  }
  return out;
}

csm::daemon::NodeNameList_t GenerateNodeList( const int i_Size )
{
  csm::daemon::NodeNameList_t ret;
  std::set<std::string> nodeSet;
  for( int n=0; n<i_Size; ++n )
  {
    std::string name;
    do
    {
      name = RandomString( n+1 );
    } while ( nodeSet.find( name ) != nodeSet.end() );
    nodeSet.insert( name );
//    std::cout << "  " << name;
    ret.push_back( name );
  }
  return ret;
}

csm::daemon::NodeNameList_t PickNodeList( const csm::daemon::NodeNameList_t &i_NodeList,
                                          const int i_Size,
                                          const unsigned i_Offset )
{
  csm::daemon::NodeNameList_t ret;
  if( i_Offset == RANDOM_MAGIC )
  {
    for( int n=0; n<i_Size; ++n )
      ret.push_back( i_NodeList[ random() % NODE_COUNT_MAX ]);
  }
  else
  {
    for( int n=0; n<i_Size; ++n )
      ret.push_back( i_NodeList[ (i_Offset + n) % NODE_COUNT_MAX ]);
  }
  return ret;
}


int NodeMTCTest( const csm::daemon::NodeNameList_t &nodes,
                 const BitFilter_t *aggSets,
                 const unsigned mtcSize )
{
  Stats_t SetMsg, SingleMsg;
  SetMsg.reset();
  SingleMsg.reset();

  for( int i=0; i<TEST_COUNT; ++i )
  {
    csm::daemon::NodeNameList_t mtcNodes = PickNodeList( nodes, mtcSize, (random() % NODE_COUNT_MAX) & ~0xF );
//    csm::daemon::NodeNameList_t mtcNodes = PickNodeList( nodes, mtcSize, RANDOM_MAGIC );

    std::chrono::high_resolution_clock::time_point start_set = std::chrono::high_resolution_clock::now();
    BitFilter_t mtcSet( mtcNodes );

    int msgSetCount = 0;
    for( int i=0; i<AGG_COUNT; ++i )
    {
      if( aggSets[i].MatchSet( mtcSet ) )
        ++msgSetCount;
    }
    std::chrono::high_resolution_clock::time_point end_set = std::chrono::high_resolution_clock::now();
    uint64_t micros = std::chrono::duration_cast<std::chrono::microseconds>(end_set-start_set).count();
    SetMsg.update( msgSetCount, micros );

    std::chrono::high_resolution_clock::time_point start_single = std::chrono::high_resolution_clock::now();
    int msgSingleCount = 0;
    for( int i=0; i<AGG_COUNT; ++i )
    {
      if( aggSets[i].MatchSet( mtcNodes ) )
        ++msgSingleCount;
    }
    std::chrono::high_resolution_clock::time_point end_single = std::chrono::high_resolution_clock::now();
    micros = std::chrono::duration_cast<std::chrono::microseconds>(end_single-start_single).count();
    SingleMsg.update( msgSingleCount, micros );

    // need to send to at least 2 aggregators
    if(( msgSingleCount < 2 )||( msgSetCount < 2 ))
      std::cout << "Coverage error: " << msgSingleCount << ":" << msgSetCount << std::endl;
  }

  double optimal = ceil( (double)mtcSize / ((double)aggSets[0].GetEntries()/2) ) + 2;
  std::cout << "Agg-Msgs for nodes= " << mtcSize
      << ":   Res_set=(" << SetMsg._avg/TEST_COUNT << ", "<< SetMsg._min << "-" << SetMsg._max
      << "; " << SetMsg._mic/TEST_COUNT <<"us/m;" << SetMsg._avg/TEST_COUNT / optimal * 100. << "%)"

      << ";   Res_single=(" << SingleMsg._avg/TEST_COUNT << ", "<< SingleMsg._min << "-" << SingleMsg._max
      << "; " << SingleMsg._mic/TEST_COUNT <<"us/m;" << SingleMsg._avg/TEST_COUNT / optimal * 100. << "%)"

      << " entries = " << aggSets[0].GetEntries() << " opt = " << optimal
      << std::endl;

#if 0
  unsigned printLimit = std::min( TEST_COUNT, 100 );
  for( unsigned n=0; n<printLimit; ++n )
    std::cout << SetMsg._msgs[ n ] << " ";
  std::cout << std::endl;
  for( unsigned n=0; n<printLimit; ++n )
    std::cout << SingleMsg._msgs[ n ] << " ";
  std::cout << std::endl;
  std::cout << std::endl;
#endif


  return 0;
}


int bitset_test( int argc, char **argv )
{
  setLoggingLevel(csmnet, info);

//  srand( time(nullptr) );

  BitFilter_t aggSets[ AGG_COUNT ];

  int listSize = atoi( argv[1] );
  int mtcSize = atoi( argv[2] );
  csm::daemon::NodeNameList_t nodes = GenerateNodeList( NODE_COUNT_MAX );

  BitFilter_t check( nodes );
  std::cout << "Total collisions: " << check.GetCollisions() << std::endl;

  double avg = 0.0;
  for( int i=0; i<AGG_COUNT; ++i )
  {
    aggSets[ i ] = BitFilter_t( PickNodeList( nodes, listSize, (i * listSize/2) ) );
    avg += aggSets[ i ].GetCollisions();
//    std::cout << " entries = " << aggSets[i].GetEntries();
    std::cout << "Coll[" << i << "]=" << aggSets[i].GetCollisions() << "; ";
  }
  std::cout << "Average hash-collisions: " << avg/(double)AGG_COUNT << std::endl;

  int size = 1;
  while( size <= mtcSize )
  {
    NodeMTCTest( nodes, aggSets, size );
    size *= 2;
  }
  return 0;
}

csm::daemon::ComputeNodeList_t GenerateAddrList( const int i_Size )
{
  csm::daemon::ComputeNodeList_t ret;
  std::set<std::string> nodeSet;
  for( int n=0; n<i_Size; ++n )
  {
    std::string name;
    do
    {
      name = RandomString( n+1 );
    } while ( nodeSet.find( name ) != nodeSet.end() );
    nodeSet.insert( name );
//    std::cout << "  " << name;
    ret.push_back( name );
  }
  return ret;
}

int computeset_test( int argc, char **argv )
{
  int rc = 0;

  unsigned listSize = atoi( argv[1] );
//  int mtcSize = atoi( argv[2] );

  csm::daemon::NodeNameList_t AllNodes = GenerateAddrList( NODE_COUNT_MAX );
  csm::daemon::ComputeSet starter;
  size_t c = 0;
  for( csm::daemon::ComputeNodeList_t::iterator it = AllNodes.begin();
       it != AllNodes.end() && (c<listSize);
       ++it )
  {
    if(( c > 10 ) && ( (random() % 20) == 0 ) && ( it - AllNodes.begin() > 10 ))
    { it--; it--; it--; it--; it--; }
    int action = random() % 6;
    int index = it - AllNodes.begin();
    switch( action )
    {
      case 0:
        LOG( csmd, always ) << index << ":" << "Commit; " << starter.GetUncommittedInsert()
          << ":" << starter.GetUncommittedDelete() << " c=" << c;
        starter.Commit( csm::daemon::ComputeSet::SET_COMMIT_BOTH );
        rc += TEST( starter.GetSize(), c );
        if( rc )
          LOG( csmd, always ) << index << ":" << "Commit: c=" << c << " size=" << starter.GetSize();
        it--; // don't count that address
        break;
      case 1:
      case 2:
        if( starter.DelNode( *it ) == true )
          --c;
        LOG( csmd, always ) << index << ":" << "Delete: " << (*it) << "; c=" << c << "; list=" << starter.GetSize();
        rc += TEST( starter.GetSize(), c );
        rc += TEST( starter.HasNode( *it ), false );
        break;
      default:
        if( starter.AddNode( *it ) == true )
          ++c;
        LOG( csmd, always ) << index << ":" << "Insert: " << (*it) << "; c=" << c;
        rc += TEST( starter.GetSize(), c );
        rc += TEST( starter.HasNode( *it ), true );
        break;
    }
    if( rc != 0 )
      LOG( csmd, always ) << index << ":" << " rc=" << rc << " c=" << c << " it=" << (*it) << " Action: " << action;
  }
  rc += TEST( starter.GetSize(), c );

  csm::daemon::ComputeSet comp1( PickNodeList( AllNodes, listSize, (0 * listSize/2) ));
  csm::daemon::ComputeSet comp2( PickNodeList( AllNodes, listSize, (1 * listSize/2) ));

  if( listSize < 20 )
  {
    for( auto it : comp1.GetAddrList() )
      LOG( csmd, always ) << "Comp1: " << it;
    for( auto it : comp2.GetAddrList() )
      LOG( csmd, always ) << "Comp2: " << it;
  }

  rc += TEST( comp1.GetSize(), listSize );
  rc += TEST( comp2.GetSize(), listSize );

  csm::daemon::ComputeNodeList_t isct = comp1.InterSectNodes( comp2.GetAddrList() );
  LOG( csmd, always ) << " Intersection list-size: " << isct.size();
  if( listSize < 20 )
  {
    for( auto it : isct )
    {
      LOG( csmd, always ) << "isct: " << it;
    }

  }
  rc += TEST( isct.size() * 2, comp2.GetSize() );

  csm::daemon::ComputeNodeList_t un = comp1.UnionNodes( comp2.GetAddrList() );
  LOG( csmd, always ) << " Union list-size: " << un.size();
  if( listSize < 20 )
    for( auto it : un )
      LOG( csmd, always ) << "  un: " << it;
  rc += TEST( un.size() * 2, comp1.GetSize() * 3 );

  csm::daemon::ComputeNodeList_t x = comp1.DifferenceNodes( comp2.GetAddrList() );
  LOG( csmd, always ) << " Difference list-size: " << x.size();
  if( listSize < 20 )
    for( auto it : x )
      LOG( csmd, always ) << "   x: " << it;
  rc += TEST( x.size(), comp1.GetSize() / 2 );

  csm::daemon::ComputeSet ser;
  std::string msgdata = csm::daemon::ComputeSet::ConvertDiffToBytes( ser );
  rc += TESTFAIL( msgdata, "" );

  ser.AddNode( "bla1" );
  ser.AddNode( "bla2" );

  msgdata = csm::daemon::ComputeSet::ConvertDiffToBytes( ser );
  rc += TESTFAIL( msgdata.find("bla1"), std::string::npos );
  rc += TESTFAIL( msgdata.find("bla2"), std::string::npos );

  ser.DelNode( "bla3" );
  msgdata = csm::daemon::ComputeSet::ConvertDiffToBytes( ser );
  rc += TEST( msgdata.find("bla3"), std::string::npos );

  ser.Commit();
  ser.AddNode( "bla3" );
  ser.DelNode( "bla1" );
  msgdata = csm::daemon::ComputeSet::ConvertDiffToBytes( ser );
  rc += TESTFAIL( msgdata.find("bla1"), std::string::npos );
  rc += TEST( msgdata.find("bla2"), std::string::npos );
  rc += TESTFAIL( msgdata.find("bla3"), std::string::npos );

  ser.Commit();
  csm::daemon::ComputeSet deser;
  csm::daemon::ComputeSet::ConvertDiffToClass( msgdata, deser );
  rc += TEST( deser.HasNode("bla1"), false );
  rc += TEST( deser.HasNode("bla2"), false );
  rc += TEST( deser.HasNode("bla3"), true );

  rc += TEST( deser.GetDeleteList().size(), 1 );
  rc += TEST( deser.GetInsertList().size(), 1 );



//  for( int i=0; i<AGG_COUNT; ++i )
//  {
//    aggSets[ i ] = BitFilter_t( PickNodeList( AllNodes, listSize, (i * listSize/2) ) );
//    avg += aggSets[ i ].GetCollisions();
////    std::cout << " entries = " << aggSets[i].GetEntries();
//    std::cout << "Coll[" << i << "]=" << aggSets[i].GetCollisions() << "; ";
//  }
//  std::cout << "Average hash-collisions: " << avg/(double)AGG_COUNT << std::endl;
//

  return rc;
}

int aggregatorset_test( int argc, char **argv )
{
  int rc = 0;

  csm::network::Address_sptr agg[ AGG_COUNT ];

  int listSize = atoi( argv[1] );
  csm::daemon::NodeNameList_t nodes = GenerateNodeList( NODE_COUNT_MAX );

  // create aggregator addresses
  for( int i=0; i<AGG_COUNT; ++i )
    agg[ i ] = std::make_shared<csm::network::AddressAggregator>( random() % 65536, random() % 65536  );

  // create the master set of aggregators
  csm::daemon::AggregatorSet master;
  for( int i=0; i<AGG_COUNT; ++i )
  {
    master.Add( agg[i], csm::daemon::ComputeSet( PickNodeList( nodes, listSize, (i * listSize/2) ) ) );
    master.Connect( agg[ i ] );
  }

  // check the coverage of aggs and computes (only the first and the last should have half of the nodes uncovered when failing
  rc += TEST( master.GetAggrDisconnectedNodes( agg[ 0 ] ).size(), (size_t)listSize/2 );
  for( int i=1; i<AGG_COUNT-1; ++i )
    rc += TEST( master.GetAggrDisconnectedNodes( agg[ i ] ).size(), 0 );
  rc += TEST( master.GetAggrDisconnectedNodes( agg[ AGG_COUNT-1 ] ).size(), (size_t)listSize/2 );

  // check node connectivity
  for( size_t i=0; i<nodes.size(); ++i )
  {
    if( i < (size_t)listSize/2 * (AGG_COUNT+1))
      rc += TEST( master.IsNodeConnected( nodes[ i ] ), true );
    else
      rc += TEST( master.IsNodeConnected( nodes[ i ] ), false );
  }

  // disconnect agg 3 and check for full connectivity
  master.Disconnect( agg[ 3 ] );
  for( size_t i=(size_t)listSize; i<(size_t)listSize * 3; ++i )
    rc += TEST( master.IsNodeConnected( nodes[ i ] ), true );

  master.Disconnect( agg[ 4 ] );
  for( size_t i=(size_t)listSize * 2; i<(size_t)(listSize * 5) / 2; ++i )
    rc += TEST( master.IsNodeConnected( nodes[ i ] ), false );

  master.Connect( agg[ 3 ] );
  for( size_t i=(size_t)listSize * 2; i<(size_t)(listSize * 5) / 2; ++i )
    rc += TEST( master.IsNodeConnected( nodes[ i ] ), true );

  return rc;
}


int main( int argc, char **argv )
{
  int rc = 0;

//  rc += bitset_test( argc, argv );
  rc += computeset_test( argc, argv );
  rc += aggregatorset_test( argc, argv );

  LOG( csmd, always ) << "Test exiting with rc=" << rc;
  return rc;
}
