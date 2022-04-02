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


 int save_mail(char * buffer, user_list_t recievers) {

    // write to temp file
    char fileName[7];
    int fd = mkstemp(fileName);
    if (write(fd, buffer, strlen(buffer)) == -1) {
        return -1;
    } 

    save_user_mail(fileName, recievers);
    destroy_user_list(recievers);
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

    user_list_t reverse_users_list = NULL;
    user_list_t forward_users_list = NULL;
    char * mail_data_buffer = NULL;
    int data_mode = 0;

    while (1)
    {
        int readlineVal;
        readlineVal = nb_read_line(nb, recvbuf);

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

        if (data_mode) {
            if (splitCount == 1 && strcasecmp(parts[0], ".") == 0) {
                // end of data command
                data_mode = 0;
                send_formatted(fd, "250 Message accepted for delivery.\r\n");
                continue;
            }
            for (int i = 0; i < splitCount; i++) {
                // expand and copy string
                char *str = malloc(strlen(mail_data_buffer) + strlen(parts[i]) + 1);
                strcpy(str, mail_data_buffer);
                strcat(str, parts[i]);
                mail_data_buffer = str;
            }
        } else if (strcasecmp("NOOP", command) == 0) {
            // ignore extra params, still good
            send_formatted(fd, "250 OK\r\n");
        } else if (strcasecmp("QUIT", command) == 0) {
            send_formatted(fd, "221 OK %s\r\n", my_uname.nodename);
            // save_mail(mail_data_buffer, ) TODO
            return;
        } else if (strcasecmp("HELO", command) == 0) {
            send_formatted(fd, "250 %s\r\n", my_uname.nodename);
        } else if (strcasecmp("EHLO", command) == 0) {
            send_formatted(fd, "250 %s\r\n", my_uname.nodename);
        } else if (strcasecmp("VRFY", command) == 0) {
            // assume only need to check the user name part.
            if (is_valid_user(parts[1], NULL)) {
                send_formatted(fd, "250 %s\r\n", parts[2]);
            } else {
                send_formatted(fd, "550 %s\r\n", "user name does not exist");
            }
        } else if (strcasecmp("MAIL", command) == 0) {
            // This command clears the reverse-path buffer, the forward-path buffer,
            // and the mail data buffer, and it inserts the reverse-path information
            // from its argument clause into the reverse-path buffer.

            // format: "MAIL FROM:<reverse-path> [SP <mail-parameters> ] <CRLF>"
            
            // free the paths.
            if (reverse_users_list)
                destroy_user_list(reverse_users_list);
            reverse_users_list = NULL;
            if (forward_users_list)
                destroy_user_list(forward_users_list);
            forward_users_list = NULL;
            if (mail_data_buffer)
                free(mail_data_buffer);
            mail_data_buffer = NULL;


            int str_len = strlen(parts[1]);
            char * str = malloc(str_len + 1);
            // 6 is length of "FROM:<"
            strncpy(str, &parts[1] + 6, str_len - 6 - 1);
            add_user_to_list(reverse_users_list, str);
            send_formatted(fd, "250 OK\r\n");
        } else if (strcasecmp("RCPT", command) == 0) {
            // RCPT TO:<forward-path> [ SP <rcpt-parameters> ] <CRLF>
            if (reverse_users_list == NULL) {
                send_formatted(fd, "503 Bad sequence of commands\r\n");

            } else {
                add_user_to_list(forward_users_list, parts[1]);
                send_formatted(fd, "250 OK\r\n");
            }
        } else if (strcasecmp("DATA", command) == 0) {
            data_mode = 1;
            send_formatted(fd, "354 Enter mail, end with '.' on a line by itself.\r\n");
        } else if (strcasecmp("RSET", command) == 0) {
            // free the paths
            if (reverse_users_list)
                destroy_user_list(reverse_users_list);
            reverse_users_list = NULL;
            if (forward_users_list)
                destroy_user_list(forward_users_list);
            forward_users_list = NULL;
            if (mail_data_buffer)
                free(mail_data_buffer);
            mail_data_buffer = NULL;

            send_formatted(fd, "250 OK\r\n");
        } else if (strcasecmp("EXPN", command == 0) || strcasecmp("HELP", command == 0)) {
            send_formatted(fd, "502 Unsupported command.\r\n");
            
        } else {
            send_formatted(fd, "500 Invalid command.\r\n");
        }
    }

    nb_destroy(nb);
}
