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

#include "contiki.h"
#include "uart1-arch.h"
#include "CC1312R1_LAUNCHXL.h"

#include <ti/drivers/UART.h>

#include <stdio.h>
#include <string.h>

PROCESS(uart_process, "UART process");
AUTOSTART_PROCESSES(&uart_process);

PROCESS_THREAD(uart_process, ev, data)
{
  static struct etimer timer;

  PROCESS_BEGIN();

  uart1_init();

  /* Setup a periodic timer that expires after 10 seconds. */
  etimer_set(&timer, CLOCK_SECOND * 10);

  int tx = CC1312R1_LAUNCHXL_UART1_TX;
  int rx = CC1312R1_LAUNCHXL_UART1_RX;

  printf("RX: %d\n", rx);
  printf("TX: %d\n", tx);

  while(1) {
    int_fast32_t rc = uart1_write("Hello world\n", strlen("Hello world\n"));
    if (rc == UART_ERROR) {
        printf("Failed!");
        break;
    }

    /* Wait for the periodic timer to expire and then restart the timer. */
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
    etimer_reset(&timer);
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
