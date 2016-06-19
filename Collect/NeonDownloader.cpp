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

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdarg.h>
#include <pthread.h>
#include <iostream>

#include <neon/ne_socket.h>
#include <neon/ne_session.h>
#include <neon/ne_request.h>

#include "Url.h"
#include "HtmlFilter.h"
#include "FilterUtils.h"
#include "NeonDownloader.h"

using namespace std;

unsigned int NeonDownloader::m_initialized = 0;

NeonDownloader::NeonDownloader() :
	DownloaderInterface()
{
	if (m_initialized == 0)
	{
		// Initialize
		ne_sock_init();

		++m_initialized;
	}
}

NeonDownloader::~NeonDownloader()
{
	--m_initialized;
	if (m_initialized == 0)
	{
		// Shutdown
		ne_sock_exit();
	}
}

string NeonDownloader::handleRedirection(const char *pBody, unsigned int length)
{
	if ((pBody == NULL) ||
		(length == 0))
	{
		return "";
	}

	Document doc;
	Dijon::HtmlFilter htmlFilter("text/html");
	set<Dijon::Link> linksSet;

	doc.setData(pBody, length);

	// Extract the link from the 3xx message
	if ((FilterUtils::feedFilter(doc, &htmlFilter) == true) &&
		(htmlFilter.get_links(linksSet) == true))
	{
		// There should be one and only one
		if (linksSet.size() > 1)
		{
#ifdef DEBUG
			clog << "NeonDownloader::handleRedirection: " << linksSet.size() << " links found in " << length << " bytes" << endl;
			clog << "NeonDownloader::handleRedirection: redirection message was " << pBody << endl;
#endif
			return "";
		}

		set<Dijon::Link>::const_iterator iter = linksSet.begin();
		if (iter != linksSet.end())
		{
			// Update the URL
			return iter->m_url;
		}
	}

	return "";
}

//
// Implementation of DownloaderInterface
//

/// Retrieves the specified document; NULL if error.
Document *NeonDownloader::retrieveUrl(const DocumentInfo &docInfo)
{
	map<string, string> headers;

	return retrieveUrl(docInfo, headers);
}

