/*******************************************************************************
 |    nodecontroller_csm.cc
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

#include "logging.h"
#include <boost/property_tree/json_parser.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <queue>
#include "csm_api_burst_buffer.h"
#include "csm_api_ras.h"
#include "csm_api_common.h"
#include "nodecontroller_csm.h"
#include "util.h"
#include "connections.h"
#include "LVLookup.h"
#include "bbconndata.h"

#if BBPROXY
#include "bbproxy_flightlog.h"
#elif BBSERVER
#include "bbserver_flightlog.h"
#elif BBAPI
#include "bbapi_flightlog.h"
#endif

using namespace std;
using namespace boost::posix_time;

NodeController_CSM::NodeController_CSM()
{
    int rc;
    csmhandle = NULL;
    auto lines = runCommand("grep '^NODE=' /opt/xcat/xcatinfo");
    if(lines.size() != 1)
    {
        throw runtime_error(string("Unable to initialize CSM library"));
    }
    auto toks  = buildTokens(lines[0], "=");
    if(toks.size() != 2)
    {
        throw runtime_error(string("Unable to initialize CSM library"));
    }
    myhostname = toks[1];
    LOG(bb,info) << "xCAT nodename=" << myhostname;

    if(FLCSM)
        FL_Write(FLCSM, CSMInit, "CSM: call csm_init_lib",0,0,0,0);
    rc = csm_init_lib();
    if(FLCSM)
        FL_Write(FLCSM, CSMInitRC, "CSM: call csm_init_lib.  rc=%ld",rc,0,0,0);

    if(rc)
    {
	LOG(bb,error) << "Unable to initialize CSM library.  rc=" << rc;
	throw runtime_error(string("Unable to initialize CSM library"));
    }
#if BBPROXY
    // temporary workaround for beta2

    ssize_t vgfree, vgtotal;
    csm_bb_vg_create_input_t vg;
    csmi_bb_vg_ssd_info_t ssdinfo;
    string nvmssd = getNVMeDeviceInfo(getNVMeByIndex(0), "sn");

    getVGSize(config.get(process_whoami+".volumegroup", "bb"), vgfree, vgtotal);

    ssdinfo.ssd_serial_number = (char*)nvmssd.c_str();
    ssdinfo.ssd_allocation    = vgtotal;

    vg.node_name      = (char*)myhostname.c_str();
    vg.total_size     = vgtotal;
    vg.available_size = vgfree;
    vg.vg_name        = (char*)config.get(process_whoami+".volumegroup", "bb").c_str();
    vg.ssd_info_count = 1;
    vg.ssd_info       = (csmi_bb_vg_ssd_info_t**)malloc(vg.ssd_info_count * sizeof(csmi_bb_vg_ssd_info_t*));
    vg.ssd_info[0]    = &ssdinfo;

    LOG(bb,info) << "Trying to create volume group:  ssd=" << nvmssd << "  vgfree=" << vgfree << "  vgtotal=" << vgtotal;
    rc = csm_bb_vg_create(&csmhandle, &vg);
    LOG(bb,info) << "csm_bb_vg_create() rc=" << rc;

    if(rc)
    {
        LOG(bb,info) << "Volume group create failed with rc=" << rc << " ...  ignoring as might already be created";
    }
    free(vg.ssd_info);

#endif
};

int NodeController_CSM::gethostlist(string& hostlist)
{
    int rc = 0;
    LOG(bb,info) << "NodeController_CSM::gethostlist:  hostlist=" << hostlist;
    const char* allocid = getenv("CSM_ALLOCATION_ID");
    const char* jobid   = getenv("LSF_STAGE_JOBID");
    const char* jobindex= getenv("LSF_STAGE_JOBINDEX");

    csm_allocation_query_input_t input;
    if(jobid && jobindex)
    {
        input.allocation_id    = 0;
        input.primary_job_id   = stol(jobid);
        input.secondary_job_id = stol(jobindex);
    }
    else if(allocid)
    {
        input.allocation_id    = stol(allocid);
        input.primary_job_id   = 0;
        input.secondary_job_id = 0;
    }
    else
    {
        LOG(bb,error) << "NodeController_CSM neither allocationid nor jobid and jobindex was specified";
        return -1;
    }
    csm_allocation_query_output_t* output;

    FL_Write(FLCSM, CSMAllocQuery2, "CSM: call csm_allocation_query(allocid=%ld, jobid=%ld, jobindex=%ld)", input.allocation_id, input.primary_job_id, input.secondary_job_id, 0);
    rc = csm_allocation_query(&csmhandle, &input, &output);
    FL_Write(FLCSM, CSMAllocQuery2RC, "CSM: call csm_allocation_query(allocid=%ld, jobid=%ld, jobindex=%ld)  rc=%ld", input.allocation_id, input.primary_job_id, input.secondary_job_id, rc);
    if(rc)
    {
        LOG(bb,error) << "NodeController_CSM allocation query failed with rc=" << rc;
        return -1;
    }
    if(output->allocation->num_nodes == 0)
    {
        LOG(bb,error) << "CSM: allocation query returned zero compute nodes";
        return -1;
    }

    hostlist = output->allocation->compute_nodes[0];
    for(unsigned int x=1; x<output->allocation->num_nodes; x++)
    {
        hostlist += string(",") + string(output->allocation->compute_nodes[x]);
    }
    return rc;
}

int NodeController_CSM::getAllocationInfo(csmi_allocation_t& alloc)
{
    int rc = 0;
    csmi_allocation_t* tmp = NULL;

    auto jobid = getJobId(bbconnectionName);
    auto job = std::make_pair( (jobid&0xffffffff), (jobid>>32));
    if(job2allocationmap.find(job) == job2allocationmap.end())
    {
        FL_Write(FLCSM, CSMAllocQuery, "CSM: call csm_allocation_query(jobid=%ld)",jobid,0,0,0);

        // FIXME This code was implemented by John Dunham 9/20/2017 as a workaround for the refactor.
        csm_allocation_query_input_t input;
        input.allocation_id    = 0;
        input.primary_job_id   = job.first;
        input.secondary_job_id = job.second;
        csm_allocation_query_output_t *output;

        rc = csm_allocation_query(&csmhandle, &input, &output);

        FL_Write(FLCSM, CSMAllocQueryRC, "CSM: call csm_allocation_query(jobid=%ld)  rc=%ld",jobid,rc,0,0);

        if(rc == 0)
        {
            // FIXME Code added by John Dunham 9/20/2017.
            // Pull out the allocation and then free the wrapper.
            tmp                    = output->allocation;
            output->allocation     = NULL;
            // Frees any allocations from csm_allocation_query.
            csm_api_object_clear(csmhandle);

            job2allocationmap[job] = tmp;

            LOG(bb,info) << "Lookup of jobid(" << job.first << "," << job.second << ") ==> AllocationID " << tmp->allocation_id;
        }
        else
        {
            LOG(bb,info) << "Lookup of jobid(" << job.first << "," << job.second << ") failed";
            return -1;
        }
    }
    else
    {
        tmp = job2allocationmap[job];
    }
    alloc = *tmp;

    return rc;
};

NodeController_CSM::~NodeController_CSM()
{
    int rc;
    if(csmhandle)
    {
        FL_Write(FLCSM, CSMObjDest, "CSM: call csm_api_object_destroy",0,0,0,0);
	csm_api_object_destroy(csmhandle);
        FL_Write(FLCSM, CSMObjDestRC, "CSM: call csm_api_object_destroy (no rc)",0,0,0,0);
    }
    FL_Write(FLCSM, CSMTerm, "CSM: call csm_term_lib",0,0,0,0);
    rc = csm_term_lib();
    FL_Write(FLCSM, CSMTermRC, "CSM: call csm_term_lib.  rc=%ld",rc,0,0,0);
    if(rc)
    {
	LOG(bb,error) << "Unable to shutdown CSM library.  rc=" << rc;
    }
};

int NodeController_CSM::convertState(enum LVState state, char& statechar)
{
    switch(state)
    {
	case LVStateCreated:   statechar = 'C'; break;
	case LVStateMounted:   statechar = 'M'; break;
	case LVStateShrinking: statechar = 'S'; break;
	case LVStateRemoved:   statechar = 'R'; break;
	default:               statechar = '?'; break;
    }
    return 0;
}

int NodeController_CSM::gethostname(std::string& pHostName)
{
    pHostName = myhostname;
    return 0;
}

int NodeController_CSM::lvcreate(const string& lvname, enum LVState state, size_t current_size,
				 const string& mountpath, const string& fstype)
{
    int rc;
    csm_bb_lv_create_input_t bbargs;
    csmi_allocation_t allocinfo;
    char statechar;

    LOG(bb,info) << "Created logical volume for Uuid '" << lvname << "'  size=" << current_size << "  state=" << state << "  mountpath=" << mountpath << "   fstype=" << fstype;

    convertState(state, statechar);
    rc = getAllocationInfo(allocinfo);
    if(rc)
        return rc;

    bbargs.logical_volume_name = (char*)lvname.c_str();
    bbargs.node_name           = (char*)myhostname.c_str();
    bbargs.allocation_id       = allocinfo.allocation_id;
    bbargs.vg_name             = (char*)config.get(process_whoami+".volumegroup", "bb").c_str();
    bbargs.state               = statechar;
    bbargs.current_size        = current_size;
    bbargs.file_system_mount   = (char*)mountpath.c_str();
    bbargs.file_system_type    = (char*)fstype.c_str();
    FL_Write(FLCSM, CSMLVCreate, "CSM: call csm_bb_lv_create.  AllocID=%ld, Size=%ld",allocinfo.allocation_id, current_size,0,0);
    rc = csm_bb_lv_create(&csmhandle, &bbargs);
    FL_Write(FLCSM, CSMLVCreateRC, "CSM: call csm_bb_lv_create.  AllocID=%ld, Size=%ld.  rc=%ld",allocinfo.allocation_id, current_size,rc,0);
    if(rc)
    {
	LOG(bb,error) << "Error posting csm_bb_lv_create().  rc=" << rc;
	return -1;
    }

    return 0;
}

int NodeController_CSM::lvremove(const string& lvname, const BBUsage_t& usage)
{
    int rc;
    csm_bb_lv_delete_input_t bbargs;
    csmi_allocation_t allocinfo;

    LOG(bb,info) << "Removed logical volume for Uuid '" << lvname << "'";
    rc = getAllocationInfo(allocinfo);
    if(rc)
        return rc;

    bbargs.logical_volume_name = (char*)lvname.c_str();
    bbargs.allocation_id       = allocinfo.allocation_id;
    bbargs.node_name           = (char*)myhostname.c_str();
    bbargs.num_bytes_read      = usage.totalBytesRead;
    bbargs.num_bytes_written   = usage.totalBytesWritten;

    FL_Write(FLCSM, CSMLVDelete, "CSM: call csm_bb_lv_delete.  AllocID=%ld",allocinfo.allocation_id, 0,0,0);
    rc = csm_bb_lv_delete(&csmhandle, &bbargs);
    FL_Write(FLCSM, CSMLVDeleteRC, "CSM: call csm_bb_lv_delete.  AllocID=%ld.  rc=%ld",allocinfo.allocation_id,rc,0,0);
    if(rc)
    {
	LOG(bb,error) << "Error posting csm_bb_lv_delete().  rc=" << rc;
	return -1;
    }
    return 0;
}

int NodeController_CSM::lvupdate(const string& lvname, enum LVState state, size_t current_size)
{
    int rc;
    char statechar;
    csm_bb_lv_update_input_t bbargs;
    csmi_allocation_t allocinfo;

    LOG(bb,info) << "Updated logical volume having Uuid '" << lvname << "'  size=" << current_size << "  state=" << state;

    convertState(state, statechar);
    rc = getAllocationInfo(allocinfo);
    if(rc)
        return rc;

    bbargs.logical_volume_name = (char*)lvname.c_str();
    bbargs.allocation_id       = allocinfo.allocation_id;
    bbargs.state               = statechar;
    bbargs.node_name           = (char*)myhostname.c_str();
    bbargs.current_size        = current_size;
    FL_Write(FLCSM, CSMLVUpdate, "CSM: call csm_bb_lv_update.  AllocID=%ld, Size=%ld",allocinfo.allocation_id, current_size,0,0);
    rc = csm_bb_lv_update(&csmhandle, &bbargs);
    FL_Write(FLCSM, CSMLVUpdateRC, "CSM: call csm_bb_lv_update.  AllocID=%ld, Size=%ld.  rc=%ld",allocinfo.allocation_id, current_size,rc,0);
    if(rc)
    {
	LOG(bb,error) << "Error posting csm_bb_lv_update().  rc=" << rc;
	return -1;
    }
    return 0;
}

int NodeController_CSM::postRAS(const TSHandler& tsthis)
{
    int rc;
    string csvparms;
    list<pair<string,string> > keyvalues;
    boost::property_tree::ptree pt;

    LOG(bb,error) << "RAS: " << tsthis.get("json");

    tsthis.get(keyvalues);
    tsthis.get(pt);
    for(auto& kv : keyvalues)
    {
        if(csvparms != "")
            csvparms += ",";
        csvparms += kv.first + "=" + kv.second;
    }

    ptime current_time = microsec_clock::universal_time();

    LOG(bb,always) << "RAS: msg_id   =" << pt.get("msgid", "unknown");
    LOG(bb,always) << "RAS: timestamp=" << to_iso_extended_string(current_time) + "Z";
    LOG(bb,always) << "RAS: hostname =" << myhostname;
    LOG(bb,always) << "RAS: kvcsv    =" << csvparms;

    FL_Write(FLCSM, CSMPostRAS, "CSM: call csm_ras_event_create",0,0,0,0);
    rc = csm_ras_event_create(&csmhandle,
                              pt.get("msgid", "bb.unknown").c_str(),
                              (to_iso_extended_string(current_time) + "Z").c_str(),
                              myhostname.c_str(),
                              NULL,
                              csvparms.c_str());
    FL_Write(FLCSM, CSMPostRASRC, "CSM: call csm_ras_event_create.  rc=%ld",rc,0,0,0);
    if(rc)
    {
        LOG(bb,always) << "csm_ras_event_create failed.  rc=" << rc;
    }

    return 0;
}

int NodeController_CSM::bbcmd(std::vector<std::uint32_t> ranklist,
                              std::vector<std::string> nodelist,
                              std::string executable,
                              std::vector<std::string> arguments,
                              boost::property_tree::ptree& output,
                              bool nodebcast)
{
    int rc;
    csm_bb_cmd_input_t   in;
    csm_bb_cmd_output_t* out = NULL;
    string args;
    boost::property_tree::ptree pt;
    queue<uint32_t> ranklistqueue;
    vector<boost::property_tree::ptree> results;
    string cuml_rank = "-1";
    int    cuml_rankrc = 0;
    string exception_text = "";

    unsigned int MAXNODESPERCSMCALL = 32;
    if(nodebcast)
    {
        MAXNODESPERCSMCALL = nodelist.size();
    }

    const char** nodenames = (const char**)malloc(sizeof(const char*) * MAXNODESPERCSMCALL);
    if(nodenames == NULL)
    {
        throw runtime_error(string("malloc returned NULL"));
    }

    for(const auto& rank: ranklist)
    {
        ranklistqueue.push(rank);
    }

    while(ranklistqueue.size() > 0)
    {
        list<string> tmparguments;
        list<uint32_t> csmbbranklist;
        list<string> host2rank;

        tmparguments.clear();
        in.node_names_count = 0;

        map<string, bool> inarray;
        inarray.clear();

        do
        {
            auto rank = ranklistqueue.front();
            ranklistqueue.pop();

            if(rank < nodelist.size())
            {
                csmbbranklist.push_back(rank);

                if(inarray[nodelist[rank]] == false)
                {
                    inarray[nodelist[rank]] = true;
                    nodenames[in.node_names_count++] = nodelist[rank].c_str();
                }
                host2rank.push_back(nodelist[rank] + "=" + to_string(rank));
                tmparguments.push_back(nodelist[rank] + ":" + to_string(rank));
            }
        }
        while((in.node_names_count < MAXNODESPERCSMCALL) && (ranklistqueue.size() > 0));

        args = "";
        for(const auto& arg : arguments)
        {
            if(args != "") args += "^";
            args += arg;
        }
        if(nodebcast)
        {
            args += "^--csmcommand=localhost:0";
        }
        else
        {
            args += "^--csmcommand=";
            
            bool first = true;
            for(const auto& arg : tmparguments)
            {
                if(first) first = false;
                else      args += ",";
                
                args += arg;
            }
        }

        in.command_arguments = (char*)args.c_str();
        in.node_names        = (char**)nodenames;
        
        LOG(bb,info) << "csm_bb_cmd:  arguments=" << args;
        for(uint32_t x=0; x<in.node_names_count; x++)
        {
            LOG(bb,info) << "csm_bb_cmd:  node[" << x << "]=" << in.node_names[x];
        }

        FL_Write(FLCSM, CSMBBCMD, "CSM: call csm_bb_cmd to %ld compute nodes",in.node_names_count,0,0,0);
        rc = csm_bb_cmd(&csmhandle, &in, &out);
        FL_Write(FLCSM, CSMBBCMDRC, "CSM: call csm_bb_cmd.  rc=%ld",rc,0,0,0);
        
        LOG(bb,info) << "csm_bb_cmd return code=" << rc;
        if(rc == 0)
        {
            LOG(bb,info) << "csm_bb_output: " << out->command_output << "#";
            try
            {
                for(const auto& line : buildTokens(out->command_output, "\n"))
                {
                    size_t off = line.find(" : ");
                    if(off != string::npos)
                    {
                        istringstream resultstream(string("{") + line.substr(off+3) + string("}"));
                        results.emplace_back();
                        boost::property_tree::read_json(resultstream, results.back());
                    }
                }
            }
            catch(exception& e)
            {
                rc = -1;
                exception_text = e.what();
            }
        }
        if(rc)
        {
            for(const auto& rank: csmbbranklist)
            {
                results.emplace_back();
                results.back().put(to_string(rank) + ".rc", rc);
                results.back().put(to_string(rank) + ".error.text", "csm_bb_cmd failure");
            }
        }
    }
    free(nodenames);

    try
    {
        int    index = 0;
        int    rank_rc;
        for (auto &e : results)
        {
            string rank = e.front().first;
            LOG(bb,info) << "processing results from rank " << rank;
            if (nodebcast)
                rank = to_string(ranklist[index++]);
            output.boost::property_tree::ptree::put_child(rank, e.front().second);
            rank_rc = e.get(e.front().first + ".rc", 0);
            if ((cuml_rankrc == 0) && (rank_rc != 0))
            {
                cuml_rankrc = rank_rc;
                cuml_rank   = rank;
            }
        }
    }
    catch(exception& e)
    {
        cuml_rankrc = -1;
        exception_text = e.what();
    }
    if(cuml_rank != "-1")
    {
        output.put("error.firstFailRank", stoi(cuml_rank));
    }
    if(exception_text.size() > 0)
    {
        output.put("error.exception", exception_text);
    }
    if(cuml_rankrc)
    {
        output.put("error.command", args);
    }
    output.put("rc", cuml_rankrc);

    return 0;
};
