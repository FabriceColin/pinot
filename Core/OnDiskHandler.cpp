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

#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <set>
#include <iostream>
#include <fstream>
#include <sstream>

#include "config.h"
#include "NLS.h"
#include "MIMEScanner.h"
#include "StringManip.h"
#include "Url.h"
#include "FilterWrapper.h"
#include "PinotSettings.h"
#include "OnDiskHandler.h"

using namespace std;

OnDiskHandler::OnDiskHandler() :
	MonitorHandler(),
	m_history(PinotSettings::getInstance().getHistoryDatabaseName()),
	m_metaData(PinotSettings::getInstance().getHistoryDatabaseName()),
	m_pIndex(PinotSettings::getInstance().getIndex(PinotSettings::getInstance().m_daemonIndexLocation))
{
	pthread_mutex_init(&m_mutex, NULL);
}

OnDiskHandler::~OnDiskHandler()
{
	pthread_mutex_destroy(&m_mutex);

	// Disconnect the signal
	sigc::signal2<void, DocumentInfo, bool>::slot_list_type slotsList = m_signalFileFound.slots();
	sigc::signal2<void, DocumentInfo, bool>::slot_list_type::iterator slotIter = slotsList.begin();
	if (slotIter != slotsList.end())
	{
		if (slotIter->empty() == false)
		{
			slotIter->block();
			slotIter->disconnect();
		}
	}

	if (m_pIndex != NULL)
	{
		delete m_pIndex;
	}
}

bool OnDiskHandler::fileMoved(const string &fileName, const string &previousFileName,
	IndexInterface::NameType type)
{
	set<unsigned int> docIdList;
	bool handledEvent = false;

#ifdef DEBUG
	clog << "OnDiskHandler::fileMoved: " << fileName << endl;
#endif
	if (m_pIndex == NULL)
	{
		return false;
	}

	pthread_mutex_lock(&m_mutex);
	// Get a list of documents in that directory/file
	if (type == IndexInterface::BY_FILE)
	{
		m_pIndex->listDocuments(string("file://") + previousFileName, docIdList, type);
	}
	else
	{
		m_pIndex->listDocuments(previousFileName, docIdList, type);
	}
	// ...and the directory/file itself
	unsigned int baseDocId = m_pIndex->hasDocument(string("file://") + previousFileName);
	if (baseDocId > 0)
	{
		docIdList.insert(baseDocId);
	}
	if (docIdList.empty() == false)
	{
		for (set<unsigned int>::const_iterator iter = docIdList.begin();
			iter != docIdList.end(); ++iter)
		{
			DocumentInfo docInfo;

#ifdef DEBUG
			clog << "OnDiskHandler::fileMoved: moving " << *iter << endl;
#endif
			if (m_pIndex->getDocumentInfo(*iter, docInfo) == true)
			{
				string newLocation(docInfo.getLocation());

				if (baseDocId == *iter)
				{
					Url previousUrlObj(previousFileName), urlObj(fileName);

					// Update the title if it was the directory/file name
					if (docInfo.getTitle() == previousUrlObj.getFile())
					{
						docInfo.setTitle(urlObj.getFile());
					}
				}

				string::size_type pos = newLocation.find(previousFileName);
				if (pos != string::npos)
				{
					newLocation.replace(pos, previousFileName.length(), fileName);

					// Change the location
					docInfo.setLocation(newLocation);

					handledEvent = replaceFile(*iter, docInfo);
#ifdef DEBUG
					clog << "OnDiskHandler::fileMoved: moved " << *iter << ", " << docInfo.getLocation() << endl;
#endif
				}
#ifdef DEBUG
				else clog << "OnDiskHandler::fileMoved: skipping " << newLocation << endl;
#endif
			}
		}
	}
#ifdef DEBUG
	else clog << "OnDiskHandler::fileMoved: no documents in " << previousFileName << endl;
#endif
	pthread_mutex_unlock(&m_mutex);

	return handledEvent;
}

