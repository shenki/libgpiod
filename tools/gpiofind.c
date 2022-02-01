// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2017-2021 Bartosz Golaszewski <bartekgola@gmail.com>

#include <dirent.h>
#include <errno.h>
#include <getopt.h>
#include <gpiod.h>
#include <stdio.h>
#include <string.h>

#include "tools-common.h"

static const struct option longopts[] = {
	{ "help",	no_argument,	NULL,	'h' },
	{ "version",	no_argument,	NULL,	'v' },
	{ GETOPT_NULL_LONGOPT },
};

static const char *const shortopts = "+hv";

static void print_help(void)
{
	printf("Usage: %s [OPTIONS] <name>\n", get_progname());
	printf("\n");
	printf("Find a GPIO line by name. The output of this command can be used as input for gpioget/set.\n");
	printf("\n");
	printf("Options:\n");
	printf("  -h, --help:\t\tdisplay this message and exit\n");
	printf("  -v, --version:\tdisplay the version and exit\n");
}

int main(int argc, char **argv)
{
	struct gpiod_chip *chip;
	int optc, opti, offset;

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
		case '?':
			die("try %s --help", get_progname());
		default:
			abort();
		}
	}

	argc -= optind;
	argv += optind;

	if (argc != 1)
		die("exactly one GPIO line name must be specified");


	chip = chip_by_line_name(argv[0]);
	if (chip) {
		offset = gpiod_chip_find_line(chip, argv[0]);

		printf("%s %u\n", gpiod_chip_get_name(chip), offset);
		gpiod_chip_unref(chip);
		return EXIT_SUCCESS;
	}

	return EXIT_FAILURE;
}
