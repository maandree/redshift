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

#include "gamma-common.h"
#include "adjustments.h"
#include "colorramp.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef ENABLE_NLS
# include <libintl.h>
# define _(s) gettext(s)
#else
# define _(s) s
#endif



/* Initialise the adjustment method common parts of a state, 
   this should be done before initialise the adjustment method
   specific parts. */
int
gamma_init(gamma_server_state_t *state)
{
	state->data = NULL;
	state->sites_used = 0;
	state->sites = NULL;
	state->selections_made = 1;
	state->selections = malloc(sizeof(gamma_selection_state_t));

	if (state->selections == NULL) {
		perror("malloc");
		state->selections_made = 0;
		return -1;
	}

	/* Defaults selection */
	state->selections->crtc = -1;
	state->selections->partition = -1;
	state->selections->site = NULL;
	state->selections->settings.gamma_correction[0] = DEFAULT_GAMMA;
	state->selections->settings.gamma_correction[1] = DEFAULT_GAMMA;
	state->selections->settings.gamma_correction[2] = DEFAULT_GAMMA;
	state->selections->settings.lut_calibration = NULL;
	state->selections->settings.gamma = DEFAULT_GAMMA;
	state->selections->settings.brightness = DEFAULT_BRIGHTNESS;
	state->selections->settings.temperature = NEUTRAL_TEMP;
	state->selections->settings.lut_pre = NULL;
	state->selections->settings.lut_post = NULL;

	return 0;
}


/* Free all CRTC selection data in a state. */
void
gamma_free_selections(gamma_server_state_t *state)
{
	size_t i;

	/* Free data in each selection. */
	for (i = 0; i < state->selections_made; i++)
		if (state->selections[i].site != NULL)
			free(state->selections[i].site);
	state->selections_made = 0;

	/* Free the selection array. */
	if (state->selections != NULL) {
		free(state->selections);
		state->selections = NULL;
	}
}

/* Free all data in a state. */
void
gamma_free(gamma_server_state_t *state)
{
	size_t s, p, c;
	gamma_site_state_t *site;
	gamma_partition_state_t *partition;
	gamma_crtc_state_t *crtc;

	/* Free selections. */
	gamma_free_selections(state);

	/* Free each site. */
	for (s = 0; s < state->sites_used; s++) {
		site = state->sites + s;
		/* Free each partition. */
		for (p = 0; p < site->partitions_available; p++) {
			partition = site->partitions + p;
			if (partition->used == 0)
				continue;
			/* Free each CRTC. */
			for (c = 0; c < partition->crtcs_used; c++) {
				crtc = partition->crtcs + c;

				/* Free lookup tables with adjustments. */
				if (crtc->settings.lut_pre != NULL) {
					if (crtc->settings.lut_pre->red != NULL)
						free(crtc->settings.lut_pre->red);
					free(crtc->settings.lut_pre);
				}
				if (crtc->settings.lut_post != NULL) {
					if (crtc->settings.lut_post->red != NULL)
						free(crtc->settings.lut_post->red);
					free(crtc->settings.lut_post);
				}

				/* Free gamma ramps. */
				if (crtc->saved_ramps.red != NULL)
					free(crtc->saved_ramps.red);
				if (crtc->current_ramps.red != NULL)
					free(crtc->current_ramps.red);

				/* Free method dependent CRTC data. */
				if (crtc->data != NULL)
					state->free_crtc_data(crtc->data);
			}
			/* Free CRTC array. */
			if (partition->crtcs != NULL)
				free(partition->crtcs);

			/* Free method dependent partition data. */
			if (partition->data != NULL)
				state->free_partition_data(partition->data);
		}
		/* Free partition array. */
		if (site->partitions != NULL)
			free(site->partitions);

		/* Free site identifier. */
		if (site->site != NULL)
			free(site->site);

		/* Free method dependent site data. */
		if (site->data != NULL)
			state->free_site_data(site->data);
	}
	/* Free site array. */
	if (state->sites != NULL) {
		free(state->sites);
		state->sites = NULL;
	}

	/* Free method dependent state data. */
	if (state->data != NULL) {
		state->free_state_data(state->data);
		state->data = NULL;
	}
}


/* Create CRTC iterator. */
gamma_iterator_t
gamma_iterator(gamma_server_state_t *state)
{
	gamma_iterator_t iterator = {
		.crtc      = NULL,
		.partition = NULL,
		.site      = NULL,
		.state     = state
	};
	return iterator;
}

