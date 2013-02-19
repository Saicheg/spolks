#define _BSD_SOURCE

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <argtable2.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/ip_icmp.h>
#include <netdb.h>

#include "pinger.h"

#define PROGRAM_NAME "pinger"

int main_pinger(const char* source_address, const char* destination_host_name, int ttl, int listen);
void main_catcher();

int pid;
int catcher_listen_all;

int main(int argc, char ** argv) {
    struct arg_str *argument_address_source = arg_str0("s",
                                                       "source",
                                                       "<source-address>",
                                                       "Source address, specified in IP header of echo request");
    struct arg_int *argument_ttl = arg_int0("t",
                                            "ttl",
                                            "<1-255>",
                                            "TTL for the echo request packet");
    struct arg_lit *argument_listen = arg_lit0("l",
                                               "listen",
                                               "Listen for all echo replies"),
        *argument_help = arg_lit0("h",
                                  "help",
                                  "Show help");
    struct arg_str *argument_address_destination = arg_str1(NULL,
                                                            NULL,
                                                            "<destination-address>",
                                                            "Address to ping.");
    struct arg_end *argument_end = arg_end(20);


    void* arguments_table[] = {
                               argument_listen,
                               argument_address_source,
                               argument_ttl,
                               argument_address_destination,
                               argument_help,
                               argument_end
    };

    if (arg_nullcheck(arguments_table) != 0) {
        printf("%s: insufficient memory\n", PROGRAM_NAME);
        exit(PINGER_EXITERROR_PARAMETERS);
    }

    argument_ttl->ival[0] = 255;
    argument_address_source->sval[0] = NULL;

    int parse_errors_count = arg_parse(argc, argv, arguments_table);

    if (argc == 1) {
        printf("Usage: %s", PROGRAM_NAME);
        arg_print_syntax(stdout, arguments_table, "\n");
        printf("Try using %s --help for more information.\n", PROGRAM_NAME);
        exit(EXIT_SUCCESS);
    }

    if (argument_help->count > 0) {
        printf("Usage: %s", PROGRAM_NAME);
        arg_print_syntax(stdout, arguments_table, "\n");
        arg_print_glossary_gnu(stdout, arguments_table);
        exit(EXIT_SUCCESS);
    }

    if (parse_errors_count > 0) {
        arg_print_errors(stderr, argument_end, PROGRAM_NAME);
        fprintf(stderr, "Try using --help option for more information on calling arguments.\n");
        exit(PINGER_EXITERROR_PARAMETERS);
    }

    int code = main_pinger(argument_address_source->sval[0],
                           argument_address_destination->sval[0],
                           argument_ttl->ival[0],
                           argument_listen->count);
    arg_freetable(arguments_table, sizeof(arguments_table) / sizeof(arguments_table[0]));
    return code;
}

int main_pinger(const char* source_address_string,
                const char* destination_address_string,
                int ttl,
                int listen) {
    if (ttl < 1) {
        ttl = 1;
    }
    if (ttl > 255) {
        ttl = 255;
    }

    printf("Arguments:\n");
    printf("  source: %s\n", source_address_string);
    printf("  destination: %s\n", destination_address_string);
    printf("  TTL: %d\n", ttl);
    printf("  listen: %s\n", listen > 0 ? "yes" : "no");

    pid = getpid();
    catcher_listen_all = listen;

    struct sigaction catcher_action;
    memset(&catcher_action, 0, sizeof(catcher_action));
    catcher_action.sa_handler = &main_catcher;
    sigaction(SIGALRM, &catcher_action, NULL);
    sigaction(SIGINT, &catcher_action, NULL);

    int sd = socket(PF_INET, SOCK_RAW, IPPROTO_ICMP);
    const int on = 1;
    setsockopt(sd, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on));
    int size = 60 * 1024;
    setsockopt(sd, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size));

    setuid(getuid());

    struct itimerval timer;
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = 1;
    timer.it_interval.tv_sec = 1;
    timer.it_interval.tv_usec = 0;
    setitimer(ITIMER_REAL, &timer, NULL);

    struct addrinfo *destination_addrinfo;
    int error_code;
    error_code = getaddrinfo(destination_address_string, NULL, NULL, &destination_addrinfo);
    if (error_code != 0) {

        printf("Destination address was not resolved: %s.\n", gai_strerror(error_code));
        return PINGER_EXITERROR_DESTINATION_ADDRESS;
    }
    struct sockaddr_in destination_address;
    memset(&destination_address, 0, sizeof(destination_address));
    destination_address.sin_family = AF_INET;
    destination_address.sin_addr = ((struct sockaddr_in*) destination_addrinfo->ai_addr)->sin_addr;
    freeaddrinfo(destination_addrinfo);

    struct sockaddr_in source_address;
    memset(&source_address, 0, sizeof(source_address));
    source_address.sin_family = AF_INET;
    if (source_address_string != NULL) {
        struct addrinfo *source_addrinfo;
        error_code = getaddrinfo(source_address_string, NULL, NULL, &source_addrinfo);
        if (error_code != 0) {

            printf("Source address was not resolved: %s.\n", gai_strerror(error_code));
            return PINGER_EXITERROR_SOURCE_ADDRESS;
        }
        source_address.sin_addr = ((struct sockaddr_in*) source_addrinfo->ai_addr)->sin_addr;
        freeaddrinfo(source_addrinfo);
    } else {
        source_address.sin_addr.s_addr = INADDR_ANY;
    }
    
    // inet_ntoa uses ONE string buffer
    printf("Pinging (%s)", inet_ntoa(destination_address.sin_addr));
    printf(" from (%s).\n", inet_ntoa(source_address.sin_addr));

    //TODO:Everything else

    return 0;
}

void main_catcher() {
    //TODO
}