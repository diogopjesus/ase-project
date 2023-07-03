#include <stdio.h>
#include <string.h>
#include <time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "soc/soc_caps.h"
#include "esp_log.h"
#include "esp_netif_sntp.h"
#include "nvs_flash.h"

#include "app_adc.h"
#include "app_eeprom.h"
#include "app_gptimer.h"
#include "app_pwm.h"
#include "app_rmaker.h"

#define ACTION_AUTO_SENSOR_READ 0x01
#define ACTION_MANUAL_SENSOR_READ 0x02
#define ACTION_AUTO_WATERING 0x04
#define ACTION_MANUAL_WATERING 0x08
#define ACTION_SET_AUTO_WATERING 0x10
#define ACTION_HUMIDITY_HISTORY 0x20

#define SENSOR_AUTO 0x01
#define SENSOR_MANUAL 0x02

#define WATERING_AUTO 0x01
#define WATERING_MANUAL 0x02

#define WAIT_AFTER_WATERING_S /*1000 * 60 * 15*/ 1000 * 10

static bool timer_on_alarm_cb(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx);
static esp_err_t auto_watering_write_cb(const esp_rmaker_device_t *device, const esp_rmaker_param_t *param,
                                        const esp_rmaker_param_val_t val, void *priv_data, esp_rmaker_write_ctx_t *ctx);
static esp_err_t manual_watering_cb(const esp_rmaker_device_t *device, const esp_rmaker_param_t *param,
                                    const esp_rmaker_param_val_t val, void *priv_data, esp_rmaker_write_ctx_t *ctx);
static esp_err_t current_moisture_cb(const esp_rmaker_device_t *device, const esp_rmaker_param_t *param,
                                     const esp_rmaker_param_val_t val, void *priv_data, esp_rmaker_write_ctx_t *ctx);

static void sensor_task(void *arg);
static void pump_task(void *arg);
static void history_task(void *arg);
static void get_data_from_terminal_task(void *arg);

struct sensor_task_arg_t
{
    uint8_t mode;
    adc_oneshot_unit_handle_t *adcHandle;
    spi_device_handle_t *spiHandle;
    uint8_t *moisture;
};
typedef struct sensor_task_arg_t sensor_task_arg_t;

struct pump_task_arg_t
{
    uint8_t mode;
    spi_device_handle_t *spiHandle;
    uint8_t activeTimeS;
};
typedef struct pump_task_arg_t pump_task_arg_t;

struct history_task_arg_t
{
    spi_device_handle_t *spiHandle;
    uint8_t moistures[TOTAL_ADDRESSES];
    uint8_t total;
    uint64_t timestamp;
};
typedef struct history_task_arg_t history_task_arg_t;

TaskHandle_t mainTaskHandle = NULL;
TaskHandle_t sensorTaskHandle = NULL;
TaskHandle_t pumpTaskHandle = NULL;
TaskHandle_t historyTaskHandle = NULL;
TaskHandle_t getDataFromTerminalTask = NULL;

static uint8_t action = ACTION_AUTO_SENSOR_READ;
static bool autoWateringEn = false;
static uint8_t timeWatering = 5;
static bool watering = false;

SemaphoreHandle_t xSemaphore = NULL;

static const char *TAG = "ASE-PROJECT";

