/*******************************************************************************
 |    termThreadSafety.c
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

#include <pthread.h>
#include <bbapi.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <unistd.h>

#define NUMTHREADS 128

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
uint64_t        successcount = 0;

void* myTermThread(void* ptr)
{
    int rc;
    rc = BB_TerminateLibrary();
    if(rc == 0)
    {
        pthread_mutex_lock(&mutex);
        successcount++;
        pthread_mutex_unlock(&mutex);
    }
    return NULL;
}

int main(int argc, char **argv)
{
    int x;
    int rc;
    pthread_t tid[NUMTHREADS];
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    
    rc = BB_InitLibrary(0, BBAPI_CLIENTVERSIONSTR);
    assert(rc == 0);
    
    for(x=0; x<NUMTHREADS; x++)
    {
        rc = pthread_create(&tid[x], NULL, myTermThread, NULL);
        assert(rc == 0);
    }
    
    for(x=0; x<NUMTHREADS; x++)
    {
        rc = pthread_join(tid[x], NULL);
        assert(rc == 0);
    }
    
    pthread_mutex_lock(&mutex);
    printf("BB_TerminateLibrary() was successful %ld times.  Should be 1.\n", successcount);
    assert(successcount == 1);
    pthread_mutex_unlock(&mutex);
    
    printf("Test passed\n");
    return 0;
}
