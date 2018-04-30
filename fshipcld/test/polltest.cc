/*******************************************************************************
 |    pollTest.cc
 |
 |  © Copyright IBM Corporation 2016. All Rights Reserved
 |
 |    This program is licensed under the terms of the Eclipse Public License
 |    v1.0 as published by the Eclipse Foundation and available at
 |    http://www.eclipse.org/legal/epl-v10.html
 |
 |    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
 |    restricted by GSA ADP Schedule Contract with IBM Corp.
 *******************************************************************************/

   
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <stdio.h>
int main(int argc, char *argv[]) {

   int fd=open("/tmp/here/myfile",O_CREAT|O_APPEND);
   struct pollfd pfd = {.fd=fd, .events=POLLIN, .revents=0};
  
   printf ("pfd.fd=%d\n",pfd.fd);
   int debugval=1;
   int readrc=0;
   char buff[256];
   while (debugval){
     pfd.revents=0;
     int pollRC = poll(&pfd,1,10);
     printf("pollRC=%d pfd.fd=%d revents=%d events=%d  \n",pollRC,pfd.fd,pfd.revents,pfd.events);
     if (pfd.revents & POLLERR) return POLLERR;
     if (pfd.revents & POLLHUP) return POLLHUP;
     readrc=read(fd,buff,256);
     if (!readrc) printf(" readrc=%d \n",readrc );
     sleep(1);
   }
   close(fd);
}
