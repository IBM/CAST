/*================================================================================

    csmnet/tests/address_test.cc

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#include "csm_test_utils.h"
#include "CPP/address.h"

int main(int argc, char **argv)
{
	int rc = 0;

	// AddressUnix

	csm::network::AddressUnix unix(CSM_NETWORK_LOCAL_SSOCKET);
	csm::network::AddressUnix unix2(CSM_NETWORK_LOCAL_SSOCKET);
	csm::network::AddressUnix empty;

	rc += TEST( unix.GetAddrType(), csm::network::CSM_NETWORK_TYPE_LOCAL );
	rc += TEST( unix.Dump(), CSM_NETWORK_LOCAL_SSOCKET );

	rc += TEST( empty.IsEmpty(), true );
	
	rc += TEST( unix == unix2, true );
	rc += TEST( unix == empty, false );
	rc += TEST( unix != unix2, false );
	rc += TEST( unix != empty, true );
	
	unix.SetEmpty(true);
	rc += TEST( unix.IsEmpty(), true );

	/*unix.SetAddrType(csm::network::CSM_NETWORK_TYPE_ABSTRACT);
	rc += TEST( unit.GetAddrType(), csm::network::CSM_NETWORK_TYPE_ABSTRACT );
	try {
		AddressUnix copy(unit);
		rc += 1;
	} catch (csm::network::Exception& e) {
		LOG(csmnet, always) << "Expected exception";
	}*/


	// AddressPTP

	csm::network::AddressPTP ptp(0x7f000001, 8000), ptp2(0x7f000001, 4000), ptp3(0,0), ptp4(0x7f000001, 8000);
	
	rc += TEST( ptp.GetAddrType(), csm::network::CSM_NETWORK_TYPE_PTP );
	rc += TEST( ptp.Dump(), "127.0.0.1:8000" );

	rc += TEST( ptp == ptp2, false );
	rc += TEST( ptp == ptp3, false );
	rc += TEST( ptp == ptp4, true );
	rc += TEST( ptp != ptp2, true );
	rc += TEST( ptp != ptp3, true );
	rc += TEST( ptp != ptp4, false );


	// AddressUtility
        csm::network::AddressUtility util(0x7f000001, 8000), util2(0x7f000001, 4000), util3(0,0), util4(0x7f000001, 8000);

        rc += TEST( util.GetAddrType(), csm::network::CSM_NETWORK_TYPE_UTILITY );
        rc += TEST( util.Dump(), "127.0.0.1:8000" );

        rc += TEST( util == util2, false );
        rc += TEST( util == util3, false );
        rc += TEST( util == util4, true );
        rc += TEST( util != util2, true );
        rc += TEST( util != util3, true );
        rc += TEST( util != util4, false );


        // AddressAggregator
        csm::network::AddressAggregator agg(0x7f000001, 8000), agg2(0x7f000001, 4000), agg3(0,0), agg4(0x7f000001, 8000);

        rc += TEST( agg.GetAddrType(), csm::network::CSM_NETWORK_TYPE_AGGREGATOR );
        rc += TEST( agg.Dump(), "127.0.0.1:8000" );

        rc += TEST( agg == agg2, false );
        rc += TEST( agg == agg3, false );
        rc += TEST( agg == agg4, true );
        rc += TEST( agg != agg2, true );
        rc += TEST( agg != agg3, true );
        rc += TEST( agg != agg4, false );


        // AddressAbstract

	csm::network::AddressAbstract abstract(csm::network::ABSTRACT_ADDRESS_MASTER);

	rc += TEST( abstract.GetAddrType(), csm::network::CSM_NETWORK_TYPE_ABSTRACT );
	rc += TEST(abstract.Dump(), "ABSTRACT_ADDRESS_MASTER" );
	
	std::cout << "rc=" << rc << std::endl;
	return rc;
}
