/*
 *  Copyright 2005-2012 Fabrice Colin
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
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#ifdef HAVE_STATFS
  #ifdef HAVE_SYS_VFS_H
  #include <sys/vfs.h>
  #define CHECK_DISK_SPACE 1
  #else
    #ifdef HAVE_SYS_STATFS_H
      #include <sys/statfs.h>
      #define CHECK_DISK_SPACE 1
    #else
      #ifdef HAVE_SYS_MOUNT_H
        #if defined(__OpenBSD__) || (defined(__FreeBSD__) && (_FreeBSD_version < 700000))
          #include <sys/param.h>
        #endif
        #include <sys/mount.h>
        #define CHECK_DISK_SPACE 1
      #endif
    #endif
  #endif
#else
  #ifdef HAVE_STATVFS
  #include <sys/statvfs.h>
  #define CHECK_DISK_SPACE 1
  #endif
#endif
#ifdef __FreeBSD__
#ifdef HAVE_SYSCTLBYNAME
#include <sys/sysctl.h>
#define CHECK_BATTERY_SYSCTL 1
#endif
#endif
#include <iostream>
#include <algorithm>
#include <glibmm/ustring.h>
#include <glibmm/stringutils.h>
#include <glibmm/convert.h>
#include <glibmm/thread.h>
#include <glibmm/random.h>

#include "Memory.h"
#include "Url.h"
#include "MonitorFactory.h"
#include "CrawlHistory.h"
#ifdef HAVE_DBUS
#include "DBusIndex.h"
#endif
#include "DaemonState.h"
#include "OnDiskHandler.h"
#include "PinotSettings.h"
#include "ServerThreads.h"

using namespace std;
using namespace Glib;

static double getFSFreeSpace(const string &path)
{
	double availableBlocks = 0.0;
	double blockSize = 0.0;
	int statSuccess = -1;
#ifdef HAVE_STATFS
	struct statfs fsStats;

	statSuccess = statfs(PinotSettings::getInstance().m_daemonIndexLocation.c_str(), &fsStats);
	availableBlocks = (uintmax_t)fsStats.f_bavail;
	blockSize = fsStats.f_bsize;
#else
#ifdef HAVE_STATVFS
	struct statvfs vfsStats;

	statSuccess = statvfs(path.c_str(), &vfsStats);
	availableBlocks = (uintmax_t)vfsStats.f_bavail;
	// f_frsize isn't supported by all implementations
	blockSize = (vfsStats.f_frsize ? vfsStats.f_frsize : vfsStats.f_bsize);
#endif
#endif
	// Did it fail ?
	if ((statSuccess == -1) ||
		(blockSize == 0.0))
	{
		return -1.0;
	}

	double mbRatio = blockSize / (1024 * 1024);
	double availableMbSize = availableBlocks * mbRatio;
#ifdef DEBUG
	clog << "DaemonState::getFSFreeSpace: " << availableBlocks << " blocks of " << blockSize
		<< " bytes (" << mbRatio << ")" << endl;
#endif

	return availableMbSize;
}

// A function object to stop Crawler threads with for_each()
struct StopCrawlerThreadFunc
{
public:
	void operator()(map<unsigned int, WorkerThread *>::value_type &p)
	{
		string type(p.second->getType());

		if (type == "CrawlerThread")
		{
			p.second->stop();
#ifdef DEBUG
			clog << "StopCrawlerThreadFunc: stopped thread " << p.second->getId() << endl;
#endif
		}
	}
};

#ifdef HAVE_DBUS
DBusServletInfo::DBusServletInfo(DBusConnection *pConnection, DBusMessage *pRequest) :
	m_pConnection(pConnection),
	m_pRequest(pRequest),
	m_pReply(NULL),
	m_pArray(NULL),
	m_simpleQuery(true),
	m_pThread(NULL),
	m_replied(false)
{
}

DBusServletInfo::~DBusServletInfo()
{
	if (m_pReply != NULL)
	{
		dbus_message_unref(m_pReply);
	}
	if (m_pRequest != NULL)
	{
		dbus_message_unref(m_pRequest);
	}
	if (m_pConnection != NULL)
	{
		dbus_connection_unref(m_pConnection);
	}
	if (m_pArray != NULL)
	{
		// Free the array
		g_ptr_array_free(m_pArray, TRUE);
	}
}

bool DBusServletInfo::newReply(void)
{
        if (m_pRequest == NULL) 
        {
                return false;
        }

        m_pReply = dbus_message_new_method_return(m_pRequest);
        if (m_pReply != NULL)
        {
                return true;
        }

        return false;
}

bool DBusServletInfo::newErrorReply(const string &name, const string &message)
{
        if (m_pRequest == NULL) 
        {
                return false;
        }

	if (m_pReply != NULL)
	{
		dbus_message_unref(m_pReply);
		m_pReply = NULL;
	}

	string fullName(PINOT_DBUS_SERVICE_NAME);
	fullName += ".";
	fullName += name;
	m_pReply = dbus_message_new_error(m_pRequest,
		fullName.c_str(), message.c_str());
        if (m_pReply != NULL)
        {
                return true;
        }

        return false;
}

bool DBusServletInfo::newReplyWithArray(void)
{
	if (newReply() == true)
	{
		dbus_message_append_args(m_pReply,
			DBUS_TYPE_ARRAY, DBUS_TYPE_STRING, &m_pArray->pdata, m_pArray->len,
			DBUS_TYPE_INVALID);

		return true;
	}

	return false;
}

bool DBusServletInfo::newQueryReply(const vector<DocumentInfo> &resultsList,
	unsigned int resultsEstimate)
{
	DBusMessageIter iter, subIter;

	if (m_simpleQuery == false)
	{
		// Create the reply
		if (newReply() == false)
		{
			return false;
		}

		// ...and attach a container
		dbus_message_iter_init_append(m_pReply, &iter);
		dbus_message_iter_append_basic(&iter, DBUS_TYPE_UINT32,
			&resultsEstimate);
		dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY,
			 DBUS_TYPE_ARRAY_AS_STRING \
			 DBUS_STRUCT_BEGIN_CHAR_AS_STRING \
			 DBUS_TYPE_STRING_AS_STRING \
			 DBUS_TYPE_STRING_AS_STRING \
			 DBUS_STRUCT_END_CHAR_AS_STRING, &subIter);
	}
	else
	{
		// Create an array
		// FIXME: use a container for this too
		m_pArray = g_ptr_array_new();
	}

	for (vector<DocumentInfo>::const_iterator resultIter = resultsList.begin();
		resultIter != resultsList.end(); ++resultIter)
	{
		unsigned int indexId = 0;
		unsigned int docId = resultIter->getIsIndexed(indexId);

#ifdef DEBUG
		clog << "DBusServletInfo::newQueryReply: adding result " << docId << endl;
#endif
		if (m_simpleQuery == false)
		{
			// The document ID isn't needed here
			if (DBusIndex::documentInfoToDBus(&subIter, 0, *resultIter) == false)
			{
				newErrorReply("Query", "Unknown error");
				return false;
			}
		}
		else if (docId > 0)
		{
			char docIdStr[64];

			// We only need the document ID
			snprintf(docIdStr, 64, "%u", docId);
			g_ptr_array_add(m_pArray, const_cast<char*>(docIdStr));
		}
	}

	if (m_simpleQuery == false)
	{
		// Close the container
		dbus_message_iter_close_container(&iter, &subIter);
		return true;
	}

	// Attach the array to the reply
	return newReplyWithArray();
}

bool DBusServletInfo::reply(void)
{
	// Send a reply ?
	if ((m_pConnection != NULL) &&
		(m_pReply != NULL) &&
		(m_replied == false))
	{
		m_replied = true;

		dbus_connection_send(m_pConnection, m_pReply, NULL);
		dbus_connection_flush(m_pConnection);
#ifdef DEBUG
		clog << "DBusServletInfo::reply: sent reply" << endl;
#endif

		return true;
	}

	return false;
}
#endif

DaemonState::DaemonState() :
	QueueManager(PinotSettings::getInstance().m_daemonIndexLocation),
	m_isReindex(false),
	m_reload(false),
	m_flush(false),
	m_crawlHistory(PinotSettings::getInstance().getHistoryDatabaseName()),
	m_pDiskMonitor(MonitorFactory::getMonitor()),
	m_pDiskHandler(NULL),
	m_crawlers(0)
{
	FD_ZERO(&m_flagsSet);

	// Check disk usage every minute
	m_timeoutConnection = Glib::signal_timeout().connect(sigc::mem_fun(*this,
		&DaemonState::on_activity_timeout), 60000);
	// Check right now before doing anything else
	DaemonState::on_activity_timeout();

	m_onThreadEndSignal.connect(sigc::mem_fun(*this, &DaemonState::on_thread_end));
}

DaemonState::~DaemonState()
{
	// Don't delete m_pDiskMonitor and m_pDiskHandler, threads may need them
	// Since DaemonState is destroyed when the program exits, it's a leak we can live with
}

bool DaemonState::on_activity_timeout(void)
{
	if (m_timeoutConnection.blocked() == false)
	{
#ifdef CHECK_DISK_SPACE
		double availableMbSize = getFSFreeSpace(PinotSettings::getInstance().m_daemonIndexLocation);
		if (availableMbSize >= 0)
		{
#ifdef DEBUG
			clog << "DaemonState::on_activity_timeout: " << availableMbSize << " Mb free for "
				<< PinotSettings::getInstance().m_daemonIndexLocation << endl;
#endif
			if (availableMbSize < PinotSettings::getInstance().m_minimumDiskSpace)
			{
				// Stop indexing
				m_stopIndexing = true;
				// Stop crawling
				set_flag(LOW_DISK_SPACE);
				stop_crawling();

				clog << "Stopped indexing because of low disk space" << endl;
			}
			else if (m_stopIndexing == true)
			{
				// Go ahead
				m_stopIndexing = false;
				reset_flag(LOW_DISK_SPACE);

				clog << "Resumed indexing following low disk space condition" << endl;
			}
		}
#endif
#ifdef CHECK_BATTERY_SYSCTL
		// Check the battery state too
		check_battery_state();
#endif
	}

	return true;
}

void DaemonState::check_battery_state(void)
{
#ifdef CHECK_BATTERY_SYSCTL
	int acline = 1;
	size_t len = sizeof(acline);
	bool onBattery = false;

	// Are we on battery power ?
	if (sysctlbyname("hw.acpi.acline", &acline, &len, NULL, 0) == 0)
	{
#ifdef DEBUG
		clog << "DaemonState::check_battery_state: acline " << acline << endl;
#endif
		if (acline == 0)
		{
			onBattery = true;
		}

		bool wasOnBattery = is_flag_set(ON_BATTERY);
		if (onBattery != wasOnBattery)
		{
			if (onBattery == true)
			{
				// We are now on battery
				set_flag(ON_BATTERY);
				stop_crawling();

				clog << "System is now on battery" << endl;
			}
			else
			{
				// Back on-line
				reset_flag(ON_BATTERY);
				start_crawling();

				clog << "System is now on AC" << endl;
			}
		}
	}
#endif
}

bool DaemonState::crawl_location(const PinotSettings::IndexableLocation &location)
{
	CrawlerThread *pCrawlerThread = NULL;
	string locationToCrawl(location.m_name);
	bool doMonitoring = location.m_monitor;
	bool isSource = location.m_isSource;
	bool inlineIndexing = false;

	// Can we go ahead and crawl ?
	if ((is_flag_set(LOW_DISK_SPACE) == true) ||
		(is_flag_set(ON_BATTERY) == true))
	{
#ifdef DEBUG
		clog << "DaemonState::crawl_location: crawling was stopped" << endl;
#endif
		return false;
	}

	if (locationToCrawl.empty() == true)
	{
		return false;
	}

	if (m_maxIndexThreads < 2)
	{
		inlineIndexing = true;
	}

	if (doMonitoring == false)
	{
		// Monitoring is not necessary, but we still have to pass the handler
		// so that we can act on documents that have been deleted
		pCrawlerThread = new CrawlerThread(locationToCrawl, isSource,
			NULL, m_pDiskHandler, inlineIndexing);
	}
	else
	{
		pCrawlerThread = new CrawlerThread(locationToCrawl, isSource,
			m_pDiskMonitor, m_pDiskHandler, inlineIndexing);
	}
	pCrawlerThread->getFileFoundSignal().connect(sigc::mem_fun(*this, &DaemonState::on_message_filefound));

	if (start_thread(pCrawlerThread, true) == true)
	{
		++m_crawlers;
		set_flag(CRAWLING);

		return true;
	}

	return false;
}

void DaemonState::flush_and_reclaim(void)
{
	IndexInterface *pIndex = PinotSettings::getInstance().getIndex(PinotSettings::getInstance().m_daemonIndexLocation);
	if (pIndex != NULL)
	{
#ifdef HAVE_DBUS
		DBusServletThread::flushIndexAndSignal(pIndex);
#endif

		delete pIndex;
	}

	int inUse = Memory::getUsage();
	Memory::reclaim();
}

void DaemonState::start(bool isReindex)
{
	// Disable implicit flushing after a change
	WorkerThread::immediateFlush(false);

	m_isReindex = isReindex;

	// Fire up the disk monitor thread
	if (m_pDiskHandler == NULL)
	{
		OnDiskHandler *pDiskHandler = new OnDiskHandler();
		pDiskHandler->getFileFoundSignal().connect(sigc::mem_fun(*this, &DaemonState::on_message_filefound));
		m_pDiskHandler = pDiskHandler;
	}
	HistoryMonitorThread *pDiskMonitorThread = new HistoryMonitorThread(m_pDiskMonitor, m_pDiskHandler);
	start_thread(pDiskMonitorThread, true);

	for (set<PinotSettings::IndexableLocation>::const_iterator locationIter = PinotSettings::getInstance().m_indexableLocations.begin();
		locationIter != PinotSettings::getInstance().m_indexableLocations.end(); ++locationIter)
	{
		m_crawlQueue.push(*locationIter);
	}
#ifdef DEBUG
	clog << "DaemonState::start: " << m_crawlQueue.size() << " locations to crawl" << endl;
#endif

	// Update all items status so that we can get rid of files from deleted sources
	m_crawlHistory.updateItemsStatus(CrawlHistory::CRAWLING, CrawlHistory::TO_CRAWL, 0, true);
	m_crawlHistory.updateItemsStatus(CrawlHistory::CRAWLED, CrawlHistory::TO_CRAWL, 0, true);
	m_crawlHistory.updateItemsStatus(CrawlHistory::CRAWL_ERROR, CrawlHistory::TO_CRAWL, 0, true);

	// Initiate crawling
	start_crawling();
}

void DaemonState::reload(void)
{
	// Reload whenever possible
	m_reload = true;
}

bool DaemonState::start_crawling(void)
{
	bool startedCrawler = false;

	if (write_lock_lists() == true)
	{
#ifdef DEBUG
		clog << "DaemonState::start_crawling: " << m_crawlQueue.size() << " locations to crawl, "
			<< m_crawlers << " crawlers" << endl;
#endif
		// Get the next location, unless something is still being crawled
		if (m_crawlers == 0)
		{
			reset_flag(CRAWLING);

			if (m_crawlQueue.empty() == false)
			{
				PinotSettings::IndexableLocation nextLocation(m_crawlQueue.front());

				startedCrawler = crawl_location(nextLocation);
			}
			else
			{
				set<string> deletedFiles;

				// All files left with status TO_CRAWL belong to deleted sources
				if ((m_pDiskHandler != NULL) &&
					(m_crawlHistory.getItems(CrawlHistory::TO_CRAWL, deletedFiles) > 0))
				{
#ifdef DEBUG
					clog << "DaemonState::start_crawling: " << deletedFiles.size() << " orphaned files" << endl;
#endif
					for(set<string>::const_iterator fileIter = deletedFiles.begin();
						fileIter != deletedFiles.end(); ++fileIter)
					{
						// Inform the MonitorHandler
						m_pDiskHandler->fileDeleted(fileIter->substr(7));

						// Delete this item
						m_crawlHistory.deleteItem(*fileIter);
					}
				}
			}
		}

		unlock_lists();
	}

	return startedCrawler;
}

void DaemonState::stop_crawling(void)
{
	if (write_lock_threads() == true)
	{
		if (m_threads.empty() == false)
		{
			// Stop all Crawler threads
			for_each(m_threads.begin(), m_threads.end(), StopCrawlerThreadFunc());
		}

		unlock_threads();
	}
}

void DaemonState::on_thread_end(WorkerThread *pThread)
{
	string indexedUrl;
	bool emptyQueue = false;

	if (pThread == NULL)
	{
		return;
	}

	string type(pThread->getType());
	bool isStopped = pThread->isStopped();
#ifdef DEBUG
	clog << "DaemonState::on_thread_end: end of thread " << type << " " << pThread->getId() << endl;
#endif

	// What type of thread was it ?
	if (type == "CrawlerThread")
	{
		CrawlerThread *pCrawlerThread = dynamic_cast<CrawlerThread *>(pThread);
		if (pCrawlerThread == NULL)
		{
			delete pThread;
			return;
		}
		--m_crawlers;
#ifdef DEBUG
		clog << "DaemonState::on_thread_end: done crawling " << pCrawlerThread->getDirectory() << endl;
#endif

		if (isStopped == false)
		{
			// Pop the queue
			m_crawlQueue.pop();

			m_flush = true;
		}
		// Else, the directory wasn't fully crawled so better leave it in the queue

		start_crawling();
	}
	else if (type == "IndexingThread")
	{
		IndexingThread *pIndexThread = dynamic_cast<IndexingThread *>(pThread);
		if (pIndexThread == NULL)
		{
			delete pThread;
			return;
		}

		// Get the URL we have just indexed
		indexedUrl = pIndexThread->getURL();

		// Did it fail ?
		int errorNum = pThread->getErrorNum();
		if ((errorNum > 0) &&
			(indexedUrl.empty() == false))
		{
			// An entry should already exist for this
			m_crawlHistory.updateItem(indexedUrl, CrawlHistory::CRAWL_ERROR, time(NULL), errorNum);
		}
	}
	else if (type == "UnindexingThread")
	{
		// FIXME: anything to do ?
	}
	else if (type == "MonitorThread")
	{
		// FIXME: do something about this
	}
#ifdef HAVE_DBUS
	else if (type == "DBusServletThread")
	{
		DBusServletThread *pDBusThread = dynamic_cast<DBusServletThread *>(pThread);
		if (pDBusThread == NULL)
		{
			delete pThread;
			return;
		}

		// Send the reply ?
		DBusServletInfo *pInfo = pDBusThread->getServletInfo();
		if (pInfo != NULL)
		{
			if (pInfo->m_pThread != NULL)
			{
				m_servletsInfo.insert(pInfo);

				start_thread(pInfo->m_pThread);
			}
			else
			{
				pInfo->reply();

				delete pInfo;
			}
		}

		if (pDBusThread->mustQuit() == true)
		{
			// Disconnect the timeout signal
			if (m_timeoutConnection.connected() == true)
			{
				m_timeoutConnection.block();
				m_timeoutConnection.disconnect();
			}
			m_signalQuit(0);
		}
	}
#endif
	else if (type == "QueryingThread")
	{
		QueryingThread *pQueryThread = dynamic_cast<QueryingThread *>(pThread);
		if (pQueryThread == NULL)
		{
			delete pThread;
			return;
		}

		bool wasCorrected = false;
		QueryProperties queryProps(pQueryThread->getQuery(wasCorrected));
		const vector<DocumentInfo> &resultsList = pQueryThread->getDocuments();

#ifdef HAVE_DBUS
		// Find the servlet info
		for (set<DBusServletInfo *>::const_iterator servIter = m_servletsInfo.begin();
			servIter != m_servletsInfo.end(); ++servIter)
		{
			DBusServletInfo *pInfo = const_cast<DBusServletInfo *>(*servIter);

			if ((pInfo != NULL) &&
				(pInfo->m_pThread->getId() == pThread->getId()))
			{
#ifdef DEBUG
				clog << "DaemonState::on_thread_end: ran query " << queryProps.getName() << endl;
#endif
				// Prepare and send the reply
				pInfo->newQueryReply(resultsList, pQueryThread->getDocumentsCount());
				pInfo->reply();

				m_servletsInfo.erase(servIter);
				delete pInfo;

				break;
			}
		}
#endif
	}
	else if (type == "RestoreMetaDataThread")
	{
		// Do the actual flush here
		flush_and_reclaim();
	}

	// Delete the thread
	delete pThread;

	// Wait until there are no threads running (except background ones)
	// to reload the configuration
	if ((m_reload == true) &&
		(get_threads_count() == 0))
	{
#ifdef DEBUG
		clog << "DaemonState::on_thread_end: stopping all threads" << endl;
#endif
		// Stop background threads
		stop_threads();
		// ...clear the queues
		clear_queues();

		// Reload
		PinotSettings &settings = PinotSettings::getInstance();
		settings.clear();
		settings.load(PinotSettings::LOAD_ALL);
		m_reload = false;

		// ...and restart everything 
		start(false);
	}

	// Try to run a queued action unless threads were stopped
	if (isStopped == false)
	{
		emptyQueue = pop_queue(indexedUrl);
	}

	// Wait until there are no threads running (except background ones)
	// and the queue is empty to flush the index
	if ((m_flush == true) &&
		(emptyQueue == true) &&
		(get_threads_count() == 0))
	{
		m_flush = false;

		if ((m_isReindex == true) &&
			(m_crawlQueue.empty() == true))
		{
			// Restore metadata on documents and flush when the tread returns
			RestoreMetaDataThread *pRestoreThread = new RestoreMetaDataThread();
			start_thread(pRestoreThread);
		}
		else
		{
			// Flush now
			flush_and_reclaim();
		}
	}
}

void DaemonState::on_message_filefound(DocumentInfo docInfo, bool isDirectory)
{
	if (isDirectory == false)
	{
		queue_index(docInfo);
	}
	else
	{
		PinotSettings::IndexableLocation newLocation;

		newLocation.m_monitor = true;
		newLocation.m_name = docInfo.getLocation().substr(7);
		newLocation.m_isSource = false;
#ifdef DEBUG
		clog << "DaemonState::on_message_filefound: new directory " << newLocation.m_name << endl;
#endif

		// Queue this directory for crawling
		m_crawlQueue.push(newLocation);
		start_crawling();
	}
}

sigc::signal1<void, int>& DaemonState::getQuitSignal(void)
{
	return m_signalQuit;
}

void DaemonState::set_flag(StatusFlag flag)
{
	FD_SET((int)flag, &m_flagsSet);
}

bool DaemonState::is_flag_set(StatusFlag flag)
{
	if (FD_ISSET((int)flag, &m_flagsSet))
	{
		return true;
	}

	return false;
}

void DaemonState::reset_flag(StatusFlag flag)
{
	FD_CLR((int)flag, &m_flagsSet);
}

