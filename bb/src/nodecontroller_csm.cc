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
#include <algorithm>
#include "nodecontroller_csm.h"
#include "util.h"
#include "connections.h"
#include "LVLookup.h"
#include "bbconndata.h"
#include "bbGrabStderr.h"

#if BBPROXY
#include "bbproxy_flightlog.h"
#elif BBSERVER
#include "bbserver_flightlog.h"
#elif BBAPI
#include "bbapi_flightlog.h"
#endif

using namespace std;
using namespace boost::posix_time;

static void handleSymError(const char * symbol){
LOG(bb,error) << "dlsym failed for string="<<symbol;
    char * tempstr=dlerror();
    if (tempstr) LOG(bb,error) << "dlerror()="<<tempstr;
    abort();
}

int NodeController_CSM::getcsmSymbols(const std::string& controllerPath){
    void * _dlopen_csmi = dlopen(controllerPath.c_str(), RTLD_LAZY);
    if (_dlopen_csmi){
        dlerror(); //clear error
        _csm_init_lib_vers_func = (csm_init_lib_vers_t)dlsym(_dlopen_csmi,"csm_init_lib_vers");
        if(!_csm_init_lib_vers_func)handleSymError("csm_init_lib_vers");
        dlerror();
        _csm_term_lib_func = (csm_term_lib_t )dlsym(_dlopen_csmi,"csm_term_lib");
        if(!_csm_term_lib_func)handleSymError("csm_term_lib");
        dlerror();       
        _csm_api_object_clear_func = (csm_api_object_clear_t)dlsym(_dlopen_csmi,"csm_api_object_clear");
        if(!_csm_api_object_clear_func)handleSymError("csm_api_object_clear");
        dlerror();
        _csm_api_object_destroy_func = (csm_api_object_destroy_t)dlsym(_dlopen_csmi,"csm_api_object_destroy");
        if(!_csm_api_object_destroy_func)handleSymError("csm_api_object_destroy");
        dlerror();
        _csm_bb_vg_create_func = (csm_bb_vg_create_t      )dlsym(_dlopen_csmi,"csm_bb_vg_create");
        if(!_csm_bb_vg_create_func)handleSymError("csm_bb_vg_create");
        dlerror();
        _csm_bb_lv_create_func =  (csm_bb_lv_create_t       )dlsym(_dlopen_csmi,"csm_bb_lv_create");
        if(!_csm_bb_lv_create_func)handleSymError("csm_bb_lv_create");
        dlerror();
        _csm_bb_lv_delete_func = (csm_bb_lv_delete_t       )dlsym(_dlopen_csmi,"csm_bb_lv_delete");
        if(!_csm_bb_lv_delete_func)handleSymError("csm_bb_lv_delete");
        dlerror();
        _csm_bb_lv_update_func = (csm_bb_lv_update_t       )dlsym(_dlopen_csmi,"csm_bb_lv_update");
        if(!_csm_bb_lv_update_func)handleSymError("csm_bb_lv_update");
        dlerror();
        _csm_bb_cmd_func = (csm_bb_cmd_t             )dlsym(_dlopen_csmi,"csm_bb_cmd");
        if(!_csm_bb_cmd_func)handleSymError("csm_bb_cmd");
        dlerror();
        _csm_ras_event_create_func = (csm_ras_event_create_t   )dlsym(_dlopen_csmi,"csm_ras_event_create");
        if(!_csm_ras_event_create_func)handleSymError("csm_ras_event_create");
        dlerror();
        _csm_allocation_query_func = (csm_allocation_query_t)dlsym(_dlopen_csmi,"csm_allocation_query");
        if(!_csm_allocation_query_func)handleSymError("csm_allocation_query");
        dlerror();
        _csm_allocation_query_active_all_func = (csm_allocation_query_active_all_t)dlsym(_dlopen_csmi,"csm_allocation_query_active_all");
        if(!_csm_allocation_query_active_all_func)handleSymError("csm_allocation_query_active_all");
    }
    else{
        LOG(bb,error) << "dlopen failed for controller path="<<controllerPath;
        abort();
    }
    return 0;

}


