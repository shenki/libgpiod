// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * This file is part of libgpiod.
 *
 * Copyright (C) 2020 Bartosz Golaszewski <bgolaszewski@baylibre.com>
 */

#include <getopt.h>
#include <gpiod.h>
#include <limits.h>
#include <poll.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "tools-common.h"

static const struct option longopts[] = {
	{ "help",		no_argument,		NULL,	'h' },
	{ "version",		no_argument,		NULL,	'v' },
	{ "line-buffered",	no_argument,		NULL,	'b' },
	{ "num-events",		required_argument,	NULL,	'n' },
	{ "silent",		no_argument,		NULL,	's' },
	{ "verbose",		no_argument,		NULL,	'V' },
	{ "filter",		required_argument,	NULL,	'f' },
	{ GETOPT_NULL_LONGOPT },
};

static const char *const shortopts = "+hvbn:sVf:";

static void print_help(void)
{
	printf("Usage: %s [OPTIONS] <chip name/number> <offset 1> <offset2> ...\n",
	       get_progname());
	printf("\n");
	printf("Monitor state changes of GPIO lines (request, release and config operations).\n");
	printf("\n");
	printf("Options:\n");
	printf("  -h, --help:\t\tdisplay this message and exit\n");
	printf("  -v, --version:\tdisplay the version and exit\n");
	printf("  -b, --line-buffered:\tset standard output as line buffered\n");
	printf("  -n, --num-events=NUM:\texit after processing NUM events\n");
	printf("  -s, --silent:\t\tdon't print event info\n");
	printf("  -V, --verbose:\t\tprint previous and new line info on every event\n");
	printf("  -f, --filter=[request,release,config]\tspecify a comma-separated list of event types to display\n");
}

#define FLAG_OPEN_DRAIN		GPIOD_BIT(0)
#define FLAG_OPEN_SOURCE	GPIOD_BIT(1)
#define FLAG_PULL_UP		GPIOD_BIT(2)
#define FLAG_PULL_DOWN		GPIOD_BIT(3)
#define FLAG_BIAS_DISABLE	GPIOD_BIT(4)

struct line_config {
	bool named;
	char name[32];
	bool used;
	char consumer[32];
	bool dir_out;
	bool active_low;
	int flags;
};

static const char *evtypestr(struct gpiod_watch_event *event)
{
	const char *ret = NULL;

	switch (event->event_type) {
	case GPIOD_WATCH_EVENT_LINE_REQUESTED:
		ret = "     REQUESTED";
		break;
	case GPIOD_WATCH_EVENT_LINE_RELEASED:
		ret = "      RELEASED";
		break;
	case GPIOD_WATCH_EVENT_LINE_CONFIG_CHANGED:
		ret = "CONFIG CHANGED";
		break;
	}

	return ret;
}

static void line_to_linecfg(struct gpiod_line *line, struct line_config *cfg)
{
	const char *name, *consumer;

	memset(cfg, 0, sizeof(*cfg));

	name = gpiod_line_name(line);
	consumer = gpiod_line_consumer(line);

	if (name) {
		cfg->named = true;
		strncpy(cfg->name, name, sizeof(cfg->name) - 1);
	}
	cfg->used = gpiod_line_is_used(line);
	if (consumer)
		strncpy(cfg->consumer, consumer, sizeof(cfg->consumer) - 1);
	cfg->dir_out
		= gpiod_line_direction(line) == GPIOD_LINE_DIRECTION_OUTPUT;
	cfg->active_low
		= gpiod_line_active_state(line) == GPIOD_LINE_ACTIVE_STATE_LOW;

	if (gpiod_line_is_open_drain(line))
		cfg->flags |= FLAG_OPEN_DRAIN;
	if (gpiod_line_is_open_source(line))
		cfg->flags |= FLAG_OPEN_SOURCE;

	if (gpiod_line_bias(line) == GPIOD_LINE_BIAS_PULL_UP)
		cfg->flags |= FLAG_PULL_UP;
	else if (gpiod_line_bias(line) == GPIOD_LINE_BIAS_PULL_DOWN)
		cfg->flags |= FLAG_PULL_DOWN;
	else if (gpiod_line_bias(line) == GPIOD_LINE_BIAS_DISABLE)
		cfg->flags |= FLAG_BIAS_DISABLE;
}

static void print_event(struct gpiod_watch_event *event)
{
	printf("event: %s offset: %3u timestamp: [%8ld.%09ld]\n",
	       evtypestr(event),
	       gpiod_line_offset(event->line),
	       event->ts.tv_sec, event->ts.tv_nsec);
}

