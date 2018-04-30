/*******************************************************************************
 |    txpServer2.cc
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

#include <ctype.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
// #include <syslog.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "Msg.h"

// DEBUG values
// 0 - Connection accepted,
//     log entry after every l_NumberOfMsgsLogEntry msgs,
//     connection closed.
// 1 - Level 0 plus each receive and send response
// 2 - Level 1 plus additional trace points
// 3 - Level 2 plus logging of the first
//     256 bytes of the send/receive buffers
uint32_t DEBUG = 1;

txp::DeserializeOption l_DeserializeOption(txp::DO_NOT_COPY_DATA);
//txp::DeserializeOption l_DeserializeOption(txp::COPY_DATA_TO_OBJECT);

int FD=0, FD2=0;

// Log for this program...
// NOTE:  Open the log file after daemonize().  daeomize closes all open files...
txp::Log d_log(txp::Log::LOGFILE, "/bglhome/dherms/git/bluecoral/work/transport/tests/txpServer.log",
	           txp::Log::OPEN, txp::Log::ALL_LOGLEVEL);


void closeFD(int &pFD) {
    if (pFD > 0) {
        close(pFD);
        pFD = 0;
    }

    return;
}


void cleanUp() {
    closeFD(FD2);
    closeFD(FD);

    return;
}


void error(const char* pMsg) {
    perror(pMsg);
    cleanUp();
    exit(EXIT_FAILURE);
}


static void daemonize()
{
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
    int l_FD;
    for (l_FD = sysconf(_SC_OPEN_MAX); l_FD>0; l_FD--) {
        close (l_FD);
    }
}


char l_LogBuffer[133] = {'\0'};

int l_Port, l_Daemonize;


void processArgs(int argc, char *argv[]) {
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


int main(int argc, char *argv[])
{
    int l_RC = 0;

    // Process the pased parameters...
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

    // Log invocation...
    // NOTE:  daemonize() will close all files...  The log has to be opened after daemonize()...
    d_log.open();
    snprintf(l_LogBuffer, sizeof(l_LogBuffer), "argc = %d, l_Daemonize = %d, l_Port = %d", argc, l_Daemonize, l_Port);
    d_log.write(l_LogBuffer, txp::Log::INFO);

    struct sockaddr_in l_ServerAddr, l_ClientAddr;
    socklen_t l_ClientLen = sizeof(l_ClientAddr);

    // Open a socket...
    FD = socket(AF_INET, SOCK_STREAM, 0);
    if (FD < 0) {
        snprintf(l_LogBuffer, sizeof(l_LogBuffer), "%s: Error opening socket, line %d", argv[0], __LINE__);
        d_log.write(l_LogBuffer, txp::Log::ERROR);
        error(l_LogBuffer);
    }

    // Bind to a port...
    bzero((char *) &l_ServerAddr, sizeof(l_ServerAddr));
    l_ServerAddr.sin_family = AF_INET;
    l_ServerAddr.sin_addr.s_addr = INADDR_ANY;
    l_ServerAddr.sin_port = htons(l_Port);
    if (bind(FD, (struct sockaddr *) &l_ServerAddr, sizeof(l_ServerAddr)) < 0) {
        snprintf(l_LogBuffer, sizeof(l_LogBuffer), "%s: Error binding socket, line %d", argv[0], __LINE__);
        d_log.write(l_LogBuffer, txp::Log::ERROR);
        error(l_LogBuffer);
    }

//    const int l_uSecondsBetweenReadsMax = 0;
    const int l_ReadsWithZeroBytesMax = 12500000;    //  This is ~10 seconds...
    const uint64_t l_NumberOfMsgsLogEntry = 1000000;


    int l_uSecondsBetweenReads = 0, l_ReadsWithZeroBytes = 0;
//    int l_uSecondsBump = 0;
//    int l_ZeroReadsBeforeBump = 12500000;

    int32_t l_NumBytes;
    uint64_t l_NumberOfMsgs = 0;
    char l_Buffer[txp::DEFAULT_BUFFER_SIZE];
    char l_Temp[64] = {'\0'};

    txp::Msg* l_ResponseMsg = 0;

    while (1)
    {
//        printf("Listen...\n");
       // Listen for connections on FD...
        listen(FD, 5);

        // Accept connection on FD2...
        FD2 = accept(FD, (struct sockaddr *) &l_ClientAddr, &l_ClientLen);
        if (FD2 < 0) {
            snprintf(l_LogBuffer, sizeof(l_LogBuffer), "%s: Error on accept on socket, line %d", argv[0], __LINE__);
            d_log.write(l_LogBuffer, txp::Log::ERROR);
            error(l_LogBuffer);
        }

        d_log.write(" ");
        snprintf(l_LogBuffer, sizeof(l_LogBuffer), "Connection %d accepted...", FD2);
        d_log.write(l_LogBuffer, txp::Log::INFO);

        l_NumberOfMsgs = 0;
        txp::Msg* l_Msg = 0;
        bool l_txpMsgReceived = false;
        char* l_Data = 0;
        ssize_t l_DataLen = 0;

        int32_t l_ReadHeader = 1;
        bzero(l_Buffer, txp::OFFSET_TO_FIRST_ATTRIBUTE);
        while ((!l_RC) && l_ReadsWithZeroBytes < l_ReadsWithZeroBytesMax) {
            // Receive message...
            if (DEBUG>3 && l_ReadsWithZeroBytes%l_NumberOfMsgsLogEntry==0) {
                snprintf(l_LogBuffer, sizeof(l_LogBuffer), "Reading socket...");
                d_log.write(l_LogBuffer, txp::Log::INFO);
            }
            if (l_ReadHeader) {
                l_NumBytes = read(FD2, l_Buffer, txp::OFFSET_TO_FIRST_ATTRIBUTE);
            } else {
                l_NumBytes = read(FD2, l_Buffer, txp::DEFAULT_BUFFER_SIZE-1);
            }
            if (l_NumBytes == 0) {
//              if (l_ReadsWithZeroBytes > l_ZeroReadsBeforeBump) {
//                  if (l_uSecondsBetweenReads < l_uSecondsBetweenReadsMax) {
//                      l_uSecondsBetweenReads += l_uSecondsBump;
//                  }
//              }
                ++l_ReadsWithZeroBytes;
            } else {
                if (l_NumBytes < 0) {
                    if (l_NumBytes == -1) {
                        break;
                    } else {
                        snprintf(l_LogBuffer, sizeof(l_LogBuffer), "%s: Negative number of bytes received on read, %d, line %d", argv[0], l_NumBytes, __LINE__);
                        d_log.write(l_LogBuffer, txp::Log::ERROR);
                        error(l_LogBuffer);
                    }
                } else {
                    // Log the receiving of a request buffer...
                    if (DEBUG > 1) {
                        snprintf(l_LogBuffer, sizeof(l_LogBuffer), "Received: Request buffer, bytes=%d...", l_NumBytes);
                        d_log.write(l_LogBuffer, txp::Log::DIAG);
                    }
                    if (DEBUG > 2) {
                        txp::Log::dump_buffer_raw(d_log, l_Buffer, 256, "Request buffer, before deserialize...");
                    }

                    // Process the request...
                    l_txpMsgReceived = false;
                    l_Data = 0;
                    l_DataLen = 0;
                    txp::HeapBuffer l_HeapBuffer(l_Buffer, txp::DEFAULT_BUFFER_SIZE);

                    if (l_ReadHeader) {
                        l_RC = txp::Msg::deserializeToMsg(l_Msg, &l_HeapBuffer, l_NumBytes);
                    } else {
                        l_RC = l_Msg->deserializeAttrsToMsg(&l_HeapBuffer, l_DeserializeOption, l_NumBytes);
                    }

                    if (!l_RC) {
                        l_txpMsgReceived = true;
                        txp::Id l_Id(l_Msg->getMsgId());
                        if (txp::Msg::isMsgIdValid(l_Id)) {
                            if (txp::Msg::isMsgIdSupported(l_Id)) {
                                txp::Msg::msgIdToChar(l_Id, l_Temp, sizeof(l_Temp));

                                // Log received message...
                                if (DEBUG) {
                                    snprintf(l_LogBuffer, sizeof(l_LogBuffer), "<- Msg(%s), %d attr(s), msg# %d, bytes/msglen %d/%zd", l_Temp, l_Msg->getNumberOfAttributes(), l_Msg->getMsgNumber(), l_NumBytes, l_Msg->getMsgLengthWithDataValues());
                                    d_log.write(l_LogBuffer, txp::Log::INFO);
                                }

                                if (!l_ReadHeader) {
                                    // Build response...
                                    if (!l_Msg->buildResponseMsg(l_ResponseMsg)) {
                                        l_ResponseMsg->allocateHeapBuffer(txp::OFFSET_TO_FIRST_ATTRIBUTE);
//                                        l_ResponseMsg->dump(d_log, "Before serialize");
                                        l_DataLen = l_ResponseMsg->serializeWithValuesToHeapBuffer();
//                                        printf("l_DataLen=%d\n", l_DataLen);
                                        if (l_DataLen >= (ssize_t)txp::OFFSET_TO_FIRST_ATTRIBUTE){
                                            l_Data = l_ResponseMsg->getDataBufferPtr();
                                            if (DEBUG > 2) {
                                                txp::Log::dump_buffer_raw(d_log, l_Data, l_DataLen, "Response buffer");
                                            }
                                        } else {
                                            l_RC = (int)l_DataLen;
                                            snprintf(l_LogBuffer, sizeof(l_LogBuffer), "serializeWithValuesToHeapBuffer() failed, RC=%d", l_RC);
                                            d_log.write(l_LogBuffer, txp::Log::ERROR);
                                        }
                                    } else {
                                        l_RC = -10;
                                        snprintf(l_LogBuffer, sizeof(l_LogBuffer), "buildResponseMsg() failed, RC=%d", l_RC);
                                        d_log.write(l_LogBuffer, txp::Log::ERROR);
                                    }
                                }
                            } else {
                                l_RC = -11;
                                snprintf(l_LogBuffer, sizeof(l_LogBuffer), "Id::isSupported() failed, RC=%d", l_RC);
                                d_log.write(l_LogBuffer, txp::Log::ERROR);
                            }
                        } else {
                            l_RC = -12;
                            snprintf(l_LogBuffer, sizeof(l_LogBuffer), "Id::isValid() failed, RC=%d", l_RC);
                            d_log.write(l_LogBuffer, txp::Log::ERROR);
                        }

                        // Delete the message used to serialize the received buffer into...
                        if (!l_ReadHeader) {
                            txp::Msg* l_TempMsg = l_Msg;
                            l_Msg = 0;
                            delete(l_TempMsg);
                        }
                        l_ReadHeader ^= 1;

                    } else {
                        snprintf(l_LogBuffer, sizeof(l_LogBuffer), "deserializeXXXX() failed, l_NumBytes = %d, RC=%d", l_NumBytes, l_RC);
                        d_log.write(l_LogBuffer, txp::Log::ERROR);
                        l_RC = 0;
                    }

                    if (!l_RC) {
                        if (!l_txpMsgReceived) {
                            // Log received text message...
                            std::string l_Message(l_Buffer);
                            l_Message.erase(std::remove(l_Message.begin(), l_Message.end(), '\n'), l_Message.end());

                            if (DEBUG) {
                                snprintf(l_LogBuffer, sizeof(l_LogBuffer), "<- text('%s')", (char*)l_Message.c_str());
                                d_log.write(l_LogBuffer, txp::Log::INFO);
                            }

                            char l_Response[256] = {'\0'};
                            snprintf(l_Response, sizeof(l_Response), "%s", (char*)l_Message.c_str());

                            l_Data = l_Response;
                            l_DataLen = strlen(l_Response);
                            if (DEBUG > 2) {
                                txp::Log::dump_buffer_raw(d_log, l_Data, sizeof(l_Response), "Response buffer");
                            }

                            // Log the ensuing send attempt...
                            if (DEBUG > 1) {
                                snprintf(l_LogBuffer, sizeof(l_LogBuffer), "Sending: text, bytes = %zd", l_DataLen);
                                d_log.write(l_LogBuffer, txp::Log::INFO);
                            }
                        } else {
                            // Log the ensuing send attempt...
                            if (DEBUG > 1) {
                                txp::Msg::msgIdToChar(l_ResponseMsg->getMsgId(), l_Temp, sizeof(l_Temp));
                                snprintf(l_LogBuffer, sizeof(l_LogBuffer), "Sending: Msg(%s) with %d attr(s), msg# %d, rcv'd bytes/msglen = %zd/%zd", l_Temp, l_ResponseMsg->getNumberOfAttributes(), l_ResponseMsg->getMsgNumber(), l_DataLen, l_ResponseMsg->getSerializedLen());
                                d_log.write(l_LogBuffer, txp::Log::INFO);
                            }
                        }
                    }

                    // Respond to message...
                    if ((!l_RC) && l_DataLen) {
                        l_NumBytes = write(FD2, l_Data, l_DataLen);
                        if (l_NumBytes <= 0) {
                            snprintf(l_LogBuffer, sizeof(l_LogBuffer), "%s: %d number of bytes written, msglen %zd, line %d", argv[0], l_NumBytes, l_DataLen, __LINE__);
                            d_log.write(l_LogBuffer, txp::Log::INFO);
                            error(l_LogBuffer);
                        } else {
                            if (l_DataLen == l_NumBytes) {
                                if (DEBUG) {
                                    if (l_txpMsgReceived) {
                                        txp::Msg::msgIdToChar(l_ResponseMsg->getMsgId(), l_Temp, sizeof(l_Temp));
                                        snprintf(l_LogBuffer, sizeof(l_LogBuffer), "-> Msg(%s), %d attr(s), msg# %d, rqst_msg# %d, bytes/msglen %d/%zd, data=%p, datalen=%zd", l_Temp, l_ResponseMsg->getNumberOfAttributes(), l_ResponseMsg->getMsgNumber(), l_ResponseMsg->getRequestMsgNumber(), l_NumBytes, l_ResponseMsg->getMsgLengthWithDataValues(), l_Data, l_DataLen);
                                    } else {
                                        snprintf(l_LogBuffer, sizeof(l_LogBuffer), "-> text('%s')", l_Data);
                                    }
                                    d_log.write(l_LogBuffer, txp::Log::INFO);
                                }
                            } else {
                                if (l_txpMsgReceived) {
                                    txp::Msg::msgIdToChar(l_ResponseMsg->getMsgId(), l_Temp, sizeof(l_Temp));
                                    snprintf(l_LogBuffer, sizeof(l_LogBuffer), "-> **ERROR** bytes != datalen:  Msg(%s), %d attr(s), msg# %d, rqst_msg# %d, bytes/msglen %d/%zd, data=%p, datalen=%zd", l_Temp, l_ResponseMsg->getNumberOfAttributes(), l_ResponseMsg->getMsgNumber(), l_ResponseMsg->getRequestMsgNumber(), l_NumBytes, l_ResponseMsg->getMsgLengthWithDataValues(), l_Data, l_DataLen);
                                } else {
                                    snprintf(l_LogBuffer, sizeof(l_LogBuffer), "-> **ERROR** bytes != datalen:  data=%p, bytes %d, datalen=%zd", l_Data, l_NumBytes, l_DataLen);
                                }
                                d_log.write(l_LogBuffer, txp::Log::ERROR);
                                l_RC = -14;
                            }
                        }
                    }

                    // If created, delete the response message...
                    if (l_ResponseMsg) {
                        txp::Msg* l_TempMsg = l_ResponseMsg;
                        l_ResponseMsg = 0;
                        delete(l_TempMsg);
                    }

                    // Reset the wait between reads/zero byte reads...
                    l_uSecondsBetweenReads = 0;
                    l_ReadsWithZeroBytes = 0;

                    ++l_NumberOfMsgs;

                    // Output a count every once in a while...
                    if (l_NumberOfMsgs && (l_NumberOfMsgs%l_NumberOfMsgsLogEntry == 0)) {
                        snprintf(l_LogBuffer, sizeof(l_LogBuffer), "%lld messages received...", (signed long long)l_NumberOfMsgs);
                        d_log.write(l_LogBuffer, txp::Log::INFO);
                    }
                }
            }
            // Delay...
            if (l_uSecondsBetweenReads) {
                usleep(l_uSecondsBetweenReads);
            }
        }
        l_uSecondsBetweenReads = 0;
        l_ReadsWithZeroBytes = 0;

        snprintf(l_LogBuffer, sizeof(l_LogBuffer), "%lld total messages received for connection %d...", (signed long long)l_NumberOfMsgs, FD2);
        d_log.write(l_LogBuffer, txp::Log::INFO);

        snprintf(l_LogBuffer, sizeof(l_LogBuffer), "Connection %d closed...", FD2);
        d_log.write(l_LogBuffer, txp::Log::INFO);

        closeFD(FD2);
    }

    cleanUp();

    return EXIT_SUCCESS;
}
