/*******************************************************************************
 |    getTransferHandle2.c
 |
 |  © Copyright IBM Corporation 2017. All Rights Reserved
 |
 |    This program is licensed under the terms of the Eclipse Public License
 |    v1.0 as published by the Eclipse Foundation and available at
 |    http://www.eclipse.org/legal/epl-v10.html
 |
 |    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
 |    restricted by GSA ADP Schedule Contract with IBM Corp.
*******************************************************************************/


// getTransferHandle2.c
// Generate a transfer handle for a given BBTAG, for multiple contributors
//
//  This test is based on the test_basic_xfer.c sample. This test will use a
//  common BBTAG for all fo the tasks.
//

#include <stdio.h>
#include <mpi.h>
#include <assert.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
       
#include "bb/include/bbapi.h"



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
    
    snprintf(cmd, sizeof(cmd), "/opt/ibm/bb/tools/randfile --file=%s --size=%d", sfn, 1024*1024 * rank + 123);
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
    for( i = 0; i < size; i++ )  contrib_list[i] = i;  
    
    printf("Obtaining transfer handle\n");
    rc = BB_GetTransferHandle( tag, size, contrib_list, &thandle ); 
    check(rc);
    
    printf("Creating transfer definition\n");
    rc = BB_CreateTransferDef(&tdef);
    check(rc);

    printf("Adding files to transfer definition\n");
    rc = BB_AddFiles(tdef, sfn, tfn, 0);
    check(rc);
    
    printf("Starting transfer\n");
    rc = BB_StartTransfer(tdef, thandle);
    check(rc);    
    
    
    printf("Terminating BB library\n");
    rc = BB_TerminateLibrary();
    check(rc);

    MPI_Finalize();
    return 0;
}
