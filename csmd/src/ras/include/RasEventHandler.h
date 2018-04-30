/*================================================================================

    csmd/src/ras/include/RasEventHandler.h

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#ifndef RASEVENTHANDLER_H_
#define RASEVENTHANDLER_H_

#include <string>
#include "RasEvent.h"


/** 
 * \class RasEventHandler
 *
 * RasEventHandler is responsible for handling a RasEvent 
 * which is an object that contains the details about a RasEvent. 
 * 
 * The handler can add and update event details.
 */

class RasEventHandler
{
public:
  /** 
   * Handle the RasEvent.
   * @return event which is the same event returned (ostream design pattern)
   */
  virtual RasEvent& handle(RasEvent& event) = 0;

  virtual ~RasEventHandler() {}
  
  /** 
   * Get the name of the handler.
   * @return string handler name
   */
  virtual const std::string& name() = 0;

  #if 0
  static std::string logPrefix();
  #endif
protected:
};

#endif /*RASEVENTHANDLER_H_*/


