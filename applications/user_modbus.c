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
#include <stdlib.h>

// 功能函数：大小端转换
#define swap32Big2Little(x)    (   ( (x)&(0x0000ffff) ) << 32 |  ( (x)&(0xffff0000) >> 32   ))

uint8_t userRegValue[2];
rs485_inst_t *hinst;

agile_modbus_rtu_t ctx_rtu;
agile_modbus_t *ctx;

int modbus_send_till_recv(uint16_t slave_addr, uint16_t reg_addr, uint16_t val);
uint16_t modbus_read16t_till_recv(uint16_t slave_addr, uint16_t reg_addr);
void userModbusInitialize(void);

static void send_thread_entry(void *parameter)
{
    // eMBMasterReqErrCode error_code = MB_MRE_NO_ERR;
    CAN_DATA messageStack;
    CAN_DATA CanSendMess;

    DataStack data_stack = {0};
    
    while (1)
    {
        // if (modbus_tx_queue.entry != 0)
        if(can_rx_queue.entry != 0)
        {
            // rt_mq_recv(&modbus_tx_queue, messageStack.data, MESSAGE_SIZE, RT_WAITING_FOREVER);
            rt_mq_recv(&can_rx_queue, messageStack.data, MESSAGE_SIZE, RT_WAITING_FOREVER);
            uint16_t mode = 0;
            int16_t speed = 0;
            uint16_t zero_mode = 0;
            int position = 0;
            uint16_t slaveAddr = messageStack.can_msg.addr;

            // flag == 1: LS电机读取操作
            if (messageStack.can_msg.msgType == MSG_LS_CHAIN_CTRL && messageStack.can_msg.flag == 1)
            {   
                switch (messageStack.can_msg.index)
                {
                case 5: // 读取当前位置（单位：转）
                    data_stack.data_16t[0] = modbus_read16t_till_recv(slaveAddr, 0x0B18);
                    data_stack.data_16t[1] = modbus_read16t_till_recv(slaveAddr, 0x0B19);

                    /* 将modbus返回的位置值塞到CAN TX队列 */
                    CanSendMess.can_msg.addr = slaveAddr;
                    CanSendMess.can_msg.flag = 1;
                    CanSendMess.can_msg.index = 5;
                    CanSendMess.can_msg.msgType = MSG_LS_CHAIN_CTRL;
                    memcpy(CanSendMess.can_msg.dataBytes, data_stack.val, 4);

                    rt_mq_send(&can_tx_queue, CanSendMess.data, MESSAGE_SIZE);
                    break;

                case 6: // 读取当前转速（单位：rpm）
                    // modbus_read16t_till_recv(slaveAddr, 0x0B09, &data_stack.data_16t[0]);
                    data_stack.data_16t[0] = modbus_read16t_till_recv(slaveAddr, 0x0B09);
                    
                    /* 将modbus返回的速度值塞到CAN TX队列 */
                    CanSendMess.can_msg.addr = slaveAddr;
                    CanSendMess.can_msg.flag = 1;
                    CanSendMess.can_msg.index = 6;
                    CanSendMess.can_msg.msgType = MSG_LS_CHAIN_CTRL;
                    memcpy(CanSendMess.can_msg.dataBytes, data_stack.val, 4);

                    rt_mq_send(&can_tx_queue, CanSendMess.data, MESSAGE_SIZE);
                    break;


                case 7: // 读取实时状态
                    data_stack.data_16t[0] = modbus_read16t_till_recv(slaveAddr, 0x0B05);

                    /* 将modbus返回的状态值塞到CAN TX队列 */
                    CanSendMess.can_msg.addr = slaveAddr;
                    CanSendMess.can_msg.flag = 1;
                    CanSendMess.can_msg.index = 7;
                    CanSendMess.can_msg.msgType = MSG_LS_CHAIN_CTRL;
                    memcpy(CanSendMess.can_msg.dataBytes, data_stack.val, 4);

                    rt_mq_send(&can_tx_queue, CanSendMess.data, MESSAGE_SIZE);
                    break;

                case 8: // 报警读取(屏蔽高四位)
                    data_stack.data_16t[0] = modbus_read16t_till_recv(slaveAddr, 0x2203);

                    /* 将modbus返回的报警值塞到CAN TX队列 */
                    CanSendMess.can_msg.addr = slaveAddr;
                    CanSendMess.can_msg.flag = 1;
                    CanSendMess.can_msg.index = 8;
                    CanSendMess.can_msg.msgType = MSG_LS_CHAIN_CTRL;
                    data_stack.data_16t[0] &= 0x0FFF;
                    memcpy(CanSendMess.can_msg.dataBytes, data_stack.val, 4);

                    rt_mq_send(&can_tx_queue, CanSendMess.data, MESSAGE_SIZE);
                    break;

                default:
                    break;
                }
                continue;
            }
            // flag == 0: LS电机写操作
            else if (messageStack.can_msg.msgType == MSG_LS_CHAIN_CTRL && messageStack.can_msg.flag == 0)
            {
                int sendFlag = 0;
                // rt_sem_take(modbus_signal, 2000);
                switch (messageStack.can_msg.index)
                {
                case 0: // 电机控制
                    memcpy(&mode, &messageStack.can_msg.dataBytes[0], 2);
                    if (mode == 2) // 启动电机
                    { 
                        sendFlag = modbus_send_till_recv(messageStack.can_msg.addr,
                                                0x6002,
                                                0x0010);

                    }
                    if (mode == 8) //停止电机
                    {
                        sendFlag = modbus_send_till_recv(messageStack.can_msg.addr,
                                                0x6002,
                                                0x0040);
                    }
                    
                    else if (mode == 4) // 回零启动
                    {
                        // /* 设置高速 */
                        // modbus_send_till_recv(messageStack.can_msg.addr,
                        //                         0x600F,
                        //                         0x0064);
                        
                        // /* 设定低速 */
                        // modbus_send_till_recv(messageStack.can_msg.addr,
                        //                         0x6010,
                        //                         0x001E);
                        
                        /* 触发回零 */
                        sendFlag = modbus_send_till_recv(messageStack.can_msg.addr,
                                                0x6002,
                                                0x0020);
                    }
                    break;

                case 1: // 设置模式
                    memcpy(&mode, &messageStack.can_msg.dataBytes[0], 2);
                    if (mode == 0) // 位置模式
                    {
                        // 设置位置模式
                        sendFlag = modbus_send_till_recv(messageStack.can_msg.addr,
                                                0x6200,
                                                0x0001);
                    }
                    else if (mode == 1) // 速度模式
                    {
                        sendFlag = modbus_send_till_recv(messageStack.can_msg.addr,
                                                0x6200,
                                                0x0002);
                    }
                    
                case 2: // 设置回零模式
                    zero_mode = messageStack.can_msg.dataBytes[0];
                    if (zero_mode == 1)
                    {
                        sendFlag = modbus_send_till_recv(messageStack.can_msg.addr,
                                                0x600A,
                                                0x0005);
                    }
                    else if (zero_mode == 2)
                    {
                        sendFlag = modbus_send_till_recv(messageStack.can_msg.addr,
                                                0x600A,
                                                0x0004);
                    }
                    break;

                case 3: // 设置加速度
                    memcpy(&data_stack.data_16t[0], &messageStack.can_msg.dataBytes[0], 2);
                    sendFlag = modbus_send_till_recv(messageStack.can_msg.addr,
                                                0x620C,
                                                data_stack.data_16t[0]);

                case 4: // 设置减速度
                    memcpy(&data_stack.data_16t[0], &messageStack.can_msg.dataBytes[0], 2);
                    sendFlag = modbus_send_till_recv(messageStack.can_msg.addr,
                                                0x620D,
                                                data_stack.data_16t[0]);
                
                case 5: // 设置电机位置
                    memcpy(&data_stack.data_32t, messageStack.can_msg.dataBytes, 4);
                    // data_stack.data_32t *= 10000;
                    // data_stack.data_32t = swap32Big2Little(data_stack.data_32t); 
                    if (modbus_send_till_recv(messageStack.can_msg.addr,
                                            0x6201,
                                            data_stack.data_16t[1]) == 0)
                    {
                        rt_thread_mdelay(10);
                        sendFlag = modbus_send_till_recv(messageStack.can_msg.addr,
                                            0x6202,
                                            data_stack.data_16t[0]);
                    }
                    break;

                case 6: // 设置电机转速
                    // memcpy(&data_stack.data_16t[0], messageStack.can_msg.dataBytes[0], 2);
                    data_stack.data_8t[0] = messageStack.can_msg.dataBytes[0];
                    data_stack.data_8t[1] = messageStack.can_msg.dataBytes[1];
                    sendFlag = modbus_send_till_recv(messageStack.can_msg.addr,
                                            0x6203,
                                            (uint16_t)data_stack.data_16t[0]);
                    break;

                default:
                    break;
                }

                // 若Modbus写操作成功，将CAN报文原样返回
                if (sendFlag == 0)
                {
                    rt_mq_send(&can_tx_queue, messageStack.data, MESSAGE_SIZE);
                }
                
                continue;
                // rt_sem_release(modbus_signal);
            }
            // MSG_MODBUS_POWER_READ_CURRENT：电源电流读取
            else if (messageStack.can_msg.msgType == MSG_MODBUS_POWER_READ_CURRENT && messageStack.can_msg.flag == 1)
            {
                rt_thread_delay(1); // 延时1ms，确保线程有时间调度
                rt_uint16_t input_register[1] = {0};

                agile_modbus_set_slave(ctx, messageStack.can_msg.index);
                int send_len = agile_modbus_serialize_read_input_registers(ctx, 0x33, 2);
                int read_len = rs485_send_then_recv(hinst, ctx->send_buf, send_len, ctx->read_buf, ctx->read_bufsz);

                int rc = agile_modbus_deserialize_read_registers(ctx, read_len, input_register);
                
                messageStack.can_msg.dataBytes[0] = input_register[0];
                messageStack.can_msg.dataBytes[1] = input_register[0] >> 8;
                rt_mq_send(&can_tx_queue, messageStack.data, MESSAGE_SIZE);
                rt_thread_delay(1); 
                continue;
            }
            // MSG_MODBUS_POWER_READ_VOLTAGE：电源电压读取
            else if (messageStack.can_msg.msgType == MSG_MODBUS_POWER_READ_VOLTAGE && messageStack.can_msg.flag == 1)
            {
                rt_thread_delay(1); // 延时1ms，确保线程有时间调度
                rt_uint16_t input_register[1] = {0};

                agile_modbus_set_slave(ctx, messageStack.can_msg.index);
                int send_len = agile_modbus_serialize_read_input_registers(ctx, 0x32, 2);
                int read_len = rs485_send_then_recv(hinst, ctx->send_buf, send_len, ctx->read_buf, ctx->read_bufsz);

                int rc = agile_modbus_deserialize_read_registers(ctx, read_len, input_register);
                
                messageStack.can_msg.dataBytes[0] = input_register[0];
                messageStack.can_msg.dataBytes[1] = input_register[0] >> 8;
                rt_mq_send(&can_tx_queue, messageStack.data, MESSAGE_SIZE);
                rt_thread_delay(1); 
                continue;
            }
            // MSG_MODBUS_POWER_READ_POWER：功率读取
            else if (messageStack.can_msg.msgType == MSG_MODBUS_POWER_READ_POWER && messageStack.can_msg.flag == 1)
            {
                rt_thread_delay(1); // 延时1ms，确保线程有时间调度
                rt_uint16_t input_register[1] = {0};

                agile_modbus_set_slave(ctx, messageStack.can_msg.index);
                int send_len = agile_modbus_serialize_read_input_registers(ctx, 0x34, 2);
                int read_len = rs485_send_then_recv(hinst, ctx->send_buf, send_len, ctx->read_buf, ctx->read_bufsz);

                int rc = agile_modbus_deserialize_read_registers(ctx, read_len, input_register);
                
                messageStack.can_msg.dataBytes[0] = input_register[0];
                messageStack.can_msg.dataBytes[1] = input_register[0] >> 8;
                rt_mq_send(&can_tx_queue, messageStack.data, MESSAGE_SIZE);
                rt_thread_delay(1); 
                continue;
            }

            data_stack.data_32t = 0;
        }
    }
}

