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

#include "TimeConverter.h"
#include "MonitorEvent.h"

using std::string;

MonitorEvent::MonitorEvent() :
	m_isWatch(false),
	m_type(UNKNOWN),
	m_isDirectory(false),
	m_time(time(NULL))
{
}

MonitorEvent::MonitorEvent(const MonitorEvent &other) :
	m_location(other.m_location),
	m_previousLocation(other.m_previousLocation),
	m_isWatch(other.m_isWatch),
	m_type(other.m_type),
	m_isDirectory(other.m_isDirectory),
	m_time(other.m_time)
{
}

MonitorEvent::~MonitorEvent()
{
}

MonitorEvent& MonitorEvent::operator=(const MonitorEvent& other)
{
	if (this != &other)
	{
		m_location = other.m_location;
		m_previousLocation = other.m_previousLocation;
		m_isWatch = other.m_isWatch;
		m_type = other.m_type;
		m_isDirectory = other.m_isDirectory;
		m_time = other.m_time;
	}

	return *this;
}

bool MonitorEvent::operator<(const MonitorEvent& other) const
{
	if (m_location < other.m_location)
	{
		return true;
	}
	else if (m_location == other.m_location)
	{
		if (m_type < other.m_type)
		{
			return true;
		}
	}

	return false;
}

bool MonitorEvent::operator==(const MonitorEvent& other) const
{
	if ((m_location == other.m_location) &&
		(m_type == other.m_type))
	{
		return true;
	}

	return false;
}
