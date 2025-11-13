/** 
 * @FilePath     \project\applications\Inc\userconfig.h
 * @Author       wunna
 * Version       V1.0.0
 * @Date         2025-04-03 12:19:30
 * @Description  
 * 
 * LastEditTime  2025-04-21 08:12:09
 */
#ifndef __USERCONFIG_H__
#define __USERCONFIG_H__

#include <rtdbg.h>
#include <rs485.h>
#include <string.h>
#include "agile_modbus.h"

/* ------------------------------- User Define ------------------------------ */
#define DBG_ENABLE
#define DBG_COLOR
#define DBG_SECTION_NAME            "rtu_master"
#define DBG_LEVEL                   DBG_LOG

#define MESSAGE_SIZE                8

#define MODBUS_BAUD_RATE            57600
#define MODBUS_PORT_NUM             "uart2"
#define MODBUS_PORT_PARITY          0

#define FLASH_BASE_ADDRESS          0x080C0000

/* ---------------------------- Private Variable ---------------------------- */
extern struct rt_messagequeue can_rx_queue;
extern struct rt_messagequeue can_tx_queue;
extern struct rt_messagequeue modbus_tx_queue;
extern rt_uint8_t msg_pool[2048];
extern rt_uint8_t msg_pool2[4096];
extern rt_uint8_t msg_pool3[2048];
extern rt_sem_t modbus_signal;
extern uint8_t userRegValue[2];
extern rs485_inst_t *hinst;

/* ------------------------------ User Function ----------------------------- */
void userCANInitialize();
void userModbusInitialize();
uint8_t board_get_address(void);

typedef union {
  int data_32t;
  int16_t data_16t[2];
  int8_t data_8t[4];
  int8_t val[4];
}DataStack;



#endif // !__USERCONFIG_H__