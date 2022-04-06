#include "netbuffer.h"
#include "mailuser.h"
#include "server.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>

#include <sys/utsname.h>

#define MAX_LINE_LENGTH 1024

// global variables
int state; // start: not authorized (0), 1 = authorized, 2 = transaction state
int userentered; // 0 = no username entered, 1 = valid username entered
char username[MAX_USERNAME_SIZE];
char password[MAX_PASSWORD_SIZE];
mail_list_t maillist;
int mailcount;
int mailsize;

static void handle_client(int fd);

int main(int argc, char *argv[]) {
  
    if (argc != 2) {
	fprintf(stderr, "Invalid arguments. Expected: %s <port>\n", argv[0]);
	return 1;
    }
    run_server(argv[1], handle_client);
  
    return 0;
}

// helper functions
// sends greeting
// params: file descriptor, utsname
void greeting(int fd, struct utsname name) {
    uname(&name);
    char *welcome_msg = "+OK POP3 server ready\r\n";
    send_formatted(fd, welcome_msg, name.nodename);
    state = 1;
}

// sends when command isn't valid for state
void send_invalid(int fd) {
    send_formatted(fd, "-ERR invalid command\r\n");
}

// sends when incorrect number of arguments in command
void send_invalidnumargs(int fd) {
    send_formatted(fd, "-ERR invalid number of arguments\r\n");
}

// sends when message number is too big (mail item's index is outside of range of indices)
void send_nomssg(int fd, int mailcount) {
    send_formatted(fd, "-ERR no such message, only %d messages in maildrop\r\n", mailcount);
}

// sends when message number is too big (smallest possible mail item index is 1)
void send_invalidmssgnum(int fd) {
    send_formatted(fd, "-ERR invalid message number\r\n");
}

// destroys mail
void destroymail(mail_list_t mail) {
    reset_mail_list_deleted_flag(mail);
    destroy_mail_list(mail);
}

// command = QUIT
// params: current state, mail list, name
void handlequit(int state, mail_list_t mail, int fd, struct utsname name) {
    if (state == 2) { // if in transaction state, destroy mail before signing off
        destroy_mail_list(mail);
        }
        char *msg = "+OK POP3 server signing off\r\n";
        send_formatted(fd, msg, name.nodename);
}

// command = USER username case
// params: num of parts in command, file descriptor, first argument of command
void handleuser(int splitCount, int fd, char* argument) {
    if (splitCount != 2) { // incorrect number of arguments
        send_invalidnumargs(fd);
    } else if (is_valid_user(argument, NULL) == 0) { // username doesnt exist
        send_formatted(fd, "-ERR never heard of mailbox\r\n");
    } else { // username exists
        strcpy(username, argument);
        userentered = 1; // username successfully entered, set flag as 1.
        send_formatted(fd, "+OK %s is a valid mailbox, enter password\r\n", argument);
    }
}

// command = PASSWORD case
// params: num of parts in command, file descriptor, first argument of command
void handlepass(int splitCount, int fd, char* argument) {
    if (userentered == 0 || splitCount != 2) { // did not enter username, or invalid number of arguments
        send_formatted(fd, "-ERR missing login information\r\n");
        userentered = 0; // reset userentered flag to 0, now need to resend USER command
    } 
    else if (is_valid_user(username, argument) == 0) { // username doesnt match password
        send_formatted(fd, "-ERR invalid password, enter username again\r\n");
        username[0] = '\0'; // delete data in username string because it's incorrect
        userentered = 0; // reset userentered flag to 0, now need to resend USER command
    } 
    else { //username matches password
        send_formatted(fd, "+OK maildrop locked and ready \r\n");
        maillist = load_user_mail(username);
        strcpy(password, argument); // save password into password string. actually might not be needed
        userentered = 0; // reset userentered flag to 0
        state = 2; // set transaction state
    }
}

