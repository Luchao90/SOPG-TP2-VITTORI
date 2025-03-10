/**
 * @file UserSignals.h
 * @author Luciano Francisco Vittori (lucianovittori99@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2020-08-17
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#define EXIT_SIGNAL 1

volatile sig_atomic_t user_signal;

void signals_init(void);
void signals_thread_disable(void);
void signals_thread_enable(void);