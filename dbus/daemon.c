// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * This file is part of libgpiod.
 *
 * Copyright (C) 2018-2019 Bartosz Golaszewski <bartekgola@gmail.com>
 */

#include <glib.h>
#include <gudev/gudev.h>

#include "gpio-dbus.h"

struct _GpioDBusDaemon {
	GObject parent;

	GDBusConnection *conn;
	GUdevClient *udev;
	GDBusObjectManagerServer *manager;
	GHashTable *chips;
};

typedef struct {
	GpioDBusChipObject *chip_obj;
	gchar *obj_path;
	GDBusObjectManagerServer *chip_manager;
	GDBusObjectManagerServer *line_manager;
	GList *line_data_list;
} GpioDBusDaemonChipData;

typedef struct {
	GpioDBusLineObject *line_obj;
	gchar *obj_path;
} GpioDBusDaemonLineData;

G_DEFINE_TYPE(GpioDBusDaemon, gpiodbus_daemon, G_TYPE_OBJECT);

static const gchar* const gpiodbus_daemon_udev_subsystems[] = { "gpio", NULL };

static void gpiodbus_daemon_dispose(GObject *obj)
{
	GpioDBusDaemon *daemon = GPIODBUS_DAEMON(obj);

	g_debug("disposing of GPIO daemon");

	g_hash_table_unref(daemon->chips);
	daemon->chips = NULL;
	g_clear_object(&daemon->manager);
	g_clear_object(&daemon->udev);

	G_OBJECT_CLASS(gpiodbus_daemon_parent_class)->dispose(obj);
}

static void gpiodbus_daemon_finalize(GObject *obj)
{
	GpioDBusDaemon *daemon = GPIODBUS_DAEMON(obj);
	g_autoptr(GError) err = NULL;
	gboolean rv;

	g_debug("finalizing GPIO daemon");

	if (daemon->conn) {
		rv = g_dbus_connection_close_sync(daemon->conn, NULL, &err);
		if (!rv)
			g_warning("error closing dbus connection: %s",
				  err->message);
	}

	G_OBJECT_CLASS(gpiodbus_daemon_parent_class)->finalize(obj);
}

static void gpiodbus_daemon_class_init(GpioDBusDaemonClass *daemon_class)
{
	GObjectClass *class = G_OBJECT_CLASS(daemon_class);

	class->dispose = gpiodbus_daemon_dispose;
	class->finalize = gpiodbus_daemon_finalize;
}

static void gpiodbus_daemon_unexport_line(gpointer elem, gpointer data)
{
	GDBusObjectManagerServer *manager = data;
	GpioDBusDaemonLineData *line_data = elem;

	g_debug("unexporting dbus object for GPIO line: '%s'",
		line_data->obj_path);

	g_dbus_object_manager_server_unexport(manager,
					      line_data->obj_path);
}

static void gpiodbus_daemon_line_data_free(gpointer data)
{
	GpioDBusDaemonLineData *line_data = data;

	g_object_unref(line_data->line_obj);
	g_free(line_data->obj_path);
	g_free(line_data);
}

static void gpiodbus_daemon_chip_data_free(gpointer data)
{
	GpioDBusDaemonChipData *chip_data = data;

	g_list_foreach(chip_data->line_data_list,
		       gpiodbus_daemon_unexport_line, chip_data->line_manager);

	g_debug("unexporting dbus object for GPIO chip: %s",
		chip_data->obj_path);

	g_dbus_object_manager_server_unexport(chip_data->chip_manager,
					      chip_data->obj_path);

	g_list_free_full(chip_data->line_data_list,
			 gpiodbus_daemon_line_data_free);
	g_object_unref(chip_data->chip_obj);
	g_free(chip_data->obj_path);
	g_object_unref(chip_data->line_manager);
	g_free(chip_data);
}

static void gpiodbus_daemon_init(GpioDBusDaemon *daemon)
{
	g_debug("initializing GPIO DBus daemon");

	daemon->conn = NULL;
	daemon->udev = g_udev_client_new(gpiodbus_daemon_udev_subsystems);
	daemon->manager = g_dbus_object_manager_server_new("/org/gpiod1");
	daemon->chips = g_hash_table_new_full(g_str_hash, g_str_equal, g_free,
					      gpiodbus_daemon_chip_data_free);
}

GpioDBusDaemon *gpiodbus_daemon_new(void)
{
	return GPIODBUS_DAEMON(g_object_new(GPIODBUS_DAEMON_TYPE, NULL));
}

static void gpiodbus_daemon_export_line(gpointer elem, gpointer data)
{
	g_autoptr(GpioDBusObjectSkeleton) skeleton = NULL;
	GpioDBusDaemonChipData *chip_data = data;
	GpioDBusDaemonLineData *line_data;
	GpiodLine *line = elem;
	const gchar *base_path;

	line_data = g_malloc0(sizeof(*line_data));
	line_data->line_obj = gpiodbus_line_object_new(line);
	chip_data->line_data_list = g_list_append(chip_data->line_data_list,
						  line_data);

	base_path = g_dbus_object_manager_get_object_path(
				G_DBUS_OBJECT_MANAGER(chip_data->line_manager));
	line_data->obj_path = g_strdup_printf("%s/line%u", base_path,
					      g_gpiod_line_get_offset(line));

	skeleton = gpio_dbus_object_skeleton_new(line_data->obj_path);
	gpio_dbus_object_skeleton_set_line(skeleton,
					   GPIO_DBUS_LINE(line_data->line_obj));

	g_debug("exporting dbus object for GPIO line: '%s'",
		line_data->obj_path);

	g_dbus_object_manager_server_export(chip_data->line_manager,
					    G_DBUS_OBJECT_SKELETON(skeleton));
}

