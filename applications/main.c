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
rt_uint8_t msg_pool2[2048];
rt_uint8_t msg_pool3[2048];

/* --------------------------- Variable of MODBUS --------------------------- */
// uint8_t slaveAddr = 0;
void messageQueueInit(void);
uint8_t board_get_address(void);

// 主线程用于初始化、指令处理、发送心跳包
int main(void)
{
    CAN_DATA MessageCAN;
    CAN_DATA MessageHeartBeat;

    rt_pin_mode(rt_pin_get("PB.5"), PIN_MODE_INPUT);
    rt_pin_mode(MCU_CAN_AD0, PIN_MODE_INPUT);
    rt_pin_mode(MCU_CAN_AD1, PIN_MODE_INPUT);
    rt_pin_mode(MCU_CAN_AD2, PIN_MODE_INPUT);
    rt_pin_mode(MCU_CAN_AD3, PIN_MODE_INPUT);
    rt_pin_mode(MCU_CAN_AD4, PIN_MODE_INPUT);
    rt_pin_mode(MCU_CAN_AD5, PIN_MODE_INPUT);
    rt_pin_mode(MCU_CAN_AD6, PIN_MODE_INPUT);
    rt_pin_mode(MCU_CAN_AD7, PIN_MODE_INPUT);
    
    
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
        // 发心跳包
        MessageHeartBeat.can_msg.msgType = MSG_HEARTBEAT;
        MessageHeartBeat.can_msg.addr = board_get_address();
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

            //链速控制
            if (MessageCAN.can_msg.msgType == MSG_CHAIN_CTRL)
            {
                // 读取
                if (MessageCAN.can_msg.flag == 1)
                {
                    float stackSet;
                    // 测试逻辑：从flash中读取数据的偏移量为 （通道数）*4 + index低位
                    uint8_t offset = (MessageCAN.can_msg.index >> 4) * 4 + (MessageCAN.can_msg.index & 0x00ff);
                    stm32_flash_read(FLASH_BASE_ADDRESS, &stackSet, sizeof(float));
                    
                    // 返回消息给上位机
                    CAN_DATA sendData;
                    sendData.can_msg.msgType = MSG_CHAIN_CTRL;
                    sendData.can_msg.flag = 1;
                    sendData.can_msg.index = MessageCAN.can_msg.index;
                    rt_memcpy(sendData.can_msg.dataBytes, &stackSet, sizeof(float));
                    
                    rt_mq_send(&can_tx_queue, sendData.data, MESSAGE_SIZE);
                }

                // 写入
                if (MessageCAN.can_msg.flag == 0)
                {
                    float stackSet;
                    rt_memcpy(&stackSet, &MessageCAN.can_msg.dataBytes, sizeof(MessageCAN.can_msg.dataBytes));
                    // 测试逻辑：往flash中写入数据的偏移量为 （通道数）*4 + index低位
                    uint8_t offset = (MessageCAN.can_msg.index >> 4) * 4 + (MessageCAN.can_msg.index & 0x00ff);
                    int writeFlashRes = stm32_flash_write(FLASH_BASE_ADDRESS + offset, &stackSet, sizeof(float));
                    if (writeFlashRes <= 0)
                    {
                        stm32_flash_erase(FLASH_BASE_ADDRESS + offset, sizeof(float));
                        stm32_flash_write(FLASH_BASE_ADDRESS + offset, &stackSet, sizeof(float));
                    }
                    
                    // 返回消息给上位机
                    rt_mq_send(&can_tx_queue, MessageCAN.data, MESSAGE_SIZE);                    
                }
                
            }
            
            // 链速控制——雷赛
            // if (MessageCAN.can_msg.msgType == MSG_LS_CHAIN_CTRL)
            // {
            //     // 写逻辑
            //     if (MessageCAN.can_msg.flag == 0)
            //     {
            //         if (MessageCAN.can_msg.index == 1 || 
            //             MessageCAN.can_msg.index == 2 || 
            //             MessageCAN.can_msg.index == 4 || 
            //             MessageCAN.can_msg.index == 5)
            //         {
            //             rt_mq_send(&modbus_tx_queue, MessageCAN.data, MESSAGE_SIZE); 
            //         }
                    
            //     }
            //     // 读逻辑
            //     else if (MessageCAN.can_msg.flag == 1)
            //     {
                    
            //     }
                
                
            // }
            
            
            
            
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

void messageQueueInit(void)
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