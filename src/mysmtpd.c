#include "netbuffer.h"
#include "mailuser.h"
#include "server.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <ctype.h>

#define MAX_LINE_LENGTH 1024

static void handle_client(int fd);

int main(int argc, char *argv[]) {
  
    if (argc != 2) {
	fprintf(stderr, "Invalid arguments. Expected: %s <port>\n", argv[0]);
	return 1;
    }
  
    run_server(argv[1], handle_client);
  
    return 0;
}

void handle_client(int fd) {
  
    char recvbuf[MAX_LINE_LENGTH + 1];
    net_buffer_t nb = nb_create(fd, MAX_LINE_LENGTH);

    struct utsname my_uname;
    uname(&my_uname);
  
    /* TO BE COMPLETED BY THE STUDENT */
    // connection message
    send_formatted(fd, "220 Server is ready");
    
    // TODO extra parameters count as invalid syntax

    // // max amount of parts is half taken up by spaces
    // char parts[(MAX_LINE_LENGTH + 1) / 2];
    // char * out;

    // // read next line
    // nb_read_line(nb, &out)
    // // split line to tokens
    // split(out, &parts);
  
    nb_destroy(nb);
}