/* Get next CRTC. */
int
gamma_iterator_next(gamma_iterator_t *iterator)
{
	/* First CRTC. */
	if (iterator->crtc == NULL) {
		iterator->site      = iterator->state->sites;
		iterator->partition = iterator->site->partitions;
		while (iterator->partition->used == 0)
			iterator->partition++;
		iterator->crtc = iterator->partition->crtcs;
		return 1;
	}

	/* Next CRTC. */
	size_t crtc_i      = iterator->crtc->crtc + 1;
	size_t partition_i = iterator->crtc->partition;
	size_t site_i      = iterator->crtc->site_index;

	if (crtc_i == iterator->partition->crtcs_used) {
		crtc_i = 0;
		partition_i += 1;
	}
next_partition:
	if (partition_i == iterator->site->partitions_available) {
		partition_i = 0;
		site_i += 1;
	}
	if (site_i == iterator->state->sites_used)
		return 0;
	if (iterator->state->sites[site_i].partitions[partition_i].used == 0) {
		partition_i += 1;
		goto next_partition;
	}

	iterator->site      = iterator->state->sites     + site_i;
	iterator->partition = iterator->site->partitions + partition_i;
	iterator->crtc      = iterator->partition->crtcs + crtc_i;
	return 1;
}


/* Find the index of a site or the index for a new site. */
size_t
gamma_find_site(gamma_server_state_t *state, char *site)
{
	size_t site_index;

	/* Find matching already opened site. */
	for (site_index = 0; site_index < state->sites_used; site_index++) {
		char *test_site = state->sites[site_index].site;
		if (test_site == NULL || site == NULL) {
			if (test_site == NULL && site == NULL)
				break;
		}
		else if (!strcmp(site, test_site))
			break;
	}

	return site_index;
}


/* Resolve selections. */
int
gamma_resolve_selections(gamma_server_state_t *state)
{
	int default_selection = state->selections_made == 1;
	int r;

	/* Shift the selections so that the iteration finds the
	   default selection if no other selection is made. */
	if (default_selection) {
		state->selections_made += 1;
		state->selections--;
	}

	for (size_t i = 1; i < state->selections_made; i++) {
		gamma_selection_state_t *selection = state->selections + i;
		gamma_site_state_t *site;
		size_t site_index;
		size_t partition_start;
		size_t partition_end;

		/* Find matching already opened site. */
		site_index = gamma_find_site(state, selection->site);

		/* Open site if not found. */
		if (site_index == state->sites_used) {
			/* Grow array with sites, we temporarily store the new array
			   a temporarily variable so that we can properly release
			   resources on error. */
			gamma_site_state_t *new_sites;
			size_t alloc_size = (site_index + 1) * sizeof(gamma_site_state_t);
			new_sites = state->sites != NULL ?
					realloc(state->sites, alloc_size) :
					malloc(alloc_size);
			if (new_sites == NULL) {
				perror(state->sites != NULL ? "realloc" : "malloc");
				return -1;
			}
			state->sites = new_sites;
			site = state->sites + site_index;

			r = state->open_site(state, selection->site, site);
			site->site = selection->site;
			if (r != 0) return r;

			/* calloc is used so that `used` in each partition is set to false. */
			site->partitions = calloc(site->partitions_available,
						  sizeof(gamma_partition_state_t));
			if (site->partitions == NULL) {
				site->partitions_available = 0;
				perror(state->sites != NULL ? "realloc" : "malloc");
				return -1;
			}

			/* Increment this last, so we do not get segfault on error. */
			state->sites_used += 1;
		} else {
			site = state->sites + site_index;
		}

		/* Select partitions. */
		if (selection->partition >= site->partitions_available) {
			state->invalid_partition(site, selection->partition);
			return -1;
		}
		partition_start = selection->partition < 0 ? 0 : selection->partition;
		partition_end = selection->partition < 0 ? site->partitions_available : partition_start + 1;
		/* Open partitions. */
		for (size_t p = partition_start; p < partition_end; p++) {
			gamma_partition_state_t *partition = site->partitions + p;
			if (partition->used) continue;

			r = state->open_partition(state, site, p, partition);
			if (r != 0) return r;

			partition->used = 1;
		}

		/* Open CRTC:s. */
		for (size_t p = partition_start; p < partition_end; p++) {
			gamma_partition_state_t *partition = site->partitions + p;
			size_t crtc_start = selection->crtc < 0 ? 0 : selection->crtc;
			size_t crtc_end = selection->crtc < 0 ? partition->crtcs_available : crtc_start + 1;

			if (selection->crtc >= partition->crtcs_available) {
				fprintf(stderr, _("CRTC %d does not exist. "),
					selection->crtc);
				if (partition->crtcs_available > 1) {
					fprintf(stderr, _("Valid CRTCs are [0-%d].\n"),
						partition->crtcs_available - 1);
				} else {
					fprintf(stderr, _("Only CRTC 0 exists.\n"));
				}
				return -1;
			}

			/* Grow array with selected CRTC:s, we temporarily store
			   the new array a temporarily variable so that we can
			   properly release resources on error. */
			gamma_crtc_state_t *new_crtcs;
			size_t alloc_size = partition->crtcs_used + crtc_end - crtc_start;
			alloc_size *= sizeof(gamma_crtc_state_t);
			new_crtcs = partition->crtcs != NULL ?
					realloc(partition->crtcs, alloc_size) :
					malloc(alloc_size);
			if (new_crtcs == NULL) {
				perror(partition->crtcs != NULL ? "realloc" : "malloc");
				return -1;
			}
			partition->crtcs = new_crtcs;

			for (size_t c = crtc_start; c < crtc_end; c++) {
				gamma_crtc_state_t *crtc = partition->crtcs;
				size_t total_ramp_size = 0, rrs, grs;
				uint16_t *ramps;

				r = state->open_crtc(state, site, partition, c, crtc);
				if (r != 0) return r;
				crtc->crtc = c;
				crtc->partition = p;
				crtc->site_index = site_index;
				partition->crtcs_used += 1;

				/* Store adjustment settigns. */
				crtc->settings = selection->settings;

				/* Create crtc->current_ramps. */
				crtc->current_ramps = crtc->saved_ramps;
				total_ramp_size += rrs = crtc->current_ramps.red_size;
				total_ramp_size += grs = crtc->current_ramps.green_size;
				total_ramp_size +=       crtc->current_ramps.blue_size;
				total_ramp_size *= sizeof(uint16_t);
				ramps = malloc(total_ramp_size);
				crtc->current_ramps.red = ramps;
				if (ramps == NULL) {
					perror("malloc");
					return -1;
				}
				crtc->current_ramps.red   = ramps;
				crtc->current_ramps.green = ramps + rrs;
				crtc->current_ramps.blue  = ramps + rrs + grs;
			}
		}
	}

	/* Undo the shift made in the beginning of this function. */
	if (default_selection) {
		state->selections_made -= 1;
		state->selections++;
	}

	gamma_free_selections(state);
	return 0;
}


