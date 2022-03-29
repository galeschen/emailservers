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
    char *welcome_msg = "220 %s simple mail transfer protocol ready\r\n";
    send_formatted(fd, welcome_msg, my_uname.nodename);

    while (nb_read_line(nb, recvbuf))
    {
        char *parts[(MAX_LINE_LENGTH + 1) / 2];

        int splitTimes = split(recvbuf, parts);
        if (splitTimes == 1) {
            continue;
        }

        char * command = parts[0];
        printf("%s\n", command);
        if (strcasecmp("NOOP", command) == 0) {
            // ignore extra params, still good
            send_formatted(fd, "250 OK\r\n");
        } else if (strcasecmp("QUIT", command) == 0) {
            char * msg = "221 OK\r\n";
            send_formatted(fd, msg);
            return;
        } else {
            send_formatted(fd, "500\r\n");
        }
    }

    nb_destroy(nb);
}