/// Retrieves the specified document; NULL if error.
Document *NeonDownloader::retrieveUrl(const DocumentInfo &docInfo,
	const map<string, string> &headers)
{
	Document *pDocument = NULL;
	string url = Url::escapeUrl(docInfo.getLocation());
	char *pContent = NULL;
	size_t contentLen = 0;
	int statusCode = 200;
	unsigned int redirectionsCount = 0;

	if (url.empty() == true)
	{
#ifdef DEBUG
		clog << "NeonDownloader::retrieveUrl: no URL specified !" << endl;
#endif
		return NULL;
	}
	Url urlObj(url);
	string protocol(urlObj.getProtocol());
	string hostName(urlObj.getHost());
	string location(urlObj.getLocation());
	string file(urlObj.getFile());
	string parameters(urlObj.getParameters());
	string lastModifiedHeaderValue, locationHeaderValue, contentTypeHeaderValue;

	// Create a session
	ne_session *pSession = ne_session_create(protocol.c_str(), hostName.c_str(), 80); // urlObj.getPort());
	if (pSession == NULL)
	{
#ifdef DEBUG
		clog << "NeonDownloader::retrieveUrl: couldn't create session !" << endl;
#endif
		return NULL;
	}
	// Set the user agent
	ne_set_useragent(pSession, m_userAgent.c_str());
	// ...and the timeout
	ne_set_read_timeout(pSession, (int)m_timeout);
	// Is a proxy defined ?
	if ((m_proxyAddress.empty() == false) &&
		(m_proxyPort > 0))
	{
		// Type is HTTP
		ne_session_proxy(pSession, m_proxyAddress.c_str(), m_proxyPort);
	}

	string fullLocation = "/";
	if (location.empty() == false)
	{
		fullLocation += location;
	}
	if (file.empty() == false)
	{
		if (location.empty() == false)
		{
			fullLocation += "/";
		}
		fullLocation += file;
	}
	if (parameters.empty() == false)
	{
		fullLocation += "?";
		fullLocation += parameters;
	}

	// Create a request for this URL
	ne_request *pRequest = NULL;
	if (m_method == "POST")
	{
		pRequest = ne_request_create(pSession, "POST", fullLocation.c_str());
		if ((pRequest != NULL) &&
			(m_postFields.empty() == false))
		{
			ne_set_request_body_buffer(pRequest, m_postFields.c_str(), m_postFields.length());
		}
	}
	else
	{
		pRequest = ne_request_create(pSession, "GET", fullLocation.c_str());
	}
	if (pRequest == NULL)
	{
#ifdef DEBUG
		clog << "NeonDownloader::retrieveUrl: couldn't create request !" << endl;
#endif
		ne_session_destroy(pSession);
		return NULL;
	}
#ifdef DEBUG
	clog << "NeonDownloader::retrieveUrl: request for " << fullLocation << " on " << hostName << endl;
#endif

	int requestStatus = NE_RETRY;
	while (requestStatus == NE_RETRY)
	{
		lastModifiedHeaderValue.clear();
		locationHeaderValue.clear();
		contentTypeHeaderValue.clear();

		// Begin the request
		requestStatus = ne_begin_request(pRequest);
#ifdef DEBUG
		clog << "NeonDownloader::retrieveUrl: request begun with status " << requestStatus << endl;
#endif
		if (requestStatus == NE_OK)
		{
			ssize_t bytesRead = 0;
			char buffer[1024];

			// Get the status
			const ne_status *pStatus = ne_get_status(pRequest);
			if (pStatus != NULL)
			{
				statusCode = pStatus->code;
#ifdef DEBUG
				clog << "NeonDownloader::retrieveUrl: status is " << statusCode << endl;
#endif
			}
			else
			{
				// Assume all is well
				statusCode = 200;
			}

			// Read the content
			while ((bytesRead = ne_read_response_block(pRequest, buffer, 1024)) > 0)
			{
				pContent = (char*)realloc(pContent, contentLen + bytesRead);
				memcpy((void*)(pContent + contentLen), (const void*)buffer, bytesRead);
				contentLen += bytesRead;
			}

			// Get headers
			const char *pValue = ne_get_response_header(pRequest, "Last-Modified");
			if (pValue != NULL)
			{
				lastModifiedHeaderValue = pValue;
			}
			pValue = ne_get_response_header(pRequest, "Location");
			if (pValue != NULL)
			{
				locationHeaderValue = pValue;
			}
			pValue = ne_get_response_header(pRequest, "Content-Type");
			if (pValue != NULL)
			{
				contentTypeHeaderValue = pValue;
			}

			// Redirection ?
			if ((statusCode >= 300) &&
				(statusCode < 400) &&
				(redirectionsCount < 10))
			{
				ne_end_request(pRequest);
				ne_request_destroy(pRequest);
				pRequest = NULL;

				string documentUrl = handleRedirection(pContent, contentLen);
				if (documentUrl.empty() == true)
				{
					// Did we find a Location header ?
					if (locationHeaderValue.empty() == true)
					{
						// Fail
						free(pContent);
						pContent = NULL;
						contentLen = 0;
						break;
					}
					documentUrl = locationHeaderValue;
				}

#ifdef DEBUG
				clog << "NeonDownloader::retrieveUrl: redirected to " << documentUrl << endl;
#endif
				urlObj = Url(documentUrl);
				location = urlObj.getLocation();
				file = urlObj.getFile();

				// Is this on the same host ?
				if (hostName != urlObj.getHost())
				{
					// No, it isn't
					hostName = urlObj.getHost();

					// Create a new session
					ne_session_destroy(pSession);
					pSession = ne_session_create(protocol.c_str(), hostName.c_str(), 80); // urlObj.getPort());
					if (pSession == NULL)
					{
#ifdef DEBUG
						clog << "NeonDownloader::retrieveUrl: couldn't create session !" << endl;
#endif
						return NULL;
					}
					ne_set_useragent(pSession, m_userAgent.c_str());
					ne_set_read_timeout(pSession, (int)m_timeout);
				}

				// Try again
				fullLocation = "/";
				if (location.empty() == false)
				{
					fullLocation += location;
					fullLocation += "/";
				}
				if (file.empty() == false)
				{
					fullLocation += file;
				}
#ifdef DEBUG
				clog << "NeonDownloader::retrieveUrl: redirected to " << fullLocation << " on " << hostName << endl;
#endif

				// Create a new request for this URL
				pRequest = ne_request_create(pSession, "GET", fullLocation.c_str());
				if (pRequest == NULL)
				{
#ifdef DEBUG
					clog << "NeonDownloader::retrieveUrl: couldn't create request !" << endl;
#endif
					ne_session_destroy(pSession);
					return NULL;
				}
				redirectionsCount++;
				requestStatus = NE_RETRY;

				// Discard whatever content we have already got
				free(pContent);
				pContent = NULL;
				contentLen = 0;
				continue;
			}
		}

		// End the request
		requestStatus = ne_end_request(pRequest);
	}

	if ((pContent != NULL) &&
		(contentLen > 0))
	{
		if (statusCode < 400)
		{
			// Copy the document content
			pDocument = new Document(docInfo);
			pDocument->setData(pContent, contentLen);
			pDocument->setLocation(url);
			pDocument->setType(contentTypeHeaderValue);
			if (lastModifiedHeaderValue.empty() == false)
			{
				pDocument->setTimestamp(lastModifiedHeaderValue);
			}
#ifdef DEBUG
			clog << "NeonDownloader::retrieveUrl: document size is " << contentLen << endl;
#endif
		}
		free(pContent);
	}

	// Cleanup
	ne_request_destroy(pRequest);
	ne_session_destroy(pSession);

	return pDocument;
}

Document *NeonDownloader::putUrl(const DocumentInfo &docInfo,
	const map<string, string> &headers,
	const string &url)
{
	// FIXME: implement this
	return NULL;
}

