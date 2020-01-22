/**
 * @file project-conf.h
 * @author Locha Mesh project developers (locha.io)
 * @brief 
 * @version 0.1
 * @date 2019-12-08
 * 
 * @copyright Copyright (c) 2019 Locha Mesh project developers
 * @license Apache 2.0, see LICENSE file for details
 */

#ifndef PROJECT_CONF_H_
#define PROJECT_CONF_H_

#define TI_UART_CONF_ENABLE 1                          /*!< Enable TI UART config */
#define TI_UART_CONF_UART0_ENABLE  TI_UART_CONF_ENABLE /*!< Enable UART0 */
#define TI_UART_CONF_UART1_ENABLE  TI_UART_CONF_ENABLE /*!< Enable UART1 */
#define TI_UART_CONF_BAUD_RATE     115200              /*! Baud rate */

#define LOG_CONF_LEVEL_IPV6                        LOG_LEVEL_DBG
#define LOG_CONF_LEVEL_6LOWPAN                     LOG_LEVEL_DBG
#define LOG_CONF_LEVEL_TCPIP                       LOG_LEVEL_DBG

#define UIP_CONF_ND6_SEND_RA 0
#define UIP_CONF_ND6_SEND_NS 0
#define UIP_CONF_ND6_SEND_NA 0

#endif // PROJECT_CONF_H_
