// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * This file is part of libgpiod.
 *
 * Copyright (C) 2018-2019 Bartosz Golaszewski <bartekgola@gmail.com>
 */

#include <glib.h>

#include "gpio-dbus.h"

struct _GpioDBusChipObject {
	GpioDBusChipSkeleton parent_instance;

	GpiodChip *chip;
};

enum {
	GPIODBUS_CHIP_OBJECT_PROP_CHIP = 1,
	GPIODBUS_CHIP_OBJECT_PROP_NAME,
	GPIODBUS_CHIP_OBJECT_PROP_LABEL,
	GPIODBUS_CHIP_OBJECT_PROP_NUM_LINES,
};

G_DEFINE_TYPE(GpioDBusChipObject, gpiodbus_chip_object,
	      GPIO_DBUS_TYPE_CHIP_SKELETON);

static void gpiodbus_chip_object_set_property(GObject *obj, guint prop_id,
					      const GValue *val,
					      GParamSpec *pspec)
{
	GpioDBusChipObject *chip_obj = GPIODBUS_CHIP_OBJECT(obj);

	switch (prop_id) {
	case GPIODBUS_CHIP_OBJECT_PROP_CHIP:
		chip_obj->chip = g_object_ref(g_value_get_object(val));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
		break;
	}
}

static void gpiodbus_chip_object_get_property(GObject *obj, guint prop_id,
					      GValue *val, GParamSpec *pspec)
{
	GpioDBusChipObject *chip_obj = GPIODBUS_CHIP_OBJECT(obj);
	g_autofree gchar *name = g_gpiod_chip_dup_name(chip_obj->chip);
	g_autofree gchar *label = NULL;

	g_debug("property '%s' requested for %s", pspec->name, name);

	switch (prop_id) {
	case GPIODBUS_CHIP_OBJECT_PROP_NAME:
		g_value_set_string(val, name);
		break;
	case GPIODBUS_CHIP_OBJECT_PROP_LABEL:
		label = g_gpiod_chip_dup_label(chip_obj->chip);
		g_value_set_string(val, label);
		break;
	case GPIODBUS_CHIP_OBJECT_PROP_NUM_LINES:
		g_value_set_uint(val, g_gpiod_chip_get_num_lines(chip_obj->chip));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
		break;
	}
}

static void gpiodbus_chip_object_dispose(GObject *obj)
{
	GpioDBusChipObject *chip_obj = GPIODBUS_CHIP_OBJECT(obj);

	g_clear_object(&chip_obj->chip);

	G_OBJECT_CLASS(gpiodbus_chip_object_parent_class)->dispose(obj);
}

static void
gpiodbus_chip_object_class_init(GpioDBusChipObjectClass *chip_obj_class)
{
	GObjectClass *class = G_OBJECT_CLASS(chip_obj_class);

	class->set_property = gpiodbus_chip_object_set_property;
	class->get_property = gpiodbus_chip_object_get_property;
	class->dispose = gpiodbus_chip_object_dispose;

	g_object_class_install_property(class, GPIODBUS_CHIP_OBJECT_PROP_CHIP,
		g_param_spec_object("chip", "Chip", "GPIO Chip object.",
			G_GPIOD_TYPE_CHIP,
			G_PARAM_CONSTRUCT_ONLY | G_PARAM_WRITABLE));

	g_object_class_override_property(class,
					 GPIODBUS_CHIP_OBJECT_PROP_NAME,
					 "name");

	g_object_class_override_property(class,
					 GPIODBUS_CHIP_OBJECT_PROP_LABEL,
					 "label");

	g_object_class_override_property(class,
					 GPIODBUS_CHIP_OBJECT_PROP_NUM_LINES,
					 "num-lines");
}

static void gpiodbus_chip_object_init(GpioDBusChipObject *chip_obj)
{
	chip_obj->chip = NULL;
}

GpioDBusChipObject *gpiodbus_chip_object_new(GpiodChip *chip)
{
	return GPIODBUS_CHIP_OBJECT(g_object_new(GPIODBUS_CHIP_OBJECT_TYPE,
						 "chip", chip, NULL));
}
