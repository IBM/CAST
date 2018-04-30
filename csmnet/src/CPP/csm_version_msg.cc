/*================================================================================

    csmnet/src/CPP/csm_version_msg.cc

  © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#include "csm_version_msg.h"


csm::network::VersionMsg* csm::network::VersionMsg::_version = NULL;


csm::network::VersionMsg::VersionMsg( const std::string v,
            const std::string h )
: _Version( v ), _Hostname( h ), _Sequence(0)
{
}

