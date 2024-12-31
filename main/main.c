#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "driver/gpio.h"
#include "esp_timer.h"

#include "bt_srv.h"
#include "gatt_svc.h"
#include "ds3231_state.h"
#include "ds3231_srv.h"
#include "common.h"

#define GPIO_D4 GPIO_NUM_12
#define GPIO_D5 GPIO_NUM_13

esp_err_t gpio_init()
{
  gpio_config_t io_conf = {
      .intr_type = GPIO_INTR_DISABLE,
      .mode = GPIO_MODE_OUTPUT,
      .pin_bit_mask = (1ULL << GPIO_D4) | (1ULL << GPIO_D5),
      .pull_down_en = 1,
      .pull_up_en = 0};

  return gpio_config(&io_conf);
}

void rtc_read_task(void *params)
{
  i2c_dev_t *dev = get_dev();

  struct tm time = {
      .tm_year = 0,
      .tm_mon = 0,
      .tm_mday = 0,
      .tm_hour = 0,
      .tm_min = 0,
      .tm_sec = 0,
      .tm_wday = 0,
      .tm_yday = 0,
      .tm_isdst = 0};
  float temperature = -100.0f;

  while (1)
  {
    struct tm new_time;
    float new_temp;

    ds3231_get_time(dev, &new_time);
    ds3231_get_temp_float(dev, &new_temp);

    const bool changed = memcmp(&time, &new_time, sizeof(struct tm)) != 0 || temperature != new_temp;

    if (changed)
    {
      set_current_time(&new_time);
      set_temperature(new_temp);

      send_current_time_notification();
      send_temperature_notification();

      gpio_set_level(GPIO_D4, new_time.tm_sec % 2);
      gpio_set_level(GPIO_D5, abs((new_time.tm_sec % 2) - 1));
    }

    time = new_time;
    temperature = new_temp;

    vTaskDelay(pdMS_TO_TICKS(100));
  }

  vTaskDelete(NULL);
}

void print_task(void *params)
{
  while (1)
  {
    struct tm *current_time = get_current_time();

    if (current_time->tm_sec % 60 == 0)
      ESP_LOGI("Current time", "%02d:%02d:%02d", current_time->tm_hour, current_time->tm_min, current_time->tm_sec);

    vTaskDelay(pdMS_TO_TICKS(1000));
  }

  vTaskDelete(NULL);
}

void app_main()
{
  ESP_ERROR_CHECK(gpio_init());
  ESP_ERROR_CHECK(i2cdev_init());
  ESP_ERROR_CHECK(ds3231_init());
  bt_init();

  xTaskCreate(rtc_read_task, "RTCRead", configMINIMAL_STACK_SIZE * 3, NULL, 5, NULL);
  xTaskCreate(print_task, "Print", configMINIMAL_STACK_SIZE * 3, NULL, 5, NULL);
  xTaskCreate(nimble_host_task, "NimbleHost", configMINIMAL_STACK_SIZE * 3, NULL, 5, NULL);
}