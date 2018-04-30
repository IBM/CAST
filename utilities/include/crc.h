/*******************************************************************************
 |    crc.h
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


#ifndef COMMON_CRC_H
#define COMMON_CRC_H

#include <sys/types.h>

extern unsigned short Crc16n( unsigned short usInitialCrc,
                              unsigned char *pData,
                              unsigned long ulLen );

extern unsigned long Crc32n(unsigned long ulInitialCrc,
                            unsigned char *pData,
                            unsigned long ulLen);

#endif // COMMON_CRC_H

