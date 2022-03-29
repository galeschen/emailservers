#include "netbuffer.h"
#include "mailuser.h"
#include "server.h"
#include <sys/socket.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <ctype.h>

#define MAX_LINE_LENGTH 1024

static void handle_client(int fd);

int main(int argc, char *argv[])
{

    if (argc != 2)
    {
        fprintf(stderr, "Invalid arguments. Expected: %s <port>\n", argv[0]);
        return 1;
    }

    run_server(argv[1], handle_client);

    return 0;
}

// Have your program send the initial welcome message and immediately return.
void greeting(int fd, struct utsname name)
{
    int error = uname(&name); // returns system information in the structure pointed to by buf
    if (error == -1)
    {
        return;
    }
    else
    {
        char *msg = "220 %s simple mail transfer protocol ready\r\n";
        int len, bytes_sent;
        len = strlen(msg);
        bytes_sent = send_formatted(fd, msg, name.__domainname);

        if (bytes_sent == -1)
        {
            return;
        }
        else
        {
            //   char *msg = "220 %i simple mail transfer protocol ready\r\n";
            //   int len = strlen(msg);
            //   bytes_sent = send_formatted(fd, msg, len);
            //   if (bytes_sent == - 1) {
            //   return;
            //   }
        }
    }
}

void handle_client(int fd)
{

    char recvbuf[MAX_LINE_LENGTH + 1];
    net_buffer_t nb = nb_create(fd, MAX_LINE_LENGTH);

    struct utsname my_uname;
    uname(&my_uname);

    /* TO BE COMPLETED BY THE STUDENT */
    greeting(fd, my_uname);

    nb_destroy(nb);
}
