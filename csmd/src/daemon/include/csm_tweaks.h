/*================================================================================

    csmd/src/daemon/include/csm_tweaks.h

  © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#ifndef CSMD_SRC_DAEMON_INCLUDE_CSM_TWEAKS_H_
#define CSMD_SRC_DAEMON_INCLUDE_CSM_TWEAKS_H_

namespace csm {
namespace daemon {

struct Tweaks
{
  unsigned _NetMgr_polling_loops;
  uint64_t _DCGM_update_interval_s;
  double   _DCGM_max_keep_age_s; 
  uint32_t _DCGM_max_keep_samples;
};

template<class stream>
static stream&
operator<<( stream &out, const csm::daemon::Tweaks &data )
{
  out << " net:ploops=" << data._NetMgr_polling_loops << " dcgm:updint=" << data._DCGM_update_interval_s
      << " dcgm:maxage=" << data._DCGM_max_keep_age_s << " dcgm:maxsamples=" << data._DCGM_max_keep_samples;
  return (out);
}



}  // daemon
} // csm

#endif /* CSMD_SRC_DAEMON_INCLUDE_CSM_TWEAKS_H_ */
