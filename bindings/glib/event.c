// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * This file is part of libgpiod.
 *
 * Copyright (C) 2019 Bartosz Golaszewski <bgolaszewski@baylibre.com>
 */

#include <errno.h>
#include <gio/gio.h>
#include <glib.h>
#include <gpiod.h>
#include <gpiod-glib.h>

struct _GpiodEvent {
	GObject parent_instance;

	gint edge;
	guint64 timestamp;
	GWeakRef source;
};

enum {
	G_GPIOD_EVENT_PROP_EDGE = 1,
	G_GPIOD_EVENT_PROP_TIMESTAMP,
	G_GPIOD_EVENT_PROP_SOURCE,
};

G_DEFINE_TYPE(GpiodEvent, g_gpiod_event, G_TYPE_OBJECT);

static void g_gpiod_event_set_property(GObject *obj, guint prop_id,
				       const GValue *val, GParamSpec *pspec)
{
	GpiodEvent *self = G_GPIOD_EVENT(obj);

	switch (prop_id) {
	case G_GPIOD_EVENT_PROP_EDGE:
		self->edge = g_value_get_int(val);
		break;
	case G_GPIOD_EVENT_PROP_TIMESTAMP:
		self->timestamp = g_value_get_uint64(val);
		break;
	case G_GPIOD_EVENT_PROP_SOURCE:
		g_weak_ref_init(&self->source, g_value_get_object(val));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
		break;
	}
}

static void g_gpiod_event_get_property(GObject *obj, guint prop_id,
				       GValue *val, GParamSpec *pspec)
{
	GpiodEvent *self = G_GPIOD_EVENT(obj);
	g_autoptr(GpiodLine) line = NULL;

	switch (prop_id) {
	case G_GPIOD_EVENT_PROP_EDGE:
		g_value_set_int(val, self->edge);
		break;
	case G_GPIOD_EVENT_PROP_TIMESTAMP:
		g_value_set_uint64(val, self->timestamp);
		break;
	case G_GPIOD_EVENT_PROP_SOURCE:
		line = g_weak_ref_get(&self->source);
		g_value_set_object(val, line);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
		break;
	}
}

static void g_gpiod_event_dispose(GObject *obj)
{
	GpiodEvent *self = G_GPIOD_EVENT(obj);

	g_weak_ref_clear(&self->source);

	G_OBJECT_CLASS(g_gpiod_event_parent_class)->dispose(obj);
}

static void g_gpiod_event_class_init(GpiodEventClass *event_class)
{
	GObjectClass *class = G_OBJECT_CLASS(event_class);

	class->set_property = g_gpiod_event_set_property;
	class->get_property = g_gpiod_event_get_property;
	class->dispose = g_gpiod_event_dispose;

	g_object_class_install_property(class, G_GPIOD_EVENT_PROP_EDGE,
		g_param_spec_int("edge", "Edge",
			"Edge of the event: 0 - falling edge, 1 - rising edge.",
			0, 1, 0,
			G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE));

	g_object_class_install_property(class, G_GPIOD_EVENT_PROP_TIMESTAMP,
		g_param_spec_uint64("timestamp", "Timestamp",
			"Timestamp of the event as nanoseconds since epoch.",
			0, G_MAXUINT64, 0,
			G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE));

	g_object_class_install_property(class, G_GPIOD_EVENT_PROP_SOURCE,
		g_param_spec_object("source", "Source",
			"GPIO Line Object - source of this event.",
			G_GPIOD_TYPE_LINE,
			G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE));
}

static void g_gpiod_event_init(GpiodEvent *self)
{
	self->edge = 0;
	self->timestamp = 0;
}

gint g_gpiod_event_get_edge(GpiodEvent *self)
{
	gint edge;

	g_object_get(self, "edge", &edge, NULL);

	return edge;
}

guint64 g_gpiod_event_get_timestamp(GpiodEvent *self)
{
	guint64 timestamp;

	g_object_get(self, "timestamp", &timestamp, NULL);

	return timestamp;
}

GpiodLine *g_gpiod_event_get_source(GpiodEvent *self)
{
	GpiodLine *source;

	g_object_get(self, "source", &source, NULL);

	return source;
}