// command: STAT case
// params: file descriptor
void handlestat(int fd) {
    mailcount = get_mail_count(maillist, 0);
    mailsize = get_mail_list_size(maillist);
    send_formatted(fd, "+OK %d %d\r\n", mailcount, mailsize);
}

// command: LIST (no arguments)
// params: file descriptor, includedelete flag that indicates if our mail list should include deleted items (should be set as 1)
void handlelistnoarg(int fd, int includedelete) {
    send_formatted(fd, "+OK %d messages (%d octets)\r\n", mailcount, mailsize); // return total number of messages and total size of all messages
    int i = 0;
    while (i < includedelete) { // return each message number and its size on new line
        mail_item_t mail = get_mail_item(maillist, i);
        if (mail != NULL) { // if mail item exists
            int size = get_mail_item_size(mail);
            send_formatted(fd, "%d %d\r\n", i + 1, size); // send mail item's messagenum and size
        }
        i++;
    }
    send_formatted(fd, ".\r\n"); // terminate with dot on separate line
}

// command: LIST (with arg)
// params: file descriptor, first argument of command
void handlelistarg(int fd, char * argument) {
    int messagenumber = atoi(argument); // messagenumber is index of the mail item
    if (messagenumber == 0) { // check that message's number is not 0
        send_invalidmssgnum(fd);
        return;
    }
    mail_item_t mail = get_mail_item(maillist, messagenumber - 1);
    if (mail == NULL) { // if message does not exist (e.g. message's number is outside range of existing messages)
        send_nomssg(fd,mailcount);
    } else {
        int size = get_mail_item_size(mail); // message exists
        send_formatted(fd, "+OK %d %d\r\n", messagenumber, size);
    }
}

// command: LIST (all cases: with arg or no arg)
// params: number of parts in command, file descriptor, first argument of command
void handlelist(int splitCount, int fd, char * argument) {
    mailcount = get_mail_count(maillist, 0);
    mailsize = get_mail_list_size(maillist);
    int includedelete = get_mail_count(maillist, 1);
    if (splitCount == 1) { // if no arguments:
        handlelistnoarg(fd, includedelete);
    } 
    else if (splitCount == 2) { // if has argument:
        handlelistarg(fd, argument);
    }
    else {
        send_invalidnumargs(fd);
    }
}

// command: RSET
// params: file descriptor
void handlerset(int fd) {
    int precount = get_mail_count(maillist, 0);
    reset_mail_list_deleted_flag(maillist);
    int postcount = get_mail_count(maillist, 0);
    int difference = postcount - precount;
    send_formatted(fd, "+OK %d messages restored\r\n", difference);
}


// command: RETR
// params: number of parts in command, file descriptor, value of first argument in command
void handleretr(int splitCount, int fd, char * argument) {
    if (splitCount != 2) { // if wrong number of args, send invalid num args error
        send_invalidnumargs(fd);
    } 
    else {
        int messagenumber = atoi(argument);
        mail_item_t mail = get_mail_item(maillist, messagenumber - 1);
        mailcount = get_mail_count(maillist, 0);
        if (messagenumber == 0) { // if message number is 0, send invalid message number error
            send_invalidmssgnum(fd);
        }
        else if (mail == NULL) { // if message is NULL, send no message error
            send_nomssg(fd, mailcount);
        } 
        else { // if message is exists
            int size = get_mail_item_size(mail);
            send_formatted(fd, "+OK %d octets\r\n", size); // send size of message
            FILE* file = get_mail_item_contents(mail);
            char buffer[MAX_LINE_LENGTH + 1];
            while (fgets (buffer, MAX_LINE_LENGTH + 1, file) != NULL) {
                send_formatted(fd, "%s", buffer); // send message
            }
            send_formatted(fd, ".\r\n"); // end message with . on own line
            fclose(file);
        }
    }
}

