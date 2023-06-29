#include "driver/ledc.h"

#define PWM_TIMER LEDC_TIMER_0
#define PWM_MODE LEDC_LOW_SPEED_MODE
#define PWM_OUTPUT_IO (19) // Define the output GPIO
#define PWM_CHANNEL LEDC_CHANNEL_0
#define PWM_DUTY_RES LEDC_TIMER_13_BIT // Set duty resolution to 13 bits
#define PWM_0_DUTY (0)
#define PWM_25_DUTY (2047)
#define PWM_50_DUTY (4095) // Set duty to 50%. ((2 ** 13) - 1) * 50% = 4095
#define PWM_75_DUTY (6143)
#define PWM_100_DUTY (8191)
#define PWM_FREQUENCY (5000) // Frequency in Hertz. Set frequency at 5 kHz

void pwm_init();
void pwm_set_duty(uint16_t duty);