bool OnDiskHandler::fileDeleted(const string &fileName, IndexInterface::NameType type)
{
	set<unsigned int> docIdList;
	string location(string("file://") + fileName);
	bool unindexedDocs = false, handledEvent = false;

#ifdef DEBUG
	clog << "OnDiskHandler::fileDeleted: " << fileName << endl;
#endif
	if (m_pIndex == NULL)
	{
		return false;
	}

	pthread_mutex_lock(&m_mutex);
	// Unindex all of the directory/file's documents
	if (type == IndexInterface::BY_FILE)
	{
		unindexedDocs = m_pIndex->unindexDocuments(location, type);
	}
	else
	{
		unindexedDocs = m_pIndex->unindexDocuments(fileName, type);
	}
	if (unindexedDocs == true)
	{
		// ...as well as the actual directory/file
		m_pIndex->unindexDocument(location);

		m_history.deleteItems(location);
		handledEvent = true;
	}
	pthread_mutex_unlock(&m_mutex);

	return handledEvent;
}

bool OnDiskHandler::indexFile(const string &fileName, bool isDirectory, unsigned int &sourceId)
{
	string location(string("file://") + fileName);
	Url urlObj(location);

	if (fileName.empty() == true)
	{
		return false;
	}

	// Is it black-listed ?
	if (PinotSettings::getInstance().isBlackListed(fileName) == true)
	{
		return false;
	}

	DocumentInfo docInfo("", location, MIMEScanner::scanUrl(urlObj), "");

	// What source does it belong to ?
	for (map<unsigned int, string>::const_iterator sourceIter = m_fileSources.begin();
		sourceIter != m_fileSources.end(); ++sourceIter)
	{
		sourceId = sourceIter->first;

		if (sourceIter->second.length() > location.length())
		{
			// Skip
			continue;
		}

		if (location.substr(0, sourceIter->second.length()) == sourceIter->second)
		{
			set<string> labels;
			stringstream labelStream;

			// That's the one
			labelStream << "X-SOURCE" << sourceIter->first;
#ifdef DEBUG
			clog << "OnDiskHandler::indexFile: source label for " << location << " is " << labelStream.str() << endl;
#endif
			labels.insert(labelStream.str());
			docInfo.setLabels(labels);
			break;
		}
#ifdef DEBUG
		else clog << "OnDiskHandler::indexFile: not " << sourceIter->second << endl;
#endif
	}

	m_metaData.getItem(docInfo, DocumentInfo::SERIAL_ALL);

	m_signalFileFound(docInfo, isDirectory);

	return true;
}

bool OnDiskHandler::replaceFile(unsigned int docId, DocumentInfo &docInfo)
{
	if (m_pIndex == NULL)
	{
		return false;
	}

	// Unindex the destination file
	FilterWrapper wrapFilter(m_pIndex);
	wrapFilter.unindexDocument(docInfo.getLocation());

	// Update the document info
	return m_pIndex->updateDocumentInfo(docId, docInfo);
}

