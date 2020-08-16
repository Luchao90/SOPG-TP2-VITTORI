#include <stdio.h>
#include <stdlib.h>
#include "SerialManager.h"
#include <string.h>
#include <unistd.h>

#define msToUs (x)

#define BUFFER_LENGTH 20
#define MILLISECONDS_500 (500 * 1000U)

typedef struct
{
	int32_t file;
	char buffer[BUFFER_LENGTH];
	int number;
	int baudrate;
} SerialPort;

int main(void)
{
	SerialPort mainPort = {.file = 0, .buffer = ">OUTS:2,2,2,2\r\n", .number = 1, .baudrate = 115200};
	int bytes_length;

	printf("Inicio Serial Service\r\n");
	mainPort.file = serial_open(mainPort.number, mainPort.baudrate);

	while (1)
	{
		/* bytes_length = serial_receive(mainPort.buffer, strlen(mainPort.buffer));
		usleep(MILLISECONDS_500); */
		strcpy(mainPort.buffer, ">OUTS:1,0,1,0\r\n");
		serial_send(mainPort.buffer, strlen(mainPort.buffer));
		sleep(1);
		strcpy(mainPort.buffer, ">OUTS:0,1,0,1\r\n");
		serial_send(mainPort.buffer, strlen(mainPort.buffer));
		sleep(1);
	}

	exit(EXIT_SUCCESS);
	return 0;
}
