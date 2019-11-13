/*******************************************************************************
 |    test_spanner_perf.c
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
#include <mpi.h>
#include <assert.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

#include "bb/include/bbapi.h"


//  mpicc test_basic_xfer.c -o test_basic_xfer -I/opt/ibm -Wl,--rpath /opt/ibm/bb/lib -L/opt/ibm/bb/lib -lbbAPI

int rank = 0;
int size = 0;

int addMetadata(const char* label, const char* value)
{
    if(rank == 0) printf("METADATA %s=%s\n", label, value);
    return 0;
}
int addMetadata_fp(const char* label, double value)
{
    if(rank == 0) printf("METADATA %s=%g\n", label, value);
    return 0;
}
int addMetric(const char* label, double value)
{
    if(rank == 0) printf("METRIC %s=%g\n", label, value);
    return 0;
}
int addMetric_str(const char* label, const char* value)
{
    if(rank == 0) printf("METRIC %s=%s\n", label, value);
    return 0;
}

void getLastErrorDetails(BBERRORFORMAT pFormat, char** pBuffer)
{
    int rc;
    size_t l_NumBytesAvailable;
    if(pBuffer)
    {
        rc = BB_GetLastErrorDetails(pFormat, &l_NumBytesAvailable, 0, NULL);
        if(rc == 0)
        {
            *pBuffer = (char*)malloc(l_NumBytesAvailable+1);
            BB_GetLastErrorDetails(pFormat, NULL, l_NumBytesAvailable, *pBuffer);
        }
        else
        {
            *pBuffer = NULL;
        }
    }
}

int check(int rc)
{
    if(rc)
    {
        char* errstring = 0;
        getLastErrorDetails(BBERRORJSON, &errstring);
        printf("Error rc:       %d\n", rc);
        printf("Error details:  %s\n", errstring);
        free(errstring);

        printf("Aborting due to failures\n");
        exit(-1);
    }
    return 0;
}

int main(int argc, char** argv)
{
    int rc;
    int x;

    if(argc < 2)
    {
        printf("testcase <iterations>\n");
        exit(-1);
    }
    int maxiterations = strtoul(argv[1], NULL, 10);
    double start, stop;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    printf("My rank is %d out of %d\n", rank, size);

    rc = BB_InitLibrary(rank, BBAPI_CLIENTVERSIONSTR);
    check(rc);
    
    addMetadata("branch",      getenv("BRANCH_NAME"));
    addMetadata("gitcommit",   getenv("GIT_COMMIT"));
    addMetadata("job",         getenv("TESTNAME"));
    addMetadata("jobid",       getenv("LSB_JOBID"));
    addMetadata("jobstep",     getenv("PMIX_NAMESPACE"));
    addMetadata("user",        getenv("USER"));
    addMetadata_fp("iterations",  (double)maxiterations);
    addMetadata("execname",    argv[0]);    
    addMetadata_fp("ranks",    size);

    BBTransferHandle_t thandle;
    uint32_t* contribListArray;
    int i;
    contribListArray = malloc(sizeof(uint32_t)*size);
    for(i=0;i<size;i++) contribListArray[i]=i;

    printf("Obtaining transfer handle\n");
    MPI_Barrier(MPI_COMM_WORLD);
    start = MPI_Wtime();
    for(x=0; x<maxiterations; x++)
    {
        rc = BB_GetTransferHandle( x, size, contribListArray, &thandle);
        check(rc);
    }
    MPI_Barrier(MPI_COMM_WORLD);
    stop = MPI_Wtime();
    if(rank == 0)
    {
        printf("PERF(%d, %d):  BB_GetTransferHandle took %g seconds\n", maxiterations, size, stop-start);
        addMetric("bbGetTransferHandle_time", stop-start);
    }

    printf("Terminating BB library\n");
    rc = BB_TerminateLibrary();
    check(rc);

    MPI_Finalize();
    return 0;
}