// command: DELE
// params: number of parts in command, file descriptor, value of first argument in command
void handledele(int splitCount, int fd, char * argument) {
    if (splitCount != 2) { // if wrong number of arguments, send invalid number of arguments error
        send_invalidnumargs(fd);
    } else {
        int messagenumber = atoi(argument);
        if (messagenumber == 0) { // if message number is 0, send invalid message number error
            send_invalidmssgnum(fd);
            return;
        }
        mailcount = get_mail_count(maillist, 1);
        if (messagenumber > mailcount) { // if messagenumber is bigger than total number of messages, reutrn no such message error
            send_nomssg(fd, mailcount);
        } else { // if message exists
            mail_item_t mail = get_mail_item(maillist, messagenumber - 1); // get the message
            if (mail == NULL) { // if message is already deleted
                send_formatted(fd, "-ERR message %d already deleted\r\n", messagenumber);
            } else { /// if message is not already deleted
                mark_mail_item_deleted(mail);
                send_formatted(fd, "+OK message %d deleted\r\n", messagenumber);
            }
        }
    }
}

void handle_client(int fd) {
  
    char recvbuf[MAX_LINE_LENGTH + 1];
    net_buffer_t nb = nb_create(fd, MAX_LINE_LENGTH);
  
    /* TO BE COMPLETED BY THE STUDENT */
    struct utsname my_uname;
    uname(&my_uname);
    state = 0; // start: not authorized (0), 1 = authorized, 2 = transaction state
    userentered = 0; // 0 = no username entered, 1 = valid username entered
    maillist = NULL;

    greeting(fd, my_uname); // send greeting

    while (1) {
        int readlineVal;
        readlineVal = nb_read_line(nb, recvbuf);
        
        dlog("readlineVal: %i, %s", readlineVal, recvbuf);

        // if readlineVal is -1, error
        if (readlineVal == -1) {
            char *msg = "-ERR\r\n";
            send_formatted(fd, msg, my_uname.nodename);
            if (maillist != NULL) {
                destroymail(maillist);
            }
            break;
        }

        // if readlineVal is 0, we're done
        if (readlineVal == 0) {
            char *msg = "+OK\r\n";
            send_formatted(fd, msg, my_uname.nodename);
            if (maillist != NULL) {
                destroymail(maillist);
            }
            break;
        }

        char *parts[(MAX_LINE_LENGTH + 1) / 2];

        int splitCount = split(recvbuf, parts); // splitCount = number of parts in command, including command itself
        dlog("%i\n", splitCount * 1);
        
        if (splitCount <= 0) {
            continue;
        }

        char * command = parts[0]; // parts[] contains all parts of command, with command itself being parts[0]
        dlog("%s\n", command);
        
        if (strcasecmp("QUIT", command) == 0) { // can be given in any state
            handlequit(state, maillist, fd, my_uname);
            break;
        }

        if (strcasecmp("NOOP", command) == 0) { // can be given in any state
            send_formatted(fd, "+OK\r\n");
            continue;
        }

        if (state == 1) { // two options in authorization (1) state: user and pass
            if (strcasecmp("USER", command) == 0) {
                handleuser(splitCount, fd, parts[1]);
            }
            else if (strcasecmp("PASS", command) == 0) {
                handlepass(splitCount,fd,parts[1]);
            }
            else {
                send_invalid(fd);
            }
            continue;
        }

        if (state == 2) {
            if (strcasecmp("STAT", command) == 0) {
                handlestat(fd);
            }
            else if (strcasecmp("LIST", command) == 0) {
                handlelist(splitCount, fd, parts[1]);
            }
            else if (strcasecmp("RETR", command) == 0) {
                handleretr(splitCount, fd, parts[1]);
            }
            else if (strcasecmp("DELE", command) == 0) {
                handledele(splitCount, fd, parts[1]);
            }
            else if (strcasecmp("RSET", command) == 0) {
                handlerset(fd);
            }
            else {
                send_invalid(fd);
            }
        }
    }
    nb_destroy(nb);
}