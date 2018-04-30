/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/csmi_stateful_db.cc

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#include "csmi_stateful_db.h"
#include "csmi_stateful_db/CSMIStatefulDBInit.h"
#include "csmi_stateful_db/CSMIStatefulDBRecvDB.h"
#include "csmi_stateful_db/CSMIStatefulDBRecvPrivate.h"


CSMIStatefulDB::CSMIStatefulDB(
    csmi_cmd_t cmd,
    csm::daemon::HandlerOptions& options, 
    uint32_t numStates):
    CSMIStateful(cmd,options)
{
    if ( numStates >= STATEFUL_DB_DONE )
    {
        uint32_t offset = numStates - STATEFUL_DB_DONE;

        SetInitialState(STATEFUL_DB_INIT);

        ResizeStates(numStates);

        SetState( STATEFUL_DB_INIT,
            new StatefulDBInit(
                this,
                STATEFUL_DB_RECV_DB,
                numStates,
                numStates,
                csm::daemon::helper::BAD_STATE,
                csm::daemon::helper::BAD_TIMEOUT_LEN,
                STATEFUL_DB_RECV_PRI)
        );

        SetState( STATEFUL_DB_RECV_PRI,
            new StatefulDBRecvPrivate(
                this,
                STATEFUL_DB_RECV_DB,
                numStates,
                numStates
            )
        );
        
        SetState( STATEFUL_DB_RECV_DB + offset,
            new StatefulDBRecv(
                this, 
                STATEFUL_DB_DONE,
                numStates,
                numStates                
            )
        );
    }
}
