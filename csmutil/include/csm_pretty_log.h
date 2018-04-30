/*================================================================================

    csmutil/include/csm_pretty_log.h

  © Copyright IBM Corporation 2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#ifndef CSMUTIL_INCLUDE_CSM_PRETTY_LOG_H_
#define CSMUTIL_INCLUDE_CSM_PRETTY_LOG_H_

#include "logging.h"


#ifdef logprefix

#define CSMLOG( component, level ) LOG( component, level ) << logprefix << ": "
#define CSMLOGp( component, level, prefix ) LOG( component, level ) << prefix << ": "

#else
#define CSMLOG( component, level ) LOG( component, level )
#define CSMLOGp( component, level, prefix ) LOG( component, level ) << prefix << ": "
#endif


#endif /* CSMUTIL_INCLUDE_CSM_PRETTY_LOG_H_ */
