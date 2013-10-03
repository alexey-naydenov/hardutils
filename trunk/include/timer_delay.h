#include <timer_delay_config.h>
/* TD_TIMER_TYPE must be set to some type */
/* struct td_timer; */
struct td_timer {
  TD_TIMER_TYPE (*get_counter)(void);
  TD_TIMER_TYPE period;
  TD_TIMER_TYPE start;
};

int td_init(struct td_timer *timer, TD_TIMER_TYPE (*get_counter)(void),
	    TD_TIMER_TYPE period);
int td_start(struct td_timer *timer);
int td_wait(struct td_timer *timer, TD_TIMER_TYPE delay);
int td_has_elapsed(struct td_timer *timer, TD_TIMER_TYPE delay);

