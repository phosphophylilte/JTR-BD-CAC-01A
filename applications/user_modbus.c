/** 
 * @FilePath     \project\applications\user_modbus.c
 * @Author       wunna
 * Version       V1.0.0
 * @Date         2025-04-03 11:51:28
 * @Description  
 * 
 * LastEditTime  2025-04-11 12:18:53
 */
#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include "protcol.h"
#include "userconfig.h"
#include "agile_modbus.h"


uint8_t userRegValue[2];
rs485_inst_t *hinst;

agile_modbus_rtu_t ctx_rtu;
agile_modbus_t *ctx;

static void send_thread_entry(void *parameter)
{
    // eMBMasterReqErrCode error_code = MB_MRE_NO_ERR;
    CAN_DATA messageStack;
    rt_uint16_t error_count = 0;
    rt_err_t result;
    rt_uint16_t dataStack;
    // USHORT data[2] = {0};
    

    
    while (1)
    {
        result =rt_mq_recv(&modbus_tx_queue, messageStack.data, MESSAGE_SIZE, RT_WAITING_FOREVER);        
        if (result != 0)
        {
            uint16_t slaveAddr = messageStack.can_msg.dataBytes[0];
            // flag == 1: 读取保持寄存器
            if (messageStack.can_msg.flag == 1)
            {   
                // 从站ID为dataBytes[0]
                rt_sem_take(modbus_signal, 2000);
                agile_modbus_set_slave(ctx, slaveAddr);
                
                int sendLen = agile_modbus_serialize_read_registers(ctx, MODBUS_TARGET_HOLDING_REG, 1);
                int recLen = rs485_send_then_recv(hinst, ctx->send_buf, sendLen, ctx->read_buf, ctx->read_bufsz);

                if (recLen < 0)
                {
                    LOG_E("Recieve error");
                    break;
                }
                else if (recLen == 0) 
                {
                    LOG_W("Receive timeout.");
                    break;LOG_W("Receive timeout.");
                    break;
                }

                int rc = agile_modbus_deserialize_read_registers(ctx, recLen, &dataStack);
                dataStack /= 200;
                LOG_I("Hold Registers: %d", dataStack);

                CAN_DATA freqRecall;
                freqRecall.can_msg.msgType = MSG_FAN_CTRL;
                freqRecall.can_msg.addr = 1;
                freqRecall.can_msg.flag = 1;
                freqRecall.can_msg.dataBytes[0] = slaveAddr;
                freqRecall.can_msg.dataBytes[1] = dataStack >> 8;
                freqRecall.can_msg.dataBytes[2] = dataStack & 0x00ff;
                // rt_memcpy(&freqRecall.can_msg.dataBytes[1], &dataStack, 2);// 将返回的频率存储于dataBytes[1:2]
                
                //将返回数据通过can发送出去
                rt_mq_urgent(&can_tx_queue, freqRecall.data, MESSAGE_SIZE);

                rt_sem_release(modbus_signal);
            }
            // flag == 0: 写保持寄存器
            else if (messageStack.can_msg.flag == 0)
            {
                rt_sem_take(modbus_signal, 2000);
                agile_modbus_set_slave(ctx, slaveAddr);

                uint16_t fanFreq = 0;
                uint16_t slaveAddr = messageStack.can_msg.dataBytes[0];
                // 将8位长的dataBytes[1:2]合并为uint16_t
                fanFreq = (messageStack.can_msg.dataBytes[1] << 8) | messageStack.can_msg.dataBytes[2];
                fanFreq *= 200;
                int sendlen = agile_modbus_serialize_write_register(ctx, MODBUS_TARGET_HOLDING_REG, fanFreq);
                int recLen = rs485_send_then_recv(hinst, ctx->send_buf, sendlen, ctx->read_buf, ctx->read_bufsz);

                if (recLen < 0)
                {
                    LOG_E("Send error");
                    break;
                }
                else if (recLen == 0)
                {
                    LOG_W("Send timeout.");
                    break;
                }

                LOG_I("Send Success");
                rt_sem_release(modbus_signal);
                
            }
        }
    }
}

static void mb_master_poll(void *parameter)
{
    const char read_cmd[] = "read datas test\r\n";
    static rt_uint8_t buf[256];
    

    while (1)
    {
        // int len = strlen(read_cmd);
        // len = rs485_send_then_recv(hinst, (void *)read_cmd, len, buf, sizeof(buf));
        // if (len < 0)
        // {
        //     rt_kprintf("rs485 send datas error.");
        //     break;
        // }
            
        // if (len == 0)
        // {
            
        //     continue;
        // }

        // rt_kprintf("rs485 recv %d datas : %s", len, buf);
    }
    
    rs485_destory(hinst);
    // eMBMasterInit(MB_RTU, MODBUS_PORT_NUM, MODBUS_BAUD_RATE, MODBUS_PORT_PARITY);
    // eMBMasterEnable();

    // while (1)
    // {
    //     eMBMasterPoll();
    //     rt_thread_mdelay(100);
    // }
}

void userModbusInitialize()
{
    static rt_uint8_t is_init = 0;
    
    uint8_t ctx_send_buf[AGILE_MODBUS_MAX_ADU_LENGTH];
    uint8_t ctx_read_buf[AGILE_MODBUS_MAX_ADU_LENGTH];
    uint16_t hold_register;
    ctx = &ctx_rtu._ctx;

    agile_modbus_rtu_init(&ctx_rtu, ctx_send_buf, sizeof(ctx_send_buf), ctx_read_buf, sizeof(ctx_read_buf));

    // RS485 Initalize
    hinst = rs485_create(MODBUS_PORT_NUM, MODBUS_BAUD_RATE, MODBUS_PORT_PARITY, -1, 0);
    
    if (hinst == RT_NULL)
    {
        rt_kprintf("create rs485 instance fail.");
        return;
    }

    rs485_set_recv_tmo(hinst, 5000);
    if (rs485_connect(hinst) != RT_EOK)
    {
        rs485_destory(hinst);
        rt_kprintf("rs485 connect fail.");
        return;
    }
    
    // // 发个报文试试
    // int send_len =agile_modbus_serialize_read_registers(ctx, 3099, 1);
    // rs485_send(hinst, ctx->send_buf, send_len);
    // int read_len = rs485_recv(hinst, ctx->read_buf, ctx->read_bufsz);
    // if (read_len < 0)
    // {
    //     LOG_E("Recieve error");
    // }
    // else if (read_len == 0) 
    // {
    //     LOG_W("Receive timeout.");
    // }
    // else
    // {
    //     int rc = agile_modbus_deserialize_read_registers(ctx, read_len, &hold_register);
    //     LOG_I("Hold Registers: %d", hold_register);
    // }

    rt_thread_t tid1 = RT_NULL, tid2 = RT_NULL;
    // tid1 = rt_thread_create("md_m_poll", mb_master_poll, RT_NULL, 2048, 10, 10);

    // if (tid1 != RT_NULL)
    // {
    //     rt_thread_startup(tid1);
    // }
    // else
    // {
    //     goto __exit;
    // }

    tid2 = rt_thread_create("md_m_send", send_thread_entry, RT_NULL, 2048, 30, 10);
    if (tid2 != RT_NULL)
    {
        rt_thread_startup(tid2);
    }
    else
    {
        goto __exit;
    }

    is_init = 1;
    return RT_EOK;

__exit:
    // if (tid1)
    //     rt_thread_delete(tid1);
    if (tid2)
        rt_thread_delete(tid2);
}