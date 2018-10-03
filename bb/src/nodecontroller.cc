/*******************************************************************************
 |    nodecontroller.cc
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

#include "nodecontroller.h"
#include <boost/property_tree/json_parser.hpp>
#include "logging.h"
#include "util.h"
#include "bberror.h"

#ifdef HAVE_CSMI
#include "nodecontroller_csm.h"
#endif

using namespace std;

NodeController  defaultcontroller;
NodeController* activecontroller = &defaultcontroller;

NodeController::NodeController()
{
};

NodeController::~NodeController()
{
};

int NodeController::gethostname(std::string& pHostName)
{
    // \todo - Fix this...  @DLH
    // So, if CSM is around, nodecontroller_csm provides the hostname as known by xcat.
    // This works great for bbProxy.  However, CSM isn't available on ESS (i.e., the bbServers).
    // Therefore, for now we simply use the system call gethostname() to get a hostname
    // when invoked on bbServer via this base class.
    //
    // Also need to figure out what we will do for the hostname that is used in the utils
    // library for tstate.h that is used for bberror.  (Hostname is used as top level branch
    // within the property tree.)
    //
    // Probably would be nice for all of the above to use the hostnames as known by xcat...

    char l_HostName[64] = {'\0'};
    ::gethostname(l_HostName, sizeof(l_HostName));

    pHostName = l_HostName;

    return 0;
}

int NodeController::gethostlist(string& hostlist)
{
    const char* env;
    bool gethostlist = false;
    if((env = getenv("LSF_STAGE_HOSTFILE")) != NULL)
    {
        gethostlist = true;
    }
    else if((env = getenv("LSF_STAGE_HOSTS")) != NULL)
    {
        if(strstr(env, " ") != NULL)
        {
            hostlist = strstr(env, " ")+1;
            replace(hostlist.begin(), hostlist.end(), ' ', ',');
        }
    }
    else if((env = getenv("LSB_DJOB_HOSTFILE")) != NULL)
    {
        gethostlist = true;
    }
    else if((env = getenv("LSB_MCPU_HOSTS")) != NULL)
    {
        vector<string> tok = buildTokens(" ", env);
        for(unsigned i=2; i<tok.size(); i+=2)
        {
            for(unsigned long j=0; j<stoul(tok[i+1]); j++)
            {
                if((i > 2) || (j != 0)) hostlist += ",";
                hostlist += tok[i];
            }
        }
    }
    else if((env = getenv("LSB_HOSTS")) != NULL)
    {
        if(strstr(env, " ") != NULL)
        {
            hostlist = strstr(env, " ")+1;
            replace(hostlist.begin(), hostlist.end(), ' ', ',');
        }
    }

    if(gethostlist)
    {
        string line;
        ifstream hostfile(env);
        while (getline(hostfile, line))
        {
            if(hostlist != "") hostlist += ",";
            hostlist += line;
        }
    }
    if(hostlist == "")
        hostlist = "localhost";
    return 0;
}

int NodeController::lvcreate(const std::string& lvname, enum LVState state, size_t current_size, const std::string& mountpath, const std::string& fstype)
{
    LOG(bb,info) << "Created logical volume for Uuid '" << lvname << "'  size=" << current_size << "  state=" << state << "  mountpath=" << mountpath << "   fstype=" << fstype;
    return 0;
}

int NodeController::lvremove(const std::string& lvname, const BBUsage_t& usage)
{
    LOG(bb,info) << "Removed logical volume for Uuid '" << lvname << "'";
    return 0;
}

int NodeController::lvupdate(const std::string& lvname, enum LVState state, size_t current_size)
{
    LOG(bb,info) << "Updated logical volume having Uuid '" << lvname << "'  size=" << current_size << "  state=" << state;
    return 0;
}

int NodeController::postRAS(const TSHandler& tsthis)
{
    int l_RC = tsthis.getRC();
    switch (l_RC)
    {
        case 0:
        {
            LOG(bb, info) << "RAS: " << tsthis.get("json");
        }
        break;

        case -2:
        case -107:
        {
            LOG(bb, warning) << "RAS: " << tsthis.get("json");
        }
        break;

        default:
        {
            LOG(bb, error) << "RAS: " << tsthis.get("json");
        }
    }

    return 0;
}


class thread_inout
{
public:
    pthread_t tid;
    string    cmd;
    string    out;
};

void* bbcmd_thread(void* ptr)
{
    thread_inout* ts = (thread_inout*)ptr;
    LOG(bb,info) << "running: " << ts->cmd;
    for(const auto& line : runCommand(ts->cmd))
    {
        LOG(bb,info) << "bbcmd: " << line;
        ts->out += line + "\n";
    }

    return NULL;
}

int NodeController::bbcmd(vector<uint32_t> ranklist,
                          vector<string> nodelist,
                          string executable,
                          vector<string> arguments,
                          boost::property_tree::ptree& output)
{
    int rc = 0;
    int rank_rc;
    string args;
    map<uint32_t, thread_inout> ts;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    for(const auto& arg : arguments)
    {
        if(args != "") args += " ";
        args += arg;
    }

    uint32_t l_nodeListSize=nodelist.size();
    for(const auto& rank: ranklist)
    {
        if (rank > l_nodeListSize-1) {
            output.put(to_string(rank) + ".rc", -1);
            ts[rank].out="rank outof range for hostlist";
            output.put(to_string(rank) + ".error.text", ts[rank].out);
            return -1;
        }
        auto& node = nodelist[rank];

        ts[rank].out = "";
        ts[rank].cmd = string("/usr/bin/ssh -x ") + node + " " + executable + " " + args + " " + "--contribid=" + to_string(rank) + " 2>&1";
    }

    for(const auto& rank: ranklist)
    {
        pthread_create(&ts[rank].tid, &attr, bbcmd_thread, (void*)&ts[rank]);
    }
    for(const auto& rank: ranklist)
    {
        pthread_join(ts[rank].tid, NULL);
    }

    for(const auto& rank: ranklist)
    {
        std::istringstream result_stream(ts[rank].out);

        try
        {
            boost::property_tree::ptree pt;
            boost::property_tree::read_json(result_stream, pt);
            for(auto& e : pt)
            {
                output.boost::property_tree::ptree::put_child(to_string(rank) + "." + e.first, e.second);
            }
        }
        catch(exception& e)
        {
            output.put(to_string(rank) + ".rc", -1);
            output.put(to_string(rank) + ".error.text", ts[rank].out);
        }
        rank_rc = output.get(to_string(rank) + "." + "rc", 0);
        if(rank_rc)
        {
            if(rc == 0)
            {
                output.put("error.firstFailRank", rank);
            }
            rc = rank_rc;
        }
    }

    return rc;
}


int setupNodeController(const std::string& who)
{
#ifdef HAVE_CSMI
    LOG(bb,info) << "Selecting NodeController '" << config.get(who + ".controller", NO_CONFIG_VALUE) << "'";
    if(config.get(who + ".controller", NO_CONFIG_VALUE) == "csm")
    {
        activecontroller = new NodeController_CSM();
    }
#endif
    return 0;
}

TSHandler& operator<<(TSHandler& tsthis, const RAS& ras)
{
    tsthis.add("msgid", ras.getmsgid());
    activecontroller->postRAS(tsthis);
    return tsthis;
};
