/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-01-26     armink       the first version
 */

#include <fal.h>
#include "drv_flash.h"



static int stm32_flash_init(void)
{
    // Do nothing here
}

const struct fal_flash_dev stm32f4_onchip_flash =
{
    .name       = "stm32_onchip",
    .addr       = 0x08000000,
    .len        = 1024*1024,
    .blk_size   = 128*1024,
    // Four function defined in libraries\HAL_Drivers\drivers\drv_flash\drv_flash_f4.c
    .ops        = {stm32_flash_init, stm32_flash_read, stm32_flash_write, stm32_flash_erase},
    .write_gran = 8
};

