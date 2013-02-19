#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <netinet/ip_icmp.h>
#include <argtable2.h>

#define PROGRAM_NAME "pinger"

int main_pinger(const char* source_address, const char* destination_address, int ttl, int listen, int continuous);

int main(int argc, char ** argv) {
    struct arg_str *argument_source_ip = arg_str0("s",
                                                  "source-ip",
                                                  "<ip>",
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
    struct arg_str *argument_destination_address = arg_str1(NULL,
                                                            NULL,
                                                            "<destination-address>",
                                                            "Address to ping.");

    struct arg_lit *argument_continuous = arg_lit0("c",
                                                   "continuous",
                                                   "Send requests continuously.");

    struct arg_end *argument_end = arg_end(20);


    void* arguments_table[] = {
                               argument_listen,
                               argument_continuous,
                               argument_source_ip,
                               argument_ttl,
                               argument_destination_address,
                               argument_help,
                               argument_end
    };

    if (arg_nullcheck(arguments_table) != 0) {
        printf("%s: insufficient memory\n", PROGRAM_NAME);
        exit(1);
    }

    argument_ttl->ival[0] = 255;
    argument_source_ip->sval[0] = NULL;

    int parse_errors_count = arg_parse(argc, argv, arguments_table);

    if (argc == 1) {
        printf("Usage: %s", PROGRAM_NAME);
        arg_print_syntax(stdout, arguments_table, "\n");
        printf("Try using %s --help for more information.\n", PROGRAM_NAME);
        exit(0);
    }

    if (argument_help->count > 0) {
        printf("Usage: %s", PROGRAM_NAME);
        arg_print_syntax(stdout, arguments_table, "\n");
        arg_print_glossary_gnu(stdout, arguments_table);
        exit(0);
    }

    if (parse_errors_count > 0) {
        arg_print_errors(stderr, argument_end, PROGRAM_NAME);
        fprintf(stderr, "Try using --help option for more information on calling arguments.\n");
        exit(1);
    }

    int code = main_pinger(argument_source_ip->sval[0],
                           argument_destination_address->sval[0],
                           argument_ttl->ival[0],
                           argument_listen->count,
                           argument_continuous->count);
    arg_freetable(arguments_table, sizeof(arguments_table) / sizeof(arguments_table[0]));
    return code;
}

int main_pinger(const char* source_address,
                const char* destination_address,
                int ttl,
                int listen,
                int continuous) {
    if (ttl < 1) {
        ttl = 1;
    }
    if (ttl > 255) {
        ttl = 255;
    }

    if (source_address == NULL) {
        source_address == "127.0.0.1";
    }

    printf("Arguments:\n");
    printf("  source: %s\n", source_address);
    printf("  destination: %s\n", destination_address);
    printf("  TTL: %d\n", ttl);
    printf("  listen: %s\n", listen > 0 ? "yes" : "no");
    printf("  continuous: %s\n", continuous > 0 ? "yes" : "no");

    return 0;
}
