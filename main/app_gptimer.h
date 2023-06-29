#include "driver/gptimer.h"

#define GPTIMER_PERIOD_S /*3 * 60*/ 20
#define GPTIMER_PERIOD_MS (GPTIMER_PERIOD_S * 1000)
#define GPTIMER_PERIOD_US (GPTIMER_PERIOD_MS * 1000)

void gptimer_create(gptimer_handle_t *gptimer, void *alarm_cb);
void gptimer_delete(gptimer_handle_t *gptimer);
void gptimer_update_alarm_count(gptimer_handle_t *gptimer, uint64_t alarm_count);
void gptimer_update_alarm_auto_reload(gptimer_handle_t *gptimer, bool auto_reload);
