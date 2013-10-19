/* segment_display.c
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

/*! \file segment_display.c 
  Library for 7 segment displays.
*/

#include <stdlib.h>
#include <string.h>

#include "segment_display.h"

static const unsigned char char_segment_patterns[] = {
  // numbers 0-F
  0x3f, 0x6, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x7,
  0x7f, 0x6f, 0x77, 0x7c, 0x39, 0x5e, 0x79, 0x71,
  // minus
  0x40,
  // individual segments
  0x0, 0x1, 0x2, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80 
};

struct sd_display {
  int_fast8_t refcount;
  int_fast8_t digit_count;
  int_fast8_t current_digit;
  int_fast8_t dot_position;
  enum sd_character *displayed_data;
  struct sd_segment *digit_pin_map;
  struct sd_segment *segment_pin_map;
  
  void (*init_digit)(sd_port_t, sd_pin_t, sd_pin_config_t);
  void (*turn_on_digit)(sd_port_t, sd_pin_t);
  void (*turn_off_digit)(sd_port_t, sd_pin_t);
  void (*init_segment)(sd_port_t, sd_pin_t, sd_pin_config_t);
  void (*turn_on_segment)(sd_port_t, sd_pin_t);
  void (*turn_off_segment)(sd_port_t, sd_pin_t);
};

void sd_init_plug(sd_port_t port, sd_pin_t pin, sd_pin_config_t cfg) {}
void sd_onoff_plug(sd_port_t port, sd_pin_t pin) {}

int sd_new(struct sd_display **display) {
  struct sd_display *new_display;
  new_display = calloc(1, sizeof(struct sd_display));
  if (!new_display) {
    return -1;
  }
  // init data fields
  new_display->refcount = 1;
  new_display->digit_count = -1;
  new_display->current_digit = 0;
  new_display->dot_position = -1;
  new_display->displayed_data = NULL;
  new_display->digit_pin_map = NULL;
  new_display->segment_pin_map = calloc(SD_SEGMENTS_PER_DIGIT,
                                        sizeof(struct sd_segment));
  if (!new_display->segment_pin_map) {
    sd_free(new_display);
    return -1;
  }
  // plug function pointers
  new_display->init_digit = sd_init_plug;
  new_display->turn_on_digit = sd_onoff_plug;
  new_display->turn_off_digit = sd_onoff_plug;
  new_display->init_segment = sd_init_plug;
  new_display->turn_on_segment = sd_onoff_plug;
  new_display->turn_off_segment = sd_onoff_plug;
  // object constructed
  *display = new_display;
  return 0;
}

struct sd_display *sd_ref(struct sd_display *display) {
  if (display == NULL) {
    return NULL;
  }
  display->refcount++;
  return display;
}

struct sd_display *sd_unref(struct sd_display *display) {
  if (display == NULL) {
    return NULL;
  }
  display->refcount--;
  if (display->refcount > 0) {
    return display;
  }
  sd_free(display);
  return NULL;
}

void sd_free(struct sd_display *display) {
  if (display == NULL) {
    return;
  }
  free(display->displayed_data);
  free(display->digit_pin_map);
  free(display->segment_pin_map);
  free(display);
}

int sd_set_digit_functions(
    struct sd_display *display,
    void (*init_digit)(sd_port_t, sd_pin_t, sd_pin_config_t),
    void (*turn_on_digit)(sd_port_t, sd_pin_t),
    void (*turn_off_digit)(sd_port_t, sd_pin_t)) {
  if (display == NULL || init_digit == NULL || turn_on_digit == NULL
      || turn_off_digit == NULL) {
    return -1;
  }
  display->init_digit = init_digit;
  display->turn_on_digit = turn_on_digit;
  display->turn_off_digit = turn_off_digit;
  return 0;
}

int sd_set_segment_functions(
    struct sd_display *display,
    void (*init_segment)(sd_port_t, sd_pin_t, sd_pin_config_t),
    void (*turn_on_segment)(sd_port_t, sd_pin_t),
    void (*turn_off_segment)(sd_port_t, sd_pin_t)) {
  if (display == NULL || init_segment == NULL || turn_on_segment == NULL
      || turn_off_segment == NULL) {
    return -1;
  }
  display->init_segment = init_segment;
  display->turn_on_segment = turn_on_segment;
  display->turn_off_segment = turn_off_segment;
  return 0;
}

