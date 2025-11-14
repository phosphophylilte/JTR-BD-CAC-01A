#ifndef __MODBUS_TO_CAN_HANDLE_H__
#define __MODBUS_TO_CAN_HANDLE_H__
#include <string.h>
#include <stdint.h>
#include <protcol.h>
#include <userconfig.h>
extern int modbus_send_till_recv(uint16_t slave_addr, uint16_t reg_addr, uint16_t val);

extern int modbus_send_32t_till_recv(uint16_t slave_addr, uint16_t reg_addr, int val);

extern uint16_t modbus_read16t_till_recv(uint16_t slave_addr, uint16_t reg_addr);

extern agile_modbus_t *ctx;

void handle_motor6000_write(const CAN_DATA *rx);
void handle_ls_read(const CAN_DATA *rx);
void handle_ls_write(const CAN_DATA *rx);
void handle_power_read(const CAN_DATA *rx, uint16_t reg);

#endif /* __MODBUS_TO_CAN_HANDLE_H__ */