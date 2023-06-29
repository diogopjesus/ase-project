#include <string.h>

#include "esp_log.h"

#include "app_rmaker.h"

void rmaker_add_auto_watering_switch(esp_rmaker_node_t *node, void *auto_watering_write_cb);
void rmaker_add_current_moisture(esp_rmaker_node_t *node, void *current_moisture_cb);
void rmaker_add_manual_watering(esp_rmaker_node_t *node, void *manual_watering_cb);

static const char *TAG = "ASE-PROJECT-RMAKER";

void rmaker_init(void *auto_watering_write_cb, void *current_moisture_cb, void *manual_watering_cb)
{
    esp_err_t err;

    /* Initialize Wi-Fi. Note that, this should be called before esp_rmaker_init() */
    app_wifi_init();

    /* Initialize the ESP RainMaker Agent.
     * Note that this should be called after app_wifi_init() but before app_wifi_start()
     * */
    esp_rmaker_config_t rainmakerCfg = {
        .enable_time_sync = true,
    };
    esp_rmaker_node_t *node = esp_rmaker_node_init(&rainmakerCfg, "ESP RainMaker Device", "ASE Project");
    if (!node)
    {
        ESP_LOGE(TAG, "Could not initialise node. Aborting!!!");
        vTaskDelay(5000 / portTICK_PERIOD_MS);
        abort();
    }

    rmaker_add_auto_watering_switch(node, auto_watering_write_cb);

    rmaker_add_current_moisture(node, current_moisture_cb);

    rmaker_add_manual_watering(node, manual_watering_cb);

    /* Enable OTA */
    esp_rmaker_ota_config_t ota_config = {
        .server_cert = ESP_RMAKER_OTA_DEFAULT_SERVER_CERT,
    };
    esp_rmaker_ota_enable(&ota_config, OTA_USING_TOPICS);

    /* Start the ESP RainMaker Agent */
    esp_rmaker_start();

    /* Start the Wi-Fi.
     * If the node is provisioned, it will start connection attempts,
     * else, it will start Wi-Fi provisioning. The function will return
     * after a connection has been successfully established
     */
    err = app_wifi_start(POP_TYPE_RANDOM);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Could not start Wifi. Aborting!!!");
        vTaskDelay(5000 / portTICK_PERIOD_MS);
        abort();
    }

    /* Wait until sntp time is defined */
    if (esp_rmaker_time_wait_for_sync(pdMS_TO_TICKS(40000)) != ESP_OK)
    {
        ESP_LOGW(TAG, "Failed to update system time within 40s timeout! Proceeding with wrong time!");
    }

    /* Set timezone */
    setenv("TZ", "WET0WEST,M3.5.0/1,M10.5.0", 1);
    tzset();
}

void rmaker_add_auto_watering_switch(esp_rmaker_node_t *node, void *auto_watering_write_cb)
{
    autoWateringSwitchDevice = esp_rmaker_device_create("Auto Watering", ESP_RMAKER_DEVICE_SWITCH, NULL);

    esp_rmaker_device_add_cb(autoWateringSwitchDevice, auto_watering_write_cb, NULL);

    esp_rmaker_device_add_param(autoWateringSwitchDevice, esp_rmaker_name_param_create("Name", "Auto Watering"));

    esp_rmaker_param_t *powerParam = esp_rmaker_power_param_create(ESP_RMAKER_DEF_POWER_NAME, DEFAULT_AUTO_WATERING_POWER);
    esp_rmaker_device_add_param(autoWateringSwitchDevice, powerParam);
    esp_rmaker_device_assign_primary_param(autoWateringSwitchDevice, powerParam);

    esp_rmaker_node_add_device(node, autoWateringSwitchDevice);
}

