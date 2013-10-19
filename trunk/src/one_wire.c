#include <stdlib.h>

#include "one_wire.h"

#define ARRAY_SIZE(x) sizeof(x)/sizeof(x[0])

uint8_t ow_crc(uint8_t *data, int length) {
  int bi, di;
  uint8_t crc = 0;
  for (di = 0; di < length; ++di) {
    crc ^= data[di];
    for (bi = 0; bi < 8; ++bi) {
      if (crc & 0x1) {
	crc = (crc >> 1) ^ 0x8c;
      } else {
	crc = crc >> 1;
      }
    }
  }

  return crc;
}

enum ow_errors {
  OW_ERROR = 1,
  OW_ERROR_BUSY,
  OW_ERROR_NOOP,
  OW_ERROR_NO_RESPONSE,
  OW_ERROR_BUS_DOWN
};

enum ow_bus_states {
  OW_BUS_IDLE,
  OW_BUS_RESET_PULSE,
  OW_BUS_RESET_RECOVER,
  OW_BUS_WRITE,
  OW_BUS_READ
};

struct ow_bus {
  int_fast8_t refcount;
  struct td_timer *timer;
  enum ow_bus_states state;
  uint_fast8_t data;
  uint_fast8_t *out_data;
  uint_fast8_t bit;
  void (*output_fn)(void);
  void (*input_fn)(void);
  void (*pull_up_fn)(void);
  void (*pull_down_fn)(void);
  uint_fast8_t (*read_fn)(void);
};
/* Create and destruction. */
int_fast8_t ow_bus_new(struct ow_bus **bus) {
  struct ow_bus *new_bus = calloc(1, sizeof(struct ow_bus));
  if (!new_bus) return -OW_ERROR;
  new_bus->refcount = 1;
  new_bus->state = OW_BUS_IDLE;
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
  if (bus == NULL || timer == NULL) return -OW_ERROR;
  bus->timer = timer;
  return 0;
}
int_fast8_t ow_bus_set_output_fn(struct ow_bus *bus, void (*output_fn)(void)) {
  if (bus == NULL || output_fn == NULL) return -OW_ERROR;
  bus->output_fn = output_fn;
  return 0;
}
int_fast8_t ow_bus_set_input_fn(struct ow_bus *bus, void (*input_fn)(void)) {
  if (bus == NULL || input_fn == NULL) return -OW_ERROR;
  bus->input_fn = input_fn;
  return 0;
}
int_fast8_t ow_bus_set_pull_up_fn(struct ow_bus *bus, void (*pull_up_fn)(void)) {
  if (bus == NULL || pull_up_fn == NULL) return -OW_ERROR;
  bus->pull_up_fn = pull_up_fn;
  return 0;
}
int_fast8_t ow_bus_set_pull_down_fn(struct ow_bus *bus, void (*pull_down_fn)(void)) {
  if (bus == NULL || pull_down_fn == NULL) return -OW_ERROR;
  bus->pull_down_fn = pull_down_fn;
  return 0;
}
int_fast8_t ow_bus_set_read_fn(struct ow_bus *bus, uint_fast8_t (*read_fn)(void)) {
  if (bus == NULL || read_fn == NULL) return -OW_ERROR;
  bus->read_fn = read_fn;
  return 0;
}
/* interface helper functions */
int_fast8_t ow_bus_continue(struct ow_bus *bus) {
  int_fast8_t rc;
  switch (bus->state) {
  case OW_BUS_IDLE:
    return -OW_ERROR_NOOP;
  case OW_BUS_RESET_PULSE:
    rc = ow_bus_check_reset_response(bus);
    if (rc < 0) bus->state = OW_BUS_IDLE;
    return rc;
  case OW_BUS_RESET_RECOVER:
    if (!td_has_elapsed(bus->timer, 500)) {
      return 1;
    }
    bus->state = OW_BUS_IDLE;
    return 0;
  case OW_BUS_WRITE:
    return ow_bus_write_next_bit(bus);
  case OW_BUS_READ:
    return ow_bus_read_next_bit(bus);
  default:
    return -OW_ERROR;
  }
}
int_fast8_t ow_bus_terminate_operation(struct ow_bus *bus) {
  bus->state = OW_BUS_IDLE;
  return 0;
}
/* Interface functions. */
int_fast8_t ow_bus_reset(struct ow_bus *bus) {
  if (bus->state != OW_BUS_IDLE) {
    return -OW_ERROR_BUSY;
  }
  /* pull down bus and wait for 500 us */
  bus->output_fn();
  bus->pull_down_fn();
  td_start(bus->timer);
  bus->state = OW_BUS_RESET_PULSE;
  return 1;
}

int_fast8_t ow_bus_check_reset_response(struct ow_bus *bus) {
  uint16_t capture;
  if (!td_has_elapsed(bus->timer, 500)) {
    return 1;
  }
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
    return -OW_ERROR_NO_RESPONSE;
  }
  /* wait during down time */
  td_start(bus->timer); /* wait 480 usec from now to let device recover*/
  while (bus->read_fn() == 0
  	 && (capture = td_get_elapsed(bus->timer)) < 480);
  bus->output_fn();
  if (capture >= 480) { /* the bus was not released */
    return -OW_ERROR_BUS_DOWN;
  }
  bus->state = OW_BUS_RESET_RECOVER;
  return 1;
}

