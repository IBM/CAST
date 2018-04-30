/*******************************************************************************
 |    fshipMacros.h
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


//! \file  fshipMacros.h
//! \brief Define commonly used Macros for fship daemons

#ifndef FSHIPMACRO_H
#define FSHIPMACRO_H

#define FSHIP_NOOP  0
#define LLUS long long unsigned int
#define _LI long int
//! \brief define a common syslog type macro
#define LOGERRNO(SUBCOMPONENT,SEVERITY,VALUE) {LOG(SUBCOMPONENT,SEVERITY) << __FILE__ ":" << __LINE__ << " " << __PRETTY_FUNCTION__ << " had errno=" << VALUE << " (" << strerror(VALUE) << ")";}
#define MAX_NAME_STORE 256
#define MAXDIRENTPLUSENTRY sizeof(struct fuse_direntplus)+MAX_NAME_STORE

#if __BYTE_ORDER == __LITTLE_ENDIAN
#define htonll(y) __bswap_64(y)
#else 
#define htonll(y) y
#endif

#endif //FSHIPMACRO_H
