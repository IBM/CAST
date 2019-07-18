/*******************************************************************************
 |    test_basic_xfer.cc
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
    int rank, size;

    if(argc < 4)
    {
        printf("testcase <dir> <pfspath> <size>\n");
        exit(-1);
    }
    int dir         = strtoul(argv[1], NULL, 10);
    char* pfspath   = argv[2];
    size_t filesize = strtoul(argv[3], NULL, 10);

    BBTransferInfo_t info;
    double start, stop;
    int dogenerate = 1;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    printf("My rank is %d out of %d\n", rank, size);

    rc = BB_InitLibrary(rank, BBAPI_CLIENTVERSIONSTR);
    check(rc);

    char sfn[256];
    char tfn[256];
    char cmd[256];
    const char* bbpath = getenv("BBPATH");

    switch(dir)
    {
        case 0: // GPFS -> SSD
            if(strcmp(pfspath, "/dev/zero") == 0)
            {
                snprintf(sfn, sizeof(sfn), "%s", pfspath);
                dogenerate = 0;
            }
            else
            {
                snprintf(sfn, sizeof(sfn), "%s/rank.%d", pfspath, rank);
            }
            snprintf(tfn, sizeof(tfn), "%s/rank.%d", bbpath, rank);
            break;
        case 1:  // SSD -> GPFS
            snprintf(sfn, sizeof(sfn), "%s/rank.%d", bbpath, rank);
            if(strcmp(pfspath, "/dev/null") == 0)
            {
                snprintf(tfn, sizeof(tfn), "%s", pfspath);
            }
            else
            {
                snprintf(tfn, sizeof(tfn), "%s/rank.%d", pfspath, rank);
            }
            break;
        default:
            exit(-1);
    }
    
    printf("source file: %s\n", sfn);
    printf("target file: %s\n", tfn);

    if(dogenerate)
    {
        snprintf(cmd, sizeof(cmd), "/opt/ibm/bb/tools/randfile --file=%s --size=%ld", sfn, filesize);
        printf("generate random file: %s\n", cmd);
        system(cmd);
    }

    BBTransferDef_t* tdef;
    BBTransferHandle_t thandle;
    uint32_t contriblist = rank;

    printf("Creating transfer definition\n");
    rc = BB_CreateTransferDef(&tdef);
    check(rc);

    printf("Adding files to transfer definition\n");
    rc = BB_AddFiles(tdef, sfn, tfn, 0);
    check(rc);

    printf("Obtaining transfer handle\n");
    MPI_Barrier(MPI_COMM_WORLD);
    start = MPI_Wtime();
    rc = BB_GetTransferHandle(getpid(), 1, &contriblist, &thandle); /* \todo tag generation uses getpid() - need something better */
    check(rc);
    MPI_Barrier(MPI_COMM_WORLD);
    stop = MPI_Wtime();
    if(rank == 0)
    {
        printf("PERF(%d,%s,%ld x %d):  BB_GetTransferHandle took %g seconds\n", dir, pfspath, filesize, size, stop-start);
    }

    printf("Starting transfer\n");
    start = MPI_Wtime();
    rc = BB_StartTransfer(tdef, thandle);
    check(rc);
    MPI_Barrier(MPI_COMM_WORLD);
    stop = MPI_Wtime();
    if(rank == 0)
    {
        printf("PERF(%d,%s,%ld x %d):  BB_StartTransfer took %g seconds\n", dir, pfspath, filesize, size, stop-start);
    }

    MPI_Barrier(MPI_COMM_WORLD);
    start = MPI_Wtime();
    do
    {
        rc = BB_GetTransferInfo(thandle, &info);
        check(rc);
        if(info.status != BBFULLSUCCESS)
        {
            sleep(1);
        }
    }
    while(info.status != BBFULLSUCCESS);
    
    MPI_Barrier(MPI_COMM_WORLD);

    stop = MPI_Wtime();
    if(rank == 0)
    {
        printf("PERF(%d,%s,%ld x %d):  Transfer took %g seconds (%g MiBps)\n", dir, pfspath, filesize, size, stop-start, (double)filesize * size / (stop-start) / 1024 / 1024);
    }

    printf("Terminating BB library\n");
    rc = BB_TerminateLibrary();
    check(rc);

    MPI_Finalize();
    return 0;
}
