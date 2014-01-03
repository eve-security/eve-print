#include "arduino_lock.h"

int fd = -1;
char* port = NULL;

void arduino_connect(char* Lport)
{
    port = Lport;
    fd = serialport_init(Lport, BAUDRATE);
    if (fd == -1)
    {
        fprintf(stderr,"Couldn't open port %s\n",port);
    }
    sleep(1);
}


void arduino_send_char(char c)
{
    int rc;
    rc = serialport_writebyte(fd, (uint8_t)c);

    if (rc == -1){
        fprintf(stderr,"Error writing to arduino\n");
        //TODO attempt reconnect
    }

    serialport_flush(fd);
}


//unlock the door
void arduino_unlock()
{
    arduino_send_char('g');
}


//blink the red LED
void arduino_blink()
{
    arduino_send_char('b');
}


void arduino_close()
{
    if (fd != -1) {
        serialport_close(fd);
        fd = -1;
    }
}

