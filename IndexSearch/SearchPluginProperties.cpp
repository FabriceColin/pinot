/*
 *  Copyright 2005-2008 Fabrice Colin
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

#include "SearchPluginProperties.h"

using std::string;
using std::map;
using std::set;

SearchPluginProperties::SearchPluginProperties() :
	ModuleProperties(),
	m_method(GET_METHOD),
	m_scrolling(PER_PAGE),
	m_nextIncrement(0),
	m_nextBase(0),
	m_response(UNKNOWN_RESPONSE)
{
}

SearchPluginProperties::SearchPluginProperties(const SearchPluginProperties &other) :
	ModuleProperties(other),
	m_languages(other.m_languages),
	m_baseUrl(other.m_baseUrl),
	m_method(other.m_method),
	m_variableParameters(other.m_variableParameters),
	m_editableParameters(other.m_editableParameters),
	m_remainder(other.m_remainder),
	m_outputType(other.m_outputType),
	m_scrolling(other.m_scrolling),
	m_nextIncrement(other.m_nextIncrement),
	m_nextBase(other.m_nextBase),
	m_response(other.m_response)
{
}

SearchPluginProperties::~SearchPluginProperties()
{
}

SearchPluginProperties& SearchPluginProperties::operator=(const SearchPluginProperties& other)
{
	ModuleProperties::operator=(other);

	if (this != &other)
	{
		m_languages = other.m_languages;
		m_baseUrl = other.m_baseUrl;
		m_method = other.m_method;
		m_variableParameters = other.m_variableParameters;
		m_editableParameters = other.m_editableParameters;
		m_remainder = other.m_remainder;
		m_outputType = other.m_outputType;
		m_scrolling = other.m_scrolling;
		m_nextIncrement = other.m_nextIncrement;
		m_nextBase = other.m_nextBase;
		m_response = other.m_response;
	}

	return *this;
}

bool SearchPluginProperties::operator==(const SearchPluginProperties &other) const
{
	if (ModuleProperties::operator==(other) == true)
	{
		return true;
	}

	return false;
}

bool SearchPluginProperties::operator<(const SearchPluginProperties &other) const
{
	if (ModuleProperties::operator<(other) == true)
	{
		return true;
	}

	return false;
}

