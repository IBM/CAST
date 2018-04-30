/*******************************************************************************
 |    txpClient.cc
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


#include <algorithm>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "Common.h"
#include "Log.h"
#include "Msg.h"

// Log for this program...
txp::Log d_log(txp::Log::LOGFILE, "/bglhome/dherms/git/bluecoral/work/transport/tests/txpClient.log", std::ofstream::out | std::ofstream::app);

void error(const char *msg) {
    perror(msg);
    exit(0);
}

int main(int argc, char *argv[])
{
    const char DEFAULT_HOST_NAME[] = "localhost";
    const int BufferLen = 256;

	int l_RC = 0;
    char l_HostName[256] = {'\0'};
    char l_LogBuffer[133] = {'\0'};
    int l_FD, l_Port;
    int32_t l_NumBytes;

	// Process the passed parameters...
    if (argc > 3) {
     	fprintf(stderr, "Usage %s: Only 2 parameters are allowed.  Extra parameters are ignored.\n", argv[0]);
    }
   	if (argc > 2) {
    	if (txp::isDigits(argv[2])) {
    		l_Port = atoi(argv[2]);
    	} else {
            fprintf(stderr, "Usage %s: Port specification is optional, but must be numeric. If not specified, 2048 is the default.\n", argv[0]);
      		exit(EXIT_FAILURE);
    	}
    } else {
        l_Port = 2048;
    }
   	if (argc > 1) {
   		txp::strCpy(l_HostName, argv[1], sizeof(l_HostName));
    } else {
   		txp::strCpy(l_HostName, DEFAULT_HOST_NAME, sizeof(l_HostName));
    }

	// Initialize logging/flightlog
	pt::ptree l_TxpConfig;
	char l_ConfigFile[1024] = {'\0'};
	if (!txp::Msg::readConfigFile(l_TxpConfig, l_ConfigFile, sizeof(l_ConfigFile))) {
	    initializeLogging("txp.log", l_TxpConfig);
		std::string l_FlightLog = l_TxpConfig.get("txp.flightlog", "none").c_str();
    	l_RC = FL_CreateAll(l_FlightLog.c_str());
    	if (l_RC) {
			LOG(bb,error) << "Unable to initialize flightlog (" << l_FlightLog << ") l_RC=" << l_RC;
			exit(-1);
    	}
    } else {
		printf("Configuration file could not be found/read\n");
		exit(-1);
    }

	// Log invocation...
    d_log.open();
	d_log.write(l_LogBuffer);
	snprintf(l_LogBuffer, sizeof(l_LogBuffer), "argc = %d, l_HostName = %s, l_Port = %d", argc, l_HostName, l_Port);
	d_log.write(l_LogBuffer);

    struct sockaddr_in l_ServerAddr;
    struct hostent* l_Server;

	// Open a socket...
    l_FD = socket(AF_INET, SOCK_STREAM, 0);
    if (l_FD < 0) {
		snprintf(l_LogBuffer, sizeof(l_LogBuffer), "%s: Error opening socket, line %d", argv[0], __LINE__);
		d_log.write(l_LogBuffer);
        error(l_LogBuffer);
    }

	// Resolve host...
    l_Server = gethostbyname(l_HostName);
    if (l_Server == NULL) {
		snprintf(l_LogBuffer, sizeof(l_LogBuffer), "%s: host %s not found, line %d", argv[0], l_HostName, __LINE__);
		d_log.write(l_LogBuffer);
        error(l_LogBuffer);
    }

	// Connect to the server...
    bzero((char *) &l_ServerAddr, sizeof(l_ServerAddr));
    l_ServerAddr.sin_family = AF_INET;
    bcopy((char *)l_Server->h_addr,
          (char *)&l_ServerAddr.sin_addr.s_addr,
          l_Server->h_length);
    l_ServerAddr.sin_port = htons(l_Port);
    if (connect(l_FD,(struct sockaddr *) &l_ServerAddr, sizeof(l_ServerAddr)) < 0) {
		snprintf(l_LogBuffer, sizeof(l_LogBuffer), "%s: connection to server %s failed, line %d", argv[0], l_HostName, __LINE__);
		d_log.write(l_LogBuffer);
        error(l_LogBuffer);
    }

    // Prompt for a message to send...
    printf("Enter message: ");

    // Prepare to send message...
    char l_Buffer[BufferLen];
    bzero(l_Buffer, BufferLen);
    fgets(l_Buffer, BufferLen-1, stdin);

	std::string l_Message(l_Buffer);
	l_Message.erase(std::remove(l_Message.begin(), l_Message.end(), '\n'), l_Message.end());

	char* l_Data = 0;
	uint32_t l_DataLen = 0;
    if (!txp::isDigits(l_Buffer)) {
    	txp::Id l_Id = (txp::Id)(atoi(l_Buffer));
    	if (txp::Msg::isMsgIdValid(l_Id)) {
    		if (txp::Msg::isMsgIdSupported(l_Id)) {
	    		txp::Msg l_Msg(l_Id);
	    		l_Msg.allocateHeapBuffer();
				int32_t l_SerializedLen = l_Msg.serializeWithValuesToHeapBuffer();
				if (l_SerializedLen>0) {
		    		l_Data = l_Msg.getDataBufferPtr();
		    		l_DataLen = l_Msg.getSerializedLen();
                    snprintf(l_LogBuffer, sizeof(l_LogBuffer), "-> Msg(%s), msgnbr %d", (char*)l_Message.c_str(), l_Msg.getMsgNumber());
					d_log.write(l_LogBuffer);
		    	} else {
		    		l_RC = (int)l_SerializedLen;
					snprintf(l_LogBuffer, sizeof(l_LogBuffer), "serializeWithValuesToHeapBuffer() failed for %s, RC = %d", (char*)l_Message.c_str(), l_RC);
					d_log.write(l_LogBuffer);
		    	}
			} else {
		    	l_RC = -10;
				snprintf(l_LogBuffer, sizeof(l_LogBuffer), "Id::isSupported() failed for '%s'", (char*)l_Message.c_str());
				d_log.write(l_LogBuffer);
			}
		} else {
		    l_RC = -11;
			snprintf(l_LogBuffer, sizeof(l_LogBuffer), "Id::isValid() failed for '%s'", (char*)l_Message.c_str());
			d_log.write(l_LogBuffer);
		}
	} else {
		l_RC = -12;
		snprintf(l_LogBuffer, sizeof(l_LogBuffer), "isDigits() failed for '%s'", (char*)l_Message.c_str());
		d_log.write(l_LogBuffer);
	}

    // Send message...
	if (!l_Data) {
		l_Data = l_Buffer;
		l_DataLen = strlen(l_Buffer);
        snprintf(l_LogBuffer, sizeof(l_LogBuffer), "-> text('%s')", (char*)l_Message.c_str());
		d_log.write(l_LogBuffer);
	}

    l_NumBytes = write(l_FD, l_Data, l_DataLen);
    if (l_NumBytes < 0) {
		snprintf(l_LogBuffer, sizeof(l_LogBuffer), "%s: Negative number of bytes written on write, %d, line %d", argv[0], l_NumBytes, __LINE__);
		d_log.write(l_LogBuffer);
    	error(l_LogBuffer);
    }

	// Receive response...
    bzero(l_Buffer, BufferLen);
    l_NumBytes = read(l_FD, l_Buffer, BufferLen-1);
    if (l_NumBytes < 0) {
		snprintf(l_LogBuffer, sizeof(l_LogBuffer), "%s: Negative number of bytes read, %d, line %d", argv[0], l_NumBytes, __LINE__);
		d_log.write(l_LogBuffer);
    	error(l_LogBuffer);
	}

	// Process the response...
	txp::Msg* l_ResponseMsg = 0;
	txp::HeapBuffer l_HeapBuffer(l_Buffer, sizeof(l_Buffer));

	l_HeapBuffer.dump_buffer_raw(d_log, sizeof(l_Buffer), "Response buffer");

	std::string l_ResponseMessage(l_Buffer);
	l_ResponseMessage.erase(std::remove(l_ResponseMessage.begin(), l_ResponseMessage.end(), '\n'), l_ResponseMessage.end());

	bool l_txpMsgReceived = false;
	char l_Response[256] = {'\0'};

	l_Data = 0;
	l_DataLen = 0;
	l_RC = txp::Msg::deserializeToMsg(l_ResponseMsg, &l_HeapBuffer, l_NumBytes);
	if (!l_RC) {
    	txp::Id l_Id(l_ResponseMsg->getMsgId());
    	if (txp::Msg::isMsgIdValid(l_Id)) {
    		if (txp::Msg::isMsgIdSupported(l_Id)) {
    			l_txpMsgReceived = true;
				char l_Temp[64] = {'\0'};
				txp::Msg::msgIdToChar(l_Id, l_Temp, sizeof(l_Temp));

				// Log received message...
                snprintf(l_Response, sizeof(l_Response), "<- Msg(%s), response msgnbr %d to request msgnbr %d", l_Temp, l_ResponseMsg->getMsgNumber(), l_ResponseMsg->getRequestMsgNumber());
				d_log.write(l_Response);

			} else {
				l_RC = -13;
				snprintf(l_LogBuffer, sizeof(l_LogBuffer), "Id::isSupported() failed for response message '%s'", (char*)l_ResponseMessage.c_str());
				d_log.write(l_LogBuffer);
			}
		} else {
			l_RC = -14;
			snprintf(l_LogBuffer, sizeof(l_LogBuffer), "Id::isValid() failed for response message '%s'", (char*)l_ResponseMessage.c_str());
			d_log.write(l_LogBuffer);
		}
	} else {
		l_RC = -15;
		snprintf(l_LogBuffer, sizeof(l_LogBuffer), "deserializeToMsg() failed for response message '%s', RC=%d", (char*)l_ResponseMessage.c_str(), l_RC);
		d_log.write(l_LogBuffer);
	}

	if (!l_txpMsgReceived) {
		// Log received response...
        snprintf(l_LogBuffer, sizeof(l_LogBuffer), "<- text('%s')", (char*)l_ResponseMessage.c_str());
		d_log.write(l_LogBuffer);
	}

	if (l_FD) {
	    close(l_FD);
    }

    return l_RC;
}
