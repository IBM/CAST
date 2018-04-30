/*******************************************************************************
 |    test_GetDeviceUsage.c
 |
 |  The purpose of the progam is to verify that the burst buffer API interfaces,
 |  as defined in bbapi.h, match the actual implementations for those APIs.
 |  The intent is that this program should successfully compile and link.
 |  It is not intended for this program to be run.
 |
 |  � Copyright IBM Corporation 2015,2017. All Rights Reserved
 |
 |    This program is licensed under the terms of the Eclipse Public License
 |    v1.0 as published by the Eclipse Foundation and available at
 |    http://www.eclipse.org/legal/epl-v10.html
 |
 |    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
 |    restricted by GSA ADP Schedule Contract with IBM Corp.
 *******************************************************************************/

#include "bb/include/bbapi.h"
#include "bb/include/bbapiAdmin.h"
/*
typedef struct
{
    uint64_t critical_warning;    ///< Fault register for the SSD
    double   temperature;         ///< Temperature of the SSD in degrees C
    double   available_spare;     ///< Amount of SSD capacity over-provisioning that remains
    double   percentage_used;     ///< Estimate of the amount of SSD life consumed.

    uint64_t data_read;           ///< Number of bytes read from the SSD over the life of the device.
    uint64_t data_written;        ///< Number of bytes written to the SSD over the life of the device.
    uint64_t num_read_commands;   ///< Number of I/O read commands received from the compute node.
    uint64_t num_write_commands;  ///< Number of completed I/O write commands from the compute node.

    uint64_t busy_time;           ///< Amount of time that the I/O controller was handling I/O requests.
    uint64_t power_cycles;        ///< Number of power on events for the SSD.
    uint64_t power_on_hours;      ///< Number of hours that the SSD has power.

    uint64_t unsafe_shutdowns;    ///< Number of unexpected power OFF events.
    uint64_t media_errors;        ///< Count of all data unrecoverable events.
    uint64_t num_err_log_entries; ///< Number of error log entries available.
} BBDeviceUsage_t;
*/


void printDevUsage(BBDeviceUsage_t pD){
printf("crit warn=%ld  ",pD.critical_warning);
printf("temp C=%G ",pD.temperature);
printf("spare%%=%G ",pD.available_spare);
printf("used%%=%G ",pD.percentage_used);

printf("read(B)=%ld  ",pD.data_read);
printf("write(B)=%ld  ",pD.data_written);
printf("read cmds=%ld  ",pD.num_read_commands);
printf("write cmd=%ld  ",pD.num_write_commands);

printf("busy=%ld ",pD.busy_time);
printf("pow cycles=%ld  ",pD.power_cycles);
printf("pow(hrs)=%ld  ",pD.power_on_hours);
printf("unsafe_shutdowns=%ld ",pD.unsafe_shutdowns);
printf("errors=%ld ",pD.media_errors);
printf("errlogs=%ld  ",pD.num_err_log_entries);
}

//comparison: 
//nvme smart-log /dev/nvme0n1
//https://www.google.com/search?q=nvme+man+page&ie=utf-8&oe=utf-8 
//http://manpages.ubuntu.com/manpages/xenial/man1/nvme-smart-log.1.html
//
/*
sudo nvme smart-log /dev/nvme0n1 
[sudo] password for meaho: 
Smart Log for NVME device:nvme0n1 namespace-id:ffffffff
critical_warning                    : 0
temperature                         : 55 C
available_spare                     : 100%
available_spare_threshold           : 10%
percentage_used                     : 0%
data_units_read                     : 216,801,689
data_units_written                  : 283,451,594
host_read_commands                  : 217,439,678
host_write_commands                 : 4,052,930,000
controller_busy_time                : 1,461
power_cycles                        : 62
power_on_hours                      : 2,817
unsafe_shutdowns                    : 27
media_errors                        : 0
num_err_log_entries                 : 0
Warning Temperature Time            : 0
Critical Composite Temperature Time : 0
Temperature Sensor 1                : 55 C
Temperature Sensor 2                : 52 C
Temperature Sensor 3                : 52 C

[1735][meaho@c650f06p25:~/bluecoral/work/bb/tests/bin]$ ./test_GetDeviceUsage 
crit warn=0	temp C=3.4766E-310	spare=3.47665E-310	% used=5.43672E-312	read(B)=110929368576000	write(B)=144913421824000	read cmds=217319190	write cmd=4052585455
busy=1459	pow cycles=62	pow(hrs)=2802	unsafe_shutdowns=27	errors=0	errlogs=0

*/
int main(int argc, char** argv)
{
    int rc;
    uint32_t uint32 = 0;

    BBERRORFORMAT l_ErrorFormat = (BBERRORFORMAT)0;

    size_t l_Size = (size_t)0;
    BBDeviceUsage_t l_DeviceUsage;
    char* l_CharArrayPtr = 0;

    rc=0;
    rc=BB_InitLibrary(uint32, BBAPI_CLIENTVERSIONSTR);
    rc=BB_GetLastErrorDetails(l_ErrorFormat, &l_Size, l_Size, l_CharArrayPtr);
    rc=BB_GetDeviceUsage(uint32, &l_DeviceUsage);
    printDevUsage(l_DeviceUsage);
    rc=BB_TerminateLibrary();

    return rc;
}
