// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * This file is part of libgpiod.
 *
 * Copyright (C) 2018-2019 Bartosz Golaszewski <bgolaszewski@baylibre.com>
 */

#include <errno.h>
#include <glib.h>
#include <gpiod-glib.h>

G_DEFINE_QUARK(g-gpiod-error, g_gpiod_error)

GpiodError g_gpiod_error_from_errno(gint errnum)
{
	switch (errnum) {
#ifdef EPERM
	case EPERM:
		return G_GPIOD_ERR_PERM;
#endif
#ifdef ENOENT
	case ENOENT:
		return G_GPIOD_ERR_NOENT;
#endif
#ifdef EINTR
	case EINTR:
		return G_GPIOD_ERR_INTR;
#endif
#ifdef EIO
	case EIO:
		return G_GPIOD_ERR_IO;
#endif
#ifdef ENXIO
	case ENXIO:
		return G_GPIOD_ERR_NXIO;
#endif
#ifdef EBADFD
	case EBADFD:
		return G_GPIOD_ERR_BADFD;
#endif
#ifdef ECHILD
	case ECHILD:
		return G_GPIOD_ERR_CHILD;
#endif
#ifdef EAGAIN
	case EAGAIN:
		return G_GPIOD_ERR_AGAIN;
#endif
#ifdef ENOMEM
	case ENOMEM:
		return G_GPIOD_ERR_NOMEM;
#endif
#ifdef EACCES
	case EACCES:
		return G_GPIOD_ERR_ACCES;
#endif
#ifdef EFAULT
	case EFAULT:
		return G_GPIOD_ERR_FAULT;
#endif
#ifdef EBUSY
	case EBUSY:
		return G_GPIOD_ERR_BUSY;
#endif
#ifdef EEXIST
	case EEXIST:
		return G_GPIOD_ERR_EXIST;
#endif
#ifdef ENODEV
	case ENODEV:
		return G_GPIOD_ERR_NODEV;
#endif
#ifdef EINVAL
	case EINVAL:
		return G_GPIOD_ERR_INVAL;
#endif
#ifdef ENOTTY
	case ENOTTY:
		return G_GPIOD_ERR_NOTTY;
#endif
#ifdef EPIPE
	case EPIPE:
		return G_GPIOD_ERR_PIPE;
#endif
	default:
		return G_GPIOD_ERR_FAILED;
	}
}
