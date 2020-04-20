// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * This file is part of libgpiod.
 *
 * Copyright (C) 2020 Bartosz Golaszewski <bgolaszewski@baylibre.com>
 */

#include <errno.h>
#include <poll.h>
#include <string.h>

#include "gpiod-test.h"

#define GPIOD_TEST_GROUP "watch"

/*
 * TODO We could probably improve the testing framework with support for a
 * thread pool both for regular line events and for line state changes here.
 * For now we'll just trigger the latter synchronously.
 */

GPIOD_TEST_CASE(single_line_one_request_event, 0, { 8 })
{
	g_autoptr(gpiod_chip_struct) chip = NULL;
	struct gpiod_watch_event event;
	struct timespec ts = { 1, 0 };
	struct gpiod_line *line;
	gint ret;

	chip = gpiod_chip_open(gpiod_test_chip_path(0));
	g_assert_nonnull(chip);
	gpiod_test_return_if_failed();

	line = gpiod_chip_get_line_watched(chip, 2);
	g_assert_nonnull(line);
	gpiod_test_return_if_failed();

	ret = gpiod_line_request_input(line, GPIOD_TEST_CONSUMER);
	g_assert_cmpint(ret, ==, 0);
	gpiod_test_return_if_failed();

	ret = gpiod_chip_watch_event_wait(chip, &ts);
	g_assert_cmpint(ret, ==, 1);
	gpiod_test_return_if_failed();

	ret = gpiod_chip_watch_event_read(chip, &event);
	g_assert_cmpint(ret, ==, 0);
	gpiod_test_return_if_failed();

	g_assert_cmpint(event.event_type, ==, GPIOD_WATCH_EVENT_LINE_REQUESTED);
	g_assert_cmpint(gpiod_line_offset(line), ==,
			gpiod_line_offset(event.line));
	g_assert_true(gpiod_line_is_used(line));
}

GPIOD_TEST_CASE(flags_are_updated_on_request_event, 0, { 8 })
{
	g_autoptr(gpiod_chip_struct) chip = NULL;
	struct gpiod_watch_event event;
	struct timespec ts = { 1, 0 };
	struct gpiod_line *line;
	gint ret;

	chip = gpiod_chip_open(gpiod_test_chip_path(0));
	g_assert_nonnull(chip);
	gpiod_test_return_if_failed();

	line = gpiod_chip_get_line_watched(chip, 2);
	g_assert_nonnull(line);
	gpiod_test_return_if_failed();

	g_assert_false(gpiod_line_is_open_drain(line));

	ret = gpiod_line_request_output_flags(line, GPIOD_TEST_CONSUMER,
				GPIOD_LINE_REQUEST_FLAG_OPEN_DRAIN, 0);
	g_assert_cmpint(ret, ==, 0);
	gpiod_test_return_if_failed();

	ret = gpiod_chip_watch_event_wait(chip, &ts);
	g_assert_cmpint(ret, ==, 1);
	gpiod_test_return_if_failed();

	ret = gpiod_chip_watch_event_read(chip, &event);
	g_assert_cmpint(ret, ==, 0);
	gpiod_test_return_if_failed();

	g_assert_cmpint(event.event_type, ==, GPIOD_WATCH_EVENT_LINE_REQUESTED);
	g_assert_cmpint(gpiod_line_offset(line), ==,
			gpiod_line_offset(event.line));
	g_assert_true(gpiod_line_is_used(line));
	g_assert_true(gpiod_line_is_open_drain(line));
}

