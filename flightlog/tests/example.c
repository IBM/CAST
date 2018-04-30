/*******************************************************************************
 |    example.c
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


#include <sys/time.h>
#include <stdlib.h>
#include "example_flightlog.h"

int main(int argc, char** argv)
{
    int x;
    int iterations = 0;
    if(argc == 2)
	iterations = atoi(argv[1]);
    
    struct timeval start, end;
    FL_CreateAll(".");
    FL_Write(mytestregistry, FL_STARTUP, "Starting example.  PID=%ld  UID=%ld  GID=%ld", (uint64_t)getpid(), (uint64_t)getuid(), (uint64_t)getgid(), 0);
    
    if(iterations > 0)
    {
	gettimeofday(&start, NULL);
	for(x=0; x<iterations; x++)
	{
	    FL_Write(mytestregistry, FL_ITER, "Performing iteration %ld", (uint64_t)x, 0,0,0);
	}
	gettimeofday(&end, NULL);
	printf("time = %gns per log entry\n", (end.tv_usec - start.tv_usec) * 1000.0 / iterations);
    }
    else
    {
	FL_Write(mytestregistry, FL_NOTIMING, "Skipping timing test", 0,0,0,0);
    }
    
    FL_Write(mytestregistry, FL_EXIT,       "Exiting example", 0,0,0,0);
    return 0;
}