void app_main(void)
{

    /* Initialize NVS. */
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    /* Init rainmaker */
    rmaker_init(auto_watering_write_cb, current_moisture_cb, manual_watering_cb);

    mainTaskHandle = xTaskGetCurrentTaskHandle();

    xSemaphore = xSemaphoreCreateBinary();

    /* create gptimer */
    gptimer_handle_t gptimer = NULL;
    gptimer_create(&gptimer, timer_on_alarm_cb);

    /* Init ADC2 */
    adc_oneshot_unit_handle_t adcHandle;
    adc_cali_handle_t adcCaliHandle;
    bool doCalibration;
    adc_init(&adcHandle, &adcCaliHandle, &doCalibration);

    /* Init pwm */
    pwm_init();

    /* Init eeprom */
    spi_device_handle_t spiHandle;
    eeprom_init(&spiHandle);

    uint64_t now = time(NULL);
    eeprom_write_timestamp(spiHandle, now);

    xTaskCreate(get_data_from_terminal_task, "Data_From_Terminal_Task", 1024, NULL, 5, &getDataFromTerminalTask);

    sensor_task_arg_t sensorTaskArg;
    sensorTaskArg.adcHandle = &adcHandle;
    sensorTaskArg.spiHandle = &spiHandle;

    pump_task_arg_t pumpTaskArg;
    pumpTaskArg.spiHandle = &spiHandle;

    history_task_arg_t historyTaskArg;
    historyTaskArg.spiHandle = &spiHandle;

    /* run once to get first sensor read */
    xSemaphoreGive(xSemaphore);

    while (1)
    {
        if (xSemaphoreTake(xSemaphore, pdMS_TO_TICKS(1000 * 60 * 4)) == pdTRUE)
        {

            if (action & ACTION_AUTO_SENSOR_READ) // update moisture history and current moisture
            {
                sensorTaskArg.mode = SENSOR_AUTO;

                xTaskCreate(sensor_task, "Sensor_Task", 8192, &sensorTaskArg, 5, &sensorTaskHandle);

                action &= ~ACTION_AUTO_SENSOR_READ;
            }
            else if (action & ACTION_MANUAL_SENSOR_READ) // get current moisture
            {
                uint8_t moisture;

                sensorTaskArg.mode = SENSOR_MANUAL;
                sensorTaskArg.moisture = &moisture;

                xTaskCreate(sensor_task, "Sensor_Task", 8192, &sensorTaskArg, 8, &sensorTaskHandle);

                action &= ~ACTION_MANUAL_SENSOR_READ;
            }

            if (action & ACTION_SET_AUTO_WATERING) // enable/disable auto watering
            {
                rmaker_update_auto_watering(autoWateringEn);

                ESP_LOGI(TAG, "AUTO_WATERING SET TO %s", autoWateringEn ? "true" : "false");

                action &= ~ACTION_SET_AUTO_WATERING;
            }

            if (action & ACTION_AUTO_WATERING) // auto watering
            {
                if (pumpTaskHandle == NULL && autoWateringEn)
                {
                    ESP_LOGI(TAG, "ENTERING AUTO_WATERING. ENABLED = %s", autoWateringEn ? "true" : "false");

                    pumpTaskArg.mode = WATERING_AUTO;

                    xTaskCreate(pump_task, "Pump_Task", 8192, &pumpTaskArg, 5, &pumpTaskHandle);
                }

                action &= ~ACTION_AUTO_WATERING;
            }
            else if (action & ACTION_MANUAL_WATERING) // manual watering
            {
                ESP_LOGI(TAG, "ENTERING MANUAL_WATERING");

                if (pumpTaskHandle != NULL)
                    vTaskDelete(pumpTaskHandle);

                pumpTaskArg.mode = WATERING_MANUAL;
                pumpTaskArg.activeTimeS = timeWatering;

                xTaskCreate(pump_task, "Pump_Task", 8192, &pumpTaskArg, 8, &pumpTaskHandle);

                action &= ~ACTION_MANUAL_WATERING;
            }

            if (action & ACTION_HUMIDITY_HISTORY) // check moisture history
            {
                uint8_t *moistures;
                uint8_t total;

                xTaskCreate(history_task, "History_Task", 8192, &historyTaskArg, 8, &historyTaskHandle);

                action &= ~ACTION_HUMIDITY_HISTORY;

                /*Example for usage*/
                do
                {
                    vTaskDelay(10);
                } while (historyTaskHandle != NULL);

                moistures = historyTaskArg.moistures;
                total = historyTaskArg.total;

                for (int i = 0; i < total; i++)
                {
                    time_t timestamp = historyTaskArg.timestamp + i * GPTIMER_PERIOD_S;
                    struct tm tm;
                    char s[64];

                    localtime_r(&timestamp, &tm);
                    strftime(s, sizeof(s), "%c", &tm);

                    ESP_LOGI(TAG, "HISTORY VALUE [%d] = %u | date = %s", i, moistures[i], s);
                }
            }
        }
    }

    adc_deinit(&adcHandle, &adcCaliHandle, doCalibration);
    gptimer_delete(&gptimer);
}

