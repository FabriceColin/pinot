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

#ifndef _SHERLOCK_PARSER_H
#define _SHERLOCK_PARSER_H

#include <pthread.h>
#include <string>
#include <vector>

#include "Document.h"
#include "PluginParsers.h"

/// Parses output of Sherlock-based search engines.
class SherlockResponseParser : public ResponseParserInterface
{
	public:
		SherlockResponseParser();
		virtual ~SherlockResponseParser();

		/// Parses the response; false if not all could be parsed.
		virtual bool parse(const Document *pResponseDoc, std::vector<DocumentInfo> &resultsList,
			unsigned int &totalResults, unsigned int &firstResultIndex,
			std::string &charset) const;

		std::string m_resultListStart;
		std::string m_resultListEnd;
		std::string m_resultItemStart;
		std::string m_resultItemEnd;
		std::string m_resultTitleStart;
		std::string m_resultTitleEnd;
		std::string m_resultLinkStart;
		std::string m_resultLinkEnd;
		std::string m_resultExtractStart;
		std::string m_resultExtractEnd;
		bool m_skipLocal;

	private:
		SherlockResponseParser(const SherlockResponseParser &other);
		SherlockResponseParser& operator=(const SherlockResponseParser& other);

};

/** A parser for Sherlock plugin files.
  * See http://developer.apple.com/technotes/tn/tn1141.html
  * and http://mycroft.mozdev.org/deepdocs/deepdocs.html
  */
class SherlockParser : public PluginParserInterface
{
	public:
		SherlockParser(const std::string &fileName);
		virtual ~SherlockParser();

		/// Parses the plugin and returns a response parser.
		virtual ResponseParserInterface *parse(SearchPluginProperties &properties,
			bool minimal = false);

	protected:
		static pthread_mutex_t m_mutex;

	private:
		SherlockParser(const SherlockParser &other);
		SherlockParser& operator=(const SherlockParser& other);

};

#endif // _SHERLOCK_PARSER_H
