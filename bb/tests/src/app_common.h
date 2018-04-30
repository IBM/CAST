/*******************************************************************************
 |  app_common.h
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

#ifndef APP_COMMON_H_
#define APP_COMMON_H_

#include <sys/types.h>
//#include <arpa/inet.h>
//#include <sys/socket.h>
//#include <netdb.h>


/* reliable read from file descriptor (retries, if necessary, until hard error) */
ssize_t reliable_read(int fd, void* buf, size_t size);

/* reliable write to file descriptor (retries, if necessary, until hard error) */
ssize_t reliable_write(int fd, const void* buf, size_t size);

/* write the checkpoint data to fd, and return whether the write was successful */
int write_checkpoint(int fd, void* buf, size_t size);

/* write the checkpoint data to fd, and return whether the write was successful (leads to fragmentation)*/
int write_checkpoint_fragmented(int fd, void* buf, size_t size, int hard_fragmentation);


/* read the checkpoint data from file into buf, and return whether the read was successful */
int read_checkpoint(int fd, void* buf, size_t size);


#endif
