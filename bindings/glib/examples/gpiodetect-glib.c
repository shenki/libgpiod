// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * This file is part of libgpiod.
 *
 * Copyright (C) 2018-2019 Bartosz Golaszewski <bgolaszewski@baylibre.com>
 */

#include <glib.h>
#include <glib/gprintf.h>
#include <gpiod-glib.h>
#include <stdlib.h>

static void print_chip_info(gpointer data, gpointer user_data G_GNUC_UNUSED)
{
	GpiodChip *chip = data;
	g_autofree gchar *name = g_gpiod_chip_dup_name(chip);
	g_autofree gchar *label = g_gpiod_chip_dup_label(chip);
	guint num_lines = g_gpiod_chip_get_num_lines(chip);

	g_printf("%s [%s] (%u lines)\n", name, label, num_lines);
}

int main(int argc G_GNUC_UNUSED, char **argv G_GNUC_UNUSED)
{
	g_autoptr(GPtrArray) chips = NULL;
	g_autoptr(GError) err = NULL;

	chips = g_gpiod_get_all_chips(&err);
	if (!chips) {
		g_printerr("unable to list GPIO chips: %s\n", err->message);
		return EXIT_FAILURE;
	}

	g_ptr_array_foreach(chips, print_chip_info, NULL);

	return EXIT_SUCCESS;
}
