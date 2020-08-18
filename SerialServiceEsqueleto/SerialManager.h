#include <pthread.h>
#define BUFFER_LENGTH 20

typedef struct
{
    pthread_mutex_t mutex;
    int32_t file;
    char buffer[BUFFER_LENGTH];
    int number;
    int baudrate;
    int bytesReaded;
} SerialPort;

int serial_open(int pn, int baudrate);
void serial_send(char *pData, int size);
void serial_close(void);
int serial_receive(char *buf, int size);
