/*================================================================================

    utilities/src/string_tools.c

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
*   2 ERROR: noderange not present. generic
*   3 ERROR: noderange not properly formatted. node range digits precision don't match. ie: 003 VS 05 ie: node[005-98]
*   4 ERROR: noderange not properly formatted. node range is negative. ie: 100 VS 001 ie: node[100-001]
*   5 ERROR: noderange not present. opening bracket '[' was not found
*   6 ERROR: noderange not present. mid range dash '-' was not found
*   7 ERROR: noderange not present. ending bracket ']' was not found
*/
int CORAL_stringTools_nodeRangeParser(char* myString, uint32_t* stringCount, char*** stringArray)
{

	// Function Variables
	// pointer location of the opening '[' bracket for the node range
	char* bracket_pointer = NULL;

	//search for the opening bracket
	bracket_pointer = strchr(myString,'[');

	if(bracket_pointer == NULL)
	{
		//No range was found
		//fprintf(stderr, "ERROR: opening bracket '[' was not found.\n");
		//fprintf(stderr, "ERROR: determined noderange not present.\n");
		return 5;
	}else{
		//range was found

		// === Function Variables ===
		// number of reserved characters before the opening bracket. aka, base node name
		int reserved_chars = 0;
		//number of digits in the node range, calculated later by subtracting pointer locations
		int range_digits = 0;
		//number of digits in the node range before '-', used to compare for a quality check
		int range_digits_begin = 0;
		//number of digits in the node range after '-', used to compare for a quality check
		int range_digits_end = 0;
		//used to later construct the full node name incrementally
		int range_counter = 0;
		char range_counter_buffer[512];
		// a reused string that will contain a full node name.
		char* base_node_name = NULL;
		// ===========================

		//record the number of characters in the string before the '['
		//aka, the base node name
		reserved_chars = bracket_pointer-myString;

		//find the dash
		char* dash_pointer = NULL;
		dash_pointer = strchr(bracket_pointer,'-');

		if(dash_pointer == NULL)
		{
			//No range was found
			//fprintf(stderr, "ERROR: mid range dash '-' was not found.\n");
			//fprintf(stderr, "ERROR: determined noderange not present.\n");
			return 6;
		}else
		{
			//good - found more format

			//calculate the range digits
			range_digits = (dash_pointer-myString+1) - (bracket_pointer-myString+1) -1;
			range_digits_begin = range_digits;
		}

		//find the end bracket
		//only needed for testing quality.
		char* end_bracket_pointer = NULL;
		end_bracket_pointer = strchr(myString,']');

		if(end_bracket_pointer == NULL)
		{
			//No range was found
			//fprintf(stderr, "ERROR: ending bracket ']' was not found.\n");
			//fprintf(stderr, "ERROR: determined noderange not present.\n");
			return 7;
		}

		//calculate the range digits
		range_digits = (end_bracket_pointer-myString+1) - (dash_pointer-myString+1) -1;
		range_digits_end = range_digits;

		//Quality check to make sure there is no mismatch between the opening range and closing range
		if(range_digits_begin != range_digits_end)
		{
			//illegal format detected.
			//fprintf(stderr, "ERROR: node range digits precision don't match.\n");
			//fprintf(stderr, "ERROR: range_digits_begin: %i\n", range_digits_begin);
			//fprintf(stderr, "ERROR: range_digits_end: %i\n", range_digits_end);
			//fprintf(stderr, "ERROR: determined noderange was not properly formatted.\n");
			return 3;
		}

		char* first_node_digit = NULL;

		first_node_digit = (char*)calloc(range_digits+1,sizeof(char));
		//memcpy the stuff out of myString
		memcpy(first_node_digit, bracket_pointer+1, range_digits);

		//grab the first node digit. save to int.
		//use this later to find total number of nodes
		int first_node_digit_int = 0;
		first_node_digit_int = atoi(first_node_digit);

		free(first_node_digit);

		//find end of range
		char* last_node_digit = NULL;
		last_node_digit = (char*)calloc(range_digits+1,sizeof(char));
		//memcpy the stuff out of myString
		memcpy(last_node_digit, dash_pointer+1, range_digits);

		//grab the last node digit. save to int. use this later to find total number of nodes
		int last_node_digit_int = 0;
		last_node_digit_int = atoi(last_node_digit);

		free(last_node_digit);

		int totalNumberOfNodesInNodeRange = 0;
		totalNumberOfNodesInNodeRange = last_node_digit_int - first_node_digit_int + 1;

		//make sure the begining of the node range is smaller than the end.
		if(totalNumberOfNodesInNodeRange < 0)
		{
			//illegal format detected.
			//fprintf(stderr, "ERROR: node range is negative: %i\n", totalNumberOfNodesInNodeRange);
			//fprintf(stderr, "ERROR: determined noderange was not properly formatted.\n");
			return 4;
		}

		//now that you know the total number of nodes in the range
		//set the num nodes
		*stringCount = totalNumberOfNodesInNodeRange;
		//allocate the array
		//so we can start filling up the node names
		*stringArray = (char**)calloc(*stringCount, sizeof(char*));

		//grab the base node name for the range
		base_node_name = (char*)calloc(reserved_chars+1, sizeof(char));

		memcpy(base_node_name, myString, reserved_chars);

		//fill up all the node names in the range

		//set up the first range counter
		range_counter = first_node_digit_int;

		char* full_node_name = NULL;

		int i = 0;
		for(i = 0; i < *stringCount; i++)
		{
			//reserve a node name with enough space for characters
			// the reserved chars from above, plus the range digits, plus one for null terminated char
			// example: c123f01p[01-10]
			// reserved chars: c123f01p = 8
			// range_digits: 01 = 2
			// total space for malloc = 11
			full_node_name = (char*)calloc(reserved_chars+range_digits+1,sizeof(char));

			//memcpy the base node name
			memcpy(full_node_name, base_node_name, reserved_chars);

			//temp var for help
			sprintf(range_counter_buffer, "%0*d", range_digits, range_counter);

			//memcpy the node digits
			memcpy(full_node_name + reserved_chars, range_counter_buffer, range_digits);

			(*stringArray)[i] = strdup(full_node_name);

			range_counter++;
			free(full_node_name);
		}
        free(base_node_name);
	}

	//ToDo: What about a case where the node range has text?
	//Example: node[001-f45]
	//it currently returns as node range is negative, improperly formatted.
	//prevents an error, but could be more specific

	return 0;
}


