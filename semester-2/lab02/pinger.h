/*
 * File:   pinger.h
 * Author: keritaf
 *
 * Created on February 19, 2013, 11:29 AM
 */

#ifndef PINGER_H
#define	PINGER_H

#include <sys/types.h>
#include <sys/time.h>

#define PINGER_EXITERROR_PARAMETERS 1
#define PINGER_EXITERROR_DESTINATION_ADDRESS 2
#define PINGER_EXITERROR_SOURCE_ADDRESS 3
#define PINGER_EXITERROR_SOCKET_CREATE 4

#define RECEIVED_PACKET_BUFFER_SIZE 65535

struct icmp_custom_packet {
  u_int8_t icmp_type;
  u_int8_t icmp_code;
  u_int16_t icmp_cksum;
  u_int16_t icmp_pid;
  u_int16_t icmp_seqnum;
  struct timeval icmp_timestamp;
};

struct pseudoip {
  u_int32_t source_address;
  u_int32_t dest_address;
  unsigned char place_holder;
  unsigned char protocol;
  u_int16_t length;
};

#endif	/* PINGER_H */

