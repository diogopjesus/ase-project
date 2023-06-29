#include "app_pwm.h"

/*---------------------------------------------------------------
        PWM Creation
---------------------------------------------------------------*/
void pwm_init(void)
{
    // Prepare and then apply the PWM timer configuration
    ledc_timer_config_t pwm_timer = {
        .speed_mode = PWM_MODE,
        .timer_num = PWM_TIMER,
        .duty_resolution = PWM_DUTY_RES,
        .freq_hz = PWM_FREQUENCY, // Set output frequency at 5 kHz
        .clk_cfg = LEDC_AUTO_CLK};
    ESP_ERROR_CHECK(ledc_timer_config(&pwm_timer));

    // Prepare and then apply the PWM channel configuration
    ledc_channel_config_t pwm_channel = {
        .speed_mode = PWM_MODE,
        .channel = PWM_CHANNEL,
        .timer_sel = PWM_TIMER,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = PWM_OUTPUT_IO,
        .duty = PWM_0_DUTY, // Set duty to 0%
        .hpoint = 0};
    ESP_ERROR_CHECK(ledc_channel_config(&pwm_channel));
}

void pwm_set_duty(uint16_t duty)
{
    if (duty > PWM_100_DUTY)
        duty = PWM_100_DUTY;

    ESP_ERROR_CHECK(ledc_set_duty(PWM_MODE, PWM_CHANNEL, duty));
    ESP_ERROR_CHECK(ledc_update_duty(PWM_MODE, PWM_CHANNEL));
}