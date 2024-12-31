#include "common.h"
#include "gatt_svc.h"
#include "ds3231_state.h"

static int current_time_chr_access(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg);
static int temperature_access(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg);

static int firmware_revision_access(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg);

/* Current Time Service */
static const ble_uuid16_t current_time_svc_uuid = BLE_UUID16_INIT(0x1805);

/* Current Time Characteristic */
static uint8_t current_time_chr_val[20];
static uint16_t current_time_chr_val_handle;
static const ble_uuid16_t current_time_chr_uuid = BLE_UUID16_INIT(0x2A2B);

static uint16_t current_time_chr_conn_handle = 0;
static bool current_time_chr_conn_handle_inited = false;
static bool current_time_notify_status = false;

/* Temperature Characteristic */
static uint8_t temperature_chr_val[5];
static uint16_t temperature_chr_val_handle;
static const ble_uuid16_t temperature_chr_uuid = BLE_UUID16_INIT(0x2A1C);

static uint16_t temperature_chr_conn_handle = 0;
static bool temperature_chr_conn_handle_inited = false;
static bool temperature_notify_status = false;

/* Device Information Service */
static const ble_uuid16_t device_info_svc_uuid = BLE_UUID16_INIT(0x180A);

/* Firmware Revision Characteristic */
static const ble_uuid16_t firmware_revision_uuid = BLE_UUID16_INIT(0x2A26);

static const struct ble_gatt_svc_def gatt_svr_svcs[] = {
    {.type = BLE_GATT_SVC_TYPE_PRIMARY,
     .uuid = &current_time_svc_uuid.u,
     .characteristics = (struct ble_gatt_chr_def[]){
         {
             .uuid = &current_time_chr_uuid.u,
             .access_cb = current_time_chr_access,
             .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_NOTIFY,
             .val_handle = &current_time_chr_val_handle,
         },
         {
             .uuid = &temperature_chr_uuid.u,
             .access_cb = temperature_access,
             .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
             .val_handle = &temperature_chr_val_handle,
         },
         {
             0,
         }}},
    {.type = BLE_GATT_SVC_TYPE_PRIMARY, .uuid = &device_info_svc_uuid.u, .characteristics = (struct ble_gatt_chr_def[]){{
                                                                                                                            .uuid = &firmware_revision_uuid.u,
                                                                                                                            .access_cb = firmware_revision_access,
                                                                                                                            .flags = BLE_GATT_CHR_F_READ,
                                                                                                                        },
                                                                                                                        {
                                                                                                                            0,
                                                                                                                        }}},
    {
        0,
    },
};

static int current_time_chr_access(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    int rc;

    switch (ctxt->op)
    {

    case BLE_GATT_ACCESS_OP_READ_CHR:
        if (attr_handle == current_time_chr_val_handle)
        {
            tm_to_current_time(current_time_chr_val, get_current_time());
            rc = os_mbuf_append(ctxt->om, &current_time_chr_val,
                                sizeof(current_time_chr_val));
            return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
        }

        goto error;

    case BLE_GATT_ACCESS_OP_WRITE_CHR:
        if (attr_handle == current_time_chr_val_handle)
        {
            if (ctxt->om->om_len == 10)
            {
                struct tm time;
                ds3231_set_time(get_dev(), set_current_time(current_time_to_tm(&time, ctxt->om->om_data)));
            }
            else
            {
                goto error;
            }
            return BLE_ATT_ERR_INSUFFICIENT_RES;
        }
        goto error;

    default:
        goto error;
    }

error:
    ESP_LOGE(
        TAG,
        "unexpected access operation to current time characteristic, opcode: %d",
        ctxt->op);
    return BLE_ATT_ERR_UNLIKELY;
}

static int temperature_access(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    encode_temperature(temperature_chr_val, get_temperature());
    return os_mbuf_append(ctxt->om, &temperature_chr_val, sizeof(temperature_chr_val));
}

static int firmware_revision_access(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    const char *revision = FIRMWARE_REVISION;
    return os_mbuf_append(ctxt->om, revision, strlen(revision));
}

void send_current_time_notification()
{
    if (current_time_notify_status && current_time_chr_conn_handle_inited)
    {
        ble_gatts_notify(current_time_chr_conn_handle, current_time_chr_val_handle);
    }
}

void send_temperature_notification()
{
    if (temperature_notify_status && temperature_chr_conn_handle_inited)
    {
        ble_gatts_notify(temperature_chr_conn_handle, temperature_chr_val_handle);
    }
}

void gatt_svr_register_cb_(struct ble_gatt_register_ctxt *ctxt, void *arg)
{
    char buf[BLE_UUID_STR_LEN];

    switch (ctxt->op)
    {

    case BLE_GATT_REGISTER_OP_SVC:
        ESP_LOGD(TAG, "registered service %s with handle=%d",
                 ble_uuid_to_str(ctxt->svc.svc_def->uuid, buf),
                 ctxt->svc.handle);
        break;

    case BLE_GATT_REGISTER_OP_CHR:
        ESP_LOGD(TAG,
                 "registering characteristic %s with "
                 "def_handle=%d val_handle=%d",
                 ble_uuid_to_str(ctxt->chr.chr_def->uuid, buf),
                 ctxt->chr.def_handle, ctxt->chr.val_handle);
        break;

    case BLE_GATT_REGISTER_OP_DSC:
        ESP_LOGD(TAG, "registering descriptor %s with handle=%d",
                 ble_uuid_to_str(ctxt->dsc.dsc_def->uuid, buf),
                 ctxt->dsc.handle);
        break;

    default:
        assert(0);
        break;
    }
}

void gatt_svr_subscribe_cb(struct ble_gap_event *event)
{
    if (event->subscribe.conn_handle != BLE_HS_CONN_HANDLE_NONE)
    {
        ESP_LOGI(TAG, "subscribe event; conn_handle=%d attr_handle=%d",
                 event->subscribe.conn_handle, event->subscribe.attr_handle);
    }
    else
    {
        ESP_LOGI(TAG, "subscribe by nimble stack; attr_handle=%d",
                 event->subscribe.attr_handle);
    }

    if (event->subscribe.attr_handle == current_time_chr_val_handle)
    {
        current_time_chr_conn_handle = event->subscribe.conn_handle;
        current_time_chr_conn_handle_inited = true;
        current_time_notify_status = event->subscribe.cur_notify;
    }
    else if (event->subscribe.attr_handle == temperature_chr_val_handle)
    {
        temperature_chr_conn_handle = event->subscribe.conn_handle;
        temperature_chr_conn_handle_inited = true;
        temperature_notify_status = event->subscribe.cur_notify;
    }
}

int gatt_svc_init(void)
{
    int rc;

    ble_svc_gatt_init();

    rc = ble_gatts_count_cfg(gatt_svr_svcs);
    if (rc != 0)
    {
        return rc;
    }

    rc = ble_gatts_add_svcs(gatt_svr_svcs);
    if (rc != 0)
    {
        return rc;
    }

    return 0;
}
