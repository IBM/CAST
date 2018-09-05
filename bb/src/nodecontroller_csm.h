/*******************************************************************************
 |    nodecontroller.h
 |
 |  � Copyright IBM Corporation 2015,2016. All Rights Reserved
 |
 |    This program is licensed under the terms of the Eclipse Public License
 |    v1.0 as published by the Eclipse Foundation and available at
 |    http://www.eclipse.org/legal/epl-v10.html
 |
 |    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
 |    restricted by GSA ADP Schedule Contract with IBM Corp.
 *******************************************************************************/

#ifndef D_NODECONTROLLER_CSM_H
#define D_NODECONTROLLER_CSM_H

#include <string>
#include "nodecontroller.h"
#include "csm_api_workload_manager.h"

class NodeController_CSM : public NodeController
{
  private:
    csm_api_object* csmhandle;
    std::string myhostname;
    std::map< std::pair<uint64_t,uint32_t>, csmi_allocation_t* > job2allocationmap;

    int convertState(enum LVState state, char& statechar);
    int getAllocationInfo(csmi_allocation_t& allocation);

  public:
    NodeController_CSM();
    virtual ~NodeController_CSM();

    virtual int gethostname(std::string& pHostName);
    virtual int gethostlist(std::string& hostlist);
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

#endif