/*---------------------------------------------------------------
        Auto watering callback
---------------------------------------------------------------*/
static esp_err_t auto_watering_write_cb(const esp_rmaker_device_t *device, const esp_rmaker_param_t *param,
                                        const esp_rmaker_param_val_t val, void *priv_data, esp_rmaker_write_ctx_t *ctx)
{
    if (ctx)
    {
        ESP_LOGI(TAG, "Received write request via : %s", esp_rmaker_device_cb_src_to_str(ctx->src));
    }
    if (strcmp(esp_rmaker_param_get_name(param), ESP_RMAKER_DEF_POWER_NAME) == 0)
    {
        autoWateringEn = val.val.b ? true : false;
        ESP_LOGI(TAG, "RECEIVED_AUTO_WATERING_EN: %s", autoWateringEn ? "true" : "false");
        action |= ACTION_SET_AUTO_WATERING;

        xSemaphoreGiveFromISR(xSemaphore, NULL);
    }
    return ESP_OK;
}

/*---------------------------------------------------------------
        Watering device callback
---------------------------------------------------------------*/
static esp_err_t manual_watering_cb(const esp_rmaker_device_t *device, const esp_rmaker_param_t *param,
                                    const esp_rmaker_param_val_t val, void *priv_data, esp_rmaker_write_ctx_t *ctx)
{
    if (ctx)
    {
        ESP_LOGI(TAG, "Received write request via : %s", esp_rmaker_device_cb_src_to_str(ctx->src));
    }
    if (strcmp(esp_rmaker_param_get_name(param), "time irrigating (seconds)") == 0)
    {
        timeWatering = val.val.i;
        esp_rmaker_param_update_and_report(param, val);
    }
    else if (strcmp(esp_rmaker_param_get_name(param), "trigger pump") == 0 && !watering)
    {
        action |= ACTION_MANUAL_WATERING;

        xSemaphoreGiveFromISR(xSemaphore, NULL);
    }

    return ESP_OK;
}

/*---------------------------------------------------------------
        Moisture sensor device callback
---------------------------------------------------------------*/
static esp_err_t current_moisture_cb(const esp_rmaker_device_t *device, const esp_rmaker_param_t *param,
                                     const esp_rmaker_param_val_t val, void *priv_data, esp_rmaker_write_ctx_t *ctx)
{
    if (ctx)
    {
        ESP_LOGI(TAG, "Received write request via : %s", esp_rmaker_device_cb_src_to_str(ctx->src));
    }
    if (strcmp(esp_rmaker_param_get_name(param), "trigger reading") == 0)
    {
        action |= ACTION_MANUAL_SENSOR_READ;

        xSemaphoreGiveFromISR(xSemaphore, NULL);
    }

    return ESP_OK;
}

/*---------------------------------------------------------------
        Timer callback
---------------------------------------------------------------*/
static bool timer_on_alarm_cb(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    action |= ACTION_AUTO_SENSOR_READ;
    action |= ACTION_AUTO_WATERING;

    xSemaphoreGiveFromISR(xSemaphore, &xHigherPriorityTaskWoken);

    // return whether we need to yield at the end of ISR
    return xHigherPriorityTaskWoken;
}

/*---------------------------------------------------------------
        Sensor Task
---------------------------------------------------------------*/
static void sensor_task(void *arg)
{
    static bool warned = false;

    sensor_task_arg_t *sensorTaskArg = (sensor_task_arg_t *)arg;

    adc_oneshot_unit_handle_t *adcHandle = (adc_oneshot_unit_handle_t *)(sensorTaskArg->adcHandle);

    int average = 0, raw;
    for (int i = 0; i < 50; i++)
    {
        adc_get_raw(adcHandle, &raw);
        average += raw;
        vTaskDelay(pdMS_TO_TICKS(20));
    }

    average /= 50;

    uint8_t percentage = (average * 100) / 4095;

    uint8_t old;
    eeprom_read_last_moisture(*sensorTaskArg->spiHandle, &old);

    if (sensorTaskArg->mode == SENSOR_AUTO)
    {
        eeprom_write_moisture(*sensorTaskArg->spiHandle, percentage);
        ESP_LOGI(TAG, "SENSOR_TASK: AUTO_SENSOR_READ %u stored to EEPROM", percentage);
    }

    /* update value on cloud service if it changed or was manually requested */
    if ((old != percentage) || sensorTaskArg->mode == SENSOR_MANUAL)
    {
        ESP_LOGI(TAG, "SENSOR_TASK: %d UPDATED IN THE CLOUD", percentage);
        rmaker_update_moisture(percentage);
    }

    /* warn user if moisture value is critical */
    if (percentage < 10 && !warned)
    {
        rmaker_warn_user("Moisture on critical level!");
        warned = true;
    }
    else if (percentage > 20 && warned) // clean warned flag
    {
        warned = false;
    }

    if (xTaskGetCurrentTaskHandle() == sensorTaskHandle)
        sensorTaskHandle = NULL;
    vTaskDelete(NULL);
}

