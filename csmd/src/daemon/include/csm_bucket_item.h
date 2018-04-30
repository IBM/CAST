/*================================================================================

    csmd/src/daemon/include/csm_bucket_item.h

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#ifndef CSMD_SRC_DAEMON_INCLUDE_CSM_BUCKET_ITEM_H_
#define CSMD_SRC_DAEMON_INCLUDE_CSM_BUCKET_ITEM_H_



namespace csm {
namespace daemon {

typedef enum
{
  #define bucket_item(n) n,
  #include "include/bucket_item_type_definitions.h"
  #undef bucket_item
  NUM_BUCKET_ITEMS
} BucketItemType;

static std::map<std::string, BucketItemType> str2BucketItemType =
{
#define bucket_item(n) { #n, n },
#include "include/bucket_item_type_definitions.h"
#undef bucket_item
};

static const char* BucketItemTypeStr[] =
{
#define bucket_item(n) #n,
#include "include/bucket_item_type_definitions.h"
#undef bucket_item
};

template<class stream>
static stream&
operator<<( stream &out, const csm::daemon::BucketItemType &aType )
{
  if (aType < csm::daemon::NUM_BUCKET_ITEMS) out << csm::daemon::BucketItemTypeStr[aType];
  else out << "BucketItemType Out Of Range";

  return out;
}

}
}

#endif /* CSMD_SRC_DAEMON_INCLUDE_CSM_BUCKET_ITEM_H_ */
