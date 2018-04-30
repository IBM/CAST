/*================================================================================

    csmd/src/ras/include/RasSmartLock.h

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#ifndef RASSMARTLOCK_H_
#define RASSMARTLOCK_H_

#include <pthread.h>

// This class implements a "smart lock" idiom for a pthread mutex.
// The constructor locks the mutex and the destructor unlocks it.
class RasSmartLock
{
public:
	pthread_mutex_t* _lock;
	RasSmartLock(pthread_mutex_t* lock)
	{
		_lock = lock;
		_islocked = true;
		pthread_mutex_lock(_lock);
	}
	void unlock() {
		if (_islocked) {
			pthread_mutex_unlock(_lock);
			_islocked = false;
		}
	}
  
	~RasSmartLock()
	{
		unlock();
	}
protected:
	bool _islocked;
};

#endif /*RASSMARTLOCK_H_*/



