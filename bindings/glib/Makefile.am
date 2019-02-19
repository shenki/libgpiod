# SPDX-License-Identifier: LGPL-2.1-or-later

#
# This file is part of libgpiod.
#
# Copyright (C) 2019 Bartosz Golaszewski <bgolaszewski@baylibre.com>
#

lib_LTLIBRARIES = libgpiod-glib.la
libgpiod_glib_la_SOURCES = chip.c error.c event.c line.c misc.c
libgpiod_glib_la_CFLAGS = -Wall -Wextra -g
libgpiod_glib_la_CFLAGS += -I$(top_srcdir)/include/ -include $(top_builddir)/config.h
libgpiod_glib_la_CFLAGS += $(GLIB_CFLAGS) $(GIO_CFLAGS) $(GIO_UNIX_CFLAGS)
libgpiod_glib_la_CFLAGS += -DG_LOG_DOMAIN=\"gpiod-glib\"
libgpiod_glib_la_LDFLAGS = -version-info $(subst .,:,$(ABI_GLIB_VERSION))
libgpiod_glib_la_LDFLAGS += -lgpiod -L$(top_builddir)/lib
libgpiod_glib_la_LDFLAGS += $(GLIB_LIBS) $(GIO_LIBS) $(GIO_UNIX_LIBS)

include_HEADERS = gpiod-glib.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = gpiod-glib.pc

SUBDIRS = gpiod-glib . examples
