#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

#define SENSOR_ADC1_CHAN0 ADC_CHANNEL_6

#define SENSOR_ADC_ATTEN ADC_ATTEN_DB_11

void adc_init(adc_oneshot_unit_handle_t *adc2_handle, adc_cali_handle_t *adc2_cali_handle, bool *do_calibration);
void adc_deinit(adc_oneshot_unit_handle_t *adc2_handle, adc_cali_handle_t *adc2_cali_handle, bool do_calibration);
void adc_get_raw(adc_oneshot_unit_handle_t *adc2_handle, int *raw_value);
