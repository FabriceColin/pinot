/*
 *  Copyright 2005-2021 Fabrice Colin
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

#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <strings.h>
#include <signal.h>
#include <errno.h>
#include <exception>
#include <iostream>
#include <fstream>
#include <sstream>
#include <tuple>
#include <glibmm/miscutils.h>
#include <glibmm/ustring.h>

#include "config.h"
#include "NLS.h"
#include "MIMEScanner.h"
#include "TimeConverter.h"
#include "Timer.h"
#include "Url.h"
#include "MetaDataBackup.h"
#include "ModuleFactory.h"
#include "DaemonState.h"
#include "PinotSettings.h"
#include "ServerThreads.h"

using namespace Glib;
using namespace std;

CrawlerThread::CrawlerThread(const string &dirName, bool isSource,
	MonitorInterface *pMonitor, MonitorHandler *pHandler,
	bool inlineIndexing) :
	DirectoryScannerThread(dirName,
		PinotSettings::getInstance().m_daemonIndexLocation,
		0, inlineIndexing, true),
	m_sourceId(0),
	m_pMonitor(pMonitor),
	m_pHandler(pHandler),
	m_crawlHistory(PinotSettings::getInstance().getHistoryDatabaseName())
{
	if (m_dirName.empty() == false)
	{
		if (isSource == true)
		{
			// Does this source exist ?
			if (m_crawlHistory.hasSource("file://" + m_dirName, m_sourceId) == false)
			{
				// Create it
				m_sourceId = m_crawlHistory.insertSource("file://" + m_dirName);
			}
#ifdef DEBUG
			clog << "CrawlerThread: source " << m_sourceId << endl;
#endif
		}
		else
		{
			map<unsigned int, string> fileSources;

			// What source does this belong to ?
			for(map<unsigned int, string>::const_iterator sourceIter = fileSources.begin();
				sourceIter != fileSources.end(); ++sourceIter)
			{
				if (sourceIter->second.length() < m_dirName.length())
				{
					// Skip
					continue;
				}

				if (sourceIter->second.substr(0, m_dirName.length()) == m_dirName)
				{
					// That's the one
					m_sourceId = sourceIter->first;
					break;
				}
			}
#ifdef DEBUG
			clog << "CrawlerThread: under source " << m_sourceId << endl;
#endif
		}
	}
}

CrawlerThread::~CrawlerThread()
{
}

string CrawlerThread::getType(void) const
{
	return "CrawlerThread";
}

void CrawlerThread::recordCrawled(const string &location, time_t itemDate)
{
	// It may still be in the cache
	map<string, CrawlItem>::iterator updateIter = m_crawlCache.find(location);
	if (updateIter != m_crawlCache.end())
	{
		updateIter->second.m_itemStatus = CrawlHistory::CRAWLED;
		updateIter->second.m_itemDate = itemDate;
#ifdef DEBUG
		clog << "CrawlerThread::recordCrawled: updated " << location << endl;
#endif
	}
	else
	{
#ifdef DEBUG
		clog << "CrawlerThread::recordCrawled: cached " << location << endl;
#endif
		m_crawlCache[location] = CrawlItem(CrawlHistory::CRAWLED, itemDate, 0);
		if (m_crawlCache.size() > 500)
		{
			flushUpdates();
		}
	}
}

bool CrawlerThread::isIndexable(const string &entryName) const
{
	string entryDir(path_get_dirname(entryName) + "/");

	// Is this under one of the locations configured for indexing ?
	for (set<PinotSettings::IndexableLocation>::const_iterator locationIter = PinotSettings::getInstance().m_indexableLocations.begin();
		locationIter != PinotSettings::getInstance().m_indexableLocations.end(); ++locationIter)
	{
		string locationDir(locationIter->m_name + "/");

		if ((entryDir.length() >= locationDir.length()) &&
			(entryDir.substr(0, locationDir.length()) == locationDir))
		{
			// Yes, it is
#ifdef DEBUG
			clog << "CrawlerThread::isIndexable: under " << locationDir << endl;
#endif
			return true;
		}
	}

	return false;
}

bool CrawlerThread::wasCrawled(const string &location, time_t &itemDate)
{
	CrawlHistory::CrawlStatus itemStatus = CrawlHistory::UNKNOWN;

	// Is it in the cache ?
	map<string, CrawlItem>::const_iterator updateIter = m_crawlCache.find(location);
	if (updateIter != m_crawlCache.end())
	{
		itemStatus = updateIter->second.m_itemStatus;
		itemDate = updateIter->second.m_itemDate;

		return true;
	}

	if (m_crawlHistory.hasItem(location, itemStatus, itemDate) == true)
	{
		return true;
	}

	return false;
}

void CrawlerThread::recordCrawling(const string &location, bool itemExists, time_t &itemDate)
{
	if (itemExists == false)
	{
		// Record it
		m_crawlHistory.insertItem(location, CrawlHistory::CRAWLING, m_sourceId, itemDate);
#ifdef DEBUG
		clog << "CrawlerThread::recordCrawling: inserted " << location << " " << (itemExists ? "exists" : "is new") << endl;
#endif
	}
	else
	{
#ifdef DEBUG
		clog << "CrawlerThread::recordCrawling: cached " << location << " " << (itemExists ? "exists" : "is new") << endl;
#endif
		// Change the status
		m_crawlCache[location] = CrawlItem(CrawlHistory::CRAWLING, itemDate, 0);
		if (m_crawlCache.size() > 500)
		{
			flushUpdates();
		}
	}
}

void CrawlerThread::recordError(const string &location, int errorCode)
{
	// It may still be in the cache
	map<string, CrawlItem>::iterator updateIter = m_crawlCache.find(location);
	if (updateIter != m_crawlCache.end())
	{
		updateIter->second.m_itemStatus = CrawlHistory::CRAWL_ERROR;
		updateIter->second.m_itemDate = time(NULL);
		updateIter->second.m_errNum = errorCode;
	}
	else
	{
		m_crawlCache[location] = CrawlItem(CrawlHistory::CRAWL_ERROR, time(NULL), errorCode);
		if (m_crawlCache.size() > 500)
		{
			flushUpdates();
		}
	}
}

void CrawlerThread::recordSymlink(const string &location, time_t itemDate)
{
	m_crawlHistory.insertItem(location, CrawlHistory::CRAWL_LINK, m_sourceId, itemDate);
}

bool CrawlerThread::monitorEntry(const string &entryName)
{
	if (m_pMonitor != NULL)
	{
		return m_pMonitor->addLocation(entryName, true);
	}

	return true;
}

void CrawlerThread::unmonitorEntry(const string &entryName)
{
	if (m_pMonitor != NULL)
	{
		m_pMonitor->removeLocation(entryName);
	}
}

void CrawlerThread::foundFile(const DocumentInfo &docInfo)
{
	DocumentInfo docInfoWithLabels(docInfo);
	set<string> labels;
	stringstream labelStream;

	// Insert a label that identifies the source
	labelStream << "X-SOURCE" << m_sourceId;
	labels.insert(labelStream.str());
	docInfoWithLabels.setLabels(labels);

	DirectoryScannerThread::foundFile(docInfoWithLabels);
}

void CrawlerThread::flushUpdates(void)
{
#ifdef DEBUG
	clog << "CrawlerThread::flushUpdates: flushing updates" << endl;
#endif

	// Update these records
	m_crawlHistory.updateItems(m_crawlCache);
	m_crawlCache.clear();

#ifdef DEBUG
	clog << "CrawlerThread::flushUpdates: flushed updates" << endl;
#endif
}

void CrawlerThread::doWork(void)
{
	MetaDataBackup metaData(PinotSettings::getInstance().getHistoryDatabaseName());
	::Timer scanTimer;
	set<string> urls;
	unsigned int currentOffset = 0;

	if (m_dirName.empty() == true)
	{
		return;
	}
	scanTimer.start();

	clog << "Scanning " << m_dirName << endl;

	// Remove errors and links
	m_crawlHistory.deleteItems(m_sourceId, CrawlHistory::CRAWL_ERROR);
	m_crawlHistory.deleteItems(m_sourceId, CrawlHistory::CRAWL_LINK);
	// ...and entries the previous instance didn't have time to crawl
	m_crawlHistory.deleteItems(m_sourceId, CrawlHistory::CRAWLING);

	// Update this source's items status so that we can detect files that have been deleted
	m_crawlHistory.updateItemsStatus(CrawlHistory::CRAWLED, CrawlHistory::TO_CRAWL, m_sourceId);

	if (scanEntry(m_dirName) == false)
	{
		m_errorNum = OPENDIR_FAILED;
		m_errorParam = m_dirName;
	}
	flushUpdates();

	clog << "Scanned " << m_dirName << " in " << scanTimer.stop() << " ms" << endl;

	if (m_done == true)
	{
#ifdef DEBUG
		clog << "CrawlerThread::doWork: leaving cleanup until next crawl" << endl;
#endif
		return;
	}

	scanTimer.start();

	// All files left with status TO_CRAWL were not found in this crawl
	// Chances are they were removed after the last full scan
	while ((m_pHandler != NULL) &&
		(m_crawlHistory.getSourceItems(m_sourceId, CrawlHistory::TO_CRAWL, urls,
			currentOffset, currentOffset + 100) > 0))
	{
		for (set<string>::const_iterator urlIter = urls.begin();
			urlIter != urls.end(); ++urlIter)
		{
#ifdef DEBUG
			clog << "CrawlerThread::doWork: didn't find " << *urlIter << endl;
#endif
			// Inform the MonitorHandler
			m_pHandler->fileDeleted(urlIter->substr(7));

			// Delete this item
			m_crawlHistory.deleteItem(*urlIter);
			metaData.deleteItem(DocumentInfo("", *urlIter, "", ""), DocumentInfo::SERIAL_ALL);
		}

		// Next
		if (urls.size() < 100)
		{
			break;
		}
		currentOffset += 100;
	}

	clog << "Cleaned up " << currentOffset + urls.size()
		<< " history entries in " << scanTimer.stop() << " ms" << endl;
}

RestoreMetaDataThread::RestoreMetaDataThread() :
	WorkerThread()
{
}

RestoreMetaDataThread::~RestoreMetaDataThread()
{
}

string RestoreMetaDataThread::getType(void) const
{
	return "RestoreMetaDataThread";
}

void RestoreMetaDataThread::doWork(void)
{
	PinotSettings &settings = PinotSettings::getInstance();
	MetaDataBackup metaData(settings.getHistoryDatabaseName());
	::Timer restoreTimer;
	set<string> urls;
	unsigned int currentOffset = 0, totalCount = 0;

	IndexInterface *pIndex = settings.getIndex(settings.m_daemonIndexLocation);
	if (pIndex == NULL)
	{
		return;
	}
	if (pIndex->isGood() == false)
	{
		delete pIndex;
		return;
	}

	// Restore user-set metadata on all documents 
	for (set<PinotSettings::IndexableLocation>::const_iterator locationIter = settings.m_indexableLocations.begin();
		locationIter != settings.m_indexableLocations.end(); ++locationIter)
	{
		string dirName(locationIter->m_name);

		restoreTimer.start();

		while (metaData.getItems(string("file://") + dirName, urls,
			currentOffset, currentOffset + 100) == true)
		{
			for (set<string>::const_iterator urlIter = urls.begin();
				urlIter != urls.end(); ++urlIter)
			{
				unsigned int docId = pIndex->hasDocument(*urlIter);
				if (docId == 0)
				{
#ifdef DEBUG
					clog << "RestoreMetaDataThread::doWork: " << *urlIter << " is not indexed, can't be restored" << endl;
#endif
					continue;
				}

				DocumentInfo docInfo("", *urlIter, "", "");
				if (metaData.getItem(docInfo, DocumentInfo::SERIAL_FIELDS) == true)
				{
#ifdef DEBUG
					clog << "RestoreMetaDataThread::doWork: restored fields on " << *urlIter << endl;
#endif
					pIndex->updateDocumentInfo(docId, docInfo);
				}
				if (metaData.getItem(docInfo, DocumentInfo::SERIAL_LABELS) == true)
				{
#ifdef DEBUG
					clog << "RestoreMetaDataThread::doWork: restored " << docInfo.getLabels().size() << " labels on " << *urlIter << endl;
#endif
					pIndex->setDocumentLabels(docId, docInfo.getLabels(), true);
				}
			}

			// Next
			totalCount += urls.size();
			if (urls.size() < 100)
			{
				break;
			}
			currentOffset += 100;
			urls.clear();
		}
		clog << "Restored user-set metadata for " << totalCount << " documents in "
			<< dirName << ", in " << restoreTimer.stop() << " ms" << endl;
	}

	delete pIndex;
}

