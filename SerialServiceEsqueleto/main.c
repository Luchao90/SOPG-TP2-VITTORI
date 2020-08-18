#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "SerialManager.h"
#include "UserSignals.h"
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>

#define msToUs(x) (x * 1000U)
#define END_STRING '\0'
#define PORT 10000
#define SA struct sockaddr

typedef struct
{
	pthread_mutex_t mutex;
	char echo[BUFFER_LENGTH];
} Terminal_t;

Terminal_t console = {
	.mutex = PTHREAD_MUTEX_INITIALIZER,
	.echo[0] = END_STRING};

typedef struct
{
	pthread_mutex_t mutex;
	int sockfd;
	int newfd;
	socklen_t len;
	struct sockaddr_in serveraddr;
	struct sockaddr_in clientaddr;
	char buff[BUFFER_LENGTH];
	int n;
} Socket_t;

Socket_t mysocket = {
	.mutex = PTHREAD_MUTEX_INITIALIZER,
};

void console_print(const char *string);

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
		console_print("thread main\r\n");
		pthread_join(usb, NULL);
		pthread_join(tcp, NULL);
		printf("\r\nExit succesful\r\n");

		exit(EXIT_SUCCESS);
		return 0;
	}
}

void *thread_usb(void *nothing)
{
	console_print("thread USB\r\n");
	pthread_mutex_lock(&mainPort.mutex);
	mainPort.file = serial_open(mainPort.number, mainPort.baudrate);
	pthread_mutex_unlock(&mainPort.mutex);

	while (1)
	{
		/* Send blinky option to edu ciaa */
		pthread_mutex_lock(&mainPort.mutex);
		mainPort.bytesReaded = serial_receive(mainPort.buffer, BUFFER_LENGTH);
		strncpy(console.echo, mainPort.buffer, mainPort.bytesReaded);
		console.echo[mainPort.bytesReaded] = END_STRING;
		pthread_mutex_unlock(&mainPort.mutex);
		usleep(msToUs(50));

		if (mainPort.bytesReaded > 0)
		{
			pthread_mutex_lock(&console.mutex);
			printf("EDU-CIAA: %s", console.echo);
			pthread_mutex_unlock(&console.mutex);
		}
	}
	return NULL;
}

void *thread_tcp(void *nothing)
{
	console_print("thread TCP\r\n");

	/* Create socket */
	mysocket.sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (mysocket.sockfd == -1)
	{
		console_print("socket creation failed...\n");
		pthread_exit(NULL);
	}
	else
	{
		console_print("Socket successfully created..\n");
	}
	/* assign IP, PORT  */
	bzero(&mysocket.serveraddr, sizeof(mysocket.serveraddr));
	mysocket.serveraddr.sin_family = AF_INET;
	mysocket.serveraddr.sin_port = htons(PORT);
	//mysocket.serveraddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	if (inet_pton(AF_INET, "127.0.0.1", &(mysocket.serveraddr.sin_addr)) <= 0)
	{
		console_print("ERROR invalid server IP\r\n");
		pthread_exit(NULL);
	}

	// Binding newly created socket to given IP and verification
	if ((bind(mysocket.sockfd, (SA *)&mysocket.serveraddr, sizeof(mysocket.serveraddr))) != 0)
	{
		close(mysocket.sockfd);
		console_print("socket bind failed...\n");
		pthread_exit(NULL);
	}

	// Now server is ready to listen and verification
	if ((listen(mysocket.sockfd, 10)) != 0)
	{
		console_print("Listen failed...\n");
		pthread_exit(NULL);
	}
	else
	{
		console_print("TCP listening..\n");
	}

	while (1)
	{
		mysocket.len = sizeof(mysocket.clientaddr);
		// Accept the data packet from client and verification
		mysocket.newfd = accept(mysocket.sockfd, (SA *)&mysocket.clientaddr, &mysocket.len);
		if (mysocket.newfd < 0)
		{
			console_print("server acccept failed...\n");
			exit(1);
		}

		char ipClient[32];
		inet_ntop(AF_INET, &(mysocket.clientaddr.sin_addr), ipClient, sizeof(ipClient));
		pthread_mutex_lock(&console.mutex);
		printf("server:  conexion desde:  %s\n", ipClient);
		pthread_mutex_unlock(&console.mutex);

		bzero(mysocket.buff, BUFFER_LENGTH);

		while (1)
		{
			if ((mysocket.n = read(mysocket.newfd, mysocket.buff, sizeof(mysocket.buff))) == -1)
			{
				console_print("Error leyendo mensaje en socket\r\n");
				return NULL;
			}

			if (mysocket.n == 0)
			{
				return NULL;
			}

			mysocket.buff[mysocket.n] = END_STRING;
			pthread_mutex_lock(&console.mutex);
			printf("Interace service: %s", mysocket.buff);
			pthread_mutex_unlock(&console.mutex);

			pthread_mutex_lock(&mainPort.mutex);
			//Creacion del string para enviar por la UART
			sprintf(mainPort.buffer, ">OUTS:%c,%c,%c,%c\r\n", mysocket.buff[7], mysocket.buff[8], mysocket.buff[9], mysocket.buff[10]);
			serial_send(mainPort.buffer, sizeof(mainPort.buffer));
			pthread_mutex_unlock(&mainPort.mutex);

			/* Request to client */
			/* write(mysocket.sockfd, buff, sizeof(buff)); */
		}
	}
	return NULL;
}

void console_print(const char *string)
{
	pthread_mutex_lock(&console.mutex);
	printf("%s", string);
	pthread_mutex_unlock(&console.mutex);
}