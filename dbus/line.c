// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * This file is part of libgpiod.
 *
 * Copyright (C) 2018-2019 Bartosz Golaszewski <bgolaszewski@baylibre.com>
 */

#include <glib.h>

#include "gpio-dbus.h"

struct _GpioDBusLineObject {
	GpioDBusLineSkeleton parent;

	GpiodLine *line;
};

enum {
	GPIODBUS_LINE_OBJECT_PROP_LINE = 1,
	GPIODBUS_LINE_OBJECT_PROP_OFFSET,
	GPIODBUS_LINE_OBJECT_PROP_NAME,
	GPIODBUS_LINE_OBJECT_PROP_CONSUMER,
	GPIODBUS_LINE_OBJECT_PROP_OUTPUT,
	GPIODBUS_LINE_OBJECT_PROP_ACTIVE_LOW,
	GPIODBUS_LINE_OBJECT_PROP_USED,
	GPIODBUS_LINE_OBJECT_PROP_OPEN_DRAIN,
	GPIODBUS_LINE_OBJECT_PROP_OPEN_SOURCE,
};

G_DEFINE_TYPE(GpioDBusLineObject, gpiodbus_line_object,
	      GPIO_DBUS_TYPE_LINE_SKELETON);

static void gpiodbus_line_object_set_property(GObject *obj, guint prop_id,
					      const GValue *val,
					      GParamSpec *pspec)
{
	GpioDBusLineObject *line_obj = GPIODBUS_LINE_OBJECT(obj);

	switch (prop_id) {
	case GPIODBUS_LINE_OBJECT_PROP_LINE:
		line_obj->line = g_object_ref(g_value_get_object(val));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
		break;
	}
}

static void gpiodbus_line_object_get_property(GObject *obj, guint prop_id,
					      GValue *val, GParamSpec *pspec)
{
	GpioDBusLineObject *line_obj = GPIODBUS_LINE_OBJECT(obj);
	g_autoptr(GpiodChip) chip = g_gpiod_line_get_owner(line_obj->line);
	g_autofree gchar *chip_name = g_gpiod_chip_dup_name(chip);
	g_autofree gchar *line_consumer = NULL;
	g_autofree gchar *line_name = NULL;
	g_autoptr(GError) err = NULL;
	gboolean ret;

	g_assert(chip);

	g_debug("property '%s' requested for line %u of %s",
		pspec->name, g_gpiod_line_get_offset(line_obj->line), chip_name);

	ret = g_gpiod_line_update(line_obj->line, &err);
	if (!ret) {
		g_critical("error trying to re-read line info: %s",
			   err->message);
		return;
	}

	switch (prop_id) {
	case GPIODBUS_LINE_OBJECT_PROP_OFFSET:
		g_value_set_uint(val, g_gpiod_line_get_offset(line_obj->line));
		break;
	case GPIODBUS_LINE_OBJECT_PROP_NAME:
		line_name = g_gpiod_line_dup_name(line_obj->line);
		g_value_set_string(val, line_name);
		break;
	case GPIODBUS_LINE_OBJECT_PROP_CONSUMER:
		line_consumer = g_gpiod_line_dup_consumer(line_obj->line);
		g_value_set_string(val, line_consumer);
		break;
	case GPIODBUS_LINE_OBJECT_PROP_OUTPUT:
		g_value_set_boolean(val,
				    g_gpiod_line_is_output(line_obj->line));
		break;
	case GPIODBUS_LINE_OBJECT_PROP_ACTIVE_LOW:
		g_value_set_boolean(val,
				g_gpiod_line_is_active_low(line_obj->line));
		break;
	case GPIODBUS_LINE_OBJECT_PROP_USED:
		g_value_set_boolean(val, g_gpiod_line_is_used(line_obj->line));
		break;
	case GPIODBUS_LINE_OBJECT_PROP_OPEN_DRAIN:
		g_value_set_boolean(val,
				    g_gpiod_line_is_open_drain(line_obj->line));
		break;
	case GPIODBUS_LINE_OBJECT_PROP_OPEN_SOURCE:
		g_value_set_boolean(val,
				g_gpiod_line_is_open_source(line_obj->line));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
		break;
	}
}

static void gpiodbus_line_object_dispose(GObject *obj)
{
	GpioDBusLineObject *line_obj = GPIODBUS_LINE_OBJECT(obj);

	g_clear_object(&line_obj->line);

	G_OBJECT_CLASS(gpiodbus_line_object_parent_class)->dispose(obj);
}

static void
gpiodbus_line_object_class_init(GpioDBusLineObjectClass *line_obj_class)
{
	GObjectClass *class = G_OBJECT_CLASS(line_obj_class);

	class->set_property = gpiodbus_line_object_set_property;
	class->get_property = gpiodbus_line_object_get_property;
	class->dispose = gpiodbus_line_object_dispose;

	g_object_class_install_property(class, GPIODBUS_LINE_OBJECT_PROP_LINE,
		g_param_spec_object("line", "Line", "GPIO Line object.",
			G_GPIOD_TYPE_LINE,
			G_PARAM_CONSTRUCT_ONLY | G_PARAM_WRITABLE));

	g_object_class_override_property(class,
					 GPIODBUS_LINE_OBJECT_PROP_OFFSET,
					 "offset");

	g_object_class_override_property(class,
					 GPIODBUS_LINE_OBJECT_PROP_NAME,
					 "name");

	g_object_class_override_property(class,
					 GPIODBUS_LINE_OBJECT_PROP_CONSUMER,
					 "consumer");

	g_object_class_override_property(class,
					 GPIODBUS_LINE_OBJECT_PROP_OUTPUT,
					 "output");

	g_object_class_override_property(class,
					 GPIODBUS_LINE_OBJECT_PROP_ACTIVE_LOW,
					 "active-low");

	g_object_class_override_property(class,
					 GPIODBUS_LINE_OBJECT_PROP_USED,
					 "used");

	g_object_class_override_property(class,
					 GPIODBUS_LINE_OBJECT_PROP_OPEN_DRAIN,
					 "open-drain");

	g_object_class_override_property(class,
					 GPIODBUS_LINE_OBJECT_PROP_OPEN_SOURCE,
					 "open-source");
}

static void gpiodbus_line_object_init(GpioDBusLineObject *line_obj)
{
	line_obj->line = NULL;
}

GpioDBusLineObject *gpiodbus_line_object_new(GpiodLine *line)
{
	return GPIODBUS_LINE_OBJECT(g_object_new(GPIODBUS_LINE_OBJECT_TYPE,
						 "line", line, NULL));
}