GPIOD_TEST_CASE(read_multiple_events, 0, { 8 })
{
	g_autoptr(gpiod_chip_struct) chip = NULL;
	struct gpiod_watch_event events[3];
	struct timespec ts = { 1, 0 };
	struct gpiod_line *line;
	gint ret;

	chip = gpiod_chip_open(gpiod_test_chip_path(0));
	g_assert_nonnull(chip);
	gpiod_test_return_if_failed();

	line = gpiod_chip_get_line_watched(chip, 2);
	g_assert_nonnull(line);
	gpiod_test_return_if_failed();

	ret = gpiod_line_request_input(line, GPIOD_TEST_CONSUMER);
	g_assert_cmpint(ret, ==, 0);
	ret = gpiod_line_set_direction_output(line, 1);
	g_assert_cmpint(ret, ==, 0);
	gpiod_line_release(line);
	gpiod_test_return_if_failed();

	ret = gpiod_chip_watch_event_wait(chip, &ts);
	g_assert_cmpint(ret, ==, 1);
	gpiod_test_return_if_failed();

	ret = gpiod_chip_watch_event_read_multiple(chip, events, 3);
	g_assert_cmpint(ret, ==, 3);
	gpiod_test_return_if_failed();

	g_assert_cmpint(events[0].event_type, ==,
			GPIOD_WATCH_EVENT_LINE_REQUESTED);
	g_assert_cmpint(events[1].event_type, ==,
			GPIOD_WATCH_EVENT_LINE_CONFIG_CHANGED);
	g_assert_cmpint(events[2].event_type, ==,
			GPIOD_WATCH_EVENT_LINE_RELEASED);

	g_assert_cmpint(gpiod_line_offset(line), ==,
			gpiod_line_offset(events[0].line));
	g_assert_cmpint(gpiod_line_offset(line), ==,
			gpiod_line_offset(events[1].line));
	g_assert_cmpint(gpiod_line_offset(line), ==,
			gpiod_line_offset(events[2].line));
}

GPIOD_TEST_CASE(watch_multiple_lines, 0, { 8 })
{
	g_autoptr(gpiod_chip_struct) chip = NULL;
	struct gpiod_line *line0, *line1, *line2;
	struct gpiod_watch_event events[5];
	struct timespec ts = { 0, 100000 };
	struct gpiod_line_bulk lines;
	unsigned int offsets[4];
	gint ret;

	chip = gpiod_chip_open(gpiod_test_chip_path(0));
	g_assert_nonnull(chip);
	gpiod_test_return_if_failed();

	gpiod_line_bulk_init(&lines);
	offsets[0] = 2;
	offsets[1] = 3;
	offsets[2] = 5;
	offsets[3] = 7;
	ret = gpiod_chip_get_lines_watched(chip, offsets, 4, &lines);
	g_assert_cmpint(ret, ==, 0);
	gpiod_test_return_if_failed();

	/* Verify that no events are queued. */
	ret = gpiod_chip_watch_event_wait(chip, &ts);
	g_assert_cmpint(ret, ==, 0);
	gpiod_test_return_if_failed();

	line0 = gpiod_line_bulk_get_line(&lines, 0);
	line1 = gpiod_line_bulk_get_line(&lines, 2);
	line2 = gpiod_line_bulk_get_line(&lines, 3);

	ret = gpiod_line_request_input(line0, GPIOD_TEST_CONSUMER);
	g_assert_cmpint(ret, ==, 0);
	ret = gpiod_line_request_output(line1, GPIOD_TEST_CONSUMER, 0);
	g_assert_cmpint(ret, ==, 0);
	ret = gpiod_line_request_input(line2, GPIOD_TEST_CONSUMER);
	g_assert_cmpint(ret, ==, 0);
	ret = gpiod_line_set_direction_output(line0, 1);
	g_assert_cmpint(ret, ==, 0);
	gpiod_test_return_if_failed();
	gpiod_line_release(line1);

	ts.tv_sec = 1;
	ts.tv_nsec = 0;
	ret = gpiod_chip_watch_event_wait(chip, &ts);
	g_assert_cmpint(ret, ==, 1);
	gpiod_test_return_if_failed();

	ret = gpiod_chip_watch_event_read_multiple(chip, events, 5);
	g_assert_cmpint(ret, ==, 5);
	gpiod_test_return_if_failed();

	g_assert_cmpint(events[0].event_type, ==,
			GPIOD_WATCH_EVENT_LINE_REQUESTED);
	g_assert_cmpint(gpiod_line_offset(events[0].line), ==,
			gpiod_line_offset(line0));

	g_assert_cmpint(events[1].event_type, ==,
			GPIOD_WATCH_EVENT_LINE_REQUESTED);
	g_assert_cmpint(gpiod_line_offset(events[1].line), ==,
			gpiod_line_offset(line1));

	g_assert_cmpint(events[2].event_type, ==,
			GPIOD_WATCH_EVENT_LINE_REQUESTED);
	g_assert_cmpint(gpiod_line_offset(events[2].line), ==,
			gpiod_line_offset(line2));

	g_assert_cmpint(events[3].event_type, ==,
			GPIOD_WATCH_EVENT_LINE_CONFIG_CHANGED);
	g_assert_cmpint(gpiod_line_offset(events[3].line), ==,
			gpiod_line_offset(line0));

	g_assert_cmpint(events[4].event_type, ==,
			GPIOD_WATCH_EVENT_LINE_RELEASED);
	g_assert_cmpint(gpiod_line_offset(events[4].line), ==,
			gpiod_line_offset(line1));
}