/* Restore gamma ramps. */
void
gamma_restore(gamma_server_state_t *state)
{
	gamma_iterator_t iter = gamma_iterator(state);
	while (gamma_iterator_next(&iter)) {
		state->set_ramps(state, iter.crtc, iter.crtc->saved_ramps);
	}
}


/* Update gamma ramps. */
void
gamma_update(gamma_server_state_t *state)
{
	gamma_iterator_t iter = gamma_iterator(state);
	while (gamma_iterator_next(&iter)) {
		colorramp_fill(iter.crtc->current_ramps, iter.crtc->settings);
		state->set_ramps(state, iter.crtc, iter.crtc->current_ramps);
	}
}


/* Parse and apply an option. */
int
gamma_set_option(gamma_server_state_t *state, const char *key, const char *value, int section)
{
	int r;

	if (section == state->selections_made) {
		/* Grow array with selections, we temporarily store
		   the new array a temporarily variable so that we can
		   properly release resources on error. */
		gamma_selection_state_t *new_selections;
		size_t alloc_size = state->selections_made + 1;
		alloc_size *= sizeof(gamma_selection_state_t);
		new_selections = realloc(state->selections, alloc_size);
		if (new_selections == NULL) {
			perror("realloc");
			return -1;
		}
		state->selections = new_selections;

		/* Copy default selection. */
		state->selections[section] = *(state->selections);

		/* Increment this last, so we do not get segfault on error. */
		state->selections_made += 1;
	}

	if (strcasecmp(key, "gamma") == 0) {
		float gamma[3];
		if (parse_gamma_string(value, gamma) < 0) {
			fputs(_("Malformed gamma setting.\n"),
			      stderr);
			return -1;
		}
#ifdef MAX_GAMMA
		if (gamma[0] < MIN_GAMMA || gamma[0] > MAX_GAMMA ||
		    gamma[1] < MIN_GAMMA || gamma[1] > MAX_GAMMA ||
		    gamma[2] < MIN_GAMMA || gamma[2] > MAX_GAMMA) {
			fprintf(stderr,
				_("Gamma value must be between %.1f and %.1f.\n"),
				MIN_GAMMA, MAX_GAMMA);
			return -1;
		}
#else
		if (gamma[0] < MIN_GAMMA ||
		    gamma[1] < MIN_GAMMA ||
		    gamma[2] < MIN_GAMMA) {
			fprintf(stderr,
				_("Gamma value must be atleast %.1f.\n"),
				MIN_GAMMA);
			return -1;
		}
#endif
		state->selections[section].settings.gamma_correction[0] = gamma[0];
		state->selections[section].settings.gamma_correction[1] = gamma[1];
		state->selections[section].settings.gamma_correction[2] = gamma[2];
	} else {
		r = state->set_option(state, key, value, section);
		if (r <= 0)
			return r;
		fprintf(stderr, _("Unknown method parameter: `%s'.\n"), key);
		return -1;
	}
	return 0;
}


