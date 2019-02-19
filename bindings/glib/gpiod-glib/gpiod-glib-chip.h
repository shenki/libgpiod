/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * This file is part of libgpiod.
 *
 * Copyright (C) 2019 Bartosz Golaszewski <bgolaszewski@baylibre.com>
 */

#ifndef __GPIOD_GLIB_CHIP_H__
#define __GPIOD_GLIB_CHIP_H__

#if !defined (__GPIOD_GLIB_INSIDE__)
#error "Only <gpiod-glib.h> can be included directly."
#endif

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

typedef struct _GpiodLine GpiodLine;

G_DECLARE_FINAL_TYPE(GpiodChip, g_gpiod_chip, G_GPIOD, CHIP, GObject);

#define G_GPIOD_TYPE_CHIP (g_gpiod_chip_get_type())
#define G_GPIOD_CHIP(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST((obj), G_GPIOD_TYPE_CHIP, GpiodChip))

GpiodChip *g_gpiod_chip_new(const gchar *devname, GError **err);
gchar *g_gpiod_chip_dup_name(GpiodChip *self);
gchar *g_gpiod_chip_dup_label(GpiodChip *self);
guint g_gpiod_chip_get_num_lines(GpiodChip *self);
GpiodLine *g_gpiod_chip_get_line(GpiodChip *self, guint offset, GError **err);
GPtrArray *g_gpiod_chip_get_all_lines(GpiodChip *self, GError **err);

G_END_DECLS

#endif /* __GPIOD_GLIB_CHIP_H__ */
