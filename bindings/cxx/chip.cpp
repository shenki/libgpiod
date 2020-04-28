// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * This file is part of libgpiod.
 *
 * Copyright (C) 2017-2018 Bartosz Golaszewski <bartekgola@gmail.com>
 * Copyright (C) 2020 Bartosz Golaszewski <bgolaszewski@baylibre.com>
 */

#include <functional>
#include <gpiod.hpp>
#include <map>
#include <system_error>
#include <utility>

namespace gpiod {

namespace {

::gpiod_chip* open_lookup(const ::std::string& device)
{
	return ::gpiod_chip_open_lookup(device.c_str());
}

::gpiod_chip* open_by_path(const ::std::string& device)
{
	return ::gpiod_chip_open(device.c_str());
}

::gpiod_chip* open_by_name(const ::std::string& device)
{
	return ::gpiod_chip_open_by_name(device.c_str());
}

::gpiod_chip* open_by_label(const ::std::string& device)
{
	return ::gpiod_chip_open_by_label(device.c_str());
}

::gpiod_chip* open_by_number(const ::std::string& device)
{
	return ::gpiod_chip_open_by_number(::std::stoul(device));
}

using open_func = ::std::function<::gpiod_chip* (const ::std::string&)>;

const ::std::map<int, open_func> open_funcs = {
	{ chip::OPEN_LOOKUP,	open_lookup,	},
	{ chip::OPEN_BY_PATH,	open_by_path,	},
	{ chip::OPEN_BY_NAME,	open_by_name,	},
	{ chip::OPEN_BY_LABEL,	open_by_label,	},
	{ chip::OPEN_BY_NUMBER,	open_by_number,	},
};

void chip_deleter(::gpiod_chip* chip)
{
	::gpiod_chip_close(chip);
}

} /* namespace */

chip::chip(const ::std::string& device, int how)
	: _m_chip()
{
	this->open(device, how);
}

chip::chip(::gpiod_chip* chip)
	: _m_chip(chip, chip_deleter)
{

}

void chip::open(const ::std::string& device, int how)
{
	auto func = open_funcs.at(how);

	::gpiod_chip *chip = func(device);
	if (!chip)
		throw ::std::system_error(errno, ::std::system_category(),
					  "cannot open GPIO device " + device);

	this->_m_chip.reset(chip, chip_deleter);
}

void chip::reset(void) noexcept
{
	this->_m_chip.reset();
}

::std::string chip::name(void) const
{
	this->throw_if_noref();

	return ::std::string(::gpiod_chip_name(this->_m_chip.get()));
}

::std::string chip::label(void) const
{
	this->throw_if_noref();

	return ::std::string(::gpiod_chip_label(this->_m_chip.get()));
}

unsigned int chip::num_lines(void) const
{
	this->throw_if_noref();

	return ::gpiod_chip_num_lines(this->_m_chip.get());
}

line chip::get_line(unsigned int offset, bool watched) const
{
	::std::function<::gpiod_line* (::gpiod_chip*, unsigned int)> func;
	::gpiod_line* line_handle;

	this->throw_if_noref();

	func = watched ? ::gpiod_chip_get_line_watched : ::gpiod_chip_get_line;

	if (offset >= this->num_lines())
		throw ::std::out_of_range("line offset greater than the number of lines");

	line_handle = func(this->_m_chip.get(), offset);
	if (!line_handle)
		throw ::std::system_error(errno, ::std::system_category(),
					  "error getting GPIO line from chip");

	return line(line_handle, *this);
}

line chip::find_line(const ::std::string& name, bool watched) const
{
	::std::function<::gpiod_line* (::gpiod_chip*, const char*)> func;
	::gpiod_line* handle;

	this->throw_if_noref();

	func = watched ? ::gpiod_chip_find_line_watched : ::gpiod_chip_find_line;

	handle = func(this->_m_chip.get(), name.c_str());
	if (!handle && errno != ENOENT)
		throw ::std::system_error(errno, ::std::system_category(),
					  "error looking up GPIO line by name");

	return handle ? line(handle, *this) : line();
}

line_bulk chip::get_lines(const ::std::vector<unsigned int>& offsets, bool watched) const
{
	line_bulk lines;

	for (auto& it: offsets)
		lines.append(this->get_line(it, watched));

	return lines;
}

line_bulk chip::get_all_lines(bool watched) const
{
	line_bulk lines;

	for (unsigned int i = 0; i < this->num_lines(); i++)
		lines.append(this->get_line(i, watched));

	return lines;
}

line_bulk chip::find_lines(const ::std::vector<::std::string>& names, bool watched) const
{
	line_bulk lines;
	line line;

	for (auto& it: names) {
		line = this->find_line(it, watched);
		if (!line) {
			lines.clear();
			return lines;
		}

		lines.append(line);
	}

	return lines;
}

bool chip::watch_event_wait(const ::std::chrono::nanoseconds& timeout) const
{
	this->throw_if_noref();

	::timespec ts;
	int ret;

	ts.tv_sec = timeout.count() / 1000000000ULL;
	ts.tv_nsec = timeout.count() % 1000000000ULL;

	ret = ::gpiod_chip_watch_event_wait(this->_m_chip.get(), ::std::addressof(ts));
	if (ret < 0)
		throw ::std::system_error(errno, ::std::system_category(),
					  "error waiting for line watch events");

	return ret > 0 ? true : false;
}

watch_event chip::make_watch_event(const ::gpiod_watch_event& event_buf) const noexcept
{
	watch_event event;

	if (event_buf.event_type == GPIOD_WATCH_EVENT_LINE_REQUESTED)
		event.event_type = watch_event::REQUESTED;
	else if (event_buf.event_type == GPIOD_WATCH_EVENT_LINE_RELEASED)
		event.event_type = watch_event::RELEASED;
	else if (event_buf.event_type == GPIOD_WATCH_EVENT_LINE_CONFIG_CHANGED)
		event.event_type = watch_event::CONFIG_CHANGED;

	event.timestamp = ::std::chrono::nanoseconds(
				event_buf.ts.tv_nsec + (event_buf.ts.tv_sec * 1000000000));

	event.source = line(event_buf.line, *this);

	return event;
}

watch_event chip::watch_event_read(void) const
{
	this->throw_if_noref();

	::gpiod_watch_event event_buf;
	int ret;

	ret = ::gpiod_chip_watch_event_read(this->_m_chip.get(),
					    ::std::addressof(event_buf));
	if (ret < 0)
		throw ::std::system_error(errno, ::std::system_category(),
					  "error reading line watch event");

	return this->make_watch_event(event_buf);
}

::std::vector<watch_event> chip::watch_event_read_multiple(void) const
{
	this->throw_if_noref();

	/* 32 is the maximum number of watch events stored in the kernel FIFO. */
	::std::array<::gpiod_watch_event, 32> event_buf;
	::std::vector<watch_event> events;
	int ret;

	ret = ::gpiod_chip_watch_event_read_multiple(this->_m_chip.get(),
						     event_buf.data(), event_buf.size());
	if (ret < 0)
		throw ::std::system_error(errno, ::std::system_category(),
					  "error reading line watch events");

	events.reserve(ret);
	for (int i = 0; i < ret; i++)
		events.push_back(this->make_watch_event(event_buf[i]));

	return events;
}

int chip::watch_event_get_fd(void) const
{
	this->throw_if_noref();

	return ::gpiod_chip_watch_get_fd(this->_m_chip.get());
}

void chip::unwatch_all(void) const
{
	this->throw_if_noref();

	::gpiod_chip_unwatch_all(this->_m_chip.get());
}

bool chip::operator==(const chip& rhs) const noexcept
{
	return this->_m_chip.get() == rhs._m_chip.get();
}

bool chip::operator!=(const chip& rhs) const noexcept
{
	return this->_m_chip.get() != rhs._m_chip.get();
}

chip::operator bool(void) const noexcept
{
	return this->_m_chip.get() != nullptr;
}

bool chip::operator!(void) const noexcept
{
	return this->_m_chip.get() == nullptr;
}

void chip::throw_if_noref(void) const
{
	if (!this->_m_chip.get())
		throw ::std::logic_error("object not associated with an open GPIO chip");
}

} /* namespace gpiod */
