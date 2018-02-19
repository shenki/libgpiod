/*
 * This file is part of libgpiod.
 *
 * Copyright (C) 2017-2018 Bartosz Golaszewski <bartekgola@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or (at
 * your option) any later version.
 */

#include <gpiod.hpp>

#include <system_error>

namespace gpiod {

namespace {

void chip_iter_deleter(::gpiod_chip_iter* iter)
{
	::gpiod_chip_iter_free_noclose(iter);
}

void line_iter_deleter(::gpiod_line_iter* iter)
{
	::gpiod_line_iter_free(iter);
}

::gpiod_line_iter* make_line_iterator(::gpiod_chip* chip)
{
	::gpiod_line_iter* iter;

	iter = ::gpiod_line_iter_new(chip);
	if (!iter)
		throw ::std::system_error(errno, ::std::system_category(),
					  "error creating GPIO line iterator");

	return iter;
}

} /* namespace */

chip_iterator make_chip_iterator(void)
{
	::gpiod_chip_iter* iter = ::gpiod_chip_iter_new();
	if (!iter)
		throw ::std::system_error(errno, ::std::system_category(),
					  "error creating GPIO chip iterator");

	return ::std::move(chip_iterator(iter));
}

bool chip_iterator::operator==(const chip_iterator& rhs) const noexcept
{
	return this->_m_current == rhs._m_current;
}

bool chip_iterator::operator!=(const chip_iterator& rhs) const noexcept
{
	return this->_m_current != rhs._m_current;
}

chip_iterator::chip_iterator(::gpiod_chip_iter *iter)
	: _m_iter(iter, chip_iter_deleter),
	  _m_current(chip(::gpiod_chip_iter_next_noclose(this->_m_iter.get())))
{

}

chip_iterator& chip_iterator::operator++(void)
{
	::gpiod_chip* next = ::gpiod_chip_iter_next_noclose(this->_m_iter.get());

	this->_m_current = next ? chip(next) : chip();

	return *this;
}

const chip& chip_iterator::operator*(void) const
{
	return this->_m_current;
}

const chip* chip_iterator::operator->(void) const
{
	return ::std::addressof(this->_m_current);
}

chip_iterator begin(chip_iterator iter) noexcept
{
	return iter;
}

chip_iterator end(const chip_iterator&) noexcept
{
	return ::std::move(chip_iterator());
}

line_iterator begin(line_iterator iter) noexcept
{
	return iter;
}

line_iterator end(const line_iterator&) noexcept
{
	return ::std::move(line_iterator());
}

line_iterator::line_iterator(const chip& owner)
	: _m_iter(make_line_iterator(owner._m_chip.get()), line_iter_deleter),
	  _m_current(line(::gpiod_line_iter_next(this->_m_iter.get()), owner))
{

}

line_iterator& line_iterator::operator++(void)
{
	::gpiod_line* next = ::gpiod_line_iter_next(this->_m_iter.get());

	this->_m_current = next ? line(next, this->_m_current._m_chip) : line();

	return *this;
}

const line& line_iterator::operator*(void) const
{
	return this->_m_current;
}

const line* line_iterator::operator->(void) const
{
	return ::std::addressof(this->_m_current);
}

bool line_iterator::operator==(const line_iterator& rhs) const noexcept
{
	return this->_m_current._m_line == rhs._m_current._m_line;
}

bool line_iterator::operator!=(const line_iterator& rhs) const noexcept
{
	return this->_m_current._m_line != rhs._m_current._m_line;
}

} /* namespace gpiod */