void rmaker_add_current_moisture(esp_rmaker_node_t *node, void *current_moisture_cb)
{
    currentMoistureInfoDevice = esp_rmaker_device_create("Current Moisture", NULL, NULL);

    esp_rmaker_device_add_cb(currentMoistureInfoDevice, current_moisture_cb, NULL);

    esp_rmaker_device_add_param(currentMoistureInfoDevice, esp_rmaker_name_param_create("name", "Moisture Sensor"));

    esp_rmaker_param_t *moistureParam = esp_rmaker_param_create("Moisture (%)", "MoistureSensor", esp_rmaker_int(0), PROP_FLAG_READ | PROP_FLAG_TIME_SERIES);
    esp_rmaker_param_add_ui_type(moistureParam, ESP_RMAKER_UI_TEXT);
    esp_rmaker_device_add_param(currentMoistureInfoDevice, moistureParam);
    esp_rmaker_device_assign_primary_param(currentMoistureInfoDevice, moistureParam);

    esp_rmaker_param_t *triggerParam = esp_rmaker_param_create("trigger reading", "TriggerReading", esp_rmaker_bool(false), PROP_FLAG_READ | PROP_FLAG_WRITE);
    esp_rmaker_param_add_ui_type(triggerParam, ESP_RMAKER_UI_TRIGGER);
    esp_rmaker_device_add_param(currentMoistureInfoDevice, triggerParam);

    esp_rmaker_node_add_device(node, currentMoistureInfoDevice);
}

void rmaker_add_manual_watering(esp_rmaker_node_t *node, void *manual_watering_cb)
{
    manualWateringDevice = esp_rmaker_device_create("Manual Watering", NULL, NULL);

    esp_rmaker_device_add_cb(manualWateringDevice, manual_watering_cb, NULL);

    esp_rmaker_device_add_param(manualWateringDevice, esp_rmaker_name_param_create("name", "Watering"));

    esp_rmaker_param_t *statusParam = esp_rmaker_param_create("status", "Status", esp_rmaker_str("Disabled"), PROP_FLAG_READ);
    esp_rmaker_param_add_ui_type(statusParam, ESP_RMAKER_UI_TEXT);
    esp_rmaker_device_add_param(manualWateringDevice, statusParam);
    esp_rmaker_device_assign_primary_param(manualWateringDevice, statusParam);

    esp_rmaker_param_t *timeParam = esp_rmaker_param_create("time irrigating (seconds)", "Time", esp_rmaker_int(5), PROP_FLAG_READ | PROP_FLAG_WRITE);
    esp_rmaker_param_add_ui_type(timeParam, ESP_RMAKER_UI_SLIDER);
    esp_rmaker_param_add_bounds(timeParam, esp_rmaker_int(5), esp_rmaker_int(100), esp_rmaker_int(1));
    esp_rmaker_device_add_param(manualWateringDevice, timeParam);

    esp_rmaker_param_t *triggerParam = esp_rmaker_param_create("trigger pump", "Trigger", esp_rmaker_bool(false), PROP_FLAG_READ | PROP_FLAG_WRITE);
    esp_rmaker_param_add_ui_type(triggerParam, ESP_RMAKER_UI_TRIGGER);
    esp_rmaker_device_add_param(manualWateringDevice, triggerParam);

    esp_rmaker_node_add_device(node, manualWateringDevice);
}

void rmaker_update_moisture(uint8_t value)
{
    esp_rmaker_param_update_and_report(esp_rmaker_device_get_param_by_type(currentMoistureInfoDevice, "MoistureSensor"), esp_rmaker_int(value));
}

void rmaker_update_watering_status(bool watering)
{
    if (watering)
    {
        esp_rmaker_param_update_and_report(esp_rmaker_device_get_param_by_type(manualWateringDevice, "Status"), esp_rmaker_str("Watering the plant..."));
        rmaker_warn_user("Watering the plant...");
    }
    else
    {
        esp_rmaker_param_update_and_report(esp_rmaker_device_get_param_by_type(manualWateringDevice, "Status"), esp_rmaker_str("Disabled"));
    }
}

void rmaker_warn_user(char *str)
{
    ESP_ERROR_CHECK(esp_rmaker_raise_alert(str));
}

void rmaker_get_watering_status(char *status)
{
    esp_rmaker_param_val_t *val = esp_rmaker_param_get_val(esp_rmaker_device_get_param_by_type(currentMoistureInfoDevice, "Status"));
    strcpy(status, (val->val).s);
}