GPIOD_TEST_CASE(poll_watch_fd, 0, { 8 })
{
	g_autoptr(gpiod_chip_struct) chip = NULL;
	struct gpiod_watch_event event;
	struct gpiod_line *line;
	struct pollfd pfd;
	gint ret, fd;

	chip = gpiod_chip_open(gpiod_test_chip_path(0));
	g_assert_nonnull(chip);
	gpiod_test_return_if_failed();

	line = gpiod_chip_get_line_watched(chip, 2);
	g_assert_nonnull(line);
	gpiod_test_return_if_failed();

	fd = gpiod_chip_watch_get_fd(chip);
	g_assert_cmpint(fd, >=, 0);
	gpiod_test_return_if_failed();

	memset(&pfd, 0, sizeof(pfd));
	pfd.fd = fd;
	pfd.events = POLLIN | POLLPRI;

	ret = poll(&pfd, 1, 10);
	/*
	 * We're expecting timeout - there must not be any events in the
	 * kernel queue.
	 */
	g_assert_cmpint(ret, ==, 0);

	ret = gpiod_line_request_input(line, GPIOD_TEST_CONSUMER);
	g_assert_cmpint(ret, ==, 0);
	gpiod_test_return_if_failed();

	ret = poll(&pfd, 1, 1000);
	g_assert_cmpint(ret, >, 0);

	ret = gpiod_chip_watch_event_read(chip, &event);
	g_assert_cmpint(ret, ==, 0);
	gpiod_test_return_if_failed();

	g_assert_cmpint(event.event_type, ==, GPIOD_WATCH_EVENT_LINE_REQUESTED);
	g_assert_cmpint(gpiod_line_offset(line), ==,
			gpiod_line_offset(event.line));
}

