/*================================================================================

    csmnet/tests/network_ctrl_path_test.cc

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
//#include <memory>
#include <logging.h>
#include "csm_test_utils.h"
#include "CPP/address.h"
#include "CPP/network_ctrl_path.h"


int main(int argc, char *argv[])
{
	int rc = 0;
	csm::network::NetworkCtrlPath ctrlpath;

	rc += TEST( ctrlpath.CtrlEventCount(), 0 );
	rc += TEST( ctrlpath.GetCtrlEvent(), nullptr );

	ctrlpath.AddCtrlEvent(csm::network::NET_CTL_TIMEOUT, 1);
	ctrlpath.AddCtrlEvent(csm::network::NET_CTL_DISCONNECT, std::make_shared<csm::network::AddressUnix>(CSM_NETWORK_LOCAL_SSOCKET));
	ctrlpath.AddCtrlEvent(std::make_shared<csm::network::NetworkCtrlInfo>());

	rc += TEST( ctrlpath.CtrlEventCount(), 3 );


	csm::network::NetworkCtrlInfo *a = new csm::network::NetworkCtrlInfo(), *b = new csm::network::NetworkCtrlInfo(),
									*c = new csm::network::NetworkCtrlInfo();
	a->_Next = b;
	b->_Next = c;
	c->_Next = nullptr;

	ctrlpath.PushBack(a); // deletes original list
	rc += TEST( ctrlpath.CtrlEventCount(), 6 );


	csm::network::NetworkCtrlInfo_sptr info = ctrlpath.GetCtrlEvent();
	rc += TEST( ctrlpath.CtrlEventCount(), 5 );

	rc += TEST( (csm::network::NET_CTL_TIMEOUT == info->_Type && 
				1 == info->_MsgId && nullptr == info->_Address),
				true );

	return rc;
}