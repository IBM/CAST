#================================================================================
#   
#    include/functions.sh
# 
#  © Copyright IBM Corporation 2015-2018. All Rights Reserved
#
#    This program is licensed under the terms of the Eclipse Public License
#    v1.0 as published by the Eclipse Foundation and available at
#    http://www.eclipse.org/legal/epl-v10.html
#
#    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
#    restricted by GSA ADP Schedule Contract with IBM Corp.
# 
#================================================================================

# ----------------------------------------------------------------
# check_all_output
# Input: Set of strings expected in command line API output  
# Functionality: This will return 1 if any of the input strings 
#		 are not found, 0 if they are all found
# ----------------------------------------------------------------
check_all_output () {
        for arg in "$@"
        do
                cat ${TEMP_LOG} | grep "${arg}" > /dev/null
                if [ $? -ne 0 ]
                then
			return 1
                fi
        done
        return 0
}

# ----------------------------------------------------------------
# shutdown_daemons
# Inputs: none
# Functionality: Shut down daemons, clean database.
# ----------------------------------------------------------------
shutdown_daemons () {
        systemctl stop csmd-master
        xdsh ${AGGREGATOR_A} "systemctl stop csmd-aggregator"
        xdsh utility "systemctl stop csmd-utility"
        xdsh csm_comp "systemctl stop csmd-compute"
        echo "y" | /opt/ibm/csm/db/csm_db_script.sh -d csmdb
        /opt/ibm/csm/db/csm_db_script.sh -n csmdb
}

# ----------------------------------------------------------------
# check_return_exit 
# Input 1: return code from command line API 
# Input 2: expected return code from command line API  
# Input 3: Test Case name
# Functionality: This will exit the bucket on unexpected 
#		 return code
# ----------------------------------------------------------------
check_return_exit () {
        if [ $1 -ne $2 ]
        then
                #echo -e "$3:\tFAILED" >> ${LOG}
                printf "%-100s %8s\n" "$3:" "FAILED" >> ${LOG}
                printf "\n$3\n" >> ${TEMP_LOG}
		printf "Expected RC: $2\n" >> ${TEMP_LOG}
		printf "Actual RC: $1\n" >> ${TEMP_LOG}
		exit 1
        else
                #echo -e "$3:\tPASS" >> ${LOG}
                printf "%-100s %8s\n" "$3:" "PASS" >> ${LOG}
        fi
}

# ----------------------------------------------------------------
# check_return_exit 
# Input 1: return code from command line API 
# Input 2: Invalid Error Code (
# Input 3: Test Case name
# Functionality:Verifies that input 1 doesn't match input 2. If the
#    two match fail the test.
# ----------------------------------------------------------------
check_return_error () {
        if [ $1 -eq $2 ]
        then
            printf "%-100s %8s\n" "$3:" "FAILED" >> ${LOG}
            printf "\n$3\n" >> ${TEMP_LOG}
		    printf "Invalid RC: $1\n" >> ${TEMP_LOG}
		    exit 1
        else
            printf "%-100s %8s\n" "$3:" "PASS" >> ${LOG}
        fi
}

# ----------------------------------------------------------------
# check_return_flag
# Input 1: return come from command line API
# Input 2: Test Case name
# Functionality: This will flag the test case as failed on a 
#		 non-zero return code. Additionally, the contents
#		 of the temp log will be appended to a flag log.
# ----------------------------------------------------------------
check_return_flag () {
        if [ $1 -ne 0 ]
        then
                FLAGS+="\n$2"
                #echo -e "$2:\tFAILED" >> ${LOG}
                printf "%-100s %8s\n" "$2:" "FAILED" >> ${LOG}
		echo "$2" >> ${FLAG_LOG}
                cat ${TEMP_LOG} >> ${FLAG_LOG}
                echo -e "\n" >> ${FLAG_LOG}
        else
                #echo -e "$2:\tPASS" >> ${LOG}
		printf "%-100s %8s\n" "$2:" "PASS" >> ${LOG}
        fi
}

# ----------------------------------------------------------------
# check_return_flag_nz
# Input 1: return come from command line API
# Input 2: expected return code from command line API
# Input 3: Test Case name
# Functionality: This will flag the test case as failed unless
#		 a specified non-zero return code is returned.
#                Additionally, the contents of the temp log will 
#                be appended to a flag log.
# ----------------------------------------------------------------
check_return_flag_nz () {
        if [ $1 -ne $2 ]
        then
                FLAGS+="\n$3"
                #echo -e "$2:\tFAILED" >> ${LOG}
                printf "%-100s %8s\n" "$3:" "FAILED" >> ${LOG}
                echo "$3" >> ${FLAG_LOG}
                cat ${TEMP_LOG} >> ${FLAG_LOG}
                echo -e "\n" >> ${FLAG_LOG}
        else
                #echo -e "$3:\tPASS" >> ${LOG}
                printf "%-100s %8s\n" "$3:" "PASS" >> ${LOG}
        fi
}
