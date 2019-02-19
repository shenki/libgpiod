/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * This file is part of libgpiod.
 *
 * Copyright (C) 2019 Bartosz Golaszewski <bgolaszewski@baylibre.com>
 */

#ifndef __GPIOD_GLIB_EVENT_H__
#define __GPIOD_GLIB_EVENT_H__

#if !defined (__GPIOD_GLIB_INSIDE__)
#error "Only <gpiod-glib.h> can be included directly."
#endif

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

typedef struct _GpiodLine GpiodLine;

G_DECLARE_FINAL_TYPE(GpiodEvent, g_gpiod_event, G_GPIOD, EVENT, GObject);

#define G_GPIOD_TYPE_EVENT (g_gpiod_event_get_type())
#define G_GPIOD_EVENT(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST((obj), G_GPIOD_TYPE_EVENT, GpiodEvent))

gint g_gpiod_event_get_edge(GpiodEvent *self);
guint64 g_gpiod_event_get_timestamp(GpiodEvent *self);
GpiodLine *g_gpiod_event_get_source(GpiodEvent *self);

G_END_DECLS

#endif /* __GPIOD_GLIB_EVENT_H__ */
