/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * This file is part of libgpiod.
 *
 * Copyright (C) 2020 Bartosz Golaszewski <bgolaszewski@baylibre.com>
 */

#include <catch2/catch.hpp>
#include <chrono>
#include <gpiod.hpp>

#include "gpio-mockup.hpp"

using ::gpiod::test::mockup;

namespace {

const ::std::string consumer = "line-watch-test";

} /* namespace */

TEST_CASE("Detecting line state change events works", "[watch][chip]")
{
	mockup::probe_guard mockup_chips({ 8 });
	::gpiod::chip chip(mockup::instance().chip_name(0));
	::gpiod::line_request config;

	SECTION("single request event")
	{
		auto line = chip.get_line(4, true);

		config.consumer = consumer.c_str();
		config.request_type = ::gpiod::line_request::DIRECTION_OUTPUT;
		line.request(config);

		auto got_event = chip.watch_event_wait(::std::chrono::seconds(1));
		REQUIRE(got_event);

		auto event = chip.watch_event_read();
		REQUIRE(event.source == line);
		REQUIRE(event.event_type == ::gpiod::watch_event::REQUESTED);
	}
}
