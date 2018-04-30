/*******************************************************************************
 |    Log.cc
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


#include <iostream>

#include <stdio.h>
#include <sys/time.h>

#include "Common.h"
#include "Log.h"

extern boost::property_tree::ptree txpConfig;
extern boost::program_options::variables_map txpVm;

//*****************************************************************************
//  Static data members
//*****************************************************************************
// NOTE: DEFAULT_LOG_DESTINATION is 'inited' by Msg::init() using coral.cfg.
//       However, that method does not run until after the Log module is loaded.
//       Therefore, some of the default d_logs used in txp are initialized with
//       a destination as set at construct time below.  \todo @@DLH
txp::Log::Destination txp::Log::DEFAULT_LOG_DESTINATION(txp::Log::BOOST);

// NOTE: DEFAULT_OPEN_LOGSTATE is 'inited' by Msg::init() using coral.cfg.
//       Note, that method does not run until after the Log module is loaded.
txp::Log::OpenState txp::Log::DEFAULT_OPEN_LOGSTATE(txp::Log::OPEN);

// NOTE: DEFAULT_OPEN_LOGLEVEL is 'inited' by Msg::init() using coral.cfg.
//       Note, that method does not run until after the Log module is loaded.
txp::Log::LogLevel txp::Log::DEFAULT_OPEN_LOGLEVEL(txp::Log::ERROR_LOGLEVEL);

// NOTE: DEFAULT_OPEN_DUMP_LOGLEVEL is 'inited' by Msg::init() using coral.cfg.
//       Note, that method does not run until after the Log module is loaded.
txp::Log::LogLevel txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL(txp::Log::NO_LOGLEVEL);

//*****************************************************************************
//  Constructors
//*****************************************************************************
txp::Log::Log(OpenState pOpen, LogLevel pLogLevel) :
	destination(DEFAULT_LOG_DESTINATION) {
    bzero((char *)&logFile, sizeof(logFile));
    logFileMode = std::ofstream::out;
	init(pOpen, pLogLevel);
}

txp::Log::Log(Destination pDestination, OpenState pOpen, LogLevel pLogLevel) :
	destination(pDestination) {
    bzero((char *)&logFile, sizeof(logFile));
    logFileMode = std::ofstream::out;
	init(pOpen, pLogLevel);
}

txp::Log::Log(Destination pDestination, const char* pLogFile, OpenState pOpen, LogLevel pLogLevel) :
	destination(pDestination) {
	logFileMode = std::ofstream::out | std::ofstream::trunc;
	strCpy(logFile, pLogFile, sizeof(logFile));
	init(pOpen, pLogLevel);
}

txp::Log::Log(Destination pDestination, const char* pLogFile, const std::ios_base::openmode pMode, OpenState pOpen, LogLevel pLogLevel) :
	destination(pDestination) {
	logFileMode = std::ofstream::out | pMode;
	strCpy(logFile, pLogFile, sizeof(logFile));
	init(pOpen, pLogLevel);
}

//*****************************************************************************
//  Destructors
//*****************************************************************************
txp::Log::~Log() {
	close();
}

txp::Log::Indent::~Indent() {

}

//*****************************************************************************
//  Static methods
//*****************************************************************************
void txp::Log::dump_buffer_raw(Log& pLog, const char* pBuffer, const size_t pSize, const char* pPrefix) {
	char l_Buffer[133] = {'\0'};

	if (pLog.logLevelCheck(txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL)) {

		size_t l_Size = std::min(pSize, txp::MAXIMUM_DUMP_SIZE);

		if (pBuffer) {
			START_PREFIX(pPrefix, ">>>>> Start raw dump of buffer:  %s");

			size_t l_Offset = 0;
			int l_RowAllZeros = 0;
			bool l_PrintRow = true, l_WithinFiller = false;
			// NOTE:  The last line of output could get addressing issues...  Need to fix it...  @@DLH
			while (l_Offset<((l_Size+15)/16)*16) {
				l_PrintRow = true;
				if ( (*((uint64_t*)(pBuffer+l_Offset)) == 0) && (*((uint64_t*)(pBuffer+l_Offset+8)) == 0) ) {
					if (l_RowAllZeros>1) {
						if (l_RowAllZeros==3 && ((((l_Size+15)/16)*16)-l_Offset > 32)) {
							snprintf(l_Buffer, sizeof(l_Buffer), "                 .        .        .        .");
							pLog.write(l_Buffer, txp::Log::DEBUG, txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL);
							pLog.write(l_Buffer, txp::Log::DEBUG, txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL);
							l_WithinFiller = true;
						}
						if (l_WithinFiller) {
    	    				l_PrintRow = false;
    	    			}
					}
					++l_RowAllZeros;
				} else {
					l_RowAllZeros = 0;
					l_WithinFiller = false;
				}

				if (l_PrintRow) {
					snprintf(l_Buffer, sizeof(l_Buffer), "0x%08X | %02X%02X%02X%02X %02X%02X%02X%02X %02X%02X%02X%02X %02X%02X%02X%02X * %c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c", (uint32_t)l_Offset,
							 *((uint8_t*)(pBuffer+l_Offset)), *((uint8_t*)(pBuffer+l_Offset+1)), *((uint8_t*)(pBuffer+l_Offset+2)), *((uint8_t*)(pBuffer+l_Offset+3)),
							 *((uint8_t*)(pBuffer+l_Offset+4)), *((uint8_t*)(pBuffer+l_Offset+5)), *((uint8_t*)(pBuffer+l_Offset+6)), *((uint8_t*)(pBuffer+l_Offset+7)),
							 *((uint8_t*)(pBuffer+l_Offset+8)), *((uint8_t*)(pBuffer+l_Offset+9)), *((uint8_t*)(pBuffer+l_Offset+10)), *((uint8_t*)(pBuffer+l_Offset+11)),
							 *((uint8_t*)(pBuffer+l_Offset+12)), *((uint8_t*)(pBuffer+l_Offset+13)), *((uint8_t*)(pBuffer+l_Offset+14)), *((uint8_t*)(pBuffer+l_Offset+15)),
							 isprint(*((uint8_t*)(pBuffer+l_Offset))) ? *((uint8_t*)(pBuffer+l_Offset)) : *((uint8_t*)(pBuffer+l_Offset))==0x20 ? ' ' : '.',
							 isprint(*((uint8_t*)(pBuffer+l_Offset+1))) ? *((uint8_t*)(pBuffer+l_Offset+1)) : *((uint8_t*)(pBuffer+l_Offset+1))==0x20 ? ' ' : '.',
							 isprint(*((uint8_t*)(pBuffer+l_Offset+2))) ? *((uint8_t*)(pBuffer+l_Offset+2)) : *((uint8_t*)(pBuffer+l_Offset+2))==0x20 ? ' ' : '.',
							 isprint(*((uint8_t*)(pBuffer+l_Offset+3))) ? *((uint8_t*)(pBuffer+l_Offset+3)) : *((uint8_t*)(pBuffer+l_Offset+3))==0x20 ? ' ' : '.',
							 isprint(*((uint8_t*)(pBuffer+l_Offset+4))) ? *((uint8_t*)(pBuffer+l_Offset+4)) : *((uint8_t*)(pBuffer+l_Offset+4))==0x20 ? ' ' : '.',
							 isprint(*((uint8_t*)(pBuffer+l_Offset+5))) ? *((uint8_t*)(pBuffer+l_Offset+5)) : *((uint8_t*)(pBuffer+l_Offset+5))==0x20 ? ' ' : '.',
							 isprint(*((uint8_t*)(pBuffer+l_Offset+6))) ? *((uint8_t*)(pBuffer+l_Offset+6)) : *((uint8_t*)(pBuffer+l_Offset+6))==0x20 ? ' ' : '.',
							 isprint(*((uint8_t*)(pBuffer+l_Offset+7))) ? *((uint8_t*)(pBuffer+l_Offset+7)) : *((uint8_t*)(pBuffer+l_Offset+7))==0x20 ? ' ' : '.',
							 isprint(*((uint8_t*)(pBuffer+l_Offset+8))) ? *((uint8_t*)(pBuffer+l_Offset+8)) : *((uint8_t*)(pBuffer+l_Offset+8))==0x20 ? ' ' : '.',
							 isprint(*((uint8_t*)(pBuffer+l_Offset+9))) ? *((uint8_t*)(pBuffer+l_Offset+9)) : *((uint8_t*)(pBuffer+l_Offset+9))==0x20 ? ' ' : '.',
							 isprint(*((uint8_t*)(pBuffer+l_Offset+10))) ? *((uint8_t*)(pBuffer+l_Offset+10)) : *((uint8_t*)(pBuffer+l_Offset+10))==0x20 ? ' ' : '.',
							 isprint(*((uint8_t*)(pBuffer+l_Offset+11))) ? *((uint8_t*)(pBuffer+l_Offset+11)) : *((uint8_t*)(pBuffer+l_Offset+11))==0x20 ? ' ' : '.',
							 isprint(*((uint8_t*)(pBuffer+l_Offset+12))) ? *((uint8_t*)(pBuffer+l_Offset+12)) : *((uint8_t*)(pBuffer+l_Offset+12))==0x20 ? ' ' : '.',
							 isprint(*((uint8_t*)(pBuffer+l_Offset+13))) ? *((uint8_t*)(pBuffer+l_Offset+13)) : *((uint8_t*)(pBuffer+l_Offset+13))==0x20 ? ' ' : '.',
							 isprint(*((uint8_t*)(pBuffer+l_Offset+14))) ? *((uint8_t*)(pBuffer+l_Offset+14)) : *((uint8_t*)(pBuffer+l_Offset+14))==0x20 ? ' ' : '.',
							 isprint(*((uint8_t*)(pBuffer+l_Offset+15))) ? *((uint8_t*)(pBuffer+l_Offset+15)) : *((uint8_t*)(pBuffer+l_Offset+15))==0x20 ? ' ' : '.'
							);
					pLog.write(l_Buffer, txp::Log::DEBUG, txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL);
				}
				l_Offset += 16;
			}

			if (l_WithinFiller) {
				l_Offset -= 16;
				snprintf(l_Buffer, sizeof(l_Buffer), "0x%08X | %02X%02X%02X%02X %02X%02X%02X%02X %02X%02X%02X%02X %02X%02X%02X%02X * ................", (uint32_t)l_Offset,
						*((uint8_t*)(pBuffer+l_Offset)), *((uint8_t*)(pBuffer+l_Offset+1)), *((uint8_t*)(pBuffer+l_Offset+2)), *((uint8_t*)(pBuffer+l_Offset+3)),
						*((uint8_t*)(pBuffer+l_Offset+4)), *((uint8_t*)(pBuffer+l_Offset+5)), *((uint8_t*)(pBuffer+l_Offset+6)), *((uint8_t*)(pBuffer+l_Offset+7)),
						*((uint8_t*)(pBuffer+l_Offset+8)), *((uint8_t*)(pBuffer+l_Offset+9)), *((uint8_t*)(pBuffer+l_Offset+10)), *((uint8_t*)(pBuffer+l_Offset+11)),
						*((uint8_t*)(pBuffer+l_Offset+12)), *((uint8_t*)(pBuffer+l_Offset+13)), *((uint8_t*)(pBuffer+l_Offset+14)), *((uint8_t*)(pBuffer+l_Offset+15)));
				pLog.write(l_Buffer, txp::Log::DEBUG, txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL);
				l_Offset += 16;
			}

			END_PREFIX(pPrefix, ">>>>>   End raw dump of buffer:  %s", 0);
                        (void)l_Offset;
		}
	}

	return;
}

void txp::Log::destinationToChar(const Destination pValue, char* pBuffer, const int32_t pSize) {
	if (pSize) {
		pBuffer[0] = '\0';
		switch (pValue) {
			case NONE:
				strCpy(pBuffer, "NONE", pSize);
				break;
			case BOOST:
				strCpy(pBuffer, "BOOST", pSize);
				break;
			case CONSOLE:
				strCpy(pBuffer, "CONSOLE", pSize);
				break;
			case LOGFILE:
				strCpy(pBuffer, "LOGFILE", pSize);
				break;
			case CONSOLE_AND_LOGFILE:
				strCpy(pBuffer, "CONSOLE_AND_LOGFILE", pSize);
				break;

			default:
				snprintf(pBuffer, pSize, "%s (%d)", "UNDEFINED", pValue);
		}
	}

	return;
};


void txp::Log::setDefaultLogDestination(const std::string pValue) {
	if (pValue.compare("NONE") == 0)
		txp::Log::DEFAULT_LOG_DESTINATION = NONE;
	else if (pValue.compare("BOOST") == 0)
		txp::Log::DEFAULT_LOG_DESTINATION = BOOST;
	else if (pValue.compare("CONSOLE") == 0)
		txp::Log::DEFAULT_LOG_DESTINATION = CONSOLE;
	else if (pValue.compare("LOGFILE") == 0)
		txp::Log::DEFAULT_LOG_DESTINATION = LOGFILE;
	else if (pValue.compare("CONSOLE_AND_LOGFILE") == 0)
		txp::Log::DEFAULT_LOG_DESTINATION = CONSOLE_AND_LOGFILE;
	else {
		LOG(txp,always) << "Invalid value (" << pValue << ") specified for the log destination.  Value set to BOOST";
		txp::Log::DEFAULT_LOG_DESTINATION = BOOST;
	}

	return;
};


void txp::Log::setDefaultOpenLogState(const std::string pValue) {
	if (pValue.compare("OPEN") == 0)
		txp::Log::DEFAULT_OPEN_LOGSTATE = OPEN;
	else if (pValue.compare("CLOSED") == 0)
		txp::Log::DEFAULT_OPEN_LOGSTATE = CLOSED;
	else {
		LOG(txp,always) << "Invalid value (" << pValue << ") specified for the default log open state.  Value set to OPEN";
		txp::Log::DEFAULT_OPEN_LOGSTATE = OPEN;
	}

	return;
};


void txp::Log::setDefaultOpenLogLevel(const std::string pValue) {
	if (pValue.compare("NO_LOGLEVEL") == 0)
		txp::Log::DEFAULT_OPEN_LOGLEVEL = NO_LOGLEVEL;
	else if (pValue.compare("ALL_LOGLEVEL") == 0)
		txp::Log::DEFAULT_OPEN_LOGLEVEL = ALL_LOGLEVEL;
	else if (pValue.compare("DEFAULT_LOGLEVEL") == 0)
		txp::Log::DEFAULT_OPEN_LOGLEVEL = DEFAULT_LOGLEVEL;
	else if (pValue.compare("ERROR_LOGLEVEL") == 0)
		txp::Log::DEFAULT_OPEN_LOGLEVEL = ERROR_LOGLEVEL;
	else if (pValue.compare("WARN_LOGLEVEL") == 0)
		txp::Log::DEFAULT_OPEN_LOGLEVEL = WARN_LOGLEVEL;
	else if (pValue.compare("DIAG_LOGLEVEL") == 0)
		txp::Log::DEFAULT_OPEN_LOGLEVEL = DIAG_LOGLEVEL;
	else if (pValue.compare("INFO_LOGLEVEL") == 0)
		txp::Log::DEFAULT_OPEN_LOGLEVEL = INFO_LOGLEVEL;
	else if (pValue.compare("DEBUG_LOGLEVEL") == 0)
		txp::Log::DEFAULT_OPEN_LOGLEVEL = DEBUG_LOGLEVEL;
	else if (pValue.compare("DEBUG_LOGLEVEL_2") == 0)
		txp::Log::DEFAULT_OPEN_LOGLEVEL = DEBUG_LOGLEVEL_2;
	else if (pValue.compare("DEBUG_LOGLEVEL_3") == 0)
		txp::Log::DEFAULT_OPEN_LOGLEVEL = DEBUG_LOGLEVEL_3;
	else if (pValue.compare("DEBUG_LOGLEVEL_4") == 0)
		txp::Log::DEFAULT_OPEN_LOGLEVEL = DEBUG_LOGLEVEL_4;
	else if (pValue.compare("DEBUG_LOGLEVEL_5") == 0)
		txp::Log::DEFAULT_OPEN_LOGLEVEL = DEBUG_LOGLEVEL_5;
	else if (pValue.compare("C_DTOR_LOGLEVEL") == 0)
		txp::Log::DEFAULT_OPEN_LOGLEVEL = C_DTOR_LOGLEVEL;
	else if (pValue.compare("C_DTOR_LOGLEVEL_2") == 0)
		txp::Log::DEFAULT_OPEN_LOGLEVEL = C_DTOR_LOGLEVEL_2;
	else if (pValue.compare("ENTER_EXIT_LOGLEVEL") == 0)
		txp::Log::DEFAULT_OPEN_LOGLEVEL = ENTER_EXIT_LOGLEVEL;
	else {
		LOG(txp,always) << "Invalid value (" << pValue << ") specified for the default open loglevel.  Value set to NO_LOGLEVEL";
		txp::Log::DEFAULT_OPEN_LOGLEVEL = NO_LOGLEVEL;
	}

	return;
};


void txp::Log::setDefaultOpenDumpLogLevel(const std::string pValue) {
	if (pValue.compare("NO_LOGLEVEL") == 0)
		txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL = NO_LOGLEVEL;
	else if (pValue.compare("ALL_LOGLEVEL") == 0)
		txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL = ALL_LOGLEVEL;
	else if (pValue.compare("DEFAULT_LOGLEVEL") == 0)
		txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL = DEFAULT_LOGLEVEL;
	else if (pValue.compare("ERROR_LOGLEVEL") == 0)
		txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL = ERROR_LOGLEVEL;
	else if (pValue.compare("WARN_LOGLEVEL") == 0)
		txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL = WARN_LOGLEVEL;
	else if (pValue.compare("DIAG_LOGLEVEL") == 0)
		txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL = DIAG_LOGLEVEL;
	else if (pValue.compare("INFO_LOGLEVEL") == 0)
		txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL = INFO_LOGLEVEL;
	else if (pValue.compare("DEBUG_LOGLEVEL") == 0)
		txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL = DEBUG_LOGLEVEL;
	else if (pValue.compare("DEBUG_LOGLEVEL_2") == 0)
		txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL = DEBUG_LOGLEVEL_2;
	else if (pValue.compare("DEBUG_LOGLEVEL_3") == 0)
		txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL = DEBUG_LOGLEVEL_3;
	else if (pValue.compare("DEBUG_LOGLEVEL_4") == 0)
		txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL = DEBUG_LOGLEVEL_4;
	else if (pValue.compare("DEBUG_LOGLEVEL_5") == 0)
		txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL = DEBUG_LOGLEVEL_5;
	else if (pValue.compare("C_DTOR_LOGLEVEL") == 0)
		txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL = C_DTOR_LOGLEVEL;
	else if (pValue.compare("C_DTOR_LOGLEVEL_2") == 0)
		txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL = C_DTOR_LOGLEVEL_2;
	else if (pValue.compare("ENTER_EXIT_LOGLEVEL") == 0)
		txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL = ENTER_EXIT_LOGLEVEL;
	else {
		LOG(txp,always) << "Invalid value (" << pValue << ") specified for the default open dump loglevel.  Value set to NO_LOGLEVEL";
		txp::Log::DEFAULT_OPEN_DUMP_LOGLEVEL = NO_LOGLEVEL;
	}

	return;
};


//*****************************************************************************
//  Non static methods
//*****************************************************************************
int txp::Log::close() {
	int l_RC = 0;

	if ( isOpen ) {
		isOpen = false;
		if ( logFile[0] ) {
			logFileStream.flush();
			logFileStream.close();
		}
	}

	return l_RC;
}

int txp::Log::enter(const char* pFile, const char* pModule, const LogLevel pLogLevel, const Destination* pDestination) {
	int l_RC = 0;

	char l_Line[DEFAULT_LINE_LENGTH] = {'\0'};
	snprintf(l_Line, sizeof(l_Line), "Entering -> %s.%s", pFile, pModule);
	write(l_Line, INFO, ENTER_EXIT_LOGLEVEL);

	return l_RC;
}

int txp::Log::exit(const char* pFile, const char* pModule, const int pRC, const LogLevel pLogLevel, const Destination* pDestination) {
	int l_RC = 0;

	char l_Line[DEFAULT_LINE_LENGTH] = {'\0'};
	snprintf(l_Line, sizeof(l_Line), "Exiting  <- %s.%s, rc=%d", pFile, pModule, pRC);
	write(l_Line, INFO, ENTER_EXIT_LOGLEVEL);

	return l_RC;
}

void txp::Log::getCurrentTime( char* pBuffer, const int32_t pSize ) {
	char l_Buffer[20] = {'\0'};

    timeval l_CurrentTime;
    gettimeofday(&l_CurrentTime, NULL);
    unsigned long l_Micro = l_CurrentTime.tv_usec;

    //localtime is not thread safe
    strftime(l_Buffer, sizeof(l_Buffer), "%Y-%m-%d %H:%M:%S", localtime((const time_t*)&l_CurrentTime.tv_sec));
    snprintf(pBuffer, pSize, "%s.%06lu", l_Buffer, l_Micro);

	return;
}

void txp::Log::init(OpenState pOpen, LogLevel pLogLevel) {
	myIndent.init();
	numberOfOpens = 0;
	isOpen = false;

    if (pOpen) {
        open();
    }

	if (pLogLevel == DEFAULT_LOGLEVEL) {
		setLogLevel(txp::Log::DEFAULT_OPEN_LOGLEVEL);
	} else {
		setLogLevel(pLogLevel);
	}

	return;
}

int txp::Log::logLevelCheck(const LogLevel pLogLevel) {
	int l_RC = 0;
    if (pLogLevel != NO_LOGLEVEL && getLogLevel() != NO_LOGLEVEL) {
		if (pLogLevel == ALL_LOGLEVEL || getLogLevel() == ALL_LOGLEVEL || pLogLevel <= getLogLevel()) {
			l_RC = 1;
		}
	}

	return l_RC;
}


void txp::Log::logLevelToChar( const LogLevel pLogLevel, char* pBuffer, const int32_t pSize ) {
	if ( pSize ) {
		pBuffer[0] = '\0';
		switch(pLogLevel) {
			case NO_LOGLEVEL:
				strCpy(pBuffer, "NO_LOGLEVEL", pSize);
				break;
			case ALL_LOGLEVEL:
				strCpy(pBuffer, "ALL_LOGLEVEL", pSize);
				break;
			case ERROR_LOGLEVEL:
				strCpy(pBuffer, "ERROR_LOGLEVEL", pSize);
				break;
			case WARN_LOGLEVEL:
				strCpy(pBuffer, "WARN_LOGLEVEL", pSize);
				break;
			case DIAG_LOGLEVEL:
				strCpy(pBuffer, "DIAG_LOGLEVEL", pSize);
				break;
			case INFO_LOGLEVEL:
				strCpy(pBuffer, "INFO_LOGLEVEL", pSize);
				break;
			case DEBUG_LOGLEVEL:
				strCpy(pBuffer, "DEBUG_LOGLEVEL", pSize);
				break;
			case DEBUG_LOGLEVEL_2:
				strCpy(pBuffer, "DEBUG_LOGLEVEL_2", pSize);
				break;
			case DEBUG_LOGLEVEL_3:
				strCpy(pBuffer, "DEBUG_LOGLEVEL_3", pSize);
				break;
			case DEBUG_LOGLEVEL_4:
				strCpy(pBuffer, "DEBUG_LOGLEVEL_4", pSize);
				break;
			case DEBUG_LOGLEVEL_5:
				strCpy(pBuffer, "DEBUG_LOGLEVEL_5", pSize);
				break;
			case C_DTOR_LOGLEVEL:
				strCpy(pBuffer, "C_DTOR_LOGLEVEL", pSize);
				break;
			case C_DTOR_LOGLEVEL_2:
				strCpy(pBuffer, "C_DTOR_LOGLEVEL_2", pSize);
				break;
			case ENTER_EXIT_LOGLEVEL:
				strCpy(pBuffer, "ENTER_EXIT_LOGLEVEL", pSize);
				break;

			default:
				snprintf(pBuffer, pSize, "%s (%d)", "USER_DEFINED", pLogLevel);
				break;
		}
	}

	return;
}

int txp::Log::open() {
	int l_RC = 0;

	if (!isOpen) {
		if (logFile[0]) {
			if (numberOfOpens==0) {
  				logFileStream.open(logFile, logFileMode);
  			} else {
  				logFileStream.open(logFile, std::ofstream::out | std::ofstream::app);
  			}
  		}
		isOpen = true;
		++numberOfOpens;
	}

	return l_RC;
}

int txp::Log::processBuffer(const char* pBuffer, const LogLevel pLogLevel, const Type pType, const Destination pDestination) {
	int l_RC = 0;

	if ( isOpen ) {
//		printf("getLogLevel()=%d, pLogLevel=%d\n", getLogLevel(), pLogLevel);
		if (logLevelCheck(pLogLevel)) {
			if ( toBoost(pDestination) ) {
				switch (pType) {
					case INFO:
					case DIAG:
						LOG(txp,info) << pBuffer;
						break;
					case WARN:
						LOG(txp,warning) << pBuffer;
						break;
					case ERROR:
						LOG(txp,error) << pBuffer;
						break;
					case DEBUG:
					case DBG2:
					case DBG3:
					case DBG4:
					case DBG5:
						LOG(txp,debug) << pBuffer;
						break;
					default:
						LOG(txp,always) << pBuffer;
						break;
				}
			} else {
				if ( toConsole(pDestination) ) {
					std::cout << pBuffer << std::endl << std::flush;
				}
				if ( toLogFile(pDestination) ) {
  					logFileStream << pBuffer << std::endl << std::flush;
				}
			}
		}
	}

	return l_RC;
}

void txp::Log::typeToChar( const Type pType, char* pBuffer, const int32_t pSize ) {
	if ( pSize ) {
		pBuffer[0] = '\0';
		switch(pType) {
			case INFO:
				strCpy(pBuffer, "INFO", pSize);
				break;
			case DIAG:
				strCpy(pBuffer, "DIAG", pSize);
				break;
			case WARN:
				strCpy(pBuffer, "WARN", pSize);
				break;
			case ERROR:
				strCpy(pBuffer, "ERROR", pSize);
				break;
			case DEBUG:
				strCpy(pBuffer, "DEBUG", pSize);
				break;
			case DBG2:
				strCpy(pBuffer, "DBG2", pSize);
				break;
			case DBG3:
				strCpy(pBuffer, "DBG3", pSize);
				break;
			case DBG4:
				strCpy(pBuffer, "DBG4", pSize);
				break;
			case DBG5:
				strCpy(pBuffer, "DBG5", pSize);
				break;

			default:
				break;
		}
	}

	return;
}

int txp::Log::write(const char* pData, const Type pType, const LogLevel pLogLevel, const Destination* pDestination) {
	int l_RC = 0;
	Destination l_Destination;

	if ( pDestination ) {
		l_Destination = *pDestination;
	} else {
		l_Destination = destination;
	}

	if (l_Destination != NONE) {
		l_RC = write(pData, pType, pLogLevel, l_Destination);
	}

	return l_RC;
}

int txp::Log::write(const char* pData, const Type pType, const LogLevel pLogLevel, const Destination pDestination) {
	int l_RC = 0;

	char l_MsgType[6] = {'\0'};
	char l_CurrentTime[27] = {'\0'};
	char l_Buffer[292] = {'\0'};  // NOTE:  This allows for 255 characters pData...

	if (pDestination != NONE) {
		LogLevel l_LogLevel = pLogLevel;
		if (l_LogLevel == DEFAULT_LOGLEVEL) {
			switch (pType) {
				case INFO:
					l_LogLevel = INFO_LOGLEVEL;
					break;
				case DIAG:
					l_LogLevel = DIAG_LOGLEVEL;
					break;
				case WARN:
					l_LogLevel = WARN_LOGLEVEL;
					break;
				case ERROR:
					l_LogLevel = ERROR_LOGLEVEL;
					break;
				case DEBUG:
					l_LogLevel = DEBUG_LOGLEVEL;
					break;
				case DBG2:
					l_LogLevel = DEBUG_LOGLEVEL_2;
					break;
				case DBG3:
					l_LogLevel = DEBUG_LOGLEVEL_3;
					break;
				case DBG4:
					l_LogLevel = DEBUG_LOGLEVEL_4;
					break;
				case DBG5:
					l_LogLevel = DEBUG_LOGLEVEL_5;
					break;

				default:
					// Shouldn't get here...  @DLH
					break;
			}
		}


		if ( toBoost(pDestination) ) {
			//NOTE:  We offset the transport logging messages 2 spaces to the right
			//       of the BOOST logging messages...  @@DLH
			snprintf(l_Buffer, sizeof(l_Buffer), "%*s%s", getIndent()+2, "", pData);
        } else {
			typeToChar(pType, l_MsgType, sizeof(l_MsgType));
			getCurrentTime(l_CurrentTime, sizeof(l_CurrentTime));
			snprintf(l_Buffer, sizeof(l_Buffer), "%26s | %5s | %*s%s",
			    	 l_CurrentTime, l_MsgType, getIndent(), " ", pData);
		}

		l_RC = processBuffer(l_Buffer, l_LogLevel, pType, pDestination);
	}

	return l_RC;
}
