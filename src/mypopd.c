#include "netbuffer.h"
#include "mailuser.h"
#include "server.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>

#include <sys/utsname.h>

#define MAX_LINE_LENGTH 1024

static void handle_client(int fd);
int authstate;
int transstate;

int main(int argc, char *argv[]) {
  
    if (argc != 2) {
	fprintf(stderr, "Invalid arguments. Expected: %s <port>\n", argv[0]);
	return 1;
    }
    run_server(argv[1], handle_client);
  
    return 0;
}

// send greeting
void greeting(int fd, struct utsname name) {
    uname(&name);
    char *welcome_msg = "+OK %s POP3 server ready\r\n";
    send_formatted(fd, welcome_msg, name.nodename);
    authstate = 1;
}

void handle_client(int fd) {
  
    char recvbuf[MAX_LINE_LENGTH + 1];
    net_buffer_t nb = nb_create(fd, MAX_LINE_LENGTH);
  
    /* TO BE COMPLETED BY THE STUDENT */
    struct utsname my_uname;
    uname(&my_uname);

    char username[MAX_USERNAME_SIZE];
    char password[MAX_PASSWORD_SIZE];
    authstate = 0; // start: not authorized
    transstate = 0; // start: not in transaction state

    greeting(fd, my_uname); // send greeting

    while (nb_read_line(nb, recvbuf)) { // command reading and parsing
        char *parts[(MAX_LINE_LENGTH + 1) / 2];
        if (split(recvbuf, parts) <= 0) {
            continue;
        }

        char * command = parts[0];
        dlog("%s\n", command);

        if (strcasecmp("USER", command) == 0) { // USER
        // do something
            }

        if (strcasecmp("QUIT", command) == 0) {
            // load_user_mail(user); <- only do this in auth state
            // destroy_mail_list(mail_list);
            char * msg = "+OK %s POP3 server signing off \r\n";
            send_formatted(fd, msg);
        }
    }
    nb_destroy(nb);
}