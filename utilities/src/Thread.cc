/*******************************************************************************
 |    Thread.cc
 |
 |  © Copyright IBM Corporation 2015,2016. All Rights Reserved
 |
 |    This program is licensed under the terms of the Eclipse Public License
 |    v1.0 as published by the Eclipse Foundation and available at
 |    http://www.eclipse.org/legal/epl-v10.html
 |
 |    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
 |    restricted by GSA ADP Schedule Contract with IBM Corp.
 *******************************************************************************/


//! \file  Thread.cc
//! \brief Methods for txp::Thread class.

// Includes
#include "Thread.h"
#include <errno.h>


using namespace txp;

int
Thread::setDetached(void)
{
   // Note that attributes can only be set before the thread is started.
   int rc = EBUSY;
   if (_threadId == 0) {
      rc = pthread_attr_setdetachstate(&_attributes, PTHREAD_CREATE_DETACHED);
   }
   return rc;
}

bool
Thread::isDetached(void)
{
   int value = 0;
   pthread_attr_getdetachstate(&_attributes, &value);
   return (value == PTHREAD_CREATE_DETACHED);
}

int
Thread::setStackSize(size_t stackSize)
{
   // Note that attributes can only be set before the thread is started.
   int rc = EBUSY;
   if (_threadId == 0) {
      rc = pthread_attr_setstacksize(&_attributes, stackSize);
   }
   return rc;
}

size_t
Thread::getStackSize(void)
{
   size_t stackSize = 0;
   pthread_attr_getstacksize(&_attributes, &stackSize);
   return stackSize;
}

int
Thread::start(void)
{
   int rc = EBUSY;
   if (_threadId == 0) {
      rc = pthread_create(&_threadId, &_attributes, startWrapper, this);
   }
   return rc;
}

void *
Thread::startWrapper(void *arg)
{
   Thread *thisPtr = (Thread *)arg;
   void *returnVal = thisPtr->run();
   return returnVal;
}
