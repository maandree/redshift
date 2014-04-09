/* gamma-drm.c -- DRM gamma adjustment source
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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <alloca.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#ifdef ENABLE_NLS
# include <libintl.h>
# define _(s) gettext(s)
#else
# define _(s) s
#endif

#ifndef O_CLOEXEC
  #define O_CLOEXEC  02000000
#endif

#include "gamma-drm.h"
#include "colorramp.h"


static void
drm_free_partition(void *data)
{
	drm_card_data_t *card_data = data;
	if (card_data->res != NULL)
		drmModeFreeResources(card_data->res);
	if (card_data->fd >= 0)
		close(card_data->fd);
	free(data);
}

static void
drm_free_crtc(void *data)
{
	(void) data;
}

static int
drm_open_site(gamma_server_state_t *state, char *site, gamma_site_state_t *site_out)
{
	(void) state;
	(void) site;
	site_out->data = NULL;
	site_out->partitions_available = 1; /* FIXME */
	return 0;
}

static int
drm_open_partition(gamma_server_state_t *state, gamma_site_state_t *site,
		   size_t partition, gamma_partition_state_t *partition_out)
{
	(void) state;
	(void) site;

	partition_out->data = NULL;
	drm_card_data_t *data = malloc(sizeof(drm_card_data_t));
	if (data == NULL) {
		perror("malloc");
		return -1;
	}
	data->fd = -1;
	data->res = NULL;
	data->index = partition;
	partition_out->data = data;

	/* Acquire access to a graphics card. */
	char pathname[4096];
	snprintf(pathname, 4096, DRM_DEV_NAME, DRM_DIR_NAME, (int)partition);

	data->fd = open(pathname, O_RDWR | O_CLOEXEC);
	if (data->fd < 0) {
		/* TODO Check if access permissions, normally root or
		        membership of the video group is required.
			It could also be locked by e.g. a display server. */
		perror("open");
		return -1;
	}

	/* Acquire mode resources. */
	data->res = drmModeGetResources(data->fd);
	if (data->res == NULL) {
		fprintf(stderr, _("Failed to get DRM mode resources\n"));
		return -1;
	}

	partition_out->crtcs_available = data->res->count_crtcs;
	return 0;
}

static int
drm_open_crtc(gamma_server_state_t *state, gamma_site_state_t *site,
	      gamma_partition_state_t *partition, size_t crtc, gamma_crtc_state_t *crtc_out)
{
	drm_card_data_t *card = partition->data;
	int crtc_id = card->res->crtcs[crtc];
	crtc_out->data = (void*)(size_t)crtc_id;
	drmModeCrtc *crtc_info = drmModeGetCrtc(card->fd, crtc_id);
	if (crtc_info == NULL) {
		fprintf(stderr, _("Please do no unplug monitors!\n"));
		return -1;
	}

	ssize_t gamma_size = crtc_info->gamma_size;
	drmModeFreeCrtc(crtc_info);
	if (gamma_size < 2) {
		fprintf(stderr, _("Could not get gamma ramp size for CRTC %i\n"
				  "on graphics card %i.\n"),
			crtc, card->index);
		return -1;
	}
	crtc_out->saved_ramps.red_size   = (size_t)gamma_size;
	crtc_out->saved_ramps.green_size = (size_t)gamma_size;
	crtc_out->saved_ramps.blue_size  = (size_t)gamma_size;
	
	/* Valgrind complains about us reading uninitialize memory if we just use malloc. */
	crtc_out->saved_ramps.red   = calloc(3 * gamma_size, sizeof(uint16_t));
	crtc_out->saved_ramps.green = crtc_out->saved_ramps.red   + gamma_size;
	crtc_out->saved_ramps.blue  = crtc_out->saved_ramps.green + gamma_size;
	if (crtc_out->saved_ramps.red == NULL) {
		perror("malloc");
		return -1;
	}

	int r = drmModeCrtcGetGamma(card->fd, crtc_id, gamma_size, crtc_out->saved_ramps.red,
				    crtc_out->saved_ramps.green, crtc_out->saved_ramps.blue);
	if (r < 0) {
		fprintf(stderr, _("DRM could not read gamma ramps on CRTC %i on\n"
				  "graphics card %i.\n"),
			crtc, card->index);
		return -1;
	}

	return 0;
}

static void
drm_invalid_partition(gamma_site_state_t *site, size_t partition)
{
	fprintf(stderr, _("Card %d does not exist. "),
		partition);
	if (site->partitions_available > 1) {
  		fprintf(stderr, _("Valid cards are [0-%d].\n"),
			site->partitions_available - 1);
	} else {
		fprintf(stderr, _("Only card 0 exists.\n"),
			partition);
	}
}

static int
drm_set_ramps(gamma_server_state_t *state, gamma_crtc_state_t *crtc, gamma_ramps_t ramps)
{
	drm_card_data_t *card_data = state->sites[crtc->site_index].partitions[crtc->partition].data;
	drmModeCrtcSetGamma(card_data->fd, (int)(size_t)(crtc->data),
			    ramps.red_size, ramps.red, ramps.green, ramps.blue);

	/* Errors must be ignored, because we do not have
	   permission to do this will a display server is active. */
	return 0;
}

static int
drm_set_option(gamma_server_state_t *state, const char *key, const char *value, ssize_t section)
{
	if (strcasecmp(key, "card") == 0) {
		ssize_t card = strcasecmp(value, "all") ? (ssize_t)atoi(value) : -1;
		if (card < 0) {
			/* TRANSLATORS: `all' must not be translated. */
			fprintf(stderr, _("Card must be `all' or a non-negative integer.\n"));
			return -1;
		}
		state->selections[section].partition = card;
		return 0;
	} else if (strcasecmp(key, "crtc") == 0) {
		ssize_t crtc = strcasecmp(value, "all") ? (ssize_t)atoi(value) : -1;
		if (crtc < 0) {
			/* TRANSLATORS: `all' must not be translated. */
			fprintf(stderr, _("CRTC must be `all' or a non-negative integer.\n"));
			return -1;
		}
		state->selections[section].crtc = crtc;
		return 0;
	}
	return 1;
}

int
drm_init(gamma_server_state_t *state)
{
	int r;
	r = gamma_init(state);
	if (r != 0) return r;

	state->free_partition_data = drm_free_partition;
	state->free_crtc_data      = drm_free_crtc;
	state->open_site           = drm_open_site;
	state->open_partition      = drm_open_partition;
	state->open_crtc           = drm_open_crtc;
	state->invalid_partition   = drm_invalid_partition;
	state->set_ramps           = drm_set_ramps;
	state->set_option          = drm_set_option;

	return 0;
}

int
drm_start(gamma_server_state_t *state)
{
	return gamma_resolve_selections(state);
}

void
drm_print_help(FILE *f)
{
	fputs(_("Adjust gamma ramps with Direct Rendering Manager.\n"), f);
	fputs("\n", f);

	/* TRANSLATORS: DRM help output
	   left column must not be translated */
	fputs(_("  card=N\tGraphics card to apply adjustments to\n"
		"  crtc=N\tCRTC to apply adjustments to\n"), f);
	fputs("\n", f);
}
