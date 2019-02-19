// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * This file is part of libgpiod.
 *
 * Copyright (C) 2018-2019 Bartosz Golaszewski <bgolaszewski@baylibre.com>
 */

#include <errno.h>
#include <gio/gio.h>
#include <glib.h>
#include <gpiod.h>
#include <gpiod-glib.h>

struct _GpiodChip {
	GObject parent_instance;

	gchar *devname;
	GError *construct_err;

	struct gpiod_chip *handle;
	GPtrArray *lines;
};

enum {
	G_GPIOD_CHIP_PROP_DEVNAME = 1,
	G_GPIOD_CHIP_PROP_HANDLE,
	G_GPIOD_CHIP_PROP_NAME,
	G_GPIOD_CHIP_PROP_LABEL,
	G_GPIOD_CHIP_PROP_NUM_LINES,
};

static gboolean
g_gpiod_chip_initable_init(GInitable *initable,
			   GCancellable *cancellable G_GNUC_UNUSED,
			   GError **err)
{
	GpiodChip *self = G_GPIOD_CHIP(initable);

	if (self->construct_err) {
		g_propagate_error(err, self->construct_err);
		return FALSE;
	}

	return TRUE;
}

static void g_gpiod_chip_initable_iface_init(GInitableIface *iface)
{
	iface->init = g_gpiod_chip_initable_init;
}

G_DEFINE_TYPE_WITH_CODE(GpiodChip, g_gpiod_chip, G_TYPE_OBJECT,
			G_IMPLEMENT_INTERFACE(
				G_TYPE_INITABLE,
				g_gpiod_chip_initable_iface_init));

static void g_gpiod_chip_constructed(GObject *obj)
{
	GpiodChip *self = G_GPIOD_CHIP(obj);
	guint num_lines;

	if (!self->handle) {
		g_assert(self->devname);

		self->handle = gpiod_chip_open_lookup(self->devname);
		if (!self->handle) {
			g_set_error(&self->construct_err, G_GPIOD_ERROR,
				    g_gpiod_error_from_errno(errno),
				    "unable to open GPIO chip '%s': %s",
				    self->devname, g_strerror(errno));
		} else {
			num_lines = gpiod_chip_num_lines(self->handle);
			self->lines = g_ptr_array_sized_new(num_lines);
			g_ptr_array_set_size(self->lines, num_lines);
		}
		g_clear_pointer(&self->devname, g_free);
	}

	G_OBJECT_CLASS(g_gpiod_chip_parent_class)->constructed(obj);
}

static void g_gpiod_chip_set_property(GObject *obj, guint prop_id,
				      const GValue *val, GParamSpec *pspec)
{
	GpiodChip *self = G_GPIOD_CHIP(obj);

	switch (prop_id) {
	case G_GPIOD_CHIP_PROP_DEVNAME:
		self->devname = g_value_dup_string(val);
		break;
	case G_GPIOD_CHIP_PROP_HANDLE:
		self->handle = g_value_get_pointer(val);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
		break;
	}
}

static void g_gpiod_chip_get_property(GObject *obj, guint prop_id,
				      GValue *val, GParamSpec *pspec)
{
	GpiodChip *self = G_GPIOD_CHIP(obj);

	switch (prop_id) {
	case G_GPIOD_CHIP_PROP_NAME:
		g_value_set_string(val, gpiod_chip_name(self->handle));
		break;
	case G_GPIOD_CHIP_PROP_LABEL:
		g_value_set_string(val, gpiod_chip_label(self->handle));
		break;
	case G_GPIOD_CHIP_PROP_NUM_LINES:
		g_value_set_uint(val, gpiod_chip_num_lines(self->handle));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
		break;
	}
}

/* Lines may be NULL so we can't use g_object_unref unconditionally. */
static void g_gpiod_chip_array_free_line(gpointer elem,
					 gpointer user_data G_GNUC_UNUSED)
{
	GpiodLine *line = elem;

	if (line)
		g_object_unref(line);
}

static void g_gpiod_chip_dispose(GObject *obj)
{
	GpiodChip *self = G_GPIOD_CHIP(obj);

	if (self->lines) {
		g_ptr_array_foreach(self->lines,
				    g_gpiod_chip_array_free_line, NULL);
		g_ptr_array_unref(self->lines);
		self->lines = NULL;
	}

	G_OBJECT_CLASS(g_gpiod_chip_parent_class)->dispose(obj);
}

