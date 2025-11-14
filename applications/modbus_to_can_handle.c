#include <modbus_to_can_handle.h>


/*=================== 辅助函数实现 ===================*/

/* 研蓝电机————设置（写操作） Reg 0x6000 */
void handle_motor6000_write(const CAN_DATA *rx)
{
    DataStack ds = {0};

    /* 原代码这里的 memcpy 第二个参数少了 &，这里修正 */
    memcpy(&ds.data_16t[0], &rx->can_msg.dataBytes[0], 2);

    /* 原代码用 index 做从站地址，这里保持原逻辑 */
    (void)modbus_send_till_recv(rx->can_msg.index, 0x6000, ds.data_16t[0]);

    /* 如果需要写成功后回 CAN，可以在这里补发消息 */
}

/* LS 电机 - 读操作（flag == 1） */
void handle_ls_read(const CAN_DATA *rx)
{
    DataStack ds = {0};
    uint16_t slaveAddr = rx->can_msg.addr;
    CAN_DATA tx;

    memset(&tx, 0, sizeof(tx));

    tx.can_msg.addr   = slaveAddr;
    tx.can_msg.flag   = 1;
    tx.can_msg.msgType= MSG_LS_CHAIN_CTRL;
    tx.can_msg.index  = rx->can_msg.index;

    switch (rx->can_msg.index)
    {
    case 5: /* 读取当前位置（单位：转） */
        ds.data_16t[0] = modbus_read16t_till_recv(slaveAddr, 0x0B18);
        ds.data_16t[1] = modbus_read16t_till_recv(slaveAddr, 0x0B19);
        memcpy(tx.can_msg.dataBytes, ds.val, 4);
        break;

    case 6: /* 读取当前转速（单位：rpm） */
        ds.data_16t[0] = modbus_read16t_till_recv(slaveAddr, 0x0B09);
        memcpy(tx.can_msg.dataBytes, ds.val, 4);
        break;

    case 7: /* 读取实时状态 */
        ds.data_16t[0] = modbus_read16t_till_recv(slaveAddr, 0x0B05);
        memcpy(tx.can_msg.dataBytes, ds.val, 4);
        break;

    case 8: /* 报警读取(屏蔽高四位) */
        ds.data_16t[0] = modbus_read16t_till_recv(slaveAddr, 0x2203);
        ds.data_16t[0] &= 0x0FFF;
        memcpy(tx.can_msg.dataBytes, ds.val, 4);
        break;

    default:
        /* 不支持的 index，不回消息 */
        return;
    }

    rt_mq_send(&can_tx_queue, tx.data, MESSAGE_SIZE);
}

/* LS 电机 - 写操作（flag == 0） */
void handle_ls_write(const CAN_DATA *rx)
{
    DataStack ds = {0};
    uint16_t addr = rx->can_msg.addr;
    uint16_t mode      = 0;
    uint16_t zero_mode = 0;
    int sendFlag = -1;

    switch (rx->can_msg.index)
    {
    case 0: /* 电机控制 */
        memcpy(&mode, &rx->can_msg.dataBytes[0], 2);

        if (mode == 2)      /* 启动电机 */
        {
            sendFlag = modbus_send_till_recv(addr, 0x6002, 0x0010);
        }
        else if (mode == 8) /* 停止电机 */
        {
            sendFlag = modbus_send_till_recv(addr, 0x6002, 0x0040);
        }
        else if (mode == 4) /* 回零启动 */
        {
            sendFlag = modbus_send_till_recv(addr, 0x6002, 0x0020);
        }
        break;

    case 1: /* 设置模式 */
        memcpy(&mode, &rx->can_msg.dataBytes[0], 2);

        if (mode == 0)      /* 位置模式 */
        {
            sendFlag = modbus_send_till_recv(addr, 0x6200, 0x0001);
        }
        else if (mode == 1) /* 速度模式 */
        {
            sendFlag = modbus_send_till_recv(addr, 0x6200, 0x0002);
        }
        break;              /* ★ 原代码这里缺 break，已修复 */

    case 2: /* 设置回零模式 */
        zero_mode = rx->can_msg.dataBytes[0];

        if (zero_mode == 1)
        {
            sendFlag = modbus_send_till_recv(addr, 0x600A, 0x0005);
        }
        else if (zero_mode == 2)
        {
            sendFlag = modbus_send_till_recv(addr, 0x600A, 0x0004);
        }
        break;

    case 3: /* 设置加速度 */
        memcpy(&ds.data_16t[0], &rx->can_msg.dataBytes[0], 2);
        sendFlag = modbus_send_till_recv(addr, 0x620C, ds.data_16t[0]);
        break;              /* ★ 原代码这里缺 break，已修复 */

    case 4: /* 设置减速度 */
        memcpy(&ds.data_16t[0], &rx->can_msg.dataBytes[0], 2);
        sendFlag = modbus_send_till_recv(addr, 0x620D, ds.data_16t[0]);
        break;              /* ★ 原代码这里缺 break，已修复 */

    case 5: /* 设置电机位置 */
        memcpy(&ds.data_32t, rx->can_msg.dataBytes, 4);
        /* ds.data_32t *= 10000; */
        /* ds.data_32t = swap32Big2Little(ds.data_32t); */

        if (modbus_send_till_recv(addr, 0x6201, ds.data_16t[1]) == 0)
        {
            rt_thread_mdelay(10);
            sendFlag = modbus_send_till_recv(addr, 0x6202, ds.data_16t[0]);
        }
        break;

    case 6: /* 设置电机转速 */
        ds.data_8t[0] = rx->can_msg.dataBytes[0];
        ds.data_8t[1] = rx->can_msg.dataBytes[1];
        sendFlag = modbus_send_till_recv(addr, 0x6203, (uint16_t)ds.data_16t[0]);
        break;

    default:
        break;
    }

    /* 若 Modbus 写操作成功，将 CAN 报文原样返回 */
    if (sendFlag == 0)
    {
        rt_mq_send(&can_tx_queue, (void *)rx->data, MESSAGE_SIZE);
    }
}

/* 电源读数：电流/电压/功率 公共处理函数 */
void handle_power_read(const CAN_DATA *rx, uint16_t reg)
{
    rt_thread_delay(1); /* 延时1ms，确保线程有时间调度 */

    rt_uint16_t input_register[1] = {0};

    /* 原代码使用 index 作为从站地址，这里保持一致 */
    agile_modbus_set_slave(ctx, rx->can_msg.index);

    int send_len = agile_modbus_serialize_read_input_registers(ctx, reg, 2);
    int read_len = rs485_send_then_recv(hinst,
                                        ctx->send_buf, send_len,
                                        ctx->read_buf, ctx->read_bufsz);

    int rc = agile_modbus_deserialize_read_registers(ctx, read_len, input_register);
    (void)rc; /* 如果暂时不用 rc，就先丢弃，免得告警 */

    CAN_DATA tx = *rx;  /* 基于原消息构造返回 */

    /* 按照原逻辑：低字节先，高字节后 */
    tx.can_msg.dataBytes[0] = (uint8_t)(input_register[0] & 0x00FF);
    tx.can_msg.dataBytes[1] = (uint8_t)(input_register[0] >> 8);

    rt_mq_send(&can_tx_queue, tx.data, MESSAGE_SIZE);
    rt_thread_delay(1);
}
