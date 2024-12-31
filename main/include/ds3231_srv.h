#pragma once

#include <string.h>

#include "driver/i2c_master.h"
#include "ds3231.h"

#define DS3231_SDA GPIO_NUM_18
#define DS3231_SCL GPIO_NUM_19

i2c_dev_t *get_dev();

esp_err_t ds3231_init();