/* Methods for updating adjustments on all CRTC:s. */

static gamma_crtc_selection_t all_crtcs = {
	.site = -1,
	.partition = -1,
	.crtc = -1
};

#define __gamma_update_all(PROP)						\
	void									\
	gamma_update_all_##PROP(gamma_server_state_t *state, float PROP)	\
	{									\
		gamma_update_##PROP(state, all_crtcs, PROP);			\
	}

__gamma_update_all(gamma)
__gamma_update_all(brightness)
__gamma_update_all(temperature)

#undef __gamma_update_all


/* Methods for updating adjustments on selected CRTC:s. */

#define __test(ANCESTOR, THIS)  (crtcs.THIS < 0 || iter.THIS == iter.ANCESTOR->THIS##s + crtcs.THIS)
#define __test_all()            (__test(state, site) && __test(site, partition) && __test(partition, crtc))

#define __update(PROP)									\
	if (crtcs.site < 0 || crtcs.partition < 0 || crtcs.crtc < 0) {			\
		gamma_iterator_t iter = gamma_iterator(state);				\
		if (crtcs.site < 0 && crtcs.partition < 0 && crtcs.crtc < 0) {		\
			while (gamma_iterator_next(&iter))				\
				iter.crtc->settings.PROP = PROP;			\
		} else {								\
			while (gamma_iterator_next(&iter))				\
				if (__test_all())					\
					iter.crtc->settings.PROP = PROP;		\
		}									\
	} else if (crtcs.site < state->sites_used) {					\
		gamma_site_state_t site = state->sites[crtcs.site];			\
		gamma_partition_state_t partition;					\
		if (crtcs.partition < site.partitions_available) {			\
			partition = site.partitions[crtcs.partition];			\
			if (partition.used && crtcs.crtc < partition.crtcs_used)	\
				partition.crtcs[crtcs.crtc].settings.PROP = PROP;	\
		}									\
	}

void
gamma_update_gamma(gamma_server_state_t *state, gamma_crtc_selection_t crtcs, float gamma)
{
	if (gamma < MIN_GAMMA) gamma = MIN_GAMMA;
#ifdef MAX_BRIGHTNESS
	if (gamma > MAX_GAMMA) gamma = MAX_GAMMA;
#endif
	__update(gamma)
}

void
gamma_update_brightness(gamma_server_state_t *state, gamma_crtc_selection_t crtcs, float brightness)
{
	if (brightness < MIN_BRIGHTNESS) brightness = MIN_BRIGHTNESS;
#ifdef MAX_BRIGHTNESS
	if (brightness > MAX_BRIGHTNESS) brightness = MAX_BRIGHTNESS;
#endif
	__update(brightness)
}

void
gamma_update_temperature(gamma_server_state_t *state, gamma_crtc_selection_t crtcs, float temperature)
{
	if (temperature < MIN_TEMP) temperature = MIN_TEMP;
	if (temperature > MAX_TEMP) temperature = MAX_TEMP;
	__update(temperature)
}

#undef __update
#undef __test_all
#undef __test


/* A gamma string contains either one floating point value,
   or three values separated by colon. */
int
parse_gamma_string(const char *str, float gamma[3])
{
	char *s = strchr(str, ':');
	if (s == NULL) {
		/* Use value for all channels */
		float g = atof(str);
		gamma[0] = gamma[1] = gamma[2] = g;
	} else {
		/* Parse separate value for each channel */
		*(s++) = '\0';
		char *g_s = s;
		s = strchr(s, ':');
		if (s == NULL) return -1;

		*(s++) = '\0';
		gamma[0] = atof(str); /* Red */
		gamma[1] = atof(g_s); /* Blue */
		gamma[2] = atof(s); /* Green */
	}

	return 0;
}

