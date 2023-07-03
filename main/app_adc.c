#include <stdio.h>

#include "esp_log.h"

#include "app_adc.h"

static bool sensor_adc_calibration_init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle);
static void sensor_adc_calibration_deinit(adc_cali_handle_t handle);

static int adc_raw[2][10];

static const char *TAG = "ASE-PROJECT-ADC";

/*---------------------------------------------------------------
        ADC Initialization
---------------------------------------------------------------*/
void adc_init(adc_oneshot_unit_handle_t *adc_handle, adc_cali_handle_t *adc_cali_handle, bool *do_calibration)
{
    //-------------ADC1 Init---------------//
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, adc_handle));

    //-------------ADC1 Calibration Init---------------//
    *adc_cali_handle = NULL;
    *do_calibration = sensor_adc_calibration_init(ADC_UNIT_1, SENSOR_ADC1_CHAN0, SENSOR_ADC_ATTEN, adc_cali_handle);

    //-------------ADC1 Config---------------//
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = SENSOR_ADC_ATTEN,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(*adc_handle, SENSOR_ADC1_CHAN0, &config));
}

void adc_deinit(adc_oneshot_unit_handle_t *adc2_handle, adc_cali_handle_t *adc2_cali_handle, bool do_calibration)
{
    ESP_ERROR_CHECK(adc_oneshot_del_unit(*adc2_handle));
    if (do_calibration)
    {
        sensor_adc_calibration_deinit(*adc2_cali_handle);
    }
}

void adc_get_raw(adc_oneshot_unit_handle_t *adc2_handle, int *raw_value)
{
    ESP_ERROR_CHECK(adc_oneshot_read(*adc2_handle, SENSOR_ADC1_CHAN0, &adc_raw[1][0]));

    *raw_value = adc_raw[1][0];
}

/*---------------------------------------------------------------
        ADC Calibration
---------------------------------------------------------------*/
static bool sensor_adc_calibration_init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle)
{
    adc_cali_handle_t handle = NULL;
    esp_err_t ret = ESP_FAIL;
    bool calibrated = false;

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    if (!calibrated)
    {
        ESP_LOGI(TAG, "calibration scheme version is %s", "Curve Fitting");
        adc_cali_curve_fitting_config_t cali_config = {
            .unit_id = unit,
            .chan = channel,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_curve_fitting(&cali_config, &handle);
        if (ret == ESP_OK)
        {
            calibrated = true;
        }
    }
#endif

#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    if (!calibrated)
    {
        ESP_LOGI(TAG, "calibration scheme version is %s", "Line Fitting");
        adc_cali_line_fitting_config_t cali_config = {
            .unit_id = unit,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_line_fitting(&cali_config, &handle);
        if (ret == ESP_OK)
        {
            calibrated = true;
        }
    }
#endif

    *out_handle = handle;
    if (ret == ESP_OK)
    {
        ESP_LOGI(TAG, "Calibration Success");
    }
    else if (ret == ESP_ERR_NOT_SUPPORTED || !calibrated)
    {
        ESP_LOGW(TAG, "eFuse not burnt, skip software calibration");
    }
    else
    {
        ESP_LOGE(TAG, "Invalid arg or no memory");
    }

    return calibrated;
}

static void sensor_adc_calibration_deinit(adc_cali_handle_t handle)
{
#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    ESP_LOGI(TAG, "deregister %s calibration scheme", "Curve Fitting");
    ESP_ERROR_CHECK(adc_cali_delete_scheme_curve_fitting(handle));

#elif ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    ESP_LOGI(TAG, "deregister %s calibration scheme", "Line Fitting");
    ESP_ERROR_CHECK(adc_cali_delete_scheme_line_fitting(handle));
#endif
}
