/*
 *  Copyright 2007-2016 Fabrice Colin
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

#include "config.h"
#ifdef HAVE_VSNPRINTF
#include <stdio.h>
#include <stdarg.h>
#endif
#include <string.h>
#include <iostream>
#include <algorithm>
#include <utility>

#include "HtmlFilter.h"

using std::clog;
using std::clog;
using std::endl;
using std::string;
using std::for_each;
using std::map;
using std::set;
using std::copy;
using std::inserter;

using namespace std;
using namespace Dijon;

static const unsigned int HASH_LEN = ((4 * 8 + 5) / 6);

#ifdef _DYNAMIC_DIJON_HTMLFILTER
DIJON_FILTER_EXPORT bool get_filter_types(MIMETypes &mime_types)
{
	mime_types.m_mimeTypes.clear();
	mime_types.m_mimeTypes.insert("text/html");

	return true;
}

DIJON_FILTER_EXPORT bool check_filter_data_input(int data_input)
{
	Filter::DataInput input = (Filter::DataInput)data_input;

	if ((input == Filter::DOCUMENT_DATA) ||
		(input == Filter::DOCUMENT_STRING))
	{
		return true;
	}

	return false;
}

DIJON_FILTER_EXPORT Filter *get_filter(void)
{
	return new HtmlFilter();
}
#endif

// A function object to lower case strings with for_each()
struct ToLower
{
	public:
		void operator()(char &c)
		{
			c = (char)tolower((int)c);
		}
};

static unsigned int removeCharacters(string &str, const string &characters)
{
	unsigned int count = 0;

	string::size_type charPos = str.find_first_of(characters.c_str());
	while (charPos != string::npos)
	{
		str.erase(charPos, 1);
		++count;

		charPos = str.find_first_of(characters.c_str(), charPos);
	}

	return count;	
}

static unsigned int trimSpaces(string &str)
{
	string::size_type pos = 0;
	unsigned int count = 0;

	while ((str.empty() == false) && (pos < str.length()))
	{
		if (isspace(str[pos]) == 0)
		{
			++pos;
			break;
		}

		str.erase(pos, 1);
		++count;
	}

	for (pos = str.length() - 1;
		(str.empty() == false) && (pos >= 0); --pos)
	{
		if (isspace(str[pos]) == 0)
		{
			break;
		}

		str.erase(pos, 1);
		++count;
	}

	return count;
}

static string toLowerCase(const string &str)
{
        string tmp(str);

        for_each(tmp.begin(), tmp.end(), ToLower());

        return tmp;
}

static string findCharset(const string &content)
{
	// Is a charset specified ?
	string::size_type startPos = content.find("charset=\"");
	if ((startPos != string::npos) &&
		(content.length() > 9))
	{
		string::size_type endPos = content.find('"', startPos + 9);

		if (endPos != string::npos)
		{
			return content.substr(startPos + 9, endPos - startPos - 9);
		}
	}
	else
	{
		startPos = content.find("charset=");
		if (startPos != string::npos)
		{
			return content.substr(startPos + 8);
		}
	}

	return "";
}

Link::Link() :
	m_index(0),
	m_startPos(0),
	m_endPos(0)
{
}

Link::Link(const Link &other) :
	m_url(other.m_url),
	m_name(other.m_name),
	m_index(other.m_index),
	m_startPos(other.m_startPos),
	m_endPos(other.m_endPos)
{
}

Link::~Link()
{
}

Link& Link::operator=(const Link& other)
{
	if (this != &other)
	{
		m_url = other.m_url;
		m_name = other.m_name;
		m_index = other.m_index;
		m_startPos = other.m_startPos;
		m_endPos = other.m_endPos;
	}

	return *this;
}

bool Link::operator==(const Link &other) const
{
	return m_url == other.m_url;
}

bool Link::operator<(const Link &other) const
{
	return m_index < other.m_index;
}

HtmlFilter::ParserState::ParserState(dstring &text) :
	m_isValid(true),
	m_findAbstract(true),
	m_textPos(0),
	m_inHead(false),
	m_foundHead(false),
	m_appendToTitle(false),
	m_appendToText(false),
	m_appendToLink(false),
	m_skip(0),
	m_text(text)
{
}

HtmlFilter::ParserState::~ParserState()
{
}

bool HtmlFilter::ParserState::get_links_text(unsigned int currentLinkIndex)
{
	if ((m_links.empty() == true) ||
		(m_currentLink.m_index == 0))
	{
		string abstract(m_text.c_str());

		trimSpaces(abstract);

		m_abstract = abstract;

		return true;
	}

	// Get the text between the current link and the previous one
	for (set<Link>::const_iterator linkIter = m_links.begin();
		linkIter != m_links.end(); ++linkIter)
	{
		// Is this the previous link ?
		if (linkIter->m_index == currentLinkIndex - 1)
		{
			// Is there text in between ?
			if (linkIter->m_endPos + 1 < m_textPos)
			{
				unsigned int abstractLen = m_textPos - linkIter->m_endPos - 1;
				string abstract(m_text.substr(linkIter->m_endPos, abstractLen).c_str());

				trimSpaces(abstract);

				// The longer, the better
				if (abstract.length() > m_abstract.length())
				{
					m_abstract = abstract;
#ifdef DEBUG
					clog << "HtmlFilter::get_links_text: abstract after link "
						<< linkIter->m_index << " to " << linkIter->m_url << endl;
#endif

					return true;
				}
			}

			break;
		}
	}

	return false;
}

void HtmlFilter::ParserState::append_whitespace(void)
{
	// Append a single space
	if (m_appendToTitle == true)
	{
		m_title += " ";
	}
	else
	{
		if (m_appendToText == true)
		{
			m_text += " ";
			m_textPos += 1;
		}

		// Appending to text and to link are not mutually exclusive operations
		if (m_appendToLink == true)
		{
			m_currentLink.m_name += " ";
		}
	}
}

void HtmlFilter::ParserState::append_text(const string &text)
{
	// Append current text
	if (m_appendToTitle == true)
	{
		m_title += text;
	}
	else
	{
		if (m_appendToText == true)
		{
			m_text.append(text.c_str(), text.length());
			m_textPos += text.length();
		}

		// Appending to text and to link are not mutually exclusive operations
		if (m_appendToLink == true)
		{
			m_currentLink.m_name += text;
		}
	}
}

void HtmlFilter::ParserState::process_text(const string &text)
{
	if (text.empty() == true)
	{
		return;
	}

	if (m_skip > 0)
	{
		// Skip this
		return;
	}

        string::size_type nonSpace = text.find_first_not_of(" \t\n\r");
	bool appendSpace = false;

        if (nonSpace > 0)
	{
		appendSpace = true;
	}
        while (nonSpace != string::npos)
	{
		if (appendSpace == true)
		{
			append_whitespace();
		}

		string::size_type nonSpaceEnd = text.find_first_of(" \t\n\r", nonSpace);
		if (nonSpaceEnd != string::npos)
		{
			appendSpace = true;
			append_text(text.substr(nonSpace, nonSpaceEnd - nonSpace));

			nonSpace = text.find_first_not_of(" \t\n\r", nonSpaceEnd + 1);
		}
		else
		{
			append_text(text.substr(nonSpace, text.size() - nonSpace));

			nonSpace = string::npos;
		}
	}
}

void HtmlFilter::ParserState::opening_tag(const string &tag)
{
	if (tag.empty() == true)
	{
		return;
	}

	// What tag is this ?
	string tagName(toLowerCase(tag));
	if ((m_foundHead == false) &&
		(tagName == "head"))
	{
		// Expect to find META tags and a title
		m_inHead = true;
		// One head is enough :-)
		m_foundHead = true;
	}
	else if ((m_inHead == true) &&
		(tagName == "meta"))
	{
		string metaName, metaContent, httpEquiv;

		// Get the META tag's name and content
		get_parameter("name", metaName);
		get_parameter("content", metaContent);

		if ((metaName.empty() == false) &&
			(metaContent.empty() == false))
		{
			// Store this META tag
			metaName = toLowerCase(metaName);
			m_metaTags[metaName] = metaContent;
		}

		// Is a charset specified ?
		get_parameter("http-equiv", httpEquiv);
		if ((metaContent.empty() == false) &&
			(m_charset.empty() == true))
		{
			metaContent = toLowerCase(metaContent);
			m_charset = findCharset(metaContent);
		}

		// Look for a HTML5 charset definition
		if (m_charset.empty() == true)
		{
			get_parameter("charset", m_charset);
		}
	}
	else if ((m_inHead == true) &&
		(tagName == "title"))
	{
		// Extract title
		m_appendToTitle = true;
	}
	else if (tagName == "body")
	{
		// Index text
		m_appendToText = true;
	}
	else if (tagName == "a")
	{
		m_currentLink.m_url.clear();
		m_currentLink.m_name.clear();

		// Get the href
		get_parameter("href", m_currentLink.m_url);

		if (m_currentLink.m_url.empty() == false)
		{
			// FIXME: get the NodeInfo to find out the position of this link
			m_currentLink.m_startPos = m_textPos;

			// Find abstract ?
			if (m_findAbstract == true)
			{
				get_links_text(m_currentLink.m_index);
			}

			// Extract link
			m_appendToLink = true;
		}
	}
	else if (tagName == "frame")
	{
		Link frame;

		// Get the name and source
		get_parameter("name", frame.m_name);
		get_parameter("src", frame.m_url);

		if (frame.m_url.empty() == false)
		{
			// Store this frame
			m_frames.insert(frame);
		}
	}
	else if ((tagName == "frameset") ||
		(tagName == "script") ||
		(tagName == "style"))
	{
		// Skip
		++m_skip;
	}

	// Replace tags with spaces
	if (m_appendToTitle == true)
	{
		m_title += " ";
	}
	if (m_appendToText == true)
	{
		m_text += " ";
		m_textPos += 1;
	}
	if (m_appendToLink == true)
	{
		m_currentLink.m_name += " ";
	}
}

void HtmlFilter::ParserState::closing_tag(const string &tag)
{
	if (tag.empty() == true)
	{
		return;
	}

	// Reset state
	string tagName(toLowerCase(tag));
	if (tagName == "head")
	{
		m_inHead = false;
	}
	else if (tagName == "title")
	{
		trimSpaces(m_title);
		removeCharacters(m_title, "\r\n");
#ifdef DEBUG
		clog << "HtmlFilter::endHandler: title is " << m_title << endl;
#endif
		m_appendToTitle = false;
	}
	else if (tagName == "body")
	{
		m_appendToText = false;
	}
	else if (tagName == "a")
	{
		if (m_currentLink.m_url.empty() == false)
		{
			trimSpaces(m_currentLink.m_name);
			removeCharacters(m_currentLink.m_name, "\r\n");

			m_currentLink.m_endPos = m_textPos;

			// Store this link
			m_links.insert(m_currentLink);
			++m_currentLink.m_index;
		}

		m_appendToLink = false;
	}
	else if ((tagName == "frameset") ||
		(tagName == "script") ||
		(tagName == "style"))
	{
		--m_skip;
	}
}

HtmlFilter::HtmlFilter() :
	Filter(),
	m_pParserState(NULL),
	m_skipText(false),
	m_findAbstract(true)
{
}

HtmlFilter::~HtmlFilter()
{
	rewind();
}

bool HtmlFilter::is_data_input_ok(DataInput input) const
{
	if ((input == DOCUMENT_DATA) ||
		(input == DOCUMENT_STRING))
	{
		return true;
	}

	return false;
}

bool HtmlFilter::set_property(Properties prop_name, const string &prop_value)
{
	if (prop_name == OPERATING_MODE)
	{
		if (prop_value == "view")
		{
			// This will ensure text is skipped
			m_skipText = true;
			// ..and that we don't attempt finding an abstract
			m_findAbstract = false;
		}
		else
		{
			m_skipText = false;
			m_findAbstract = true;
		}

		return true;
	}

	return false;
}

bool HtmlFilter::set_document_data(const char *data_ptr, off_t data_length)
{
	if ((data_ptr == NULL) ||
		(data_length == 0))
	{
		return false;
	}

	string html_doc(data_ptr, data_length);
	return set_document_string(html_doc);
}

bool HtmlFilter::set_document_string(const string &data_str)
{
	if (data_str.empty() == true)
	{
		return false;
	}

	rewind();

	// Try to cope with pages that have scripts or other rubbish prepended
	string::size_type htmlPos = data_str.find("<!DOCTYPE");
	if (htmlPos == string::npos)
	{
		htmlPos = data_str.find("<!doctype");
	}
	if ((htmlPos != string::npos) &&
		(htmlPos > 0))
	{
#ifdef DEBUG
		clog << "HtmlFilter::set_document_string: removed " << htmlPos << " characters" << endl;
#endif
		return parse_html(data_str.substr(htmlPos));
	}

	return parse_html(data_str);
}

bool HtmlFilter::set_document_file(const string &file_path, bool unlink_when_done)
{
	return false;
}

bool HtmlFilter::set_document_uri(const string &uri)
{
	return false;
}

bool HtmlFilter::has_documents(void) const
{
	if (m_pParserState != NULL)
	{
		return true;
	}

	return false;
}

bool HtmlFilter::next_document(void)
{
	if (m_pParserState != NULL)
	{
		m_metaData["charset"] = m_pParserState->m_charset;
		m_metaData["title"] = m_pParserState->m_title;
		m_metaData["abstract"] = m_pParserState->m_abstract;
		m_metaData["ipath"] = "";
		m_metaData["mimetype"] = "text/plain";
		for (map<string, string>::const_iterator iter = m_pParserState->m_metaTags.begin();
			iter != m_pParserState->m_metaTags.end(); ++iter)
		{
			if (iter->first == "charset")
			{
				continue;
			}
			m_metaData[iter->first] = iter->second;
		}
		// FIXME: shove the links in there somehow !

		delete m_pParserState;
		m_pParserState = NULL;

		return true;
	}

	return false;
}

bool HtmlFilter::skip_to_document(const string &ipath)
{
	if (ipath.empty() == true)
	{
		return next_document();
	}

	return false;
}

string HtmlFilter::get_error(void) const
{
	return m_error;
}

void HtmlFilter::rewind(void)
{
	Filter::rewind();

	if (m_pParserState != NULL)
	{
		delete m_pParserState;
		m_pParserState = NULL;
	}
}

bool HtmlFilter::parse_html(const string &html)
{

	if (html.length() == true)
	{
		return false;
	}

	m_content.clear();
	m_pParserState = new ParserState(m_content);
	if (m_skipText == true)
	{
		++m_pParserState->m_skip;
	}

	// FIXME: parse here
	m_pParserState->parse_html(html);

	// The text after the last link might make a good abstract
	if (m_pParserState->m_findAbstract == true)
	{
		m_pParserState->get_links_text(m_pParserState->m_currentLink.m_index);
	}

	// Append META keywords, if any were found
	map<string, string>::iterator keywordsIter = m_pParserState->m_metaTags.find("keywords");
	if (keywordsIter != m_pParserState->m_metaTags.end())
	{
		m_pParserState->m_text.append(keywordsIter->second.c_str(), keywordsIter->second.length());
	}
#ifdef DEBUG
	clog << "HtmlFilter::parse_html: " << m_pParserState->m_text.size() << " bytes of text" << endl;
#endif

	// Assume charset is UTF-8 by default
	if (m_pParserState->m_charset.empty() == true)
	{
		m_pParserState->m_charset = "utf-8";
	}
	else
	{
		m_pParserState->m_charset = toLowerCase(m_pParserState->m_charset);
#ifdef DEBUG
		clog << "HtmlFilter::parse_html: found charset " << m_pParserState->m_charset << endl;
#endif
	}

	return true;
}

bool HtmlFilter::get_links(set<Link> &links) const
{
	links.clear();

	if (m_pParserState != NULL)
	{
		copy(m_pParserState->m_links.begin(), m_pParserState->m_links.end(),
			inserter(links, links.begin()));

		return true;
	}

	return false;
}

