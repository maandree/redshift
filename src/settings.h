/* settings.h -- Main program settings header
   This file is part of Redshift.

   Redshift is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Redshift is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Redshift.  If not, see <http://www.gnu.org/licenses/>.

   Copyright (c) 2013  Jon Lund Steffensen <jonlst@gmail.com>
   Copyright (c) 2014  Mattias Andrée <maandree@member.fsf.org>
*/

#ifndef REDSHIFT_SETTINGS_H
#define REDSHIFT_SETTINGS_H

#include "adjustments.h"
#include "redshift.h"


typedef struct
{
  int temp_set;
  int temp_day;
  int temp_night;
  float brightness_day;
  float brightness_night;
  int transition;
  float transition_low;
  float transition_high;
  int reload_transition;
  int preserve_calibrations;
  
} settings_t;


void settings_init(settings_t *settings);
void settings_copy(settings_t *restrict dest, const settings_t *restrict src);
void settings_finalize(settings_t *settings);
int settings_parse(settings_t *settings, const char* name, char* value, int mode);
int settings_validate(settings_t *settings, int manual_mode, int reset_mode);
void settings_interpolate(settings_t *out, settings_t low, settings_t high, double weight);


#endif /* ! REDSHIFT_SETTINGS_H */
