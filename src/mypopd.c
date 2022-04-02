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
int state; // 0 = initial, 1 = authorized, 2 = transaction, 3 = update
int userentered; // 0 = not entered, 1 = entered
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
    char *welcome_msg = "+OK POP3 server ready\r\n";
    send_formatted(fd, welcome_msg, name.nodename);
    state = 1;
}

void destroymail(mail_list_t mail) {
    reset_mail_list_deleted_flag(mail);
    destroy_mail_list(mail);
}

void handlequit(int state, mail_list_t mail, int fd, struct utsname name) {
    if (state == 2) {
        destroy_mail_list(mail);
        }
        char *msg = "+OK POP3 server signing off\r\n";
        send_formatted(fd, msg, name.nodename);
}

void handle_client(int fd) {
  
    char recvbuf[MAX_LINE_LENGTH + 1];
    net_buffer_t nb = nb_create(fd, MAX_LINE_LENGTH);
  
    /* TO BE COMPLETED BY THE STUDENT */
    struct utsname my_uname;
    uname(&my_uname);
    state = 0; // start: not authorized
    userentered = 0; // start: no username entered
    // int mailcount = 0;
    char *username = NULL;
    char *password = NULL;
    mail_list_t mail = NULL;

    greeting(fd, my_uname); // send greeting

    while (1) {
        int readlineVal;
        readlineVal = nb_read_line(nb, recvbuf);

        //  -1 is error, 0 is we are done
        if (readlineVal == -1) {
            char *msg = "-ERR\r\n";
            send_formatted(fd, msg, my_uname.nodename);
            if (mail != NULL) {
                destroymail(mail);
            }
            break;
        }
        if (readlineVal == 0) {
            char *msg = "+OK\r\n";
            send_formatted(fd, msg, my_uname.nodename);
            if (mail != NULL) {
                destroymail(mail);
            }
            break;
        }

        char *parts[(MAX_LINE_LENGTH + 1) / 2];

        int splitCount = split(recvbuf, parts);
        dlog("%i\n", splitCount * 1);
        
        if (splitCount <= 0) {
            continue;
        }

        char * command = parts[0];
        dlog("%s\n", command);
        
        if (strcasecmp("QUIT", command) == 0) { // can be given in any state
            handlequit(state, mail, fd, my_uname);
            break;
        }

        if (strcasecmp("NOOP", command) == 0) {
            send_formatted(fd, "+OK\r\n");
        }

        if (state == 1) { // two options in authorization (1) state: user and pass
            if (strcasecmp("USER", command) == 0) {
                if (is_valid_user(parts[1], NULL) != 0 ) { // username exists
                    strcpy(username, parts[1]);
                    send_formatted(fd, "+OK %s is a valid mailbox, enter password\r\n", parts[1]);
                    parts[1] = '\0';
                    userentered = 1;
                } else {
                    send_formatted(fd, "-ERR never heard of mailbox\r\n");
                }
            }

            if (strcasecmp("PASS", command) == 0) {
                if (userentered == 0) {
                    send_formatted(fd, "-ERR no username provided\r\n");
                }
                if (parts[1] == NULL) {
                    send_formatted(fd, "-ERR invalid password, enter username again\r\n");
                    parts[1] = '\0';
                    username[0] = '\0';
                    userentered = 0;
                }
                if (is_valid_user(username, parts[1]) == 0) {
                    send_formatted(fd, "+OK matching username and password \r\n");
                }
                if (is_valid_user(username, parts[1]) != 0) { //username matches password
                    mail = load_user_mail(username);
                    send_formatted(fd, "+OK matching username and password \r\n");
                    strcpy(password, parts[1]);
                    state = 2;
                }
            }
        }
    }
    nb_destroy(nb);
}