/*******************************************************************************
 |    randfile.cc
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
#include <boost/program_options.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>

using namespace std;
namespace po = boost::program_options;

#ifndef MIN
#define MIN(a,b) (((a)>(b))?(b):(a))
#endif

int main(int argc, char *argv[])
{
    int exitrc = 0;
    po::variables_map vm;
    try
    {
        po::options_description desc("Allowed options");
        
        desc.add_options()
        ("help", "Display this help message")
        ("file", po::value<string>()->default_value("random_file"))
        
            ("genfilelist", "Generate a list of files")
            ("sourcepath", po::value<string>()->default_value("/tmp/source_not_specified"))
            ("targetpath", po::value<string>()->default_value("/tmp/target_not_specified"))	
        ("minfiles", po::value<unsigned long>()->default_value(1))
        ("maxfiles", po::value<unsigned long>()->default_value(1))
        
        ("size", po::value<unsigned long>(), "fixed size of file")
        ("minsize", po::value<unsigned long>()->default_value(65536))
        ("maxsize", po::value<unsigned long>()->default_value(65536))
        ("by",   po::value<unsigned long>()->default_value(4))
        ("seed", po::value<unsigned long>(), "Random seed")
        ("specialChars", po::value<string>(), "Special Characters")
            
            ("buffersize", po::value<unsigned long>()->default_value(16*1024*1024), "buffer size for write syscall")
            ("barriersize", po::value<unsigned long>()->default_value(0), "barrier size")
        ;
    
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);
    }
    catch (exception& e)
    {
        cerr << "Error: " << e.what() << "\n";
        exit(1);
    }

    int rank;
#if USE_MPI
    int size;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
#else
    rank = getpid();
#endif
    try
    {
        unsigned long seed;
        if (vm.count("seed") > 0)
        {
            seed = vm["seed"].as<unsigned long>();
        }
        else
        {
            seed = time(NULL) + rank;
        }

        boost::random::mt19937 gen(seed);
        boost::random::uniform_int_distribution<unsigned long> sizerange(vm["minsize"].as<unsigned long>(), vm["maxsize"].as<unsigned long>());
        boost::random::uniform_int_distribution<> bufrange(0, 0x8000);

        vector<string> randfiles;
        if (vm.count("genfilelist") > 0)
        {
            boost::random::uniform_int_distribution<> filerange(vm["minfiles"].as<unsigned long>(), vm["maxfiles"].as<unsigned long>());
            unsigned long numfiles = filerange(gen);

            FILE *filelist = fopen(vm["file"].as<string>().c_str(), "w");
            if (filelist == NULL) 
            {
                throw runtime_error("Unable to create+open filelist");
            }
            for (unsigned long x = 0; x < numfiles; x++)
            {
                string srcpath = vm["sourcepath"].as<string>();
                if((srcpath != "/dev/null") && (srcpath != "/dev/zero"))
                {
                    if (vm.count("specialChars") > 0 )
                    {
                        srcpath += string("/") + to_string(rank) + vm["specialChars"].as<string>() + string(".") + to_string(x);
                    } else {
                        srcpath += string("/") + to_string(rank) + string(".") + to_string(x);
                    }
                    randfiles.push_back(srcpath);
                }
                
                string dstpath = vm["targetpath"].as<string>();
                if((dstpath != "/dev/null") && (dstpath != "/dev/zero"))
                {

                    if (vm.count("specialChars") > 0 )
                    {
                        dstpath += string("/") + to_string(rank) + vm["specialChars"].as<string>() + string(".") + to_string(x);
                    } else {
                        dstpath += string("/") + to_string(rank) + string(".") + to_string(x);
                    }


                }
                
            fprintf(filelist, "%s %s 0\n", srcpath.c_str(), dstpath.c_str());
        }
        fclose(filelist);
        }
        else
        {
            string filename = vm["file"].as<string>();
#if USE_MPI
            filename += "." + to_string(rank);
#endif
        randfiles.push_back(filename);
        }

        for (auto &fn : randfiles)
        {
            size_t filesize = 0;
            if (vm.count("size") > 0)
            {
                filesize = vm["size"].as<unsigned long>();
                cerr << "fixed size: " << filesize << endl;
            }
            else
            {
                filesize = sizerange(gen);
                cerr << "random size: " << filesize << endl;
            }

            size_t skipby = vm["by"].as<unsigned long>();

            cout << "filename: " << fn << endl;

            int fd;
            unsigned long BUFFERSIZE = 0;
            uint32_t* buffer;

            BUFFERSIZE = vm["buffersize"].as<unsigned long>();
            buffer = (uint32_t*)malloc(BUFFERSIZE);
            if(buffer == NULL) throw runtime_error("malloc returned NULL");
            memset(buffer, 0, BUFFERSIZE);

#if USE_MPI
            unsigned long barriersize = vm["barriersize"].as<unsigned long>();
            unsigned long barriercount = size - barriersize;
#endif

            fd = open(fn.c_str(), O_CREAT | O_TRUNC | O_WRONLY, 0644);
            if(fd < 0)
            {
                cout << "Unable to open " << fn << "  Skipping." << endl;
                continue;
            }
            for (unsigned long x = 0; x < filesize; x += BUFFERSIZE)
            {
                for (unsigned long y = 0; y < BUFFERSIZE; y += skipby)
                {
                    buffer[y / 4] = bufrange(gen);
                }
#if USE_MPI
                if (barriersize > 0)
                {
                    do
                    {
                        MPI_Barrier(MPI_COMM_WORLD);
                        barriercount += barriersize;
                    } while ((barriercount % size) / barriersize != rank / barriersize);
                }
#endif
                write(fd, buffer, MIN(filesize - x, BUFFERSIZE));
#if USE_MPI
                if (barriersize > 0)
                {
                    syncfs(fd);
                }
#endif
            }
            free(buffer);
            fsync(fd);
#if USE_MPI
            if (barriersize > 0)
            {
                int remainder = (size - 1) / barriersize - (barriercount % size) / barriersize;
                for (; remainder > 0; remainder--)
                {
                    MPI_Barrier(MPI_COMM_WORLD);
                }
            }
#endif
            close(fd);
        }
    }
    catch (exception& e)
    {
        cerr << "Error: " << e.what() << "\n";
        exitrc = 1;
    }
#if USE_MPI
        MPI_Finalize();
#endif
    return exitrc;
}
