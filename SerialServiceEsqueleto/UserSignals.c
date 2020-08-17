/**
 * @file UserSignals.c
 * @author Luciano Francisco Vittori (lucianovittori99@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2020-08-17
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#include "UserSignals.h"

extern pthread_t usb;

static void signals_handler(int sig);

void signals_init(void)
{
  SigTerm.sa_handler = signals_handler;
  SigTerm.sa_flags = 0;
  sigemptyset(&SigTerm.sa_mask);

  SigInt.sa_handler = signals_handler;
  SigInt.sa_flags = 0;
  sigemptyset(&SigInt.sa_mask);

  /* Map handlers for user signals */
  sigaction(SIGTERM, &SigTerm, NULL);
  sigaction(SIGINT, &SigInt, NULL);
}

void signals_thread_disable(void)
{
  sigset_t set;
  sigemptyset(&set);
  sigfillset(&set);
  pthread_sigmask(SIG_BLOCK, &set, NULL);
}

void signals_thread_enable(void)
{
  sigset_t set;
  sigemptyset(&set);
  sigaddset(&set, SIGINT);
  sigaddset(&set, SIGTERM);
  pthread_sigmask(SIG_UNBLOCK, &set, NULL);
}

static void signals_handler(int sig)
{
  pthread_cancel(usb);
}