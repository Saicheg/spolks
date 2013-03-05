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
#include <errno.h>

#include "pinger.h"

#define PROGRAM_NAME "pinger"

int main_pinger(const char* source_address_string,
                const char* destination_address_string,
                int ttl,
                int listen);
void main_pinger_sender(int signal_number);
void main_pinger_receiver(char *packet_buffer,
                          int packet_length,
                          struct sockaddr_in *from);

u_int16_t data_checksum(u_short* data, size_t length);
char * data_icmp_packet_type(int t);

int socket_descriptor;

int request_packet_source_address;
int request_packet_destination_address;
int request_packet_pid;
int request_packet_ttl;

char received_packet[RECEIVED_PACKET_BUFFER_SIZE];

struct sockaddr_in destination_address;
struct sockaddr_in source_address;

int sender_packets_sent = 0;
int sender_stop_flag = 0;

double statistics_ping_min;
double statistics_ping_max;
double statistics_ping_sum;
int statistics_packets_received;

int catcher_listen_all;
int verbose = 0;

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
    struct arg_lit *argument_verbose = arg_lit0("v",
                                                "verbose",
                                                "Display more information.");
    struct arg_end *argument_end = arg_end(20);


    void* arguments_table[] = {
                               argument_listen,
                               argument_address_source,
                               argument_ttl,
                               argument_address_destination,
                               argument_verbose,
                               argument_help,
                               argument_end
    };

    if (arg_nullcheck(arguments_table) != 0) {
        printf("%s: insufficient memory\n", PROGRAM_NAME);
        exit(PINGER_EXITERROR_PARAMETERS);
    }

    if (argc == 1) {
        printf("Usage: %s", PROGRAM_NAME);
        arg_print_syntax(stdout, arguments_table, "\n");
        printf("Try using %s --help for more information.\n", PROGRAM_NAME);
        exit(EXIT_SUCCESS);
    }

    argument_ttl->ival[0] = 255;
    argument_address_source->sval[0] = NULL;

    int parse_errors_count = arg_parse(argc, argv, arguments_table);

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

    verbose = argument_verbose->count;

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
    struct sockaddr_in from;

    if (ttl < 1) {
        ttl = 1;
    }
    if (ttl > 255) {
        ttl = 255;
    }

    request_packet_pid = getpid();
    catcher_listen_all = listen;

    struct sigaction catcher_action;
    memset(&catcher_action, 0, sizeof(catcher_action));
    catcher_action.sa_handler = &main_pinger_sender;
    sigaction(SIGALRM, &catcher_action, NULL);
    sigaction(SIGINT, &catcher_action, NULL);

    socket_descriptor = socket(PF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (socket_descriptor == -1) {
        perror("Socket allocation error");
        exit(PINGER_EXITERROR_SOCKET_CREATE);
    }
    const int on = 1;
    setsockopt(socket_descriptor, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on));
    int size = 60 * 1024;
    setsockopt(socket_descriptor, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size));

    setuid(getuid());

    struct addrinfo *destination_addrinfo;
    int error_code;
    error_code = getaddrinfo(destination_address_string, NULL, NULL, &destination_addrinfo);
    if (error_code != 0) {
        printf("Destination address was not resolved: %s.\n", gai_strerror(error_code));
        return PINGER_EXITERROR_DESTINATION_ADDRESS;
    }

    memset(&destination_address, 0, sizeof(destination_address));
    destination_address.sin_family = AF_INET;
    destination_address.sin_addr = ((struct sockaddr_in*) destination_addrinfo->ai_addr)->sin_addr;
    freeaddrinfo(destination_addrinfo);

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

    if (verbose) {
        printf("Arguments:\n");
        printf("  source: %s\n", source_address_string);
        printf("  destination: %s\n", destination_address_string);
        printf("  TTL: %d\n", ttl);
    }

    printf("Pinging %s.\n", inet_ntoa(destination_address.sin_addr));

    struct itimerval timer;
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = 1;
    timer.it_interval.tv_sec = 1;
    timer.it_interval.tv_usec = 0;
    setitimer(ITIMER_REAL, &timer, NULL);

    while (sender_stop_flag == 0) {
        int received_packet_buffer_length = sizeof(received_packet);
        socklen_t fromlen = sizeof(from);
        int received_bytes;

        if ((received_bytes = recvfrom(socket_descriptor,
                                       received_packet,
                                       received_packet_buffer_length,
                                       0,
                                       (struct sockaddr*) &from,
                                       &fromlen)) < 0) {
            if (errno == EINTR)
                continue;
            perror("Receive packet error");
            continue;
        }
        main_pinger_receiver(received_packet, received_bytes, &from);
    }

    printf("\n--- Stats ---\n");
    printf("%d packets sent, %d received, %0.1f%% loss\n",
           sender_packets_sent,
           statistics_packets_received,
           (double) (sender_packets_sent - statistics_packets_received) * 100.0 / sender_packets_sent);
    if (verbose) {
        printf("Round-trip time (ms): min %0.2f, avg %0.2f, max %0.2f\n",
               statistics_ping_min,
               statistics_ping_sum / sender_packets_sent,
               statistics_ping_max);
    }
    return 0;
}