int sd_set_digit_map(struct sd_display *display, int_fast8_t digit_count,
                     const struct sd_segment *digit_map) {
  int i;
  /* if (display == NULL || digit_map == NULL || digit_count <= 0) { */
  /*   return -1; */
  /* } */
  //assert(display->digit_pin_map == NULL);
  display->digit_pin_map = calloc(digit_count, sizeof(struct sd_segment));
  if (!display->digit_pin_map) {
    return -1;
  }
  /* display->displayed_data = calloc(digit_count, sizeof(enum sd_character)); */
  /* if (!display->displayed_data) { */
  /*   return -1; */
  /* } */
  display->digit_count = digit_count;
  memcpy(display->digit_pin_map, digit_map,
         digit_count*sizeof(struct sd_segment));
  for (i = 0; i < digit_count; ++i) {
    display->displayed_data[i] = SD_SEGMENT_NONE;
  }
  return 0;
}

int sd_set_segment_map(struct sd_display *display,
                       const struct sd_segment *segment_map) {
  /* if (display == NULL || segment_map == NULL) { */
  /*   return -1; */
  /* } */
  //assert(display->segment_pin_map == NULL);
  memcpy(display->segment_pin_map, segment_map,
         SD_SEGMENTS_PER_DIGIT*sizeof(struct sd_segment));
  return 0;
}

int sd_init(struct sd_display *display) {
  int i;
  /* if (display == NULL || display->digit_pin_map == NULL */
  /*     || display->segment_pin_map == NULL) { */
  /*   return -1; */
  /* } */
  /* if (display->current_digit >= 0) { */
  /*   return -1; */
  /* } */
  // init digits
  for (i = 0; i < display->digit_count; ++i) {
    display->init_digit(display->digit_pin_map[i].port,
                        display->digit_pin_map[i].pin,
                        display->digit_pin_map[i].config);
  }
  // init segments
  for (i = 0; i < SD_SEGMENTS_PER_DIGIT; ++i) {
    display->init_segment(display->segment_pin_map[i].port,
                        display->segment_pin_map[i].pin,
                        display->segment_pin_map[i].config);
  }
  return 0;
}

int sd_connect_display_to_data(struct sd_display *display, enum sd_character *data) {
  /* if (display == NULL || display->digit_count <= 0) { */
  /*   return -1; */
  /* } */
  /* memcpy(display->displayed_data, data,  */
  /* 	 display->digit_count*sizeof(enum sd_character)); */
  display->displayed_data = data;
  return 0;
}

int sd_set_dot_position(struct sd_display *display, int_fast8_t pos) {
  if (display == NULL) {
    return -1;
  }
  display->dot_position = pos;
}

int sd_show_next(struct sd_display *display) {
  int seg;
  unsigned char current_char;
  if (display == NULL) {
    return -1;
  }
  // turn off current digit
  display->turn_off_digit(
           display->digit_pin_map[display->current_digit].port,
           display->digit_pin_map[display->current_digit].pin);
  for (seg = 0; seg < SD_SEGMENTS_PER_DIGIT; ++seg) {
    display->turn_off_segment(
           display->segment_pin_map[seg].port,
           display->segment_pin_map[seg].pin);  
  }
  // increment to next digit
  display->current_digit = display->current_digit + 1;
  if (display->current_digit == display->digit_count) {
    display->current_digit = 0;
  }
  // show digit
  display->turn_on_digit(
           display->digit_pin_map[display->current_digit].port,
           display->digit_pin_map[display->current_digit].pin);
  current_char = char_segment_patterns[display->displayed_data[display->current_digit]];
  for (seg = 0; seg < SD_SEGMENTS_PER_DIGIT; ++seg) {
    if (current_char & 0x1) {
      display->turn_on_segment(
           display->segment_pin_map[seg].port,
           display->segment_pin_map[seg].pin);  
    } else {
      display->turn_off_segment(
           display->segment_pin_map[seg].port,
           display->segment_pin_map[seg].pin);  
    }
    current_char >>=1;
  }
  if (display->dot_position == display->current_digit) {
    display->turn_on_segment(display->segment_pin_map[7].port,
			     display->segment_pin_map[7].pin);
  }
  return 0;
}

int sd_display_uint(struct sd_display *display, int_fast8_t first_digit,
		    int_fast8_t last_digit, unsigned value) {
  uint_fast8_t i;
  for (i = first_digit; i <= last_digit; ++i) {
    display->displayed_data[i] = value % 10;
    value /= 10;
  }
}

int sd_display_int(struct sd_display *display, int_fast8_t first_digit,
		   int_fast8_t last_digit, int value) {
  if (value >= 0) {
    display->displayed_data[last_digit] = SD_SEGMENT_NONE;
    return sd_display_uint(display, first_digit, last_digit - 1, value);
  } else {
    display->displayed_data[last_digit] = SD_CHARACTER_MINUS;
    return sd_display_uint(display, first_digit, last_digit - 1, -value);
  }
}