static void print_config(struct line_config *cfg)
{
	printf("{");

	if (cfg->named)
		printf("\"%s\" ", cfg->name);
	else
		printf("unnamed ");

	if (cfg->used)
		printf("\"%s\" ", cfg->consumer);
	else
		printf("unused ");

	printf("%s ", cfg->dir_out ? "output" : "input");
	printf("%s", cfg->active_low ? "active-low" : "active-high");

	if (cfg->flags) {
		printf(" [");
		if (cfg->used)
			printf("used");
		if (cfg->flags & FLAG_OPEN_DRAIN)
			printf(" open-drain");
		if (cfg->flags & FLAG_OPEN_SOURCE)
			printf(" open-source");
		if (cfg->flags & FLAG_PULL_UP)
			printf(" pull-up");
		if (cfg->flags & FLAG_PULL_DOWN)
			printf(" pull-down");
		if (cfg->flags & FLAG_BIAS_DISABLE)
			printf(" bias-disabled");
		printf("]");
	}

	printf("}");
}

static void print_config_change(struct line_config *cfg,
				struct gpiod_line *line)
{
	print_config(cfg);
	line_to_linecfg(line, cfg); /* Replace old config. */
	printf(" -> ");
	print_config(cfg);
	printf("\n");
}

int main(int argc, char **argv)
{
	struct gpiod_line_bulk lines = GPIOD_LINE_BULK_INITIALIZER;
	unsigned int max_events = 0, processed_events = 0, offset;
	struct line_config *cfg[GPIOD_LINE_BULK_MAX_LINES];
	struct gpiod_watch_event events[32], *event;
	int optc, opti, num_lines, i, ret, fd;
	bool silent = false, verbose = false;
	struct gpiod_line *line;
	struct gpiod_chip *chip;
	unsigned int *offsets;
	struct pollfd pfd[2];
	char *device, *end;

	for (;;) {
		optc = getopt_long(argc, argv, shortopts, longopts, &opti);
		if (optc < 0)
			break;

		switch (optc) {
		case 'h':
			print_help();
			return EXIT_SUCCESS;
		case 'v':
			print_version();
			return EXIT_SUCCESS;
		case 'b':
			setlinebuf(stdout);
			break;
		case 'n':
			max_events = strtoul(optarg, &end, 10);
			if (*end != '\0')
				die("invalid number: %s", optarg);
			break;
		case 's':
			silent = true;
			break;
		case 'V':
			verbose = true;
			break;
		case '?':
			die("try %s --help", get_progname());
		default:
			abort();
		}
	}

	argc -= optind;
	argv += optind;

	if (argc < 1)
		die("gpiochip must be specified");

	if (argc < 2)
		die("at least one GPIO line offset must be specified");

	if (silent && verbose)
		die("-s/--silent and -V/--verbose must not be used at the same time");

	device = argv[0];
	argc--;
	argv++;
	num_lines = argc;

	offsets = calloc(num_lines, sizeof(*offsets));
	if (!offsets)
		die("out of memory");

	for (i = 0; i < num_lines; i++) {
		offsets[i] = strtoul(argv[i], &end, 10);
		if (*end != '\0' || offsets[i] > INT_MAX)
			die("invalid GPIO offset: %s", argv[i]);
	}

	chip = gpiod_chip_open_lookup(device);
	if (!chip)
		die_perror("unable to access the GPIO chip %s", device);

	ret = gpiod_chip_get_lines_watched(chip, offsets, num_lines, &lines);
	if (ret)
		die_perror("unable to retrieve GPIO lines");

	if (verbose) {
		memset(cfg, 0, sizeof(cfg));

		gpiod_line_bulk_foreach_line_off(&lines, line, offset) {
			cfg[offset] = malloc(sizeof(struct line_config));
			if (!cfg[offset])
				die("out of memory");

			line_to_linecfg(line, cfg[offset]);
		}
	}

	fd = gpiod_chip_watch_get_fd(chip);

	memset(pfd, 0, sizeof(pfd));
	pfd[0].fd = fd;
	pfd[1].fd = make_signalfd();
	pfd[0].events = pfd[1].events = POLLIN | POLLPRI;

	for (;;) {
		ret = poll(pfd, 2, 10000);
		if (ret < 0)
			die_perror("poll error");
		else if (ret == 0)
			continue;

		if (pfd[1].revents)
			/*
			 * We received SIGINT or SIGTERM. Don't bother reading
			 * siginfo.
			 */
			break;

		/* New watch events queued. */

		ret = gpiod_chip_watch_event_read_multiple(chip, events,
							   ARRAY_SIZE(events));
		if (ret < 0)
			die_perror("error reading line state change events");

		for (i = 0; i < ret; i++) {
			event = &events[i];
			if (!silent)
				print_event(event);

			if (verbose) {
				offset = gpiod_line_offset(event->line);
				print_config_change(cfg[offset], event->line);
			}

			if (max_events) {
				processed_events++;
				if (processed_events == max_events)
					goto out;
			}
		}
	}

out:
	if (verbose) {
		gpiod_line_bulk_foreach_line_off(&lines, line, offset)
			free(cfg[offset]);
	}

	close(pfd[1].fd); /* signalfd */
	gpiod_chip_close(chip);
	free(offsets);

	return EXIT_SUCCESS;
}
