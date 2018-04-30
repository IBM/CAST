/*******************************************************************************
 |    nodecontroller.h
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


#ifndef BB_NODECONTROLLER_H_
#define BB_NODECONTROLLER_H_

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string>
#include <unistd.h>

#include <boost/property_tree/ptree.hpp>

#include "bberror.h"
#include "bbapi_types.h"
#include "usage.h"

enum LVState
{
    LVStateInvalid = 100,
    LVStateCreated,
    LVStateMounted,
    LVStateShrinking,
    LVStateRemoved
};

//
// Control System abstraction class
//
class NodeController
{
  public:
    /**
     * \brief Destructor
     */
    NodeController();
    virtual ~NodeController();

    virtual int gethostname(std::string& pHostName);
    virtual int lvcreate(const std::string& lvname, enum LVState state, size_t current_size,
                         const std::string& mountpath, const std::string& fstype);
    virtual int lvremove(const std::string& lvname, const BBUsage_t& usage);
    virtual int lvupdate(const std::string& lvname, enum LVState state, size_t current_size);
    virtual int postRAS(const TSHandler& tsthis);

    virtual int bbcmd(std::vector<std::uint32_t> ranklist,
                      std::vector<std::string> nodelist,
                      std::string executable,
                      std::vector<std::string> arguments,
                      boost::property_tree::ptree& output);
};

extern NodeController* activecontroller;

int setupNodeController(const std::string& who);

#endif
