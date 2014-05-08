/* quartz-test.c -- Mac OS X Quartz gamma adjustment test source
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
#include <string.h>
#include <unistd.h>


#ifdef FAKE_QUARTZ
#  include "fake-quartz.h"
#else
#  include <CoreGraphics/CGDirectDisplay.h>
#  define close_fake_quartz() /* for compatibility with "fake-quartz.h" */
#endif


int
main(int argc, char **argv)
{
	uint32_t cap = 4;
	CGDirectDisplayID *crtcs = malloc((size_t)cap * sizeof(CGDirectDisplayID));
	if (crtcs == NULL) {
		perror("malloc");
		return 1;
	}

	CGError r;
	uint32_t crtc_count;
	while (1) {
		r = CGGetOnlineDisplayList(cap, crtcs, &crtc_count);
		if (r != kCGErrorSuccess) {
			fputs("CGGetOnlineDisplayList failed", stderr);
			goto fail;
		}
		if (crtc_count < cap)
			break;
		cap <<= 1;
		if (cap == 0) { /* We could also test ~0, but it is still too many. */
			fputs("Too many CRTCs", stderr);
			goto fail;
		}
		crtcs = realloc(crtcs, (size_t)cap * sizeof(CGDirectDisplayID));
		if (crtcs == NULL) {
			perror("realloc");
			goto fail;
		}
	}

	printf("CRTC count: %u\n", crtc_count);

	for (uint32_t crtc = 0; crtc < crtc_count; crtc++) {
		printf("CRTC: %u\n", crtc);

		CGDirectDisplayID crtc_id = crtcs[crtc];
		size_t gamma_size = CGDisplayGammaTableCapacity(crtc_id);
		uint32_t gamma_size_out;
		CGGammaValue *saved_red = NULL;
		CGGammaValue *saved_green;
		CGGammaValue *saved_blue;
		CGGammaValue *red = NULL;
		CGGammaValue *green;
		CGGammaValue *blue;
		CGError r;

		if (gamma_size < 2) {
			fputs("Too small gamma ramp", stderr);
			goto crtc_fail;
		}

		printf("    Gamma ramp size: %lu\n", gamma_size);

		if ((saved_red = malloc(3 * gamma_size * sizeof(CGGammaValue))) == NULL)
		  goto crtc_fail;
		saved_green = saved_red   + gamma_size;
		saved_blue  = saved_green + gamma_size;

		if ((red = malloc(3 * gamma_size * sizeof(CGGammaValue))) == NULL)
		  goto crtc_fail;
		green = red   + gamma_size;
		blue  = green + gamma_size;

		r = CGGetDisplayTransferByTable(crtc_id, gamma_size, saved_red,
						saved_green, saved_blue, &gamma_size_out);
		if (r != kCGErrorSuccess) {
			fputs("Cannot read gamma ramps", stderr);
			goto crtc_fail;
		}
		if (gamma_size_out != gamma_size) {
			fputs("Gamma ramps size changed", stderr);
			goto crtc_fail;
		}

		printf("    Red   gamma ramp (every 51:th):");
		for (size_t i = 0; i < gamma_size; i += 51)
			printf(" %.2f", (float)(saved_red[i]));
		printf("\n");

		printf("    Green gamma ramp (every 51:th):");
		for (size_t i = 0; i < gamma_size; i += 51)
			printf(" %.2f", (float)(saved_green[i]));
		printf("\n");

		printf("    Blue  gamma ramp (every 51:th):");
		for (size_t i = 0; i < gamma_size; i += 51)
			printf(" %.2f", (float)(saved_blue[i]));
		printf("\n");

		printf("    Dimming monitor for one second\n");
		for (size_t i = 0; i < gamma_size; i++) {
			red  [i] = saved_red  [i] / 2.f;
			green[i] = saved_green[i] / 2.f;
			blue [i] = saved_blue [i] / 2.f;
		}
		r = CGSetDisplayTransferByTable(crtc_id, (uint32_t)gamma_size, red, green, blue);
		if (r != kCGErrorSuccess) {
			fputs("Cannot set gamma ramps", stderr);
			goto crtc_fail;
		}
		sleep(1);

		printf("    Restoring gamma ramps\n");
		r = CGSetDisplayTransferByTable(crtc_id, (uint32_t)gamma_size,
						saved_red, saved_green, saved_blue);
		if (r != kCGErrorSuccess) {
			fputs("Cannot set gamma ramps", stderr);
			goto crtc_fail;
		}

		free(red);
		free(saved_red);
		continue;

	crtc_fail:
		free(red);
		free(saved_red);
		goto fail;
	}

	free(crtcs);
	close_fake_quartz();
	return 0;

fail:
	free(crtcs);
	close_fake_quartz();
	return 1;
}

