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

static void release_line(gpointer elem, gpointer data G_GNUC_UNUSED)
{
	GpiodLine *line = elem;

	g_gpiod_line_release(line);
}

int main(int argc, char **argv)
{
	g_autoptr(GPtrArray) lines = NULL;
	g_autoptr(GpiodLine) line = NULL;
	g_autoptr(GpiodChip) chip = NULL;
	g_autoptr(GError) err = NULL;
	g_auto(GStrv) tokens = NULL;
	guint offset, value;
	gboolean ret;
	gint i;

	if (argc < 3)
		goto err_usage;

	chip = g_gpiod_chip_new(argv[1], &err);
	if (!chip)
		goto err_out;

	lines = g_ptr_array_new_full(argc - 2, g_object_unref);

	for (i = 2; i < argc; i++) {
		tokens = g_strsplit_set(argv[i], "=", 2);
		if (!tokens || !tokens[0] || !tokens[1])
			goto err_usage;

		offset = strtoul(tokens[0], NULL, 10);
		value = strtoul(tokens[1], NULL, 10);
		g_strfreev(tokens);
		tokens = NULL;

		line = g_gpiod_chip_get_line(chip, offset, &err);
		if (!line)
			goto err_out;

		ret = g_gpiod_line_request_output(line, "gpioget-glib",
						  FALSE, value, &err);
		if (!ret)
			goto err_out;

		g_ptr_array_add(lines, line);
		line = NULL;
	}

	getchar();

	g_ptr_array_foreach(lines, release_line, NULL);

	return EXIT_SUCCESS;

err_out:
	g_printerr("unable to set GPIO line values: %s\n", err->message);
	return EXIT_FAILURE;

err_usage:
	g_printerr("usage: %s <chip-name> <line0>=<val0> <line1>=<val1> ... \n",
		   argv[0]);
	return EXIT_FAILURE;
}
