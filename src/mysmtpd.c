#include "netbuffer.h"
#include "mailuser.h"
#include "server.h"
#include <sys/socket.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <ctype.h>
#include <strings.h>

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

// return 1 if email exists, else return 0
int isEmailInFile(char* fileName, char * userName) {
    FILE* fp;
    char * line = NULL;
    size_t len = 0;
    ssize_t read;
    char * parts[2];
    int userNameFound = 0;
    // open users file
    fp = fopen(fileName, "r");
    if (fp == NULL)
        exit(EXIT_FAILURE);

    while ((read = getline(&line, &len, fp)) != -1) {
        // assume user.txt always well formed
        split(line, parts);
        // @ added to ensure nothing funky happens at the end
        if (strncasecmp(parts[0], userName + '@', strlen(userName) + 1) == 0) {
            userNameFound = 1;
            break;
        }
    }

    fclose(fp);
    if (line)
        free(line);
    exit(EXIT_SUCCESS);

    return userNameFound;
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

    while (1)
    {
        int readlineVal = nb_read_line(nb, recvbuf);

        //  -1 is error, 0 is we are done
        if (readlineVal == 0 || readlineVal == -1) {
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


        if (strcasecmp("NOOP", command) == 0) {
            // ignore extra params, still good
            send_formatted(fd, "250 OK\r\n");
        } else if (strcasecmp("QUIT", command) == 0) {
            send_formatted(fd, "221 OK\r\n");
            return;
        } else if (strcasecmp("HELO", command) == 0) {
            send_formatted(fd, "250 %s\r\n", my_uname.nodename);
        } else if (strcasecmp("EHLO", command) == 0) {
            send_formatted(fd, "250 %s\r\n", my_uname.nodename);
        } else if (strcasecmp("VRFY", command) == 0) {
            // assume only need to check the user name part.
            if (isEmailInFile("users.txt", parts[1])) {
                send_formatted(fd, "250 %s\r\n", parts[2]);
            } else {
                send_formatted(fd, "550 %s\r\n", "user name does not exist");
            }
        } else {
            send_formatted(fd, "504 Command not recognized.\r\n");
        }
    }

    nb_destroy(nb);
}

