/* begin_generated_IBM_copyright_prolog                             */
/*                                                                  */
/* This is an automatically generated copyright prolog.             */
/* After initializing,  DO NOT MODIFY OR MOVE                       */
/* ================================================================ */
/*                                                                  */
/* Licensed Materials - Property of IBM                             */
/*                                                                  */
/* Blue Gene/Q                                                      */
/* CORAL                                                            */
/*                                                                  */
/* (C) Copyright IBM Corp.  2011, 2016                              */
/*                                                                  */
/* US Government Users Restricted Rights -                          */
/* Use, duplication or disclosure restricted                        */
/* by GSA ADP Schedule Contract with IBM Corp.                      */
/*                                                                  */
/* This software is available to you under the                      */
/* Eclipse Public License (EPL).                                    */
/*                                                                  */
/* ================================================================ */
/*                                                                  */
/* end_generated_IBM_copyright_prolog                               */

//! \file  SignalHandler.cc
//! \brief Methods for bgcios::SignalHandler and bgcios::SigtermHandler classes.

// Includes
#include "SignalHandler.h"
#include <assert.h>
#include <errno.h>
#include <execinfo.h>
#include <fcntl.h>
#include <iomanip>
#include <logging.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

int setHandler(info_handler_t handler, int pSignalNumber) {
  // Set a handler for the signal.
  LOG(fshipcld, always) << __PRETTY_FUNCTION__ << " setting handler";
  struct sigaction sigact;
  memset(&sigact, '\0', sizeof(sigact));
  sigact.sa_flags = SA_SIGINFO;
  sigact.sa_sigaction = handler;
  if (sigaction(pSignalNumber, &sigact, NULL) != 0) {
    LOG(fshipcld, always) << __PRETTY_FUNCTION__ << " errno=" << errno;
    return errno;
  }

  return 0;
}

// void SignalHandler::myHandler(int signum, siginfo_t *siginfo, void
// *sigcontext)

//! Pointer to SigWritePipe object so handler can find it.
void *SigWritePipe::thisPtr;

SigWritePipe::SigWritePipe(int signal_number) {
  // Set data members.
  for (int i = 0; i < MAXSIGNALS; i++)
    signalNumbers[i] = 0;
  signalNumbers[0] = signal_number;
  LOG(fshipcld, always) << __PRETTY_FUNCTION__ << " signo=" << signal_number;
  this->thisPtr = this;

  // pipe info for signal handler
  int rcPipe2 = pipe2(_pipe_descriptor, O_DIRECT | O_NONBLOCK);
  if (rcPipe2)
    LOG(fshipcld, always) << "pipe2 creation failed errno=" << errno << ":"
                          << strerror(errno);

  // Enable a handler for the specified signal.
  setHandler(myHandler, signal_number);
}

void SigWritePipe::addSignal(int signal_number) {
  // Set data members.
  for (int i = 0; i < MAXSIGNALS; i++) {
    if (!signalNumbers[i]) {
      signalNumbers[i] = signal_number;
      break;
    }
  }
  // Enable a handler for the specified signal.
  setHandler(myHandler, signal_number);
}

SigWritePipe::~SigWritePipe() {
  close(_pipe_descriptor[0]);
  _pipe_descriptor[0] = -1;
  close(_pipe_descriptor[1]);
  _pipe_descriptor[1] = -1;

  // need to set back to DFL action
  struct sigaction sigact;
  memset(&sigact, 0, sizeof(sigact));
  sigact.sa_handler = SIG_DFL;
  for (int i = 0; i < MAXSIGNALS; i++) {
    if (signalNumbers[i])
      sigaction(signalNumbers[i], &sigact, NULL);
  }
}

void SigWritePipe::myHandler(int, siginfo_t *siginfo, void *) {
  SigWritePipe *me = (SigWritePipe *)thisPtr;
  (void)write(me->writePipeFd(), siginfo, sizeof(siginfo_t));
  return;
}
