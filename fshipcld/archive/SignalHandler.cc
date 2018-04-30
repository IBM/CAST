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
#include <sys/types.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <execinfo.h>
#include <iomanip>
#include <unistd.h>
#include <fcntl.h>
#include <logging.h>


//! Pointer to SigtermHandler object so handler can find it.
void *SigtermHandler::thisPtr;

SignalHandler::SignalHandler(int signum)
{
   // Set data members.
   _signalNumber = signum;

   // Enable handler for the signal.
   setHandler();
}

int
SignalHandler::setHandler(info_handler_t handler)
{
   // Set a handler for the signal.
  LOG(fshipcld,always)<<__PRETTY_FUNCTION__<<" setting handler";
   struct sigaction sigact;
   memset(&sigact, '\0', sizeof(sigact));
   sigact.sa_flags |= SA_SIGINFO;
   sigact.sa_sigaction = handler;
   if (sigaction(_signalNumber, &sigact, NULL) != 0) {
      LOG(fshipcld,always)<<__PRETTY_FUNCTION__<<" errno="<<errno;
      return errno;
   }
   
   return 0;
}

int
SignalHandler::setIgnore(void)
{
   // Ignore the signal.
   int err = 0;
   struct sigaction sigact;
   memset(&sigact, '\0', sizeof(sigact));
   sigact.sa_handler = SIG_IGN;
   if (sigaction(_signalNumber, &sigact, NULL) != 0) {
      err = errno;
   }

   return err;
}

int
SignalHandler::setDefault(void)
{
   // Set default action for the signal.
   int err = 0;
   struct sigaction sigact;
   memset(&sigact, '\0', sizeof(sigact));
   sigact.sa_handler = SIG_DFL;
   if (sigaction(_signalNumber, &sigact, NULL) != 0) {
      err = errno;
      //LOG(fshipcld,always)<<"sigaction to default for signal=" << _signalNumber << " errno="<<errno;
   }

   return err;
}

void
SignalHandler::myHandler(int signum, siginfo_t *siginfo, void *sigcontext)
{
#ifdef __PPC64__
   // Log information from the context.
   ucontext_t *context = (ucontext_t *)sigcontext;
   /*//LOG(fshipcld,always)<<"Received signal " << signum << ", code=" << siginfo->si_code << " errno=" << siginfo->si_errno <<
                 " address=" << siginfo->si_addr << " nip=" << (void *)(context->uc_mcontext.regs->nip) <<
                 " lr=" << (void *)(context->uc_mcontext.regs->link));*/
   if (context) context=NULL; //shut off compiler warning
#else

   // Don't do anything, just need to reference the variables so it compiles.
   siginfo = 0; sigcontext = 0;

#endif


   // Log a back trace of the stack.
   //logStackBackTrace(MaxBackTraceSize);

   // End the program.
   switch (signum){
    case SIGABRT:
    case SIGSEGV:
    {  
      struct sigaction sigact;
      memset(&sigact,0,sizeof(sigact) );
      sigact.sa_handler = SIG_DFL;
      sigaction(signum,&sigact,NULL);
      //printlastLogEntries(4);
      raise(signum);
      break;
    }
    default:
      raise(signum);
  }
}

SigtermHandler::SigtermHandler()
{
   // Set data members.
   _signalNumber = SIGTERM;
   _caught = false;
   this->thisPtr = this;

   // Enable a handler for the specified signal.
   setHandler(myHandler);
}


void SigtermHandler::myHandler(int signum, siginfo_t *siginfo, void *sigcontext)

{
  /*LOG(fshipcld,always)<<"Received signal " << signum << ", code=" << siginfo->si_code << " errno=" << siginfo->si_errno <<
                " address=" << siginfo->si_addr << " nip=" << (void *)(((ucontext_t *)sigcontext)->uc_mcontext.regs->nip) <<
                " lr=" << (void *)(((ucontext_t *)sigcontext)->uc_mcontext.regs->link);
  */

   // Remember that signal has been caught.
   SigtermHandler *me = (SigtermHandler *)thisPtr;
   me->_caught = true;

   return;
}

Sigusr2Handler::Sigusr2Handler()
{
   // Set data members.
   _signalNumber = SIGUSR2;

   // Enable a handler for the specified signal.
   setHandler(myHandler);
}

void
Sigusr2Handler::myHandler(int, siginfo_t *, void *)
{  

   return;
}

//! Pointer to SigWritePipe object so handler can find it.
void *SigWritePipe::thisPtr;

SigWritePipe::SigWritePipe(int signal_number)
{
   // Set data members.
   _signalNumber = signal_number;
   LOG(fshipcld,always)<<__PRETTY_FUNCTION__<<" signo="<<signal_number;
   this->thisPtr = this;

   // Enable a handler for the specified signal.
   setHandler(myHandler);
   
   // pipe info for signal handler
   int rcPipe2 = pipe2(_pipe_descriptor,O_DIRECT|O_NONBLOCK);
   if (rcPipe2) LOG(fshipcld,always)<<"pipe2 creation failed errno="<<errno<<":"<<strerror(errno);
}

SigWritePipe::~SigWritePipe()
{
    close( _pipe_descriptor[0] );
    _pipe_descriptor[0] = -1;
    close( _pipe_descriptor[1] );
    _pipe_descriptor[1] = -1;

    //need to set back to DFL action
    struct sigaction sigact;
    memset(&sigact,0,sizeof(sigact) );
    sigact.sa_handler = SIG_DFL;
    sigaction(_signalNumber, &sigact, NULL);
}

void
SigWritePipe::myHandler(int , siginfo_t *siginfo, void *)
{  
   SigWritePipe *me = (SigWritePipe *)thisPtr;
   (void)write( me->writePipeFd(), siginfo, sizeof(siginfo_t) );
   return;
}
