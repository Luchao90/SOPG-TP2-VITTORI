#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "SerialManager.h"
#include "UserSignals.h"
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

#define msToUs(x) (x * 1000U)
#define END_STRING '\0'

typedef struct
{
	pthread_mutex_t mutex;
	char echo[BUFFER_LENGTH];
} Terminal_t;

Terminal_t console = {
	.mutex = PTHREAD_MUTEX_INITIALIZER,
	.echo[0] = END_STRING};

SerialPort mainPort = {
	.mutex = PTHREAD_MUTEX_INITIALIZER,
	.file = 0,
	.buffer = ">OUTS:2,2,2,2\r\n",
	.number = 1,
	.baudrate = 115200,
	.bytesReaded = 0};

pthread_t usb, tcp;
void *thread_usb(void *nothing);
void *thread_tcp(void *nothing);

int main(void)
{
	void *usb_ret;
	int thread_usb_created;
	int thread_tcp_created;

	signals_init();
	signals_thread_disable();
	thread_usb_created = pthread_create(&usb, NULL, thread_usb, NULL);
	thread_tcp_created = pthread_create(&usb, NULL, thread_tcp, NULL);
	if (thread_usb_created != 0)
	{
		errno = thread_usb_created;
		perror("pthread_create");
		return -1;
	}
	else if (thread_tcp_created != 0)
	{
		errno = thread_tcp_created;
		perror("pthread_create");
		return -1;
	}
	else
	{
		signals_thread_enable();
		pthread_mutex_lock(&console.mutex);
		printf("thread main\r\n");
		pthread_mutex_unlock(&console.mutex);
		pthread_join(usb, NULL);
		pthread_join(tcp, NULL);
		printf("\r\nExit succesful\r\n");

		exit(EXIT_SUCCESS);
		return 0;
	}
}

void *thread_usb(void *nothing)
{
	pthread_mutex_lock(&console.mutex);
	printf("thread USB\r\n");
	pthread_mutex_unlock(&console.mutex);

	mainPort.file = serial_open(mainPort.number, mainPort.baudrate);
	serial_send(mainPort.buffer, strlen(mainPort.buffer));

	while (1)
	{
		mainPort.bytesReaded = serial_receive(mainPort.buffer, BUFFER_LENGTH);
		usleep(msToUs(50));

		pthread_mutex_lock(&console.mutex);
		strncpy(console.echo, mainPort.buffer, mainPort.bytesReaded);
		console.echo[mainPort.bytesReaded] = END_STRING;
		if (mainPort.bytesReaded > 0)
		{
			printf("RX: %s", console.echo);
		}
		pthread_mutex_unlock(&console.mutex);
	}
	return NULL;
}

void *thread_tcp(void *nothing)
{
	pthread_mutex_lock(&console.mutex);
	printf("thread TCP\r\n");
	pthread_mutex_unlock(&console.mutex);

	while (1)
	{
		sleep(1);
		pthread_mutex_lock(&console.mutex);
		strncpy(console.echo, mainPort.buffer, mainPort.bytesReaded);
		console.echo[mainPort.bytesReaded] = END_STRING;
		if (mainPort.bytesReaded > 0)
		{
			printf("TCP: %s", console.echo);
		}
		pthread_mutex_unlock(&console.mutex);
	}
	return NULL;
}