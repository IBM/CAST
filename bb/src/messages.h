/*******************************************************************************
 |    messages.h
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


typedef struct BB_Attributes_s {
    int responseHandle;
    int whoami;
    int instance;
    int resultCode;
    int newpathname;
    int userid;
    int groupid;
    int mountpoint;
    int mountsize;
    int numcontrib;
    int tag;
    int contrib;
    int transferHandle;
    int files;
    int totalBytesRead;
    int totalBytesWritten;
    int burstBytesRead;
    int burstBytesWritten;
    int localBytesRead;
    int localBytesWritten;
    int keys;
    int values;
    int extents;
    int devicenum;
    int critical_warning;
    int temperature;
    int available_spare;
    int percentage_used;
    int data_read;
    int data_written;
    int num_read_commands;
    int num_write_commands;
    int busy_time;
    int power_cycles;
    int power_on_hours;
    int unsafe_shutdowns;
    int media_errors;
    int num_err_log_entries;
    int newowner;
    int newgroup;
    int newmode;
    int pathname;
    int version;
    int knownSerials;
    int nackSerials;
    int localReadCount;
    int localWriteCount;
} BB_Attributes;
