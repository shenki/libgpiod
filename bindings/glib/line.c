// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * This file is part of libgpiod.
 *
 * Copyright (C) 2018-2019 Bartosz Golaszewski <bgolaszewski@baylibre.com>
 */

#include <errno.h>
#include <gio/gio.h>
#include <glib.h>
#include <glib-unix.h>
#include <gpiod.h>
#include <gpiod-glib.h>

struct _GpiodLine {
	GObject parent;

	struct gpiod_line *handle;
	GWeakRef owner;

	guint event_source;
};

enum {
	G_GPIOD_LINE_PROP_HANDLE = 1,
	G_GPIOD_LINE_PROP_OWNER,
	G_GPIOD_LINE_PROP_OFFSET,
	G_GPIOD_LINE_PROP_NAME,
	G_GPIOD_LINE_PROP_CONSUMER,
	G_GPIOD_LINE_PROP_OUTPUT,
	G_GPIOD_LINE_PROP_ACTIVE_LOW,
	G_GPIOD_LINE_PROP_USED,
	G_GPIOD_LINE_PROP_OPEN_SOURCE,
	G_GPIOD_LINE_PROP_OPEN_DRAIN,
};

enum {
	G_GPIOD_LINE_SIGNAL_EVENT,
	G_GPIOD_LINE_NUM_SIGNALS,
};

static guint g_gpiod_line_signals[G_GPIOD_LINE_NUM_SIGNALS];

G_DEFINE_TYPE(GpiodLine, g_gpiod_line, G_TYPE_OBJECT);

static void g_gpiod_line_set_property(GObject *obj, guint prop_id,
				      const GValue *val, GParamSpec *pspec)
{
	GpiodLine *self = G_GPIOD_LINE(obj);

	switch (prop_id) {
	case G_GPIOD_LINE_PROP_HANDLE:
		self->handle = g_value_get_pointer(val);
		break;
	case G_GPIOD_LINE_PROP_OWNER:
		g_weak_ref_init(&self->owner, g_value_get_object(val));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
		break;
	}
}

static void g_gpiod_line_get_property(GObject *obj, guint prop_id,
				      GValue *val, GParamSpec *pspec)
{
	GpiodLine *self = G_GPIOD_LINE(obj);
	g_autoptr(GpiodChip) chip = NULL;
	gboolean dir, active;

	switch (prop_id) {
	case G_GPIOD_LINE_PROP_OWNER:
		chip = g_weak_ref_get(&self->owner);
		g_value_set_object(val, chip);
		break;
	case G_GPIOD_LINE_PROP_OFFSET:
		g_value_set_uint(val, gpiod_line_offset(self->handle));
		break;
	case G_GPIOD_LINE_PROP_NAME:
		g_value_set_string(val, gpiod_line_name(self->handle));
		break;
	case G_GPIOD_LINE_PROP_CONSUMER:
		g_value_set_string(val, gpiod_line_consumer(self->handle));
		break;
	case G_GPIOD_LINE_PROP_OUTPUT:
		dir = gpiod_line_direction(self->handle);
		g_value_set_boolean(val, dir == GPIOD_LINE_DIRECTION_OUTPUT);
		break;
	case G_GPIOD_LINE_PROP_ACTIVE_LOW:
		active = gpiod_line_active_state(self->handle);
		g_value_set_boolean(val, active == GPIOD_LINE_ACTIVE_STATE_LOW);
		break;
	case G_GPIOD_LINE_PROP_USED:
		g_value_set_boolean(val, gpiod_line_is_used(self->handle));
		break;
	case G_GPIOD_LINE_PROP_OPEN_SOURCE:
		g_value_set_boolean(val,
				    gpiod_line_is_open_source(self->handle));
		break;
	case G_GPIOD_LINE_PROP_OPEN_DRAIN:
		g_value_set_boolean(val,
				    gpiod_line_is_open_drain(self->handle));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
		break;
	}
}

static void g_gpiod_line_dispose(GObject *obj)
{
	GpiodLine *self = G_GPIOD_LINE(obj);

	g_weak_ref_clear(&self->owner);

	G_OBJECT_CLASS(g_gpiod_line_parent_class)->dispose(obj);
}

