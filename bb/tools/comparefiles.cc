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
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <boost/filesystem.hpp>
#include "tstate.h"
#include "logging.h"
#include "util.h"

using namespace std;
namespace po = boost::program_options;
namespace bfs = boost::filesystem;

#ifndef MIN
#define MIN(a,b) (((a)>(b))?(b):(a))
#endif

void displayMiscompare(size_t low, size_t high, uint64_t count, size_t blockSize)
{
    if(low == high)
    {
        LOG(bb,always) << "Miscompare block " << low << "  Miscompares=" << count << "  Blocksize=" << blockSize;
    }
    else
    {
        LOG(bb,always) << "Miscompare block " << low << " to " << high << "  Miscompares=" << count << "  Blocksize=" << blockSize;
    }
}

int main(int argc, char *argv[])
{
    po::variables_map vm;
    try
    {
        po::options_description desc("Allowed options");
        boost::property_tree::ptree config;
        config.put("comparefiles.log.consoleLog", true);
        config.put("comparefiles.log.consoleStream", "stdout");
        config.put("comparefiles.log.default_sev", "info");
        
        initializeLogging("comparefiles.log", config);
        
        desc.add_options()
        ("help", "Display this help message")
            ("filelist", po::value<string>()->default_value("/tmp/filelist_not_specified"));
    
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);
    }
    catch (exception& e)
    {
        LOG(bb,error) << "Early exit exception: " << e.what();
        exit(-1);
    }

#if USE_MPI
    MPI_Init(&argc, &argv);
#endif
    
    bool mismatch = false;
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
            for (size_t i=0; i<p.we_wordc;i++ )
            {
                toks.push_back(p.we_wordv[i]);
            }
            wordfree( &p );
            
            LOG(bb,info) << "Comparing " << toks[0] << " to " << toks[1];
            
            bfs::path f1 = toks[0];
            bfs::path f2 = toks[1];
            size_t f1size = bfs::file_size(f1);
            size_t f2size = bfs::file_size(f2);
            LOG(bb,always) << "size(" << f1 << ") = " << f1size;
            LOG(bb,always) << "size(" << f2 << ") = " << f2size;
            
            if(f1size == f2size)
            {
                map<size_t, uint64_t> blockCount;
                size_t lastElem, low;
                bool lastElemSet = false;
                uint64_t miscompares;
                
                size_t blockSize;
                if(f1size > 1024ULL*1024*1024)  blockSize = 65536;
                else if(f1size > 128*4096)      blockSize = 4096;
                else                            blockSize = 1;
                
#define BUFFERSIZE ((size_t)(16*1024*1024))
                uint64_t* buffer1 = (uint64_t*)malloc(BUFFERSIZE);
                uint64_t* buffer2 = (uint64_t*)malloc(BUFFERSIZE);
                
                memset(buffer1, 0, BUFFERSIZE);
                memset(buffer2, 0, BUFFERSIZE);
                
                int fd1 = open(f1.string().c_str(), O_RDONLY);
                int fd2 = open(f2.string().c_str(), O_RDONLY);
                size_t len, origlen;
                for(size_t x=0, miscompares=0; x<f1size; )
                {
                    len = read(fd1, buffer1, BUFFERSIZE);
                    assert(len == MIN(BUFFERSIZE, (size_t)(f1size-x)));
                    len = read(fd2, buffer2, BUFFERSIZE);
                    assert(len == MIN(BUFFERSIZE, (size_t)(f2size-x)));
                    
                    origlen = len;
                    len /= sizeof(uint64_t);
                    for(size_t index=0; index<len; index++)
                    {
                        if(buffer1[index] != buffer2[index])
                        {
                            miscompares++;
                            blockCount[(x+index*sizeof(uint64_t))/blockSize]++;
                            if(miscompares < 16)
                            {
                                LOG(bb,always) << "Miscompare at offset 0x" << hex << x+index*sizeof(uint64_t)
                                               << "  File0=0x" << (uint32_t)buffer1[index] 
                                               << "  File1=0x" << (uint32_t)buffer2[index] << dec;
                            }
                        }
                    }
                    x += origlen;
                }
                close(fd1);
                close(fd2);
                
                LOG(bb,always) << "Displaying block count";
                for(const auto& e : blockCount)
                {
                    if(lastElemSet)
                    {
                        if(lastElem + 1 != e.first)
                        {
                            displayMiscompare(low, lastElem, miscompares, blockSize);
                            low = e.first;
                            miscompares = e.second;
                        }
                        else
                        {
                            miscompares += e.second;
                        }
                    }
                    else
                    {
                        low = e.first;
                        miscompares = e.second;
                    }
                    lastElem    = e.first;
                    lastElemSet = true;
                }
                if(lastElemSet)
                    displayMiscompare(low, lastElem, miscompares, blockSize);
            }
            else
            {
                mismatch = true;
                LOG(bb,always) << "File sizes are different";
            }
        }
    }
    catch(ExceptionBailout& e)
    {
        LOG(bb,always) << "ExceptionBailout";
        mismatch = true;
    }
    catch(exception& e)
    {
        LOG(bb,always) << "Exception: " << e.what();
        mismatch = true;
    }
    
#if USE_MPI
    MPI_Finalize();
#endif
    if(mismatch)
        return -1;
    return 0;
}
