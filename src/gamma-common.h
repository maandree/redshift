/* gamma-drm.h -- DRM gamma adjustment header
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

   Copyright (c) 2014  Mattias Andrée <maandree@member.fsf.org>
*/

#ifndef REDSHIFT_GAMMA_COMMON_H
#define REDSHIFT_GAMMA_COMMON_H

#include <stdint.h>
#include <unistd.h>



typedef void gamma_data_free_func(void *data);

typedef int gamma_open_site_func(gamma_state_t *state,
				 char *site, gamma_site_state_t *site_out);

typedef int gamma_open_partition_func(gamma_state_t *state,
				      gamma_site_state_t *site,
				      size_t partition, gamma_partition_state_t *partition_out);

typedef int gamma_open_crtc_func(gamma_state_t *state,
				 gamma_site_state_t *site,
				 gamma_partition_state_t *partition,
				 size_t crtc, gamma_crtc_state_t *crtc_out);

typedef void gamma_invalid_partition_func(gamma_site_state_t *site, size_t partition);

typedef int gamma_set_ramps_func(gamma_state_state_t *state, gamma_crtc_state_t *crtc, gamma_ramps_t ramps);



/* Gamma ramp trio */
typedef struct {
	/* The number of stops in each ramp */
	size_t red_size;
	size_t green_size;
	size_t blue_size;
	/* The actual ramps */
	uint16_t *red;
	uint16_t *green;
	uint16_t *blue;
} gamma_ramps_t;

/* Colour adjustment settings */
typedef struct {
	float gamma[3];
	float brightness;
	float temperature;
} gamma_settings_t;

/* CRTC state */
typedef struct {
	/* Adjustment method implementation specific data */
	void *data;
	/* The CRTC, partition (e.g. screen) and site (e.g. display) indices */
	size_t crtc;
	size_t partition;
	size_t site_index;
	/* Saved (restored to on exit) and
	   current (about to be applied) gamma ramps */
	gamma_ramps_t saved_ramps;
	gamma_ramps_t current_ramps;
	/* Colour adjustments */
	gamma_settings_t settings;
} gamma_crtc_state_t;

/* Partition (e.g. screen) state */
typedef struct {
	/* Whether this partion is used */
	int used;
	/* Adjustment method implementation specific data */
	void *data;
	/* The number of CRTC:s that is available on this site */
	size_t crtcs_available;
	/* The selected CRTC:s */
	size_t crtcs_used;
	gamma_crtc_state_t *crtcs;
} gamma_partition_state_t;

/* Site (e.g. display) state */
typedef struct {
	/* Adjustment method implementation specific data */
	void *data;
	/* The site identifier */
	char *site;
	/* The number of partitions that is available on this site */
	size_t partitions_available;
	/* The partitions */
	gamma_partition_state_t *partitions;
} gamma_site_state_t;

/* CRTC selection state */
typedef struct {
	/* The CRTC, partition (e.g. screen) and site (e.g. display) indices */
	ssize_t crtc;
	ssize_t partition;
	size_t site_index;
	/* The site identifier */
	char *site;
	/* Colour adjustments */
	gamma_settings_t settings;
} gamma_selection_state_t;

/* Method state */
typedef struct {
	/* Adjustment method implementation specific data */
	void *data;
	/* The selected sites */
	size_t sites_used;
	gamma_site_state_t *sites;
	/* The selections, zeroth determines the defaults */
	size_t selections_made;
	gamma_site_state_t *selections;
	/* Functions that releases adjustment method implementation specific data */
	gamma_data_free_func *free_state_data;
	gamma_data_free_func *free_site_data;
	gamma_data_free_func *free_partition_data;
	gamma_data_free_func *free_crtc_data;
	/* Functions that open sites, partitions and CRTC:s */
	gamma_open_site_func *open_site;
	gamma_open_partition_func *open_partition;
	gamma_open_crtc_func *open_crtc;
	/* Function that inform about invalid selection of partition */
	gamma_invalid_partition_func *invalid_partition;
	/*Function that applies a gamma ramp */
	gamma_set_ramps_func *set_gamma;
} gamma_state_t;


/* CRTC iterator */
typedef struct {
	/* The current CRTC, partition and site */
	gamma_crtc_state_t *crtc;
	gamma_partition_state_t *partition;
	gamma_site_state_t *site;
	/* The gamma state whose CRTC:s are being iterated */
	gamma_state_t *state;
} gamma_iterator_t;



/* Free all CRTC selection data in a state */
void gamma_free_selections(gamma_state_t *state);

/* Free all data in a state */
void gamma_free(gamma_state_t *state);


/* Create CRTC iterator */
gamma_iterator_t gamma_iterator(gamma_state_t *state);

/* Get next CRTC */
int gamma_iterator_next(gamma_iterator_t *iterator);


/* Resolve selections */
int gamma_resolve_selections(gamma_state_t *state);


/* Restore gamma ramps */
void gamma_restore(gamma_state_t *state);

/* Update gamma ramps */
void gamma_update(gamma_state_t *state);



#endif /* ! REDSHIFT_GAMMA_COMMON_H */
