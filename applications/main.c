/** 
 * @FilePath     \project\applications\main.c
 * @Author       wunna
 * Version       V1.0.0
 * @Date         2025-04-02 15:15:25
 * @Description  
 * 
 * LastEditTime  2025-04-11 11:16:42
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
#include "userconfig.h"
#include "protcol.h"

/* ----------------------------- Variable of CAN ---------------------------- */
struct rt_messagequeue can_rx_queue;
struct rt_messagequeue can_tx_queue;
struct rt_messagequeue modbus_rx_queue;
struct rt_messagequeue modbus_tx_queue;
rt_sem_t modbus_signal = RT_NULL;
rt_uint8_t msg_pool[2048];
rt_uint8_t msg_pool2[2048];
rt_uint8_t msg_pool3[2048];

/* --------------------------- Variable of MODBUS --------------------------- */
// uint8_t slaveAddr = 0;

void messageQueueInit()
{
    rt_err_t mq_create_res = rt_mq_init(&can_rx_queue, "CAN_Rx_quece", &msg_pool[0], MESSAGE_SIZE, sizeof(msg_pool), RT_IPC_FLAG_FIFO);
    rt_err_t mq_create_res2 = rt_mq_init(&can_tx_queue, "CAN_Tx_quece", &msg_pool2[0], MESSAGE_SIZE, sizeof(msg_pool2), RT_IPC_FLAG_FIFO);
    rt_err_t mq_create_res3 = rt_mq_init(&modbus_tx_queue, "MODBUS_Tx_quece", &msg_pool3[0], MESSAGE_SIZE, sizeof(msg_pool3), RT_IPC_FLAG_FIFO);
    // rt_err_t mq_create_res4 = rt_mq_init(&can_tx_queue, "CAN_Tx_quece", &msg_pool2[0], sizeof(CAN_DATA), sizeof(msg_pool2), RT_IPC_FLAG_FIFO);
    if (mq_create_res != RT_EOK || mq_create_res2 != RT_EOK || mq_create_res3 != RT_EOK)
    {
        rt_kprintf("Init message queue of CAN failed!\r\n");
        return ;
    }
    rt_kprintf("Message Queue Create Succeed\r\n");
}

// 主线程用于初始化、指令处理、发送心跳包
int main(void)
{
    CAN_DATA MessageCAN;
    CAN_DATA MessageHeartBeat;

    rt_pin_mode(rt_pin_get("PB.5"), PIN_MODE_INPUT);
    rt_err_t beadResult = 0;

    modbus_signal = rt_sem_create("modbus_signal", 1, RT_IPC_FLAG_FIFO);
    if (modbus_signal == RT_NULL)
    {
        rt_kprintf("Modbus Signal Create failed.\n");
        return -1;
    }
    
    messageQueueInit();
    userCANInitialize();
    userModbusInitialize();
    /* set LED0 pin mode to output */
    while (1)
    {
        // 发心跳包
        MessageHeartBeat.can_msg.msgType = MSG_HEARTBEAT;
        MessageHeartBeat.can_msg.addr = 1;
        MessageHeartBeat.can_msg.flag = 0;
        rt_mq_send(&can_tx_queue, MessageHeartBeat.data, MESSAGE_SIZE);

        if (can_rx_queue.entry != 0)
        {
            rt_mq_recv(&can_rx_queue, MessageCAN.data, MESSAGE_SIZE, RT_WAITING_FOREVER);
            
            //风机控制
            if (MessageCAN.can_msg.msgType == MSG_FAN_CTRL)
            {
                //发modbus报文读取风机频率(uint16_t)
                if (MessageCAN.can_msg.flag == 1 && MessageCAN.can_msg.dataBytes[0])
                {
                    // slaveAddr = MessageCAN.can_msg.dataBytes[0];
                    rt_mq_send(&modbus_tx_queue, MessageCAN.data, MESSAGE_SIZE); 
                   
                }

                //发modbus报文启动风机频率(uint16_t)
                if (MessageCAN.can_msg.flag == 0 && MessageCAN.can_msg.dataBytes[0])
                {
                    // slaveAddr = MessageCAN.can_msg.dataBytes[0];
                    rt_mq_send(&modbus_tx_queue, MessageCAN.data, MESSAGE_SIZE); 
                   
                }

            }
            
            
            
            // // 测试段 1 
            // for (int i = 0; i < MESSAGE_SIZE; i++)
            // {
            //     rt_kprintf("%d ", MessageCAN.data[i]);
            // }
            // rt_kprintf("\n");
            // rt_kprintf("now queue have %d mess\r\n", can_rx_queue.entry);
            
        }
        

        
        rt_thread_mdelay(100);
    }

    return RT_EOK;
}
