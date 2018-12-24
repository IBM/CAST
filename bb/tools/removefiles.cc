/*******************************************************************************
 |    comparefiles.cc
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


#if USE_MPI
#include <mpi.h>
#endif

#include <unistd.h>
#include <fcntl.h>
#include <string>
#include <iostream>
#include <limits.h>
#include <wordexp.h>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include "tstate.h"
#include "logging.h"
#include "util.h"

using namespace std;
namespace po = boost::program_options;


int main(int argc, char *argv[])
{
    int exitrc = 0;
    po::variables_map vm;
    po::options_description desc("Allowed options");

    try
    {
        desc.add_options()
        ("help", "Display this help message")
        ("filelist", po::value<string>()->default_value("/tmp/filelist_not_specified"));
    
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);
    }
    catch (exception& e)
    {
        LOG(bb,error) << "Exception: " << e.what();
        exit(-1);
    }

#if USE_MPI
    MPI_Init(&argc, &argv);
//    int rank;
//    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
#endif
    
    try
    {
        for(auto& ln : runCommand(vm["filelist"].as<string>(), true))
        {
            vector<string> toks;

            int rc;
            wordexp_t p;
            rc = wordexp(ln.c_str(), &p, WRDE_NOCMD);
            if(rc)
                continue;
            
            vector<string> strs;
            for (size_t i=0; i<p.we_wordc; i++)
            {
                toks.push_back(p.we_wordv[i]);
            }
            wordfree(&p);
            
            for(int x=0; x<2; x++)
            {
                if(is_regular_file(boost::filesystem::path(toks[x])))
                {
                    LOG(bb,info) << "Removing file: " << toks[x];
                    unlink(toks[x].c_str());
                }
            }
        }
    }
    catch(ExceptionBailout& e)
    {
        LOG(bb,always) << "ExceptionBailout";
        exitrc = -1;
    }
    catch (exception& e)
    {
        LOG(bb,error) << "Exception: " << e.what();
        exitrc = -1;
    }

#if USE_MPI
    MPI_Finalize();
#endif
    return exitrc;
}
