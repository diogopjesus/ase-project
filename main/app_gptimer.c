#include "app_gptimer.h"

/*---------------------------------------------------------------
        Timer Creation
---------------------------------------------------------------*/
void gptimer_create(gptimer_handle_t *gptimer, void *alarm_cb)
{
    gptimer_config_t timer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 1000000, // 1MHz, 1 tick=1us
    };
    ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, gptimer));

    gptimer_event_callbacks_t cbs = {
        .on_alarm = alarm_cb,
    };
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(*gptimer, &cbs, NULL));

    ESP_ERROR_CHECK(gptimer_enable(*gptimer));

    gptimer_alarm_config_t alarm_config = {
        .reload_count = 0,
        .alarm_count = GPTIMER_PERIOD_US,
        .flags.auto_reload_on_alarm = true,
    };
    ESP_ERROR_CHECK(gptimer_set_alarm_action(*gptimer, &alarm_config));

    ESP_ERROR_CHECK(gptimer_start(*gptimer));
}

void gptimer_delete(gptimer_handle_t *gptimer)
{
    ESP_ERROR_CHECK(gptimer_stop(*gptimer));

    ESP_ERROR_CHECK(gptimer_disable(*gptimer));

    ESP_ERROR_CHECK(gptimer_del_timer(*gptimer));
}

void gptimer_update_alarm_count(gptimer_handle_t *gptimer, uint64_t alarm_count)
{
    ESP_ERROR_CHECK(gptimer_stop(*gptimer));

    ESP_ERROR_CHECK(gptimer_set_raw_count(*gptimer, 0));

    gptimer_alarm_config_t alarm_config = {
        .alarm_count = alarm_count, // period = (alarm_count / 1_000_000)s
    };
    ESP_ERROR_CHECK(gptimer_set_alarm_action(*gptimer, &alarm_config));

    ESP_ERROR_CHECK(gptimer_start(*gptimer));
}

void gptimer_update_alarm_auto_reload(gptimer_handle_t *gptimer, bool auto_reload)
{
    ESP_ERROR_CHECK(gptimer_stop(*gptimer));

    gptimer_alarm_config_t alarm_config = {
        .flags.auto_reload_on_alarm = auto_reload,
    };
    ESP_ERROR_CHECK(gptimer_set_alarm_action(*gptimer, &alarm_config));

    ESP_ERROR_CHECK(gptimer_start(*gptimer));
}
