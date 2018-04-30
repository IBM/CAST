/*******************************************************************************
 |  test_ckpt_mpi.c
 |
 |  The purpose of the progam is to verify that the burst buffer API interfaces,
 |  as defined in bbapi.h, match the actual implementations for those APIs.
 |  The intent is that this program should successfully compile and link.
 |  It is not intended for this program to be run.
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

#define _GNU_SOURCE 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include <signal.h>
#include <stdint.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <poll.h>
#include <sys/time.h>

#include "mpi.h"
#include "app_common.h"
#include "bb/include/bbapi.h"
#include "bb/include/bbapiAdmin.h"

#define TIME_TO_SLEEP 1
#define FINAL_ITER 5000
#define ARRAY_SIZE 1024

void   *a;                                     //main array to checkpoint
unsigned long long memory_size = 512 * 1024;   //main array size to checkpoint --> drives the checkpoint file size/time
unsigned int my_rank = 0;                      //mpi rank
int num_procs = 0;                             //comm world size
unsigned int wave = 0;                         //current checkpoint number
unsigned int final_wave = 0;                   //total number of checkpoints to take
unsigned int skip_lvcreate = 0;
unsigned int skip_bbapi = 0;

unsigned int job_id = 0;                       //LSF job id
char* local_path;                              //path to burst buffer directory
char* pfs_path;                                //path to parallel file system directory
int restart_mode = 0;                          //is it a restart? 0 means 'no', N!=0 means load checkpoint N
int fragmentation = 0;                         //are we trying to get fragmentation on SSD
int small_writes = 0;                          //are we writing using 64KB write chuncks
BBTransferHandle_t thandles[2];                //transfer handles
struct timeval start_transfer_ckpt[2];
struct timeval end_transfer_ckpt[2];
/**********************************************************************
 * timing functions
 **********************************************************************/
static inline struct timeval take_time(void)
{
    struct timeval tv;
    gettimeofday( &tv, NULL );
    return tv;
}

//provides time in "microseconds"
static inline uint64_t time_diff( struct timeval start, struct timeval end )
{
    uint64_t diff;
    diff = (end.tv_sec - start.tv_sec) * 1000000 +
        (end.tv_usec - start.tv_usec);
    return diff;
}


static inline void getLastErrorDetails(BBERRORFORMAT pFormat, char** pBuffer)
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

/**********************************************************************
 * general bb helpers
 **********************************************************************/
void printbbret(int rc)
{
    if(rc)
        {
            char* errstring = 0;
            getLastErrorDetails(BBERRORJSON, &errstring);
            printf("Error rc:       %d\n", rc);
            printf("Error details:  %s\n", errstring);
            free(errstring);
        }
    return;
}

void getStrFromBBStatus(BBSTATUS pValue, char* pBuffer, const size_t pSize)
{
    if (pSize) {
        pBuffer[0] = '\0';
        switch (pValue) {
            case BBNONE:
                strncpy(pBuffer, "BBNONE", pSize);
                break;
            case BBNOTSTARTED:
                strncpy(pBuffer, "BBNOTSTARTED", pSize);
                break;
            case BBINPROGRESS:
                strncpy(pBuffer, "BBINPROGRESS", pSize);
                break;
            case BBPARTIALSUCCESS:
                strncpy(pBuffer, "BBPARTIALSUCCESS", pSize);
                break;
            case BBFULLSUCCESS:
                strncpy(pBuffer, "BBFULLSUCCESS", pSize);
                break;
            case BBCANCELED:
                strncpy(pBuffer, "BBCANCELED", pSize);
                break;
            case BBFAILED:
                strncpy(pBuffer, "BBFAILED", pSize);
                break;
            case BBSTOPPED:
                strncpy(pBuffer, "BBSTOPPED", pSize);
                break;
            case BBNOTACONTRIB:
                strncpy(pBuffer, "BBNOTACONTRIB", pSize);
                break;
            case BBNOTREPORTED:
                strncpy(pBuffer, "BBNOTREPORTED", pSize);
                break;
            case BBALL:
                strncpy(pBuffer, "BBALL", pSize);
                break;

            default:
                snprintf(pBuffer, pSize, "%s (%zu)", "UNDEFINED", pValue);
        }
    }

    return;
}

