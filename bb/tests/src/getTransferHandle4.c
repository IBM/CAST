/*******************************************************************************
 |    getTransferHandle4.c
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

// getTransferHandle4.c
//  After genererating a transfer handle for a given BBTAG for multiple
//  contributors. retrieve that same transfer handle again. (confirm they are 
//  the same

#include <stdio.h>
#include <mpi.h>
#include <assert.h>
#include <stdlib.h>
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
    
    if(argc != 1 )
    {
        printf("testcase has not options!\n");
        exit(-1);
    }
    
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    printf("My rank is %d out of %d\n", rank, size);
    
    rc = BB_InitLibrary(rank, BBAPI_CLIENTVERSIONSTR);
    check(rc);
    
    if( rank == 0 )  tag = getpid();
    MPI_Bcast( &tag, sizeof(tag), MPI_BYTE, 0, MPI_COMM_WORLD );
   
    if( !(contrib_list = (uint32_t*)malloc(size*sizeof(uint32_t))) ) {
        fprintf( stderr, "Malloc Failed\n" );
        
        MPI_Abort( MPI_COMM_WORLD, EXIT_FAILURE );
    }
    
    for( i = 0; i < size; i++ )  contrib_list[i] = i;  
    BBTransferHandle_t thandle1;
    BBTransferHandle_t thandle2;
        
    if( rank == 0 )  printf("Obtaining transfer handle\n");
    rc = BB_GetTransferHandle( tag, size, (uint32_t*)&rank, &thandle1 ); 
    check(rc);
    
    if( rank == 0 )  printf("Obtaining transfer handle a second time\n");
    rc = BB_GetTransferHandle( tag, size, (uint32_t*)&rank, &thandle2 ); 
    check(rc);
    
    if( thandle1 != thandle2 ) {
        printf( "ERROR: %d: expecting handles to be same\n\t%lu != %lu\n", 
                rank, thandle1, thandle2 );
        
        MPI_Abort( MPI_COMM_WORLD, EXIT_FAILURE );
    }
    
    
    if( rank == 0 )  printf("Terminating BB library\n");
    rc = BB_TerminateLibrary();
    check(rc);

    MPI_Finalize();
    return 0;
}
