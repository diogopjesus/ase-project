#include <esp_rmaker_core.h>
#include <esp_rmaker_standard_types.h>
#include <esp_rmaker_standard_params.h>
#include <esp_rmaker_standard_devices.h>
#include <esp_rmaker_schedule.h>
#include <esp_rmaker_scenes.h>
#include <esp_rmaker_console.h>
#include <esp_rmaker_ota.h>
#include <esp_rmaker_utils.h>

#include <esp_rmaker_common_events.h>

#include <app_wifi.h>
#include <app_insights.h>

#define DEFAULT_AUTO_WATERING_POWER false

void rmaker_init(void *auto_watering_write_cb, void *current_moisture_cb, void *manual_watering_cb);
void rmaker_update_moisture(uint8_t value);
void rmaker_update_watering_status(bool watering);
void rmaker_get_watering_status(char *status);
void rmaker_warn_user(char *str);
void rmaker_update_auto_watering(bool value);