static void g_gpiod_line_class_init(GpiodLineClass *line_class)
{
	GObjectClass *class = G_OBJECT_CLASS(line_class);

	class->dispose = g_gpiod_line_dispose;
	class->set_property = g_gpiod_line_set_property;
	class->get_property = g_gpiod_line_get_property;

	g_object_class_install_property(class, G_GPIOD_LINE_PROP_HANDLE,
		g_param_spec_pointer("handle", "Handle",
			"GPIO line handle.",
			G_PARAM_CONSTRUCT_ONLY | G_PARAM_WRITABLE));

	g_object_class_install_property(class, G_GPIOD_LINE_PROP_OWNER,
		g_param_spec_object("owner", "Owner",
			"GpiodChip object owning this line.",
			G_GPIOD_TYPE_CHIP,
			G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE));

	g_object_class_install_property(class, G_GPIOD_LINE_PROP_OFFSET,
		g_param_spec_uint("offset", "Offset",
			"Offset of this GPIO line.",
			0, G_MAXUINT, 0,
			G_PARAM_READABLE));

	g_object_class_install_property(class, G_GPIOD_LINE_PROP_NAME,
		g_param_spec_string("name", "Name",
			"Optional name of this GPIO line.",
			NULL,
			G_PARAM_READABLE));

	g_object_class_install_property(class, G_GPIOD_LINE_PROP_CONSUMER,
		g_param_spec_string("consumer", "Consumer",
			"Name of the consumer of this GPIO line (if used).",
			NULL,
			G_PARAM_READABLE));

	g_object_class_install_property(class, G_GPIOD_LINE_PROP_OUTPUT,
		g_param_spec_boolean("output", "Output",
			"This GPIO line is in output mode.",
			FALSE,
			G_PARAM_READABLE));

	g_object_class_install_property(class, G_GPIOD_LINE_PROP_ACTIVE_LOW,
		g_param_spec_boolean("active-low", "ActiveLow",
			"This GPIO line is active-low.",
			FALSE,
			G_PARAM_READABLE));

	g_object_class_install_property(class, G_GPIOD_LINE_PROP_USED,
		g_param_spec_boolean("used", "Used",
			"This GPIO line is currently used.",
			FALSE,
			G_PARAM_READABLE));

	g_object_class_install_property(class, G_GPIOD_LINE_PROP_OPEN_DRAIN,
		g_param_spec_boolean("open-drain", "OpenDrain",
			"This GPIO line is open-drain.",
			FALSE,
			G_PARAM_READABLE));

	g_object_class_install_property(class, G_GPIOD_LINE_PROP_OPEN_SOURCE,
		g_param_spec_boolean("open-source", "OpenSource",
			"This GPIO line is open-source.",
			FALSE,
			G_PARAM_READABLE));

	g_gpiod_line_signals[G_GPIOD_LINE_SIGNAL_EVENT] =
		g_signal_new("event", G_TYPE_FROM_CLASS(class),
			     0, 0, NULL, NULL, NULL, G_TYPE_NONE, 0);
}

static void g_gpiod_line_init(GpiodLine *self)
{
	self->handle = NULL;
	self->event_source = 0;
}

GpiodChip *g_gpiod_line_get_owner(GpiodLine *self)
{
	GpiodChip *owner;

	g_object_get(self, "owner", &owner, NULL);

	return owner;
}

guint g_gpiod_line_get_offset(GpiodLine *self)
{
	guint offset;

	g_object_get(self, "offset", &offset, NULL);

	return offset;
}

gchar *g_gpiod_line_dup_name(GpiodLine *self)
{
	gchar *name;

	g_object_get(self, "name", &name, NULL);

	return name;
}

gchar *g_gpiod_line_dup_consumer(GpiodLine *self)
{
	gchar *consumer;

	g_object_get(self, "consumer", &consumer, NULL);

	return consumer;
}

gboolean g_gpiod_line_is_output(GpiodLine *self)
{
	gboolean ret;

	g_object_get(self, "output", &ret, NULL);

	return ret;
}

gboolean g_gpiod_line_is_active_low(GpiodLine *self)
{
	gboolean ret;

	g_object_get(self, "active-low", &ret, NULL);

	return ret;
}

gboolean g_gpiod_line_is_used(GpiodLine *self)
{
	gboolean ret;

	g_object_get(self, "used", &ret, NULL);

	return ret;
}

gboolean g_gpiod_line_is_open_drain(GpiodLine *self)
{
	gboolean ret;

	g_object_get(self, "open-drain", &ret, NULL);

	return ret;
}

gboolean g_gpiod_line_is_open_source(GpiodLine *self)
{
	gboolean ret;

	g_object_get(self, "open-source", &ret, NULL);

	return ret;
}

gboolean g_gpiod_line_update(GpiodLine *self, GError **err)
{
	gint ret;

	ret = gpiod_line_update(self->handle);
	if (ret) {
		g_set_error(err, G_GPIOD_ERROR,
			    g_gpiod_error_from_errno(errno),
			    "unable to update line info: %s",
			    g_strerror(errno));
		return FALSE;
	}

	return TRUE;
}

