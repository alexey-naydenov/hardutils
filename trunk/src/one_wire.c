#include <stdlib.h>

#include "one_wire.h"

struct ow_bus {
  int8_fast_t refcount;
  struct td_timer *timer;
  void (*output_fn)(void);
  void (*input_fn)(void);
  void (*pull_up_fn)(void);
  void (*pull_down_fn)(void);
  uint8_fast_t (*read_fn)(void);
};
/* Create and destruction. */
int8_fast_t ow_bus_new(struct ow_bus **bus) {
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
int8_fast_t ow_bus_set_timer(struct ow_bus *bus, struct td_timer *timer) {
  if (bus == NULL || timer == NULL) return -1;
  bus->timer = timer;
  return 0;
}
int8_fast_t ow_bus_set_output_fn(struct ow_bus *bus, void (*output_fn)(void)) {
  if (bus == NULL || output_fn == NULL) return -1;
  bus->output_fn = output_fn;
  return 0;
}
int8_fast_t ow_bus_set_input_fn(struct ow_bus *bus, void (*input_fn)(void)) {
  if (bus == NULL || input_fn == NULL) return -1;
  bus->input_fn = input_fn;
  return 0;
}
int8_fast_t ow_bus_set_pull_up_fn(struct ow_bus *bus, void (*pull_up_fn)(void)) {
  if (bus == NULL || pull_up_fn == NULL) return -1;
  bus->pull_up_fn = pull_up_fn;
  return 0;
}
int8_fast_t ow_bus_set_pull_down_fn(struct ow_bus *bus, void (*pull_down_fn)(void)) {
  if (bus == NULL || pull_down_fn == NULL) return -1;
  bus->pull_down_fn = pull_down_fn;
  return 0;
}
int8_fast_t ow_bus_set_read_fn(struct ow_bus *bus, uint8_fast_t (*read_fn)(void)) {
  if (bus == NULL || read_fn == NULL) return -1;
  bus->read_fn = read_fn;
  return 0;
}
/* Interface functions. */

struct ow_device {
  int8_fast_t refcount;
  struct ow_bus *bus;
  uint8_fast_t address[OW_ADDRESS_LENGTH];
};

/* Create and destruction. */
int8_fast_t ow_device_new(struct ow_device **device) {
  int8_fast_t i;
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
