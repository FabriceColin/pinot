/*
 *  Copyright 2005-2011 Fabrice Colin
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
#include <glibmm/miscutils.h>

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

static void updateLabels(unsigned int docId, MetaDataBackup &metaData,
	IndexInterface *pIndex, set<string> &labels, gboolean resetLabels)
{
	DocumentInfo docInfo;

	if (pIndex == NULL)
	{
		return;
	}

	// If it's a reset, remove labels from the metadata backup
	if ((resetLabels == TRUE) &&
		(pIndex->getDocumentInfo(docId, docInfo) == true))
	{
		metaData.deleteItem(docInfo, DocumentInfo::SERIAL_LABELS);
	}

	// Get the current labels 
	if (resetLabels == TRUE)
	{
		labels.clear();
		pIndex->getDocumentLabels(docId, labels);
	}
	docInfo.setLabels(labels);
	metaData.addItem(docInfo, DocumentInfo::SERIAL_LABELS);
}

static ustring g_xmlDescription;

static bool loadXMLDescription(void)
{
	bool readFile = false;

	if (g_xmlDescription.empty() == false)
	{
		return true;
	}

	ifstream xmlFile;
	string xmlFileName(PREFIX);

	xmlFileName += "/share/pinot/pinot-dbus-daemon.xml";
	xmlFile.open(xmlFileName.c_str());
	if (xmlFile.good() == true)
	{
		xmlFile.seekg(0, ios::end);
		int length = xmlFile.tellg();
		xmlFile.seekg(0, ios::beg);

		char *pXmlBuffer = new char[length + 1];
		xmlFile.read(pXmlBuffer, length);
		if (xmlFile.fail() == false)
		{
			pXmlBuffer[length] = '\0';
			g_xmlDescription = pXmlBuffer;
			readFile = true;
		}
		delete[] pXmlBuffer;
	}
	xmlFile.close();

	if (readFile == false)
	{
		clog << "File " << xmlFileName << " couldn't be read" << endl;
	}
	return readFile;
}

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
	}
	else
	{
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

	return m_crawlHistory.hasItem(location, itemStatus, itemDate);
}

void CrawlerThread::recordCrawling(const string &location, bool itemExists, time_t &itemDate)
{
	if (itemExists == false)
	{
		// Record it
		m_crawlHistory.insertItem(location, CrawlHistory::CRAWLING, m_sourceId, itemDate);
	}
	else
	{
		// Change the status from TO_CRAWL to CRAWLING
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
	Timer scanTimer;
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

#ifdef HAVE_DBUS
DBusGConnection *DBusServletThread::m_pSystemBus = NULL;
DBusGConnection *DBusServletThread::m_pSessionBus = NULL;

DBusServletThread::DBusServletThread(DaemonState *pServer, DBusServletInfo *pInfo) :
	WorkerThread(),
	m_pServer(pServer),
	m_pServletInfo(pInfo),
	m_mustQuit(false)
{
}

DBusServletThread::~DBusServletThread()
{
}

void DBusServletThread::flushIndexAndSignal(IndexInterface *pIndex)
{
	if (pIndex == NULL)
	{
		return;
	}
#ifdef DEBUG
	clog << "DBusServletThread::flushIndexAndSignal: flushing" << endl;
#endif

	// Flush
	pIndex->flush();

	// Signal
	if (m_pSessionBus != NULL)
	{
		DBusMessage *pMessage = dbus_message_new_signal(PINOT_DBUS_OBJECT_PATH,
			PINOT_DBUS_SERVICE_NAME, "IndexFlushed");
		unsigned int docsCount = pIndex->getDocumentsCount();

		dbus_message_append_args(pMessage,
			DBUS_TYPE_UINT32, &docsCount,
			DBUS_TYPE_INVALID);
		DBusConnection *pConnection = dbus_g_connection_get_connection(m_pSessionBus);
		if (pConnection != NULL)
		{
			dbus_connection_send(pConnection, pMessage, NULL);
#ifdef DEBUG
			clog << "DBusServletThread::flushIndexAndSignal: sent signal IndexFlushed " << docsCount << endl;
#endif
		}
		dbus_message_unref(pMessage);
	}
#ifdef DEBUG
	else clog << "DBusServletThread::flushIndexAndSignal: no bus" << endl;
#endif
}

string DBusServletThread::getType(void) const
{
	return "DBusServletThread";
}

DBusServletInfo *DBusServletThread::getServletInfo(void) const
{
	return m_pServletInfo;
}

bool DBusServletThread::mustQuit(void) const
{
	return m_mustQuit;
}

void DBusServletThread::doWork(void)
{
	PinotSettings &settings = PinotSettings::getInstance();
	IndexInterface *pIndex = settings.getIndex(settings.m_daemonIndexLocation);
	MetaDataBackup metaData(settings.getHistoryDatabaseName());
	DBusError error;
	bool updateLabelsCache = false, flushIndex = false;

	if ((m_pServer == NULL) ||
		(m_pServletInfo == NULL) ||
		(pIndex == NULL))
	{
		return;
	}

	dbus_error_init(&error);

	// Access the settings' labels list directly
	set<string> &labelsCache = settings.m_labels;
	if (labelsCache.empty() == true)
	{
		pIndex->getLabels(labelsCache);
	}

#ifdef DEBUG
	const char *pSender = dbus_message_get_sender(m_pServletInfo->m_pRequest);
	if (pSender != NULL)
	{
		clog << "DBusServletThread::doWork: called by " << pSender << endl;
	}
	else
	{
		clog << "DBusServletThread::doWork: called by unknown sender" << endl;
	}
#endif

	if (dbus_message_is_method_call(m_pServletInfo->m_pRequest, PINOT_DBUS_SERVICE_NAME, "GetStatistics") == TRUE)
	{
		CrawlHistory crawlHistory(settings.getHistoryDatabaseName());
		unsigned int crawledFilesCount = crawlHistory.getItemsCount(CrawlHistory::CRAWLED);
		unsigned int docsCount = pIndex->getDocumentsCount();
		gboolean lowDiskSpace = FALSE, onBattery = FALSE, crawling = FALSE;

#ifdef DEBUG
		clog << "DBusServletThread::doWork: received GetStatistics" << endl;
#endif
		// Prepare the reply
		if (m_pServletInfo->newReply() == true)
		{
			if (m_pServer->is_flag_set(DaemonState::LOW_DISK_SPACE) == true)
			{
				lowDiskSpace = TRUE;
			}
			if (m_pServer->is_flag_set(DaemonState::ON_BATTERY) == true)
			{
				onBattery = TRUE;
			}
			if (m_pServer->is_flag_set(DaemonState::CRAWLING) == true)
			{
				crawling = TRUE;
			}
#ifdef DEBUG
			clog << "DBusServletThread::doWork: replying with " << crawledFilesCount
				<< " " << docsCount << " " << lowDiskSpace << onBattery << crawling << endl;
#endif

			dbus_message_append_args(m_pServletInfo->m_pReply,
				DBUS_TYPE_UINT32, &crawledFilesCount,
				DBUS_TYPE_UINT32, &docsCount,
				DBUS_TYPE_BOOLEAN, &lowDiskSpace,
				DBUS_TYPE_BOOLEAN, &onBattery,
				DBUS_TYPE_BOOLEAN, &crawling,
				DBUS_TYPE_INVALID);
		}
	}
	else if (dbus_message_is_method_call(m_pServletInfo->m_pRequest, PINOT_DBUS_SERVICE_NAME, "Reload") == TRUE)
	{
		if (dbus_message_get_args(m_pServletInfo->m_pRequest, &error,
			DBUS_TYPE_INVALID) == TRUE)
		{
			gboolean reloading = TRUE;

#ifdef DEBUG
			clog << "DBusServletThread::doWork: received Reload" << endl;
#endif
			m_pServer->reload();

			// Prepare the reply
			if (m_pServletInfo->newReply() == true)
			{
				dbus_message_append_args(m_pServletInfo->m_pReply,
					DBUS_TYPE_BOOLEAN, &reloading,
					DBUS_TYPE_INVALID);
			}
		}
	}
	else if (dbus_message_is_method_call(m_pServletInfo->m_pRequest, PINOT_DBUS_SERVICE_NAME, "Stop") == TRUE)
	{
		if (dbus_message_get_args(m_pServletInfo->m_pRequest, &error,
			DBUS_TYPE_INVALID) == TRUE)
		{
			int exitStatus = EXIT_SUCCESS;

#ifdef DEBUG
			clog << "DBusServletThread::doWork: received Stop" << endl;
#endif
			m_pServer->set_flag(DaemonState::STOPPED);

			// Prepare the reply
			if (m_pServletInfo->newReply() == true)
			{
				dbus_message_append_args(m_pServletInfo->m_pReply,
					DBUS_TYPE_INT32, &exitStatus,
					DBUS_TYPE_INVALID);
			}

			m_mustQuit = true;
		}
	}
	else if (dbus_message_is_method_call(m_pServletInfo->m_pRequest, PINOT_DBUS_SERVICE_NAME, "HasDocument") == TRUE)
	{
		char *pUrl = NULL;
		unsigned int docId = 0;

		if (dbus_message_get_args(m_pServletInfo->m_pRequest, &error,
			DBUS_TYPE_STRING, &pUrl,
			DBUS_TYPE_INVALID) == TRUE)
		{
#ifdef DEBUG
			clog << "DBusServletThread::doWork: received HasDocument " << pUrl << endl;
#endif
			if (pUrl != NULL)
			{
				string url(pUrl);

				// Check the index
				docId = pIndex->hasDocument(url);
			}

			// Prepare the reply
			if (m_pServletInfo->newReply() == true)
			{
				dbus_message_append_args(m_pServletInfo->m_pReply,
					DBUS_TYPE_UINT32, &docId,
					DBUS_TYPE_INVALID);
			}
		}
	}
	else if (dbus_message_is_method_call(m_pServletInfo->m_pRequest, PINOT_DBUS_SERVICE_NAME, "GetLabels") == TRUE)
	{
#ifdef DEBUG
		clog << "DBusServletThread::doWork: received GetLabels" << endl;
#endif
		// This method doesn't take any argument
		m_pServletInfo->m_pArray = g_ptr_array_new();

		for (set<string>::const_iterator labelIter = labelsCache.begin();
			labelIter != labelsCache.end(); ++labelIter)
		{
			string labelName(*labelIter);

			g_ptr_array_add(m_pServletInfo->m_pArray, const_cast<char*>(labelName.c_str()));
		}

		// Prepare the reply
		m_pServletInfo->newReplyWithArray();
	}
	else if (dbus_message_is_method_call(m_pServletInfo->m_pRequest, PINOT_DBUS_SERVICE_NAME, "AddLabel") == TRUE)
	{
		char *pLabel = NULL;

		if (dbus_message_get_args(m_pServletInfo->m_pRequest, &error,
			DBUS_TYPE_STRING, &pLabel,
			DBUS_TYPE_INVALID) == TRUE)
		{
#ifdef DEBUG
			clog << "DBusServletThread::doWork: received AddLabel " << pLabel << endl;
#endif
			if (pLabel != NULL)
			{
				string labelName(pLabel);

				// Add the label
				flushIndex = pIndex->addLabel(labelName);
				// Is this a known label ?
				if (labelsCache.find(labelName) == labelsCache.end())
				{
					// No, it isn't but that's okay
					labelsCache.insert(labelName);
					updateLabelsCache = true;
				}
			}

			// Prepare the reply
			if (m_pServletInfo->newReply() == true)
			{
				dbus_message_append_args(m_pServletInfo->m_pReply,
					DBUS_TYPE_STRING, &pLabel,
					DBUS_TYPE_INVALID);
			}
		}
	}
	else if (dbus_message_is_method_call(m_pServletInfo->m_pRequest, PINOT_DBUS_SERVICE_NAME, "RenameLabel") == TRUE)
	{
		char *pOldLabel = NULL;
		char *pNewLabel = NULL;

		if (dbus_message_get_args(m_pServletInfo->m_pRequest, &error,
			DBUS_TYPE_STRING, &pOldLabel,
			DBUS_TYPE_STRING, &pNewLabel,
			DBUS_TYPE_INVALID) == TRUE)
		{
			// Nothing to do, this was obsoleted
#ifdef DEBUG
			clog << "DBusServletThread::doWork: received RenameLabel " << pOldLabel << ", " << pNewLabel << endl;
#endif
			
			// Prepare the reply
			if (m_pServletInfo->newReply() == true)
			{
				dbus_message_append_args(m_pServletInfo->m_pReply,
					DBUS_TYPE_STRING, &pNewLabel,
					DBUS_TYPE_INVALID);
			}
		}
	}
	else if (dbus_message_is_method_call(m_pServletInfo->m_pRequest, PINOT_DBUS_SERVICE_NAME, "DeleteLabel") == TRUE)
	{
		char *pLabel = NULL;

		if (dbus_message_get_args(m_pServletInfo->m_pRequest, &error,
			DBUS_TYPE_STRING, &pLabel,
			DBUS_TYPE_INVALID) == TRUE)
		{
#ifdef DEBUG
			clog << "DBusServletThread::doWork: received DeleteLabel " << pLabel << endl;
#endif
			if (pLabel != NULL)
			{
				// Delete the label
				flushIndex = pIndex->deleteLabel(pLabel);
				// Update the labels list
				set<string>::const_iterator labelIter = labelsCache.find(pLabel);
				if (labelIter != labelsCache.end())
				{
					labelsCache.erase(labelIter);
					updateLabelsCache = true;
				}

				// Update the metadata backup
				metaData.deleteLabel(pLabel);
			}

			// Prepare the reply
			if (m_pServletInfo->newReply() == true)
			{
				dbus_message_append_args(m_pServletInfo->m_pReply,
					DBUS_TYPE_STRING, &pLabel,
					DBUS_TYPE_INVALID);
			}
		}
	}
	else if (dbus_message_is_method_call(m_pServletInfo->m_pRequest, PINOT_DBUS_SERVICE_NAME, "GetDocumentLabels") == TRUE)
	{
		unsigned int docId = 0;

		if (dbus_message_get_args(m_pServletInfo->m_pRequest, &error,
			DBUS_TYPE_UINT32, &docId,
			DBUS_TYPE_INVALID) == TRUE)
		{
			set<string> labels;

#ifdef DEBUG
			clog << "DBusServletThread::doWork: received GetDocumentLabels " << docId << endl;
#endif
			if (pIndex->getDocumentLabels(docId, labels) == true)
			{
				m_pServletInfo->m_pArray = g_ptr_array_new();

				for (set<string>::const_iterator labelIter = labels.begin();
					labelIter != labels.end(); ++labelIter)
				{
					string labelName(*labelIter);

					g_ptr_array_add(m_pServletInfo->m_pArray, const_cast<char*>(labelName.c_str()));
				}

				// Prepare the reply
				m_pServletInfo->newReplyWithArray();
			}
			else
			{
				m_pServletInfo->newErrorReply("GetDocumentLabels",
					" failed");
			}
		}
	}
	else if (dbus_message_is_method_call(m_pServletInfo->m_pRequest, PINOT_DBUS_SERVICE_NAME, "SetDocumentLabels") == TRUE)
	{
		char **ppLabels = NULL;
		dbus_uint32_t labelsCount = 0;
		unsigned int docId = 0;
		gboolean resetLabels = TRUE;

		if (dbus_message_get_args(m_pServletInfo->m_pRequest, &error,
			DBUS_TYPE_UINT32, &docId,
			DBUS_TYPE_ARRAY, DBUS_TYPE_STRING, &ppLabels, &labelsCount,
			DBUS_TYPE_BOOLEAN, &resetLabels,
			DBUS_TYPE_INVALID) == TRUE)
		{
			set<string> labels;

			for (dbus_uint32_t labelIndex = 0; labelIndex < labelsCount; ++labelIndex)
			{
				if (ppLabels[labelIndex] == NULL)
				{
					break;
				}

				string labelName(ppLabels[labelIndex]);
				labels.insert(labelName);
				// Is this a known label ?
				if (labelsCache.find(labelName) == labelsCache.end())
				{
					// No, it isn't but that's okay
					labelsCache.insert(labelName);
					updateLabelsCache = true;
				}
			}
#ifdef DEBUG
			clog << "DBusServletThread::doWork: received SetDocumentLabels on ID " << docId
				<< ", " << labelsCount << " labels" << ", " << resetLabels << endl;
#endif

			// Set labels
			flushIndex = pIndex->setDocumentLabels(docId, labels, ((resetLabels == TRUE) ? true : false));

			// Update the metadata backup
			updateLabels(docId, metaData, pIndex, labels, resetLabels);

			// Free container types
			g_strfreev(ppLabels);

			// Prepare the reply
			if (m_pServletInfo->newReply() == true)
			{
				dbus_message_append_args(m_pServletInfo->m_pReply,
					DBUS_TYPE_UINT32, &docId,
					DBUS_TYPE_INVALID);
			}
		}
	}
	else if (dbus_message_is_method_call(m_pServletInfo->m_pRequest, PINOT_DBUS_SERVICE_NAME, "SetDocumentsLabels") == TRUE)
	{
		char **ppDocIds = NULL;
		char **ppLabels = NULL;
		dbus_uint32_t idsCount = 0;
		dbus_uint32_t labelsCount = 0;
		gboolean resetLabels = TRUE;

		if (dbus_message_get_args(m_pServletInfo->m_pRequest, &error,
			DBUS_TYPE_ARRAY, DBUS_TYPE_STRING, &ppDocIds, &idsCount,
			DBUS_TYPE_ARRAY, DBUS_TYPE_STRING, &ppLabels, &labelsCount,
			DBUS_TYPE_BOOLEAN, &resetLabels,
			DBUS_TYPE_INVALID) == TRUE)
		{
			set<unsigned int> docIds;
			set<string> labels;

			for (dbus_uint32_t idIndex = 0; idIndex < idsCount; ++idIndex)
			{
				if (ppDocIds[idIndex] == NULL)
				{
					break;
				}

				docIds.insert((unsigned int)atoi(ppDocIds[idIndex]));
			}
			for (dbus_uint32_t labelIndex = 0; labelIndex < labelsCount; ++labelIndex)
			{
				if (ppLabels[labelIndex] == NULL)
				{
					break;
				}

				string labelName(ppLabels[labelIndex]);
				labels.insert(labelName);
				// Is this a known label ?
				if (labelsCache.find(labelName) == labelsCache.end())
				{
					// No, it isn't but that's okay
					labelsCache.insert(labelName);
					updateLabelsCache = true;
				}
			}
#ifdef DEBUG
			clog << "DBusServletThread::doWork: received SetDocumentsLabels on " << docIds.size()
				<< " IDs, " << labelsCount << " labels" << ", " << resetLabels << endl;
#endif
			// Set labels
			if (pIndex->setDocumentsLabels(docIds, labels, ((resetLabels == TRUE) ? true : false)) == true)
			{
				resetLabels = TRUE;
				flushIndex = true;
			}

			// Update the metadata backup
			for (set<unsigned int>::const_iterator docIter = docIds.begin();
				docIter != docIds.end(); ++docIter)
			{
				updateLabels(*docIter, metaData, pIndex, labels, resetLabels);
			}

			// Free container types
			g_strfreev(ppDocIds);
			g_strfreev(ppLabels);

			// Prepare the reply
			if (m_pServletInfo->newReply() == true)
			{
				dbus_message_append_args(m_pServletInfo->m_pReply,
					DBUS_TYPE_BOOLEAN, &resetLabels,
					DBUS_TYPE_INVALID);
			}
		}
	}
	else if (dbus_message_is_method_call(m_pServletInfo->m_pRequest, PINOT_DBUS_SERVICE_NAME, "GetDocumentInfo") == TRUE)
	{
		unsigned int docId = 0;

		if (dbus_message_get_args(m_pServletInfo->m_pRequest, &error,
			DBUS_TYPE_UINT32, &docId,
			DBUS_TYPE_INVALID) == TRUE)
		{
			DocumentInfo docInfo;

#ifdef DEBUG
			clog << "DBusServletThread::doWork: received GetDocumentInfo on " << docId << endl;
#endif
			if (pIndex->getDocumentInfo(docId, docInfo) == true)
			{
				// Prepare the reply
				if (m_pServletInfo->newReply() == true)
				{
					DBusMessageIter iter;

					dbus_message_iter_init_append(m_pServletInfo->m_pReply, &iter);
					if (DBusIndex::documentInfoToDBus(&iter, 0, docInfo) == false)
					{
						dbus_message_unref(m_pServletInfo->m_pReply);
						m_pServletInfo->newErrorReply("GetDocumentInfo",
							"Unknown error");
					}
				}
			}
			else
			{
				m_pServletInfo->newErrorReply("GetDocumentInfo",
					"Unknown document");
			}
		}
	}
	else if (dbus_message_is_method_call(m_pServletInfo->m_pRequest, PINOT_DBUS_SERVICE_NAME, "SetDocumentInfo") == TRUE)
	{
		DBusMessageIter iter;
		DocumentInfo docInfo;
		unsigned int docId = 0;

		dbus_message_iter_init(m_pServletInfo->m_pRequest, &iter);
		if (DBusIndex::documentInfoFromDBus(&iter, docId, docInfo) == false)
		{
			m_pServletInfo->newErrorReply("SetDocumentInfo",
				"Unknown error");
		}
		else
		{
#ifdef DEBUG
			clog << "DBusServletThread::doWork: received SetDocumentInfo on " << docId << endl;
#endif

			// Update the document info
			flushIndex = pIndex->updateDocumentInfo(docId, docInfo);

			// Update the metadata backup
			metaData.addItem(docInfo, DocumentInfo::SERIAL_FIELDS);

			// Prepare the reply
			if (m_pServletInfo->newReply() == true)
			{
				dbus_message_append_args(m_pServletInfo->m_pReply,
					DBUS_TYPE_UINT32, &docId,
					DBUS_TYPE_INVALID);
			}
		}
	}
	else if (dbus_message_is_method_call(m_pServletInfo->m_pRequest, PINOT_DBUS_SERVICE_NAME, "Query") == TRUE)
	{
		char *pSearchText = NULL;
		char *pEngineType = NULL;
		char *pEngineOption = NULL;
		dbus_uint32_t startDoc = 0, maxHits = 0;

		if (dbus_message_get_args(m_pServletInfo->m_pRequest, &error,
			DBUS_TYPE_STRING, &pEngineType,
			DBUS_TYPE_STRING, &pEngineOption,
			DBUS_TYPE_STRING, &pSearchText,
			DBUS_TYPE_UINT32, &startDoc,
			DBUS_TYPE_UINT32, &maxHits,
			DBUS_TYPE_INVALID) == TRUE)
		{
			bool replyWithError = true;

#ifdef DEBUG
			clog << "DBusServletThread::doWork: received Query " << pSearchText << ", " << startDoc << "/" << maxHits << endl;
#endif
			if (pSearchText != NULL)
			{
				stringstream queryNameStr;

				// Give the query a unique name
				queryNameStr << "DBUS" << m_id;
				m_pServletInfo->m_simpleQuery = false;

				QueryProperties queryProps(queryNameStr.str(), pSearchText);
				queryProps.setMaximumResultsCount(maxHits);

				string engineType, engineOption;

				// Provide reasonable defaults 
				if (((pEngineType == NULL) || (strlen(pEngineType) == 0)) &&
					((pEngineOption == NULL) || (strlen(pEngineOption) == 0)))
				{
					engineType = settings.m_defaultBackend;
					engineOption = settings.m_daemonIndexLocation;
				}
				else
				{
					engineType = pEngineType;
					engineOption = pEngineOption;
				}

				m_pServletInfo->m_pThread = new EngineQueryThread(engineType,
					engineType, engineOption, queryProps, startDoc);
			}

			if (replyWithError == true)
			{
				m_pServletInfo->newErrorReply("SimpleQuery",
					"Query failed");
			}
		}
	}
	// FIXME: this method will soon be obsoleted
	else if (dbus_message_is_method_call(m_pServletInfo->m_pRequest, PINOT_DBUS_SERVICE_NAME, "SimpleQuery") == TRUE)
	{
		char *pSearchText = NULL;
		dbus_uint32_t maxHits = 0;

		if (dbus_message_get_args(m_pServletInfo->m_pRequest, &error,
			DBUS_TYPE_STRING, &pSearchText,
			DBUS_TYPE_UINT32, &maxHits,
			DBUS_TYPE_INVALID) == TRUE)
		{
			bool replyWithError = true;

#ifdef DEBUG
			clog << "DBusServletThread::doWork: received SimpleQuery " << pSearchText << ", " << maxHits << endl;
#endif
			if (pSearchText != NULL)
			{
				stringstream queryNameStr;

				// Give the query a unique name
				queryNameStr << "DBUS" << m_id;
				m_pServletInfo->m_simpleQuery = true;

				QueryProperties queryProps(queryNameStr.str(), pSearchText);
				queryProps.setMaximumResultsCount(maxHits);

				m_pServletInfo->m_pThread = new EngineQueryThread(settings.m_defaultBackend,
					settings.m_defaultBackend, settings.m_daemonIndexLocation,
					queryProps, 0);
			}

			if (replyWithError == true)
			{
				m_pServletInfo->newErrorReply("SimpleQuery",
					"Query failed");
			}
		}
	}
	else if (dbus_message_is_method_call(m_pServletInfo->m_pRequest, PINOT_DBUS_SERVICE_NAME, "UpdateDocument") == TRUE)
	{
		unsigned int docId = 0;

		if (dbus_message_get_args(m_pServletInfo->m_pRequest, &error,
			DBUS_TYPE_UINT32, &docId,
			DBUS_TYPE_INVALID) == TRUE)
		{
			DocumentInfo docInfo;

#ifdef DEBUG
			clog << "DBusServletThread::doWork: received UpdateDocument " << docId << endl;
#endif
			if (pIndex->getDocumentInfo(docId, docInfo) == true)
			{
				// Update document
				m_pServer->queue_index(docInfo);
			}

			// Prepare the reply
			if (m_pServletInfo->newReply() == true)
			{
				dbus_message_append_args(m_pServletInfo->m_pReply,
					DBUS_TYPE_UINT32, &docId,
					DBUS_TYPE_INVALID);
			}
		}
	}
	else if (dbus_message_is_method_call(m_pServletInfo->m_pRequest, "org.freedesktop.DBus.Introspectable", "Introspect") == TRUE)
	{
#ifdef DEBUG
		clog << "DBusServletThread::doWork: received Introspect" << endl;
#endif
		if (loadXMLDescription() == true)
		{
			// Prepare the reply
			if (m_pServletInfo->newReply() == true)
			{
				const char *pXmlData = g_xmlDescription.c_str();

				dbus_message_append_args(m_pServletInfo->m_pReply,
					DBUS_TYPE_STRING, &pXmlData,
					DBUS_TYPE_INVALID);
			}
		}
	}
	else
	{
#ifdef DEBUG
		clog << "DBusServletThread::doWork: foreign message for/from " << dbus_message_get_interface(m_pServletInfo->m_pRequest)
			<< " " << dbus_message_get_member(m_pServletInfo->m_pRequest) << endl;
#endif
	}

	// Did an error occur ?
	if (error.message != NULL)
	{
#ifdef DEBUG
		clog << "DBusServletThread::doWork: error occurred: " << error.message << endl;
#endif
		// Use the error message as reply
		m_pServletInfo->newErrorReply(error.name, error.message);
	}

	dbus_error_free(&error);

	// Set labels ?
	if ((updateLabelsCache == true) &&
		(pIndex->setLabels(labelsCache, false) == false))
	{
		// Updating failed... reset the cache
		labelsCache.clear();
		pIndex->getLabels(labelsCache);
#ifdef DEBUG
		clog << "DBusServletThread::doWork: failed to update labels" << endl;
#endif
	}

	// Flush the index ?
	if (flushIndex == true)
	{
		flushIndexAndSignal(pIndex);
	}

	delete pIndex;
}
#endif

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
	Timer restoreTimer;
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

