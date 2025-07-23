/** 
 * @FilePath     \project\applications\Inc\protcol.h
 * @Author       wunna
 * Version       V1.0.0
 * @Date         2025-04-03 09:57:33
 * @Description  
 * 
 * LastEditTime  2025-04-18 09:28:24
 */
#ifndef __PROTCOL_H__
#define __PROTCOL_H__

#include "stdint.h"

// CAN消息类型
typedef enum {
    MSG_BOARDCAST = 0,        // 广播消息
    MSG_HEARTBEAT,            // 心跳消息
    MSG_TEMP_MEASURE,         // 温度测量结果消息
    MSG_TEMP_MEASURE_ERROR,   // 温度测量错误
    MSG_HIGH_SPEED_COUNTER,   // 高速脉冲计数消息
    MSG_RELAY_STATUS,         // 继电器状态消息
    MSG_ANALOG_IN,            // 模拟输入消息
    MSG_ANALOG_OUT,           // 模拟输出消息
    MSG_DIGITAL_IN,           // 数字输入消息
    MSG_DIGITAL_OUT,          // 数字输出消息
    MSG_SSR_STATE,            // 固态继电器状态消息
    MSG_RELAY_CTRL,           // 继电器控制消息
    MSG_MOTOR_CTRL,           // 伺服电机控制消息
    MSG_TEMP_CTRL,            // 温度控制板消息
    MSG_RS485,                // R485 消息
    MSG_FAN_CTRL,             // 风机转速控制
    MSG_CHAIN_CTRL,           // 传输链条控制
    MSG_CHAIN_RUNNING_STATUS, // 传输链条运行状态
    MSG_TEMP_WARN,            // 温度警告

    MSG_MODBUS_MOTOR_REG6000, // 模式选择
    MSG_MODBUS_MOTOR_REG6001, // 目标速度
    MSG_MODBUS_MOTOR_REG6003, // 回零模式
    MSG_MODBUS_MOTOR_REG6004, // 加速度
    MSG_MODBUS_MOTOR_REG6006, // 减速度
    MSG_MODBUS_MOTOR_REG6008, // 目标位置
    MSG_MODBUS_MOTOR_REG6010, // 定位目标速度

    MSG_MODBUS_MOTOR_REG5000, // 实时速度
    MSG_MODBUS_MOTOR_REG5002, // 实时位置
    MSG_MODBUS_MOTOR_REG5004, // 状态
    MSG_MODBUS_MOTOR_REG5005, // 错误代码
    MSG_TCM_PWM_DIRECT_OUT,   //控温直接输出

    MSG_MODBUS_POWER_READ_VOLTAGE, //获取电源输出电压
    MSG_MODBUS_POWER_READ_CURRENT, //获取电源输出电流
    MSG_MODBUS_POWER_READ_POWER,   //获取电源输出功率
    MSG_PID_OUT_REPORT,            //PID上报检测

    MSG_HD_FAN_CTRL = 101,   // hd风扇控制
    MSG_HD_CHAIN_CTRL = 102, // hd链速控制
    MSG_DEK_CHAIN_CTRL = 103, // 德恩科链速控制

    MSG_LS_CHAIN_CTRL = 104, // 雷赛电机控制

} CanMsgType;

typedef union {
  uint8_t data[8];

  struct CAN_MSG_DATA {
    uint8_t msgType;
    uint8_t addr;
    uint8_t flag;
    uint8_t index;
    uint8_t dataBytes[4];
  } can_msg;
}CAN_DATA;

typedef struct
{
  float target_value;
  float p_value;
  float i_value;
  float d_value;
}CHAIN_CONTROL_DATA;




#endif // !__PROTCOL_H__
