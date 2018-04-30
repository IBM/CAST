/*================================================================================

    csmnet/src/CPP/csm_version_msg.h

  © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#ifndef CSMNET_SRC_CPP_CSM_VERSION_MSG_H_
#define CSMNET_SRC_CPP_CSM_VERSION_MSG_H_

#include <string>
#include <sstream>

#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/split_member.hpp>

#include "csm_version.h"
#include "csm_network_exception.h"

namespace csm {
namespace network {

typedef struct {
  std::string _Version;
  std::string _Hostname;
  uint64_t _Sequence;

  template <class Archive>
  void serialize(Archive &ar, const unsigned int version)
  {
    ar & _Version;
    ar & _Hostname;
    ar & _Sequence;
  }
} VersionStruct;



class VersionMsg {
  static VersionMsg *_version;
  std::string _Version;
  std::string _Hostname;
  uint64_t _Sequence;

  VersionMsg( const std::string v,
              const std::string h );

public:

  static VersionMsg* Init( const std::string h )
  {
    if( _version == nullptr )
      _version = new VersionMsg( std::string( CSM_VERSION), h );
    return _version;
  }

  static VersionMsg* Get()
  {
    if( _version == nullptr )
      throw csm::network::ExceptionFatal("VersionMsg not initialized. Can't continue." );
    return _version;
  }

  static std::string ConvertToBytes( VersionMsg *msg )
  {
    std::stringstream ss;
    boost::archive::text_oarchive oa(ss);
    oa << *msg;
    msg->Next();  // bump the sequence number each time we generate a string
    return ss.str();
  }

  static void ConvertToClass(const std::string &i_payload, VersionStruct &msg )
  {
    std::stringstream ss;
    ss << i_payload;
    boost::archive::text_iarchive ia(ss);
    ia >> msg;
  }

  inline std::string GetVersion() const { return _Version; }
  inline std::string GetHostName() const { return _Hostname; }
  inline uint64_t GetSequence() const { return _Sequence; }


  inline void Next() { ++_Sequence; }

  bool operator==( const VersionMsg &msg )
  {
    return ( 0 == _Version.compare( msg._Version ) );
  }

private:
  friend class boost::serialization::access;

  template <class Archive>
  void serialize(Archive &ar, const unsigned int version)
  {
    ar & _Version;
    ar & _Hostname;
    ar & _Sequence;
  }

};

}
}

#endif /* CSMNET_SRC_CPP_CSM_VERSION_MSG_H_ */
