/*******************************************************************************
 |    testidentity.cc
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



#include <pthread.h>
#include "identity.h"
#include <stdio.h>
#include "testidentity_fl.h"

void* spinuser(void* vuid)
{
    uid_t uid = (uid_t)((unsigned long)vuid);
    printf("becoming userid=%d\n", uid);
    becomeUser(uid);
    printf("spinning userid=%d\n", uid);
    while(1)
    {
    }
    return NULL;
}

int main(int argc, char** argv)
{
    int rc;
    printf("hello\n");
    FL_CreateAll("/tmp");
    
    printf("toggle test\n");
    becomeUser(34941);
    becomeUser(34941);
    becomeUser(500);
    sleep(2);
    becomeUser(59692);
    becomeUser(59692);
    becomeUser(34941);
    becomeUser(59692);
    bool valid,primary;
    rc = checkUserGroup(75501, 600, &valid, &primary);
    printf("checkUser 75501/600:  rc=%d  valid=%d  primary=%d\n", rc, valid, primary);
    rc = checkUserGroup(59692, 3, &valid, &primary);
    printf("checkUser 59692/3:  rc=%d  valid=%d  primary=%d\n", rc, valid, primary);
    rc = checkUserGroup(59692, 57500, &valid, &primary);
    printf("checkUser 59692/57500:  rc=%d  valid=%d  primary=%d\n", rc, valid, primary);
    rc = checkUserGroup(59692, 57467, &valid, &primary);
    printf("checkUser 59692/57467:  rc=%d  valid=%d  primary=%d\n", rc, valid, primary);

    printf("thread test\n");
    pthread_t tid;
    pthread_create(&tid, NULL, spinuser, (void*)34941); //tgooding
    pthread_create(&tid, NULL, spinuser, (void*)59692); //meaho
    
    while(1)
    {
    }
    
    return 0;
}
