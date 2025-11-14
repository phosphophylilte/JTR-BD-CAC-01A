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
#include <modbus_to_can_handle.h>

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
    CAN_DATA rx;

    while (1)
    {
        if (can_rx_queue.entry == 0)
        {
            continue;
        }

        rt_mq_recv(&can_rx_queue, rx.data, MESSAGE_SIZE, RT_WAITING_FOREVER);

        /* 研蓝电机写 6000 寄存器 */
        if (rx.can_msg.msgType == MSG_MODBUS_MOTOR_REG6000 && rx.can_msg.flag == 0)
        {
            handle_motor6000_write(&rx);
            continue;
        }

        /* LS 电机 */
        if (rx.can_msg.msgType == MSG_LS_CHAIN_CTRL)
        {
            if (rx.can_msg.flag == 1)
            {
                /* 读取 */
                handle_ls_read(&rx);
            }
            else
            {
                /* 写入 */
                handle_ls_write(&rx);
            }
            continue;
        }

        /* 电源读数 */
        if (rx.can_msg.flag == 1)
        {
            switch (rx.can_msg.msgType)
            {
            case MSG_MODBUS_POWER_READ_CURRENT:  /* 电流 */
                handle_power_read(&rx, 0x33);
                break;

            case MSG_MODBUS_POWER_READ_VOLTAGE:  /* 电压 */
                handle_power_read(&rx, 0x32);
                break;

            case MSG_MODBUS_POWER_READ_POWER:    /* 功率 */
                handle_power_read(&rx, 0x34);
                break;

            default:
                break;
            }

            continue;
        }

        /* 兜底 */
        /* 如果还有其他 msgType，以后可以在这里再扩展 */
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
* @brief 利用aglie_modbus和485协议发送功能码06的报文，并接收返回以判断正确性
* 
* @param slave_addr 从站地址
* @param reg_addr 寄存器地址
* @param val 写入值
* 
* @return 写入结果：0为成功，否则失败
* */
int modbus_send_32t_till_recv(uint16_t slave_addr, uint16_t reg_addr, int val)
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

