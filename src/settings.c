/* settings.c -- Main program settings source
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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "settings.h"

#ifdef ENABLE_NLS
# include <libintl.h>
# define _(s) gettext(s)
#else
# define _(s) s
#endif

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


void
settings_init(settings_t *settings)
{
  settings->temp_set = -1;
  settings->temp_day = -1;
  settings->temp_night = -1;
  settings->brightness_day = NAN;
  settings->brightness_night = NAN;
  settings->transition = -1;
  settings->transition_low = TRANSITION_LOW;
  settings->transition_high = TRANSITION_HIGH;
  settings->reload_transition = -1;
  settings->preserve_calibrations = -1;
}


void
settings_copy(settings_t *restrict dest, const settings_t *restrict src)
{
  memcpy(dest, src, sizeof(settings_t));
}


void
settings_finalize(settings_t *settings)
{
  if (settings->temp_day < 0)                settings->temp_day              = DEFAULT_DAY_TEMP;
  if (settings->temp_night < 0)              settings->temp_night            = DEFAULT_NIGHT_TEMP;
  if (isnan(settings->brightness_day))       settings->brightness_day        = DEFAULT_BRIGHTNESS;
  if (isnan(settings->brightness_night))     settings->brightness_night      = DEFAULT_BRIGHTNESS;
  if (settings->transition < 0)              settings->transition            = 1;
  if (settings->reload_transition < 0)       settings->reload_transition     = 1;
  if (settings->preserve_calibrations < 0)   settings->preserve_calibrations = 0;
}


int
settings_parse(settings_t *settings, const char* name, char* value, int mode)
{
	if (strcasecmp(name, "temp-day") == 0) {
		if (settings->temp_day < 0) settings->temp_day = atoi(value);
	} else if (strcasecmp(name, "temp-night") == 0) {
		if (settings->temp_night < 0) settings->temp_night = atoi(value);
	} else if (strcasecmp(name, "transition") == 0) {
		if (settings->transition < 0) settings->transition = !!atoi(value);
	} else if (strcasecmp(name, "reload-transition") == 0) {
		if (settings->reload_transition < 0) settings->reload_transition = !!atoi(value);
	} else if (strcasecmp(name, "brightness") == 0) {
		if (isnan(settings->brightness_day)) settings->brightness_day = atof(value);
		if (isnan(settings->brightness_night)) settings->brightness_night = atof(value);
	} else if (strcasecmp(name, "brightness-day") == 0) {
		if (isnan(settings->brightness_day)) settings->brightness_day = atof(value);
	} else if (strcasecmp(name, "brightness-night") == 0) {
		if (isnan(settings->brightness_night)) settings->brightness_night = atof(value);
	} else if (strcasecmp(name, "elevation-high") == 0) {
		settings->transition_high = atof(value);
	} else if (strcasecmp(name, "elevation-low") == 0) {
		settings->transition_low = atof(value);
	} else if (strcasecmp(name, "preserve-calibrations") == 0) {
		if (settings->preserve_calibrations < 0 && mode == PROGRAM_MODE_CONTINUAL) {
			settings->preserve_calibrations = !!atoi(value);
		}
	} else {
		return 1;
	}
	return 0;
}


int
settings_validate(settings_t *settings, int manual_mode, int reset_mode)
{
	int rc = 0;

	if (manual_mode) {
		/* Check color temperature to be set */
		if (settings->temp_set < MIN_TEMP || settings->temp_set > MAX_TEMP) {
			fprintf(stderr,
				_("Temperature must be between %uK and %uK.\n"),
				MIN_TEMP, MAX_TEMP);
			rc = -1;
		}
	} else if (reset_mode == 0) {
		/* Color temperature at daytime */
		if (settings->temp_day < MIN_TEMP || settings->temp_day > MAX_TEMP) {
			fprintf(stderr,
				_("Temperature must be between %uK and %uK.\n"),
				MIN_TEMP, MAX_TEMP);
		        rc = -1;
		}

		/* Color temperature at night */
		if (settings->temp_night < MIN_TEMP || settings->temp_night > MAX_TEMP) {
			fprintf(stderr,
				_("Temperature must be between %uK and %uK.\n"),
				MIN_TEMP, MAX_TEMP);
		        rc = -1;
		}

		/* Solar elevations */
		if (settings->transition_high < settings->transition_low) {
		        fprintf(stderr,
		                _("High transition elevation cannot be lower than"
				  " the low transition elevation.\n"));
		        rc = -1;
		}
	}

	/* Brightness */
	if (settings->brightness_day < MIN_BRIGHTNESS ||
	    settings->brightness_day > MAX_BRIGHTNESS ||
	    settings->brightness_night < MIN_BRIGHTNESS ||
	    settings->brightness_night > MAX_BRIGHTNESS) {
		fprintf(stderr,
			_("Brightness values must be between %.1f and %.1f.\n"),
			MIN_BRIGHTNESS, MAX_BRIGHTNESS);
		rc = -1;
	}

	return rc;
}


void
settings_interpolate(settings_t *out, settings_t low, settings_t high, double weight)
{
#define interpol(F) (low.F * (1.0 - weight) + high.F * weight)

  out->temp_set = (int)interpol(temp_set);
  out->temp_day = (int)interpol(temp_day);
  out->temp_night = (int)interpol(temp_night);
  out->brightness_day = (float)interpol(brightness_day);
  out->brightness_night = (float)interpol(brightness_night);
  out->transition_low = (float)interpol(transition_low);
  out->transition_high = (float)interpol(transition_high);

#undef interpol
}
