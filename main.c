/**
 * @file main.c
 * @author Locha Mesh Developers (contact@locha.io)
 * @brief Main firmware file
 * @version 0.1
 * @date 2020-02-02
 * 
 * @copyright Copyright (c) 2020 Locha Mesh project developers
 * @license Apache 2.0, see LICENSE file for details
 * 
 */

// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
#include <unistd.h> // for getting the pid

#define ENABLE_DEBUG (1)
#include "debug.h"
#include <inttypes.h>

#include "aodvv2.h"
// #include "routingtable.h"

#include "msg.h"
#include "shell.h"



// #include <arpa/inet.h>
// #include <netinet/in.h>
// #include <sys/socket.h>

#include "eui64.h"
#include "net/gnrc/ipv6/nib.h"

#include "net/gnrc/ipv6/nib.h"
#include "net/gnrc/ipv6/nib/nc.h"

#include "net/gnrc.h"
// #include "net/gnrc/pktdump.h"
#include "net/netdev_test.h"
#include "od.h"
#include "xtimer.h"

#include "net/gnrc/ipv6.h"

#include "luid.h"
#include "shell_functions.c"


#include "shell.h"
#include "msg.h"
#include "net/ipv6/addr.h"
#include "net/gnrc/pkt.h"
#include "net/gnrc/pktbuf.h"
#include "net/gnrc/netreg.h"
#include "net/gnrc/netapi.h"
#include "net/gnrc/netif.h"
#include "net/gnrc/netif/conf.h"
#include "net/gnrc/netif/ieee802154.h"
#include "net/gnrc/netif/hdr.h"
#include "net/gnrc/pktdump.h"
#include "net/netdev_test.h"
#include "xtimer.h"



// static gnrc_netreg_entry_t _dumper;
// static msg_t _dumper_queue[DUMPER_QUEUE_SIZE];
// static char _dumper_stack[THREAD_STACKSIZE_MAIN];

extern void init_socket(void);
extern int show_routingtable(int argc, char **argv);
extern int demo_send(int argc, char **argv);
extern int udp_cmd(int argc, char **argv);
extern int demo_add_neighbor(int argc, char **argv);
extern void *_demo_receiver_thread(void *arg);
extern int list_neighbors(int argc, char **argv);

const shell_command_t shell_commands[] = {
    {"print_rt", "print routingtable", show_routingtable},
    {"add_neighbor", "add neighbor to Neighbor Cache", demo_add_neighbor},
    {"send", "send message to ip", demo_send},
    {"list", "listar neighbors", list_neighbors},
    {NULL, NULL, NULL}};

int main(void) {
  void *iter_state = NULL;
  gnrc_ipv6_nib_nc_t nce;

  msg_init_queue(msg_q, RCV_MSG_Q_SIZE);
  init_socket();
  aodv_init();


  (void)puts("Welcome to RIOT!\n");
  _mock_netif = gnrc_netif_iter(_mock_netif);
  if (_mock_netif != NULL) {
    DEBUG("tenemos una interface PID: %d\n", _mock_netif->pid);
    ipv6_addr_t ipv6_addr;
    int res = gnrc_netapi_get(_mock_netif->pid, NETOPT_IPV6_ADDR, 0,
                              (void *)&ipv6_addr, sizeof(ipv6_addr));

    char addr_str[IPV6_ADDR_MAX_STR_LEN];
     ipv6_addr_to_str(addr_str, &ipv6_addr, sizeof(ipv6_addr));
    DEBUG("-------------------dst = %s",addr_str);
    printf("***********la salida es %d\n", res);
  }

  uint8_t hwaddr[GNRC_NETIF_L2ADDR_MAXLEN];
  uint16_t hwaddr_len;

  if (gnrc_netapi_get(_mock_netif->pid, NETOPT_SRC_LEN, 0, &hwaddr_len,
                      sizeof(hwaddr_len)) < 0) {
    return false;
  }

  luid_get(hwaddr, hwaddr_len);
  for (unsigned int j = 0; j < GNRC_NETIF_L2ADDR_MAXLEN; j++) {
    printf(" 0x%02x", hwaddr[j]);
  }
  printf("\n");

  // neighbor information base init
  gnrc_ipv6_nib_init();
  gnrc_netif_acquire(_mock_netif);
  // to bind desired iface to nib
  gnrc_ipv6_nib_init_iface(_mock_netif);
  gnrc_netif_release(_mock_netif);

  // this should print the neighbord list but I cannnot see anything
  while (gnrc_ipv6_nib_nc_iter(0, &iter_state, &nce)) {
    gnrc_ipv6_nib_nc_print(&nce);
  }
  printf("LA IFACE ES: %u", gnrc_ipv6_nib_nc_get_iface(&nce));

  thread_create(_rcv_stack_buf, sizeof(_rcv_stack_buf), THREAD_PRIORITY_MAIN,
                THREAD_CREATE_STACKTEST, _demo_receiver_thread, NULL,
                "_demo_rcv_thread");

  puts("All up, running the shell now");
  char line_buf[SHELL_DEFAULT_BUFSIZE];
  shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);

  return 0;
}
