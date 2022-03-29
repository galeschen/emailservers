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

void handle_client(int fd)
{

    char recvbuf[MAX_LINE_LENGTH + 1];
    net_buffer_t nb = nb_create(fd, MAX_LINE_LENGTH);

    struct utsname my_uname;
    uname(&my_uname);

    /* TO BE COMPLETED BY THE STUDENT */
    // welcome message
    static char *msg = "220 %s simple mail transfer protocol ready\r\n";
    send_formatted(fd, msg, my_uname.nodename);

    while (nb_read_line(nb, recvbuf))
    {
        char *parts[(MAX_LINE_LENGTH + 1) / 2];
        // todo check if nothing in there
        split(recvbuf, parts);
        char * command = parts[0];
        if (strcasecmp("NOOP", command)) {
            // ignore extra params, still good
            send_formatted(fd, "250 OK\r\n");
        } else if strcasecmp("QUIT", command) {
            send_formatted(fd, "221 OK\r\n");
            return;
        }
    }

    nb_destroy(nb);
}
