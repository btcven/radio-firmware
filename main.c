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

#include <unistd.h> // for getting the pid

#define ENABLE_DEBUG (1)
#include "debug.h"
#include <inttypes.h>

#include "aodvv2.h"
#include "msg.h"
#include "shell.h"

#include "eui64.h"
#include "net/gnrc/ipv6/nib.h"

#include "net/gnrc/ipv6/nib.h"
#include "net/gnrc/ipv6/nib/nc.h"

#include "net/gnrc.h"


#include "msg.h"
#include "shell.h"
#include "shell_functions.c"
#include "xtimer.h"

extern void init_socket(void);
extern int demo_send(int argc, char **argv);
extern int udp_cmd(int argc, char **argv);
extern void *_demo_receiver_thread(void *arg);

const shell_command_t shell_commands[] = {
    {"send", "send message to ip", demo_send}, {NULL, NULL, NULL}};

int main(void) {

  msg_init_queue(msg_q, RCV_MSG_Q_SIZE);
  init_socket();
  gnrc_aodvv2_init();

  puts("Welcome to RIOT!\n");
  _mock_netif = gnrc_netif_iter(_mock_netif);

  if (_mock_netif != NULL) {
    DEBUG("interface PID: %d\n", _mock_netif->pid);
    ipv6_addr_t ipv6_addr;
    gnrc_netapi_get(_mock_netif->pid, NETOPT_IPV6_ADDR, 0,
                              (void *)&ipv6_addr, sizeof(ipv6_addr));

    char addr_str[IPV6_ADDR_MAX_STR_LEN];
    ipv6_addr_to_str(addr_str, &ipv6_addr, sizeof(ipv6_addr));
    DEBUG("-------------------dst = %s", addr_str);
  }

  // neighbor information base init
  gnrc_ipv6_nib_init();
  gnrc_netif_acquire(_mock_netif);
  // to bind desired iface to nib
  gnrc_ipv6_nib_init_iface(_mock_netif);
  gnrc_netif_release(_mock_netif);

  thread_create(_rcv_stack_buf, sizeof(_rcv_stack_buf), THREAD_PRIORITY_MAIN,
                THREAD_CREATE_STACKTEST, _demo_receiver_thread, NULL,
                "_demo_rcv_thread");

  puts("All up, running the shell now");
  char line_buf[SHELL_DEFAULT_BUFSIZE];
  shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);

  return 0;
}
