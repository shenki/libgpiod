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

static void die(GError *err)
{
	g_printerr("unable to print GPIO chip info: %s\n", err->message);
	exit(EXIT_FAILURE);
}

static void print_line_info(gpointer data, gpointer user_data G_GNUC_UNUSED)
{
	gboolean is_output, active_low, used, open_drain, open_source;
	g_autofree gchar *flags_final = NULL;
	g_autofree gchar *flags_base = NULL;
	g_autofree gchar *consumer = NULL;
	g_autofree gchar *name = NULL;
	GpiodLine *line = data;
	gchar *flags_stripped;
	guint offset;

	offset = g_gpiod_line_get_offset(line);
	name = g_gpiod_line_dup_name(line);
	consumer = g_gpiod_line_dup_consumer(line);
	is_output = g_gpiod_line_is_output(line);
	active_low = g_gpiod_line_is_active_low(line);
	used = g_gpiod_line_is_used(line);
	open_drain = g_gpiod_line_is_open_drain(line);
	open_source = g_gpiod_line_is_open_source(line);

	flags_base = g_strdup_printf("%s %s %s",
				     used ? "used" : "",
				     open_drain ? "open-drain" : "",
				     open_source ? "open-source" : "");
	flags_stripped = g_strchomp(flags_base);
	if (strlen(flags_stripped) > 0)
		flags_final = g_strdup_printf(" [%s]", flags_stripped);

	g_printf("\tline %3u: %12s %12s %8s %10s%s\n",
		 offset,
		 name ?: "unnamed",
		 consumer ?: "unused",
		 is_output ? "output" : "input",
		 active_low ? "active-low" : "active-high",
		 flags_final ?: "");
}

static void print_chip_info(gpointer data, gpointer user_data G_GNUC_UNUSED)
{
	g_autoptr(GPtrArray) lines = NULL;
	g_autoptr(GError) err = NULL;
	GpiodChip *chip = data;
	g_autofree gchar *name =  g_gpiod_chip_dup_name(chip);
	guint num_lines = g_gpiod_chip_get_num_lines(chip);

	g_printf("%s - %u lines:\n", name, num_lines);

	lines = g_gpiod_chip_get_all_lines(chip, &err);
	if (!lines)
		die(err);

	g_ptr_array_foreach(lines, print_line_info, NULL);
}

int main(int argc G_GNUC_UNUSED, char **argv G_GNUC_UNUSED)
{
	g_autoptr(GPtrArray) chips = NULL;
	g_autoptr(GError) err = NULL;

	chips = g_gpiod_get_all_chips(&err);
	if (!chips)
		die(err);

	g_ptr_array_foreach(chips, print_chip_info, NULL);

	return EXIT_SUCCESS;
}
