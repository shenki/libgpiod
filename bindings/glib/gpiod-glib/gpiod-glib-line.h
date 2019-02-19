/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * This file is part of libgpiod.
 *
 * Copyright (C) 2019 Bartosz Golaszewski <bgolaszewski@baylibre.com>
 */

#ifndef __GPIOD_GLIB_LINE_H__
#define __GPIOD_GLIB_LINE_H__

#if !defined (__GPIOD_GLIB_INSIDE__)
#error "Only <gpiod-glib.h> can be included directly."
#endif

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

typedef struct _GpiodChip GpiodChip;
typedef struct _GpiodEvent GpiodEvent;

G_DECLARE_FINAL_TYPE(GpiodLine, g_gpiod_line, G_GPIOD, LINE, GObject);

#define G_GPIOD_TYPE_LINE (g_gpiod_line_get_type())
#define G_GPIOD_LINE(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST((obj), G_GPIOD_TYPE_LINE, GpiodLine))

GpiodChip *g_gpiod_line_get_owner(GpiodLine *self);
guint g_gpiod_line_get_offset(GpiodLine *self);
gchar *g_gpiod_line_dup_name(GpiodLine *self);
gchar *g_gpiod_line_dup_consumer(GpiodLine *self);
gboolean g_gpiod_line_is_output(GpiodLine *self);
gboolean g_gpiod_line_is_active_low(GpiodLine *self);
gboolean g_gpiod_line_is_used(GpiodLine *self);
gboolean g_gpiod_line_is_open_drain(GpiodLine *self);
gboolean g_gpiod_line_is_open_source(GpiodLine *self);
gboolean g_gpiod_line_update(GpiodLine *self, GError **err);
gboolean g_gpiod_line_request_input(GpiodLine *self, const gchar *consumer,
				    gboolean active_low, GError **err);
gboolean g_gpiod_line_request_output(GpiodLine *self, const gchar *consumer,
				     gboolean active_low, gint default_value,
				     GError **err);
gboolean g_gpiod_line_request_event(GpiodLine *self, const gchar *consumer,
				    gboolean active_low, gboolean rising_edge,
				    gboolean falling_edge, GError **err);
void g_gpiod_line_release(GpiodLine *self);
gint g_gpiod_line_get_value(GpiodLine *self, GError **err);
gboolean g_gpiod_line_set_value(GpiodLine *self, gint value, GError **err);
GpiodEvent *g_gpiod_line_read_event(GpiodLine *self, GError **err);

G_END_DECLS

#endif /* __GPIOD_GLIB_LINE_H__ */
