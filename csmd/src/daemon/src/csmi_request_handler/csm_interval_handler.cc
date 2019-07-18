/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/csm_interval_handler.cc

  © Copyright IBM Corporation 2015-2019. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#ifndef logprefix
#define logprefix "INTERVAL_HDLR"
#endif

#include "csm_pretty_log.h"
#include "csm_interval_handler.h"
#include "helpers/EventHelpers.h"
#include "csmd/include/csm_daemon_config.h"
#include "csmi/include/csm_api.h"
//#incllude "csmd/src/daemon/include/csm_recurring_tasks.h"


void
CSM_INTERVAL_HANDLER::Process( const csm::daemon::CoreEvent &aEvent,
                               std::vector<csm::daemon::CoreEvent*>& postEventList )
{
    CSMLOG( csmd, info ) << "INTERVAL: triggered handler to process";

    // Get the config.
    csm::daemon::RecurringTasks RT = csm::daemon::Configuration::Instance()->GetRecurringTasks();

    // Create an event for Soft Failure.
    csm::daemon::SoftFailRecovery_t sfRec = RT.GetSoftFailRecovery();
    if ( sfRec._Enabled )
    {
        // Construct the buffer.
        char *buffer = nullptr;
        uint32_t bufferLen = 0;

        csm_soft_failure_recovery_input_t sfInput;
        csm_init_struct_versioning(&sfInput);
        sfInput.retry_count = sfRec._Retry;

        csm_serialize_struct( csm_soft_failure_recovery_input_t, &sfInput, &buffer, &bufferLen );

        if( buffer == nullptr )
        {
          CSMLOG( csmd, info ) << "INTERVAL: soft_failure data serialization failed. Skipping.";
          return;
        }

        // Construct the message with a random messageID and a SELF send.
        csm::network::AddressAbstract_sptr sfAddr =
            std::make_shared<csm::network::AddressAbstract>(csm::network::ABSTRACT_ADDRESS_SELF);

        uint32_t msgID = random();
        csm::network::Message sfMessage;
        sfMessage.Init( CSM_CMD_soft_failure_recovery, 0, CSM_PRIORITY_DEFAULT,
            msgID, 0x1234, 0x4321, geteuid(), getegid(), std::string(buffer, bufferLen) );

        // Make sure the buffer is totally free.
        if ( buffer )
            free(buffer);

        // Push the Network event.
        postEventList.push_back(csm::daemon::helper::CreateNetworkEvent(sfMessage,sfAddr));
    }
}