NodeController_CSM::NodeController_CSM():
    _csm_init_lib_vers_func(NULL),       
    _csm_term_lib_func(NULL),            
    _csm_api_object_clear_func(NULL),    
    _csm_api_object_destroy_func(NULL),  
    _csm_bb_vg_create_func(NULL),        
    _csm_bb_lv_create_func(NULL),        
    _csm_bb_lv_delete_func(NULL),        
    _csm_bb_lv_update_func(NULL),        
    _csm_bb_cmd_func(NULL),              
    _csm_ras_event_create_func(NULL), 
    _csm_allocation_query_func(NULL),
    _csm_allocation_query_active_all_func(NULL),
    csmhandle(NULL)
{
    //csm_allocation_query_t   _csm_allocation_query_func;
    //csm_allocation_query_active_all_t _csm_allocation_query_active_all_func;
    //void *dlopen(const char *filename, RTLD_LAZY);
    // filename /opt/ibm/csm/lib/libcsmi.so
    int rc;
    string controllerPath = config.get("bb.controllerpath", "/opt/ibm/csm/lib/libcsmi.so");
    getcsmSymbols(controllerPath);
    

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
    rc = _csm_init_lib_vers_func(CSM_VERSION_ID);
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
    vg.scheduler      = true;
    vg.ssd_info_count = 1;
    vg.ssd_info       = (csmi_bb_vg_ssd_info_t**)malloc(vg.ssd_info_count * sizeof(csmi_bb_vg_ssd_info_t*));
    vg.ssd_info[0]    = &ssdinfo;

    LOG(bb,info) << "Trying to create volume group:  ssd=" << nvmssd << "  vgfree=" << vgfree << "  vgtotal=" << vgtotal;
    rc = _csm_bb_vg_create_func(&csmhandle, &vg);
    LOG(bb,info) << "csm_bb_vg_create() rc=" << rc;

    if(rc)
    {
        LOG(bb,info) << "Volume group create failed with rc=" << rc << " ...  ignoring as might already be created";
    }
    free(vg.ssd_info);

#endif
};

static bool hostnamecmp(const std::string& s1, const std::string& s2)
{
    int result = strverscmp(s1.c_str(), s2.c_str());
    if(result < 0)
        return true;
    return false;
}


