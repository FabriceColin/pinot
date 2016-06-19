/*
 *  Copyright 2007-2012 Fabrice Colin
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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include <set>

#include "config.h"
#include "Memory.h"
#include "MIMEScanner.h"
#include "StringManip.h"
#include "TimeConverter.h"
#include "Url.h"
#include "TextConverter.h"
#include "filters/FilterFactory.h"
#include "FilterUtils.h"

#define UNSUPPORTED_TYPE "X-Unsupported"
#define SIZE_THRESHOLD 5242880

using std::clog;
using std::clog;
using std::endl;
using std::string;
using std::set;
using std::map;

set<string> FilterUtils::m_types;
map<string, string> FilterUtils::m_typeAliases;
string FilterUtils::m_maxNestedSize;

ReducedAction::ReducedAction()
{
}

ReducedAction::ReducedAction(const ReducedAction &other)
{
}

ReducedAction::~ReducedAction()
{
}

ReducedAction &ReducedAction::operator=(const ReducedAction &other)
{
	return *this;
}

bool ReducedAction::positionFilter(const Document &doc, Dijon::Filter *pFilter)
{
	return false;
}

bool ReducedAction::isReduced(const Document &doc)
{
	// Is it reduced to plain text ?
	if ((doc.getType().length() >= 10) &&
		(doc.getType().substr(0, 10) == "text/plain"))
	{
		return true;
	}

	return false;
}

FilterUtils::FilterUtils()
{
	char *pEnvVar = getenv("PINOT_MAXIMUM_NESTED_SIZE");
	if ((pEnvVar != NULL) &&
		(strlen(pEnvVar) > 0))
	{
		off_t maxSize = (off_t)atoll(pEnvVar);
		if (maxSize > 0)
		{
			m_maxNestedSize = pEnvVar;
		}
	}
}

FilterUtils::~FilterUtils()
{
}

Dijon::Filter *FilterUtils::getFilter(const string &mimeType)
{
	Dijon::Filter *pFilter = NULL;

	// Is this type aliased ?
	map<string, string>::const_iterator aliasIter = m_typeAliases.find(mimeType);
	if (aliasIter != m_typeAliases.end())
	{
		if (aliasIter->second == UNSUPPORTED_TYPE)
		{
			// We already know that none of this type's parents are supported
			return NULL;
		}

		pFilter = Dijon::FilterFactory::getFilter(aliasIter->second);
	}
	else
	{
		// Is there a filter for this type ?
		pFilter = Dijon::FilterFactory::getFilter(mimeType);
	}

	if (pFilter != NULL)
	{
		return pFilter;
	}

	if (mimeType.empty() == false)
	{
		set<string> parentTypes;

		if (m_types.empty() == true)
		{
			Dijon::FilterFactory::getSupportedTypes(m_types);
		}

		// Try that type's parents
		MIMEScanner::getParentTypes(mimeType, m_types, parentTypes);
		for (set<string>::const_iterator parentIter = parentTypes.begin();
			parentIter != parentTypes.end(); ++parentIter)
		{
			pFilter = Dijon::FilterFactory::getFilter(*parentIter);
			if (pFilter != NULL)
			{
				// Add an alias
				m_typeAliases[mimeType] = *parentIter;
				return pFilter;
			}
		}
#ifdef DEBUG
		clog << "FilterUtils::getFilter: no valid parent for " << mimeType << endl;
#endif

		// This type has no valid parent
		m_typeAliases[mimeType] = UNSUPPORTED_TYPE;
	}

	return NULL;
}

bool FilterUtils::isSupportedType(const string &mimeType)
{
	// Is this type aliased ?
	map<string, string>::const_iterator aliasIter = m_typeAliases.find(mimeType);
	if (aliasIter != m_typeAliases.end())
	{
		if (aliasIter->second == UNSUPPORTED_TYPE)
		{
			return false;
		}

		// We were able to get a filter for this parent type
		// or a previous call to isSupportedType() succeeded
		return true;
	}

	if (Dijon::FilterFactory::isSupportedType(mimeType) == true)
	{
		return true;
	}

	if (m_types.empty() == true)
	{
		Dijon::FilterFactory::getSupportedTypes(m_types);
	}

	// Try that type's parents
	set<string> parentTypes;
	MIMEScanner::getParentTypes(mimeType, m_types, parentTypes);
	for (set<string>::const_iterator parentIter = parentTypes.begin();
		parentIter != parentTypes.end(); ++parentIter)
	{
		if (Dijon::FilterFactory::isSupportedType(*parentIter) == true)
		{
			// Add an alias
			m_typeAliases[mimeType] = *parentIter;
			return true;
		}
	}
#ifdef DEBUG
	clog << "FilterUtils::isSupportedType: no valid parent for " << mimeType << endl;
#endif

	// This type has no valid parent
	m_typeAliases[mimeType] = UNSUPPORTED_TYPE;

	return false;
}

bool FilterUtils::feedFilter(const Document &doc, Dijon::Filter *pFilter)
{
	string location(doc.getLocation());
	Url urlObj(location);
	string fileName;
	off_t dataLength = 0;
	const char *pData = doc.getData(dataLength);
	bool fedInput = false;

	if (pFilter == NULL)
	{
		return false;
	}

	if ((urlObj.getProtocol() == "file") &&
		(location.length() > 7))
	{
		fileName = location.substr(7);
	}

	// Prefer feeding the data
	if (((dataLength > 0) && (pData != NULL)) &&
		(pFilter->is_data_input_ok(Dijon::Filter::DOCUMENT_DATA) == true))
	{
		fedInput = pFilter->set_document_data(pData, dataLength);
	}
	// ... to feeding the data through a temporary file
	if ((fedInput == false) &&
		((dataLength > 0) && (pData != NULL)) &&
		(pFilter->is_data_input_ok(Dijon::Filter::DOCUMENT_FILE_NAME) == true))
	{
		char inTemplate[18] = "/tmp/filterXXXXXX";

#ifdef HAVE_MKSTEMP
		int inFd = mkstemp(inTemplate);
#else
		int inFd = -1;
		char *pInFile = mktemp(inTemplate);
		if (pInFile != NULL)
		{
			inFd = open(pInFile, O_RDONLY);
		}
#endif
		if (inFd != -1)
		{
#ifdef DEBUG
			clog << "FilterUtils::feedFilter: feeding temporary file " << inTemplate << endl;
#endif

			// Save the data
			if (write(inFd, (const void*)pData, dataLength) != -1)
			{
				fedInput = pFilter->set_document_file(inTemplate, true);
				if (fedInput == false)
				{
					// We might as well delete the file now
					unlink(inTemplate);
				}
			}

			close(inFd);
		}
	}
	// ... to feeding the file
	if ((fedInput == false) &&
		(fileName.empty() == false) &&
		(doc.getInternalPath().empty() == true))
	{ 
		if (pFilter->is_data_input_ok(Dijon::Filter::DOCUMENT_FILE_NAME) == true)
		{
#ifdef DEBUG
			clog << "FilterUtils::feedFilter: feeding file " << fileName << endl;
#endif
			fedInput = pFilter->set_document_file(fileName);
		}
		// ...and to feeding the file's contents
		if ((fedInput == false) &&
			(pFilter->is_data_input_ok(Dijon::Filter::DOCUMENT_DATA) == true))
		{
			Document docCopy(doc);

			if (docCopy.setDataFromFile(fileName) == false)
			{
				clog << "Couldn't load " << fileName << endl;

				return false;
			}
#ifdef DEBUG
			clog << "FilterUtils::feedFilter: feeding contents of file " << fileName << endl;
#endif

			pData = docCopy.getData(dataLength);
			if ((dataLength > 0) &&
				(pData != NULL))
			{
				fedInput = pFilter->set_document_data(pData, dataLength);
			}
			// Else, the file may be empty
		}
	}

	if (fedInput == false)
	{
		clog << "Couldn't feed filter for " << doc.getLocation(true) << endl;

		return false;
	}

	return true;
}

bool FilterUtils::populateDocument(Document &doc, Dijon::Filter *pFilter)
{
	string charset, uri, ipath;
	off_t size = 0;
	bool checkDataType = false, checkFileType = false;

	if (pFilter == NULL)
	{
		return false;
	}

	// Go through the whole thing
	const map<string, string> &metaData = pFilter->get_meta_data();
	for (map<string, string>::const_iterator metaIter = metaData.begin();
		metaIter != metaData.end(); ++metaIter)
	{
		if (metaIter->first == "charset")
		{
			charset = metaIter->second;
		}
		else if (metaIter->first == "date")
		{
			doc.setTimestamp(metaIter->second);
		}
		else if (metaIter->first == "ipath")
		{
			ipath = metaIter->second;
		}
		else if (metaIter->first == "language")
		{
			doc.setLanguage(metaIter->second);
		}
		else if (metaIter->first == "mimetype")
		{
			string mimeType(StringManip::toLowerCase(metaIter->second));

			if (mimeType == "scan")
			{
				checkDataType = true;
			}
			else if (mimeType == "scantitle")
			{
				checkFileType = true;
			}
			else
			{
				doc.setType(mimeType);
			}
		}
		else if (metaIter->first == "size")
		{
			size = (off_t)atoll(metaIter->second.c_str());

			if (size > 0)
			{
				doc.setSize(size);
			}
#ifdef DEBUG
			else clog << "FilterUtils::populateDocument: ignoring size zero" << endl;
#endif
		}
		else if (metaIter->first == "uri")
		{
			uri = metaIter->second;

			if ((uri.length() >= 18) &&
				(uri.find(":///tmp/filter") != string::npos))
			{
				// We fed the filter a temporary file
				uri.clear();
			}
		}
		else
		{
			doc.setOther(metaIter->first, metaIter->second);
		}
	}

	if (uri.empty() == false)
	{
		doc.setLocation(uri);
	}
	if (ipath.empty() == false)
	{
		string currentIPath(doc.getInternalPath());

		if (currentIPath.empty() == false)
		{
			currentIPath += "&next&";
		}
		currentIPath += ipath;
		doc.setInternalPath(currentIPath);
#ifdef DEBUG
		clog << "FilterUtils::populateDocument: ipath " << currentIPath << endl;
#endif
	}

	// Content and title may have to be converted
	TextConverter converter(20);

	map<string, string>::const_iterator contentIter = metaData.find("title");
	if ((contentIter != metaData.end()) &&
		(contentIter->second.empty() == false))
	{
		dstring nonUTF8Title(contentIter->second.c_str(), contentIter->second.length());

		dstring utf8Data(converter.toUTF8(nonUTF8Title, charset));

		doc.setTitle(string(utf8Data.c_str(), utf8Data.length()));
	}

	const dstring &content = pFilter->get_content();
	if (content.empty() == false)
	{
		// Scan for the MIME type ?
		if (checkFileType == true)
		{
			// Assume the title is actually a file name
			string mimeType(MIMEScanner::scanFile(doc.getTitle()));

			if ((mimeType.empty() == true) ||
				(mimeType == "application/octet-stream"))
			{
				// Revert to scanning the content 
				checkDataType = true;
			}
			else
			{
				doc.setType(mimeType);
			}
		}
		if (checkDataType == true)
		{
			doc.setType(MIMEScanner::scanData(content.c_str(), content.length()));
		}

		if (doc.getType().substr(0, 10) == "text/plain")
		{
			dstring utf8Data(converter.toUTF8(content, charset));

			if (converter.getErrorsCount() > 0)
			{
				clog << doc.getLocation(true) << " may not have been fully converted to UTF-8" << endl;
			}
			doc.setData(utf8Data.c_str(), utf8Data.length());
		}
		else
		{
			doc.setData(content.c_str(), content.length());
		}
	}

	// If the document is big'ish, try and reclaim memory
	int inUse = Memory::getUsage();
	if ((size > SIZE_THRESHOLD) ||
		(content.length() > SIZE_THRESHOLD))
	{
		Memory::reclaim();
	}

	return true;
}

bool FilterUtils::filterDocument(const Document &doc, const string &originalType,
	ReducedAction &action)
{
	Dijon::Filter *pFilter = FilterUtils::getFilter(doc.getType());
	bool fedFilter = false, positionedFilter = false, docSuccess = false, finalSuccess = false;

	if (pFilter != NULL)
	{
		// Limit the size of nested documents ?
		if (m_maxNestedSize.empty() == false)
		{
			pFilter->set_property(Dijon::Filter::MAXIMUM_NESTED_SIZE, m_maxNestedSize);
		}
		fedFilter = FilterUtils::feedFilter(doc, pFilter);
	}
	positionedFilter = action.positionFilter(doc, pFilter);

	if (fedFilter == false)
	{
		Document docCopy(doc);

		if (docCopy.getTitle().empty() == true)
		{
			Url urlObj(doc.getLocation());

			// Default to the file name as title
			docCopy.setTitle(urlObj.getFile());
		}

		// Take the appropriate action now
		finalSuccess = action.takeAction(docCopy, false);

		if (pFilter != NULL)
		{
			delete pFilter;
		}

		return finalSuccess;
	}

	// At this point, pFilter cannot be NULL
	bool hasDocs = pFilter->has_documents();
#ifdef DEBUG
	clog << "FilterUtils::filterDocument: has documents " << hasDocs << endl;
#endif
	while (hasDocs == true)
	{
		string actualType(originalType);
		bool isNested = false;
		bool emptyTitle = false;

		if ((positionedFilter == false) &&
			(pFilter->next_document() == false))
		{
#ifdef DEBUG
			clog << "FilterUtils::filterDocument: no more documents in " << doc.getLocation(true) << endl;
#endif
			break;
		}

		const DocumentInfo *pInfo = dynamic_cast<const DocumentInfo*>(&doc);
		string originalTitle(doc.getTitle());

		if (pInfo == NULL)
		{
#ifdef DEBUG
			clog << "FilterUtils::filterDocument: couldn't duplicate document information" << endl;
#endif
			break;
		}

		Document filteredDoc(*pInfo);

		filteredDoc.setType("text/plain");
		docSuccess = false;

		if (populateDocument(filteredDoc, pFilter) == false)
		{
			hasDocs = pFilter->has_documents();
			continue;
		}

		// Is this a nested document ?
		if (filteredDoc.getInternalPath().length() > doc.getInternalPath().length())
		{
			actualType = filteredDoc.getType();
#ifdef DEBUG
			clog << "FilterUtils::filterDocument: nested document of type " << actualType << endl;
#endif
			isNested = true;
		}
		else if (originalTitle.empty() == false)
		{
			// Preserve the top-level document's title
			filteredDoc.setTitle(originalTitle);
		}
		else if (filteredDoc.getTitle().empty() == true)
		{
			emptyTitle = true;
		}

		// Pass it down to another filter ?
		if (action.isReduced(filteredDoc) == true)
		{
			// Do we need to set a default title ?
			if (emptyTitle == true)
			{
				Url urlObj(doc.getLocation());

				// Default to the file name as title
				filteredDoc.setTitle(urlObj.getFile());
#ifdef DEBUG
				clog << "FilterUtils::filterDocument: set default title " << urlObj.getFile() << endl;
#endif
			}

			filteredDoc.setType(actualType);

			// Take the appropriate action
			docSuccess = action.takeAction(filteredDoc, isNested);
		}
		else
		{
			docSuccess = filterDocument(filteredDoc, actualType, action);
		}

		// Consider indexing anything a success
		if (docSuccess == true)
		{
			finalSuccess = true;
		}

		if (positionedFilter == true)
		{
			break;
		}

		// Next
		hasDocs = pFilter->has_documents();
	}

	delete pFilter;

#ifdef DEBUG
	clog << "FilterUtils::filterDocument: done with " << doc.getLocation(true) << " status " << finalSuccess << endl;
#endif

	return finalSuccess;
}

bool FilterUtils::reduceDocument(const Document &doc, ReducedAction &action)
{
	string originalType(doc.getType());

	return filterDocument(doc, originalType, action);
}

string FilterUtils::stripMarkup(const string &text)
{
	if (text.empty() == true)
	{
		return "";
	}

	Dijon::Filter *pFilter = Dijon::FilterFactory::getFilter("text/xml");

	if (pFilter == NULL)
	{
		return "";
	}

	Document doc;
	string strippedText;

	doc.setData(text.c_str(), text.length());
	if ((feedFilter(doc, pFilter) == true) &&
		(pFilter->next_document() == true))
	{
		const dstring &content = pFilter->get_content();
		if (content.empty() == false)
		{
			strippedText = string(content.c_str(), content.length());
		}
	}

	delete pFilter;

	return strippedText;
}