int_fast8_t ow_bus_write(struct ow_bus *bus, uint8_t data) {
  if (bus->state != OW_BUS_IDLE) {
    return -OW_ERROR_BUSY;
  }
  bus->state = OW_BUS_WRITE;
  bus->data = data;
  bus->bit = 0;
  return ow_bus_write_next_bit(bus);
}

int_fast8_t ow_bus_write_next_bit(struct ow_bus *bus) {
  bus->output_fn();
  bus->pull_down_fn();
  td_start(bus->timer);
  td_wait(bus->timer, 2);
  if (bus->data & (1<<(bus->bit))) {
      bus->pull_up_fn();
  }
  td_wait(bus->timer, 90);
  bus->pull_up_fn();
  bus->bit++;
  if (bus->bit == 8) {
    bus->state = OW_BUS_IDLE;
    return 0;
  }
  return 1;
}

int_fast8_t ow_bus_read(struct ow_bus *bus, uint8_t *data) {
  if (bus->state != OW_BUS_IDLE) {
    return -OW_ERROR_BUSY;
  }
  bus->state = OW_BUS_READ;
  bus->out_data = data;
  *(bus->out_data) = 0;
  bus->bit = 0;
  return ow_bus_read_next_bit(bus);
}

int_fast8_t ow_bus_read_bit(struct ow_bus *bus) {
  uint_fast8_t rc;
  bus->output_fn();
  bus->pull_down_fn();
  td_start(bus->timer);
  td_wait(bus->timer, 1);
  bus->input_fn();
  td_wait(bus->timer, 11);
  rc = bus->read_fn();
  return rc;
}

int_fast8_t ow_bus_read_next_bit(struct ow_bus *bus) {
  uint_fast8_t rc;
  bus->output_fn();
  bus->pull_down_fn();
  td_start(bus->timer);
  td_wait(bus->timer, 1);
  bus->input_fn();
  td_wait(bus->timer, 11);
  rc = bus->read_fn();
  *(bus->out_data) |= (rc<<(bus->bit));
  bus->bit++;
  if (bus->bit == 8) {
    bus->state = OW_BUS_IDLE;
    return 0;
  }
  return 1;
}

enum ow_device_operations {
  OW_DEVICE_OP_RESET = 0,
  OW_DEVICE_OP_WRITE,
  OW_DEVICE_OP_READ,
  OW_DEVICE_OP_WAIT_1
};

enum ow_device_states {
  OW_DEVICE_IDLE = 0,
  OW_DEVICE_BUSY,
  OW_DEVICE_WAIT
};

const enum ow_device_operations OW_READ_ROM_OPERATIONS[] = {
  OW_DEVICE_OP_RESET, OW_DEVICE_OP_WRITE, OW_DEVICE_OP_READ, OW_DEVICE_OP_READ,
  OW_DEVICE_OP_READ, OW_DEVICE_OP_READ, OW_DEVICE_OP_READ, OW_DEVICE_OP_READ,
  OW_DEVICE_OP_READ, OW_DEVICE_OP_READ};

const enum ow_device_operations OW_READ_SCRATCHPAD_OPERATIONS[] = {
  OW_DEVICE_OP_RESET, OW_DEVICE_OP_WRITE, /* match rom */
  OW_DEVICE_OP_WRITE, OW_DEVICE_OP_WRITE, OW_DEVICE_OP_WRITE, OW_DEVICE_OP_WRITE, 
  OW_DEVICE_OP_WRITE, OW_DEVICE_OP_WRITE, OW_DEVICE_OP_WRITE, OW_DEVICE_OP_WRITE,
  OW_DEVICE_OP_WRITE, /* read scratchpad command */
  OW_DEVICE_OP_READ, OW_DEVICE_OP_READ, OW_DEVICE_OP_READ, OW_DEVICE_OP_READ,
  OW_DEVICE_OP_READ, OW_DEVICE_OP_READ, OW_DEVICE_OP_READ, OW_DEVICE_OP_READ,
  OW_DEVICE_OP_READ};

const enum ow_device_operations OW_READ_CONVERT_TEMPERATURE_OPERATIONS[] = {
  OW_DEVICE_OP_RESET, OW_DEVICE_OP_WRITE, /* match rom */
  OW_DEVICE_OP_WRITE, OW_DEVICE_OP_WRITE, OW_DEVICE_OP_WRITE, OW_DEVICE_OP_WRITE, 
  OW_DEVICE_OP_WRITE, OW_DEVICE_OP_WRITE, OW_DEVICE_OP_WRITE, OW_DEVICE_OP_WRITE,
  OW_DEVICE_OP_WRITE,
  OW_DEVICE_OP_WAIT_1
};

#define OW_DEVICE_BUFFER_SIZE 19

