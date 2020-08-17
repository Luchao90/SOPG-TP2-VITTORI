#include <stdio.h>
#include <stdlib.h>
#include "SerialManager.h"
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

#define msToUs(x) (x * 1000U)
#define END_STRING '\0'
#define MSG_SIGUSER_1 "SIGUSR1"
#define MSG_CRTL_C "CONTROL C"

static pthread_t usb, tcp;

void *thread_usb(void *nothing);

void signals_disable(void);
void signals_enable(void);
void handler_SigUser1(int sig);
void handler_SigInt(int sig);

int main(void)
{
	void *usb_ret;

    struct sigaction SigUser1;
    struct sigaction SigInt;

    SigUser1.sa_handler = handler_SigUser1;
    SigUser1.sa_flags = 0;
    sigemptyset(&SigUser1.sa_mask);

    SigInt.sa_handler = handler_SigInt;
    SigInt.sa_flags = 0;
    sigemptyset(&SigInt.sa_mask);

    /* Map handlers for user signals */
    sigaction(SIGUSR1, &SigUser1, NULL);
    sigaction(SIGINT, &SigInt, NULL);

	signals_disable();
	pthread_create(&usb, NULL, thread_usb, NULL);
	signals_enable();
	printf("Running..\r\n");
	pthread_join(usb, &usb_ret);
	printf("\r\nExit succesful\r\n");

	exit(EXIT_SUCCESS);
	return 0;
}

void *thread_usb(void *nothing)
{
	SerialPort mainPort = {.file = 0, .buffer = ">OUTS:2,2,2,2\r\n", .number = 1, .baudrate = 115200, .bytesReaded = 0};
	char echo_terminal[BUFFER_LENGTH];

	printf("thread USB\r\n");
	mainPort.file = serial_open(mainPort.number, mainPort.baudrate);
	serial_send(mainPort.buffer, strlen(mainPort.buffer));

	while (1)
	{
		mainPort.bytesReaded = serial_receive(mainPort.buffer, BUFFER_LENGTH);
		usleep(msToUs(50));

		strncpy(echo_terminal, mainPort.buffer, mainPort.bytesReaded);
		echo_terminal[mainPort.bytesReaded] = END_STRING;

		if (mainPort.bytesReaded > 0)
		{
			printf("RX: %s", echo_terminal);
		}
	}
	return NULL;
}

void signals_disable(void)
{
	sigset_t set;
	sigemptyset(&set);
	sigfillset(&set);
	pthread_sigmask(SIG_BLOCK, &set, NULL);
}

void signals_enable(void)
{
	sigset_t set;
	sigemptyset(&set);
	sigaddset(&set, SIGINT);
	sigaddset(&set, SIGUSR1);
	pthread_sigmask(SIG_UNBLOCK, &set, NULL);
}

void handler_SigUser1(int sig)
{
	pthread_cancel(usb);
}
void handler_SigInt(int sig)
{
	pthread_cancel(usb);
}