/*---------------------------------------------------------------
        Pump Task
---------------------------------------------------------------*/
static void pump_task(void *arg)
{
    ESP_LOGI(TAG, "PUMP_TASK: ENTERING");

    pump_task_arg_t *pumpTaskArg = (pump_task_arg_t *)arg;

    if (pumpTaskArg->mode == WATERING_AUTO)
    {
        vTaskDelay(pdMS_TO_TICKS(3000));

        uint8_t moisture;

        while (autoWateringEn)
        {
            eeprom_read_last_moisture(*pumpTaskArg->spiHandle, &moisture);

            ESP_LOGI(TAG, "PUMP_TASK: AUTO_WATERING moisture read: %u", moisture);

            if (moisture < 50)
            {
                uint8_t activeTimeS = (((50 - moisture) / 10) + 1) * 10;

                watering = true;
                rmaker_update_watering_status(watering);

                ESP_LOGI(TAG, "PUMP_TASK: AUTO_WATERING for %u seconds", activeTimeS);

                pwm_set_duty(PWM_100_DUTY);
                vTaskDelay(pdMS_TO_TICKS(1000 * activeTimeS)); // time watering
                pwm_set_duty(PWM_0_DUTY);

                watering = false;
                rmaker_update_watering_status(watering);
            }

            vTaskDelay(pdMS_TO_TICKS(WAIT_AFTER_WATERING_S)); // time on hold until next check
        }
    }

    else if (pumpTaskArg->mode == WATERING_MANUAL)
    {
        watering = true;
        rmaker_update_watering_status(watering);

        ESP_LOGI(TAG, "PUMP_TASK: MANUAL_WATERING for %u seconds", pumpTaskArg->activeTimeS);

        pwm_set_duty(PWM_100_DUTY);
        vTaskDelay(pdMS_TO_TICKS(1000 * pumpTaskArg->activeTimeS)); // time watering
        pwm_set_duty(PWM_0_DUTY);

        watering = false;
        rmaker_update_watering_status(watering);

        vTaskDelay(pdMS_TO_TICKS(WAIT_AFTER_WATERING_S)); // time on hold to let the dirt irrigate
    }

    ESP_LOGI(TAG, "PUMP_TASK: WATERING CYCLE ENDED");

    if (xTaskGetCurrentTaskHandle() == pumpTaskHandle)
        pumpTaskHandle = NULL;
    vTaskDelete(NULL);
}

/*---------------------------------------------------------------
        Moisture history Task
---------------------------------------------------------------*/
static void history_task(void *arg)
{
    history_task_arg_t *historyTaskArg = (history_task_arg_t *)arg;

    for (int i = 0; i < TOTAL_ADDRESSES; i++)
    {
        if (!eeprom_read_moisture(*historyTaskArg->spiHandle, i, &(historyTaskArg->moistures[i])))
        {
            historyTaskArg->total = i;
            break;
        }
    }

    eeprom_read_timestamp(*historyTaskArg->spiHandle, &(historyTaskArg->timestamp));

    if (xTaskGetCurrentTaskHandle() == historyTaskHandle)
        historyTaskHandle = NULL;
    vTaskDelete(NULL);
}

/*---------------------------------------------------------------
        Get Data From Terminal Task
---------------------------------------------------------------*/
static void get_data_from_terminal_task(void *arg)
{
    char buf;

    while (1)
    {
        buf = getchar();

        switch (buf)
        {
        case 'h':
            action |= ACTION_HUMIDITY_HISTORY;
            xSemaphoreGive(xSemaphore);
            break;

        case 'r':
            action |= ACTION_MANUAL_SENSOR_READ;
            xSemaphoreGive(xSemaphore);
            break;

        case 'w':
            autoWateringEn = !autoWateringEn;
            action |= ACTION_SET_AUTO_WATERING;
            xSemaphoreGive(xSemaphore);
            break;
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
