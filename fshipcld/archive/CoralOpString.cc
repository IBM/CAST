/*******************************************************************************
 |    CoralOpString.cc
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


//! \file  CoralOps.C
//! \brief class CoralOps

#include <stdio.h>
#include "CoralOps.h"
#include "fshipMacros.h"


CoralOpString::CoralOpString(){
for (unsigned int i=0;i<ARRAY_SIZE_FUNC_OP;i++) opString[i]=NULL; //any unset are null pointers
opString[FSHIP_NOOP]       =  (char *)"noop";
opString[FUSE_LOOKUP]	   =  (char *)"lookup";
opString[FUSE_FORGET]	   =  (char *)"forget";
opString[FUSE_GETATTR]	   =  (char *)"gettattr";
opString[FUSE_SETATTR]	   = (char *)"setattr"; 
opString[FUSE_READLINK]	   = (char *)"readlink"; 
opString[FUSE_SYMLINK]	   = (char *)"symlink";  
opString[FUSE_MKNOD]	   = (char *)"mknod";    
opString[FUSE_MKDIR]	   = (char *)"mkdir";
opString[FUSE_UNLINK]	   = (char *)"unlink"; 
opString[FUSE_RMDIR]       = (char *)"rmdir";
opString[FUSE_RENAME]	   = (char *)"rename"; 
opString[FUSE_LINK]	   = (char *)"link";	
opString[FUSE_OPEN]	   = (char *)"open";
opString[FUSE_READ]	   = (char *)"read";
opString[FUSE_WRITE]	   = (char *)"write";
opString[FUSE_STATFS]	   = (char *)"statfs";
opString[FUSE_RELEASE]	   = (char *)"release";
opString[FUSE_FSYNC]	   = (char *)"fsync";  
opString[FUSE_SETXATTR]	   = (char *)"setxattr"; 
opString[FUSE_GETXATTR]	   = (char *)"getxattr"; 
opString[FUSE_LISTXATTR]   = (char *)"listxattr";
opString[FUSE_REMOVEXATTR] = (char *)"removexattr"; 
opString[FUSE_FLUSH]	   = (char *)"flush";
opString[FUSE_INIT]	   = (char *)"init";
opString[FUSE_OPENDIR]	   = (char *)"opendir";
opString[FUSE_READDIR]	   = (char *)"readdir";
opString[FUSE_RELEASEDIR]  = (char *)"releasedir";
opString[FUSE_FSYNCDIR]	   = (char *)"fsyncdir";
opString[FUSE_GETLK]	   = (char *)"getlk";   
opString[FUSE_SETLK]	   = (char *)"setlk";      
opString[FUSE_SETLKW]	   = (char *)"setlkw";    
opString[FUSE_ACCESS]	   = (char *)"access";
opString[FUSE_CREATE]	   = (char *)"create";
opString[FUSE_INTERRUPT]   = (char *)"interrupt";
opString[FUSE_BMAP]	   = (char *)"bmap";
opString[FUSE_DESTROY]	   = (char *)"destroy";  
opString[FUSE_IOCTL]	   = (char *)"ioctl";
opString[FUSE_POLL]	   = (char *)"poll"; 
opString[FUSE_BATCH_FORGET] =(char *)"batch_forget";
opString[FUSE_FALLOCATE]   = (char *)"fallocate";	
opString[FUSE_READDIRPLUS] = (char *)"readdirplus";
};
