#include <stdio.h>
#include <stdlib.h>
#include "SerialManager.h"
#include <string.h>
#include <unistd.h>

#define msToUs (x)

#define BUFFER_LENGTH 20
#define MILLISECONDS_100 (100 * 1000U)

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
	char echo_terminal[BUFFER_LENGTH];
	int bytes_length = 0;

	printf("Inicio Serial Service\r\n");
	mainPort.file = serial_open(mainPort.number, mainPort.baudrate);
	serial_send(mainPort.buffer, strlen(mainPort.buffer));

	while (1)
	{
		bytes_length = serial_receive(mainPort.buffer, BUFFER_LENGTH);
		usleep(MILLISECONDS_100);

		strncpy(echo_terminal, mainPort.buffer, bytes_length);
		echo_terminal[bytes_length] = '\0';

		if (bytes_length > 0)
		{
			printf("RX: %s", echo_terminal);
		}
	}

	exit(EXIT_SUCCESS);
	return 0;
}
