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

void send_invalid(int fd) {
    send_formatted(fd, "501 Invalid command.\r\n");
}

void send_out_of_order(int fd) {
    send_formatted(fd, "503 Bad sequence of commands\r\n");
}

void append_to_buffer(char * buffer, char * new_str) {
    // expand and copy string
    char *str;
    if (buffer == NULL) {
        // if mail buffer not initialized
        str = malloc(strlen(new_str) + 1);
        strcpy(str, new_str);
    } else {
        str = malloc(strlen(buffer) + strlen(new_str) + 1);
        strcpy(str, buffer);
        strcat(str, new_str);
    }
    buffer = str;
}


void handle_client(int fd)
{

    char recvbuf[MAX_LINE_LENGTH + 1];
    net_buffer_t nb = nb_create(fd, MAX_LINE_LENGTH);

    struct utsname my_uname;
    uname(&my_uname);
    char * domain = my_uname.nodename;

    // welcome message
    char *welcome_msg = "220 %s simple mail transfer protocol ready\r\n";
    send_formatted(fd, welcome_msg, domain);

    user_list_t reverse_users_list = NULL;
    user_list_t forward_users_list = NULL;
    
    FILE * temp_file_stream = NULL;
    char file_name[] = "tempfile-XXXXXX";
    
    int data_mode = 0;
    int sent_helo = 0;

    while (1)
    {
        //  -1 is error, 0 is we are done
        int readlineVal = nb_read_line(nb, recvbuf);
        if (readlineVal == 0 || readlineVal == -1) {
            break;
        }

        if (data_mode) {
            if (strcasecmp(recvbuf, ".\r\n") == 0) {
                // end of data command
                data_mode = 0;
                fclose(temp_file_stream);

                // send the mail
                save_user_mail(file_name, forward_users_list);
                
                // delete file
                unlink(file_name);
                temp_file_stream = NULL;

                send_formatted(fd, "250 %s Message accepted for delivery.\r\n", domain);
            } else {
                if (fwrite(recvbuf, 1, readlineVal, temp_file_stream) != readlineVal) {
                    dlog("Could not append line to file");
                }
            }
            continue;
        }

        char * parts[(MAX_LINE_LENGTH + 1) / 2];

        int splitCount = split(recvbuf, parts);
        dlog("%i\n", splitCount * 1);
        
        char * command = parts[0];

        if (command == NULL) {
            continue;
        }

        if (strcasecmp("NOOP", command) == 0) {
            // ignore extra params, still good
            send_formatted(fd, "250 OK\r\n");
        } else if (strcasecmp("QUIT", command) == 0) {
            send_formatted(fd, "221 OK\r\n");
            break;
        } else if (strcasecmp("HELO", command) == 0 || strcasecmp("EHLO", command) == 0) {
            sent_helo = 1;
            send_formatted(fd, "250 %s\r\n", domain);
        } else if (strcasecmp("VRFY", command) == 0) {
            if (splitCount != 2) {
                send_invalid(fd);
            } else {
                char * username_and_domain = parts[1];
                if (is_valid_user(username_and_domain, NULL)) {
                    send_formatted(fd, "250 %s\r\n", username_and_domain);
                } else {
                    send_formatted(fd, "550 user name does not exist\r\n");
                }
            }
        } else if (strcasecmp("MAIL", command) == 0) {
            // This command clears the reverse-path buffer, the forward-path buffer,
            // and the mail data buffer, and it inserts the reverse-path information
            // from its argument clause into the reverse-path buffer.

            // format: "MAIL FROM:<reverse-path> [SP <mail-parameters> ] <CRLF>"
            // helo must be sent first
            if (sent_helo == 0) {
                send_out_of_order(fd);
                continue;
            }
            
            // check for 1 param only
            if (splitCount != 2) {
                send_invalid(fd);
                continue;
            }

            // free the paths.
            if (reverse_users_list)
                destroy_user_list(reverse_users_list);
            reverse_users_list = NULL;
            if (forward_users_list)
                destroy_user_list(forward_users_list);
            forward_users_list = NULL;

            // 6 is length of "FROM:<"
            int str_len = strlen(parts[1]);
            char * str = malloc(str_len + 1);
            strncpy(str, parts[1] + 6, str_len - 6 - 1);

            add_user_to_list(&reverse_users_list, str);
            dlog("recieve: %s\n", str);
            free(str);

            send_formatted(fd, "250 OK\r\n");

        } else if (strcasecmp("RCPT", command) == 0) {
            // RCPT TO:<forward-path> [ SP <rcpt-parameters> ] <CRLF>
            if (sent_helo == 0) {
                send_out_of_order(fd);
                continue;
            }

            if (splitCount < 2) {
                send_invalid(fd);
                continue;
            }

            if (reverse_users_list == NULL) {
                send_out_of_order(fd);
            } else {
                // 6 is length of "TO:<"
                int str_len = strlen(parts[1]);
                char * str = malloc(str_len + 1);
                strncpy(str, parts[1] + 4, str_len - 4 - 1);
                dlog("forward: %s\n", str);

                if (is_valid_user(str, NULL)) {
                    add_user_to_list(&forward_users_list, str);
                    send_formatted(fd, "250 OK\r\n");
                } else {
                    send_formatted(fd, "550 user name does not exist\r\n");
                }
                free(str);
            }
        } else if (strcasecmp("DATA", command) == 0) {

            if (reverse_users_list == NULL || forward_users_list == NULL) {
                send_out_of_order(fd);
                continue;
            }

            // setup new file
            data_mode = 1;
            strcpy(file_name, "tempfile-XXXXXX");
            int fd_temp = mkstemp(file_name);
            temp_file_stream = fdopen(fd_temp, "w");

            send_formatted(fd, "354 Enter mail, end with '.' on a line by itself.\r\n");
        } else if (strcasecmp("RSET", command) == 0) {
            
            if (splitCount != 1) {
                send_invalid(fd);
                continue;
            }

            // free the paths
            if (reverse_users_list)
                destroy_user_list(reverse_users_list);
            reverse_users_list = NULL;
            if (forward_users_list)
                destroy_user_list(forward_users_list);
            forward_users_list = NULL;

            send_formatted(fd, "250 OK %s\r\n", domain);
        } else if (strcasecmp("EXPN", command) == 0 || strcasecmp("HELP", command) == 0) {
            send_formatted(fd, "502 %s Unsupported command.\r\n", domain);
        } else {
            send_formatted(fd, "500 %s Invalid command.\r\n", domain);
        }
    }

    nb_destroy(nb);
    return;
}
