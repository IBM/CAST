/*================================================================================

    utilities/include/string_tools.h

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

/*
* Authors: 
*   Nick Buonarota (nbuonar@us.ibm.com)
*
*/

//C includes
#include <string.h>

/*
* Author: Nick Buonarota
* Last Edited: Nov. 1, 2016
* Summary: This function takes in a string and counts the number of 'seperated values' based on a delimiter.
* Parameters:
*   char*   myString:   The string to parse through.
*   char    delimiter:  The delimiter to check for inside the string. (Example: ',' or ' ')
*   int*    dataCount:  A pointer to an int. When this function finishes, this will contain
*                       the number of 'seperated values' found in 'myString'.
* Returns: int 
*   0 Success
*   1 ERROR: Problem with code. Skipped dataCount check.
*   2 ERROR: dataCount is negative. Expected to be >= 0
*   3 ERROR: dataCount is 0. Expected to be != 0
*   4 ERROR: myString is NULL. Can not parse NULL string.
*/
int CORAL_stringTools_seperatedValuesCount(char* myString, char delimiter, int* dataCount);

/*
* Author: Nick Buonarota
* Last Edited: April 19, 2017
* Summary: This function takes in a string and counts the number of 'nodes' based on a xCAT syntax of ',' and '[00-XY]'.
* Parameters:
*   char*   myString:   The string to parse through.
*   int*    dataCount:  A pointer to an int. When this function finishes, this will contain
*                       the number of 'nodes' found in 'myString'.
* Returns: int 
*   0 Success
*   1 ERROR: Problem with code. Skipped dataCount check.
*   2 ERROR: dataCount is negative. Expected to be >= 0
*   3 ERROR: dataCount is 0. Expected to be != 0
*   4 ERROR: myString is NULL. Can not parse NULL string.
*/
int CORAL_stringTools_nodeCount_xCATSyntax(char* myString, int* dataCount);