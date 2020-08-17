#include <stdio.h>
#include <stdlib.h>
#include "SerialManager.h"
#include <string.h>
#include <unistd.h>

#define msToUs(x) (x * 1000U)
#define END_STRING '\0'

int main(void)
{
	SerialPort mainPort = {.file = 0, .buffer = ">OUTS:2,2,2,2\r\n", .number = 1, .baudrate = 115200, .bytesReaded = 0};
	char echo_terminal[BUFFER_LENGTH];

	printf("Inicio Serial Service\r\n");
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

	exit(EXIT_SUCCESS);
	return 0;
}
