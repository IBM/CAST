/*================================================================================

    csmd/src/daemon/include/csm_mosquitto_utils.h

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#ifndef CSM_DAEMON_SRC_CSM_MOSQUITTO_UTILS_H_
#define CSM_DAEMON_SRC_CSM_MOSQUITTO_UTILS_H_

#include <string>
#include <mosquitto.h>

#include "logging.h"

class MosqppUtils
{

public:
  // sub: subscription string to check topic against
  // topic to check
  static bool topic_matches_sub(const std::string &sub, const std::string &topic)
  {
    bool result;
    
    int ret = mosquitto_topic_matches_sub(sub.c_str(), topic.c_str(), &result);
    if (ret != MOSQ_ERR_SUCCESS)
    {
       LOG(csmd, warning) << "MosqppUtils::topic_matches_sub(): " <<  mosquitto_strerror(ret);
    }
    return result;
  }
};
#endif