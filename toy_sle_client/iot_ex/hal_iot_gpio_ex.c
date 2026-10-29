/*
 * Copyright (c) 2022 HiSilicon (Shanghai) Technologies CO., LIMITED.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "iot_errno.h"
#include "iot_gpio_ex.h"
#if CHIP_WS63
#include "pinctrl.h"
#include "los_base.h"
#else
#include "hi_gpio.h"
#include "hi_io.h"
#include "hi_task.h"
#include "hi_types_base.h"
#endif

#if CHIP_WS63
unsigned int IoSetPull(unsigned int id, IotIoPull val)
{
    if (id >= PIN_NONE) {
        return IOT_FAILURE;
    }
    return uapi_pin_set_pull((pin_t)id, (pin_pull_t)val);
}

unsigned int IoSetFunc(unsigned int id, unsigned char val)
{
    if (id >= PIN_NONE) {
        return IOT_FAILURE;
    }
    return uapi_pin_set_mode((pin_t)id, val);
}

unsigned int TaskMsleep(unsigned int ms)
{
    if (ms <= 0) {
        return IOT_FAILURE;
    }
    
    LOS_Msleep((unsigned int)ms);

    return IOT_SUCCESS;
}

#else

unsigned int IoSetPull(unsigned int id, IotIoPull val)
{
    if (id >= HI_GPIO_IDX_MAX) {
        return IOT_FAILURE;
    }
    return hi_io_set_pull((hi_io_name)id, (hi_io_pull)val);
}

unsigned int IoSetFunc(unsigned int id, unsigned char val)
{
    if (id >= HI_GPIO_IDX_MAX) {
        return IOT_FAILURE;
    }
    return hi_io_set_func((hi_io_name)id, val);
}

unsigned int TaskMsleep(unsigned int ms)
{
    if (ms <= 0) {
        return IOT_FAILURE;
    }
    return hi_sleep((hi_u32)ms);
}
#endif