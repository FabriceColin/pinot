/*
 *  Copyright 2005-2009 Fabrice Colin
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

#ifndef _PLUGIN_PARSERS_H
#define _PLUGIN_PARSERS_H

#include <string>
#include <vector>

#include "Document.h"
#include "SearchPluginProperties.h"

/// Interface implemented by response parsers.
class ResponseParserInterface
{
	public:
		virtual ~ResponseParserInterface()
		{
		}

		/// Parses the response; false if not all could be parsed.
		virtual bool parse(const Document *pResponseDoc, std::vector<DocumentInfo> &resultsList,
			unsigned int &totalResults, unsigned int &firstResultIndex,
			std::string &charset) const = 0;

	protected:
		ResponseParserInterface()
		{
		}
		
};
	
/// Interface implemented by plugin parsers.
class PluginParserInterface
{
	public:
		virtual ~PluginParserInterface()
		{
		}

		/// Parses the plugin and returns a response parser.
		virtual ResponseParserInterface *parse(SearchPluginProperties &properties,
			bool minimal = false) = 0;

	protected:
		std::string m_fileName;

		PluginParserInterface(const std::string &fileName) :
			m_fileName(fileName)
		{
		}

};

#endif // _PLUGIN_PARSERS_H
