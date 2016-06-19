/*
 *  Copyright 2005-2013 Fabrice Colin
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

#include <algorithm>
#include <fstream>
#include <iostream>
#include <cstring>

#include "config.h"
#include "Document.h"
#include "StringManip.h"
#include "Url.h"
#include "OpenSearchParser.h"
#ifdef HAVE_BOOST_SPIRIT
#include "SherlockParser.h"
#endif
#include "PluginWebEngine.h"

using std::clog;
using std::clog;
using std::endl;

PluginWebEngine::PluginWebEngine(const string &fileName) :
	WebEngine(),
	m_pResponseParser(NULL)
{
	load(fileName);
}

PluginWebEngine::~PluginWebEngine()
{
	if (m_pResponseParser != NULL)
	{
		delete m_pResponseParser;
	}
}

void PluginWebEngine::load(const string &fileName)
{
	if (fileName.empty() == true)
	{
		return;
	}

	string pluginType;
	PluginParserInterface *pParser = getPluginParser(fileName, pluginType);
	if (pParser == NULL)
	{
		return;
	}

	m_pResponseParser = pParser->parse(m_properties);
	delete pParser;
}

bool PluginWebEngine::getPage(const string &formattedQuery, unsigned int maxResultsCount)
{
	if ((m_pResponseParser == NULL) ||
		(formattedQuery.empty() == true))
	{
		return false;
	}

	DocumentInfo docInfo("Results Page", formattedQuery,
		"text/html", "");
	Document *pResponseDoc = downloadPage(docInfo);
	if (pResponseDoc == NULL)
	{
		clog << "PluginWebEngine::getPage: couldn't download "
			<< formattedQuery << endl;
		return false;
	}

	off_t contentLen;
	const char *pContent = pResponseDoc->getData(contentLen);
	if ((pContent == NULL) ||
		(contentLen == 0))
	{
#ifdef DEBUG
		clog << "PluginWebEngine::getPage: downloaded empty page" << endl;
#endif
		delete pResponseDoc;
		return false;
	}
#ifdef DEBUG
	Url urlObj(formattedQuery);
	string fileName(urlObj.getHost() + "_PluginWebEngine.html");

	ofstream pageBackup(fileName.c_str());
	pageBackup.write(pContent, contentLen);
	pageBackup.close();
#endif

	string responseCharset;
	bool success = m_pResponseParser->parse(pResponseDoc, m_resultsList,
		maxResultsCount, m_properties.m_nextBase, responseCharset);
	if (m_charset.empty() == true)
	{
		m_charset = responseCharset;
#ifdef DEBUG
		clog << "PluginWebEngine::getPage: page charset is " << m_charset << endl;
#endif
	}

	vector<DocumentInfo>::iterator resultIter = m_resultsList.begin();
	while (resultIter != m_resultsList.end())
	{
		if (processResult(formattedQuery, *resultIter) == false)
		{
			// Remove this result
			if (resultIter == m_resultsList.begin())
			{
				m_resultsList.erase(resultIter);
				resultIter = m_resultsList.begin();
			}
			else
			{
				vector<DocumentInfo>::iterator badResultIter = resultIter;
				--resultIter;
				m_resultsList.erase(badResultIter);
			}
		}
		else
		{
			// Next
			++resultIter;
		}
	}

	delete pResponseDoc;

	return success;
}

PluginParserInterface *PluginWebEngine::getPluginParser(const string &fileName,
	string &pluginType)
{
	if (fileName.empty() == true)
	{
		return NULL;
	}

	// What type of plugin is it ?
	// Look at the file extension
	string::size_type pos = fileName.find_last_of(".");
	if (pos == string::npos)
	{
		// No way to tell
		return NULL;
	}

	string extension(fileName.substr(pos + 1));
#ifdef HAVE_BOOST_SPIRIT
	if (strncasecmp(extension.c_str(), "src", 3) == 0)
	{
		pluginType = "sherlock";
		return new SherlockParser(fileName);
	}
	else
#endif
	if (strncasecmp(extension.c_str(), "xml", 3) == 0)
	{
		pluginType = "opensearch";
		return new OpenSearchParser(fileName);
	}

	return NULL;
}

bool PluginWebEngine::getDetails(const string &fileName, SearchPluginProperties &properties)
{
	if (fileName.empty() == true)
	{
		return false;
	}

	properties.m_option = fileName;
	PluginParserInterface *pParser = getPluginParser(fileName, properties.m_name);
	if (pParser == NULL)
	{
		return false;
	}

	ResponseParserInterface *pResponseParser = pParser->parse(properties, true);
	if (pResponseParser == NULL)
	{
		clog << "PluginWebEngine::getDetails: couldn't parse "
			<< fileName << endl;
		delete pParser;

		return false;
	}
	delete pResponseParser;
	delete pParser;

	if (properties.m_response == SearchPluginProperties::UNKNOWN_RESPONSE)
	{
#ifdef DEBUG
		clog << "PluginWebEngine::getDetails: bad response type for "
			<< fileName << endl;
#endif
		return false;
	}

	return true;
}

//
// Implementation of SearchEngineInterface
//

/// Runs a query; true if success.
bool PluginWebEngine::runQuery(QueryProperties& queryProps,
	unsigned int startDoc)
{
	string queryString(queryProps.getFreeQuery(true));
	char countStr[64];
	unsigned int maxResultsCount(queryProps.getMaximumResultsCount());
	unsigned int currentIncrement = 0, count = 0;
	bool firstPage = true;

	m_resultsList.clear();
	m_resultsCountEstimate = 0;

	if (queryProps.getType() != QueryProperties::XAPIAN_QP)
	{
		clog << "PluginWebEngine::runQuery: query type not supported" << endl;
		return false;
	}

	if (queryString.empty() == true)
	{
#ifdef DEBUG
		clog << "PluginWebEngine::runQuery: query is empty" << endl;
#endif
		return false;
	}

	string formattedQuery = m_properties.m_baseUrl;

	map<SearchPluginProperties::ParameterVariable, string>::iterator paramIter = m_properties.m_variableParameters.find(SearchPluginProperties::SEARCH_TERMS_PARAM);
	if (paramIter != m_properties.m_variableParameters.end())
	{
		formattedQuery += "?";
		formattedQuery += paramIter->second;
		formattedQuery += "=";
	}
#ifdef DEBUG
	else clog << "PluginWebEngine::runQuery: no user input tag" << endl;
#endif

	formattedQuery += queryString;
	if (m_properties.m_remainder.empty() == false)
	{
		formattedQuery += "&";
		formattedQuery += m_properties.m_remainder;
	}

	// Encodings ?
	paramIter = m_properties.m_variableParameters.find(SearchPluginProperties::OUTPUT_ENCODING_PARAM);
	if ((paramIter != m_properties.m_variableParameters.end()) &&
		(paramIter->second.empty() == false))
	{
		// Output encoding
		formattedQuery += "&";
		formattedQuery += paramIter->second;
		formattedQuery += "=UTF-8";
	}
	paramIter = m_properties.m_variableParameters.find(SearchPluginProperties::INPUT_ENCODING_PARAM);
	if ((paramIter != m_properties.m_variableParameters.end()) &&
		(paramIter->second.empty() == false))
	{
		// Input encoding
		formattedQuery += "&";
		formattedQuery += paramIter->second;
		formattedQuery += "=UTF-8";
	}

	// Editable parameters ?
	for (map<string, string>::const_iterator editableIter = m_properties.m_editableParameters.begin();
		editableIter != m_properties.m_editableParameters.end(); ++editableIter)
	{
		map<string, string>::const_iterator valueIter = m_editableValues.find(editableIter->second);
		if (valueIter == m_editableValues.end())
		{
			clog << "PluginWebEngine::runQuery: no value provided for plugin's editable parameter " << editableIter->second << endl;
			continue;
		}

		formattedQuery += "&";
		formattedQuery += editableIter->first;
		formattedQuery += "=";
		formattedQuery += valueIter->second;
	}

	setQuery(queryProps);

#ifdef DEBUG
	clog << "PluginWebEngine::runQuery: querying " << m_properties.m_longName << endl;
#endif
	while (count < maxResultsCount)
	{
		string pageQuery(formattedQuery);
		bool canScroll = false;

		// How do we scroll ?
		if (m_properties.m_scrolling == SearchPluginProperties::PER_INDEX)
		{
			paramIter = m_properties.m_variableParameters.find(SearchPluginProperties::COUNT_PARAM);
			if ((paramIter != m_properties.m_variableParameters.end()) &&
				(paramIter->second.empty() == false))
			{
				// Number of results requested
				pageQuery += "&";
				pageQuery += paramIter->second;
				pageQuery += "=";
				snprintf(countStr, 64, "%u", maxResultsCount);
				pageQuery += countStr;
				canScroll = true;
			}

			paramIter = m_properties.m_variableParameters.find(SearchPluginProperties::START_INDEX_PARAM);
			if ((paramIter != m_properties.m_variableParameters.end()) &&
				(paramIter->second.empty() == false))
			{
				// The offset of the first result (typically 1 or 0)
				pageQuery += "&";
				pageQuery += paramIter->second;
				pageQuery += "=";
				snprintf(countStr, 64, "%u", count + m_properties.m_nextBase);
				pageQuery += countStr;
				canScroll = true;
			}
		}
		else
		{
			paramIter = m_properties.m_variableParameters.find(SearchPluginProperties::START_PAGE_PARAM);
			if ((paramIter != m_properties.m_variableParameters.end()) &&
				(paramIter->second.empty() == false))
			{
				// The offset of the page
				pageQuery += "&";
				pageQuery += paramIter->second;
				pageQuery += "=";
				snprintf(countStr, 64, "%u", currentIncrement + m_properties.m_nextBase);
				pageQuery += countStr;
				canScroll = true;
			}
		}

		if ((firstPage == false) &&
			(canScroll == false))
		{
#ifdef DEBUG
			clog << "PluginWebEngine::runQuery: can't scroll to the next page of results" << endl;
#endif
			break;
		}

		firstPage = false;
		if (getPage(pageQuery, queryProps.getMaximumResultsCount()) == false)
		{
			break;
		}

		if (m_properties.m_nextIncrement == 0)
		{
			// That one page should have all the results...
#ifdef DEBUG
			clog << "PluginWebEngine::runQuery: performed one off call" << endl;
#endif
			break;
		}
		else
		{
			if (m_resultsList.size() < count + m_properties.m_nextIncrement)
			{
				// We got less than the maximum number of results per page
				// so there's no point in requesting the next page
#ifdef DEBUG
				clog << "PluginWebEngine::runQuery: last page wasn't full" << endl;
#endif
				break;
			}

			// Increase factor
			currentIncrement += m_properties.m_nextIncrement;
		}
		count = m_resultsList.size();
	}
	m_resultsCountEstimate = m_resultsList.size();

	return true;
}
