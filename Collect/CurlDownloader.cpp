/*
 *  Copyright 2005-2014 Fabrice Colin
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
#ifdef HAVE_REGEX_H
#include <regex.h>
#endif
#include <stdlib.h>
#include <iostream>
#include <sstream>

#include <curl/curl.h>

#include "StringManip.h"
#include "Url.h"
#include "CurlDownloader.h"

using namespace std;

struct ContentInfo
{
	char *m_pContent;
	size_t m_contentLen;
	string m_lastModified;
	map<string, string> m_headers;

};

static void freeContentInfo(struct ContentInfo *pInfo)
{
	if (pInfo == NULL)
	{
		return;
	}

	if (pInfo->m_pContent != NULL)
	{
		free(pInfo->m_pContent);
		pInfo->m_pContent = NULL;
		pInfo->m_contentLen = 0;
	}
}

static size_t writeCallback(void *pData, size_t dataSize, size_t elementsCount, void *pStream)
{
	ContentInfo *pInfo = NULL;
	size_t totalSize = elementsCount * dataSize;

	if (pStream == NULL)
	{
		return 0;
	}
	pInfo = (ContentInfo *)pStream;

	char *pNewContent = (char*)realloc(pInfo->m_pContent, pInfo->m_contentLen + totalSize + 1);
	if (pNewContent == NULL)
	{
#ifdef DEBUG
		clog << "writeCallback: failed to enlarge buffer" << endl;
#endif
		freeContentInfo(pInfo);
		return 0;
	}

	pInfo->m_pContent = pNewContent;
	memcpy(pInfo->m_pContent + pInfo->m_contentLen, pData, totalSize);
	pInfo->m_contentLen += totalSize;
	pInfo->m_pContent[pInfo->m_contentLen] = '\0';

	if (totalSize < strlen((const char*)pData))
	{
		void *pBadChar = NULL;

		// There's a NULL character in the buffer ? Replace it
		while ((pBadChar = memchr((void*)pInfo->m_pContent, '\0', pInfo->m_contentLen)) != NULL)
		{
			((char*)pBadChar)[0] = ' ';
#ifdef DEBUG
			clog << "writeCallback: bad character" << endl;
#endif
		}
	}

	return totalSize;
}

static size_t headerCallback(void *pData, size_t dataSize, size_t elementsCount, void *pStream)
{
	ContentInfo *pInfo = NULL;
	size_t totalSize = elementsCount * dataSize;

	if ((pStream == NULL) ||
		(pData == NULL) ||
		(totalSize == 0))
	{
		return 0;
	}
	pInfo = (ContentInfo *)pStream;

	string header((const char*)pData, totalSize);
	string::size_type pos = header.find("Last-Modified: ");

#ifdef DEBUG
	clog << "headerCallback: header " << header << endl;
#endif
	if (pos == 0)
	{
		pInfo->m_lastModified = header.substr(15);

		return totalSize;
	}

	pos = header.find(": ");
	if ((pos != string::npos) &&
		(header.length() > pos + 2))
	{
		string headerValue(StringManip::extractField(header.substr(pos + 2), "\"", "\""));

		if (headerValue.empty() == true)
		{
			headerValue = header.substr(pos + 2);
		}
		StringManip::trimSpaces(headerValue);

		pInfo->m_headers.insert(pair<string, string>(header.substr(0, pos),
			headerValue));
	}

	return totalSize;
}

unsigned int CurlDownloader::m_initialized = 0;

CurlDownloader::CurlDownloader() :
	DownloaderInterface()
{
	if (m_initialized == 0)
	{
		// Initialize
		curl_global_init(CURL_GLOBAL_ALL);

		++m_initialized;
	}
}

CurlDownloader::~CurlDownloader()
{
	--m_initialized;
	if (m_initialized == 0)
	{
		// Shutdown
		curl_global_cleanup();
	}
}

Document *CurlDownloader::populateDocument(const DocumentInfo &docInfo,
	const string &url, void *pHandler, void *pInfo)
{
	if ((pHandler == NULL) ||
		(pInfo == NULL))
	{
		return NULL;
	}

	Document *pDocument = new Document(docInfo);
	ContentInfo *pContentInfo = (ContentInfo *)pInfo;
	char *pContentType = NULL;
	long responseCode = 200;

	// Copy the document content
	pDocument->setData(pContentInfo->m_pContent, pContentInfo->m_contentLen);
	pDocument->setLocation(url);
	pDocument->setSize((off_t )pContentInfo->m_contentLen);

	// What's the Content-Type ?
	CURLcode res = curl_easy_getinfo((CURL *)pHandler, CURLINFO_CONTENT_TYPE, &pContentType);
	if ((res == CURLE_OK) &&
		(pContentType != NULL))
	{
		pDocument->setType(pContentType);
	}

	// What's the response code ?
	res = curl_easy_getinfo((CURL *)pHandler, CURLINFO_RESPONSE_CODE, &responseCode);
	if (res == CURLE_OK)
	{
		stringstream numStr;

		numStr << responseCode;
		pDocument->setOther("ResponseCode", numStr.str());
	}

	// The Last-Modified date ?
	if (pContentInfo->m_lastModified.empty() == false)
	{
		pDocument->setTimestamp(pContentInfo->m_lastModified);
	}
	for (map<string, string>::const_iterator headerIter = pContentInfo->m_headers.begin();
		headerIter != pContentInfo->m_headers.end(); ++headerIter)
	{
		pDocument->setOther(headerIter->first, headerIter->second);
	}

	return pDocument;
}

//
// Implementation of DownloaderInterface
//

/// Retrieves the specified document; NULL if error.
Document *CurlDownloader::retrieveUrl(const DocumentInfo &docInfo)
{
	map<string, string> headers;

	return retrieveUrl(docInfo, headers);
}

/// Retrieves the specified document; NULL if error.
Document *CurlDownloader::retrieveUrl(const DocumentInfo &docInfo,
	const map<string, string> &headers)
{
	Document *pDocument = NULL;
	string ipath(docInfo.getInternalPath());
	string url(docInfo.getLocation());
	char pBuffer[1024];
	unsigned int redirectionsCount = 0;

	if (url.empty() == true)
	{
#ifdef DEBUG
		clog << "CurlDownloader::retrieveUrl: no URL specified !" << endl;
#endif
		return NULL;
	}

	if (ipath.empty() == false)
	{
		url += "?";
		url += ipath;
	}

	// Create a session
	CURL *pCurlHandler = curl_easy_init();
	if (pCurlHandler == NULL)
	{
		return NULL;
	}

	struct curl_slist *pHeadersList = NULL;
	ContentInfo *pContentInfo = new ContentInfo;

	pContentInfo->m_pContent = NULL;
	pContentInfo->m_contentLen = 0;

	// Add headers
	for (map<string, string>::const_iterator headerIter = headers.begin();
			headerIter != headers.end(); ++headerIter)
	{
		snprintf(pBuffer, sizeof(pBuffer), "%s: %s",
			headerIter->first.c_str(),
			headerIter->second.c_str());

		pHeadersList = curl_slist_append(pHeadersList, pBuffer);
	}

	curl_easy_setopt(pCurlHandler, CURLOPT_AUTOREFERER, 1);
	curl_easy_setopt(pCurlHandler, CURLOPT_FOLLOWLOCATION, 1);
	curl_easy_setopt(pCurlHandler, CURLOPT_MAXREDIRS, 10);
	curl_easy_setopt(pCurlHandler, CURLOPT_USERAGENT, m_userAgent.c_str());
	curl_easy_setopt(pCurlHandler, CURLOPT_NOSIGNAL, (long)1);
	curl_easy_setopt(pCurlHandler, CURLOPT_TIMEOUT, (long)m_timeout);
#ifndef DEBUG
	curl_easy_setopt(pCurlHandler, CURLOPT_NOPROGRESS, 1);
#endif
	curl_easy_setopt(pCurlHandler, CURLOPT_HTTPHEADER, pHeadersList);
	curl_easy_setopt(pCurlHandler, CURLOPT_WRITEFUNCTION, writeCallback);
	curl_easy_setopt(pCurlHandler, CURLOPT_WRITEDATA, pContentInfo);
	curl_easy_setopt(pCurlHandler, CURLOPT_HEADERFUNCTION, headerCallback);
	curl_easy_setopt(pCurlHandler, CURLOPT_HEADERDATA, pContentInfo);

	// Is a proxy defined ?
	// Curl automatically checks and makes use of the *_proxy environment variables 
	if ((m_proxyAddress.empty() == false) &&
		(m_proxyPort > 0))
	{
		curl_proxytype proxyType = CURLPROXY_HTTP;

		curl_easy_setopt(pCurlHandler, CURLOPT_PROXY, m_proxyAddress.c_str());
		curl_easy_setopt(pCurlHandler, CURLOPT_PROXYPORT, m_proxyPort);
		// Type defaults to HTTP
		if (m_proxyType.empty() == false)
		{
			if (m_proxyType == "SOCKS4")
			{
				proxyType = CURLPROXY_SOCKS4;
			}
			else if (m_proxyType == "SOCKS5")
			{
				proxyType = CURLPROXY_SOCKS5;
			}
		}
		curl_easy_setopt(pCurlHandler, CURLOPT_PROXYTYPE, proxyType);
	}

#ifdef DEBUG
	clog << "CurlDownloader::retrieveUrl: URL is " << url << endl;
#endif
	while (redirectionsCount < 10)
	{
		curl_easy_setopt(pCurlHandler, CURLOPT_URL, Url::escapeUrl(url).c_str());

		if (m_method == "POST")
		{
			curl_easy_setopt(pCurlHandler, CURLOPT_POST, 1);
			if (m_postFields.empty() == false)
			{
				curl_easy_setopt(pCurlHandler, CURLOPT_POSTFIELDS, m_postFields.c_str());
			}
		}

		CURLcode res = curl_easy_perform(pCurlHandler);
		if ((res == CURLE_OK) &&
			(pContentInfo->m_pContent != NULL) &&
			(pContentInfo->m_contentLen > 0))
		{
			pDocument = populateDocument(docInfo, url,
				pCurlHandler, pContentInfo);

#ifdef HAVE_REGEX_H
			regex_t refreshRegex;
			regmatch_t pMatches[2];

			// Any REFRESH META tag ?
			// Look for <meta http-equiv="refresh" content="SECS;url=URL">
			if (regcomp(&refreshRegex,
				"<meta http-equiv=\"refresh\" content=\"([0-9]*);url=([^\"]*)\">"
				REG_EXTENDED|REG_ICASE) == 0)
			{
				if (regexec(&refreshRegex, pContentInfo->m_pContent, 2,
					pMatches, REG_NOTBOL|REG_NOTEOL) == 0)
				{
					url = pMatches[1];
#ifdef DEBUG
					clog << "CurlDownloader::retrieveUrl: redirected to URL " << url << endl;
#endif
					delete pDocument;
					pDocument = NULL;
					freeContentInfo(pContentInfo);
					++redirectionsCount;
					continue;
				}
#ifdef DEBUG
				else clog << "CurlDownloader::retrieveUrl: no REFRESH META tag" << endl;
#endif

				regfree(&refreshRegex);
			}
#ifdef DEBUG
			else clog << "CurlDownloader::retrieveUrl: couldn't look for a REFRESH META tag" << endl;
#endif
#endif
		}
		else
		{
			clog << "Couldn't download " << url << ": " << curl_easy_strerror(res) << endl;
		}

		break;
	}

	freeContentInfo(pContentInfo);
	delete pContentInfo;

	// Cleanup
	curl_easy_cleanup(pCurlHandler);

	return pDocument;
}

Document *CurlDownloader::putUrl(const DocumentInfo &docInfo,
	const map<string, string> &headers,
	const string &url)
{
	Document *pDocument = NULL;
	struct curl_slist *pHeadersList = NULL;
	string mimeType(docInfo.getType());
	string fileLocation(docInfo.getLocation());
	char pBuffer[1024];

	if (url.empty() == true)
	{
#ifdef DEBUG
		clog << "CurlDownloader::putUrl: no URL specified !" << endl;
#endif
		return NULL;
	}

	FILE *pFile = fopen(fileLocation.c_str(), "r");
	if (pFile == NULL)
	{
#ifdef DEBUG
		clog << "CurlDownloader::putUrl: couldn't open file " << fileLocation << endl;
#endif
		return NULL;
	}

	// Create a session
	CURL *pCurlHandler = curl_easy_init();
	if (pCurlHandler == NULL)
	{
		fclose(pFile);

		return NULL;
	}

	ContentInfo *pContentInfo = new ContentInfo;

	pContentInfo->m_pContent = NULL;
	pContentInfo->m_contentLen = 0;

	// Add headers
	for (map<string, string>::const_iterator headerIter = headers.begin();
		headerIter != headers.end(); ++headerIter)
	{
		snprintf(pBuffer, sizeof(pBuffer), "%s: %s",
			headerIter->first.c_str(),
			headerIter->second.c_str());

		pHeadersList = curl_slist_append(pHeadersList, pBuffer);
	}

	curl_easy_setopt(pCurlHandler, CURLOPT_FOLLOWLOCATION, 1);
	curl_easy_setopt(pCurlHandler, CURLOPT_USERAGENT, m_userAgent.c_str());
	curl_easy_setopt(pCurlHandler, CURLOPT_NOSIGNAL, (long)1);
	curl_easy_setopt(pCurlHandler, CURLOPT_TIMEOUT, (long)m_timeout);
#ifndef DEBUG
	curl_easy_setopt(pCurlHandler, CURLOPT_NOPROGRESS, 1);
#endif
	curl_easy_setopt(pCurlHandler, CURLOPT_HTTPHEADER, pHeadersList);
	curl_easy_setopt(pCurlHandler, CURLOPT_URL, url.c_str());
	// Use the default read function
	curl_easy_setopt(pCurlHandler, CURLOPT_READFUNCTION, NULL);
	curl_easy_setopt(pCurlHandler, CURLOPT_READDATA, pFile);
	curl_easy_setopt(pCurlHandler, CURLOPT_WRITEFUNCTION, writeCallback);
	curl_easy_setopt(pCurlHandler, CURLOPT_WRITEDATA, pContentInfo);
	curl_easy_setopt(pCurlHandler, CURLOPT_HEADERFUNCTION, headerCallback);
	curl_easy_setopt(pCurlHandler, CURLOPT_HEADERDATA, pContentInfo);
	curl_easy_setopt(pCurlHandler, CURLOPT_UPLOAD, 1);
	curl_easy_setopt(pCurlHandler, CURLOPT_INFILESIZE_LARGE, (curl_off_t)docInfo.getSize());

	CURLcode res = curl_easy_perform(pCurlHandler);
	if (res == CURLE_OK)
	{
#ifdef DEBUG
		clog << "CurlDownloader::putUrl: uploaded " << docInfo.getSize()
			<< " bytes to " << url << endl;
#endif
		pDocument = populateDocument(docInfo, url,
			pCurlHandler, pContentInfo);
	}
	else
	{
		clog << "Couldn't upload to " << url << ": " << curl_easy_strerror(res) << endl;
	}

	curl_slist_free_all(pHeadersList);
	curl_easy_cleanup(pCurlHandler);
	fclose(pFile);
	freeContentInfo(pContentInfo);
	delete pContentInfo;

	return pDocument;
}
