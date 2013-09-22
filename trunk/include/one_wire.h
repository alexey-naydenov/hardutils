/* one_wire.h
 *
 * Copyright (C) 2013 Alexey Naydenov <alexey.naydenovREMOVETHIS@linux.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*! \file one_wire.h 
  One wire library.
*/

#ifndef ONE_WIRE_H_
#define ONE_WIRE_H_

#include <stdint.h>

#include <timer_delay.h>

/*! 1wire bus object. */
struct ow_bus;
/* Create and destruction. */
int8_fast_t ow_bus_new(struct ow_bus **bus);
struct ow_bus *ow_bus_ref(struct ow_bus *bus);
struct ow_bus *ow_bus_unref(struct ow_bus *bus);
void ow_bus_free(struct ow_bus *bus);
/* Init functions. */
int8_fast_t ow_bus_set_timer(struct ow_bus *bus, struct td_timer *timer);
int8_fast_t ow_bus_set_output_fn(struct ow_bus *bus, void (*output_fn)(void));
int8_fast_t ow_bus_set_input_fn(struct ow_bus *bus, void (*input_fn)(void));
int8_fast_t ow_bus_set_pull_up_fn(struct ow_bus *bus, void (*pull_up_fn)(void));
int8_fast_t ow_bus_set_pull_down_fn(struct ow_bus *bus, void (*pull_down_fn)(void));
int8_fast_t ow_bus_set_read_fn(struct ow_bus *bus, uint8_fast_t (*read_fn)(void));
/* Interface functions. */

#define OW_ADDRESS_LENGTH (8)

/*! Stores address of 1 wire device. */
struct ow_device;
/* Create and destruction. */
int8_fast_t ow_device_new(struct ow_device **device);
struct ow_device *ow_device_ref(struct ow_device *device);
struct ow_device *ow_device_unref(struct ow_device *device);
void ow_device_free(struct ow_device *device);

#endif /* ONE_WIRE_H_ */