int NodeController_CSM::gethostlist(string& hostlist)
{
    int rc = 0;
    LOG(bb,info) << "NodeController_CSM::gethostlist:  hostlist=" << hostlist;
    const char* allocid_str = getenv("CSM_ALLOCATION_ID");
    const char* jobid_str   = getenv("LSF_STAGE_JOBID");
    const char* jobindex_str= getenv("LSF_STAGE_JOBINDEX");
    int64_t allocid  = 0;
    int64_t jobid    = 0;
    int64_t jobindex = 0;
    if(allocid_str)  allocid  = stol(allocid_str);
    if(jobid_str)    jobid    = stol(jobid_str);
    if(jobindex_str) jobindex = stol(jobindex_str);

    csm_allocation_query_active_all_input_t input;
    csm_allocation_query_active_all_output_t* output;
    input.limit  = 100;
    input.offset = 0;
    bool found   = false;
    while(!found)
    {
        output = NULL;
        FL_Write(FLCSM, CSMAllocQuery2, "CSM: call csm_allocation_query_active_all(limit=%ld, offset=%ld)", input.limit, input.offset, 0, 0);
        rc = (_csm_allocation_query_active_all_func)(&csmhandle, &input, &output);
        FL_Write(FLCSM, CSMAllocQuery2RC, "CSM: call csm_allocation_query_active_all(limit=%ld, offset=%ld)  rc=%ld", input.limit, input.offset, rc, 0);
        if(rc != 0)
            break;
        for(uint32_t x=0; x<output->num_allocations; x++)
        {
            if((output->allocations[x]->allocation_id == allocid) ||
                ((output->allocations[x]->primary_job_id == jobid) &&
                (output->allocations[x]->secondary_job_id == jobindex)))
                {
                    if(output->allocations[x]->num_nodes == 0)
                    {
                        LOG(bb,error) << "CSM: allocation query returned zero compute nodes";
                        bberror << err("err.csmerror","CSM: allocation query returned zero compute nodes");
                        return -1;
                    }

                    vector<string> hostvector;
                    for(unsigned int y=0; y<output->allocations[x]->num_nodes; y++)
                    {
                        hostvector.push_back(string(output->allocations[x]->compute_nodes[y]));
                    }
                    sort(hostvector.begin(), hostvector.end(), hostnamecmp);
                    hostlist = "";
                    for(const auto& ahost: hostvector)
                    {
                        if(hostlist.size() > 0)
                        {
                            hostlist += string(",");
                        }
                        hostlist += ahost;
                    }
                    found = true;
                    break;
                }
        }
        if(output->num_allocations != (uint32_t)input.limit)
            break;
        //csm_free_struct_ptr(csm_allocation_query_active_all_output_t, output);
        free(output);
        input.offset += input.limit;
    }
    if(!found)
        rc = -1;
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

        rc = (_csm_allocation_query_func)(&csmhandle, &input, &output);

        FL_Write(FLCSM, CSMAllocQueryRC, "CSM: call csm_allocation_query(jobid=%ld)  rc=%ld",jobid,rc,0,0);

        if(rc == 0)
        {
            // FIXME Code added by John Dunham 9/20/2017.
            // Pull out the allocation and then free the wrapper.
            tmp                    = output->allocation;
            output->allocation     = NULL;
            // Frees any allocations from csm_allocation_query.
            _csm_api_object_clear_func(csmhandle);

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
    	_csm_api_object_destroy_func(csmhandle);
        FL_Write(FLCSM, CSMObjDestRC, "CSM: call csm_api_object_destroy (no rc)",0,0,0,0);
    }
    FL_Write(FLCSM, CSMTerm, "CSM: call csm_term_lib",0,0,0,0);
    rc = _csm_term_lib_func();
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
    rc = _csm_bb_lv_create_func(&csmhandle, &bbargs);
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
#if BBUSAGE_COUNT
    bbargs.num_writes          = usage.localWriteCount;
    bbargs.num_reads           = usage.localReadCount;
#endif
    FL_Write(FLCSM, CSMLVDelete, "CSM: call csm_bb_lv_delete.  AllocID=%ld",allocinfo.allocation_id, 0,0,0);
    rc = _csm_bb_lv_delete_func(&csmhandle, &bbargs);
    FL_Write(FLCSM, CSMLVDeleteRC, "CSM: call csm_bb_lv_delete.  AllocID=%ld.  rc=%ld",allocinfo.allocation_id,rc,0,0);
    if(rc)
    {
	  LOG(bb,error) << "Error posting csm_bb_lv_delete().  rc=" << rc;
      bberror << err("err.csmerror", "Error posting csm_bb_lv_delete()")<<err("err.csmrc",rc);
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
    rc = _csm_bb_lv_update_func(&csmhandle, &bbargs);
    FL_Write(FLCSM, CSMLVUpdateRC, "CSM: call csm_bb_lv_update.  AllocID=%ld, Size=%ld.  rc=%ld",allocinfo.allocation_id, current_size,rc,0);
    if(rc)
    {
	  LOG(bb,error) << "Error posting csm_bb_lv_update().  rc=" << rc;
      bberror << err("err.csmerror", "Error posting csm_bb_lv_update()")<<err("err.csmrc",rc);
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
    rc = _csm_ras_event_create_func(&csmhandle,
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
    vector<std::string> results_host;
    string cuml_rank = "-1";
    int    cuml_rankrc = 0;
    string cuml_errortext;
    boost::property_tree::ptree cuml_badresult;
    string exception_text = "";
    unsigned successcount = 0;
    map<std::string, uint32_t> bcast_host2rank_map;

    unsigned int MAXNODESPERCSMCALL = 32;
    if(nodebcast)
    {
        MAXNODESPERCSMCALL = nodelist.size();
        int count = 0;
        for(const auto& node : nodelist)
        {
            bcast_host2rank_map[node] = count++;
        }
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

        GrabStderr grabstderr;
        FL_Write(FLCSM, CSMBBCMD, "CSM: call csm_bb_cmd to %ld compute nodes",in.node_names_count,0,0,0);
        rc = _csm_bb_cmd_func(&csmhandle, &in, &out);
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
                        if(results.back().empty())
                        {
                            results.pop_back();
                            rc = -1;
                            cuml_errortext = string("no data from node");
                        }
                        else
                        {
                            size_t beginoff = 0;
                            if(line.find(";") == 0) beginoff = 1;

                            std::string tmphost = line.substr(beginoff,off - beginoff); 
                            results_host.push_back( tmphost );
                        }
                        
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
            const size_t l_BUFSIZE = 2048;
            char l_buffer[l_BUFSIZE];
            int grabRC = grabstderr.getStdErrBuffer(l_buffer,l_BUFSIZE);
            if (grabRC > 0)
            {
                output.put("error.csm_stderr", l_buffer);
            }
            else
            {
                output.put("error.csm_stderrgrabrc", grabRC);
            }
            string hostlist = in.node_names[0];
            for(uint32_t x=1; x<in.node_names_count; x++)
            {
                hostlist += string(",") + in.node_names[x];
            }

            output.put("error.csm_hostlist", hostlist);
            output.put("error.csm_rc", rc);
            cuml_rankrc = -1;
        }
    }
    free(nodenames);

    try
    {
        int    index = -1;
        int    rank_rc;
        for (const auto &e : results)
        {
            index++;
            if(e.empty())
                continue;
            
            string rank = e.front().first;
            if(nodebcast)
                rank = to_string(bcast_host2rank_map[results_host[index]]);
            LOG(bb,info) << "Processing results from rank " << rank;

            rank_rc = e.get(e.front().first + ".rc", 0);
            if(rank_rc == 0)
                successcount++;
            
            if ((cuml_rankrc == 0) && (rank_rc != 0))
            {
                cuml_rankrc = rank_rc;
                cuml_rank   = rank;
                cuml_badresult = e;
                cuml_errortext = e.get(e.front().first + ".error.text", "");
            }
            if(!nodebcast)
            {
                output.boost::property_tree::ptree::put_child(rank, e.front().second);
            }
        }
        if(nodebcast) // include summary output only
        {
            if(cuml_rankrc == 0)
            {
                output.boost::property_tree::ptree::put_child(results[0].front().first, results[0].front().second); // pick first (good) result
            }
            else if(cuml_badresult.size() > 0)
            {
                output.boost::property_tree::ptree::put_child(cuml_rank, cuml_badresult.front().second); // pick first bad result
            }
            else
            {
                LOG(bb,info) << "bbcmd failure not specific to a node.  rc=" << cuml_rankrc;
            }
        }
    }
    catch(exception& e)
    {
        cuml_rankrc = -1;
        exception_text = e.what();
    }
    output.put("goodcount", successcount);                      // good rc
    output.put("failcount", results.size() - successcount);     // bad rc
    output.put("voidcount", ranklist.size() - results.size());  // response not received
    if(cuml_rank != "-1")
    {
        output.put("error.firstFailRank", stoi(cuml_rank));
        output.put("error.firstFailNode", nodelist[stoi(cuml_rank)]);
    }
    if(exception_text.size() > 0)
    {
        output.put("error.exception", exception_text);
    }
    if(cuml_rankrc)
    {
        output.put("error.command", args);
    }
    if(cuml_errortext.size() > 0)
    {
        output.put("error.text", cuml_errortext);
    }
    output.put("rc", cuml_rankrc);

    return 0;
};
