// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * This file is part of libgpiod.
 *
 * Copyright (C) 2018-2019 Bartosz Golaszewski <bgolaszewski@baylibre.com>
 */

#include <glib.h>
#include <glib-unix.h>
#include <glib/gprintf.h>
#include <gpiod-glib.h>
#include <stdlib.h>

static void die(GError *err)
{
	g_printerr("unable to read GPIO event: %s\n", err->message);
	exit(EXIT_FAILURE);
}

static void on_event(gpointer data)
{
	g_autoptr(GpiodEvent) event = NULL;
	g_autoptr(GError) err = NULL;
	GpiodLine *line = data;
	guint64 timestamp;

	event = g_gpiod_line_read_event(line, &err);
	if (!event)
		die(err);

	timestamp = g_gpiod_event_get_timestamp(event);

	g_print("event: %s offset: %u timestamp: [%lu.%lu]\n",
		g_gpiod_event_get_edge(event) == 0 ? "FALLING EDGE" :
						     " RISING EDGE",
		g_gpiod_line_get_offset(line),
		timestamp / 1000000000, timestamp % 1000000000);
}

static gboolean on_signal(gpointer data)
{
	GMainLoop *loop = data;

	g_main_loop_quit(loop);

	return G_SOURCE_REMOVE;
}

int main(int argc, char **argv)
{
	g_autoptr(GPtrArray) lines = NULL;
	g_autoptr(GpiodChip) chip = NULL;
	g_autoptr(GMainLoop) loop = NULL;
	g_autoptr(GpiodLine) line = NULL;
	g_autoptr(GError) err = NULL;
	gboolean ret;
	guint offset;
	gint i;

	if (argc < 3) {
		g_printerr("usage: %s <chip-name> <line0> <line1> ... \n",
			   argv[0]);
		return EXIT_FAILURE;
	}

	chip = g_gpiod_chip_new(argv[1], &err);
	if (!chip)
		die(err);

	loop = g_main_loop_new(NULL, FALSE);
	lines = g_ptr_array_new_full(argc - 2, g_object_unref);

	for (i = 2; i < argc; i++) {
		offset = strtoul(argv[i], NULL, 10);
		line = g_gpiod_chip_get_line(chip, offset, &err);
		if (!line)
			die(err);

		ret = g_gpiod_line_request_event(line, "gpiomon-glib",
						 FALSE, TRUE, TRUE, &err);
		if (!ret)
			die(err);

		g_signal_connect(line, "event", G_CALLBACK(on_event), line);

		g_ptr_array_add(lines, line);
		line = NULL;
	}

	g_unix_signal_add(SIGTERM, on_signal, loop);
	g_unix_signal_add(SIGINT, on_signal, loop);

	g_main_loop_run(loop);

	return EXIT_SUCCESS;
}
