// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * This file is part of libgpiod.
 *
 * Copyright (C) 2018-2019 Bartosz Golaszewski <bgolaszewski@baylibre.com>
 */

#include <errno.h>
#include <gio/gio.h>
#include <gpiod.h>
#include <gpiod-glib.h>

const gchar *g_gpiod_version_string(void)
{
	return gpiod_version_string();
}

GPtrArray *g_gpiod_get_all_chips(GError **err)
{
	struct gpiod_chip_iter *iter;
	struct gpiod_chip *handle;
	GPtrArray *chips;
	GpiodChip *chip;

	iter = gpiod_chip_iter_new();
	if (!iter) {
		g_set_error(err, G_GPIOD_ERROR,
			    g_gpiod_error_from_errno(errno),
			    "unable to create a GPIO chip iterator: %s",
			    g_strerror(errno));
		return NULL;
	}

	chips = g_ptr_array_new_with_free_func(g_object_unref);

	gpiod_foreach_chip_noclose(iter, handle) {
		chip = G_GPIOD_CHIP(g_object_new(G_GPIOD_TYPE_CHIP,
						 "handle", handle, NULL));
		g_ptr_array_add(chips, chip);
	}

	gpiod_chip_iter_free(iter);

	return chips;
}