/* 
 * @brief 自定义Modbus相关初始化函数
 * */
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
    rs485_config(hinst, MODBUS_BAUD_RATE, 8, 0, 1);

    rs485_set_recv_tmo(hinst, 500);
    if (rs485_connect(hinst) != RT_EOK)
    {
        rs485_destory(hinst);
        // rt_kprintf("rs485 connect fail.");
        LOG_E("rs485 connect fail.");
        return;
    }

    rt_thread_t tid1 = RT_NULL, tid2 = RT_NULL;

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

/*
* @brief 利用aglie_modbus和485协议发送功能码06的报文，并接收返回以判断正确性
* 
* @param slave_addr 从站地址
* @param reg_addr 寄存器地址
* @param val 写入值
* 
* @return 写入结果：0为成功，否则失败
* */
int modbus_send_till_recv(uint16_t slave_addr, uint16_t reg_addr, uint16_t val)
{
    int sendLen,recLen;
    
    rt_thread_delay(1); // 延时1ms，确保线程有时间调度
    // rt_sem_take(modbus_signal, 2000); //信号量获取
    agile_modbus_set_slave(ctx, slave_addr);
    
    //modbus发送并接收
    sendLen = agile_modbus_serialize_write_register(ctx, reg_addr, val);
    recLen = rs485_send_then_recv(hinst, ctx->send_buf, sendLen, ctx->read_buf, ctx->read_bufsz);
    
    // rt_sem_release(modbus_signal); // 信号量释放

    // 根据接收结果判断是否通信成功
    if (recLen <= 0)
    {
        return 1;
    }

    return 0;
}

/*
* @brief 读取modbus特定从站、寄存器的16位整型
* 
* @param slave_addr 从站地址
* @param reg_addr 寄存器地址
* @param 8stack 接收读取值变量的指针
* */
uint16_t modbus_read16t_till_recv(uint16_t slave_addr, uint16_t reg_addr)
{
    // 从站ID为dataBytes[0]
    // rt_sem_take(modbus_signal, 2000);
    agile_modbus_set_slave(ctx, slave_addr);
                
    int sendLen = agile_modbus_serialize_read_registers(ctx, reg_addr, 2);
    int recLen = rs485_send_then_recv(hinst, ctx->send_buf, sendLen, ctx->read_buf, ctx->read_bufsz);
    
    if (recLen < 0)
    {
        LOG_E("Recieve error");
        return 1;
    }
    
    DataStack data_stack;
    int rc = agile_modbus_deserialize_read_registers(ctx, recLen, &data_stack.data_16t[0]);
    uint16_t stack = data_stack.data_16t[0];
    // rt_sem_release(modbus_signal);
    return stack;
}

