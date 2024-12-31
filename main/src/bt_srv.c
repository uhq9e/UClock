#include "common.h"
#include "gap.h"
#include "gatt_svc.h"
#include "bt_srv.h"

void ble_store_config_init(void);

static void on_stack_reset(int reason)
{
  ESP_LOGI(TAG, "nimble stack reset, reset reason: %d", reason);
}

static void on_stack_sync(void)
{
  adv_init();
}

static void nimble_host_config_init(void)
{
  ble_hs_cfg.reset_cb = on_stack_reset;
  ble_hs_cfg.sync_cb = on_stack_sync;
  ble_hs_cfg.gatts_register_cb = gatt_svr_register_cb_;
  ble_hs_cfg.store_status_cb = ble_store_util_status_rr;

  ble_store_config_init();
}

void nimble_host_task(void *param)
{
  ESP_LOGI(TAG, "nimble host task has been started!");

  nimble_port_run();

  vTaskDelete(NULL);
}

void bt_init()
{
  int rc;
  esp_err_t ret;

  ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
      ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
  {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  if (ret != ESP_OK)
  {
    ESP_LOGE(TAG, "failed to initialize nvs flash, error code: %d ", ret);
    return;
  }

  ret = nimble_port_init();
  if (ret != ESP_OK)
  {
    ESP_LOGE(TAG, "failed to initialize nimble stack, error code: %d ",
             ret);
    return;
  }

  rc = gap_init();
  if (rc != 0)
  {
    ESP_LOGE(TAG, "failed to initialize GAP service, error code: %d", rc);
    return;
  }

  rc = gatt_svc_init();
  if (rc != 0)
  {
    ESP_LOGE(TAG, "failed to initialize GATT server, error code: %d", rc);
    return;
  }

  nimble_host_config_init();
}