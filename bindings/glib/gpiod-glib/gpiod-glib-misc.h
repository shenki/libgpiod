/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * This file is part of libgpiod.
 *
 * Copyright (C) 2019 Bartosz Golaszewski <bgolaszewski@baylibre.com>
 */

#ifndef __GPIOD_GLIB_MISC_H__
#define __GPIOD_GLIB_MISC_H__

#if !defined (__GPIOD_GLIB_INSIDE__)
#error "Only <gpiod-glib.h> can be included directly."
#endif

#include <glib.h>

G_BEGIN_DECLS

const gchar *g_gpiod_version_string(void);
GPtrArray *g_gpiod_get_all_chips(GError **err);

G_END_DECLS

#endif /* __GPIOD_GLIB_MISC_H__ */
