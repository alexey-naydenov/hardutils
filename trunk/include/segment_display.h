/* segment_display.h
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

/*! \file segment_display.h 
  Library for 7 segment displays.

  Display segment names:
  \verbatim
     A
    ---
  F|   |B
   | G |
    ---
  E|   |C
   | D |  DP
    ---   *
  \endverbatim
*/

#ifndef SEGMENT_DISPLAY_H_
#define SEGMENT_DISPLAY_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "segment_display_config.h"

#define SD_SEGMENTS_PER_DIGIT (8)

//! Convenience structure for mapping segments to pins.
struct sd_segment {
  sd_port_t port;
  sd_pin_t pin;
  sd_pin_config_t config;
};

enum sd_character {
  // numbers 0-F
  SD_CHARACTER_ZERO = 0, SD_CHARACTER_ONE, SD_CHARACTER_TWO, SD_CHARACTER_THREE,
  SD_CHARACTER_FOUR, SD_CHARACTER_FIVE, SD_CHARACTER_SIX, SD_CHARACTER_SEVEN,
  SD_CHARACTER_EIGHT, SD_CHARACTER_NINE, SD_CHARACTER_A, SD_CHARACTER_B,
  SD_CHARACTER_C, SD_CHARACTER_D, SD_CHARACTER_E, SD_CHARACTER_F,
  // minus
  SD_CHARACTER_MINUS,
  // individual segments
  SD_SEGMENT_NONE, SD_SEGMENT_A, SD_SEGMENT_B, SD_SEGMENT_C, SD_SEGMENT_D,
  SD_SEGMENT_E, SD_SEGMENT_F, SD_SEGMENT_G, SD_SEGMENT_DP
};

//! Stores segment display information.
/*! Contains mapping between segments and pins as well as display
  state information and function pointers.
 */
struct sd_display;
//! Create new structure that maps pins to segments.
/*! After creation the structure must be initialized with pin
  mappings and hardware access functions.
 */
int sd_new(struct sd_display **display);
//! Get reference already created display structure.
struct sd_display *sd_ref(struct sd_display *display);
//! Free reference to a display structure.
/*! If all references are freed then the display structure is
  destroyed.
 */
struct sd_display *sd_unref(struct sd_display *display);
//! Free reserved internal arrays and structure itself.
/*! This function should not be normally called, use sd_unref instead.
 */
void sd_free(struct sd_display *display);

//! Plug function that can be used instead of working pin init function.
void sd_init_plug(sd_port_t port, sd_pin_t pin, sd_pin_config_t cfg);
//! Plug function that can be used instead of working pin access function.
void sd_onoff_plug(sd_port_t port, sd_pin_t pin);
//! Set functions that init and toggle digit pins.
int sd_set_digit_functions(
    struct sd_display *display,
    void (*init_digit)(sd_port_t, sd_pin_t, sd_pin_config_t),
    void (*turn_on_digit)(sd_port_t, sd_pin_t),
    void (*turn_off_digit)(sd_port_t, sd_pin_t));
//! Set functions that init and toggle segment pins.
int sd_set_segment_functions(
    struct sd_display *display,
    void (*init_segment)(sd_port_t, sd_pin_t, sd_pin_config_t),
    void (*turn_on_segment)(sd_port_t, sd_pin_t),
    void (*turn_off_segment)(sd_port_t, sd_pin_t));
//! Set mapping between digits and pins.
int sd_set_digit_map(struct sd_display *display, int_fast8_t digit_count,
                     const struct sd_segment *digit_map);
//! Set mapping between segments and pins.
int sd_set_segment_map(struct sd_display *display,
                       const struct sd_segment *segment_map);
//! Init all display pins.
int sd_init(struct sd_display *display);
//! Set display data.
int sd_connect_display_to_data(struct sd_display *display, const enum sd_character *data);
//! Set dot position.
/*! Pass -1 to erase dot.
 */
int sd_set_dot_position(struct sd_display *display, int_fast8_t pos);
//! Get dot position.
/* Return -1 if dot is not shown.
 */
int_fast8_t sd_get_dot_position(struct sd_display *display);
//! Light up next digit.
int sd_show_next(struct sd_display *display);
//! Display an unsigned int at given location
int sd_display_uint(struct sd_display *display, int_fast8_t first_digit,
		    int_fast8_t last_digit, unsigned value);

#ifdef __cplusplus
} // extern C
#endif
#endif  // SEGMENT_DIPSLAY_H_