static gboolean g_gpiod_line_request(GpiodLine *self, const gchar *consumer,
				     gint request_type, gboolean active_low,
				     gint default_value, GError **err)
{
	struct gpiod_line_request_config config;
	gint ret;

	config.consumer = consumer;
	config.request_type = request_type;
	config.flags = active_low ? GPIOD_LINE_REQUEST_FLAG_ACTIVE_LOW : 0;

	ret = gpiod_line_request(self->handle, &config, default_value);
	if (ret) {
		g_set_error(err, G_GPIOD_ERROR,
			    g_gpiod_error_from_errno(errno),
			    "unable to request GPIO line: %s",
			    g_strerror(errno));
		return FALSE;
	}

	return TRUE;
}

gboolean g_gpiod_line_request_input(GpiodLine *self, const gchar *consumer,
				    gboolean active_low, GError **err)
{
	return g_gpiod_line_request(self, consumer,
				    GPIOD_LINE_REQUEST_DIRECTION_INPUT,
				    active_low, 0, err);
}

gboolean g_gpiod_line_request_output(GpiodLine *self, const gchar *consumer,
				     gboolean active_low, gint default_value,
				     GError **err)
{
	return g_gpiod_line_request(self, consumer,
				    GPIOD_LINE_REQUEST_DIRECTION_OUTPUT,
				    active_low, default_value, err);
}

static gboolean g_gpiod_line_on_event(gint fd G_GNUC_UNUSED,
				      GIOCondition condition G_GNUC_UNUSED,
				      gpointer data)
{
	GpiodLine *self = data;

	g_signal_emit(self, g_gpiod_line_signals[G_GPIOD_LINE_SIGNAL_EVENT], 0);

	return TRUE;
}

gboolean g_gpiod_line_request_event(GpiodLine *self, const gchar *consumer,
				    gboolean active_low, gboolean rising_edge,
				    gboolean falling_edge, GError **err)
{
	gint request_type, ret, fd;

	if (rising_edge && falling_edge) {
		request_type = GPIOD_LINE_REQUEST_EVENT_BOTH_EDGES;
	} else if (rising_edge) {
		request_type = GPIOD_LINE_REQUEST_EVENT_RISING_EDGE;
	} else if (falling_edge) {
		request_type = GPIOD_LINE_REQUEST_EVENT_FALLING_EDGE;
	} else {
		g_set_error(err, G_GPIOD_ERROR, G_GPIOD_ERR_INVAL,
			    "unable to request GPIO line: at least one event type must be specified");
		return FALSE;
	}

	ret = g_gpiod_line_request(self, consumer, request_type,
				   active_low, 0, err);
	if (!ret)
		return FALSE;

	fd = gpiod_line_event_get_fd(self->handle);
	if (fd < 0) {
		g_set_error(err, G_GPIOD_ERROR,
			    g_gpiod_error_from_errno(errno),
			    "unable to retrieve the event file descriptor: %s",
			    g_strerror(errno));
		gpiod_line_release(self->handle);
		return FALSE;
	}

	self->event_source = g_unix_fd_add(fd, G_IO_IN | G_IO_PRI,
					   g_gpiod_line_on_event, self);

	return TRUE;
}

void g_gpiod_line_release(GpiodLine *self)
{
	if (self->event_source > 0)
		g_source_remove(self->event_source);

	gpiod_line_release(self->handle);
}

gint g_gpiod_line_get_value(GpiodLine *self, GError **err)
{
	gint val;

	val = gpiod_line_get_value(self->handle);
	if (val < 0)
		g_set_error(err, G_GPIOD_ERROR,
			    g_gpiod_error_from_errno(errno),
			    "unable to read GPIO line value: %s",
			    g_strerror(errno));

	return val;
}

gboolean g_gpiod_line_set_value(GpiodLine *self, gint value, GError **err)
{
	gint ret;

	ret = gpiod_line_set_value(self->handle, value);
	if (ret) {
		g_set_error(err, G_GPIOD_ERROR,
			    g_gpiod_error_from_errno(errno),
			    "unable to set GPIO line value: %s",
			    g_strerror(errno));
		return FALSE;
	}

	return TRUE;
}

GpiodEvent *g_gpiod_line_read_event(GpiodLine *self, GError **err)
{
	struct gpiod_line_event event;
	guint64 timestamp;
	gint ret, edge;

	ret = gpiod_line_event_read(self->handle, &event);
	if (ret) {
		g_set_error(err, G_GPIOD_ERROR,
			    g_gpiod_error_from_errno(errno),
			    "unable to read line event: %s",
			    g_strerror(errno));
		return NULL;
	}

	edge = event.event_type == GPIOD_LINE_EVENT_RISING_EDGE ? 1 : 0;
	timestamp = event.ts.tv_sec * 1000000000 + event.ts.tv_nsec;

	return G_GPIOD_EVENT(g_object_new(G_GPIOD_TYPE_EVENT,
					  "edge", edge,
					  "timestamp", timestamp,
					  "source", self, NULL));
}