GPIOD_TEST_CASE(start_watching_non_watched_line, 0, { 8 })
{
	g_autoptr(gpiod_chip_struct) chip = NULL;
	struct timespec ts = { 0, 100000 };
	struct gpiod_watch_event event;
	struct gpiod_line *line;
	gint ret;

	chip = gpiod_chip_open(gpiod_test_chip_path(0));
	g_assert_nonnull(chip);
	gpiod_test_return_if_failed();

	line = gpiod_chip_get_line(chip, 2);
	g_assert_nonnull(line);
	gpiod_test_return_if_failed();

	/* Verify the line's not watched. */
	ret = gpiod_line_request_input(line, GPIOD_TEST_CONSUMER);
	g_assert_cmpint(ret, ==, 0);
	gpiod_test_return_if_failed();
	gpiod_line_release(line);

	ret = gpiod_chip_watch_event_wait(chip, &ts);
	g_assert_cmpint(ret, ==, 0);
	gpiod_test_return_if_failed();

	ret = gpiod_line_watch(line);
	g_assert_cmpint(ret, ==, 0);
	gpiod_test_return_if_failed();

	/* Verify it's now watched. */
	ret = gpiod_line_request_input(line, GPIOD_TEST_CONSUMER);
	g_assert_cmpint(ret, ==, 0);
	gpiod_test_return_if_failed();

	ts.tv_sec = 1;
	ts.tv_nsec = 0;

	ret = gpiod_chip_watch_event_wait(chip, &ts);
	g_assert_cmpint(ret, ==, 1);
	gpiod_test_return_if_failed();

	ret = gpiod_chip_watch_event_read(chip, &event);
	g_assert_cmpint(ret, ==, 0);
	gpiod_test_return_if_failed();

	g_assert_cmpint(event.event_type, ==, GPIOD_WATCH_EVENT_LINE_REQUESTED);
	g_assert_cmpint(gpiod_line_offset(line), ==,
			gpiod_line_offset(event.line));
}

GPIOD_TEST_CASE(watch_unwatch_bulk, 0, { 8 })
{
	g_autoptr(gpiod_chip_struct) chip = NULL;
	struct timespec ts = { 0, 1000000 };
	struct gpiod_watch_event events[3];
	struct gpiod_line *line0, *line1;
	struct gpiod_line_bulk lines;
	unsigned int offsets[2];
	gint ret;

	chip = gpiod_chip_open(gpiod_test_chip_path(0));
	g_assert_nonnull(chip);
	gpiod_test_return_if_failed();

	gpiod_line_bulk_init(&lines);
	offsets[0] = 4;
	offsets[1] = 6;

	ret = gpiod_chip_get_lines(chip, offsets, 2, &lines);
	g_assert_cmpint(ret, ==, 0);
	gpiod_test_return_if_failed();

	line0 = gpiod_line_bulk_get_line(&lines, 0);
	line1 = gpiod_line_bulk_get_line(&lines, 1);

	/* Verify lines not watched. */
	ret = gpiod_line_request_input(line0, GPIOD_TEST_CONSUMER);
	g_assert_cmpint(ret, ==, 0);
	gpiod_test_return_if_failed();

	ret = gpiod_chip_watch_event_wait(chip, &ts);
	g_assert_cmpint(ret, ==, 0);
	gpiod_test_return_if_failed();

	gpiod_line_release(line0);

	ret = gpiod_line_watch_bulk(&lines);
	g_assert_cmpint(ret, ==, 0);
	gpiod_test_return_if_failed();

	ret = gpiod_line_request_input(line0, GPIOD_TEST_CONSUMER);
	g_assert_cmpint(ret, ==, 0);
	gpiod_test_return_if_failed();

	ret = gpiod_line_request_output(line1, GPIOD_TEST_CONSUMER, 1);
	g_assert_cmpint(ret, ==, 0);
	gpiod_test_return_if_failed();

	gpiod_line_release(line1);

	ret = gpiod_chip_watch_event_wait(chip, &ts);
	g_assert_cmpint(ret, ==, 1);
	gpiod_test_return_if_failed();

	ret = gpiod_chip_watch_event_read_multiple(chip, events, 3);
	g_assert_cmpint(ret, ==, 3);
	gpiod_test_return_if_failed();

	g_assert_cmpint(events[0].event_type, ==,
			GPIOD_WATCH_EVENT_LINE_REQUESTED);
	g_assert_cmpint(gpiod_line_offset(events[0].line), ==,
			gpiod_line_offset(line0));

	g_assert_cmpint(events[1].event_type, ==,
			GPIOD_WATCH_EVENT_LINE_REQUESTED);
	g_assert_cmpint(gpiod_line_offset(events[1].line), ==,
			gpiod_line_offset(line1));

	g_assert_cmpint(events[2].event_type, ==,
			GPIOD_WATCH_EVENT_LINE_RELEASED);
	g_assert_cmpint(gpiod_line_offset(events[2].line), ==,
			gpiod_line_offset(line1));

	gpiod_test_return_if_failed();

	ret = gpiod_line_unwatch_bulk(&lines);
	g_assert_cmpint(ret, ==, 0);
	gpiod_test_return_if_failed();

	/* Verify the lines are no longer watched again. */
	ret = gpiod_line_request_input(line1, GPIOD_TEST_CONSUMER);
	g_assert_cmpint(ret, ==, 0);
	gpiod_test_return_if_failed();

	ret = gpiod_chip_watch_event_wait(chip, &ts);
	g_assert_cmpint(ret, ==, 0);
}

