/*******************************************************************************
 |    crc.cc
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


//////////////////////////////////////////////////////
//
// This file contains the function to generate a 16 bit
// CRC utilizing a 16 byte table.
//
//
// The CRC polynomial used here is:
//
// x^16 + x^12 + x^5 + 1
//
//
//
//#include <hwi/include/common/compiler_support.h>
//#include <firmware/include/crc.h>

#include "crc.h"


///////////////////////////////////////////////////////////
unsigned short Crc16n( unsigned short usInitialCrc,
                       unsigned char *pData,
                       unsigned long ulLen )
//
// Calcuate the CRC for a given buffer of data.
// To do just one buffer start with an usInitialCrc of 0.
// To continue a multibuffer CRC provide the value
// returned from the last call to Crc16 as the usInitialCrcValue.
//
// inputs:
//    usInitialCrc -- initial value for the CRC.
//    pData -- pointer to the data to calculate the CRC for.
//    ulLen -- length of the data to calculate the CRC for.
// outputs:
//    returns -- the CRC of the buffer.
//
//
{
	static unsigned short Crc16x4_Table[16] = {
    	0x0000, 0x1081, 0x2102, 0x3183, 0x4204, 0x5285, 0x6306, 0x7387,
    	0x8408, 0x9489, 0xa50a, 0xb58b, 0xc60c, 0xd68d, 0xe70e, 0xf78f,
	};

    unsigned long n;
    unsigned short t;
    unsigned char *p;
    unsigned short usCrc = usInitialCrc;


    for (n = ulLen, p = pData;
         n > 0;
         n--, p++)
    {
        unsigned char c;
        c = *p;                     // brab the character.

        t = usCrc ^ (c & 0x0f);                         // lower nibble
        usCrc = (usCrc>>4) ^ Crc16x4_Table[t & 0xf];
        t = usCrc ^ (c>>4);                             // upper nibble.
        usCrc = (usCrc>>4) ^ Crc16x4_Table[t & 0xf];
    }

    return(usCrc);
}

//////////////////////////////////////////////////////
//
// This file contains the function to generate a 32 bit
// CRC utilizing a 16 entry table.
//
//
// The CRC polynomial used here is:
//
// x^32 + x^26 + x^23 + x^22 + x^16 +
// x^12 + x^11 + x^10 + x^8 + x^7 + x^5 + x^4 + x^1 + x^0
//
//
//
///////////////////////////////////////////////////////////
unsigned long Crc32n(unsigned long ulInitialCrc,
                     unsigned char *pData,
                     unsigned long ulLen)
//
// Calcuate the CRC for a given buffer of data.
// To do just one buffer start with an ulInitialCrc of 0.
// To continue a multibuffer CRC provide the value
// returned from the last call to Crc32n as the ulInitialCrcValue.
//
// inputs:
//    ulInitialCrc -- initial value for the CRC.
//    pData -- pointer to the data to calculate the CRC for.
//    ulLen -- length of the data to calculate the CRC for.
// outputs:
//    returns -- the CRC of the buffer.
//
//
{

#if 0

    // +-------------------------------------------------------------------+
    // | NOTE: this is the original CRC-32 implementation.  It runs slower |
    // |       than the alternative (below) out of BeDRAM, probably due to |
    // |       the lack of data caching (fetches of the table are slow).   |
    // +-------------------------------------------------------------------+

    static unsigned long Crc32x4_Table[16] = {
      0x00000000, 0x1db71064, 0x3b6e20c8, 0x26d930ac,
      0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
      0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c,
      0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c
    };

    unsigned long n;
    unsigned long t;
    unsigned char *p;
    unsigned long ulCrc= ulInitialCrc;


    for (n = ulLen, p = pData;
         n > 0;
         n--, p++)
    {
        unsigned char c;
        c = *p;                     // gbrab the character.


        t = ulCrc ^ (c & 0x0f);                         // lower nibble
        ulCrc = (ulCrc>>4) ^ Crc32x4_Table[t & 0xf];
        t = ulCrc ^ (c>>4);                             // upper nibble.
        ulCrc = (ulCrc>>4) ^ Crc32x4_Table[t & 0xf];
    }

    return(ulCrc);

#else

    // +-------------------------------------------------------------------+
    // | NOTE: this is an alternative implementation of CRC-32.  It takes  |
    // |       more instructions, but fewer data accesses and actually     |
    // |       runs about 30% faster.                                      |
    // +-------------------------------------------------------------------+


    #define CRC_32_POLYNOMIAL   ((0xDB710641 >> 1) | 0x80000000)

    unsigned long n;
    unsigned short t;
    unsigned char *p;
    unsigned long  ulCrc = ulInitialCrc;


    for (n = ulLen, p = pData;
         n > 0;
         n--, p++)
    {
        int bit;
        for (bit = 0, t = *p;
             bit < 8;
             bit++, t >>= 1)
        {
            if ((ulCrc & 0x0001) ^ (t & 1))
                ulCrc = ((ulCrc >> 1)) ^ CRC_32_POLYNOMIAL;
            else
                ulCrc = (ulCrc >> 1);
        }
    }

    return(ulCrc);

#endif

}