struct ow_device {
  int_fast8_t refcount;
  struct ow_bus *bus;
  enum ow_device_states state;
  uint_fast8_t operation_count;
  const uint8_t *operations;
  uint8_t *buffer; /* buffer for sending data, stores device address */
  uint8_t *data_source;
  uint8_t *data_sink;
  uint_fast8_t wait_value;
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
  new_device->state = OW_DEVICE_IDLE;
  new_device->buffer = calloc(OW_DEVICE_BUFFER_SIZE, sizeof(uint8_t));
  if (new_device->buffer == NULL) {
    free(new_device);
    return -1;
  }
  *device = new_device;
  return 0;
}
struct ow_device *ow_device_ref(struct ow_device *device) {
  if (device == NULL || device->bus == NULL) return NULL;
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
  free(device->buffer);
  free(device);
}

int_fast8_t ow_device_set_bus(struct ow_device *device, struct ow_bus *bus) {
  if (device == NULL || bus == NULL || device->bus != NULL) return -1;
  device->bus = bus;
  return 0;
}

int_fast8_t ow_device_continue(struct ow_device *device) {
  int_fast8_t rc;
  switch (device->state) {
  case OW_DEVICE_IDLE:
     return -OW_ERROR_NOOP;
  case OW_DEVICE_BUSY:
    rc = ow_bus_continue(device->bus);
    if (rc == 0) {
      device->operation_count--;
      if (device->operation_count == 0) {
	device->state = OW_DEVICE_IDLE;
	return 0;
      }
      device->operations++;
      return ow_device_start_operation(device);
    }
    if (rc < 0) {
      device->state = OW_DEVICE_IDLE;
    }
    return rc;
  case OW_DEVICE_WAIT:
    rc = ow_bus_read_bit(device->bus);
    if (rc == device->wait_value) {
      device->operation_count--;
      if (device->operation_count == 0) {
	device->state = OW_DEVICE_IDLE;
	return 0;
      }
      device->operations++;
      return ow_device_start_operation(device);
    }
    if (rc < 0) {
      device->state = OW_DEVICE_IDLE;
    }
    return rc;
  default:
    return -OW_ERROR;
  }
}

int_fast8_t ow_device_start_operation(struct ow_device *device) {
  int_fast8_t rc;
  device->state = OW_DEVICE_BUSY;
  switch (*(device->operations)) {
  case OW_DEVICE_OP_RESET:
    rc = ow_bus_reset(device->bus);
    break;
  case OW_DEVICE_OP_WRITE:
    rc = ow_bus_write(device->bus, *(device->data_source));
    device->data_source++;
    break;
  case OW_DEVICE_OP_READ:
    rc = ow_bus_read(device->bus, device->data_sink);
    device->data_sink++;
    break;
  case OW_DEVICE_OP_WAIT_1:
    device->wait_value = 1;
    device->state = OW_DEVICE_WAIT;
    rc = 0;
    break;
  }
  if (rc < 0) {
    device->state = OW_DEVICE_IDLE;
  }
  return rc;
}

int_fast8_t ow_device_is_busy(struct ow_device *device) {
  return device->state != OW_DEVICE_IDLE;
}

uint8_t *ow_device_get_address(struct ow_device *device) {
  return &(device->buffer[1]);
}

int_fast8_t  ow_device_read_rom(struct ow_device *device) {
  if (device->state != OW_DEVICE_IDLE) {
    return -OW_ERROR_BUSY;
  }
  device->operation_count = ARRAY_SIZE(OW_READ_ROM_OPERATIONS);
  device->operations = OW_READ_ROM_OPERATIONS;
  device->data_source = device->buffer;
  device->data_source[0] = 0x33; /* read rom operation code */
  device->data_sink = ow_device_get_address(device); /* data_source[1] */
  return ow_device_start_operation(device);
}

int_fast8_t  ow_device_read_scratchpad(struct ow_device *device,
				       uint8_t *scratchpad) {
  if (device->state != OW_DEVICE_IDLE) {
    return -OW_ERROR_BUSY;
  }
  device->operation_count = ARRAY_SIZE(OW_READ_SCRATCHPAD_OPERATIONS);
  device->operations = OW_READ_SCRATCHPAD_OPERATIONS;
  device->data_source = device->buffer;
  device->data_source[0] = 0x55; /* match rom */
  /* address must be stored in bytes 1...8 */
  device->data_source[OW_ADDRESS_LENGTH + 1] = 0xbe; /* read scratchpad */
  device->data_sink = scratchpad;
  return ow_device_start_operation(device);
}

int_fast8_t  ow_device_convert_temperature(struct ow_device *device) {
  if (device->state != OW_DEVICE_IDLE) {
    return -OW_ERROR_BUSY;
  }
  device->operation_count = ARRAY_SIZE(OW_READ_CONVERT_TEMPERATURE_OPERATIONS);
  device->operations = OW_READ_CONVERT_TEMPERATURE_OPERATIONS;
  device->data_source = device->buffer;
  device->data_source[0] = 0x55; /* match rom */
  /* address must be stored in bytes 1...8 */
  device->data_source[OW_ADDRESS_LENGTH + 1] = 0x44; /* convert temp */
  device->data_sink = NULL;
  return ow_device_start_operation(device);
}
