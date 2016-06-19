/*
 *  Copyright 2005,2006 Fabrice Colin
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "Timer.h"

Timer::Timer()
{
	gettimeofday(&m_start, NULL);
	gettimeofday(&m_stop, NULL);
}

Timer::Timer(const Timer &other) :
	m_start(other.m_start),
	m_stop(other.m_stop)
{
}


Timer &Timer::operator=(const Timer &other)
{
	if (this != &other)
	{
		m_start = other.m_start;
		m_stop = other.m_stop;
	}

	return *this;
}

Timer::~Timer()
{
}

/// Starts the timer.
void Timer::start(void)
{
	gettimeofday(&m_start, NULL);
}

/// Stops the timer and returns the number of milliseconds elapsed.
long Timer::stop(void)
{
	gettimeofday(&m_stop, NULL);

	return (long)(((m_stop.tv_sec - m_start.tv_sec) * 1000) + ((m_stop.tv_usec - m_start.tv_usec) / 1000));
}
