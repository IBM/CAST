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

//! \file  SignalHandler.h
//! \brief Declaration and inline methods for bgcios::SignalHandler and
//! bgcios::SigtermHandler classes.

#ifndef FSHIP_SIGNALHANDLER_H
#define FSHIP_SIGNALHANDLER_H

// Includes
#include <signal.h>

typedef void (*info_handler_t)(int, siginfo_t *, void *);

//! \brief Provide a handler for signals which will use a pipe for writing.
class SigWritePipe {
public:
  static const int MAXSIGNALS = 32;
  //! Constructor.

  SigWritePipe(int);

  ~SigWritePipe();

  //! \brief  Return indicator if signal has been caught.
  //! \return True if signal has been caught, otherwise false.

  //! \brief Return read descriptor of pipe
  int readPipeFd() { return _pipe_descriptor[0]; }

  //! \brief Return write descriptor of pipe
  int writePipeFd() { return _pipe_descriptor[1]; }

  //! \brief  Handler function that is run when signal is delivered.
  //! \param  signum Signal number.
  //! \param  siginfo Pointer to signal info structure with extended info about
  //! current state.
  //! \param  sigcontext Pointer to context at time signal was delivered.
  //! \return Nothing.

  static void myHandler(int signum, siginfo_t *siginfo, void *sigcontext);

  //! Pointer to this object so handler can access it.
  static void *thisPtr;

  int _pipe_descriptor[2];

  void addSignal(int signal_number);

private:
  int signalNumbers[MAXSIGNALS];
};

#endif // FSHIP_SIGNALHANDLER_H
