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
#define CONNECTION_LOST -1
#define PAYLOAD_LENGTH sizeof(":STATESXYWZ") //:STATESXYWZ
#define LINE_A_STATE_POSITION 7
#define LINE_B_STATE_POSITION 8
#define LINE_C_STATE_POSITION 9
#define LINE_D_STATE_POSITION 10

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
	int connection;
	int sockfd;
	int newfd;
	socklen_t len;
	struct sockaddr_in serveraddr;
	struct sockaddr_in clientaddr;
	char buff[BUFFER_LENGTH];
	char buff_tx[BUFFER_LENGTH];
	int n;
	char ipClient[32];
} Socket_t;

Socket_t mysocket = {
	.mutex = PTHREAD_MUTEX_INITIALIZER,
	.connection = CONNECTION_LOST};

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
	int thread_usb_created;
	int thread_tcp_created;

	signals_init();
	signals_thread_disable();
	thread_usb_created = pthread_create(&usb, NULL, thread_usb, NULL);
	thread_tcp_created = pthread_create(&tcp, NULL, thread_tcp, NULL);

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
		while (user_signal != EXIT_SIGNAL)
		{
			sleep(1);
		}
		pthread_cancel(usb);
		pthread_cancel(tcp);
		pthread_join(usb, NULL);
		pthread_join(tcp, NULL);
		serial_close();
		close(mysocket.sockfd);
		close(mysocket.newfd);
		printf("\r\nExit program\r\n");

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

			if (strncmp(mainPort.buffer, ">OK\r\n", strlen(">OK\r\n")) != 0)
			{
				//Creacion del buffer para enviar por el socket
				pthread_mutex_lock(&mysocket.mutex);
				sprintf(mysocket.buff_tx, ":LINE%cTG\n", mainPort.buffer[14]);
				pthread_mutex_unlock(&mysocket.mutex);

				pthread_mutex_lock(&console.mutex);
				printf("Envio por el socket: %s", mysocket.buff_tx);
				pthread_mutex_unlock(&console.mutex);

				if (mysocket.connection != CONNECTION_LOST)
				{
					//Escribo por el socket
					if (write(mysocket.newfd, mysocket.buff_tx, strlen(mysocket.buff_tx)) == -1)
					{
						perror("Error escribiendo mensaje en el socket\r\n");
						exit(1);
					}
				}
			}
		}
	}
	pthread_exit(NULL);
}

void *thread_tcp(void *nothing)
{
	char line_A_state, line_B_state, line_C_state, line_D_state;

	console_print("thread TCP\r\n");

	/* Create socket */
	mysocket.sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (mysocket.sockfd == -1)
	{
		console_print("Socket creation failed...\r\n");
		pthread_exit(NULL);
	}
	else
	{
		console_print("Socket successfully created..\r\n");
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
		console_print("Socket bind failed...\r\n");
		pthread_exit(NULL);
	}

	// Now server is ready to listen and verification
	if ((listen(mysocket.sockfd, 10)) != 0)
	{
		console_print("Listen failed...\r\n");
		pthread_exit(NULL);
	}
	else
	{
		console_print("TCP listening..\r\n");
	}

	while (1)
	{
		mysocket.len = sizeof(mysocket.clientaddr);
		// Accept the data packet from client and verification
		mysocket.newfd = accept(mysocket.sockfd, (SA *)&mysocket.clientaddr, &mysocket.len);
		if (mysocket.newfd < 0)
		{
			console_print("Server acccept failed...\r\n");
			pthread_exit(NULL);
		}

		inet_ntop(AF_INET, &(mysocket.clientaddr.sin_addr), mysocket.ipClient, sizeof(mysocket.ipClient));
		pthread_mutex_lock(&console.mutex);
		printf("Server connection from:  %s\r\n", mysocket.ipClient);
		pthread_mutex_unlock(&console.mutex);
		mysocket.connection = 1;

		while (mysocket.connection != CONNECTION_LOST)
		{
			if ((mysocket.n = read(mysocket.newfd, mysocket.buff, sizeof(mysocket.buff))) == -1)
			{
				console_print("Error leyendo mensaje en socket\r\n");
				pthread_exit(NULL);
			}
			if (mysocket.n == 0)
			{
				mysocket.connection = CONNECTION_LOST;
				close(mysocket.newfd);
				console_print("Connection lost, waiting for new\r\n");
			}

			if (mysocket.n >= PAYLOAD_LENGTH)
			{
				mysocket.buff[mysocket.n] = END_STRING;
				line_A_state = mysocket.buff[LINE_A_STATE_POSITION];
				line_B_state = mysocket.buff[LINE_B_STATE_POSITION];
				line_C_state = mysocket.buff[LINE_C_STATE_POSITION];
				line_D_state = mysocket.buff[LINE_D_STATE_POSITION];

				pthread_mutex_lock(&console.mutex);
				printf("Interace service: %s", mysocket.buff);
				pthread_mutex_unlock(&console.mutex);

				pthread_mutex_lock(&mainPort.mutex);
				sprintf(mainPort.buffer, ">OUTS:%c,%c,%c,%c\r\n", line_A_state, line_B_state, line_C_state, line_D_state);
				serial_send(mainPort.buffer, sizeof(mainPort.buffer));
				pthread_mutex_unlock(&mainPort.mutex);
			}
		}
		sleep(1);
	}
	pthread_exit(NULL);
}

void console_print(const char *string)
{
	pthread_mutex_lock(&console.mutex);
	printf("%s", string);
	pthread_mutex_unlock(&console.mutex);
}