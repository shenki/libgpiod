// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * This file is part of libgpiod.
 *
 * Copyright (C) 2018-2019 Bartosz Golaszewski <bartekgola@gmail.com>
 */

#ifndef __LIBGPIOD_GPIO_DBUS_H__
#define __LIBGPIOD_GPIO_DBUS_H__

#include <gio/gio.h>
#include <glib.h>
#include <gpiod-glib.h>

#include "generated-gpio-dbus.h"

G_DECLARE_FINAL_TYPE(GpioDBusDaemon, gpiodbus_daemon,
		     GPIODBUS, DAEMON, GObject);

#define GPIODBUS_DAEMON_TYPE (gpiodbus_daemon_get_type())
#define GPIODBUS_DAEMON(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST((obj), \
		GPIODBUS_DAEMON_TYPE, GpioDBusDaemon))

GpioDBusDaemon *gpiodbus_daemon_new(void);
void gpiodbus_daemon_listen(GpioDBusDaemon *daemon, GDBusConnection *conn);

G_DECLARE_FINAL_TYPE(GpioDBusChipObject, gpiodbus_chip_object,
		     GPIODBUS, CHIP_OBJECT, GpioDBusChipSkeleton);

#define GPIODBUS_CHIP_OBJECT_TYPE (gpiodbus_chip_object_get_type())
#define GPIODBUS_CHIP_OBJECT(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST((obj), \
		GPIODBUS_CHIP_OBJECT_TYPE, GpioDBusChipObject))

GpioDBusChipObject *gpiodbus_chip_object_new(GpiodChip *chip);

G_DECLARE_FINAL_TYPE(GpioDBusLineObject, gpiodbus_line_object,
		     GPIODBUS, LINE_OBJECT, GpioDBusLineSkeleton);

#define GPIODBUS_LINE_OBJECT_TYPE (gpiodbus_line_object_get_type())
#define GPIODBUS_LINE_OBJECT(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST((obj), \
		GPIODBUS_LINE_OBJECT_TYPE, GpioDBusLineObject))

GpioDBusLineObject *gpiodbus_line_object_new(GpiodLine *line);

#endif /* __LIBGPIOD_GPIO_DBUS_H__ */
