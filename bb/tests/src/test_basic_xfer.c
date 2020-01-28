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
#include <sys/stat.h>
#include <errno.h>

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

    if(argc < 4)
    {
        printf("testcase <dir> <pfspath> <size> [use subdir]\n");
        exit(-1);
    }
    int dir         = strtoul(argv[1], NULL, 10);
    char* pfspath   = argv[2];
    size_t filesize = strtoul(argv[3], NULL, 10);
    bool use_subdir = false;
    if(argc > 4)
    {
        if('1' == argv[4][0] || 'y' == argv[4][0] || 'Y' == argv[4][0])
        {
            use_subdir = true;
        }
    }

    BBTransferInfo_t info;
    double start, stop;
    int dogenerate = 1;
    char host_name[MPI_MAX_PROCESSOR_NAME];
    int h_len;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Get_processor_name(host_name, &h_len);
    printf("My rank is %d out of %d running on %s\n", rank, size, host_name);

    rc = BB_InitLibrary(rank, BBAPI_CLIENTVERSIONSTR);
    check(rc);

    char sfn[256];
    char tfn[256];
    char tdn[256];
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
                if(use_subdir)
                {
                    snprintf(sfn, sizeof(sfn), "%s/%s/rank.%d", pfspath, host_name, rank);
                    snprintf(tdn, sizeof(tdn), "%s/%s", pfspath, host_name);
                    struct stat st_buf = {0};
                    if(0 != stat(tdn, &st_buf))
                    {
                        if(0 != mkdir(tdn, 0700))
                        {
                            // It is ok if the directory exists (our peer probably created it)
                            // Throw a warning if the error is something else.
                            if(EEXIST != errno)
                            {
                                printf("%d/%d on %s) Warning. mkdir() returned the error (%d): %s\n",
                                       rank, size, host_name, errno, strerror(errno));
                            }
                        }
                    }
                }
                else
                {
                    snprintf(sfn, sizeof(sfn), "%s/rank.%d", pfspath, rank);
                }
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
                if(use_subdir)
                {
                    snprintf(tfn, sizeof(tfn), "%s/%s/rank.%d", pfspath, host_name, rank);
                    snprintf(tdn, sizeof(tdn), "%s/%s", pfspath, host_name);
                    struct stat st_buf = {0};
                    if(0 != stat(tdn, &st_buf))
                    {
                        if(0 != mkdir(tdn, 0700))
                        {
                            // It is ok if the directory exists (our peer probably created it)
                            // Throw a warning if the error is something else.
                            if(EEXIST != errno)
                            {
                                printf("%d/%d on %s) Warning. mkdir() returned the error (%d): %s\n",
                                       rank, size, host_name, errno, strerror(errno));
                            }
                        }
                    }
                }
                else
                {
                    snprintf(tfn, sizeof(tfn), "%s/rank.%d", pfspath, rank);
                }
            }
            break;
        default:
            exit(-1);
    }
    
    printf("source file: %s\n", sfn);
    printf("target file: %s\n", tfn);
    
    addMetadata("branch",      getenv("BRANCH_NAME"));
    addMetadata("gitcommit",   getenv("GIT_COMMIT"));
    addMetadata("job",         getenv("TESTNAME"));
    addMetadata("type",        (dir)?"out":"in");
    addMetadata("jobid",       getenv("LSB_JOBID"));
    addMetadata("jobstep",     getenv("PMIX_NAMESPACE"));
    addMetadata("user",        getenv("USER"));
    addMetadata("pfspath",     pfspath);
    addMetadata("bbpath",      bbpath);
    addMetadata("execname",    argv[0]);
    
    addMetadata_fp("filesize", filesize);
    addMetadata_fp("ranks",    size);

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
        addMetric("bbGetTransferHandle_time", stop-start);
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
        addMetric("bbStartTransfer_time", stop-start);
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
        addMetric("bbTransfer_time", stop-start);
        addMetric("bbTransfer_bandwidth", filesize * size / (stop-start));
    }

    printf("Terminating BB library\n");
    rc = BB_TerminateLibrary();
    check(rc);

    MPI_Finalize();
    return 0;
}
