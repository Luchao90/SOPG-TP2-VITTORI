#include <stdio.h>
#include <stdlib.h>
#include "SerialManager.h"
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#define msToUs(x) (x * 1000U)
#define END_STRING '\0'

static pthread_t usb, tcp;

void *thread_usb(void *nothing);

int main(void)
{
	void *usb_ret;

	pthread_create(&usb, NULL, thread_usb, NULL);
	printf("Running..\r\n");
	pthread_join(usb, &usb_ret);
	printf("Exit\r\n");

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