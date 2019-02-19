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

int main(int argc, char **argv)
{
	g_autoptr(GpiodLine) line = NULL;
	g_autoptr(GpiodChip) chip = NULL;
	g_autoptr(GArray) values = NULL;
	g_autoptr(GError) err = NULL;
	gboolean ret;
	guint offset;
	gint i, val;

	if (argc < 3) {
		g_printerr("usage: %s <chip-name> <line0> <line1> ... \n",
			   argv[0]);
		return EXIT_FAILURE;
	}

	chip = g_gpiod_chip_new(argv[1], &err);
	if (!chip)
		goto err_out;

	values = g_array_sized_new(FALSE, TRUE, sizeof(gint), argc - 2);

	for (i = 2; i < argc; i++) {
		offset = strtoul(argv[i], NULL, 10);
		line = g_gpiod_chip_get_line(chip, offset, &err);
		if (!line)
			goto err_out;

		ret = g_gpiod_line_request_input(line, "gpioget-glib",
						 FALSE, &err);
		if (!ret)
			goto err_out;

		val = g_gpiod_line_get_value(line, &err);
		if (val < 0)
			goto err_out;

		g_gpiod_line_release(line);
		g_clear_object(&line);
		g_array_append_val(values, val);
	}

	for (i = 0; i < argc - 2; i++)
		g_print("%d ", g_array_index(values, gint, i));
	g_print("\n");

	return EXIT_SUCCESS;

err_out:
	g_printerr("unable to read GPIO line values: %s\n", err->message);
	return EXIT_FAILURE;
}
