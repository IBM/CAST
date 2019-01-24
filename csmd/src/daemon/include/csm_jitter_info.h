/*================================================================================

    csmd/src/daemon/include/csm_jitter_info.h

    © Copyright IBM Corporation 2019. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#ifndef CSMD_SRC_DAEMON_INCLUDE_JITTER_INFO_H_
#define CSMD_SRC_DAEMON_INCLUDE_JITTER_INFO_H_

#include <string>
#include <unistd.h>

namespace csm {
namespace daemon {


class CSM_Jitter_Info
{
private:

    int32_t     _MaxCoreIsolation;
    int32_t     _SystemSMT;
    bool        _IRQAffinity;
    std::string _SocketOrder;

public:
    CSM_Jitter_Info():
        _MaxCoreIsolation(0), _SystemSMT(0), _IRQAffinity(true), _SocketOrder("") {}

    void Init(
            std::string socketOrder,
            int32_t     maxCoreIsolation,
            int32_t     systemSMT,
            bool        irqAffinity) 
    {
        _MaxCoreIsolation = maxCoreIsolation;
        _SystemSMT        = systemSMT;
        _IRQAffinity      = irqAffinity;
        _SocketOrder      = socketOrder;
    }

    ~CSM_Jitter_Info() {}
   
    std::string GetSocketOrder()   const { return _SocketOrder;      } 
    int32_t     GetMaxCoreIso()    const { return _MaxCoreIsolation; }
    int32_t     GetSystemSMT()     const { return _SystemSMT;        }
    bool        GetIRQAffinity()   const { return _IRQAffinity;      }

    std::string toString()
    {
        return "";
            //( GetIRQAffinity() ? "true" : "false" ) + " " +
            //( GetCoreIsolation() ? "true" : "false" ) + " " + 
            //( GetCoreIsolation() ? "true" : "false" );
    }

};

}  // daemon
} // csm


#endif /* CSMD_SRC_DAEMON_INCLUDE_JITTER_INFO_H_ */
