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

#ifndef _OPENSEARCH_PARSER_H
#define _OPENSEARCH_PARSER_H

#include <string>

#include "Document.h"
#include "PluginParsers.h"

/// Parses OpenSearch Response.
class OpenSearchResponseParser : public ResponseParserInterface
{
	public:
		OpenSearchResponseParser(bool rssResponse);
		virtual ~OpenSearchResponseParser();

		/// Parses the response; false if not all could be parsed.
		virtual bool parse(const Document *pResponseDoc, std::vector<DocumentInfo> &resultsList,
			unsigned int &totalResults, unsigned int &firstResultIndex,
			std::string &charset) const;

	protected:
		bool m_rssResponse;

	private:
		OpenSearchResponseParser(const OpenSearchResponseParser &other);
		OpenSearchResponseParser& operator=(const OpenSearchResponseParser& other);

};

/** A parser for OpenSearch Description and Query Syntax, version 1.1.
  * See http://opensearch.a9.com/spec/1.1/description/
  * and http://opensearch.a9.com/spec/1.1/querysyntax/
  * It can also parse MozSearch plugins.
  * See http://developer.mozilla.org/en/docs/Creating_MozSearch_plugins
  */
class OpenSearchParser : public PluginParserInterface
{
	public:
		OpenSearchParser(const std::string &fileName);
		virtual ~OpenSearchParser();

		/// Parses the plugin and returns a response parser.
		virtual ResponseParserInterface *parse(SearchPluginProperties &properties,
			bool minimal = false);

	private:
		OpenSearchParser(const OpenSearchParser &other);
		OpenSearchParser& operator=(const OpenSearchParser& other);

};

#endif // _OPENSEARCH_PARSER_H
