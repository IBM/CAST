/*================================================================================

    csmi/src/common/include/csmi_python.h

    © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#include "csmi/include/csm_api_common.h"
#include <mutex>
#include <unordered_map>
#include <vector>
#include "csm_python_x_macros.h"

/**
 * A Singleton Class for csm api objects, used to make the python bindings work.
 */
class CSMIObj
{
    private:
        int64_t _ObjCounter;        ///< The number of objects created by by the python module.
        std::mutex _ObjCounterMutex;///< Mutex lock for the object counter.

        std::unordered_map<int64_t, csm_api_object*> _Objects; ///< The object mapping.
        std::mutex _ObjectsMutex;                           ///< Mutex lock for the objects map.

    public:
        static CSMIObj& GetInstance()
        {
            static CSMIObj instance;
            return instance;
        }

        /** @brief Destroys a csm api object with the specified oid.
         * @param[in] oid The object id in the @ref _Objects mapping.
         */
        void DestroyCSMObj(int64_t oid);

        /** @brief Inserts the csm api object into the 
         * @param[in] obj The @ref csm_api_object to cache for later.
         * @param[in] oid The object identifier, optional if specified will clear any objects present.
         */
        int64_t StoreCSMObj(csm_api_object* obj, int64_t oid = -1 );

        
        /** @brief Executes @ref csm_api_object_destroy on contents of @ref _Objects.
         */
        ~CSMIObj();

    private:
        CSMIObj(): _ObjCounter(0) {}
        CSMIObj(CSMIObj const&);
        void operator=(CSMIObj const&);
};

