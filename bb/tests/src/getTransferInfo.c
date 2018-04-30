/*******************************************************************************
 |    getTransferInfo.c
 |
 |    Copyright IBM Corporation 2018,2018. All Rights Reserved
 |
 |    This program is licensed under the terms of the Eclipse Public License
 |    v1.0 as published by the Eclipse Foundation and available at
 |    http://www.eclipse.org/legal/epl-v10.html
 |
 |    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
 |    restricted by GSA ADP Schedule Contract with IBM Corp.
*******************************************************************************/

// getTransferInfo.c
// Retrieve the transfer information for a valid transfer handle for a job
//
// This program will require >= 3 tasks. Not all of the task will contribute
//   Task 1 will not be in the cotributor list
//


#include <stdio.h>
#include <mpi.h>
#include <assert.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <inttypes.h>

#include "bb/include/bbapi.h"

#define FILE_SIZE 1024

void check(int rc)
{
    if(rc)
    {
        char errstring[256];
        size_t numBytesAvail;
        size_t buf_len = 256;

        BB_GetLastErrorDetails(BBERRORJSON, &numBytesAvail, buf_len, errstring);
        printf("Error rc:       %d\n", rc);
        printf("Error details:  %s\n", errstring );

        printf("Aborting due to failures\n");
        exit(-1);
    }
}

int main(int argc, char** argv)
{
    int rc;
    int rank, size;
    uint32_t *contrib_list;
    BBTAG tag;
    uint32_t i;
    BBTransferInfo_t info;
    
    if(argc < 3)
    {
        printf("testcase <source> <target>\n");
        exit(-1);
    }
    
    char* source = argv[1];
    char* target = argv[2];
    
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    if( size < 3 ) {
        fprintf( stderr, "This program requires 3 or more tasks" );
        MPI_Abort( MPI_COMM_WORLD, EXIT_FAILURE );
    }
        
        
    printf("My rank is %d out of %d\n", rank, size);
    
    rc = BB_InitLibrary(rank, BBAPI_CLIENTVERSIONSTR);
    check(rc);
    
    char sfn[256];
    char tfn[256];
    char cmd[256];
    
    snprintf(sfn, sizeof(sfn), "%s/rank.%d", source, rank);
    snprintf(tfn, sizeof(tfn), "%s/rank.%d", target, rank);
    printf("source file: %s\n", sfn);
    printf("target file: %s\n", tfn);
    
    snprintf(cmd, sizeof(cmd), 
             "/opt/ibm/bb/tools/randfile --file=%s --size=%d", sfn, FILE_SIZE);
    
    printf("generate random file: %s\n", cmd);
    system(cmd);
    
    BBTransferDef_t* tdef;
    BBTransferHandle_t thandle;
    
    if( rank == 0 )  tag = getpid();
    MPI_Bcast( &tag, sizeof(tag), MPI_BYTE, 0, MPI_COMM_WORLD );
    
    if( !(contrib_list = (uint32_t*)malloc(size*sizeof(uint32_t))) ) {
        fprintf( stderr, "Malloc Failed\n" );
        
        MPI_Abort( MPI_COMM_WORLD, EXIT_FAILURE );
    }
    
    // task 1 not included in the list
    contrib_list[0] = 0;
    for( i = 2; i < size; i++ )  contrib_list[i-1] = i;  
    
    if( rank == 0 )  printf("Obtaining transfer handle\n");
    rc = BB_GetTransferHandle( tag, (size-1), contrib_list, &thandle ); 
    check(rc);
    
    if( rank != 1 ) {
        if( rank == 0 )  printf("Creating transfer definition\n");
        rc = BB_CreateTransferDef(&tdef);
        check(rc);

        if( rank == 0 )  printf("Adding files to transfer definition\n");
        rc = BB_AddFiles(tdef, sfn, tfn, 0);
        check(rc);
    }
    
    /* 
    ** putting in Barrier to make sure that all tasks have called
    ** BB_GetTransferHandle() before making call to BB_GetTransferInfo() 
    */
    MPI_Barrier( MPI_COMM_WORLD );
    
    rc = BB_GetTransferInfo( thandle, &info );
        check(rc);
    
    // CHECK elements of info
    if( info.handle != thandle ) {
        fprintf( stderr, "%d: Expected info.handle: %"PRIu64"; got %"PRIu64".\n", rank, thandle, 
                 info.handle );
        
        MPI_Abort( MPI_COMM_WORLD, EXIT_FAILURE );
    }
        
    if( info.contribid != rank ) {
        fprintf( stderr, "%d: Expected contribid: %d; got %u.\n", rank, rank, 
                 info.contribid );
        
        MPI_Abort( MPI_COMM_WORLD, EXIT_FAILURE );
    }
        
    if( info.tag != tag ) {
        fprintf( stderr, "%d: Expected tag: %lu; got %lu.\n", rank, tag, info.tag );
        
        MPI_Abort( MPI_COMM_WORLD, EXIT_FAILURE );
    }
    
    if( info.numcontrib != (size-1) ) {
        fprintf( stderr, "%d: Expected numcontrib: %d; got %"PRIu64".\n", rank, size, 
                 info.numcontrib );
        
        MPI_Abort( MPI_COMM_WORLD, EXIT_FAILURE );
    }
                        
    if( info.status != BBNOTSTARTED ) {  /* all ranks should get not-started */
        fprintf( stderr, "%d: Expected status: %016llX; got %016llX.\n", 
                 rank, (long long)( BBNOTSTARTED), (long long)info.status ); 
        
        MPI_Abort( MPI_COMM_WORLD, EXIT_FAILURE );
    }
              
    if( info.numreportingcontribs != 0 ) {
        fprintf( stderr, "%d: Expected numreportingcontribs: %d; got %"PRIu64".\n", rank, 0, 
                 info.numreportingcontribs );
        
        MPI_Abort( MPI_COMM_WORLD, EXIT_FAILURE );
    }

    if( info.totalTransferSize != 0 ) {
        fprintf( stderr, 
                 "%d: Expected info.totalTransferSize: %"PRIu64"; got %"PRIu64".\n", rank,
                 (uint64_t)((size-1)*FILE_SIZE), info.totalTransferSize );
        
        MPI_Abort( MPI_COMM_WORLD, EXIT_FAILURE );
    }

    if( info.localstatus != ((rank == 1) ? BBNOTACONTRIB : BBNOTREPORTED )) {
        fprintf( stderr, "%d: Expected localstatus: %016llX; got %016llX.\n", 
                 rank, (long long)( (rank == 1) ? BBNOTACONTRIB : BBNOTREPORTED), (long long)info.localstatus ); 
        
        MPI_Abort( MPI_COMM_WORLD, EXIT_FAILURE );
    }
    
    if( info.localTransferSize != 0) {
        fprintf( stderr, 
                 "%d: Expected info.localTransferSize: %"PRIu64"; got %"PRIu64".\n", rank,
                 (uint64_t)( 0 ), info.localTransferSize );
        
        MPI_Abort( MPI_COMM_WORLD, EXIT_FAILURE );
    }
    
    
    printf("Terminating BB library\n");
    rc = BB_TerminateLibrary();
    check(rc);

    MPI_Finalize();
    return 0;
}
        
