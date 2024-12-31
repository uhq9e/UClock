#include "ds3231_srv.h"

static i2c_dev_t ds3231_dev;

i2c_dev_t *get_dev()
{
  return &ds3231_dev;
}

esp_err_t ds3231_init()
{
  memset(&ds3231_dev, 0, sizeof(i2c_dev_t));
  return ds3231_init_desc(&ds3231_dev, I2C_NUM_0, DS3231_SDA, DS3231_SCL);
}