int docopy(const char* source, const char* target)
{
    FILE* f1 = fopen(source, "rb");
    FILE* f2 = fopen(target, "wb");
    size_t bytes;
    char   buffer[65536];

    while((bytes = fread(buffer, sizeof(char), sizeof(buffer), f1)) > 0)
    {
        if (fwrite(buffer, sizeof(char), bytes, f2) != bytes)
            return -1;
    }
    fclose(f1);
    fclose(f2);
    sync();

    return 0;
}

void initbbinfra()
{
    int res, gres;
    char bbsize[50];
    unsigned long long req_size = (memory_size*sizeof(double)*3);
    res = BB_InitLibrary(my_rank, BBAPI_CLIENTVERSIONSTR);
    printbbret(res);
    if(res)
        {
            MPI_Abort(MPI_COMM_WORLD,1);
        }
    if(skip_lvcreate == 0)
    {
        //prepare size info
        sprintf(bbsize, "%lluB", (req_size < 8*67108864)? 8*67108864 : req_size);
        printf("rank: %u request new LV of size %s on path %s\n", my_rank, bbsize, local_path);
        res = BB_CreateLogicalVolume(local_path, bbsize, BBXFS);
        if(res)
        {
            printbbret(res);
        }
    }
    //check every process could create their LV
    MPI_Allreduce(&res, &gres, 1, MPI_INT, MPI_BOR ,MPI_COMM_WORLD );
    if(gres) //problem somewhere
        {
            if(skip_lvcreate == 0)
            {
                if(!res) //if problem was not here, need to remove LV before leaving
                {
                    BB_RemoveLogicalVolume(local_path);
                }
            }
            MPI_Barrier(MPI_COMM_WORLD);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
    if(my_rank == 0)
        printf("All LVs set up\n");
    return;
}

int waitprevckpt(BBTransferHandle_t thandle)
{
    int res;
    BBTransferInfo_t tinfo;
    char tinfostatus[64];
    char tinfolocalstatus[64];
    unsigned long attempts = 0;
    if(skip_bbapi)
        return 0;

    res = BB_GetTransferInfo(thandle, &tinfo);
    if(res)
        {
            printbbret(res);
        }
    while ((res == 0) && (tinfo.status != BBFULLSUCCESS)) // && (tinfo.localstatus != BBFULLSUCCESS))
    {
        attempts++;
        res = BB_GetTransferInfo(thandle, &tinfo);
        if(res)
        {
            printbbret(res);
        }
        else
        {
            if (tinfo.status == BBCANCELED || tinfo.status == BBFAILED || tinfo.status == BBSTOPPED)
                break;
            usleep(50);
        /*         getStrFromBBStatus(tinfo.localstatus,tinfolocalstatus,64); */
        /*         getStrFromBBStatus(tinfo.status,tinfostatus,64); */
        /*         printf("Rank %u polls handle request and gets local status: %s ; status: %s\n", */
        /*                my_rank, tinfolocalstatus, tinfostatus); */
        }

    }
    getStrFromBBStatus(tinfo.localstatus,tinfolocalstatus,64);
    getStrFromBBStatus(tinfo.status,tinfostatus,64);
    printf("Rank %u polls handle request and gets local status: %s ; status: %s after %lu polling requests\n",
           my_rank, tinfolocalstatus, tinfostatus, attempts);

    return 0;
}

void finalizebbinfra()
{
    int res;
    printf("wave: %u ; final_wave: %u\n", wave, final_wave);
    if((final_wave>0) && (final_wave > restart_mode))
        {
            if( (final_wave - restart_mode) > 1)
                {
                    printf("rank %u check completion of last handle [%u]\n", my_rank, (final_wave +1)%2 );
                    waitprevckpt(thandles[(final_wave +1)%2]);
                    end_transfer_ckpt[(final_wave +1)%2] = take_time();
                    fprintf(stdout, "Rank %u, time for transfer completion: %lu microseconds, bandwidth: %f MB/s\n",
                            my_rank, time_diff(start_transfer_ckpt[(final_wave +1)%2], end_transfer_ckpt[(final_wave +1)%2]),
                            (((double)(memory_size * sizeof(double)))/((double)(1024*1024))) / ( ((double)time_diff(start_transfer_ckpt[(final_wave +1)%2], end_transfer_ckpt[(final_wave +1)%2])) / 1000000.0 ));
                }

            printf("rank %u check completion of last handle [%u]\n", my_rank, final_wave%2);
            waitprevckpt(thandles[final_wave%2]);
            end_transfer_ckpt[final_wave%2] = take_time();
            fprintf(stdout, "Rank %u, time for transfer completion: %lu microseconds, bandwidth: %f MB/s\n",
                            my_rank, time_diff(start_transfer_ckpt[final_wave%2], end_transfer_ckpt[final_wave%2]),
                            (((double)(memory_size * sizeof(double)))/((double)(1024*1024))) / ( ((double)time_diff(start_transfer_ckpt[final_wave%2], end_transfer_ckpt[final_wave%2])) / 1000000.0 ));

        }
    MPI_Barrier(MPI_COMM_WORLD);
    if(skip_lvcreate == 0)
    {
        printf("rank %u removes its LV\n", my_rank);
        res = BB_RemoveLogicalVolume(local_path);
        printbbret(res);
    }
    res = BB_TerminateLibrary();
    printbbret(res);
    return;
}

/**********************************************************************
 * checkpoint/restart functions
 **********************************************************************/
int stagein()
{
    char ckpt_name[256];
    sprintf(ckpt_name, "rank_%d_%d.ckpt", my_rank, wave);
    return 0;
}

void restart()
{
    struct timeval start_time_restart, end_time_restart;
    char bbname[256];
    char pfsname[256];
    char bbckptdir[256];
    struct stat pfsckptstat;
    int res;
    int globalres = 0;
    int fails = 0;
    int lffd = -1;
    BBTransferDef_t* tdef;
    unsigned long current_tag;

    wave=restart_mode;
    printf("Rank %u is restarting from wave %u\n", my_rank, wave);

     /* build checkpoint file name of wave 0 for this mpi rank*/

    //build checkpoint file names
    sprintf(bbname, "%s/ckpt_wave%d/rank_%d.ckpt", local_path, wave, my_rank);
    sprintf(pfsname, "%s/ckpt_wave%d_%d/rank_%d.ckpt", pfs_path, job_id, wave, my_rank);

    // check if targeted checkpoint file exists and has correct size
    res = stat(pfsname, &pfsckptstat);
    if(res < 0)
        {
            printf("Rank %u: Could not stat the checkpoint file %s on restart: %s\n", my_rank, pfsname, strerror(errno));
            fails = 1;
        }
    else
        {
            if (pfsckptstat.st_size < (memory_size*sizeof(double)))
                {
                    printf("Rank %u: checkpoint file %s has size %zu < %llu\n",
                           my_rank, pfsname, pfsckptstat.st_size, memory_size*sizeof(double));
                    fails = 1;
                }
        }
    MPI_Allreduce(&fails, &globalres, 1, MPI_INT, MPI_BOR, MPI_COMM_WORLD);
    if (globalres)
        {
            if(skip_lvcreate == 0)
            {
                printf("rank %u removes its LV as it can not restart properly\n", my_rank);
                BB_RemoveLogicalVolume(local_path);
            }
            res = BB_TerminateLibrary();
            printbbret(res);
            MPI_Barrier(MPI_COMM_WORLD);
            MPI_Finalize();
            exit(0);
        }

    //every process can stage in checkpoint file
    sprintf(bbckptdir, "%s/ckpt_wave%d", local_path, wave);
    res = mkdir(bbckptdir, 0700);
    if(res < 0)
        {
            printf("Rank %u could not create directory %s on BB\n", my_rank, bbckptdir);
            MPI_Abort(MPI_COMM_WORLD,1);
        }
    //request BB transfer
    start_time_restart = take_time();

    current_tag = (wave * ((unsigned long)num_procs)) + my_rank ;
    if(skip_bbapi == 0)
    {
        printf("Obtaining transfer handle\n");
        res = BB_GetTransferHandle(current_tag, 1, &my_rank, &(thandles[wave%2]));
        printbbret(res);
        if (res) //not handles, that's bad
        {
            MPI_Abort(MPI_COMM_WORLD,1);
        }

        printf("Creating transfer definition\n");
        res = BB_CreateTransferDef(&tdef);
        printbbret(res);
        if (res)
        {
            MPI_Abort(MPI_COMM_WORLD,1);
        }

        printf("Rank %u: Adding files %s -> %s to transfer definition\n", my_rank, pfsname, bbname);
        res = BB_AddFiles(tdef, pfsname, bbname, 0);
        printbbret(res);
        if (res)
        {
            MPI_Abort(MPI_COMM_WORLD,1);
        }

        printf("Starting transfer\n");
        res = BB_StartTransfer(tdef, thandles[wave%2]);
        start_transfer_ckpt[wave%2] = take_time();

        fprintf(stdout, "Rank %u, time for setting BB_StartTransfer() stage-in ESS-->BB: %lu microseconds\n",
                my_rank, time_diff(start_time_restart,start_transfer_ckpt[wave%2]));


        printbbret(res);
        if (res)
        {
            MPI_Abort(MPI_COMM_WORLD,1);
        }

        res = BB_FreeTransferDef(tdef);
        printbbret(res);
        if (res)
        {
            MPI_Abort(MPI_COMM_WORLD,1);
            return;
        }
        printf("Rank %u checks completion of wave %d stagein\n", my_rank, wave);
        waitprevckpt(thandles[wave%2]);
    }
    else
    {
        docopy(pfsname, bbname);
        start_transfer_ckpt[wave%2] = take_time();

        fprintf(stdout, "Rank %u, time for setting BB_StartTransfer() stage-in ESS-->BB: %lu microseconds\n",
                my_rank, time_diff(start_time_restart,start_transfer_ckpt[wave%2]));
    }

    //and now wait for completion
    end_transfer_ckpt[wave%2] = take_time();
    fprintf(stdout, "Rank %u, time for stage-in transfer completion: %lu microseconds, size: %llu Bytes, bandwidth: %f MB/s\n",
            my_rank, time_diff(start_transfer_ckpt[wave%2], end_transfer_ckpt[wave%2]),
            memory_size * sizeof(double),
            (((double)(memory_size * sizeof(double)))/((double)(1024*1024))) / ( ((double)time_diff(start_transfer_ckpt[wave%2], end_transfer_ckpt[wave%2])) / 1000000.0 ));
    MPI_Barrier(MPI_COMM_WORLD);


    //file staged-in, let's read it
    lffd = open(bbname, O_RDONLY);
    if(lffd < 0)
        {
            fprintf(stdout, "Rank %u: file %s not present in BB: %s\n", my_rank, bbname, strerror(errno));
            fails = 1;
        }

    MPI_Allreduce(&fails, &globalres, 1, MPI_INT, MPI_BOR , MPI_COMM_WORLD );
    if(globalres) // could not open local file on all nodes
        {
            fprintf(stdout, "could not open local file on all nodes\n");
            if(lffd>=0)
                {
                    close(lffd);
                    unlink(bbname);
                }
            lffd = -1;

            if(skip_lvcreate == 0)
            {
                printf("rank %u removes its LV as it can not restart properly\n", my_rank);
                BB_RemoveLogicalVolume(local_path);
            }
            res = BB_TerminateLibrary();
            printbbret(res);
            MPI_Barrier(MPI_COMM_WORLD);
            MPI_Finalize();
            exit(0);
        }
    start_time_restart = take_time();
    /* read file */
    res = read_checkpoint(lffd, a, memory_size * sizeof(double));
    close(lffd);
    if (res == 0)//problem reading checkpoint file
        {
            fprintf(stdout, "Could not read checkpoint on rank %d\n", my_rank);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
    else
        {
            end_time_restart = take_time();

            fprintf(stdout, "Rank %u, time for reading checkpoint from BB: %lu microseconds, BB read bandwidth: %f MB/s\n",
                    my_rank, time_diff(start_time_restart, end_time_restart),
                    (((double)(memory_size * sizeof(double)))/((double)(1024*1024))) / ( ((double)time_diff(start_time_restart, end_time_restart)) / 1000000.0 ));


        }
    return;
}


void checkpoint()
{

    /* get the name of our checkpoint file to open for read on restart */
    char bbname[256];
    char pfsname[256];
    char toremove[256];
    int res, lffd;
    struct timeval start_time_ckpt, end_time_ckpt;
    struct stat sd;
    unsigned long current_tag;

    //create a directories
    sprintf(bbname, "%s/ckpt_wave%d", local_path, wave);
    res = mkdir(bbname, 0700);
    if(res < 0)
        {
            if (stat(bbname ,&sd) == 1)
                {
                    printf("Rank %u could not create directory %s on BB\n", my_rank, bbname);
                    MPI_Abort(MPI_COMM_WORLD,1);
                }
        }
    if (my_rank == 0)
        {
            sprintf(bbname, "%s/ckpt_wave%d_%d", pfs_path, job_id, wave);
            res = mkdir(bbname, 0700);
            if(res < 0)
                {
                    printf("could not create directory %s on PFS\n", bbname);
                    MPI_Abort(MPI_COMM_WORLD,1);
                }
        }

    sprintf(bbname, "%s/ckpt_wave%d/rank_%d.ckpt", local_path, wave, my_rank);
    sprintf(pfsname, "%s/ckpt_wave%d_%d/rank_%d.ckpt", pfs_path, job_id, wave, my_rank);
    if (my_rank == 0)
        {
            printf("local file name: %s, remote filename: %s\n", bbname, pfsname);
        }
    MPI_Barrier(MPI_COMM_WORLD);

    /* checkpoint data */

    if( (wave - restart_mode)>2 ) // already 2 checkpoints wrote into burst buffer, need to wait for transfer completion of wave-2, and erase it
        {
            printf("Rank %u checks completion of wave %d\n", my_rank, wave-2);
            waitprevckpt(thandles[(wave-2)%2]);
            end_transfer_ckpt[(wave-2)%2] = take_time();
            fprintf(stdout, "Rank %u, time for transfer completion: %lu microseconds, bandwidth: %f MB/s\n",
                            my_rank, time_diff(start_transfer_ckpt[(wave-2)%2], end_transfer_ckpt[(wave-2)%2]),
                            (((double)(memory_size * sizeof(double)))/((double)(1024*1024))) / ( ((double)time_diff(start_transfer_ckpt[(wave-2)%2], end_transfer_ckpt[(wave-2)%2])) / 1000000.0 ));
            MPI_Barrier(MPI_COMM_WORLD);
            /* now previous checkpoint wave ready to be removed */
            sprintf(toremove, "%s/ckpt_wave%d/rank_%u.ckpt", local_path, wave-2, my_rank);
            printf("rank %u removes file %s\n", my_rank, toremove);
            unlink(toremove);
            MPI_Barrier(MPI_COMM_WORLD);
            sprintf(toremove, "%s/ckpt_wave%d", local_path, wave-2 );
            printf("rank %u Removes directory %s\n", my_rank, toremove);
            rmdir(toremove);
        }
    start_time_ckpt = take_time();
    lffd = open(bbname, O_WRONLY|O_CREAT|O_EXCL|O_DIRECT, S_IRWXU);
    if(lffd < 0) //error on open
        {
            printf("Rank %u could not open local file to checkpoint: %s\n", my_rank, strerror(errno));
            finalizebbinfra();
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
    //let's write the checkpoint file
    if (fragmentation)
        res = write_checkpoint_fragmented(lffd, a, memory_size * sizeof(double), 1);
    else if (small_writes)
        res = write_checkpoint_fragmented(lffd, a, memory_size * sizeof(double), 0);
    else
        res = write_checkpoint(lffd, a, memory_size * sizeof(double));
    fsync(lffd);
    close(lffd);
    end_time_ckpt = take_time();

    fprintf(stdout, "Rank %u wrote %llu Bytes in Burst Buffer in %lu microseconds: bandwidth: %f MB/s\n",
            my_rank,
            memory_size * sizeof(double),
            time_diff(start_time_ckpt, end_time_ckpt),
            (((double)(memory_size * sizeof(double)))/((double)(1024*1024))) / ( ((double)time_diff(start_time_ckpt, end_time_ckpt)) / 1000000.0 ));

    //local checkpoint done, wait others and display time information
    MPI_Barrier(MPI_COMM_WORLD);

    if (res == 0)
        {
            //something wrong with checkpoint writing
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
    start_time_ckpt = take_time();
    //request BB transfer
    BBTransferDef_t* tdef;
    start_transfer_ckpt[wave%2] = take_time();

    if(skip_bbapi == 0)
    {
        current_tag = (wave * ((unsigned long)num_procs)) + my_rank ;
        printf("Rank %u starts file transfer\n", my_rank);
        res = BB_GetTransferHandle(current_tag, 1, &my_rank, &(thandles[wave%2]));
        printbbret(res);
        if (res)
            return;


        res = BB_CreateTransferDef(&tdef);
        printbbret(res);
        if (res)
            return;

        res = BB_AddFiles(tdef, bbname, pfsname, 0);
        printbbret(res);
        if (res)
            return;

        res = BB_StartTransfer(tdef, thandles[wave%2]);
        printbbret(res);
        if (res)
            return;

        end_time_ckpt = take_time();

        fprintf(stdout, "Rank %u called BB_StartTransfer() Burst Buffer-->ESS in %lu microseconds\n",
                my_rank,
                time_diff(start_time_ckpt, end_time_ckpt));

        res = BB_FreeTransferDef(tdef);
        printbbret(res);
        if (res)
            return;
    }
    else
    {
        docopy(bbname, pfsname);
        end_time_ckpt = take_time();

        fprintf(stdout, "Rank %u called BB_StartTransfer() Burst Buffer-->ESS in %lu microseconds\n",
                my_rank,
                time_diff(start_time_ckpt, end_time_ckpt));
    }

    return;
}

/**********************************************************************
 * the MPI application computation part
 **********************************************************************/
void run_kernel1()
{
    sleep(TIME_TO_SLEEP);
    return;
}

void run_kernel2()
{

    int rng1, rng2;
    double * b;
    unsigned int i, j;
    unsigned long iteration;
    struct timeval start_time_iter, end_time_iter;
    double multi = 0;

    /* initialize an array between with values in [0,1]*/
    b = malloc(ARRAY_SIZE * sizeof(double));
    for (i = 0 ; i < ARRAY_SIZE ; i++)
        {
            b[i]= ((double) rand()) / ((double)RAND_MAX);
        }
    if (my_rank == 0)
        {
            fprintf(stdout, "Entering computation loop\n");
            fflush(stdout);
        }
    start_time_iter = take_time();
    for ( iteration = 0 ; iteration < FINAL_ITER ; iteration++)
        {
            /* actual not meaningful computation*/
            for(j = 0 ; j < i ; j+= 1.0)
                {
                    srand(j);
                    rng1 = rand();
                    rng2 = rand();
                    multi += b[(rng1)%ARRAY_SIZE] * b[(rng2)%ARRAY_SIZE];
                }
        }
    end_time_iter = take_time();
    fprintf(stdout, "Computing %d iterations on rank %d took %lu microseconds\n",
            FINAL_ITER, my_rank, time_diff(start_time_iter, end_time_iter));
    fflush(stdout);

    return;
}


/**********************************************************************
 * main program
 **********************************************************************/
int main(int argc, char* argv[]) {
    int opt, temp_rank ;
    struct timeval start_time_app, end_time_app;
    //    char name[256];

    optind = 1;

    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &temp_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);

    my_rank = (unsigned int) temp_rank;
    if(my_rank == 0)
        {
            printf("Number of processes: %d\n", num_procs);
            start_time_app = take_time();
        }

    /* Let's initialize the BB library */


    while ((opt = getopt(argc, argv, "sfbcm:l:p:r:i:w:h")) != -1) {
        switch (opt) {
        case 'h':
            if (!my_rank)
                fprintf(stdout, "Usage: test_ckpt_mpi [options]\n"
                        "       -c  skip logical volume creation\n"
                        "       -f  slow and leading to fragmentation on BB checkpoint writes\n"
                        "       -b  write by 64KB chunck\n"
                        "	-m  local memory size as number of double(default: %llu)\n"
                        "       -l  local directory path\n"
                        "	-p  remote directory path\n"
                        "	-i  job id )\n"
                        "       -r  restart mode (0: no restart, N: restart from checkpoint N after stagein\n"
                        "       -w number of checkpoint wave (default =%u)\n"
                        "	-h  help\n", memory_size, final_wave
                        );
            MPI_Finalize();
            return 0;
        case 'c':
            skip_lvcreate = 1;
            break;
        case 'b':
            skip_bbapi = 1;
            break;
        case 'f':
            fragmentation = 1;
            break;
        case 's':
            small_writes = 1;
            break;
        case 'm':
            memory_size = strtoull(optarg, NULL, 10);
            break;
        case 'l':
            local_path = strdup(optarg);
            break;
        case 'p':
            pfs_path = strdup(optarg);
            break;
        case 'r':
            restart_mode = strtoul(optarg, NULL, 10);
            break;
        case 'i':
            job_id = strtol(optarg, NULL, 10);
            break;
        case 'w':
            final_wave = strtol(optarg, NULL, 10);
            break;
        }
    }

    if((NULL == local_path) || (NULL == pfs_path))
        {
            if(my_rank == 0)
                printf("no path provided, check -h parameter\n");
            MPI_Abort(MPI_COMM_WORLD,1);
        }
    /* array creation */
    if (!memory_size)
        {
            printf("please provide strictly positive memory size requirement\n");
            MPI_Finalize();
            return 0;
        }
    posix_memalign(&a, 4096, memory_size * sizeof(double));
    if(a == NULL)
        a = malloc(memory_size * sizeof(double));
    if(a == NULL)
    {
        printf("unable to allocate array.  memory_size=%llu\n", memory_size);
        MPI_Finalize();
        return 0;
    }

    if (my_rank == 0)
        {
            printf("Provided arguments: \n\t memory size: \t%llu\n\t local bb path: \t%s\n\t pfs path: \t%s\n\t restart: \t%d\n\t final wave: \t%d\n", memory_size, local_path, pfs_path, restart_mode, final_wave);
        }
    /* bb infrastructure initialization */
    initbbinfra();

    if (restart_mode) // restart mode
        {
            restart();
        }
    else //initial execution, must fill the array
        {
            unsigned long long i;
            double j = (double) my_rank;
            for (i=0 ; i < memory_size ; i++)
                {
                    ((double*)a)[i]=j;
                }

        }
    wave++;
    /* main loop */
    for(; wave <= final_wave ; wave++ )
        {
            /* if (my_rank == 0) */
            /*     { */
            /*         printf("Running Kernel\n"); */
            /*     } */
            /* run_kernel1(); */
            /* time to checkpoint */
            if (my_rank == 0)
                {
                    printf("Entering checkpoint wave %d\n", wave);
                }
            checkpoint();
            MPI_Barrier(MPI_COMM_WORLD);
        }
    /* Reaching end of computation */
    if (my_rank == 0)
        {
            end_time_app = take_time();
            fprintf(stdout, "End of Computation: time= %lu seconds\n",
                    time_diff(start_time_app, end_time_app) / 1000000);
            fflush(stdout);
        }

    printf("Terminating BB library\n");
    finalizebbinfra();

    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Finalize();
    return 0;
}
