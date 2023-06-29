#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

#define SENSOR_ADC1_CHAN0 ADC_CHANNEL_6

#define SENSOR_ADC_ATTEN ADC_ATTEN_DB_11

static int adc_raw[2][10];
static int voltage[2][10];

void adc_init(adc_oneshot_unit_handle_t *adc2_handle, adc_cali_handle_t *adc2_cali_handle, bool *do_calibration);
void adc_deinit(adc_oneshot_unit_handle_t *adc2_handle, adc_cali_handle_t *adc2_cali_handle, bool do_calibration);
void adc_get_raw(adc_oneshot_unit_handle_t *adc2_handle, int *raw_value);

static bool sensor_adc_calibration_init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle);
static void sensor_adc_calibration_deinit(adc_cali_handle_t handle);
