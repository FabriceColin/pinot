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

#include <iostream>

#include "Url.h"
#include "MIMEScanner.h"

#include "GoogleAPIEngine.h"
#include "GAPIGoogleSearchBindingProxy.h"
#include "GAPI.nsmap"

using std::clog;
using std::clog;
using std::endl;

struct Namespace *namespaces;

GoogleAPIEngine::GoogleAPIEngine(const string &key) :
	WebEngine(),
	m_key(key)
{
}

GoogleAPIEngine::~GoogleAPIEngine()
{
}

/// Retrieves the specified URL from the cache; NULL if error. Caller deletes.
Document *GoogleAPIEngine::retrieveCachedUrl(const string &url)
{
	GoogleSearchBinding soapProxy;
	xsd__base64Binary base64Page;

	if ((m_key.empty() == true) ||
		(soapProxy.gapi1__doGetCachedPage(m_key, url, base64Page)))
	{
		return NULL;
	}

	if ((base64Page.__ptr != NULL) &&
		(base64Page. __size > 0))
	{
		Url urlObj(url);

		Document *pDoc = new Document(url, url, MIMEScanner::scanUrl(urlObj), "");
		pDoc->setData((const char*)base64Page.__ptr, (unsigned int)base64Page. __size);

		return pDoc;
	}
	
	return NULL;
}

/// Checks spelling.
string GoogleAPIEngine::checkSpelling(const string &text)
{
	GoogleSearchBinding soapProxy;
	string spellOut;

	if ((m_key.empty() == true) ||
		(soapProxy.gapi1__doSpellingSuggestion(m_key, text, spellOut)))
	{
		return "";
	}

	return spellOut;
}

//
// Implementation of SearchEngineInterface
//

/// Runs a query; true if success.
bool GoogleAPIEngine::runQuery(QueryProperties& queryProps,
	unsigned int startDoc)
{
	string queryString(queryProps.getFreeQuery(true));
	unsigned int maxResultsCount = queryProps.getMaximumResultsCount();

	m_resultsList.clear();
	m_resultsCountEstimate = 0;

	if (queryProps.getType() != QueryProperties::XAPIAN_QP)
	{
		clog << "GoogleAPIEngine::runQuery: query type not supported" << endl;
		return false;
	}

	setQuery(queryProps);

	if (m_key.empty() == true)
	{
		clog << "GoogleAPIEngine::runQuery: no key" << endl;
		return false;
	}

	if (queryString.empty() == true)
	{
#ifdef DEBUG
		clog << "GoogleAPIEngine::runQuery: query is empty" << endl;
#endif
		return false;
	}
#ifdef DEBUG
	clog << "GoogleAPIEngine::runQuery: query is " << queryString << endl;
#endif

	GoogleSearchBinding soapProxy;
	struct gapi1__doGoogleSearchResponse queryOut;

	// No filter, no safe search
	int soapStatus = soapProxy.gapi1__doGoogleSearch(m_key, queryString, 0, (int)(maxResultsCount > 10 ? 10 : maxResultsCount),
		false, "", false, "", "utf-8", "utf-8", queryOut);
	if (soapStatus != SOAP_OK)
	{
		clog << "GoogleAPIEngine::runQuery: search failed with status " << soapStatus << endl;
		return false;
	}

	struct gapi1__GoogleSearchResult *searchResult = queryOut.return_;

	if ((searchResult != NULL) &&
		(searchResult->resultElements != NULL))
	{
		float pseudoScore = 100;

		m_resultsCountEstimate = searchResult->estimatedTotalResultsCount;

		for (int i = 0; i < searchResult->resultElements->__size; ++i)
		{
			struct gapi1__ResultElement *resultElement = searchResult->resultElements->__ptr[i];

			DocumentInfo result(resultElement->title, resultElement->URL, "", "");
			result.setExtract(resultElement->snippet);
			result.setScore(pseudoScore);

			if (processResult("http://www.google.com/", result) == true)
			{
				m_resultsList.push_back(result);
				--pseudoScore;
			}
		}
	}

	return true;
}
