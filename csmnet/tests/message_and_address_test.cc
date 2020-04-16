/*================================================================================

    csmnet/tests/message_and_address_test.cc

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#include "csm_test_utils.h"
#include "CPP/csm_message_and_address.h"

int main(int argc, char **argv)
{
	int rc = 0;
	
	csm_network_header_t header = {0};
	csm::network::Message Msg(header, "message");
	csm::network::Address_sptr Addr = std::make_shared<csm::network::AddressUnix>(CSM_NETWORK_LOCAL_SSOCKET);

	csm::network::MessageAndAddress MsgAndAddr;//(Addr);

	MsgAndAddr.SetAddr(Addr);
	rc += TEST( MsgAndAddr.GetAddr(), Addr );

	MsgAndAddr.Init(Msg, nullptr);
	rc += TEST( MsgAndAddr._Msg == Msg, true );
	rc += TEST( MsgAndAddr.GetAddr(), nullptr );
	

	return rc;
}
