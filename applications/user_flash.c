/** 
 * @FilePath     \project\applications\user_flash.c
 * @Author       wunna
 * Version       V1.0.0
 * @Date         2025-04-16 08:54:16
 * @Description  
 * 
 * LastEditTime  2025-04-21 11:19:53
 */
#include <rtthread.h>
#include <rtdevice.h>
#include <fal.h>
#include <fal_cfg.h>
#include "protcol.h"
#include "userconfig.h"
#include "drv_flash.h"

CHAIN_CONTROL_DATA DATA1;
CHAIN_CONTROL_DATA DATA2;

aaaaaaaaaaa
int test_fal(void)
{
    __aligned(1);
    DATA1.target_value = 1000.00;
    DATA1.p_value = 100.0;
    DATA1.i_value = 2.0;
    DATA1.d_value = 1.0;
    DATA2.target_value = 1000.00;
    DATA2.p_value = 100.0;
    DATA2.i_value = 2.0;
    DATA2.d_value = 1.0;
    stm32_flash_erase(FLASH_BASE_ADDRESS, sizeof(DATA2) + sizeof(DATA1));
    stm32_flash_write(FLASH_BASE_ADDRESS, &DATA1, sizeof(DATA1));
    stm32_flash_write(FLASH_BASE_ADDRESS + sizeof(DATA1), &DATA2, sizeof(DATA2));
    CHAIN_CONTROL_DATA stack2;
    stm32_flash_read(FLASH_BASE_ADDRESS + sizeof(DATA1),&stack2, sizeof(DATA2));
    LOG_I("value is %.4f, p is %.4f, i is %.4f, d is %.4f", stack2.target_value, stack2.p_value, stack2.i_value, stack2.d_value);
    
}
MSH_CMD_EXPORT(test_fal, test fal);


