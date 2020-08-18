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
#define USB_MIN_PAYLOAD sizeof(">OK")
#define PORT 10000
#define SA struct sockaddr
#define CONNECTION_LOST -1
#define LISTEN_QUEUE_LENGTH 5
#define PAYLOAD_LENGTH sizeof(":STATESXYWZ") //:STATESXYWZ
#define LINE_A_STATE_POSITION 7
#define LINE_B_STATE_POSITION 8
#define LINE_C_STATE_POSITION 9
#define LINE_D_STATE_POSITION 10
#define LINE_X_CHANGE_POSITION 14

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

Socket_t tcp = {
	.mutex = PTHREAD_MUTEX_INITIALIZER,
	.connection = CONNECTION_LOST};

void console_print(const char *string);

SerialPort usb = {
	.mutex = PTHREAD_MUTEX_INITIALIZER,
	.file = 0,
	.buffer = ">OUTS:2,2,2,2\r\n",
	.number = 1,
	.baudrate = 115200,
	.bytesReaded = 0};

pthread_t usb_thread, tcp_thread;
void *thread_usb(void *nothing);
void *thread_tcp(void *nothing);

int main(void)
{
	int thread_usb_created;
	int thread_tcp_created;

	signals_init();
	signals_thread_disable();
	thread_usb_created = pthread_create(&usb_thread, NULL, thread_usb, NULL);
	thread_tcp_created = pthread_create(&tcp_thread, NULL, thread_tcp, NULL);

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
		pthread_cancel(usb_thread);
		pthread_cancel(tcp_thread);
		pthread_join(usb_thread, NULL);
		pthread_join(tcp_thread, NULL);
		serial_close();
		close(tcp.sockfd);
		close(tcp.newfd);
		printf("\r\nExit program\r\n");

		exit(EXIT_SUCCESS);
		return 0;
	}
}

void *thread_usb(void *nothing)
{
	console_print("thread USB\r\n");

	pthread_mutex_lock(&usb.mutex);
	usb.file = serial_open(usb.number, usb.baudrate);
	pthread_mutex_unlock(&usb.mutex);

	while (1)
	{
		pthread_mutex_lock(&usb.mutex);
		usb.bytesReaded = serial_receive(usb.buffer, BUFFER_LENGTH);
		strncpy(console.echo, usb.buffer, usb.bytesReaded);
		console.echo[usb.bytesReaded] = END_STRING;
		pthread_mutex_unlock(&usb.mutex);
		usleep(msToUs(50));

		if (usb.bytesReaded >= USB_MIN_PAYLOAD)
		{
			pthread_mutex_lock(&console.mutex);
			printf("USB-RX: %s", console.echo);
			pthread_mutex_unlock(&console.mutex);

			if (strncmp(usb.buffer, ">OK\r\n", strlen(">OK\r\n")) != 0)
			{
				/* Socket tx buffer create */
				pthread_mutex_lock(&tcp.mutex);
				sprintf(tcp.buff_tx, ":LINE%cTG\n", usb.buffer[LINE_X_CHANGE_POSITION]);
				pthread_mutex_unlock(&tcp.mutex);

				//Escribo por el socket
				if (write(tcp.newfd, tcp.buff_tx, sizeof(tcp.buff_tx)) == -1)
				{
					console_print("No socket connection, waiting for new\r\n");
				}
				else
				{
					/* console echo */
					pthread_mutex_lock(&console.mutex);
					printf("Send to socket: %s", tcp.buff_tx);
					pthread_mutex_unlock(&console.mutex);
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
	tcp.sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (tcp.sockfd == -1)
	{
		console_print("Socket creation failed...\r\n");
		pthread_exit(NULL);
	}
	else
	{
		console_print("Socket successfully created..\r\n");
	}
	/* assign IP and PORT  */
	bzero(&tcp.serveraddr, sizeof(tcp.serveraddr));
	tcp.serveraddr.sin_family = AF_INET;
	tcp.serveraddr.sin_port = htons(PORT);
	//tcp.serveraddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	if (inet_pton(AF_INET, "127.0.0.1", &(tcp.serveraddr.sin_addr)) <= 0)
	{
		console_print("ERROR invalid server IP\r\n");
		pthread_exit(NULL);
	}

	/* Binding newly created socket to given IP and verification */
	if ((bind(tcp.sockfd, (SA *)&tcp.serveraddr, sizeof(tcp.serveraddr))) != 0)
	{
		console_print("Socket bind failed...\r\n");
		pthread_exit(NULL);
	}

	/* Now server is ready to listen and verification */
	if ((listen(tcp.sockfd, LISTEN_QUEUE_LENGTH)) != 0)
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
		tcp.len = sizeof(tcp.clientaddr);

		/* Accept the data packet from client and verification */
		tcp.newfd = accept(tcp.sockfd, (SA *)&tcp.clientaddr, &tcp.len);
		if (tcp.newfd < 0)
		{
			console_print("Server acccept failed...\r\n");
			pthread_exit(NULL);
		}

		inet_ntop(AF_INET, &(tcp.clientaddr.sin_addr), tcp.ipClient, sizeof(tcp.ipClient));
		pthread_mutex_lock(&console.mutex);
		printf("Server connection from:  %s\r\n", tcp.ipClient);
		pthread_mutex_unlock(&console.mutex);
		tcp.connection = 1;

		while (tcp.connection != CONNECTION_LOST)
		{
			if ((tcp.n = read(tcp.newfd, tcp.buff, sizeof(tcp.buff))) == -1)
			{
				console_print("Error leyendo mensaje en socket\r\n");
				pthread_exit(NULL);
			}
			if (tcp.n == 0)
			{
				tcp.connection = CONNECTION_LOST;
				close(tcp.newfd);
				console_print("Connection lost, waiting for new\r\n");
			}

			if (tcp.n >= PAYLOAD_LENGTH)
			{
				tcp.buff[tcp.n] = END_STRING;
				line_A_state = tcp.buff[LINE_A_STATE_POSITION];
				line_B_state = tcp.buff[LINE_B_STATE_POSITION];
				line_C_state = tcp.buff[LINE_C_STATE_POSITION];
				line_D_state = tcp.buff[LINE_D_STATE_POSITION];

				pthread_mutex_lock(&console.mutex);
				printf("Interace service: %s", tcp.buff);
				pthread_mutex_unlock(&console.mutex);

				pthread_mutex_lock(&usb.mutex);
				sprintf(usb.buffer, ">OUTS:%c,%c,%c,%c\r\n", line_A_state, line_B_state, line_C_state, line_D_state);
				serial_send(usb.buffer, sizeof(usb.buffer));
				pthread_mutex_unlock(&usb.mutex);

				pthread_mutex_lock(&console.mutex);
				printf("USB-TX: %s", usb.buffer);
				pthread_mutex_unlock(&console.mutex);
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