void OnDiskHandler::initialize(void)
{
	set<string> directories;

	// Get the map of indexable locations
	set<PinotSettings::IndexableLocation> &indexableLocations = PinotSettings::getInstance().m_indexableLocations;
	for (set<PinotSettings::IndexableLocation>::iterator dirIter = indexableLocations.begin();
		dirIter != indexableLocations.end(); ++dirIter)
	{
		directories.insert(dirIter->m_name);
	}

	// Unindex documents that belong to sources that no longer exist
	if (m_history.getSources(m_fileSources) > 0)
	{
		for(map<unsigned int, string>::const_iterator sourceIter = m_fileSources.begin();
			sourceIter != m_fileSources.end(); ++sourceIter)
		{
			unsigned int sourceId = sourceIter->first;

			if (sourceIter->second.substr(0, 7) != "file://")
			{
				// Skip
				continue;
			}

			// Is this an indexable location ?
			if (directories.find(sourceIter->second.substr(7)) == directories.end())
			{
				stringstream labelStream;

				labelStream << "X-SOURCE" << sourceId;

#ifdef DEBUG
				clog << "OnDiskHandler::initialize: " << sourceIter->second
					<< ", source " << sourceId << " was removed" << endl;
#endif
				// All documents with this label will be unindexed
				if ((m_pIndex != NULL) &&
					(m_pIndex->unindexDocuments(labelStream.str(), IndexInterface::BY_LABEL) == true))
				{
					// Delete the source itself and all its items
					m_history.deleteSource(sourceId);
					m_history.deleteItems(sourceId);
				}
			}
#ifdef DEBUG
			else clog << "OnDiskHandler::initialize: " << sourceIter->second
				<< " is still configured" << endl;
#endif
		}
	}
}

bool OnDiskHandler::fileExists(const string &fileName)
{
	// Nothing to do here
	return true;
}

bool OnDiskHandler::fileCreated(const string &fileName)
{
	unsigned int sourceId;
	bool handledEvent = false;

#ifdef DEBUG
	clog << "OnDiskHandler::fileCreated: " << fileName << endl;
#endif
	pthread_mutex_lock(&m_mutex);
	// The file may exist in the index
	handledEvent = indexFile(fileName, false, sourceId);
	if (handledEvent == true)
	{
	        string location("file://" + fileName);
		CrawlHistory::CrawlStatus status = CrawlHistory::UNKNOWN;
	        time_t itemDate;

		// ...and therefore may exist in the history database
		if (m_history.hasItem(location, status, itemDate) == true)
		{
			m_history.updateItem(location, CrawlHistory::CRAWLED, time(NULL));
		}
		else
		{
			m_history.insertItem(location, CrawlHistory::CRAWLED, sourceId, time(NULL));
		}
	}
	pthread_mutex_unlock(&m_mutex);

	return handledEvent;
}

bool OnDiskHandler::directoryCreated(const string &dirName)
{
	unsigned int sourceId;
	bool handledEvent = false;

#ifdef DEBUG
	clog << "OnDiskHandler::directoryCreated: " << dirName << endl;
#endif
	pthread_mutex_lock(&m_mutex);
	handledEvent = indexFile(dirName, true, sourceId);
	// History will be set by crawling
	pthread_mutex_unlock(&m_mutex);

	return handledEvent;
}

bool OnDiskHandler::fileModified(const string &fileName)
{
	unsigned int sourceId;
	bool handledEvent = false;

#ifdef DEBUG
	clog << "OnDiskHandler::fileModified: " << fileName << endl;
#endif
	pthread_mutex_lock(&m_mutex);
	// Update the file, or index if necessary
	handledEvent = indexFile(fileName, false, sourceId);
	if (handledEvent == true)
	{
		m_history.updateItem("file://" + fileName, CrawlHistory::CRAWLED, time(NULL));
	}
	pthread_mutex_unlock(&m_mutex);

	return handledEvent;
}

bool OnDiskHandler::fileMoved(const string &fileName, const string &previousFileName)
{
	return fileMoved(fileName, previousFileName, IndexInterface::BY_FILE);
}

bool OnDiskHandler::directoryMoved(const string &dirName,
	const string &previousDirName)
{
	return fileMoved(dirName, previousDirName, IndexInterface::BY_DIRECTORY);
}

bool OnDiskHandler::fileDeleted(const string &fileName)
{
	return fileDeleted(fileName, IndexInterface::BY_FILE);
}

bool OnDiskHandler::directoryDeleted(const string &dirName)
{
	return fileDeleted(dirName, IndexInterface::BY_DIRECTORY);
}

sigc::signal2<void, DocumentInfo, bool>& OnDiskHandler::getFileFoundSignal(void)
{
	return m_signalFileFound;
}

