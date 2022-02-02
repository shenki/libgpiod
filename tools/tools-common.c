// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2017-2021 Bartosz Golaszewski <bartekgola@gmail.com>

/* Common code for GPIO tools. */

#include <ctype.h>
#include <errno.h>
#include <gpiod.h>
#include <libgen.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/signalfd.h>

#include "tools-common.h"

const char *get_progname(void)
{
	return program_invocation_name;
}

void die(const char *fmt, ...)
{
	va_list va;

	va_start(va, fmt);
	fprintf(stderr, "%s: ", program_invocation_name);
	vfprintf(stderr, fmt, va);
	fprintf(stderr, "\n");
	va_end(va);

	exit(EXIT_FAILURE);
}

void die_perror(const char *fmt, ...)
{
	va_list va;

	va_start(va, fmt);
	fprintf(stderr, "%s: ", program_invocation_name);
	vfprintf(stderr, fmt, va);
	fprintf(stderr, ": %s\n", strerror(errno));
	va_end(va);

	exit(EXIT_FAILURE);
}

void print_version(void)
{
	printf("%s (libgpiod) v%s\n",
	       program_invocation_short_name, gpiod_version_string());
	printf("Copyright (C) 2017-2018 Bartosz Golaszewski\n");
	printf("License: LGPLv2.1\n");
	printf("This is free software: you are free to change and redistribute it.\n");
	printf("There is NO WARRANTY, to the extent permitted by law.\n");
}

int bias_flags(const char *option)
{
	if (strcmp(option, "pull-down") == 0)
		return GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_DOWN;
	if (strcmp(option, "pull-up") == 0)
		return GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_UP;
	if (strcmp(option, "disable") == 0)
		return GPIOD_LINE_REQUEST_FLAG_BIAS_DISABLED;
	if (strcmp(option, "as-is") != 0)
		die("invalid bias: %s", option);
	return 0;
}

void print_bias_help(void)
{
	printf("Biases:\n");
	printf("  as-is:\tleave bias unchanged\n");
	printf("  disable:\tdisable bias\n");
	printf("  pull-up:\tenable pull-up\n");
	printf("  pull-down:\tenable pull-down\n");
}

int make_signalfd(void)
{
	sigset_t sigmask;
	int sigfd, rv;

	sigemptyset(&sigmask);
	sigaddset(&sigmask, SIGTERM);
	sigaddset(&sigmask, SIGINT);

	rv = sigprocmask(SIG_BLOCK, &sigmask, NULL);
	if (rv < 0)
		die("error masking signals: %s", strerror(errno));

	sigfd = signalfd(-1, &sigmask, 0);
	if (sigfd < 0)
		die("error creating signalfd: %s", strerror(errno));

	return sigfd;
}

int chip_dir_filter(const struct dirent *entry)
{
	bool is_chip;
	char *path;
	int ret;

	ret = asprintf(&path, "/dev/%s", entry->d_name);
	if (ret < 0)
		return 0;

	is_chip = gpiod_is_gpiochip_device(path);
	free(path);
	return !!is_chip;
}

struct gpiod_chip *chip_open_by_name(const char *name)
{
	struct gpiod_chip *chip;
	char *path;
	int ret;

	ret = asprintf(&path, "/dev/%s", name);
	if (ret < 0)
		return NULL;

	chip = gpiod_chip_open(path);
	free(path);

	return chip;
}

static struct gpiod_chip *chip_open_by_number(unsigned int num)
{
	struct gpiod_chip *chip;
	char *path;
	int ret;

	ret = asprintf(&path, "/dev/gpiochip%u", num);
	if (!ret)
		return NULL;

	chip = gpiod_chip_open(path);
	free(path);

	return chip;
}

static bool isuint(const char *str)
{
	for (; *str && isdigit(*str); str++)
		;

	return *str == '\0';
}

struct gpiod_chip *chip_open_lookup(const char *device)
{
	struct gpiod_chip *chip;

	if (isuint(device)) {
		chip = chip_open_by_number(strtoul(device, NULL, 10));
	} else {
		if (strncmp(device, "/dev/", 5))
			chip = chip_open_by_name(device);
		else
			chip = gpiod_chip_open(device);
	}

	return chip;
}


struct gpiod_chip *chip_by_line_name(const char *name)
{
	int num_chips, offset, i;
	struct dirent **entries;
	struct gpiod_chip *chip;


	num_chips = scandir("/dev/", &entries, chip_dir_filter, alphasort);
	if (num_chips < 0)
		die_perror("unable to scan /dev");

	for (i = 0; i < num_chips; i++) {
		chip = chip_open_by_name(entries[i]->d_name);
		if (!chip) {
			if (errno == EACCES)
				continue;

			die_perror("unable to open %s", entries[i]->d_name);
		}
		free(entries[i]);

		offset = gpiod_chip_find_line(chip, name);
		if (offset >= 0) {
			free(entries);
			return chip;
		}
		gpiod_chip_unref(chip);
	}

	free(entries);
	return NULL;
}

char *split_line(const char *line_pair)
{
	char *name_end;
	size_t name_len;
	char *line_name;

	name_end = strchr(line_pair, '=');
	if (!name_end)
		die("invalid name/value '%s'", line_pair);

	name_len = name_end - line_pair;

	if (name_len > 32)
		die("line name exceeds maximum length");

	line_name = calloc(1, name_len + 1);
	strncpy(line_name, line_pair, name_len);

	return line_name;
}

int line_names_to_offsets(struct gpiod_chip *chip, char **lines,
			  unsigned int *offsets,
			  int *values,
			  int num_lines)
{
	int i;

	for (i = 0; i < num_lines; i++) {
		char *line_name;
		int value;
		int offset;

		if (values) {
			const char *line_pair = lines[i];
			char *name_end;
			int rv;

			line_name = split_line(line_pair);
			name_end = strchr(line_pair, '=');

			rv = sscanf(name_end, "=%d", &value);
			if (rv != 1)
				die("invalid offset<->value mapping: %s", line_pair);

			if (value != 0 && value != 1)
				die("value must be 0 or 1: %s", line_pair);
		} else {
			line_name = lines[i];
		}

		offset = gpiod_chip_find_line(chip, line_name);

		if (offset < 0) {
			die("chip '%s' does not contain line '%s'",
					gpiod_chip_get_name(chip),
					line_name);
		}

		offsets[i] = offset;

		if (values) {
			values[i] = value;
			free(line_name);
		}
	}

	return 0;
}
