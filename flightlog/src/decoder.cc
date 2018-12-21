/*******************************************************************************
 |    decoder.cc
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

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <iostream>
#include <boost/program_options.hpp>
#include "flightlog.h"

#define BUFFERSIZE (1024 * 1024)

using namespace std;
using namespace boost;
namespace po = boost::program_options;

int main(int argc, char** argv)
{
	int rc;
	char *buffer;
	uint64_t more;
	FlightRecorderRegistryList_t *list = NULL;
	uint64_t flags = 0;

	try
	{
		po::variables_map vm;
		po::options_description generic("flightlog decoder options");
		po::positional_options_description cmd;
		cmd.add("files", -1);
		generic.add_options()
			("files", po::value<vector<string>>(), "Flightlog files")
			("decoder", po::value<vector<string>>(), "Add search path for decoder libraries")
			("help", "Display this help message")
			("raw", "Display raw processor timestamps");
		po::store(po::command_line_parser(argc, argv).options(generic).positional(cmd).run(), vm);
		po::notify(vm);

		if (vm.count("help"))
		{
			cout << "\n" << generic << "\n";
			exit(0);
		}

		if (vm.count("raw"))
		{
			flags |= FLDECODEFLAGS_RAWMODE;
		}
		if (vm.count("decoder"))
		{
			for (auto path : vm["decoder"].as<vector<string>>())
			{
				FL_AddDecoderLibPath(path.c_str());
			}
		}
		if (vm.count("files"))
		{
			for (auto file : vm["files"].as<vector<string>>())
			{
				printf("Reading flightlog %s\n", file.c_str());
				rc = FL_AttachFile(&list, file.c_str());
				if (rc)
				{
					printf("\t *** Failure reading flightlog %s.  errno=%d (%s)\n", file.c_str(), errno, strerror(errno));
					exit(-1);
				}
			}

			buffer = (char *)malloc(BUFFERSIZE);
			do
			{
				more = 0;
				memset(buffer, 0, BUFFERSIZE);
				FL_Decode(list, BUFFERSIZE, buffer, &more, flags);
				printf("%s\n", buffer);
			} while (more);
			printf("*** END ***\n");
		}
		else
		{
			printf("No flightlogs were specified\n");
		}
	}
	catch (std::exception &e)
	{
		cerr << "Error: " << e.what() << "\n";
		exit(-1);
	}

	return 0;
}