static gboolean gpiodbus_daemon_export_lines(GpioDBusDaemon *daemon,
					     GpiodChip *chip,
					     GpioDBusDaemonChipData *chip_data)
{
	g_autofree gchar *chip_name = NULL;
	g_autofree gchar *full_path = NULL;
	g_autoptr(GPtrArray) lines = NULL;
	g_autoptr(GError) err = NULL;
	const gchar *base_path;

	lines = g_gpiod_chip_get_all_lines(chip, &err);
	if (err) {
		g_critical("unable to retrieve GPIO lines: %s",
			   err->message);
		return FALSE;
	}

	base_path = g_dbus_object_manager_get_object_path(
				G_DBUS_OBJECT_MANAGER(daemon->manager));
	chip_name = g_gpiod_chip_dup_name(chip);
	full_path = g_strdup_printf("%s/%s", base_path, chip_name);
	chip_data->line_manager = g_dbus_object_manager_server_new(full_path);

	g_ptr_array_foreach(lines, gpiodbus_daemon_export_line, chip_data);

	g_dbus_object_manager_server_set_connection(chip_data->line_manager,
						    daemon->conn);

	return TRUE;
}

static void gpiodbus_daemon_export_chip(GpioDBusDaemon *daemon,
					const gchar *devname)
{
	g_autofree GpioDBusDaemonChipData *chip_data = NULL;
	g_autoptr(GpioDBusObjectSkeleton) skeleton = NULL;
	g_autofree gchar *obj_name = NULL;
	g_autoptr(GpiodChip) chip = NULL;
	g_autoptr(GError) err = NULL;
	const gchar *base_path;
	gboolean rv;

	chip = g_gpiod_chip_new(devname, &err);
	if (!chip) {
		g_critical("unable to open the GPIO chip device: %s",
			   err->message);
		return;
	}

	chip_data = g_malloc0(sizeof(*chip_data));
	chip_data->chip_obj = gpiodbus_chip_object_new(chip);
	chip_data->chip_manager = daemon->manager;

	base_path = g_dbus_object_manager_get_object_path(
				G_DBUS_OBJECT_MANAGER(daemon->manager));
	obj_name = g_gpiod_chip_dup_name(chip);
	chip_data->obj_path = g_strdup_printf("%s/%s",
					      base_path, obj_name);

	skeleton = gpio_dbus_object_skeleton_new(chip_data->obj_path);
	gpio_dbus_object_skeleton_set_chip(skeleton,
					   GPIO_DBUS_CHIP(chip_data->chip_obj));

	g_debug("exporting dbus object for GPIO chip: '%s'",
		chip_data->obj_path);

	g_dbus_object_manager_server_export(daemon->manager,
					    G_DBUS_OBJECT_SKELETON(skeleton));

	rv = gpiodbus_daemon_export_lines(daemon, chip, chip_data);
	g_return_if_fail(rv);

	rv = g_hash_table_insert(daemon->chips, g_strdup(devname), chip_data);
	chip_data = NULL;
	/* It's a programming bug if the chip is already in the hashmap. */
	g_assert(rv);
}

static void gpiodbus_daemon_remove_chip(GpioDBusDaemon *daemon,
					const gchar *devname)
{
	gboolean rv;

	rv = g_hash_table_remove(daemon->chips, devname);
	/* It's a programming bug if the chip was not in the hashmap. */
	g_assert(rv);
}

/*
 * We get two uevents per action per gpiochip. One is for the new-style
 * character device, the other for legacy sysfs devices. We are only concerned
 * with the former, which we can tell from the latter by the presence of
 * the device file.
 */
static gboolean gpiodbus_daemon_is_gpiochip_device(GUdevDevice *dev)
{
	return g_udev_device_get_device_file(dev) != NULL;
}

static void gpiodbus_daemon_on_uevent(GUdevClient *udev G_GNUC_UNUSED,
		      const gchar *action, GUdevDevice *dev, gpointer data)
{
	GpioDBusDaemon *daemon = data;
	const gchar *devname;

	if (!gpiodbus_daemon_is_gpiochip_device(dev))
		return;

	devname = g_udev_device_get_name(dev);

	g_debug("uevent: %s action on %s device", action, devname);

	if (g_strcmp0(action, "add") == 0)
		gpiodbus_daemon_export_chip(daemon, devname);
	else if (g_strcmp0(action, "remove") == 0)
		gpiodbus_daemon_remove_chip(daemon, devname);
	else
		g_warning("unknown action for uevent: %s", action);
}

static void gpiodbus_daemon_handle_chip_dev(gpointer data, gpointer user_data)
{
	GpioDBusDaemon *daemon = user_data;
	GUdevDevice *dev = data;
	const gchar *devname;

	devname = g_udev_device_get_name(dev);

	if (gpiodbus_daemon_is_gpiochip_device(dev))
		gpiodbus_daemon_export_chip(daemon, devname);
}

void gpiodbus_daemon_listen(GpioDBusDaemon *daemon, GDBusConnection *conn)
{
	g_autolist(GUdevDevice) devs = NULL;
	gulong rv;

	g_assert(!daemon->conn); /* Don't allow to call this twice. */
	daemon->conn = conn;

	/* Subscribe for GPIO uevents. */
	rv = g_signal_connect(daemon->udev, "uevent",
			      G_CALLBACK(gpiodbus_daemon_on_uevent), daemon);
	if (rv <= 0)
		g_error("Unable to connect to 'uevent' signal");

	devs = g_udev_client_query_by_subsystem(daemon->udev, "gpio");
	g_list_foreach(devs, gpiodbus_daemon_handle_chip_dev, daemon);

	g_dbus_object_manager_server_set_connection(daemon->manager,
						    daemon->conn);

	g_debug("GPIO daemon now listening");
}
