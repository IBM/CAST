/*================================================================================
   
    csmd/src/daemon/src/csmi_request_handler/helpers/DataAggTest.cc

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
 
================================================================================*/
#include "DataAggregators.h"
#include<cstdio>

/**
 * @file DataAggTest.cc
 * This is a basic commandline program that executes the data aggregators in 
 * "DataAggregators.h" and outputs them to the command line without running 
 * a csmd instance.
 *
 * FOR INTERNAL USE ONLY.
 * REMOVE ME
 */
int main(int argc, char *argv[])
{
    int64_t rx=0, tx=0, gread=0, gwrite=0;
    csm::daemon::helper::GetIBUsage  ( rx,     tx      );
    csm::daemon::helper::GetGPFSUsage( gread, gwrite);
    printf("rx: %ld, tx: %ld\n",  rx, tx);
    printf("gread: %ld, gwrite: %ld\n", gread, gwrite);

    // OCC data.
    int64_t energy = csm::daemon::helper::GetEnergy();
    int32_t pc     = csm::daemon::helper::GetPowerCapacity();
    int32_t psr    = csm::daemon::helper::GetPowerShiftRatio();

    printf("energy: %ld pc: %d psr: %d\n", energy, pc, psr);
    
}