/*
* Author: Nick Buonarota
* Last Edited: July 11, 2018
* Summary: This function takes in a string and compares it to CSM KEYWORDS.
* Parameters:
*   char*   myString:    The string to compare.
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
int CAST_stringTools_CSM_KEYWORD_Compare(char* myString, int* compareCode)
{
	//reset to 0 (no match), just to be safe.
	*compareCode = 0;

	//Get some early returns out of the way.
	if(myString[0] == '\0')
	{
		//The string passed in was NULL.
		//Return an error code.
		return 2;
	}

	//check for special keywords that begin with '#'
	if(myString[0] != '#')
	{
		//CSM Keywords all begin with '#'
		//If one isn't found then its not a keyword.
		//return success with no match.
		*compareCode = 0;
		return 0;
	}

	//If we made it this far, then we know we have a 'KEYWORD'

	//a special CSM API keyword was found
	//go through all keywords
	// which keyword? -- too bad I can't use a switch statement
	if(strncmp ("#CSM_NULL", myString, 9) == 0)
	{
		//Special keyword "#CSM_NULL" was found.
		//this means reset Database field to NULL
		*compareCode = 2;
		return 0;

	}else{
		//final default case

		//unknown keyword
		//treat as normal value? -- well that's what i'll do for now

		//CSM Keywords all begin with '#'
		//we found the '#' but it didn't match any of our keywords.
		// i could treat this as a 'no match'
		// but i wanted to specfify that we found a 'no match' and a '#'
		// in case the caller wants to do something with this info.

		//return success with no match and '#'.
		*compareCode = 1;
		return 0;
	}

	// we should not have gotten here. something else should have triggered.
	return 3;
}