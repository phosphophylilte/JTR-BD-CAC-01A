/** 
 * @FilePath     \project\applications\user_can.c
 * @Author       wunna
 * Version       V1.0.0
 * @Date         2025-04-03 11:51:28
 * @Description  
 * 
 * LastEditTime  2025-04-18 12:06:26
 */
#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include "protcol.h"
#include "userconfig.h"

#define CAN_DEV_NAME       "can1"      /* CAN 设备名称 */

static struct rt_semaphore rx_sem;     /* 用于接收消息的信号量 */
static rt_device_t can_dev;            /* CAN 设备句柄 */

/** 
 * @author wunna
 * @brief Recall Function of CAN Recieve
 * @retval rt_err_t Judge function rutine Success or False 
 * @param dev device name
 * @param size device message size
 */
static rt_err_t can_rx_call(rt_device_t dev, rt_size_t size)
{
    rt_sem_release(&rx_sem);
    return RT_EOK;
}

/** 
 * @author wunna
 * @brief Funtion of CAN Rx thread
 * @retval NULL
 * @param parameter NULL
 */
static void can_rx_thread(void *parameter)
{
    int i;
    rt_err_t res;
    struct rt_can_msg rxmsg = {0};
    uint8_t canRxBuffer[8] = {0};

    while (1)
    {
        /* 阻塞等待接收信号量 */
        rt_sem_take(&rx_sem, RT_WAITING_FOREVER);
        
        /* 从 CAN 读取一帧数据 */
        rt_device_read(can_dev, 0, &rxmsg, sizeof(rxmsg));
        
        /* 将读取的信息放入消息队列can_rx_queue */
        if (rxmsg.data[0] == MSG_CHAIN_CTRL || rxmsg.data[0] == MSG_FAN_CTRL)
        {
            rt_mq_send(&can_rx_queue, rxmsg.data, MESSAGE_SIZE);
        }
        
        /* 延时1ms，减小仲裁负担 */
        rt_thread_mdelay(100);
    }
}

/** 
 * @author wunna
 * @brief Funtion of CAN Tx thread, if can_tx_queue not empty, send the message in queue
 * @retval NULL
 * @param parameter NULL
 */
static void can_tx_thread(void *parameter)
{
    rt_err_t res;
    struct rt_can_msg txmsg = {0};
    // rt_int8_t canTxBuffer[8];
    CAN_DATA canTxBuffer;

    while (1)
    {
        res = rt_mq_recv(&can_tx_queue, &canTxBuffer, MESSAGE_SIZE, RT_WAITING_FOREVER);
        if(res)
        {
            txmsg.id = 0x00;              /* ID 为 0x00 */
            txmsg.ide = RT_CAN_STDID;     /* 标准格式 */
            txmsg.rtr = RT_CAN_DTR;       /* 数据帧 */
            txmsg.rsv = 0;
            txmsg.rxfifo = 0;
            txmsg.hdr_index = 0;
            txmsg.len = 8;                /* 数据长度为 8 */
            
            /* 待发送的 8 字节数据 */
            for (int i = 0; i < 8; i++)
            {
                txmsg.data[i] = canTxBuffer.data[i];
            }
            
            rt_device_write(can_dev, 0, &txmsg, sizeof(txmsg));
        }

        rt_thread_mdelay(100);
    }
    
}

/** 
 * @author wunna
 * @brief Initialize Function of CAN Device
 * @retval NULL
 * @param NULL
 */
void userCANInitialize()
{
    struct rt_can_msg msg = {0};
    rt_err_t res;
    rt_size_t  size;
    rt_thread_t thread;
    rt_thread_t thread2;
    char can_name[RT_NAME_MAX];
    // rt_strncpy(can_name, CAN_DEV_NAME, RT_NAME_MAX);

    /* 查找 CAN 设备 */
    can_dev = rt_device_find(CAN_DEV_NAME);
    if (!can_dev)
    {
        LOG_E("find %s failed!\n", can_name);
        return RT_ERROR;
    }
    else
    {
        LOG_I("find %s!\n", CAN_DEV_NAME);
    }
   

    /* 初始化 CAN 接收信号量 */
    rt_sem_init(&rx_sem, "rx_sem", 0, RT_IPC_FLAG_FIFO);

    /* 以中断接收及发送方式打开 CAN 设备 */
    res = rt_device_open(can_dev, RT_DEVICE_FLAG_INT_TX | RT_DEVICE_FLAG_INT_RX);
    RT_ASSERT(res == RT_EOK);

    /* 设置接收回调函数 */
    rt_device_set_rx_indicate(can_dev, can_rx_call);
    
    /* 创建数据接收线程 */
    thread = rt_thread_create("can_rx", can_rx_thread, RT_NULL, 512, 25, 10);
    if (thread != RT_NULL)
    {
        rt_thread_startup(thread);
    }
    else
    {
        LOG_E("create can_rx thread failed!\n");
        return -1;
    }

    /* 创建数据发送线程 */
    thread2 = rt_thread_create("can_tx", can_tx_thread, RT_NULL, 512, 10, 10);
    if (thread2 != RT_NULL)
    {
        rt_thread_startup(thread2);
    }
    else
    {
        LOG_E("create can_tx thread failed!\n");
        return -1;
    }

    LOG_I("CAN Device is Init Success\r\n");
    return res;

}
