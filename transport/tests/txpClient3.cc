/*******************************************************************************
 |    txpClient3.cc
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

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "Msg.h"

// DEBUG values
// 0 - Connection accepted,
//     log entry after every l_NumberOfMsgsLogEntry msgs,
//     connection closed.
// 1 - Level 0 plus each send and receive response
// 2 - Level 1 plus additional trace points
// 3 - Level 2 plus logging of the first
//     256 bytes of the receive buffer
// 4 - Level 3 plus msg dumps
uint32_t DEBUG = 3;

txp::DeserializeOption l_DeserializeOption(txp::DO_NOT_COPY_DATA);
//txp::DeserializeOption l_DeserializeOption(txp::COPY_DATA_TO_OBJECT);

// Log for this program...
// NOTE:  Open the log file after daemonize().  daeomize closes all open files...
txp::Log d_log(txp::Log::LOGFILE, "/bglhome/dherms/git/bluecoral/work/transport/tests/txpClient.log", std::ofstream::out | std::ofstream::app,
	           txp::Log::OPEN); //, txp::Log::ALL_LOGLEVEL);

int l_FD;
char* l_Buffer;

const char DEFAULT_HOST_NAME[] = "localhost";
const uint64_t l_NumberOfMsgsLogEntry = 250000;

struct sockaddr_in l_ServerAddr;
struct hostent* l_Server;

char l_HostName[256] = {'\0'};
char l_LogBuffer[133] = {'\0'};

int l_Daemonize, l_Port;
int l_uSecondsBetweenSends;
uint64_t l_NumberOfMsgs, l_NumberOfMsgsSent = 0;

const int LOW_FUSE_OP = 1;
const int HIGH_FUSE_OP = 44;
const int LOW_ATTRIBUTE_NAME = 1;
const int HIGH_ATTRIBUTE_NAME = 78;
const int LOW_NUMBER_OF_ATTRIBUTES = 0;
const int HIGH_NUMBER_OF_ATTRIBUTES = 50;
//const int LOW_NUMBER_OF_ATTRIBUTES = 1;
//const int HIGH_NUMBER_OF_ATTRIBUTES = 1;
//const int HIGH_NUMBER_OF_ATTRIBUTES = 192;
const int LOW_CHAR_LENGTH = 1;
const int HIGH_CHAR_LENGTH = 256;

// Test data
char TestData[257]  = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
                       0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
                       0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
                       0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
                       0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,
                       0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F,
                       0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F,
                       0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F,
                       0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F,
                       0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F,
                       0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF,
                       0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF,
                       0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF,
                       0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF,
                       0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF,
                       0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF,
                      };


void cleanUp() {
    if (l_FD) {
        close(l_FD);
    }

    if (l_Buffer) {
        char* l_TempBuffer = l_Buffer;
        l_Buffer = 0;
        delete[] l_TempBuffer;
    }

    return;
}


void error(const char *msg) {
    perror(msg);
    cleanUp();
    exit(0);
}


static void daemonize() {
    pid_t l_Pid;

    // Fork off the parent process
    l_Pid = fork();
    if (l_Pid < 0) {
        exit(EXIT_FAILURE);
    }

    // Let the parent terminate
    if (l_Pid > 0) {
        exit(EXIT_SUCCESS);
    }

    // On success, the child process becomes session leader
    if (setsid() < 0) {
        exit(EXIT_FAILURE);
    }

    // Catch, ignore, and handle signals  Implement?? @@DLH
    signal(SIGCHLD, SIG_IGN);
    signal(SIGHUP, SIG_IGN);

    // Fork off for the second time
    l_Pid = fork();
    if (l_Pid < 0) {
        exit(EXIT_FAILURE);
    }

    // On success, let the parent terminate
    if (l_Pid > 0) {
        exit(EXIT_SUCCESS);
    }

    // Set new file permissions
    umask(0);

    // Change the working directory
    chdir("/bglhome/dherms/git/bluecoral/work/transport/tests/bin");

    // Close all open file descriptors
    int l_TempFD;
    for (l_TempFD = sysconf(_SC_OPEN_MAX); l_TempFD>0; l_TempFD--) {
        close (l_TempFD);
    }
}


void processArgs(int argc, char *argv[]) {
    if (argc > 6) {
        fprintf(stderr, "Usage %s: Only 5 parameters are allowed.  Extra parameters are ignored.\n", argv[0]);
    }

    if (argc > 5) {
        if (txp::isDigits(argv[5])) {
            l_Port = atoi(argv[5]);
        } else {
            fprintf(stderr, "Usage %s: Port specification is optional, but must be numeric. If not specified, 2048 is the default.\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    } else {
        l_Port = 2048;
    }
    if (argc > 4) {
        txp::strCpy(l_HostName, argv[4], sizeof(l_HostName));
    } else {
        txp::strCpy(l_HostName, DEFAULT_HOST_NAME, sizeof(l_HostName));
    }

    if (argc > 3) {
        if (txp::isDigits(argv[3])) {
            l_uSecondsBetweenSends = atoi(argv[3]);
        } else {
            fprintf(stderr, "Usage %s: uSecondsBetweenSends specification is optional, but must be numeric. If not specified, 0 (no delay between sends) is the default.\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    } else {
        l_uSecondsBetweenSends = 0;
    }

    if (argc > 2) {
        if (txp::isDigits(argv[2])) {
            l_NumberOfMsgs = atoi(argv[2]);
        } else {
            fprintf(stderr, "Usage %s: NumberOfMsgs specification is optional, but must be numeric. If not specified, 0 (no maximum) is the default.\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    } else {
        l_NumberOfMsgs = 0;
    }

    if (argc > 1) {
        if (txp::isDigits(argv[1])) {
            l_Daemonize = atoi(argv[1]);
        } else {
            fprintf(stderr, "Usage %s: Daemonize specification optional, must be numeric. If not specified, 0 (false) is the default.\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    } else {
        l_Daemonize = 0;
    }

    return;
}


int connectToServer(int argc, char *argv[]) {

    int l_FD = socket(AF_INET, SOCK_STREAM, 0);
    if (l_FD < 0) {
        snprintf(l_LogBuffer, sizeof(l_LogBuffer), "%s: Error opening socket, line %d", argv[0], __LINE__);
        d_log.write(l_LogBuffer, txp::Log::ERROR);
        error(l_LogBuffer);
    }

    // Resolve host...
    l_Server = gethostbyname(l_HostName);
    if (l_Server == NULL) {
        snprintf(l_LogBuffer, sizeof(l_LogBuffer), "%s: host %s not found, line %d", argv[0], l_HostName, __LINE__);
        d_log.write(l_LogBuffer, txp::Log::ERROR);
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
        d_log.write(l_LogBuffer, txp::Log::ERROR);
        error(l_LogBuffer);
    }

    return l_FD;
}


txp::Id getMsgId() {
    int32_t l_PrevIdValue, l_IdValue = (rand() % (HIGH_FUSE_OP - LOW_FUSE_OP + 1)) + LOW_FUSE_OP;
    while (!txp::Msg::isMsgIdValid(l_IdValue)) {
        l_PrevIdValue = l_IdValue;
        l_IdValue = (rand() % (HIGH_FUSE_OP - LOW_FUSE_OP + 1)) + LOW_FUSE_OP;
        if (DEBUG > 2) {
            snprintf(l_LogBuffer, sizeof(l_LogBuffer), "Retry on generating Id...  Old value %d, new value %d...", l_PrevIdValue, l_IdValue);
            d_log.write(l_LogBuffer, txp::Log::INFO);
        }
    }

    return (txp::Id)l_IdValue;
}


int32_t getNumberOfAttributes(unsigned int pValue) {
    return ((rand() % (HIGH_NUMBER_OF_ATTRIBUTES - LOW_NUMBER_OF_ATTRIBUTES + 1)) + LOW_NUMBER_OF_ATTRIBUTES);
}


int32_t getCharLength(unsigned int pValue) {
    return ((rand() % (HIGH_CHAR_LENGTH - LOW_CHAR_LENGTH + 1)) + LOW_CHAR_LENGTH);
}


txp::AttributeName getAttrName(unsigned int pValue) {
    return (txp::AttributeName)((rand() % (HIGH_ATTRIBUTE_NAME - LOW_ATTRIBUTE_NAME + 1)) + LOW_ATTRIBUTE_NAME);
}


int main(int argc, char *argv[]) {
    int l_RC = 0;

    l_Buffer = new char[txp::DEFAULT_BUFFER_SIZE];

    time_t l_Seconds;
    time(&l_Seconds);
    srand((int32_t) l_Seconds);

    char l_Temp[64] = {'\0'};

    // Process the passed parameters...
    processArgs(argc, argv);

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

    // If requested, daemonize...
    if (l_Daemonize) {
        daemonize();
    }

    // Log the invocation...
    // NOTE:  daemonize() will close all files...  The log has to be opened after daemonize()...

    d_log.open();
    d_log.write(" ");
    snprintf(l_LogBuffer, sizeof(l_LogBuffer), "argc = %d, l_Daemonize = %d, l_NumberOfMsgs = %lld, l_uSecondsBetweenSends = %d, l_HostName = %s, l_Port = %d", argc, l_Daemonize, (signed long long)l_NumberOfMsgs, l_uSecondsBetweenSends, l_HostName, l_Port);
    d_log.write(l_LogBuffer, txp::Log::INFO, txp::Log::ALL_LOGLEVEL, txp::Log::CONSOLE_AND_LOGFILE);

    // Open socket...
    l_FD = connectToServer(argc, argv);

    // Mainline processing...
    const int l_ReadsWithZeroBytesMax = 12500000;   //  This is ~10 seconds...
    int l_ReadsWithZeroBytes = 0;
    int32_t l_NumberOfAttributes = 0;
    ssize_t l_DataLen = 0, l_NumBytes = 0;
    char* l_Data = 0;

    txp::Msg* l_Msg = 0;
    txp::Msg* l_ResponseMsg = 0;
    txp::AttrPtr_char_array* l_Attr = 0;
    while ((!l_RC) && ((!l_NumberOfMsgs) || l_NumberOfMsgsSent<l_NumberOfMsgs)) {
        // Get a message Id
        txp::Id l_Id = getMsgId();
        {
            // Create the message (created in heap...)
            l_RC = txp::Msg::buildMsg(l_Id, l_Msg);
            if (!l_RC) {
                // Add some attributes...
                l_NumberOfAttributes = getNumberOfAttributes(rand());
                for (int32_t i=0; i<l_NumberOfAttributes && l_RC==0; ++i) {
                    // NOTE:  Each attribute created in heap...
                    l_RC = txp::AttrPtr_char_array::buildAttrPtr_char_array(getAttrName(i), TestData, getCharLength(rand()), l_Attr);
                    if (!l_RC) {
                        l_RC = l_Msg->addAttribute(l_Attr);
                    	if (l_RC == -30) {
                    		// Duplicate attribute name...
                    		l_RC = 0;
                    		--i;
                    	}
                    }
                }

                if (!l_RC) {
                    l_Temp[0] = '\0';
                    txp::Msg::msgIdToChar(l_Id, l_Temp, sizeof(l_Temp));

                    // Serialize the message 'values' into the heap buffer...
                    l_Msg->allocateHeapBuffer();
                    l_DataLen = l_Msg->serializeWithValuesToHeapBuffer();
                    if (l_DataLen>0) {

                        l_Data = l_Msg->getDataBufferPtr();

                        {
                            // Log the send attempt...
                            if (DEBUG > 1) {
                                snprintf(l_LogBuffer, sizeof(l_LogBuffer), "Sending: Msg(%s) with %d attr(s), msg# %d, bytes/msglen = %zu/%zu", l_Temp, l_NumberOfAttributes, l_Msg->getMsgNumber(), l_DataLen, l_Msg->getSerializedLen());
                                d_log.write(l_LogBuffer, txp::Log::INFO);
                                if (DEBUG > 3) {
                                    l_Msg->dump(d_log, "Msg, just before send()");
                                }
                            }

                            // Send message
                            l_NumBytes = write(l_FD, l_Data, l_DataLen);
                            if (l_NumBytes < 0) {
                                snprintf(l_LogBuffer, sizeof(l_LogBuffer), "%s: Negative number of bytes written on write, %zu, line %d", argv[0], l_NumBytes, __LINE__);
                                d_log.write(l_LogBuffer, txp::Log::ERROR);
                                l_RC = -10;
                            } else {

                                if (l_NumBytes == l_DataLen) {
                                    // Log the successful send of the message...
                                    if (DEBUG > 0) {
                                        snprintf(l_LogBuffer, sizeof(l_LogBuffer), "-> Msg(%s) %d attr(s), msg# %d, bytes/len %zu/%zu", l_Temp, l_NumberOfAttributes, l_Msg->getMsgNumber(), l_NumBytes, l_Msg->getSerializedLen());
                                        d_log.write(l_LogBuffer, txp::Log::INFO);
                                    }

                                    // Receive response...
                                    l_NumBytes = 0;
                                    bzero(l_Buffer, txp::OFFSET_TO_FIRST_ATTRIBUTE);
                                    while ((!l_RC) && (!l_NumBytes) && l_ReadsWithZeroBytes < l_ReadsWithZeroBytesMax) {
                                        l_NumBytes = read(l_FD, l_Buffer, txp::DEFAULT_BUFFER_SIZE-1);

                                        if (l_NumBytes == 0) {
                                            ++l_ReadsWithZeroBytes;
                                        } else {
                                            if (l_NumBytes < 0) {
                                                if (l_NumBytes == -1) {
                                                    break;
                                                } else {
                                                    snprintf(l_LogBuffer, sizeof(l_LogBuffer), "%s: Negative number of bytes received on read, %zu, line %d", argv[0], l_NumBytes, __LINE__);
                                                    d_log.write(l_LogBuffer, txp::Log::ERROR);
                                                    l_RC = -11;
                                                }
                                            } else {

                                                // Log the receiving of a response buffer...
                                                if (DEBUG > 1) {
                                                    snprintf(l_LogBuffer, sizeof(l_LogBuffer), "Received: Response buffer, bytes=%zu...", l_NumBytes);
                                                    d_log.write(l_LogBuffer, txp::Log::DIAG);
                                                }
                                                if (DEBUG > 2) {
                                                    txp::Log::dump_buffer_raw(d_log, l_Buffer, 32, "Response buffer, before deserialize...");
                                                }

                                                // Process the response...

                                                // Create a heap buffer to deserialize the response message.
                                                txp::HeapBuffer l_HeapBuffer(l_Buffer, txp::DEFAULT_BUFFER_SIZE);

                                                l_RC = txp::Msg::deserializeToMsg(l_ResponseMsg, &l_HeapBuffer, l_DeserializeOption, l_NumBytes);
                                                if (!l_RC) {
                                                    txp::Id l_Id(l_ResponseMsg->getMsgId());
                                                    if (txp::Msg::isMsgIdValid(l_Id)) {
                                                        if (txp::Msg::isMsgIdSupported(l_Id)) {
                                                            txp::Msg::msgIdToChar(l_Id, l_Temp, sizeof(l_Temp));

                                                            // Log received message...
                                                            if (DEBUG > 0) {
                                                                snprintf(l_LogBuffer, sizeof(l_LogBuffer), "<- Msg(%s), msg# %d to rqst_msg# %d, bytes = %zu", l_Temp, l_ResponseMsg->getMsgNumber(), l_ResponseMsg->getRequestMsgNumber(), l_NumBytes);
                                                                d_log.write(l_LogBuffer, txp::Log::INFO);
                                                                if (DEBUG > 3) {
                                                                    l_ResponseMsg->dump(d_log, "ResponseMsg, just after deserialize()");
                                                                }
                                                            }

                                                        } else {
                                                            l_RC = -12;
                                                            snprintf(l_LogBuffer, sizeof(l_LogBuffer), "Id::isSupported() failed for response message");
                                                            d_log.write(l_LogBuffer, txp::Log::ERROR);
                                                        }
                                                    } else {
                                                        l_RC = -13;
                                                        snprintf(l_LogBuffer, sizeof(l_LogBuffer), "Id::isValid() failed for response message");
                                                        d_log.write(l_LogBuffer, txp::Log::ERROR);
                                                    }

                                                    if (l_ResponseMsg) {
                                                        // Delete the response message
                                                        txp::Msg* l_TempMsg = l_ResponseMsg;
                                                        l_ResponseMsg = 0;
                                                        delete(l_TempMsg);
                                                    }

                                                } else {
                                                    snprintf(l_LogBuffer, sizeof(l_LogBuffer), "deserializeToMsg() failed, RC=%d", l_RC);
                                                    d_log.write(l_LogBuffer, txp::Log::ERROR);
                                                }
                                            }
                                        }
                                    }
                                    if (l_ReadsWithZeroBytes == l_ReadsWithZeroBytesMax) {
                                        l_RC = -14;
                                        snprintf(l_LogBuffer, sizeof(l_LogBuffer), "read() for response timed-out");
                                        d_log.write(l_LogBuffer, txp::Log::WARN);
                                    }
                                } else {
                                    l_RC = -15;
                                    snprintf(l_LogBuffer, sizeof(l_LogBuffer), "-> **ERROR** bytes != datalen: Msg(%s) %d attr(s), msg# %d, bytes/len = %zu/%zu", l_Temp, l_NumberOfAttributes, l_Msg->getMsgNumber(), l_NumBytes, l_DataLen);
                                    d_log.write(l_LogBuffer, txp::Log::ERROR);
                                }
                            }
                        }
                    } else {
                        l_RC = (int)l_DataLen;
                        snprintf(l_LogBuffer, sizeof(l_LogBuffer), "serializeWithValuesToHeapBuffer() failed, RC=%d", l_RC);
                        d_log.write(l_LogBuffer, txp::Log::ERROR);
                        l_Msg->dump(d_log, "Failed to serialize this msg");
                    }
                }

                // NOTE:  The attributes are destroyed as part of deleting the message
                if (l_Msg) {
                    delete l_Msg;
                    l_Msg = 0;
                }
            }
        }

        ++l_NumberOfMsgsSent;

        // Output a count every once in a while...
        if (l_NumberOfMsgsSent && (l_NumberOfMsgsSent%l_NumberOfMsgsLogEntry == 0)) {
            snprintf(l_LogBuffer, sizeof(l_LogBuffer), "%lld messages sent...", (signed long long)l_NumberOfMsgsSent);
            d_log.write(l_LogBuffer, txp::Log::INFO, txp::Log::ALL_LOGLEVEL, txp::Log::CONSOLE_AND_LOGFILE);
        }

        if (!l_RC && l_uSecondsBetweenSends) {
            usleep(l_uSecondsBetweenSends);
        }
    }

    snprintf(l_LogBuffer, sizeof(l_LogBuffer), "Finished sending %lld messages, RC=%d.", (signed long long)l_NumberOfMsgsSent, l_RC);
	if (!l_RC) {
    	d_log.write(l_LogBuffer, txp::Log::INFO, txp::Log::ALL_LOGLEVEL, txp::Log::CONSOLE_AND_LOGFILE);
    } else {
    	d_log.write(l_LogBuffer, txp::Log::ERROR, txp::Log::ALL_LOGLEVEL, txp::Log::CONSOLE_AND_LOGFILE);
    }

    cleanUp();

    return l_RC;
}
