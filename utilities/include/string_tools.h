/*================================================================================

    utilities/include/string_tools.h

  © Copyright IBM Corporation 2015-2018. All Rights Reserved

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

#ifdef __cplusplus
extern "C" {
#endif

//C includes
#include <inttypes.h>
#include <string.h>
#include <stdint.h>

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

/*
* Author: Nick Buonarota
* Last Edited: August 27, 2018
* Summary: This function takes in a string and counts the number of 'nodes' based on a xCAT node range syntax of '[00-XY]'.
* Parameters:
*   char*     myString:     The string to compare.
*   uint32_t* stringCount:  A pointer to an int. When this function finishes, this will contain
*                           the number of 'seperated values' found in 'myString'.
*   char***   stringArray:  A pointer to an array of strings. When this function finishes, this will contain
*                           an array of 'seperated values' found in 'myString' with 'stringCount' elements. 
*
* Returns: int 
*   0 Success
*   1 ERROR: Generic error. Default.
*/
int CORAL_stringTools_nodeRangeParser(char* myString, uint32_t* stringCount, char*** stringArray);

/*
* Author: Nick Buonarota
* Last Edited: July 11, 2018
* Summary: This function takes in a string and compares it to CSM KEYWORDS.
* Parameters:
*   char*   myString:   The string to compare.
*   int*    compareCode: The result of the comparison.
*
* compareCode values
*   0 no match
*   1 keyword detected but no known match
*   2 #CSM_NULL
*
* Returns: int 
*   0 Success
*   1 ERROR: Generic error. Default.
*   2 ERROR: myString is NULL. Can not parse NULL string.
*   3 ERROR: reached end of program without returning valid data
*/
int CAST_stringTools_CSM_KEYWORD_Compare(char* myString, int* compareCode);

#ifdef __cplusplus
}
#endif