GPIOD_TEST_CASE(unwatch_works, 0, { 8 })
{
	g_autoptr(gpiod_chip_struct) chip = NULL;
	struct gpiod_watch_event event;
	struct timespec ts = { 1, 0 };
	struct gpiod_line *line;
	gint ret;

	chip = gpiod_chip_open(gpiod_test_chip_path(0));
	g_assert_nonnull(chip);
	gpiod_test_return_if_failed();

	line = gpiod_chip_get_line_watched(chip, 2);
	g_assert_nonnull(line);
	gpiod_test_return_if_failed();

	/* Verify line watch works. */
	ret = gpiod_line_request_input(line, GPIOD_TEST_CONSUMER);
	g_assert_cmpint(ret, ==, 0);
	gpiod_test_return_if_failed();

	ret = gpiod_chip_watch_event_wait(chip, &ts);
	g_assert_cmpint(ret, ==, 1);
	gpiod_test_return_if_failed();

	ret = gpiod_chip_watch_event_read(chip, &event);
	g_assert_cmpint(ret, ==, 0);
	gpiod_test_return_if_failed();

	g_assert_cmpint(event.event_type, ==, GPIOD_WATCH_EVENT_LINE_REQUESTED);
	g_assert_cmpint(gpiod_line_offset(line), ==,
			gpiod_line_offset(event.line));

	ret = gpiod_line_unwatch(line);
	g_assert_cmpint(ret, ==, 0);
	gpiod_test_return_if_failed();

	/* Check no more events are received. */
	gpiod_line_release(line);

	ts.tv_sec = 0;
	ts.tv_nsec = 100000;

	ret = gpiod_chip_watch_event_wait(chip, &ts);
	g_assert_cmpint(ret, ==, 0);
}

GPIOD_TEST_CASE(try_to_watch_line_twice, 0, { 8 })
{
	g_autoptr(gpiod_chip_struct) chip = NULL;
	struct gpiod_line *line;
	gint ret;

	chip = gpiod_chip_open(gpiod_test_chip_path(0));
	g_assert_nonnull(chip);
	gpiod_test_return_if_failed();

	line = gpiod_chip_get_line_watched(chip, 2);
	g_assert_nonnull(line);
	gpiod_test_return_if_failed();

	ret = gpiod_line_watch(line);
	g_assert_cmpint(ret, ==, -1);
	g_assert_cmpint(errno, ==, EBUSY);
}

GPIOD_TEST_CASE(try_to_unwatch_non_watched_line, 0, { 8 })
{
	g_autoptr(gpiod_chip_struct) chip = NULL;
	struct gpiod_line *line;
	gint ret;

	chip = gpiod_chip_open(gpiod_test_chip_path(0));
	g_assert_nonnull(chip);
	gpiod_test_return_if_failed();

	line = gpiod_chip_get_line(chip, 2);
	g_assert_nonnull(line);
	gpiod_test_return_if_failed();

	ret = gpiod_line_unwatch(line);
	g_assert_cmpint(ret, ==, -1);
	g_assert_cmpint(errno, ==, EBUSY);
}