void main_pinger_sender(int signal_number) {
    if (signal_number == SIGALRM && sender_stop_flag == 0) {
        struct icmp_custom_packet *packet_icmp;
        packet_icmp = (struct icmp_custom_packet*) calloc(1, sizeof(struct icmp_custom_packet));
        packet_icmp->icmp_type = ICMP_ECHO;
        packet_icmp->icmp_code = 0;
        packet_icmp->icmp_cksum = 0;
        packet_icmp->icmp_pid = request_packet_pid;
        packet_icmp->icmp_seqnum = ++sender_packets_sent;

        gettimeofday(&packet_icmp->icmp_timestamp, NULL);

        packet_icmp->icmp_cksum = data_checksum((u_short*) packet_icmp,
                                                sizeof(struct icmp_custom_packet));

      /* TODO: source and desctination addresses here */
        sendto(socket_descriptor,
               packet_icmp,
               sizeof(packet_icmp[0]),
               0,
               (const struct sockaddr*) &destination_address,
               sizeof(struct sockaddr_in));
    } else if (signal_number == SIGINT) {
        sender_stop_flag = 1;
    }
}

u_int16_t data_checksum(u_short* data, size_t length) {
    register size_t nleft = length;
    register u_short *w = data;
    register u_short answer;
    register int sum = 0;

    while (nleft > 1) {
        sum += *w++;
        nleft -= 2;
    }

    if (nleft == 1) {
        u_short u = 0;
        *(u_char *) (&u) = *(u_char *) w;
        sum += u;
    }

    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    answer = ~sum;
    return(answer);
}

void main_pinger_receiver(char *packet_buffer, int packet_length, struct sockaddr_in *from) {
    struct ip *ip_packet;
    struct icmp *icmp_packet;
    int ip_header_length;
    struct timeval current_time, *packet_time, trip_time;

    gettimeofday(&current_time, NULL);

    ip_packet = (struct ip *) packet_buffer;
    ip_header_length = ip_packet->ip_hl << 2;
    if (packet_length < ip_header_length + ICMP_MINLEN) {
        if (verbose) {
            printf("packet too short (%d bytes) from %s\n",
                   packet_length,
                   inet_ntoa(from->sin_addr));
        }
        return;
    }
    packet_length -= ip_header_length;
    icmp_packet = (struct icmp *) (packet_buffer + ip_header_length);

    if (icmp_packet->icmp_type != ICMP_ECHOREPLY) {
        if (verbose) {
            printf("%d bytes from %s: icmp_type=%d (%s) icmp_code=%d\n",
                   packet_length,
                   inet_ntoa(from->sin_addr),
                   icmp_packet->icmp_type,
                   data_icmp_packet_type(icmp_packet->icmp_type),
                   icmp_packet->icmp_code);
        }
        return;
    }

    if (icmp_packet->icmp_id != request_packet_pid)
        return;

    packet_time = (struct timeval *) &icmp_packet->icmp_data[0];
    timersub(&current_time, packet_time, &trip_time);
    double current_time_ms = trip_time.tv_sec * 1000 + (double) trip_time.tv_usec / 1000;

    printf("%d bytes from %s: icmp_req=%d ttl=%u time=%0.1f ms\n",
           packet_length,
           inet_ntoa(from->sin_addr),
           icmp_packet->icmp_seq,
           ip_packet->ip_ttl,
           current_time_ms);
    fflush(stdout);

    statistics_ping_sum += current_time_ms;
    if (statistics_packets_received == 0 || statistics_ping_min > current_time_ms)
        statistics_ping_min = current_time_ms;
    if (statistics_ping_max < current_time_ms)
        statistics_ping_max = current_time_ms;
    statistics_packets_received++;
}

char * data_icmp_packet_type(int t) {
    static char *ttab[] = {
                           "Echo Reply",
                           "ICMP 1",
                           "ICMP 2",
                           "Dest Unreachable",
                           "Source Quench",
                           "Redirect",
                           "ICMP 6",
                           "ICMP 7",
                           "Echo",
                           "ICMP 9",
                           "ICMP 10",
                           "Time Exceeded",
                           "Parameter Problem",
                           "Timestamp",
                           "Timestamp Reply",
                           "Info Request",
                           "Info Reply"
    };
    if (t < 0 || t > 16)
        return("unknown");
    return(ttab[t]);
}
