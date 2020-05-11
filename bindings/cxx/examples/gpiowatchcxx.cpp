// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * This file is part of libgpiod.
 *
 * Copyright (C) 2020 Bartosz Golaszewski <bgolaszewski@baylibre.com>
 */

/* Simplified C++ reimplementation of the gpiowatch tool. */

#include <gpiod.hpp>

#include <cstdlib>
#include <iostream>
#include <string>

namespace {

void print_event(const ::gpiod::watch_event& event)
{
	::std::string evstr;

	if (event.event_type == ::gpiod::watch_event::REQUESTED)
		evstr = "     REQUESTED";
	else if (event.event_type == ::gpiod::watch_event::RELEASED)
		evstr = "      RELEASED";
	else if (event.event_type == ::gpiod::watch_event::CONFIG_CHANGED)
		evstr = "CONFIG CHANGED";

	::std::cout << "event: " << evstr;
	::std::cout << ::std::endl;
}

} /* namespace */

int main(int argc, char **argv)
{
	if (argc < 3) {
		::std::cout << "usage: " << argv[0] << " <chip> <offset0> ..." << ::std::endl;
		return EXIT_FAILURE;
	}

	::std::vector<unsigned int> offsets;
	offsets.reserve(argc);
	for (int i = 2; i < argc; i++)
		offsets.push_back(::std::stoul(argv[i]));

	::gpiod::chip chip(argv[1]);
	auto lines = chip.get_lines(offsets, true);

	for (;;) {
		if (chip.watch_event_wait(::std::chrono::seconds(1))) {
			auto events = chip.watch_event_read_multiple();
			for (auto& it: events)
				print_event(it);
		}
	}

	return EXIT_SUCCESS;
}
