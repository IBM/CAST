/*================================================================================

    csmd/src/ras/include/RasEventHandlerEventFilter.h

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#ifndef __RASHANDLEREVENTFILTER_H__
#define __RASHANDLEREVENTFILTER_H__


#include "RasEventHandler.h"
#include "RasEventChangeSpecList.h"

#include <string>
#include <vector>

class RasEventHandlerEventFilter : public RasEventHandler
{
 public:
  virtual RasEvent& handle(RasEvent& event);
  virtual const std::string& name() { return _name; }
  RasEventHandlerEventFilter(const RasEventChangeSpecList & specs);
  virtual ~RasEventHandlerEventFilter();
 private:
  std::string _name;
  RasEventChangeSpecList _specs;
};






#endif