static void g_gpiod_chip_finalize(GObject *obj)
{
	GpiodChip *self = G_GPIOD_CHIP(obj);

	if (self->handle)
		gpiod_chip_close(self->handle);

	G_OBJECT_CLASS(g_gpiod_chip_parent_class)->finalize(obj);
}

static void g_gpiod_chip_class_init(GpiodChipClass *chip_class)
{
	GObjectClass *class = G_OBJECT_CLASS(chip_class);

	class->constructed = g_gpiod_chip_constructed;
	class->set_property = g_gpiod_chip_set_property;
	class->get_property = g_gpiod_chip_get_property;
	class->dispose = g_gpiod_chip_dispose;
	class->finalize = g_gpiod_chip_finalize;

	g_object_class_install_property(class, G_GPIOD_CHIP_PROP_DEVNAME,
		g_param_spec_string("devname", "DevName",
			"Lookup name for a new GPIO chip.", NULL,
			G_PARAM_CONSTRUCT_ONLY | G_PARAM_WRITABLE));

	g_object_class_install_property(class, G_GPIOD_CHIP_PROP_HANDLE,
		g_param_spec_pointer("handle", "Handle",
			"Open GPIO chip handle as returned by gpiod_chip_open().",
			G_PARAM_CONSTRUCT_ONLY | G_PARAM_WRITABLE));

	g_object_class_install_property(class, G_GPIOD_CHIP_PROP_NAME,
		g_param_spec_string("name", "Name",
			"Device name of this GPIO chip.", NULL,
			G_PARAM_READABLE));

	g_object_class_install_property(class, G_GPIOD_CHIP_PROP_LABEL,
		g_param_spec_string("label", "Label",
			"Label of this GPIO chip.",
			NULL,
			G_PARAM_READABLE));

	g_object_class_install_property(class, G_GPIOD_CHIP_PROP_NUM_LINES,
		g_param_spec_uint("num-lines", "NumLines",
			"Number of GPIO lines exported by this chip.",
			1, G_MAXUINT, 1,
			G_PARAM_READABLE));
}

static void g_gpiod_chip_init(GpiodChip *self)
{
	self->devname = NULL;
	self->construct_err = NULL;
	self->handle = NULL;
	self->lines = NULL;
}

GpiodChip *g_gpiod_chip_new(const gchar *devname, GError **err)
{
	return G_GPIOD_CHIP(g_initable_new(G_GPIOD_TYPE_CHIP, NULL, err,
					   "devname", devname, NULL));
}

gchar *g_gpiod_chip_dup_name(GpiodChip *self)
{
	gchar *name;

	g_object_get(self, "name", &name, NULL);

	return name;
}

gchar *g_gpiod_chip_dup_label(GpiodChip *self)
{
	gchar *label;

	g_object_get(self, "label", &label, NULL);

	return label;
}

guint g_gpiod_chip_get_num_lines(GpiodChip *self)
{
	guint num_lines;

	g_object_get(self, "num-lines", &num_lines, NULL);

	return num_lines;
}

GpiodLine *g_gpiod_chip_get_line(GpiodChip *self, guint offset, GError **err)
{
	struct gpiod_line *handle;
	GpiodLine *line;

	g_assert(self);

	line = g_ptr_array_index(self->lines, offset);
	if (!line) {
		handle = gpiod_chip_get_line(self->handle, offset);
		if (!handle) {
			g_set_error(err, G_GPIOD_ERROR,
				    g_gpiod_error_from_errno(errno),
				    "unable to retrieve the GPIO line at offset %u: %s",
				    offset, g_strerror(errno));
			return NULL;
		}

		line = G_GPIOD_LINE(g_object_new(G_GPIOD_TYPE_LINE,
						 "handle", handle,
						 "owner", self, NULL));

		g_ptr_array_insert(self->lines, offset, line);
	}

	return g_object_ref(line);
}

GPtrArray *g_gpiod_chip_get_all_lines(GpiodChip *self, GError **err)
{
	GError *new_err = NULL;
	guint i, num_lines;
	GPtrArray *lines;
	GpiodLine *line;

	g_assert(self);
	g_assert(self->handle);

	num_lines = g_gpiod_chip_get_num_lines(self);
	lines = g_ptr_array_new_full(num_lines, g_object_unref);

	for (i = 0; i < num_lines; i++) {
		line = g_gpiod_chip_get_line(self, i, &new_err);
		if (new_err) {
			g_propagate_error(err, new_err);
			g_ptr_array_unref(lines);
			return NULL;
		}

		g_ptr_array_add(lines, line);
	}

	return lines;
}
