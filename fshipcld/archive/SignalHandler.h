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
//! \brief Declaration and inline methods for bgcios::SignalHandler and bgcios::SigtermHandler classes.

#ifndef FSHIP_SIGNALHANDLER_H
#define FSHIP_SIGNALHANDLER_H

// Includes
#include <signal.h>

typedef void (*info_handler_t) (int, siginfo_t *, void *);


//! Maximum number of entries in a stack back trace.
static const int MaxBackTraceSize = 100;

//! \brief Provide a handler for a specified signal.

class SignalHandler
{
public:

   //! \brief  Default constructor.

   SignalHandler() { _signalNumber = 0; }

   //! \brief  Constructor.
   //! \param  signum Signal number to handle.

   SignalHandler(int signum);

   //! \brief  Default destructor.

   ~SignalHandler() { } //! \todo Should remove the handler?

   //! \brief  Set the action for the signal to run the default handler.
   //! \return 0 when successful, errno when unsuccessful.

   int setHandler(void) { return setHandler(myHandler); }

   //! \brief  Set the action for the signal to run the specified handler.
   //! \param  handler Pointer to handler function.
   //! \return 0 when successful, errno when unsuccessful.

   int setHandler(info_handler_t handler);

   //! \brief  Set the action for the signal to ignore the signal.
   //! \return 0 when successful, errno when unsuccessful.

   int setIgnore(void);

   //! \brief  Set the action for the signal to the default action.
   //! \return 0 when successful, errno when unsuccessful.

   int setDefault(void);

   //! \brief  Get the signal number being handled.
   //! \return Signal number.

   int getSignalNumber(void) const { return _signalNumber; }

   //! \brief  Handler function that is run when signal is delivered.
   //! \param  signum Signal number.
   //! \param  siginfo Pointer to signal info structure with extended info about current state.
   //! \param  sigcontext Pointer to context at time signal was delivered.
   //! \return Nothing.

   static void myHandler(int signum, siginfo_t *siginfo, void *sigcontext);

protected:

   //! Signal number being handled.
   int _signalNumber;
   
};

//! \brief Provide a handler for TERM signals.

class SigtermHandler : public SignalHandler
{
public:

   //! Default constructor.

   SigtermHandler();

   //! \brief  Return indicator if signal has been caught.
   //! \return True if signal has been caught, otherwise false.

   bool isCaught(void) { return _caught; }

   //! \brief  Handler function that is run when signal is delivered.
   //! \param  signum Signal number.
   //! \param  siginfo Pointer to signal info structure with extended info about current state.
   //! \param  sigcontext Pointer to context at time signal was delivered.
   //! \return Nothing.

   static void myHandler(int signum, siginfo_t *siginfo, void *sigcontext);

   //! Pointer to this object so handler can access it.
   static void *thisPtr;

private:

   //! True when signal has been caught by handler.
   volatile bool _caught;

};

//! \brief Provide a handler for USR2 signals.

class Sigusr2Handler : public SignalHandler
{
public:

   //! Default constructor.

   Sigusr2Handler();

   //! \brief  Handler function that is run when signal is delivered.
   //! \param  signum Signal number.
   //! \param  siginfo Pointer to signal info structure with extended info about current state.
   //! \param  sigcontext Pointer to context at time signal was delivered.
   //! \return Nothing.

   static void myHandler(int signum, siginfo_t *siginfo, void *sigcontext);

};
//! \brief Provide a handler for signals which will use a pipe for writing.
class SigWritePipe : public SignalHandler
{
public:

   //! Constructor.

   SigWritePipe(int);

   ~SigWritePipe();

   //! \brief  Return indicator if signal has been caught.
   //! \return True if signal has been caught, otherwise false.

   //! \brief Return read descriptor of pipe
   int readPipeFd(){return _pipe_descriptor[0];}

   //! \brief Return write descriptor of pipe
   int writePipeFd(){return _pipe_descriptor[1];}

   //! \brief  Handler function that is run when signal is delivered.
   //! \param  signum Signal number.
   //! \param  siginfo Pointer to signal info structure with extended info about current state.
   //! \param  sigcontext Pointer to context at time signal was delivered.
   //! \return Nothing.

   static void myHandler(int signum, siginfo_t *siginfo, void *sigcontext);

   //! Pointer to this object so handler can access it.
   static void *thisPtr;

   int _pipe_descriptor[2];


private:
   
};


#endif // FSHIP_SIGNALHANDLER_H

