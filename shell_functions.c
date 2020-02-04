#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define ENABLE_DEBUG (1)
#include "debug.h"
#include <inttypes.h>

#include "net/gnrc/ipv6/nib.h"

#include "constants.h"

#include "net/gnrc/ipv6/nib.h"
#include "net/gnrc/ipv6/nib/nc.h"

#define DUMPER_QUEUE_SIZE (16)
// constants from the AODVv2 Draft, version 03
#define DISCOVERY_ATTEMPTS_MAX (1) //(3)
#define RREQ_WAIT_TIME (2000000)
char _rcv_stack_buf[THREAD_STACKSIZE_MAIN];

#define RCV_MSG_Q_SIZE (64)
msg_t msg_q[RCV_MSG_Q_SIZE];

gnrc_netif_t *_mock_netif = NULL;
struct sockaddr_in6 _sockaddr;
static int _sock_snd;
timex_t _now;

int demo_attempt_to_send(char *dest_str, char *msg) {
  uint8_t num_attempts = 0;

  // turn dest_str into ipv6_addr_t
  inet_pton(AF_INET6, dest_str, &_sockaddr.sin6_addr);
  int msg_len = strlen(msg) + 1;
  (void)msg_len;
  xtimer_now_timex(&_now);

  printf("{%" PRIu32 ":%" PRIu32
         "}[demo]   sending packet of %i bytes towards %s...\n",
         _now.seconds, _now.microseconds, msg_len, dest_str);

  while (num_attempts < DISCOVERY_ATTEMPTS_MAX) {
    int bytes_sent = sendto(_sock_snd, msg, msg_len, 0,
                            (struct sockaddr *)&_sockaddr, sizeof _sockaddr);

    printf("los bytes enviados son %d", bytes_sent);

    xtimer_now_timex(&_now);
    if (bytes_sent == -1) {
      printf(
          "{%" PRIu32 ":%" PRIu32
          "}[demo]   no bytes sent, probably because there is no route yet.\n",
          _now.seconds, _now.microseconds);
      num_attempts++;
      xtimer_usleep(RREQ_WAIT_TIME);
    } else {
      printf("{%" PRIu32 ":%" PRIu32
             "}[demo]   Success sending Data: %d bytes sent.\n",
             _now.seconds, _now.microseconds, bytes_sent);
      return 0;
    }
  }
  printf("{%" PRIu32 ":%" PRIu32
         "}[demo]  Error sending Data: no route found\n",
         _now.seconds, _now.microseconds);
  return -1;
}

int demo_send(int argc, char **argv) {
  if (argc != 3) {
    printf("Usage: send <destination ip> <message>\n");
    return 1;
  }

  char *dest_str = argv[1];
  char *msg = argv[2];

  return demo_attempt_to_send(dest_str, msg);
}

void init_socket(void) {
  _sockaddr.sin6_family = AF_INET6;
  _sockaddr.sin6_port = (int)htons(MANET_PORT);

  _sock_snd = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);

  if (-1 == _sock_snd) {
    printf("[demo]   Error Creating Socket!\n");
    return;
  }
}

void *_demo_receiver_thread(void *args) {
  (void)args;
  struct sockaddr_in6 sa_rcv;
  msg_t rcv_msg_q[RCV_MSG_Q_SIZE];
  msg_init_queue(rcv_msg_q, RCV_MSG_Q_SIZE);
  int _socket = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);

  sa_rcv.sin6_family = AF_INET6;
  memset(&sa_rcv.sin6_addr, 0, sizeof(sa_rcv.sin6_addr));
  sa_rcv.sin6_port = htons(MANET_PORT);
  if (_socket < 0) {
    puts("error initializing socket");
    _socket = 0;
    return NULL;
  }

  if (bind(_socket, (struct sockaddr *)&sa_rcv, sizeof(sa_rcv)) < 0) {
    _socket = -1;
    puts("error binding socket");
    return NULL;
  }
  printf("Success: started UDP server on port %" PRIu16 "\n", MANET_PORT);
  while (1) {
    int res;
    struct sockaddr_in6 src;
    socklen_t src_len = sizeof(struct sockaddr_in6);
    if ((res = recvfrom(_socket, _rcv_stack_buf, sizeof(_rcv_stack_buf), 0,
                        (struct sockaddr *)&src, &src_len)) < 0) {
      puts("Error on receive");
    } else if (res == 0) {
      puts("Peer did shut down");
    } else {
      printf("<APP LAYER> Received data: ");
      puts(_rcv_stack_buf);
    }
  }
  return NULL;
}