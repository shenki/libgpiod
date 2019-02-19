/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * This file is part of libgpiod.
 *
 * Copyright (C) 2019 Bartosz Golaszewski <bgolaszewski@baylibre.com>
 */

#ifndef __GPIOD_GLIB_ERROR_H__
#define __GPIOD_GLIB_ERROR_H__

#if !defined (__GPIOD_GLIB_INSIDE__)
#error "Only <gpiod-glib.h> can be included directly."
#endif

G_BEGIN_DECLS

#define G_GPIOD_ERROR g_gpiod_error_quark()

typedef enum {
	G_GPIOD_ERR_FAILED,
	G_GPIOD_ERR_PERM,
	G_GPIOD_ERR_NOENT,
	G_GPIOD_ERR_INTR,
	G_GPIOD_ERR_IO,
	G_GPIOD_ERR_NXIO,
	G_GPIOD_ERR_BADFD,
	G_GPIOD_ERR_CHILD,
	G_GPIOD_ERR_AGAIN,
	G_GPIOD_ERR_NOMEM,
	G_GPIOD_ERR_ACCES,
	G_GPIOD_ERR_FAULT,
	G_GPIOD_ERR_BUSY,
	G_GPIOD_ERR_EXIST,
	G_GPIOD_ERR_NODEV,
	G_GPIOD_ERR_INVAL,
	G_GPIOD_ERR_NOTTY,
	G_GPIOD_ERR_PIPE,
} GpiodError;

GQuark g_gpiod_error_quark(void);
GpiodError g_gpiod_error_from_errno(gint errnum);

G_END_DECLS

#endif /* __GPIOD_GLIB_ERROR_H__ */
