/*================================================================================

    csmnet/tests/csm_network_header_test.cc

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
/** file csm_network_header_test.cc
 *
 ******************************************/

#include <iostream>
#include <cstring>
#include "csmutil/include/csm_test_utils.h"
#include "csm_network_header.h"
#include <C/csm_network_msg_c.h>

int main(int argc, char **argv)
{
  int rc = 0;

  int i, datalen, deadspace;
  char *data;
  uint32_t *ptr;
  csm_network_header test_header;

  bzero(&test_header, sizeof(csm_network_header));
  rc += TEST(csm_header_validate( &test_header ), false);

  test_header._ProtocolVersion = CSM_NETWORK_PROTOCOL_VERSION;
  rc += TEST(csm_header_validate( &test_header ), false);

  test_header._MessageID = 123456;
  rc += TEST(csm_header_validate( &test_header ), false);

  test_header._CheckSum = csm_header_check_sum( &test_header, nullptr );
  rc += TEST(csm_header_validate( &test_header ), true);

  test_header._Priority = 41;
  rc += TEST(csm_header_validate( &test_header ), false);

  test_header._Flags = 0x45;
  rc += TEST(csm_header_validate( &test_header ), false);

  // crashtest...
  rc += TEST(csm_header_validate( nullptr ), false);


  // checksum testing
  deadspace = random() % 256 + sizeof( uint32_t );
  datalen = random() % 1048576 + deadspace;
  data = (char*)malloc( datalen + deadspace );
  if( data )
  {
    // populate some random data
    ptr = (uint32_t*)data;
    for( i=0; i < (datalen + 2 * deadspace) / (int)sizeof( uint32_t ); ++i )
      ptr[i] = random();

    test_header._DataLen = datalen;
    uint32_t chksum = csm_header_check_sum( &test_header, data );
    rc += TEST( csm_header_check_sum( &test_header, data ), chksum );

    // checksum should change if datalen is different
    test_header._DataLen = datalen - deadspace;
    rc += TESTFAIL( csm_header_check_sum( &test_header, data ), chksum );

    test_header._DataLen = datalen + deadspace;
    rc += TESTFAIL( csm_header_check_sum( &test_header, data ), chksum );

    // checksum should change if data changes
    data[0] ^= 0xFF;
    rc += TESTFAIL( csm_header_check_sum( &test_header, data ), chksum );

    // crashtest on data
    rc += TEST( csm_header_check_sum( nullptr, nullptr ), 0 );
    rc += TEST( csm_header_check_sum( &test_header, nullptr ), 0 );

    test_header._DataLen = 0;
    rc += TESTFAIL( csm_header_check_sum( &test_header, nullptr ), chksum );

    // header without data needs to return valid checksum ( != 0 )
    rc += TESTFAIL( csm_header_check_sum( &test_header, nullptr ), 0 );

    free( data );
  }
  else
    rc += 1;
  std::cout << "CSM Network Header Test: rc=" << rc << std::endl;
  return rc;
}
