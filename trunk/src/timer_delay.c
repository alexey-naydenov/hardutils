#include <timer_delay.h>

int td_init(struct td_timer *timer, TD_TIMER_TYPE (*get_counter)(void),
	    TD_TIMER_TYPE period) {
  if (timer == 0 || get_counter == 0) {
    return -1;
  }
  timer->get_counter = get_counter;
  timer->period = period;
  return 0;
}

int td_start(struct td_timer *timer) {
  if (timer == 0) {
    return -1;
  }
  timer->start = timer->get_counter();
  return 0;
}

int td_has_elapsed(struct td_timer *timer, TD_TIMER_TYPE delay) {
  TD_TIMER_TYPE stop, current;
  if (timer == 0) {
    return -128;
  }
  current = timer->get_counter();
  if (timer->period - timer->start > delay) {
    stop = timer->start + delay;
    if (current < stop && current > timer->start) {
      return 0;
    } else {
      return current - stop;
    }
  } else {
    stop = delay - (timer->period - timer->start);
    if (current < stop || current > timer->start) {
      return 0;
    } else {
      return current - stop;
    }
  }
}

int td_wait(struct td_timer *timer, TD_TIMER_TYPE delay) {
  TD_TIMER_TYPE stop, current;
  if (timer == 0) {
    return -128;
  }
  current = timer->get_counter();
  if (timer->period - timer->start > delay) {
    stop = timer->start + delay;
    while (current < stop && current > timer->start) {
      current = timer->get_counter();
    }
  } else {
    stop = delay - (timer->period - timer->start);
    while (current < stop || current > timer->start) {
      current = timer->get_counter();
    }
  }
  return current - stop;
}

