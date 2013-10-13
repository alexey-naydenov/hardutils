#include <stdlib.h>

#include "one_wire.h"

struct ow_bus {
  int_fast8_t refcount;
  struct td_timer *timer;
  void (*output_fn)(void);
  void (*input_fn)(void);
  void (*pull_up_fn)(void);
  void (*pull_down_fn)(void);
  uint_fast8_t (*read_fn)(void);
};
/* Create and destruction. */
int_fast8_t ow_bus_new(struct ow_bus **bus) {
  struct ow_bus *new_bus = calloc(1, sizeof(struct ow_bus));
  if (!new_bus) return -1;
  new_bus->refcount = 1;
  *bus = new_bus;
  return 0;
}
struct ow_bus *ow_bus_ref(struct ow_bus *bus) {
  if (bus == NULL) return NULL;
  bus->refcount++;
  return bus;
}
struct ow_bus *ow_bus_unref(struct ow_bus *bus) {
  if (bus == NULL) return NULL;
  bus->refcount--;
  if (bus->refcount > 0) return bus;
  ow_bus_free(bus);
  return NULL;
}
void ow_bus_free(struct ow_bus *bus) {
  if (bus == NULL) return;
  free(bus);
}
/* Init functions. */
int_fast8_t ow_bus_set_timer(struct ow_bus *bus, struct td_timer *timer) {
  if (bus == NULL || timer == NULL) return -1;
  bus->timer = timer;
  return 0;
}
int_fast8_t ow_bus_set_output_fn(struct ow_bus *bus, void (*output_fn)(void)) {
  if (bus == NULL || output_fn == NULL) return -1;
  bus->output_fn = output_fn;
  return 0;
}
int_fast8_t ow_bus_set_input_fn(struct ow_bus *bus, void (*input_fn)(void)) {
  if (bus == NULL || input_fn == NULL) return -1;
  bus->input_fn = input_fn;
  return 0;
}
int_fast8_t ow_bus_set_pull_up_fn(struct ow_bus *bus, void (*pull_up_fn)(void)) {
  if (bus == NULL || pull_up_fn == NULL) return -1;
  bus->pull_up_fn = pull_up_fn;
  return 0;
}
int_fast8_t ow_bus_set_pull_down_fn(struct ow_bus *bus, void (*pull_down_fn)(void)) {
  if (bus == NULL || pull_down_fn == NULL) return -1;
  bus->pull_down_fn = pull_down_fn;
  return 0;
}
int_fast8_t ow_bus_set_read_fn(struct ow_bus *bus, uint_fast8_t (*read_fn)(void)) {
  if (bus == NULL || read_fn == NULL) return -1;
  bus->read_fn = read_fn;
  return 0;
}
/* Interface functions. */
int16_t ow_bus_reset(struct ow_bus *bus) {
  uint16_t capture, capture2;
  /* pull down bus and wait for 500 us */
  bus->output_fn();
  bus->pull_down_fn();
  td_start(bus->timer);
  td_wait(bus->timer, 500);
  /* pull up and wait a little, this should not be required */
  bus->pull_up_fn();
  td_start(bus->timer);
  td_wait(bus->timer, 1);
  /* wait for response pull */
  bus->input_fn();
  while (bus->read_fn() > 0
  	 && (capture = td_get_elapsed(bus->timer)) < 480);
  if (capture >= 480) { /* the bus was not pulled down */
    bus->output_fn();
    return -1;
  }
  /* measure down time */
  td_start(bus->timer);
  while (bus->read_fn() == 0
  	 && (capture = td_get_elapsed(bus->timer)) < 480);
  td_start(bus->timer);
  bus->output_fn();
  if (capture >= 480) { /* the bus was not released */
    return -2;
  }
  /* wait for device to restore */
  td_wait(bus->timer, 480 - capture);
  return capture;
}



struct ow_device {
  int_fast8_t refcount;
  struct ow_bus *bus;
  uint_fast8_t address[OW_ADDRESS_LENGTH];
};

/* Create and destruction. */
int_fast8_t ow_device_new(struct ow_device **device) {
  int_fast8_t i;
  struct ow_device *new_device;
  if (device == NULL) return -1;
  new_device = calloc(1, sizeof(struct ow_device));
  if (new_device == NULL) return -1;
  new_device->refcount = 1;
  new_device->bus = NULL;
  for (i = 0; i < OW_ADDRESS_LENGTH; ++i) {
    new_device->address[i] = 0;
  }
  *device = new_device;
  return 0;
}
struct ow_device *ow_device_ref(struct ow_device *device) {
  if (device == NULL) return NULL;
  device->refcount++;
  return device;
}
struct ow_device *ow_device_unref(struct ow_device *device) {
  if (device == NULL) return NULL;
  device->refcount--;
  if (device->refcount > 0) return device;
  ow_device_free(device);
  return NULL;
}
void ow_device_free(struct ow_device *device) {
  if (device == NULL) return;
  ow_bus_unref(device->bus);
  free(device);
}

