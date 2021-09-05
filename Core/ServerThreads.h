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

#ifndef _SERVERTHREADS_HH
#define _SERVERTHREADS_HH

#include <string>
#include <stack>

#include "DocumentInfo.h"
#include "CrawlHistory.h"
#include "IndexInterface.h"
#include "MonitorInterface.h"
#include "MonitorHandler.h"
#include "QueryProperties.h"
#include "WorkerThreads.h"

class CrawlerThread : public DirectoryScannerThread
{
	public:
		CrawlerThread(const std::string &dirName, bool isSource,
			MonitorInterface *pMonitor, MonitorHandler *pHandler,
			bool inlineIndexing = false);
		virtual ~CrawlerThread();

		virtual std::string getType(void) const;

	protected:
		unsigned int m_sourceId;
		MonitorInterface *m_pMonitor;
		MonitorHandler *m_pHandler;
		CrawlHistory m_crawlHistory;
		std::map<std::string, CrawlItem> m_crawlCache;
		std::stack<std::string> m_currentLinks;
		std::stack<std::string> m_currentLinkReferrees;

		virtual void recordCrawled(const std::string &location, time_t itemDate);
		virtual bool isIndexable(const std::string &entryName) const;
		virtual bool wasCrawled(const std::string &location, time_t &itemDate);
		virtual void recordCrawling(const std::string &location, bool itemExists, time_t &itemDate);
		virtual void recordError(const std::string &location, int errorCode);
		virtual void recordSymlink(const std::string &location, time_t itemDate);
		virtual bool monitorEntry(const std::string &entryName);
		virtual void foundFile(const DocumentInfo &docInfo);

		void flushUpdates(void);
		virtual void doWork(void);

	private:
		CrawlerThread(const CrawlerThread &other);
		CrawlerThread &operator=(const CrawlerThread &other);

};

class RestoreMetaDataThread : public WorkerThread
{
	public:
		RestoreMetaDataThread();
		virtual ~RestoreMetaDataThread();

		virtual std::string getType(void) const;

	protected:
		virtual void doWork(void);

	private:
		RestoreMetaDataThread(const RestoreMetaDataThread &other);
		RestoreMetaDataThread &operator=(const RestoreMetaDataThread &other);

};

#endif // _SERVERTHREADS_HH
