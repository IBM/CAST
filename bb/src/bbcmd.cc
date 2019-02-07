/*******************************************************************************
 |    bbcmd.cc
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

#include <iostream>
#include <list>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wordexp.h>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/make_shared.hpp>
#include <boost/program_options.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/shared_ptr.hpp>

using namespace std;
using namespace boost;

namespace pt = boost::property_tree;

#include "bbapi.h"
#include "bbapiAdmin.h"
#include "bbapi2.h"
#include "bbinternal.h"
#include "BBJob.h"
#include "logging.h"
#include "nodecontroller.h"

#define NAME "bbcmd"

namespace pt = boost::property_tree;
namespace po = boost::program_options;

typedef int(*CommandHandler_t)(po::variables_map&);
typedef struct
{
    CommandHandler_t func;
    list<string> valid_options;
    uint8_t level;
} CommandData_t;

extern int bbcmd_mkdir(po::variables_map& vm);
extern int bbcmd_rmdir(po::variables_map& vm);
extern int bbcmd_chown(po::variables_map& vm);
extern int bbcmd_chmod(po::variables_map& vm);
extern int bbcmd_copy(po::variables_map& vm);
extern int bbcmd_cancel(po::variables_map& vm);
extern int bbcmd_gethandle(po::variables_map& vm);
extern int bbcmd_getstatus(po::variables_map& vm);
extern int bbcmd_gettransferkeys(po::variables_map& vm);
extern int bbcmd_gettransfers(po::variables_map& vm);
extern int bbcmd_getusage(po::variables_map& vm);
extern int bbcmd_setusagelimit(po::variables_map& vm);
extern int bbcmd_getdeviceusage(po::variables_map& vm);
extern int bbcmd_getthrottle(po::variables_map& vm);
extern int bbcmd_setthrottle(po::variables_map& vm);
extern int bbcmd_create(po::variables_map& vm);
extern int bbcmd_remove(po::variables_map& vm);
extern int bbcmd_removejobinfo(po::variables_map& vm);
extern int bbcmd_resize(po::variables_map& vm);
extern int bbcmd_sleep(po::variables_map& vm);
extern int bbcmd_suspend(po::variables_map& vm);
extern int bbcmd_resume(po::variables_map& vm);
extern int coral_cmd_getvar(po::variables_map& vm);
extern int coral_cmd_setvar(po::variables_map& vm);
extern int coral_cmd_stageout_start(po::variables_map& vm);
extern int bbcmd_getserver(po::variables_map& vm);
extern int bbcmd_setserver(po::variables_map& vm);
extern int bbcmd_adminfailover(po::variables_map& vm);


map<string, CommandData_t> bbcmd_map =
{
    { "mkdir",              { bbcmd_mkdir,                  list<string>    {"path"},                                                    0 }},
    { "chown",              { bbcmd_chown,                  list<string>    {"path", "group", "user"},                                   0 }},
    { "chmod",              { bbcmd_chmod,                  list<string>    {"mode", "path"},                                            0 }},
    { "copy",               { bbcmd_copy,                   list<string>    {"filelist", "handle", "tag", "contrib"},                    0 }},
    { "cancel",             { bbcmd_cancel,                 list<string>    {"handle", "scope"},                                         0 }},
    { "gethandle",          { bbcmd_gethandle,              list<string>    {"contrib", "tag"},                                          0 }},
    { "getstatus",          { bbcmd_getstatus,              list<string>    {"handle"},                                                  0 }},
    { "gettransfers",       { bbcmd_gettransfers,           list<string>    {"matchstatus", "numhandles"},                               0 }},
    { "gettransferkeys",    { bbcmd_gettransferkeys,        list<string>    {"handle", "buffersize"},                                    0 }},
    { "getusage",           { bbcmd_getusage,               list<string>    {"mount"},                                                   0 }},
    { "setusagelimit",      { bbcmd_setusagelimit,          list<string>    {"mount", "rl", "wl"},                                       0 }},
    { "getdeviceusage",     { bbcmd_getdeviceusage,         list<string>    {"device"},                                                  0 }},
    { "getthrottle",        { bbcmd_getthrottle,            list<string>    {"mount"},                                                   0 }},
    { "setthrottle",        { bbcmd_setthrottle,            list<string>    {"mount", "rate"},                                           0 }},
    { "create",             { bbcmd_create,                 list<string>    {"coptions", "mount", "size"},                               0 }},
    { "remove",             { bbcmd_remove,                 list<string>    {"mount"},                                                   0 }},
    { "removejobinfo",      { bbcmd_removejobinfo,          list<string>    { },                                                         0 }},
    { "resize",             { bbcmd_resize,                 list<string>    {"mount", "roptions", "size"},                               0 }},
    { "rmdir",              { bbcmd_rmdir,                  list<string>    {"path"},                                                    0 }},
    { "sleep",              { bbcmd_sleep,                  list<string>    {"delay"},                                                   0 }},
    { "getserver",          { bbcmd_getserver,              list<string>    {"connected","waitforreplycount"},                           0 }},
    { "setserver",          { bbcmd_setserver,              list<string>    {"close","open","activate","offline"},                       0 }},
    { "suspend",            { bbcmd_suspend,                list<string>    {"hostname"},                                                0 }},
    { "resume",             { bbcmd_resume,                 list<string>    {"hostname"},                                                0 }},
    { "adminfailover",      { bbcmd_adminfailover,          list<string>    {"hostname", "resume" },                                     0 }},
    { "getvar",             { coral_cmd_getvar,             list<string>    {"variable"},                                                1 }},
    { "setvar",             { coral_cmd_setvar,             list<string>    {"value", "variable"},                                       1 }},
    { "stgout_start",       { coral_cmd_stageout_start,     list<string>    {"mount"},                                                   1 }}
};

extern uint64_t BBJOBID;

#define VMEXISTS(name) if(vm.count(name) == 0) { LOG(bb,error) << "Missing parameter '" << name << "'"; bberror << err("error.text", "Missing parameter") << err("error.parm", name) << err("rc", -1); return -1; }
#define VMMUTEXCL(name1,name2) if((vm.count(name1) != 0) && (vm.count(name2) != 0)) { LOG(bb,error) << "Mutually exclusive parameters '" << name1 << "' and '" << name2 << "'"; bberror << err("error.text", "Mutually exclusive parameters") << err("rc", -1) << err("error.parm1", name1) << err("error.parm2", name2); return -1; }
#define DEFAULT_HOSTLIST "default_hostlist"

// Helper routines...

int expandRangeString(string rangestr, uint32_t min, uint32_t max, vector<uint32_t>& ranks)
{
    for(auto& e : buildTokens(rangestr, ","))
    {
        if(e.find("-") != string::npos)
        {
            uint32_t lower, upper;
            auto startend = buildTokens(e, "-");
            if(startend.size() > 2)
                throw(runtime_error(string("range parameter error calculating startend '") + e + string("'")));
            lower = min;
            upper = max;

            if(startend[0].size() != 0)
                lower = MAX(lower, stoul(startend[0]));
            if((startend.size() > 1) && (startend[1].size() != 0))
                upper = MIN(upper, stoul(startend[1]));

            for(auto x = lower; x<=upper; x++)
            {
                ranks.push_back(x);
            }
        }
        else
        {
            ranks.push_back(stoul(e));
        }
    }
    return 0;
}


void usageWithoutCommand(const char* pProgram, const po::variables_map& pVariables, const po::options_description& pOptions)
{
    if (pVariables.count("help") || pVariables.count("admin"))
    {
        if (pVariables.count("help"))
        {
            cout << "bbcmd usage:" << endl;
            cout << "\t" << pProgram << " <command> <options>\n" << endl;
            cout << "Valid commands:" << endl;
            for(auto cmd : bbcmd_map)
            {
                if (cmd.second.level == 0)
                    cout << "\t" << cmd.first << endl;
            }
        }
        if (pVariables.count("admin"))
        {
            if (!pVariables.count("help"))
            {
                cout << "bbcmd admin usage:" << endl;
                cout << "\t" << pProgram << " <admin command> <options>\n" << endl;
            }
            else
            {
                cout << endl;
            }
            cout << "Valid admin commands:" << endl;
            for(auto cmd : bbcmd_map)
            {
                if (cmd.second.level == 1)
                    cout << "\t" << cmd.first << endl;
            }
        }
    }
    else
    {
        cout << "bbcmd usage:" << endl;
        cout << "\t" << pProgram << " <command> <options>\n" << endl;
        cout << "Valid commands:" << endl;
        for(auto cmd : bbcmd_map)
        {
            if (cmd.second.level == 0)
                cout << "\t" << cmd.first << endl;
        }
    }
    cout << "\n" << pOptions << endl;

    return;
}

// Admin/service commands
int coral_cmd_getvar(po::variables_map& vm)
{
    VMEXISTS("variable");
    return Coral_GetVar(vm["variable"].as<string>().c_str());
}

int coral_cmd_setvar(po::variables_map& vm)
{
    std::string l_Value = "";
    VMEXISTS("variable");

    if(vm.count("value") > 0)
    {
        l_Value = (vm["value"].as<string>());
    }

    return Coral_SetVar(vm["variable"].as<string>().c_str(), l_Value.c_str());
}

int coral_cmd_stageout_start(po::variables_map& vm)
{
    VMEXISTS("mount");
    return Coral_StageOutStart(vm["mount"].as<string>().c_str());
}


// End-user commands
int bbcmd_mkdir(po::variables_map& vm)
{
    VMEXISTS("path");
    return BB_CreateDirectory(vm["path"].as<string>().c_str());
}

int bbcmd_rmdir(po::variables_map& vm)
{
    VMEXISTS("path");
    return BB_RemoveDirectory(vm["path"].as<string>().c_str());
}

int bbcmd_sleep(po::variables_map& vm)
{
    int rc = 0;
    int delay = 0;
    if(vm.count("delay") > 0)
    {
        delay = vm["delay"].as<int>();
    }
    sleep(delay);

    return rc;
}

int bbcmd_chown(po::variables_map& vm)
{
    VMEXISTS("path");
    VMEXISTS("user");
    VMEXISTS("group");

    return BB_ChangeOwner(vm["path"].as<string>().c_str(), vm["user"].as<string>().c_str(), vm["group"].as<string>().c_str());
}

int bbcmd_chmod(po::variables_map& vm)
{
    VMEXISTS("path");
    VMEXISTS("mode");

    mode_t mode = stol(vm["mode"].as<string>(), NULL, 8);

    return BB_ChangeMode(vm["path"].as<string>().c_str(), mode);
}

int bbcmd_copy(po::variables_map& vm)
{
    int rc=0;
    stringstream errorText;

    VMEXISTS("filelist");
    if((vm.count("handle") == 0) && ((vm.count("tag") == 0) || (vm.count("contrib") == 0)))
    {
        VMEXISTS("handle");
    }
    VMMUTEXCL("handle", "tag");
    VMMUTEXCL("handle", "contrib");
    
    BBTransferHandle_t l_Handle;
    if(vm.count("handle") > 0)
    {
        l_Handle = (BBTransferHandle_t)stoull(vm["handle"].as<string>());
    }
    else
    {
        uint64_t l_NumContribs = 0;
        uint32_t* l_Contrib = 0;
        
        if(vm.count("contrib") > 0)
        {
            vector<uint32_t> contribvec;
            expandRangeString(vm["contrib"].as<string>(), 0, MAX_NUMBER_OF_CONTRIBS, contribvec);
            
            l_NumContribs = contribvec.size();
            l_Contrib = (uint32_t*)(new char[sizeof(uint32_t)*l_NumContribs]);
            uint32_t i = 0;
            for(const auto& e : contribvec)
            {
                l_Contrib[i++] = e;
            }
        }
        
        rc = BB_GetTransferHandle((BBTAG)(vm["tag"].as<int>()), l_NumContribs, l_Contrib, &l_Handle);
        
        if (l_Contrib)
        {
            delete[] l_Contrib;
            l_Contrib = 0;
        }
        
        if(rc)
            return rc;
        
        bberror.errdirect("out.transferHandle", l_Handle);
    }
    
    BBTransferDef_t* l_Transfer = NULL;
    rc = BB_CreateTransferDef(&l_Transfer);
    if (rc)
    {
        return rc;
    }

    if(vm.count("filelist") > 0)
    {
        string line;
        ifstream filelist(vm["filelist"].as<string>());
        if(filelist.fail())
        {
            rc = -1;
            errorText << "Unable to open filelist for reading";
            bberror << err("error.filelist", vm["filelist"].as<string>());
            LOG_ERROR_TEXT_RC(errorText, rc);
            return rc;
        }
        
        while(getline(filelist, line))
        {
            int rc;
            wordexp_t p;
            char** w;
            rc = wordexp(line.c_str(), &p, WRDE_NOCMD);
            if(rc)
            {
                switch(rc)
                {
                    case WRDE_BADCHAR:
                        errorText << "Illegal occurrence of newline or one of |, &, ;, <, >, (, ), {, }.";
                        break;
                    case WRDE_BADVAL:
                        errorText << "An undefined shell variable was referenced";
                        break;
                    case WRDE_CMDSUB:
                        errorText << "Command substitution occurred";
                        break;
                    case WRDE_NOSPACE:
                        errorText << "Out of memory";
                        break;
                    case WRDE_SYNTAX:
                        errorText << "Shell syntax error, such as unbalanced parentheses or unmatched quotes";
                        break;
                }
                rc = -1;
                bberror << err("error.filelist", vm["filelist"].as<string>());
                LOG_ERROR_TEXT_RC(errorText, rc);
                BB_FreeTransferDef(l_Transfer);
                return rc;
            }
            
            w = p.we_wordv;
            vector<string> strs;
            for (size_t i=0; i<p.we_wordc; i++)
            {
                strs.push_back(w[i]);
            }
            wordfree( &p );
            
            string src, dst, flags;
            
            if (!strs.size() ) continue;
            if((strs.size() < 2) || (strs.size() > 3))
            {
                rc = -1;
                errorText << "Invalid number of parameters for filelist";
                bberror << err("error.filelist", vm["filelist"].as<string>()) << err("error.badline", line);
                LOG_ERROR_TEXT_RC(errorText, rc);
                
                BB_FreeTransferDef(l_Transfer);
                return rc;
            }
            src = strs[0];
            dst = strs[1];
            if(strs.size() >= 3)
            {
                flags = strs[2];
            } 
            else 
            {
                flags= "0";
            }
            
            // NOTE:  All validity checking for the files and file flags is done by bbapi...
            rc = BB_AddFiles(l_Transfer, src.c_str(), dst.c_str(), (BBFILEFLAGS)stoi(flags));
            if (rc)
            {
                BB_FreeTransferDef(l_Transfer);
                return rc;
            }
        }
        
    }
    if(rc)
    {
        //  NOTE:  Ignore potential error from BB_FreeTransferDef() and report original failure
        BB_FreeTransferDef(l_Transfer);
        return rc;
    }
    
    return BB_StartTransfer(l_Transfer, l_Handle);
}

int bbcmd_cancel(po::variables_map& vm)
{
    stringstream errorText;

    VMEXISTS("handle");
    BBCANCELSCOPE l_CancelScope = DEFAULT_CANCELSCOPE;

    std::string l_HandleStr = (vm["handle"].as<string>());
    BBTransferHandle_t l_Handle = (BBTransferHandle_t)strtoull(l_HandleStr.c_str(), NULL, 10);
    if (vm.count("scope"))
    {
        l_CancelScope = get_cancelScope(vm["scope"].as<string>().c_str());
        if (check_cancel_scope(l_CancelScope))
        {
            int rc = -1;
            errorText << "scope option of " << vm["scope"].as<string>() << " is invalid";
            LOG_ERROR_TEXT_RC(errorText, rc);
            return rc;
        }
    }

    return BB_CancelTransfer(l_Handle, l_CancelScope);
}

int bbcmd_gethandle(po::variables_map& vm)
{
    int rc=0;

    uint64_t l_TransferHandle = 0;
    uint64_t l_NumContribs = 0;
    uint32_t* l_Contrib = 0;

    VMEXISTS("tag");

    if(vm.count("contrib") > 0)
    {
        vector<uint32_t> contribvec;
        expandRangeString(vm["contrib"].as<string>(), 0, MAX_NUMBER_OF_CONTRIBS, contribvec);

        l_NumContribs = contribvec.size();
        l_Contrib = (uint32_t*)(new char[sizeof(uint32_t)*l_NumContribs]);
        uint32_t i = 0;
        for(const auto& e : contribvec)
        {
            l_Contrib[i++] = e;
        }
    }

    rc = BB_GetTransferHandle((BBTAG)(vm["tag"].as<int>()), l_NumContribs, l_Contrib, &l_TransferHandle);
    if(rc == 0)
    {
        bberror.errdirect("out.transferHandle", l_TransferHandle);
    }
    if (l_Contrib)
    {
        delete[] l_Contrib;
        l_Contrib = 0;
    }

    return rc;
}


#define BB_SQ_BUFFSIZE 256

int bbcmd_getserver(po::variables_map& vm)
{
    int rc=0;
    if (vm.count("connected")){
        size_t bufsize=BB_SQ_BUFFSIZE;
        char buffer[BB_SQ_BUFFSIZE];
        buffer[0]=0;
        std::string l_requestString = (vm["connected"].as<string>());
        rc=BB_GetServer(l_requestString.c_str(), bufsize, buffer);
        if (rc) return rc;
        bberror.errdirect("out.serverList", buffer);
    }
    if (vm.count("waitforreplycount") )
    {
        size_t bufsize=BB_SQ_BUFFSIZE;
        char buffer[BB_SQ_BUFFSIZE];
        buffer[0]=0;
        std::string l_requestString = (vm["waitforreplycount"].as<string>());
        rc=BB_GetServerByName(l_requestString.c_str(),"waitforreplycount", bufsize, buffer);
        if (rc) return rc;
        bberror.errdirect("out.waitforreplycount", buffer);
    }
    return rc;
}
int bbcmd_setserver(po::variables_map& vm)
{
    int countParams=0;

    if (vm.count("open") )
    {
        countParams++;
        int rc = BB_OpenServer(vm["open"].as<string>().c_str() );
        if (rc){ bberror.errdirect("out.serverOpen.rc", rc); return rc; }
    }

    if (vm.count("activate") )
    {
        countParams++;
        int rc = BB_SetServer("activate", vm["activate"].as<string>().c_str() );
        if (rc){ bberror.errdirect("out.serverSet.rc", rc); return rc; }
    }

    if (vm.count("offline") )
    {
        countParams++;
        int rc = BB_SetServer("offline", vm["offline"].as<string>().c_str() );
        if (rc){ bberror.errdirect("out.serverSet.rc", rc); return rc; }
    }

    if (vm.count("close"))
    {
        countParams++;
        int rc = BB_CloseServer(vm["close"].as<string>().c_str() );
        if (rc) { bberror.errdirect("out.serverClose.rc", rc); return rc; }

    }

    if ( countParams) return 0;
    bberror.errdirect("out.errormsg", (char *)"Need to specify one of --open, --activate, --offline, --close");
    return EINVAL;
}

int bbcmd_getstatus(po::variables_map& vm)
{
    int rc=0;

    BBTransferInfo_t l_TransferInfo;

    VMEXISTS("handle");

    std::string l_HandleStr = (vm["handle"].as<string>());
    BBTransferHandle_t l_Handle = (BBTransferHandle_t)strtoull(l_HandleStr.c_str(), NULL, 10);

    rc = BB_GetTransferInfo(l_Handle, &l_TransferInfo);

    if (!rc) {
        // Insert returned data into errstate...
        bberror.errdirect("out.handle", l_TransferInfo.handle);
        bberror.errdirect("out.contribid", l_TransferInfo.contribid);
        bberror.errdirect("out.jobid", l_TransferInfo.jobid);
        bberror.errdirect("out.jobstepid", l_TransferInfo.jobstepid);
        bberror.errdirect("out.tag", l_TransferInfo.tag);
        bberror.errdirect("out.numcontrib", l_TransferInfo.numcontrib);

        char l_Temp2[64] = {'\0'};
        getStrFromBBStatus(l_TransferInfo.status, l_Temp2, sizeof(l_Temp2));
        bberror.errdirect("out.status", l_Temp2);
        bberror.errdirect("out.numreportingcontribs", l_TransferInfo.numreportingcontribs);
        bberror.errdirect("out.totalTransferSize", l_TransferInfo.totalTransferSize);
        getStrFromBBStatus(l_TransferInfo.localstatus, l_Temp2, sizeof(l_Temp2));
        bberror.errdirect("out.localstatus", l_Temp2);
        bberror.errdirect("out.localTransferSize", l_TransferInfo.localTransferSize);
    }

    return rc;
}

int bbcmd_gettransferkeys(po::variables_map& vm)
{
    int rc=0;

    const uint64_t DEFAULT_BUFFERSIZE = 4096;
    uint64_t l_BufferSize = DEFAULT_BUFFERSIZE;

    char* l_Buffer = 0;

    VMEXISTS("handle");

    if(vm.count("buffersize") > 0)
    {
        l_BufferSize = (uint64_t)(vm["buffersize"].as<int>());
    }
    l_Buffer = new char[l_BufferSize];
    l_Buffer[0] = '\0';

    std::string l_HandleStr = (vm["handle"].as<string>());
    BBTransferHandle_t l_Handle = (BBTransferHandle_t)strtoull(l_HandleStr.c_str(), NULL, 10);

    rc = BB_GetTransferKeys(l_Handle, (size_t)l_BufferSize, l_Buffer);

    if (!rc)
    {
        // Insert returned data into errstate...
        bberror.errdirect("out.buffersize", l_BufferSize);
        bberror.errdirect("out.buffer", l_Buffer);
    }

    delete[] l_Buffer;

    return rc;
}

int bbcmd_gettransfers(po::variables_map& vm)
{
    int rc=0;
    stringstream errorText;

    uint64_t l_NumHandles = DEFAULT_NUMBER_OF_HANDLES;
    uint64_t l_NumAvailHandles = 0;
    uint64_t* l_Handles = 0;
    string l_MatchStatusStr = "BBALL";

    if(vm.count("matchstatus") > 0)
    {
        l_MatchStatusStr = vm["matchstatus"].as<string>();
    }
    vector<string> matchStatus;
    boost::split(matchStatus, l_MatchStatusStr, boost::is_any_of(", "), boost::token_compress_on);
    BBSTATUS l_MatchStatus = BBNONE;
    for (size_t i=0; i<matchStatus.size(); ++i)
    {
        if (check_match_status(matchStatus[i].c_str()))
        {
            rc = -1;
            errorText << "match status value of " << matchStatus[i].c_str() << " is invalid.";
            LOG_ERROR_TEXT_RC(errorText, rc);
            break;
        }
        l_MatchStatus = BBSTATUS_OR(l_MatchStatus, getBBStatusFromStr(matchStatus[i].c_str()));
    }

    if (!rc)
    {
        if(vm.count("numhandles") > 0)
        {
            l_NumHandles = (uint64_t)(vm["numhandles"].as<int>());
        }
        l_Handles = new BBTransferHandle_t[l_NumHandles];

        rc = BB_GetTransferList(l_MatchStatus,
                                 &l_NumHandles, l_Handles, &l_NumAvailHandles);

        if (!rc) {
            // Insert returned data into errstate...
            bberror.errdirect("out.numhandles", l_NumHandles);
            bberror.errdirect("out.numavailhandles", l_NumAvailHandles);
            stringstream l_Temp;
            BBTransferHandle_t* l_Handle = l_Handles;
            l_Temp << "(";
            for(uint64_t i=0; i<l_NumHandles; ++i) {
                if (i!=l_NumHandles-1) {
                    l_Temp << *l_Handle << ",";
                } else {
                    l_Temp << *l_Handle;
                }
                ++l_Handle;
            }
            l_Temp << ")";
            bberror.errdirect("out.handles", l_Temp.str());
        }

        if (l_Handles)
        {
            delete[] l_Handles;
        }
    }

    return rc;
}

int bbcmd_getusage(po::variables_map& vm)
{
    int rc=0;

    BBUsage_t usage;

    VMEXISTS("mount");

    rc = BB_GetUsage(vm["mount"].as<string>().c_str(), &usage);

    if(rc == 0)
    {
        bberror.errdirect("out.totalBytesRead",    usage.totalBytesRead);
        bberror.errdirect("out.totalBytesWritten", usage.totalBytesWritten);
        bberror.errdirect("out.localBytesRead",    usage.localBytesRead);
        bberror.errdirect("out.localBytesWritten", usage.localBytesWritten);
#if BBUSAGE_COUNT
        bberror.errdirect("out.localReadCount",    usage.localReadCount);
        bberror.errdirect("out.localWriteCount",   usage.localWriteCount);
#endif
        bberror.errdirect("out.burstBytesRead",    usage.burstBytesRead);
        bberror.errdirect("out.burstBytesWritten", usage.burstBytesWritten);
    }
    return rc;
}

int bbcmd_setusagelimit(po::variables_map& vm)
{
    BBUsage_t usage;

    VMEXISTS("mount");

    memset(&usage,0,sizeof(usage));
    usage.totalBytesRead = vm["rl"].as<unsigned long>();
    usage.totalBytesWritten = vm["wl"].as<unsigned long>();

    return BB_SetUsageLimit(vm["mount"].as<string>().c_str(), &usage);
}

int bbcmd_getdeviceusage(po::variables_map& vm)
{
    int rc=-1;

    BBDeviceUsage_t usage;

    VMEXISTS("device");

    rc = BB_GetDeviceUsage(vm["device"].as<int>(), &usage);

    if(rc == 0)
    {
        bberror.errdirect("out.critical_warning",    usage.critical_warning);
        bberror.errdirect("out.temperature",         usage.temperature);
        bberror.errdirect("out.available_spare",     usage.available_spare);
        bberror.errdirect("out.percentage_used",     usage.percentage_used);
        bberror.errdirect("out.data_read",           usage.data_read);
        bberror.errdirect("out.data_written",        usage.data_written);
        bberror.errdirect("out.num_read_commands",   usage.num_read_commands);
        bberror.errdirect("out.num_write_commands",  usage.num_write_commands);
        bberror.errdirect("out.busy_time",           usage.busy_time);
        bberror.errdirect("out.power_cycles",        usage.power_cycles);
        bberror.errdirect("out.power_on_hours",      usage.power_on_hours);
        bberror.errdirect("out.unsafe_shutdowns",    usage.unsafe_shutdowns);
        bberror.errdirect("out.media_errors",        usage.media_errors);
        bberror.errdirect("out.num_err_log_entries", usage.num_err_log_entries);
    }
    return rc;
}

int bbcmd_getthrottle(po::variables_map& vm)
{
    int rc=-1;
    uint64_t rate;
    VMEXISTS("mount");
    VMEXISTS("rate");

    rc = BB_GetThrottleRate(vm["mount"].as<string>().c_str(), &rate);
    if(rc == 0)
    {
        bberror.errdirect("out.rate", rate);
    }
    return rc;
}

int bbcmd_setthrottle(po::variables_map& vm)
{
    VMEXISTS("mount");
    VMEXISTS("rate");
    return BB_SetThrottleRate(vm["mount"].as<string>().c_str(),
			                  vm["rate"].as<unsigned long>());
}

int bbcmd_create(po::variables_map& vm)
{
    stringstream errorText;

    BBCREATEFLAGS l_Flags = DEFAULT_COPTION;

    VMEXISTS("mount");
    VMEXISTS("size");

    if (vm.count("coptions"))
    {
        l_Flags = get_coptions(vm["coptions"].as<string>().c_str());
        if (check_createlv_flags(l_Flags))
        {
            int rc = -1;
            errorText << "coptions value of " << vm["coptions"].as<string>() << " is invalid.  Most common value is BBXFS.";
            LOG_ERROR_TEXT_RC(errorText, rc);
            return rc;
        }
    }

    return BB_CreateLogicalVolume(vm["mount"].as<string>().c_str(),
                                  vm["size"].as<string>().c_str(), l_Flags);
}

int bbcmd_resize(po::variables_map& vm)
{
    int rc=-1;
    stringstream errorText;

    BBRESIZEFLAGS l_Flags = DEFAULT_ROPTION;

    VMEXISTS("mount");
    VMEXISTS("size");

    if (vm.count("roptions"))
    {
        l_Flags = get_roptions(vm["roptions"].as<string>().c_str());
        if (check_resize_flags(l_Flags))
        {
            rc = -1;
            errorText << "roptions value of " << vm["roptions"].as<string>() << " is invalid";
            LOG_ERROR_TEXT_RC(errorText, rc);
            return rc;
        }

    }

    rc = BB_ResizeMountPoint(vm["mount"].as<string>().c_str(),
                             vm["size"].as<string>().c_str(),
                             l_Flags);

    return rc;
}

int bbcmd_remove(po::variables_map& vm)
{
    VMEXISTS("mount");

    return BB_RemoveLogicalVolume(vm["mount"].as<string>().c_str());
}

int bbcmd_removejobinfo(po::variables_map& vm)
{
    return BB_RemoveJobInfo();
}

int bbcmd_suspend(po::variables_map& vm)
{
    std::string l_HostNameStr = UNDEFINED_HOSTNAME;
    char l_HostName[64] = {'\0'};

    if (vm.count("hostname") > 0)
    {
        l_HostNameStr = (vm["hostname"].as<string>());
        string l_Temp = boost::to_upper_copy<std::string>(l_HostNameStr);
        if (l_Temp == ALL)
        {
            l_HostNameStr = NO_HOSTNAME;
        }
    }
    strCpy(l_HostName, l_HostNameStr.c_str(), sizeof(l_HostName));

    // NOTE: Any data to be returned is also inserted into bberror/errstate
    return BB_Suspend(l_HostName);
}

int bbcmd_resume(po::variables_map& vm)
{
    std::string l_HostNameStr = UNDEFINED_HOSTNAME;
    char l_HostName[64] = {'\0'};

    if (vm.count("hostname") > 0)
    {
        l_HostNameStr = (vm["hostname"].as<string>());
        string l_Temp = boost::to_upper_copy<std::string>(l_HostNameStr);
        if (l_Temp == ALL)
        {
            l_HostNameStr = NO_HOSTNAME;
        }
    }
    strCpy(l_HostName, l_HostNameStr.c_str(), sizeof(l_HostName));

    // NOTE: Any data to be returned is also inserted into bberror/errstate
    return BB_Resume(l_HostName);
}

int bbcmd_adminfailover(po::variables_map& vm)
{
    int rc;
    string hostname;
    uint32_t l_NumTransferDefs = 0;
    uint32_t l_NumStoppedTransferDefs = 0;
    uint32_t l_NumRestartedTransferDefs = 0;
    size_t l_TransferDefsSize = 0;
    size_t l_BufferSize = 1024*1024;
    char* l_Buffer = 0;
    BBTransferHandle_t l_Handle = UNDEFINED_HANDLE;

    BB_RTV_TRANSFERDEFS_FLAGS l_Flags = DEFAULT_RTV_TRANSFERDEFS_FLAGS;

    l_Buffer = new char[l_BufferSize];
    l_Buffer[0] = '\0';

#define FAIL(text) { bberror << err("error.bbcmdstate", text) << err("rc", rc); delete [] l_Buffer; return rc;}


    char contribid[64];
    snprintf(contribid, sizeof(contribid), "%d", UNDEFINED_CONTRIBID);
    rc = Coral_SetVar("jobid",     "0");
    if(rc) FAIL("Unable to set jobid");

    rc = Coral_SetVar("jobstepid", "0");
    if(rc) FAIL("Unable to set jobstepid");

    rc = Coral_SetVar("contribid", contribid);
    if(rc) FAIL("Unable to set contribid");

    rc = activecontroller->gethostname(hostname);
    if(rc) FAIL("Unable to get hostname");

    rc = BB_RetrieveTransfers(hostname.c_str(), l_Handle, l_Flags, &l_NumTransferDefs, &l_TransferDefsSize, l_BufferSize, l_Buffer);
    if(rc) FAIL("Unable to retrieve transfers");

    rc = BB_StopTransfers(hostname.c_str(), l_Handle, &l_NumStoppedTransferDefs, l_Buffer, l_TransferDefsSize);
    if(rc) FAIL("Unable to stop transfers");

//    sleep(15);

    if (vm.count("resume") > 0)
    {
        std::string l_HostNameStr = UNDEFINED_HOSTNAME;
        rc = BB_Resume(l_HostNameStr.c_str());
        if(rc) FAIL("Unable to resume transfers");
    }

    rc = BB_RestartTransfers(hostname.c_str(), l_Handle, &l_NumRestartedTransferDefs, l_Buffer, l_TransferDefsSize);
    if(rc) FAIL("Unable to restart transfers");
    delete [] l_Buffer;
#undef FAIL

    return 0;
}

int main(int orig_argc, const char** orig_argv)
{
    int argc          = orig_argc;
    const char** argv = orig_argv;
    int rc = -999;
    stringstream errorText;

    int argc_cmd;
    int contribid = UNDEFINED_CONTRIBID;
    string command;
    static string executable = argv[0];  // persist this variable for argv during early exit processing
    vector<string> carrotTokens;
    list<int> contribidlist;
    map<int, string> contribResults;

    
    if(strstr(argv[1], "^") != NULL)
    {
        carrotTokens = buildTokens(argv[1], "^");
        command = argv[0];
        argv = (const char**)malloc(sizeof(char*) * carrotTokens.size() + 1);
        
        argc = 1;
        argv[0] = executable.c_str();
        for(const auto& e : carrotTokens)
        {
            argv[argc++] = e.c_str();
        }
    }
    
    for(argc_cmd=1; argc_cmd < argc; argc_cmd++)
    {
        if(argv[argc_cmd][0] != '-')
        {
            command = argv[argc_cmd];
            break;
        }
    }
    po::variables_map vm;
    po::options_description generic(NAME " allowed options");
    po::positional_options_description cmd;
    map<string, boost::shared_ptr<po::option_description> > optlist;

    try
    {
    #define OPTION(name) optlist[name] = boost::make_shared<po::option_description>(po::option_description
    #define OPTEND )
        OPTION("contrib")              ("contrib",              po::value<string>(),                          "Contributor list")                          OPTEND;
        OPTION("coptions")             ("coptions",             po::value<string>(),                          "Create options")                            OPTEND;
        OPTION("delay")                ("delay",                po::value<int>(),                             "Number of seconds to sleep")                OPTEND;
        OPTION("device")               ("device",               po::value<int>()->default_value(0),           "NVMe drive index")                          OPTEND;
        OPTION("filelist")             ("filelist",             po::value<string>(),                          "File containing list of files to transfer") OPTEND;
        OPTION("group")                ("group",                po::value<string>(),                          "New Group")                                 OPTEND;
        OPTION("handle")               ("handle",               po::value<string>(),                          "Transfer handle")                           OPTEND;
        OPTION("hostname")             ("hostname",             po::value<string>(),                          "Host name")                                 OPTEND;
        OPTION("lglvol")               ("lglvol",               po::value<string>(),                          "Logical volume")                            OPTEND;
        OPTION("matchstatus")          ("matchstatus",          po::value<string>(),                          "Match status")                              OPTEND;
        OPTION("mount")                ("mount",                po::value<string>(),                          "Mountpoint")                                OPTEND;
        OPTION("mode")                 ("mode",                 po::value<string>(),                          "New Mode")                                  OPTEND;
        OPTION("numhandles")           ("numhandles",           po::value<int>(),                             "Number of handles")                         OPTEND;
        OPTION("buffersize")           ("buffersize",           po::value<size_t>(),                          "Maximum buffer size")                       OPTEND;
        OPTION("numavailhandles")      ("numavailhandles",      po::value<int>(),                             "Number of available handles")               OPTEND;
        OPTION("path")                 ("path",                 po::value<string>(),                          "Pathname")                                  OPTEND;
        OPTION("rate")                 ("rate",                 po::value<unsigned long>(),                   "Throttle rate")                             OPTEND;
        OPTION("rl")                   ("rl",                   po::value<unsigned long>()->default_value(0), "Read limit")                                OPTEND;
        OPTION("roptions")             ("roptions",             po::value<string>(),                          "Resize options")                            OPTEND;
        OPTION("rtvoptions")           ("rtvoptions",           po::value<string>(),                          "Retrieve transfer definitions option")      OPTEND;
        OPTION("scope")                ("scope",                po::value<string>(),                          "Scope for the command")                     OPTEND;
        OPTION("sendto")               ("sendto",               po::value<string>(),                          "Command will be sent to this bb.proxy")     OPTEND;
        OPTION("size")                 ("size",                 po::value<string>(),                          "Size of the logical volume")                OPTEND;
        OPTION("tag")                  ("tag",                  po::value<int>(),                             "Transfer tag identifier")                   OPTEND;
        OPTION("transferdefs")         ("transferdefs",         po::value<string>(),                          "Transfer definitions (as archive)")         OPTEND;
        OPTION("transferdefs_size")    ("transferdefs_size",    po::value<size_t>(),                          "Transfer definitions (as archive) size")    OPTEND;
        OPTION("user")                 ("user",                 po::value<string>(),                          "New User")                                  OPTEND;
        OPTION("value")                ("value",                po::value<string>(),                          "Value")                                     OPTEND;
        OPTION("variable")             ("variable",             po::value<string>(),                          "Variable")                                  OPTEND;
        OPTION("wl")                   ("wl",                   po::value<unsigned long>()->default_value(0), "Write limit")                               OPTEND;
        OPTION("workperdelayinterval") ("workperdelayinterval", po::value<int>(),                             "Work items to process per delay interval")  OPTEND;
        OPTION("connected")            ("connected",            po::value<string>(),                          "all, active, ready, backup, primary")                        OPTEND;
        OPTION("resume")               ("resume",               po::value<bool>(),                            "true, false")
            OPTEND;
        OPTION("open")                 ("open",                 po::value<string>(),                          "connect bbproxy to bbserver name in JSON format bb.<server name> such as bb.server0")
            OPTEND;
        OPTION("close")                ("close",                po::value<string>(),                          "disconnect bbproxy from bbserver name in JSON format bb.<server name> such as bb.server0")
            OPTEND;
        OPTION("activate")             ("activate",             po::value<string>(),                          "change bbproxy to actively use bbserver name in JSON format bb.<server name> such as bb.server0")
            OPTEND;
        OPTION("offline")              ("offline",              po::value<string>(),                          "change bbproxy to not actively use bbserver name in JSON format bb.<server name> such as bb.server0")
            OPTEND;
        OPTION("waitforreplycount")                ("waitforreplycount",                po::value<string>(),                          "get bbproxy wait-for-reply count for bbserver name in JSON format bb.<server name> such as bb.server0")
        OPTEND;
    #undef OPTEND
    #undef OPTION
            
        cmd.add("command", -1);
        generic.add_options()
            ("command", po::value< string >(), "Command")
            ("help", "Display commands and options")
            ("config", po::value<string>()->default_value(DEFAULT_CONFIGFILE), "Path to configuration file")
            ("jobid", po::value<string>(), "bbcmd Job ID")
            ("jobindex", po::value<string>(), "bbcmd Job index")
            ("jobstepid", po::value<string>(), "bbcmd Job step ID")
            ("bbid", po::value<string>(), "bbcmd internal ID")
            ("contribid", po::value<string>()->default_value(NO_CONTRIBID_STR), "bbcmd Contributor ID")
            ("target", po::value<string>(), "Comma separated contributor list")
            ("envs", po::value<string>(), "Comma separated environment variable list")
            ("hostlist", po::value<string>()->default_value(DEFAULT_HOSTLIST), "Comma separated Node list")
            ("sendto", po::value<string>()->default_value(DEFAULT_PROXY_NAME), "bbProxy to use")
            ("csmcommand", po::value<string>(), "Format response for csm_bb_cmd")
            ("bcast", "Broadcast identical command to all targets")
            ("admin", "Display admin-level commands and options")
            ("xml", "Output via XML")
            ("pretty", "Output pretty-printed")
        ;

        if((command != "") && (bbcmd_map.find(command) != bbcmd_map.end()))
        {
            po::options_description desc_cmd(string(NAME) + " " + command + " specific options");
            bbcmd_map[command].valid_options.sort();
            for(auto opt : bbcmd_map[command].valid_options)
            {
                desc_cmd.add(optlist[opt]);
            }
            generic.add(desc_cmd);
        }

        po::store(po::command_line_parser(argc, argv).options(generic).positional(cmd).run(), vm);
        po::notify(vm);

        if (vm.count("command") == 0)
        {
            usageWithoutCommand(argv[0], vm, generic);
            exit(-1);
        }
        else if (bbcmd_map.find(command) != bbcmd_map.end())
        {
            // Valid command found
            if (vm.count("admin"))
            {
                cerr << "Option admin must be specified alone without any command" << endl;
                exit(-1);
            }
            if (vm.count("help"))
            {
                cout << "bbcmd usage:" << endl;
                cout << "\t" << argv[0] << " <command> <options>\n" << endl;
                cout << generic << endl;
                exit(0);
            }
        }
        else
        {
            cout << "Unknown command: " << command <<" Specify --help for help text."<< endl;
            exit(-1);
        }

        if(vm.count("envs") > 0)
        {
            vector<string> toks = buildTokens(vm["envs"].as<string>(), ",");
            for(const auto& t : toks)
            {
                char* newenv = (char*)malloc(t.size()+1);
                memcpy(newenv, t.c_str(), t.size()+1);
                putenv(newenv); // pointer ownership is transferred to putenv
            }
        }
    }
    catch (std::exception& e)
    {
        cerr << "Error: " << e.what() << endl;
        exit(-1);
    }

    try
    {            
        // Valid command processing
        command = vm["command"].as<string>();

        if (curConfig.load(vm["config"].as<string>()))
        {
            rc = -1;
            std::string configfile = vm["config"].as<string>();
            errorText << "Error loading configuration from " << configfile;
            cerr << errorText << endl;
            bberror << err("error.configfile", configfile);
            LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
        }
        
        config = curConfig.getTree();
        
        initializeLogging("bb.cmd.log", config);

        // NOTE:  The final support for send_to requires a proxy running on the FEN that forwards the request to the correct
        //        bbproxy server.  That has not been provided yet...
        //
        //        The bbcmd_map needs to be updated to allow for this option to be specified.  Otherwise, support for send_to
        //        should all be ready.  If send_to were to be allowed without a proxy, then a CLEAR connection would have to
        //        be used which is not secure.
        //
        //        As it stands now, bbcmd will always send to DEFAULT_PROXY_NAME, which is bb.proxy in coral.cfg.
        if (vm.count("sendto") > 0)
        {
            ProcessId = (vm["sendto"].as<string>());
        }

        if (vm.count("jobid") > 0)
        {
            string tmp = string("LSF_STAGE_JOBID=") + vm["jobid"].as<string>();
            char* newenv = (char*)malloc(tmp.size()+1);
            memcpy(newenv, tmp.c_str(), tmp.size()+1);
            putenv(newenv); // pointer ownership is transferred to putenv
        }

        if (vm.count("jobindex") > 0)
        {
            string tmp = string("LSF_STAGE_JOBINDEX=") + vm["jobindex"].as<string>();
            char* newenv = (char*)malloc(tmp.size()+1);
            memcpy(newenv, tmp.c_str(), tmp.size()+1);
            putenv(newenv); // pointer ownership is transferred to putenv
        }
        
        if (vm.count("bbid") > 0)
        {
            BBJOBID = stoul(vm["bbid"].as<string>());
        }
        
        if (vm.count("jobstepid") > 0)
        {
            string tmp = string("PMIX_NAMESPACE=") + vm["jobstepid"].as<string>();
            char* newenv = (char*)malloc(tmp.size()+1);
            memcpy(newenv, tmp.c_str(), tmp.size()+1);
            putenv(newenv); // pointer ownership is transferred to putenv
        }
        
        if (vm.count("contribid") > 0)
        {
            contribid = atoi(vm["contribid"].as<string>().c_str());
            contribidlist.push_back(contribid);
        }
        
        if(vm.count("csmcommand") > 0)
        {
            string hostname;
            rc = activecontroller->gethostname(hostname);
            contribidlist.clear();
            
            for(string host : buildTokens(vm["csmcommand"].as<string>(), ","))
            {
                vector<string> tok = buildTokens(host, ":");
                if((tok[0] == hostname) || (tok[0] == "localhost"))
                {
                    contribid = stoi(tok[1]);
                    contribidlist.push_back(contribid);
                }
            }
        }
        
        if (vm.count("target") > 0)
        {
            config.put("bb.api.noproxyinit", true);
        }

        // NOTE:  When coming through bbcmd, the whoami value (contribid)
        //        is always initialized to the UNDEFINED_CONTRIBID value.
        //        Commands that are contribid sensitive (e.g. copy and getstatus)
        //        have the contribid keyword available to be passed in.
        //        If not passed in for those commands, a default contribid
        //        value of 0 is used for those operations.
        //
        //        The retrievetransfers command can specify a NULL for the
        //        contribid value.  In that case, contribid is set to be
        //        the NO_CONTRIBID value.
        //
        //        Clients that use the BBAPI interface directly -MUST-
        //        invoke BB_InitLibrary() with their specific whoami
        //        value.
        rc = BB_InitLibrary(contribid, BBAPI_CLIENTVERSIONSTR);
        if (rc)
        {
            // NOTE:  bberror (error.text) was already written to...
            LOG_RC_AND_BAIL(rc);
        }

        rc = setupNodeController("bb.cmd");
        if (rc)
        {
            errorText << "Unable to setup nodeControllers";
            LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
        }

        if (vm.count("target") > 0)
        {
            string                      executable;
            vector<uint32_t>            ranks;
            vector<string>              hosts;
            vector<string>              args;
            boost::property_tree::ptree cmdoutput;

            executable = argv[0];
            string hostlist = vm["hostlist"].as<string>();
            if(hostlist == DEFAULT_HOSTLIST)  // option was not specified
            {
                rc = activecontroller->gethostlist(hostlist);
                if(rc)
                {
                    errorText << "Unable to obtain host list";
                    LOG_ERROR_TEXT_RC_AND_BAIL(errorText, rc);
                }
            }
            hosts = buildTokens(hostlist, ",");
            expandRangeString(vm["target"].as<string>(), 0, hosts.size()-1, ranks);
            args.push_back(vm["command"].as<string>());
            args.push_back("--bbid");
            args.push_back(to_string(BBJOBID));
            if(getenv("BBPATH"))
            {
                args.push_back("--envs");
                args.push_back(string("BBPATH=") + getenv("BBPATH"));
            }
            for (const auto& it : vm)
            {
                if((it.first == "target") || (it.first == "command") || (it.first == "pretty") || 
                   (it.first == "jobid") || (it.first == "jobindex") || (it.first == "bbid") || (it.first == "bcast") ||
                   (it.first == "contribid") || (it.first == "separator") || (it.first == "hostlist") || (it.first == "sendto"))
                    continue;
                
                string arg = string("--") + it.first + "=";
                auto& value = it.second.value();

                if((it.first == "config") && (vm["config"].as<string>() == DEFAULT_CONFIGFILE))
                    continue;
                
                if (auto v = boost::any_cast<uint32_t>(&value))
                    arg += to_string(*v);
                else if (auto v = boost::any_cast<int>(&value))
                    arg += to_string(*v);
                else if (auto v = boost::any_cast<unsigned long>(&value))
                    arg += to_string(*v);
                else if (auto v = boost::any_cast<std::string>(&value))
                    arg += *v;
                else
                    arg += "error";
                args.push_back(arg);
            }
            bool usebcast = false;
            if (vm.count("bcast"))
                usebcast = true;

            rc = activecontroller->bbcmd(ranks, hosts, executable, args, cmdoutput, usebcast);
            bberror << err("rc", rc);

            std::ostringstream result_stream;
            boost::property_tree::write_json(result_stream, cmdoutput, false);
            bberror.merge(result_stream.str());
        }
        else
        {
            for(const auto& id: contribidlist)
            {
                if(contribid != id)
                {
                    contribid = id;
                    Coral_SetVar("contribid", to_string(id).c_str());
                }
                bberror.setToNotClear(); //do not clear bberror for bbcmd calls of BB APIs
                rc = (*bbcmd_map[command].func)(vm);
                bberror << err("rc", rc);
                
                if(vm.count("csmcommand") > 0)
                {
                    if (!rc)
                    {
                        bberror.prune();
                    }

                    contribResults[contribid] = bberror.get("json");
                    bberror.resetToClear();
                    bberror.clear();
                }
            }
        }
    }
    catch(ExceptionBailout& e) { }
    catch(std::exception& e)
    {
        rc = -1;
        LOG_ERROR_RC_WITH_EXCEPTION(__FILE__, __FUNCTION__, __LINE__, e, rc);
    }

    if(vm.count("csmcommand") > 0)
    {
        try
        {            
            bool first = true;
            for(const auto& result : contribResults)
            {
                if(first) first = false;
                else      cout << ",";
                cout << "\"" << result.first << "\":" << result.second;
            }
            cout << endl;
            teardownNodeController();
        }
        catch(std::exception& e)
        {
            cout << "\"cmderror\":1";
        }
        exit(0);
    }
    else
    {
        std::string result;
        try
        {
            if (!rc)
            {
                bberror.prune();
            }

            if (vm.count("xml"))
            {
                result = bberror.get("xml");
            }
            else if (vm.count("pretty"))
            {
                result = bberror.get("pretty");
            }
            else
            {
                result = bberror.get("json");
                //result = bberror.get("pretty");
            }
        }
        catch(std::exception& e)
        {
            result = string("Unable to retrieve bberror");
        }
        cout << result << endl;
    }
    teardownNodeController();
    exit(rc);
    return 0;
}
