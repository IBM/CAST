/*******************************************************************************
 |    usage.h
 |
 |  © Copyright IBM Corporation 2015,2017. All Rights Reserved
 |
 |    This program is licensed under the terms of the Eclipse Public License
 |    v1.0 as published by the Eclipse Foundation and available at
 |    http://www.eclipse.org/legal/epl-v10.html
 |
 |    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
 |    restricted by GSA ADP Schedule Contract with IBM Corp.
 *******************************************************************************/


#ifndef BB_USAGE_H_
#define BB_USAGE_H_

extern int   proxy_GetUsage(const char* mountpoint, BBUsage_t& usage);
extern int   proxy_GetUsageLV(const char* lvuuid, BBUsage_t& usage);

extern int   proxy_BumpBBUsage(dev_t device, uint64_t byteswritten, uint64_t bytesread);
extern int   proxy_GetDeviceUsage(uint32_t devicenum, BBDeviceUsage_t& usage);
extern int   startMonitoringMount(const char* mountpoint, BBUsage_t limits);
extern int   stopMonitoringMount(const char* mountpoint);
extern void* mountMonitorThread(void* ptr);

extern int   proxy_regLV4Usage(const char* mountpoint);
extern int   proxy_deregLV4Usage(const char* mountpoint);

#endif /* BB_USAGE_H_ */
