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

#ifndef _SEARCH_PLUGIN_PROPERTIES_H
#define _SEARCH_PLUGIN_PROPERTIES_H

#include <string>
#include <map>
#include <set>

#include "ModuleProperties.h"

/// Properties of a search engine plugin.
class SearchPluginProperties : public ModuleProperties
{
	public:
		SearchPluginProperties();
		SearchPluginProperties(const SearchPluginProperties &other);
		virtual ~SearchPluginProperties();

		SearchPluginProperties& operator=(const SearchPluginProperties& other);
		bool operator==(const SearchPluginProperties &other) const;
		bool operator<(const SearchPluginProperties &other) const;

		typedef enum { GET_METHOD = 0, POST_METHOD } Method;

		typedef enum { UNKNOWN_PARAM = 0, SEARCH_TERMS_PARAM,
			 COUNT_PARAM,START_INDEX_PARAM, START_PAGE_PARAM, LANGUAGE_PARAM,
			OUTPUT_ENCODING_PARAM, INPUT_ENCODING_PARAM } ParameterVariable;

		typedef enum { PER_PAGE = 0, PER_INDEX } Scrolling;

		typedef enum { UNKNOWN_RESPONSE = 0, HTML_RESPONSE,
			RSS_RESPONSE, ATOM_RESPONSE } Response;

		// Description
		std::set<std::string> m_languages;
		// Query
		std::string m_baseUrl;
		Method m_method;
		std::map<ParameterVariable, std::string> m_variableParameters;
		std::map<std::string, std::string> m_editableParameters;
		std::string m_remainder;
		std::string m_outputType;
		// Scrolling
		Scrolling m_scrolling;
		unsigned int m_nextIncrement;
		unsigned int m_nextBase;
		// Response
		Response m_response;

};

#endif // _SEARCH_PLUGIN_PROPERTIES_H
