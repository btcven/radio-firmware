/**
 * @file radio-firmware.c
 * @author Locha Mesh project developers (locha.io)
 * @brief
 * @version 0.1
 * @date 2019-12-08
 * 
 * @copyright Copyright (c) 2019 Locha Mesh project developers
 * @license Apache 2.0, see LICENSE file for details
 */

#include <stdio.h>
#include <string.h>

#include "contiki.h"
#include "contiki-net.h"

#ifndef RENODE
#include "uart1-arch.h"
#include "CC1312R1_LAUNCHXL.h"
#include <ti/drivers/UART.h>
#endif

#include "aodv-routing.h"
#include "aodv-rt.h"

PROCESS(radio_main_process, "Radio main process");
AUTOSTART_PROCESSES(&radio_main_process);

PROCESS_THREAD(radio_main_process, ev, data)
{
  static struct etimer timer;
  static uip_ipaddr_t peeraddr; 

  PROCESS_BEGIN();

  aodv_rt_init();
  aodv_routing_init();

#ifndef RENODE
  uart1_init();
#endif

  uip_ip6addr(&peeraddr, 0xfe80,0,0,0,0x0200,0,0,3);
  aodv_request_route_to(&peeraddr);

  /* Setup a periodic timer that expires after 10 seconds. */
  etimer_set(&timer, CLOCK_SECOND * 10);

  while(1) {
#ifndef RENODE
    int_fast32_t rc = uart1_write("Hello world\n", strlen("Hello world\n"));
    if (rc == UART_ERROR) {
        printf("Failed!");
        break;
    }
#endif

    /* Wait for the periodic timer to expire and then restart the timer. */
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
    etimer_reset(&timer);
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
