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


/* Free all CRTC selection data in a state */
void gamma_common_free_selections(gamma_state_t* state)
{
	size_t i;

	/* Free data in each selection */
	for (i = 0; i < state->selections_made; i++)
		if (state->selections_made[i].site != NULL)
			free(state->selections_made[i].site);
	state->selections_made = 0;

	/* Free the selection array */
	if (state->selections != NULL) {
		free(state->selections);
		state->selections = NULL;
	}
}

/* Free all data in a state */
void gamma_common_free(gamma_state_t* state)
{
	size_t s, p, c;
	gamma_site_state_t *site;
	gamma_partition_state_t *partition;
	gamma_crtc_state_t *crtc;

	/* Free selections */
	gamma_common_free_selections(state);

	/* Free each site */
	for (s = 0; s < state->sites_used; s++) {
		site = state->sites + s;
		/* Free each partition */
		for (p = 0; p < site->partitions_used; p++) {
			partition = site->partitions + p;
			/* Free each CRTC */
			for (c = 0; c < partition->crtcs_used; c++) {
				crtc = partition->crtcs + c;

				/* Free gamma ramps */
				if (crtc->saved_ramps.red != NULL)
					free(crtc->saved_ramps.red);
				if (crtc->current_ramps.red != NULL)
					free(crtc->current_ramps.red);

				/* Free method dependent CRTC data */
				if (crtc->data != NULL)
					state->free_crtc_data(crtc->data);
			}
			/* Free CRTC array */
			if (partition->crtcs != NULL)
				free(partition->crtcs);

			/* Free method dependent partition data */
			if (partition->data != NULL)
				state->free_partition_data(partition->data);
		}
		/* Free partition array */
		if (site->partitions != NULL)
			free(site->partitions);

		/* Free site identifier */
		if (site->site != NULL)
			free(site->site);

		/* Free method dependent site data */
		if (site->data != NULL)
			state->free_site_data(site->data);
	}
	/* Free site array */
	if (state->sites != NULL) {
		free(state->sites);
		state->sites = NULL;
	}

	/* Free method dependent state data */
	if (state->data != NULL) {
		state->free_state_data(state->data);
		state->data = NULL;
	}
}


/* Create CRTC iterator */
gamma_iterator_t gamma_iterator(gamma_state_t* state)
{
	gamma_iterator_t iterator = {
		.crtc      = state->sites->partitions->crtcs,
		.partition = state->sites->partitions,
		.site      = state->sites,
		.state     = state
	}
	return iterator;
}

/* Get next CRTC */
int gamma_iterator_next(gamma_iterator_t* iterator)
{
	size_t crtc_i      = iterator->crtc->crtc + 1;
	size_t partition_i = iterator->crtc->partition;
	size_t site_i      = iterator->crtc->site_index;

	if (crtc_i == iterator->partition->crtcs_used) {
		crtc_i = 0;
		partition_i += 1;
	}
	if (partition_i == iterator->site->partitions_used) {
		partition_i = 0;
		site_i += 1;
	}
	if (site_i == iterator->state->sites_used)
		return 0;

	iterator->site      = iterator->state->sites     + site_i;
	iterator->partition = iterator->site->partitions + partition_i;
	iterator->crtc      = iterator->partition->crtcs + crtc_i;
	return 1;
}

