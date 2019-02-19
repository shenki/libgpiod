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
#include <string.h>

static void die(GError *err)
{
	g_printerr("unable to lookup GPIO line by name: %s\n", err->message);
	exit(EXIT_FAILURE);
}

static void check_line(gpointer data, gpointer user_data)
{
	GpiodLine *line = data;
	g_autofree gchar *line_name = g_gpiod_line_dup_name(line);
	gchar *name = user_data;
	gchar *chip_name;
	GpiodChip *owner;

	if (!line_name)
		return;

	if (strcmp(line_name, name) == 0) {
		owner = g_gpiod_line_get_owner(line);
		chip_name = g_gpiod_chip_dup_name(owner);
		g_print("%s %u\n", chip_name, g_gpiod_line_get_offset(line));
		g_free(chip_name);
		g_free(line_name);
		g_object_unref(owner);
		exit(EXIT_SUCCESS);
	}
}

static void find_line(gpointer data, gpointer user_data)
{
	g_autoptr(GPtrArray) lines = NULL;
	g_autoptr(GError) err = NULL;
	GpiodChip *chip = data;

	lines = g_gpiod_chip_get_all_lines(chip, &err);
	if (!lines)
		die(err);

	g_ptr_array_foreach(lines, check_line, user_data);
}

int main(int argc, char **argv)
{
	g_autoptr(GPtrArray) chips = NULL;
	g_autoptr(GError) err = NULL;

	if (argc != 2) {
		g_printerr("usage: %s <line-name>\n", argv[0]);
		return EXIT_FAILURE;
	}

	chips = g_gpiod_get_all_chips(&err);
	if (!chips)
		die(err);

	g_ptr_array_foreach(chips, find_line, argv[1]);

	/* Line not found if reached. */
	return EXIT_FAILURE;
}
