/*================================================================================

    utilities/src/string_tools.c

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

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

//C include
#include <stdio.h>
#include <stdlib.h>
// File include
#include "utilities/include/string_tools.h"

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
int CORAL_stringTools_seperatedValuesCount(char* myString, char delimiter, int* dataCount)
{
	*dataCount = 0;
	
	int counter = 0;

	char alreadyPrinted = 0;
	
	if(myString == NULL){
		fprintf(stderr, "ERROR: myString is NULL. Can not parse NULL string.\n");
		return 4;
	}
	
	while(*myString)
	{	
		if(delimiter == *myString)
		{
			//found a new item, so increase the count
			counter++;
			//move the pointer forward
			myString++;
			//check for consecutive delimiters
			while(delimiter == *myString)
			{
				if(alreadyPrinted == 0){
					// csm comp has a logging utility, does general CORAL have a logging standard?
					// I would like to use this logging standard for general utility functions like this.
					// Example found below.
					// csmutil_logging(warning, "%s-%d:", __FILE__, __LINE__);
					// csmutil_logging(warning, "  Consecutive delimiter (example: whitespace) detected while parsing string.");
					// csmutil_logging(warning, "  Ignoring consecutive delimiter and continuing onward.");
					// For now, print to stderr
					// We only print this warning once.
					fprintf(stderr, "WARNING: Consecutive delimiter detected while parsing string.\n");
					alreadyPrinted = 1;
				}
				myString++;
			}
		}
		//move the pointer forward
		myString++;
	}
	counter++;
	
	*dataCount = counter;
	
	if(*dataCount == 0){
		//something went very wrong
		fprintf(stderr, "ERROR: dataCount is 0. Expected to be != 0\n");
		return 3;
	}else if(*dataCount >= 1){
		//counted successfully
		return 0;
	}else{
		//negative number, shouldn't happen
		fprintf(stderr, "ERROR: dataCount is negative. Expected to be >= 0\n");
		return 2;
	}
	
	fprintf(stderr, "ERROR: Problem with code. Skipped dataCount check.\n");
	return 1;
}

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
int CORAL_stringTools_nodeCount_xCATSyntax(char* myString, int* dataCount)
{
	char delimiter = ',';
	
	*dataCount = 0;
	
	int counter = 0;

	char alreadyPrinted = 0;
	
	if(myString == NULL || myString[0] == '\0'){
		fprintf(stderr, "ERROR: myString is NULL. Can not parse NULL string.\n");
		return 4;
	}
	
	while(*myString)
	{	
		if(delimiter == *myString)
		{
			//found a new item, so increase the count
			counter++;
			//move the pointer forward
			myString++;
			//check for consecutive delimiters
			while(delimiter == *myString)
			{
				if(alreadyPrinted == 0){
					// csm comp has a logging utility, does general CORAL have a logging standard?
					// I would like to use this logging standard for general utility functions like this.
					// Example found below.
					// csmutil_logging(warning, "%s-%d:", __FILE__, __LINE__);
					// csmutil_logging(warning, "  Consecutive delimiter (example: whitespace) detected while parsing string.");
					// csmutil_logging(warning, "  Ignoring consecutive delimiter and continuing onward.");
					// For now, print to stderr
					// We only print this warning once.
					fprintf(stderr, "WARNING: Consecutive delimiter detected while parsing string.\n");
					alreadyPrinted = 1;
				}
				myString++;
			}
		}
		//Check for a [00-XY] scenario
		if(*myString == '[')
		{
			//We have found a consecutive range
			
			//move past the '['
			myString++;
			
			// goal of this code is to find this number
			int totalNumberInRange = 0;
			// hold the first number in the range, and used to calculate the final total.
			int firstNumberInRange = 0;
			// hold the last number in the range, and used to calculate the final total.
			int lastNumberInRange = 0;
			// used to convert strings into ints.
			char* temp = NULL;
			// used to help alloc the temp string
			int numChars = 0;
			// remember the starting point of the number. needed to help copy the number into  temp string.
			char* startMarker = myString;

			while(*myString != '-')
			{
				myString++;
				numChars++;
			}
			//alloc the appropriate size for the temp string
			temp = (char*)calloc(numChars+1, sizeof(char));
			// copy the chars over into the temp string
			strncpy(temp, startMarker, numChars);
			//convert the string to an int
			firstNumberInRange = atoi(temp);
			
			//Reset helper stuff for the next counting
			//free the temp string
			free(temp);
			temp = NULL;
			//reset numChars to 0, so we can count again
			numChars = 0;
			//move past the '-'
			myString++;
			//set the new start marker
			startMarker = myString;
			
			//start the next counting
			while(*myString != ']')
			{
				myString++;
				numChars++;
			}
			temp = calloc(numChars+1, sizeof(char));
			strncpy(temp, startMarker, numChars);
			lastNumberInRange = atoi(temp);
			free(temp);
			
			//tally the total number of nodes in this nodeRange
			totalNumberInRange = lastNumberInRange - firstNumberInRange + 1;
			//add the totalNumberInRange to the main counter
			counter = counter + totalNumberInRange;
			// remove one
			// because the overall function counts this entire range as one comma seperated value.
			// but we already calulated the proper number in here and added it 
			// so cancel out the future count of this comma seperated value
			counter--;
		}
		//move the pointer forward
		myString++;
	}
	counter++;
	*dataCount = counter;
	
	if(*dataCount == 0){
		//something went very wrong
		fprintf(stderr, "ERROR: dataCount is 0. Expected to be != 0\n");
		return 3;
	}else if(*dataCount >= 1){
		//counted successfully
		return 0;
	}else{
		//negative number, shouldn't happen
		fprintf(stderr, "ERROR: dataCount is negative. Expected to be >= 0\n");
		return 2;
	}
	
	fprintf(stderr, "ERROR: Problem with code. Skipped dataCount check.\n");
	return 1;
}
