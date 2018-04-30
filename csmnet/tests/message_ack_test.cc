/*================================================================================

    csmnet/tests/message_ack_test.cc

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#include <iostream>
#include <unordered_set>
#include "CPP/csm_message_and_address.h"
#include "CPP/message_ack.h"
#include "csm_test_utils.h"
#include <logging.h>
#include <thread>

int main(int argc, char *argv[])
{
    int rc = 0;
    csm::network::MessageACK message_ack;

    // RegisterAck,	GetAckCount, AckReceived
    //rc += TEST( message_ack.GetAckCount(), 0 );

    message_ack.SetDefaultTimeout(1);
    csm::network::MessageAndAddress MsgAddr;

    MsgAddr._Msg.SetMessageID(123456);
    MsgAddr._Msg.SetResp();
    MsgAddr._Msg.SetPriority(CSM_PRIORITY_WITH_ACK);
    message_ack.RegisterAck(MsgAddr);

    rc += TEST( message_ack.GetAckCount(), 1 );

    MsgAddr._Msg.SetMessageID(98765);
    rc += TEST( message_ack.AckReceived(MsgAddr._Msg), false );
    message_ack.RegisterAck(MsgAddr);

    MsgAddr._Msg.SetMessageID(123456);
    rc += TEST( message_ack.AckReceived(MsgAddr._Msg), true );

    // wait for both to timeout
    std::this_thread::sleep_for(std::chrono::seconds(2));
    message_ack.UpdateClock();

    rc += TEST( message_ack.CheckTimeout(), (std::make_pair<csm::network::AckKeyType, csm::network::Address_sptr>(0, nullptr)) );
    rc += TEST( message_ack.CheckTimeout(), (std::make_pair<csm::network::AckKeyType, csm::network::Address_sptr>(
                                                            csm::network::AckKeyType(98765, true), nullptr) ));


    // CheckCreateACKMsg
    csm::network::Address_sptr addr = std::make_shared<csm::network::AddressAbstract>(csm::network::ABSTRACT_ADDRESS_AGGREGATOR);
    std::unique_ptr<csm::network::MessageAndAddress> ret(message_ack.CheckCreateACKMsg(MsgAddr, addr));
    
    // check these were set
    rc += TEST( ret->GetAddr(), addr);
    rc += TEST( ret->_Msg.GetAck(), true );
    rc += TEST( ret->_Msg.GetData(), "" );
    rc += TEST( ret->_Msg.GetPriority(), CSM_PRIORITY_NO_ACK );

    // check that the others were copied
    rc += TEST( ret->_Msg.GetMessageID(), MsgAddr._Msg.GetMessageID() );
    rc += TEST( ret->_Msg.GetResp(), MsgAddr._Msg.GetResp() );

    // no ack, should return null
    rc += TEST(message_ack.CheckCreateACKMsg(*ret, nullptr), nullptr );

    std::cout << "Exiting with rc=" << rc << std::endl;
    return rc;
}
