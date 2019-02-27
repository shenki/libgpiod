// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * This file is part of libgpiod.
 *
 * Copyright (C) 2018-2019 Bartosz Golaszewski <bartekgola@gmail.com>
 */

#include <gio/gio.h>
#include <glib.h>
#include <glib-unix.h>
#include <gpiod-glib.h>
#include <stdlib.h>

#include "gpio-dbus.h"

static gboolean on_sigterm(gpointer data)
{
	GMainLoop *loop = data;

	g_debug("SIGTERM received");

	g_main_loop_quit(loop);

	return G_SOURCE_REMOVE;
}

static gboolean on_sigint(gpointer data)
{
	GMainLoop *loop = data;

	g_debug("SIGINT received");

	g_main_loop_quit(loop);

	return G_SOURCE_REMOVE;
}

static gboolean on_sighup(gpointer data G_GNUC_UNUSED)
{
	g_debug("SIGHUB received");

	return G_SOURCE_CONTINUE;
}

static void on_bus_acquired(GDBusConnection *conn,
			    const gchar *name G_GNUC_UNUSED, gpointer data)
{
	GpioDBusDaemon *daemon = data;

	g_debug("DBus connection acquired");

	gpiodbus_daemon_listen(daemon, conn);
}

static void on_name_acquired(GDBusConnection *conn G_GNUC_UNUSED,
			     const gchar *name G_GNUC_UNUSED,
			     gpointer data G_GNUC_UNUSED)
{
	g_debug("DBus name acquired: '%s'", name);
}

static void on_name_lost(GDBusConnection *conn,
			 const gchar *name, gpointer data G_GNUC_UNUSED)
{
	g_debug("DBus name lost: '%s'", name);

	if (!conn)
		g_error("unable to make connection to the bus");

	if (g_dbus_connection_is_closed(conn))
		g_error("connection to the bus closed");

	g_error("name '%s' lost on the bus", name);
}

static void parse_opts(int argc, char **argv)
{
	gboolean rv, opt_debug = FALSE;
	GError *error = NULL;
	GOptionContext *ctx;
	gchar *summary;

	const GOptionEntry opts[] = {
		{
			.long_name		= "debug",
			.short_name		= 'd',
			.flags			= 0,
			.arg			= G_OPTION_ARG_NONE,
			.arg_data		= &opt_debug,
			.description		= "Emit additional debug log messages",
			.arg_description	= NULL,
		},
		{ }
	};

	ctx = g_option_context_new(NULL);

	summary = g_strdup_printf("%s (libgpiod) v%s - dbus daemon for libgpiod",
				  g_get_prgname(), g_gpiod_version_string());
	g_option_context_set_summary(ctx, summary);
	g_free(summary);

	g_option_context_add_main_entries(ctx, opts, NULL);

	rv = g_option_context_parse(ctx, &argc, &argv, &error);
	if (!rv) {
		g_printerr("Option parsing failed: %s\n\nUse %s --help\n",
			   error->message, g_get_prgname());
		exit(EXIT_FAILURE);
	}

	g_option_context_free(ctx);

	if (opt_debug)
		g_setenv("G_MESSAGES_DEBUG", "gpio-dbus", FALSE);
}

int main(int argc, char **argv)
{
	g_autoptr(GpioDBusDaemon) daemon = NULL;
	g_autoptr(GMainLoop) loop = NULL;
	guint bus_id;

	g_set_prgname(program_invocation_short_name);

	parse_opts(argc, argv);

	g_message("initializing %s", g_get_prgname());

	loop = g_main_loop_new(NULL, FALSE);
	daemon = gpiodbus_daemon_new();

	g_unix_signal_add(SIGTERM, on_sigterm, loop);
	g_unix_signal_add(SIGINT, on_sigint, loop);
	g_unix_signal_add(SIGHUP, on_sighup, NULL); /* Ignore SIGHUP. */

	bus_id = g_bus_own_name(G_BUS_TYPE_SYSTEM, "org.gpiod1",
				G_BUS_NAME_OWNER_FLAGS_NONE,
				on_bus_acquired,
				on_name_acquired,
				on_name_lost,
				daemon, NULL);

	g_message("%s started", g_get_prgname());

	g_main_loop_run(loop);

	g_bus_unown_name(bus_id);

	g_message("%s exiting", g_get_prgname());

	return EXIT_SUCCESS;
}
