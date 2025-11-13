/** 
 * @FilePath     \project\applications\main.c
 * @Author       wunna
 * Version       V1.0.0
 * @Date         2025-04-02 15:15:25
 * @Description  
 * 
 * LastEditTime  2025-04-21 12:27:04
 */
/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-11-06     SummerGift   first version
 */

/* ----------------------------- Global Include ----------------------------- */
#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
/* ----------------------------- Private Include ---------------------------- */
#include <fal.h>
#include <fal_cfg.h>
#include "userconfig.h"
#include "protcol.h"
#include "drv_flash.h"
#include "agile_modbus.h"

/* ----------------------------- Private Define ----------------------------- */
#define MCU_CAN_AD0 rt_pin_get("PB.8")
#define MCU_CAN_AD1 rt_pin_get("PE.0")
#define MCU_CAN_AD2 rt_pin_get("PE.1")
#define MCU_CAN_AD3 rt_pin_get("PE.2")
#define MCU_CAN_AD4 rt_pin_get("PE.3")
#define MCU_CAN_AD5 rt_pin_get("PE.4")
#define MCU_CAN_AD6 rt_pin_get("PE.5")
#define MCU_CAN_AD7 rt_pin_get("PE.6") 

/* ----------------------------- Variable of CAN ---------------------------- */
struct rt_messagequeue can_rx_queue;
struct rt_messagequeue can_tx_queue;
struct rt_messagequeue modbus_rx_queue;
struct rt_messagequeue modbus_tx_queue;
rt_sem_t modbus_signal = RT_NULL;
rt_uint8_t msg_pool[2048];
rt_uint8_t msg_pool2[4096];
rt_uint8_t msg_pool3[2048];

extern agile_modbus_rtu_t ctx_rtu;
extern agile_modbus_t *ctx;

/* --------------------------- Variable of MODBUS --------------------------- */
void messageQueueInit(void);
uint8_t board_get_address(void);

// 主线程用于初始化、指令处理、发送心跳包
int main(void)
{
    CAN_DATA MessageCAN;
    CAN_DATA MessageHeartBeat;
    CAN_DATA MessageStack; 

    rt_pin_mode(rt_pin_get("PB.5"), PIN_MODE_INPUT);
    rt_pin_mode(MCU_CAN_AD0, PIN_MODE_INPUT);
    rt_pin_mode(MCU_CAN_AD1, PIN_MODE_INPUT);
    rt_pin_mode(MCU_CAN_AD2, PIN_MODE_INPUT);
    rt_pin_mode(MCU_CAN_AD3, PIN_MODE_INPUT);
    rt_pin_mode(MCU_CAN_AD4, PIN_MODE_INPUT);
    rt_pin_mode(MCU_CAN_AD5, PIN_MODE_INPUT);
    rt_pin_mode(MCU_CAN_AD6, PIN_MODE_INPUT);
    rt_pin_mode(MCU_CAN_AD7, PIN_MODE_INPUT);
    
    int heartTime = 0;
    int tictack = 0;
    rt_err_t beadResult = 0;

    modbus_signal = rt_sem_create("modbus_signal", 1, RT_IPC_FLAG_FIFO);
    if (modbus_signal == RT_NULL)
    {
        LOG_E("Modbus Signal Create failed.\n");
        return -1;
    }
    
    // 初始化区域
    messageQueueInit();
    userCANInitialize();
    userModbusInitialize();
    // fal_init();

    while (1)
    {
        // 功能1：发心跳包
        // tictack ++;
        // if (tictack == 20)
        // {
        MessageHeartBeat.can_msg.msgType = MSG_HEARTBEAT;
        MessageHeartBeat.can_msg.addr = board_get_address();
        MessageHeartBeat.can_msg.flag = 0;
            
        heartTime ++;
        memcpy(MessageHeartBeat.can_msg.dataBytes, &heartTime, 4);
        rt_mq_send(&can_tx_queue, MessageHeartBeat.data, MESSAGE_SIZE);
        // tictack = 0;
        // }

        // if (can_rx_queue.entry != 0)
        // {
        //     rt_mq_recv(&can_rx_queue, MessageCAN.data, MESSAGE_SIZE, RT_WAITING_FOREVER);
            
        //     // 功能2：链速&宽窄控制——雷赛 && 科睿源信息获取
        //     if (MessageCAN.can_msg.msgType == MSG_LS_CHAIN_CTRL || 
        //         MessageCAN.can_msg.msgType == MSG_MODBUS_POWER_READ_VOLTAGE || //获取电源输出电压
        //         MessageCAN.can_msg.msgType == MSG_MODBUS_POWER_READ_CURRENT || //获取电源输出电流
        //         MessageCAN.can_msg.msgType == MSG_MODBUS_POWER_READ_POWER) //获取电源输出功率
        //     {
        //         // 扔到modbus队列处理
        //         rt_mq_send(&modbus_tx_queue, MessageCAN.data, MESSAGE_SIZE);
        //     }

        // }
        
        rt_thread_mdelay(1000);
    }

    return RT_EOK;
}

void messageQueueInit(void)
{
    rt_err_t mq_create_res = rt_mq_init(&can_rx_queue, "CAN_Rx_quece", &msg_pool[0], MESSAGE_SIZE, sizeof(msg_pool), RT_IPC_FLAG_FIFO);
    rt_err_t mq_create_res2 = rt_mq_init(&can_tx_queue, "CAN_Tx_quece", &msg_pool2[0], MESSAGE_SIZE, sizeof(msg_pool2), RT_IPC_FLAG_FIFO);
    // rt_err_t mq_create_res3 = rt_mq_init(&modbus_tx_queue, "MODBUS_Tx_quece", &msg_pool3[0], MESSAGE_SIZE, sizeof(msg_pool3), RT_IPC_FLAG_FIFO);
    // rt_err_t mq_create_res4 = rt_mq_init(&can_tx_queue, "CAN_Tx_quece", &msg_pool2[0], sizeof(CAN_DATA), sizeof(msg_pool2), RT_IPC_FLAG_FIFO);
    if (mq_create_res != RT_EOK || mq_create_res2 != RT_EOK)
    {
        rt_kprintf("Init message queue of CAN failed!\r\n");
        return ;
    }
    rt_kprintf("Message Queue Create Succeed\r\n");
}

/*
* @brief 获取板卡设置地址（拨码）
* 
* @param NULL 
* @return 板卡地址（uint8_t）
* */
uint8_t board_get_address(void)
{
    uint8_t address = 0;
    address = rt_pin_read(MCU_CAN_AD0) << 0 |
                rt_pin_read(MCU_CAN_AD1) << 1 |
                rt_pin_read(MCU_CAN_AD2) << 2 |
                rt_pin_read(MCU_CAN_AD3) << 3 |
                rt_pin_read(MCU_CAN_AD4) << 4 |
                rt_pin_read(MCU_CAN_AD5) << 5 |
                rt_pin_read(MCU_CAN_AD6) << 6 |
                rt_pin_read(MCU_CAN_AD7) << 7 ;

    return ~address;
}