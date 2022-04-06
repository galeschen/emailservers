#include "netbuffer.h"
#include "mailuser.h"
#include "server.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>

#include <sys/utsname.h>

#define MAX_LINE_LENGTH 1024
int state;

static void handle_client(int fd);

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
    int userentered = 0; // start: no username entereded
    int mailcount;
    int mailsize;
    char username[MAX_USERNAME_SIZE];
    char password[MAX_PASSWORD_SIZE];
    mail_list_t maillist = NULL;

    greeting(fd, my_uname); // send greeting

    while (1) {
        int readlineVal;
        readlineVal = nb_read_line(nb, recvbuf);
        
        dlog("readlineVal: %i, %s", readlineVal, recvbuf);

        //  -1 is error, 0 is we are done
        if (readlineVal == -1) {
            char *msg = "-ERR\r\n";
            send_formatted(fd, msg, my_uname.nodename);
            if (maillist != NULL) {
                destroymail(maillist);
            }
            break;
        }
        if (readlineVal == 0) {
            char *msg = "+OK\r\n";
            send_formatted(fd, msg, my_uname.nodename);
            if (maillist != NULL) {
                destroymail(maillist);
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
            handlequit(state, maillist, fd, my_uname);
            break;
        }

        if (strcasecmp("NOOP", command) == 0) { // can be given in any state
            send_formatted(fd, "+OK\r\n");
        }

        if (state == 1) { // two options in authorization (1) state: user and pass
            if (strcasecmp("USER", command) == 0) {
                if (splitCount != 2) { // incorrect number of arguments
                    send_formatted(fd, "-ERR incorrect number of arguments\r\n");
                }
                else if (is_valid_user(parts[1], NULL) == 0) { // username doesnt exist
                    send_formatted(fd, "-ERR never heard of mailbox\r\n");
                } else { // username exists
                        strcpy(username, parts[1]);
                        userentered = 1;
                        send_formatted(fd, "+OK %s is a valid mailbox, enter password\r\n", parts[1]);
                    }
                }

            if (strcasecmp("PASS", command) == 0) {
                if (userentered == 0 || splitCount != 2) { // did not do username
                    send_formatted(fd, "-ERR missing login information\r\n");
                    userentered = 0;
                } else if (is_valid_user(username, parts[1]) == 0) { // username doesnt match passwordPA
                    send_formatted(fd, "-ERR invalid password, enter username again\r\n");
                    username[0] = '\0';
                    userentered = 0;
                } else { //username matches password
                    send_formatted(fd, "+OK maildrop locked and ready \r\n");
                    maillist = load_user_mail(username);
                    strcpy(password, parts[1]);
                    userentered = 0;
                    state = 2; // transaction state
                }
            }
        }

        if (state == 2) {
            if (strcasecmp("STAT", command) == 0) {
                mailcount = get_mail_count(maillist, 0);
                mailsize = get_mail_list_size(maillist);
                send_formatted(fd, "+OK %d %d\r\n", mailcount, mailsize);
            }
            if (strcasecmp("LIST", command) == 0) {
                mailcount = get_mail_count(maillist, 0);
                mailsize = get_mail_list_size(maillist);
                if (splitCount == 1) {
                    send_formatted(fd, "+OK %d messages (%d octets)\r\n", mailcount, mailsize);
                } else if (splitCount == 2) {
                    int messagenumber;
                    if (parts[1] == "0") {
                        messagenumber = 0;
                    } else {
                        messagenumber = atoi(parts[1]);
                        if (messagenumber == 0) {
                            send_formatted(fd, "-ERR invalid argument type\r\n");
                            continue;
                        }
                    }
                    
                    mail_item_t mail = get_mail_item(maillist, messagenumber);
                    if (mail == NULL) {
                        send_formatted(fd, "-ERR no such message, only %d messages in maildrop\r\n", mailcount);
                    } else {
                        int size = get_mail_item_size(mail);
                        send_formatted(fd, "+OK %d %d\r\n", messagenumber, size);
                    }
                } else {
                    send_formatted(fd, "-ERR invalid number of arguments\r\n");
                }
            }
        }
    }
    nb_destroy(nb);
}