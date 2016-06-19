/*
 *  Copyright 2008 Fabrice Colin
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

#ifndef _MODULE_PROPERTIES_H
#define _MODULE_PROPERTIES_H

#include <string>
#include <set>

/// Properties of a module.
class ModuleProperties
{
	public:
		ModuleProperties()
		{
		}
		ModuleProperties(const std::string &name,
			const std::string &longName,
			const std::string &option,
			const std::string &channel) :
			m_name(name),
			m_longName(longName),
			m_option(option),
			m_channel(channel)
		{
		}
		ModuleProperties(const ModuleProperties &other) :
			m_name(other.m_name),
			m_longName(other.m_longName),
			m_option(other.m_option),
			m_channel(other.m_channel)
		{
		}
		virtual ~ModuleProperties()
		{
		}

		ModuleProperties& operator=(const ModuleProperties& other)
		{
			if (this != &other)
			{
				m_name = other.m_name;
				m_longName = other.m_longName;
				m_option = other.m_option;
				m_channel = other.m_channel;
			}

			return *this;
		}

		bool operator==(const ModuleProperties &other) const
		{
			if ((m_name == other.m_name) &&
				(m_longName == other.m_longName))
			{
				return true;
			}

			return false;
		}
		bool operator<(const ModuleProperties &other) const
		{
			if (m_name < other.m_name)
			{
				return true;
			}
			else if (m_name == other.m_name)
			{
				if (m_longName < other.m_longName)
				{
					return true;
				}
			}

			return false;
		}

		// Description
		std::string m_name;
		std::string m_longName;
		std::string m_option;
		std::string m_channel;

};

#endif // _MODULE_PROPERTIES_H
