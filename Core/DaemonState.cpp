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
#include <fstream>
#include <algorithm>
#include <map>
#include <utility>
#include <glibmm/ustring.h>
#include <glibmm/stringutils.h>
#include <glibmm/convert.h>
#include <glibmm/thread.h>
#include <glibmm/random.h>

#include "Memory.h"
#include "Url.h"
#include "MonitorFactory.h"
#include "CrawlHistory.h"
#include "MetaDataBackup.h"
#ifdef HAVE_DBUS
#include "DBusIndex.h"
#endif
#include "DaemonState.h"
#include "OnDiskHandler.h"
#include "PinotSettings.h"
#ifdef HAVE_DBUS
#include "DBusServerThreads.h"
#endif
#include "ServerThreads.h"

#define POWER_DBUS_SERVICE_NAME "org.freedesktop.UPower"
#define POWER_DBUS_OBJECT_PATH "/org/freedesktop/UPower"

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

static string loadXMLDescription(void)
{
	ifstream xmlFile;
	string xmlFileName(PREFIX);
	ustring xmlDescription;
	bool readFile = false;

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
			xmlDescription = pXmlBuffer;
			readFile = true;
		}
		delete[] pXmlBuffer;
	}
	xmlFile.close();

	if (readFile == false)
	{
		clog << "File " << xmlFileName << " couldn't be read" << endl;
	}

	return xmlDescription;
}

static void updateLabels(unsigned int docId, MetaDataBackup &metaData,
	IndexInterface *pIndex, set<string> &labels, bool resetLabels)
{
	DocumentInfo docInfo;

	if (pIndex == NULL)
	{
		return;
	}

	// If it's a reset, remove labels from the metadata backup
	if ((resetLabels == true) &&
		(pIndex->getDocumentInfo(docId, docInfo) == true))
	{
		metaData.deleteItem(docInfo, DocumentInfo::SERIAL_LABELS);
	}

	// Get the current labels
	if (resetLabels == true)
	{
		labels.clear();
		pIndex->getDocumentLabels(docId, labels);
	}
	docInfo.setLabels(labels);
	metaData.addItem(docInfo, DocumentInfo::SERIAL_LABELS);
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
DaemonState::DBusIntrospectHandler::DBusIntrospectHandler() :
	IntrospectableStub()
{
}

DaemonState::DBusIntrospectHandler::~DBusIntrospectHandler()
{
}

void DaemonState::DBusIntrospectHandler::Introspect(IntrospectableStub::MethodInvocation &invocation)
{
	ustring xmlDescription(loadXMLDescription());

#ifdef DEBUG
	clog << "DaemonState::DBusIntrospectHandler::Introspect: called" << endl;
#endif
	invocation.ret(xmlDescription);
}

DaemonState::DBusMessageHandler::DBusMessageHandler(DaemonState *pServer) :
	PinotStub(),
	m_pServer(pServer),
	m_mustQuit(false)
{
}

DaemonState::DBusMessageHandler::~DBusMessageHandler()
{
}

bool DaemonState::DBusMessageHandler::mustQuit(void) const
{
	return m_mustQuit;
}

void DaemonState::DBusMessageHandler::emit_IndexFlushed(unsigned int docsCount)
{
	vector<ustring> busNames;

	// Emit to all listeners, not just PINOT_DBUS_SERVICE_NAME
	IndexFlushed_emitter(busNames, docsCount);
}

void DaemonState::DBusMessageHandler::flushIndexAndSignal(IndexInterface *pIndex)
{
#ifdef DEBUG
	clog << "DaemonState::DBusMessageHandler::flushIndexAndSignal: called" << endl;
#endif
	if (pIndex != NULL)
	{
		pIndex->flush();
	}

	// Signal
	emit_IndexFlushed(pIndex->getDocumentsCount());
}

void DaemonState::DBusMessageHandler::GetStatistics(PinotStub::MethodInvocation &invocation)
{
	PinotSettings &settings = PinotSettings::getInstance();
	IndexInterface *pIndex = settings.getIndex(settings.m_daemonIndexLocation);
	CrawlHistory crawlHistory(settings.getHistoryDatabaseName());
	unsigned int crawledFilesCount = crawlHistory.getItemsCount(CrawlHistory::CRAWLED);
	unsigned int docsCount = pIndex->getDocumentsCount();
	bool lowDiskSpace = false, onBattery = false, crawling = false;

#ifdef DEBUG
	clog << "DaemonState::DBusMessageHandler::GetStatistics: called" << endl;
#endif
	if (pIndex == NULL)
	{
		Gio::DBus::Error error(Gio::DBus::Error::FAILED, "Couldn't open index");

		invocation.ret(error);

		return;
	}

	if (m_pServer->is_flag_set(DaemonState::LOW_DISK_SPACE) == true)
	{
		lowDiskSpace = true;
	}
	if (m_pServer->is_flag_set(DaemonState::ON_BATTERY) == true)
	{
		onBattery = true;
	}
	if (m_pServer->is_flag_set(DaemonState::CRAWLING) == true)
	{
		crawling = true;
	}
#ifdef DEBUG
	clog << "DaemonState::DBusMessageHandler::GetStatistics: replying with " << crawledFilesCount
		<< " " << docsCount << " " << lowDiskSpace << onBattery << crawling << endl;
#endif

	invocation.ret(crawledFilesCount,
		docsCount,
		lowDiskSpace,
		onBattery,
		crawling);

	delete pIndex;
}

void DaemonState::DBusMessageHandler::Reload(PinotStub::MethodInvocation &invocation)
{
#ifdef DEBUG
	clog << "DaemonState::DBusMessageHandler::Reload: called" << endl;
#endif
	m_pServer->reload();

	invocation.ret(true);
}

void DaemonState::DBusMessageHandler::Stop(PinotStub::MethodInvocation &invocation)
{
#ifdef DEBUG
	clog << "DaemonState::DBusMessageHandler::Stop: called" << endl;
#endif
	m_pServer->set_flag(DaemonState::STOPPED);

	invocation.ret(EXIT_SUCCESS);

	m_mustQuit = true;
}

void DaemonState::DBusMessageHandler::HasDocument(const ustring &url,
	PinotStub::MethodInvocation &invocation)
{
	PinotSettings &settings = PinotSettings::getInstance();
	IndexInterface *pIndex = settings.getIndex(settings.m_daemonIndexLocation);

#ifdef DEBUG
	clog << "DaemonState::DBusMessageHandler::HasDocument: called on " << url << endl;
#endif
	if (pIndex == NULL)
	{
		Gio::DBus::Error error(Gio::DBus::Error::FAILED, "Couldn't open index");

		invocation.ret(error);

		return;
	}

	// Check the index
	unsigned int docId = pIndex->hasDocument(url);

	if (docId > 0)
	{
		invocation.ret(docId);
	}
	else
	{
		Gio::DBus::Error error(Gio::DBus::Error::FAILED, "Unknown document");

		invocation.ret(error);
	}

	delete pIndex;
}

void DaemonState::DBusMessageHandler::GetLabels(PinotStub::MethodInvocation &invocation)
{
	PinotSettings &settings = PinotSettings::getInstance();
	IndexInterface *pIndex = settings.getIndex(settings.m_daemonIndexLocation);
	set<string> &labelsCache = settings.m_labels;
	vector<ustring> labelsList;

#ifdef DEBUG
	clog << "DaemonState::DBusMessageHandler::GetLabels: called" << endl;
#endif
	if (pIndex == NULL)
	{
		Gio::DBus::Error error(Gio::DBus::Error::FAILED, "Couldn't open index");

		invocation.ret(error);

		return;
	}

	if (labelsCache.empty() == true)
	{
		pIndex->getLabels(labelsCache);
	}

	for (set<string>::const_iterator labelIter = labelsCache.begin();
		labelIter != labelsCache.end(); ++labelIter)
	{
		labelsList.push_back(labelIter->c_str());
	}

	invocation.ret(labelsList);

	delete pIndex;
}

void DaemonState::DBusMessageHandler::AddLabel(const ustring &label,
	PinotStub::MethodInvocation &invocation)
{
	PinotSettings &settings = PinotSettings::getInstance();
	IndexInterface *pIndex = settings.getIndex(settings.m_daemonIndexLocation);
	set<string> &labelsCache = settings.m_labels;
	string labelName(label.c_str());

#ifdef DEBUG
	clog << "DaemonState::DBusMessageHandler::AddLabel: called on " << label << endl;
#endif
	if (pIndex == NULL)
	{
		Gio::DBus::Error error(Gio::DBus::Error::FAILED, "Couldn't open index");

		invocation.ret(error);

		return;
	}

	if (labelsCache.empty() == true)
	{
		pIndex->getLabels(labelsCache);
	}

	// Is this a known label ?
	if ((labelsCache.find(labelName) == labelsCache.end()) &&
		(pIndex->addLabel(labelName) == true))
	{
		flushIndexAndSignal(pIndex);
	}

	invocation.ret(label);

	delete pIndex;
}

void DaemonState::DBusMessageHandler::DeleteLabel(const ustring &label,
	PinotStub::MethodInvocation &invocation)
{
	PinotSettings &settings = PinotSettings::getInstance();
	IndexInterface *pIndex = settings.getIndex(settings.m_daemonIndexLocation);
	MetaDataBackup metaData(settings.getHistoryDatabaseName());
	set<string> &labelsCache = settings.m_labels;
	string labelName(label.c_str());

#ifdef DEBUG
	clog << "DaemonState::DBusMessageHandler::DeleteLabel: called on " << label << endl;
#endif
	if (pIndex == NULL)
	{
		Gio::DBus::Error error(Gio::DBus::Error::FAILED, "Couldn't open index");

		invocation.ret(error);

		return;
	}

	if (labelsCache.empty() == true)
	{
		pIndex->getLabels(labelsCache);
	}

	// Is this a known label ?
	set<string>::iterator labelIter = labelsCache.find(labelName);
	if ((labelIter != labelsCache.end()) &&
		(pIndex->deleteLabel(labelName) == true))
	{
		labelsCache.erase(labelIter);

		pIndex->setLabels(labelsCache, true);

		flushIndexAndSignal(pIndex);

		// Update the metadata backup
		metaData.deleteLabel(label.c_str());
	}

	invocation.ret(label);

	delete pIndex;
}

void DaemonState::DBusMessageHandler::GetDocumentLabels(guint32 docId,
	PinotStub::MethodInvocation &invocation)
{
	PinotSettings &settings = PinotSettings::getInstance();
	IndexInterface *pIndex = settings.getIndex(settings.m_daemonIndexLocation);
	set<string> labels;

#ifdef DEBUG
	clog << "DaemonState::DBusMessageHandler::GetDocumentLabels: called on " << docId << endl;
#endif
	if (pIndex == NULL)
	{
		Gio::DBus::Error error(Gio::DBus::Error::FAILED, "Couldn't open index");

		invocation.ret(error);

		return;
	}

	if (pIndex->getDocumentLabels(docId, labels) == true)
	{
		vector<ustring> labelsList;

		for (set<string>::const_iterator labelIter = labels.begin();
			labelIter != labels.end(); ++labelIter)
		{
			labelsList.push_back(labelIter->c_str());
		}

		invocation.ret(labelsList);
	}
	else
	{
		Gio::DBus::Error error(Gio::DBus::Error::FAILED, "Unknown document");

		invocation.ret(error);
	}

	delete pIndex;
}

void DaemonState::DBusMessageHandler::SetDocumentLabels(guint32 docId,
	const vector<ustring> &labels,
	bool resetLabels,
	PinotStub::MethodInvocation &invocation)
{
	PinotSettings &settings = PinotSettings::getInstance();
	IndexInterface *pIndex = settings.getIndex(settings.m_daemonIndexLocation);
	MetaDataBackup metaData(settings.getHistoryDatabaseName());
	set<string> &labelsCache = settings.m_labels;
	bool updateLabelsCache = false;

#ifdef DEBUG
	clog << "DaemonState::DBusMessageHandler::SetDocumentLabels: called on " << docId
		<< ", " << labels.size() << " labels" << ", " << resetLabels << endl;
#endif
	if (pIndex == NULL)
	{
		Gio::DBus::Error error(Gio::DBus::Error::FAILED, "Couldn't open index");

		invocation.ret(error);

		return;
	}

	if (labelsCache.empty() == true)
	{
		pIndex->getLabels(labelsCache);
	}

	set<string> labelsList;

	for (vector<ustring>::const_iterator labelIter = labels.begin();
		labelIter != labels.end(); ++labelIter)
	{
		string labelName(labelIter->c_str());

		labelsList.insert(labelName);
		// Is this a known label ?
		if (labelsCache.find(labelName) == labelsCache.end())
		{
			// No, it isn't but that's okay
			labelsCache.insert(labelName);
			updateLabelsCache = true;
		}
	}

	// Set labels
	if (pIndex->setDocumentLabels(docId, labelsList, resetLabels) == true)
	{
		if (updateLabelsCache == true)
		{
			pIndex->setLabels(labelsCache, true);
		}

		flushIndexAndSignal(pIndex);

		// Update the metadata backup
		updateLabels(docId, metaData, pIndex, labelsList, resetLabels);

		invocation.ret(docId);
	}
	else
	{
		Gio::DBus::Error error(Gio::DBus::Error::FAILED, "Unknown document");

		invocation.ret(error);
	}

	delete pIndex;
}

void DaemonState::DBusMessageHandler::SetDocumentsLabels(const vector<ustring> &docIds,
	const vector<ustring> &labels,
	bool resetLabels,
	PinotStub::MethodInvocation &invocation)
{
	PinotSettings &settings = PinotSettings::getInstance();
	IndexInterface *pIndex = settings.getIndex(settings.m_daemonIndexLocation);
	MetaDataBackup metaData(settings.getHistoryDatabaseName());
	set<string> &labelsCache = settings.m_labels;
	bool updateLabelsCache = false;

#ifdef DEBUG
	clog << "DaemonState::DBusMessageHandler::SetDocumentsLabels: called on " << docIds.size()
		<< " IDs, " << labels.size() << " labels" << ", " << resetLabels << endl;
#endif
	if (pIndex == NULL)
	{
		Gio::DBus::Error error(Gio::DBus::Error::FAILED, "Couldn't open index");

		invocation.ret(error);

		return;
	}

	if (labelsCache.empty() == true)
	{
		pIndex->getLabels(labelsCache);
	}

	set<unsigned int> idsList;
	set<string> labelsList;

	for (vector<ustring>::const_iterator idIter = docIds.begin();
		idIter != docIds.end(); ++idIter)
	{
		idsList.insert((unsigned int)atoi(idIter->c_str()));
	}
	for (vector<ustring>::const_iterator labelIter = labels.begin();
		labelIter != labels.end(); ++labelIter)
	{
		string labelName(labelIter->c_str());

		labelsList.insert(labelName);
		// Is this a known label ?
		if (labelsCache.find(labelName) == labelsCache.end())
		{
			// No, it isn't but that's okay
			labelsCache.insert(labelName);
			updateLabelsCache = true;
		}
	}

	// Set labels
	if (pIndex->setDocumentsLabels(idsList, labelsList, resetLabels) == true)
	{
		if (updateLabelsCache == true)
		{
			pIndex->setLabels(labelsCache, true);
		}

		flushIndexAndSignal(pIndex);

		for (set<unsigned int>::const_iterator docIter = idsList.begin();
			docIter != idsList.end(); ++docIter)
		{
			// Update the metadata backup
			updateLabels(*docIter, metaData, pIndex, labelsList, resetLabels);
		}

		invocation.ret(resetLabels);
	}
	else
	{
		Gio::DBus::Error error(Gio::DBus::Error::FAILED, "Unknown documents");

		invocation.ret(error);
	}

	delete pIndex;
}

void DaemonState::DBusMessageHandler::GetDocumentInfo(guint32 docId,
	PinotStub::MethodInvocation &invocation)
{
	PinotSettings &settings = PinotSettings::getInstance();
	IndexInterface *pIndex = settings.getIndex(settings.m_daemonIndexLocation);
	DocumentInfo docInfo;

#ifdef DEBUG
	clog << "DaemonState::DBusMessageHandler::GetDocumentInfo: called on " << docId << endl;
#endif
	if (pIndex == NULL)
	{
		Gio::DBus::Error error(Gio::DBus::Error::FAILED, "Couldn't open index");

		invocation.ret(error);

		return;
	}

	if (pIndex->getDocumentInfo(docId, docInfo) == true)
	{
		vector<tuple<ustring, ustring>> tuples;

		DBusIndex::documentInfoToTuples(docInfo, tuples);

		invocation.ret(tuples);
	}
	else
	{
		Gio::DBus::Error error(Gio::DBus::Error::FAILED, "Unknown document");

		invocation.ret(error);
	}

	delete pIndex;
}

void DaemonState::DBusMessageHandler::SetDocumentInfo(guint32 docId,
	const vector<tuple<ustring,ustring>> &fields,
	PinotStub::MethodInvocation &invocation)
{
	PinotSettings &settings = PinotSettings::getInstance();
	IndexInterface *pIndex = settings.getIndex(settings.m_daemonIndexLocation);
	MetaDataBackup metaData(settings.getHistoryDatabaseName());
	DocumentInfo docInfo;

#ifdef DEBUG
	clog << "DaemonState::DBusMessageHandler::SetDocumentInfo: called on " << docId << endl;
#endif
	if (pIndex == NULL)
	{
		Gio::DBus::Error error(Gio::DBus::Error::FAILED, "Couldn't open index");

		invocation.ret(error);

		return;
	}

	DBusIndex::documentInfoFromTuples(fields, docInfo);

	// Update the document info
	if (pIndex->updateDocumentInfo(docId, docInfo) == true)
	{
		flushIndexAndSignal(pIndex);

		// Update the metadata backup
		metaData.addItem(docInfo, DocumentInfo::SERIAL_FIELDS);

		invocation.ret(docId);
	}
	else
	{
		Gio::DBus::Error error(Gio::DBus::Error::FAILED, "Unknown document");

		invocation.ret(error);
	}

	delete pIndex;
}

void DaemonState::DBusMessageHandler::Query(const ustring &engineType,
	const ustring &engineName,
	const ustring &searchText,
	guint32 startDoc,
	guint32 maxHits,
	PinotStub::MethodInvocation &invocation)
{
	PinotSettings &settings = PinotSettings::getInstance();

#ifdef DEBUG
	clog << "DaemonState::DBusMessageHandler::Query: called on " << searchText
		<< ", " << startDoc << "/" << maxHits << endl;
#endif
	if (searchText.empty() == true)
	{
		Gio::DBus::Error error(Gio::DBus::Error::INVALID_ARGS, "Query is not set");

		invocation.ret(error);

		return;
	}

	DBusEngineQueryThread *pEngineQueryThread = NULL;
	QueryProperties queryProps("", searchText.c_str());

	queryProps.setMaximumResultsCount(maxHits);

	// Provide reasonable defaults
	if ((engineType.empty() == true) &&
		(engineName.empty() == true))
	{
		pEngineQueryThread = new DBusEngineQueryThread(invocation.getMessage(),
			settings.m_defaultBackend, settings.m_defaultBackend,
			settings.m_daemonIndexLocation, queryProps,
			startDoc, false);
	}
	else
	{
		pEngineQueryThread = new DBusEngineQueryThread(invocation.getMessage(),
			engineType.c_str(), engineType.c_str(),
			engineName, queryProps,
			startDoc, false);
	}

	m_pServer->start_thread(pEngineQueryThread);
}

void DaemonState::DBusMessageHandler::SimpleQuery(const ustring &searchText,
	guint32 maxHits,
	PinotStub::MethodInvocation &invocation)
{
	PinotSettings &settings = PinotSettings::getInstance();

#ifdef DEBUG
	clog << "DaemonState::DBusMessageHandler::SimpleQuery: called on " << searchText
		<< ", " << maxHits << endl;
#endif
	if (searchText.empty() == true)
	{
		Gio::DBus::Error error(Gio::DBus::Error::INVALID_ARGS, "Query is not set");

		invocation.ret(error);

		return;
	}

	QueryProperties queryProps("", searchText.c_str());

	queryProps.setMaximumResultsCount(maxHits);

	m_pServer->start_thread(new DBusEngineQueryThread(invocation.getMessage(),
		settings.m_defaultBackend, settings.m_defaultBackend,
		settings.m_daemonIndexLocation, queryProps,
		0, true));
}

void DaemonState::DBusMessageHandler::UpdateDocument(guint32 docId,
	PinotStub::MethodInvocation &invocation)
{
	PinotSettings &settings = PinotSettings::getInstance();
	IndexInterface *pIndex = settings.getIndex(settings.m_daemonIndexLocation);
	DocumentInfo docInfo;

#ifdef DEBUG
	clog << "DaemonState::DBusMessageHandler::UpdateDocument: called on " << docId << endl;
#endif
	if (pIndex == NULL)
	{
		Gio::DBus::Error error(Gio::DBus::Error::FAILED, "Couldn't open index");

		invocation.ret(error);

		return;
	}

	if (pIndex->getDocumentInfo(docId, docInfo) == true)
	{
		// Update document
		m_pServer->queue_index(docInfo);

		invocation.ret(docId);
	}
	else
	{
		Gio::DBus::Error error(Gio::DBus::Error::FAILED, "Unknown document");

		invocation.ret(error);
	}

	delete pIndex;
}

DaemonState::DBusSearchProvider::DBusSearchProvider(DaemonState *pServer) :
	SearchProvider2Stub(),
	m_pServer(pServer)
{
}

DaemonState::DBusSearchProvider::~DBusSearchProvider()
{
}

void DaemonState::DBusSearchProvider::GetInitialResultSet(const vector<ustring> &terms,
	MethodInvocation &invocation)
{
	PinotSettings &settings = PinotSettings::getInstance();
	string searchText;

	for (vector<ustring>::const_iterator termIter = terms.begin();
		termIter != terms.end(); ++termIter)
	{
		if (searchText.empty() == false)
		{
			searchText += " ";
		}
		searchText += termIter->c_str();
	}
#ifdef DEBUG
	clog << "DaemonState::DBusSearchProvider::GetInitialResultSet: called on " << searchText << endl;
#endif

	QueryProperties queryProps("", searchText.c_str());

	queryProps.setMaximumResultsCount(10);

	// The caller expects the same output as that of SimpleQuery
	m_pServer->start_thread(new DBusEngineQueryThread(invocation.getMessage(),
		settings.m_defaultBackend, settings.m_defaultBackend,
		settings.m_daemonIndexLocation, queryProps, 0, true, false));
}

void DaemonState::DBusSearchProvider::GetSubsearchResultSet(const vector<ustring> &previous_results,
	const vector<ustring> &terms,
	MethodInvocation &invocation)
{
	PinotSettings &settings = PinotSettings::getInstance();
	string searchText;

	for (vector<ustring>::const_iterator termIter = terms.begin();
		termIter != terms.end(); ++termIter)
	{
		if (searchText.empty() == false)
		{
			searchText += " ";
		}
		searchText += termIter->c_str();
	}
#ifdef DEBUG
	clog << "DaemonState::DBusSearchProvider::GetSubsearchResultSet: called on " << searchText << endl;
#endif

	QueryProperties queryProps("", searchText.c_str());

	queryProps.setMaximumResultsCount(10);

	// The caller expects the same output as that of SimpleQuery
	// FIXME: is this meant to return only a subset of previous_results?
	m_pServer->start_thread(new DBusEngineQueryThread(invocation.getMessage(),
	settings.m_defaultBackend, settings.m_defaultBackend,
	settings.m_daemonIndexLocation, queryProps, 0, true, false));
}

void DaemonState::DBusSearchProvider::GetResultMetas(const vector<ustring> &identifiers,
	MethodInvocation &invocation)
{
	PinotSettings &settings = PinotSettings::getInstance();
	IndexInterface *pIndex = settings.getIndex(settings.m_daemonIndexLocation);
	vector<map<ustring, VariantBase>> idsToDictionary;

#ifdef DEBUG
	clog << "DaemonState::DBusSearchProvider::GetResultMetas: called on "
		<< identifiers.size() << " IDs" << endl;
#endif
	if (pIndex == NULL)
	{
		Gio::DBus::Error error(Gio::DBus::Error::FAILED, "Couldn't open index");

		invocation.ret(error);

		return;
	}

	for (vector<ustring>::const_iterator idIter = identifiers.begin();
		idIter != identifiers.end(); ++idIter)
	{
		DocumentInfo docInfo;
		unsigned int docId = atoi(idIter->c_str());

		if (pIndex->getDocumentInfo(docId, docInfo) == true)
		{
			map<ustring, ustring> docFields;
			map<ustring, VariantBase> docDictionary;
			RefPtr<Gio::Icon> typeIcon = Gio::content_type_get_icon(docInfo.getType().c_str());

			docFields.insert(pair<ustring, ustring>("id", *idIter));
			docFields.insert(pair<ustring, ustring>("name", docInfo.getTitle().c_str()));
			docFields.insert(pair<ustring, ustring>("gicon", typeIcon->to_string()));
			docFields.insert(pair<ustring, ustring>("description", docInfo.getExtract()));

			Variant<map<ustring, ustring>> varDictionary = Variant<map<ustring, ustring>>::create(docFields);

			docDictionary.insert(pair<ustring, VariantBase>(*idIter, varDictionary));

			idsToDictionary.push_back(docDictionary);
		}
	}

	invocation.ret(idsToDictionary);

	delete pIndex;
}

void DaemonState::DBusSearchProvider::ActivateResult(const ustring &identifier,
	const vector<ustring> &terms,
	guint32 timestamp,
	MethodInvocation &invocation)
{
	PinotSettings &settings = PinotSettings::getInstance();
	IndexInterface *pIndex = settings.getIndex(settings.m_daemonIndexLocation);
	DocumentInfo docInfo;
	unsigned int docId = atoi(identifier.c_str());

#ifdef DEBUG
	clog << "DaemonState::DBusSearchProvider::ActivateResult: called on " << identifier << endl;
#endif
	if (pIndex == NULL)
	{
		invocation.ret();

		return;
	}

	if (pIndex->getDocumentInfo(docId, docInfo) == true)
	{
		RefPtr<Gio::AppInfo> defaultApp = Gio::AppInfo::get_default_for_type(docInfo.getType().c_str());
		RefPtr<Gio::AppLaunchContext> launchContext;
		vector<string> uris;

		uris.push_back(docInfo.getLocation());
		defaultApp->launch_uris_async(uris, launchContext);
	}

	invocation.ret();

	delete pIndex;
}

void DaemonState::DBusSearchProvider::LaunchSearch(const vector<ustring> &terms,
	guint32 timestamp,
	MethodInvocation &invocation)
{
#ifdef DEBUG
	clog << "DaemonState::DBusSearchProvider::LaunchSearch: called on "
		<< terms.size() << " terms" << endl;
#endif
	// FIXME: save the terms as a query, open the UI with that
	invocation.ret();
}
#endif

DaemonState::DaemonState() :
	QueueManager(PinotSettings::getInstance().m_daemonIndexLocation),
#ifdef HAVE_DBUS
	m_refSessionBus(Gio::DBus::Connection::get_sync(Gio::DBus::BUS_TYPE_SESSION)),
	m_introspectionHandler(),
	m_messageHandler(this),
	m_searchProvider(this),
    m_powerProxy(Gio::DBus::Proxy::create_for_bus_sync(Gio::DBus::BUS_TYPE_SYSTEM,
		POWER_DBUS_SERVICE_NAME, POWER_DBUS_OBJECT_PATH,
		POWER_DBUS_SERVICE_NAME, {}, {},
		Gio::DBus::PROXY_FLAGS_DO_NOT_AUTO_START_AT_CONSTRUCTION)),
	m_connectionId(0),
#endif
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
	m_timeoutConnection = signal_timeout().connect(sigc::mem_fun(*this,
		&DaemonState::on_activity_timeout), 60000);
#ifndef CHECK_BATTERY_SYSCTL
#ifdef HAVE_DBUS
	// Listen for battery property changes
	m_powerProxy->signal_properties_changed().
		connect(sigc::mem_fun(this, &DaemonState::handle_power_properties_changed));
#endif
#endif
	// Check right now before doing anything else
	DaemonState::on_activity_timeout();

	m_onThreadEndSignal.connect(sigc::mem_fun(*this, &DaemonState::on_thread_end));
}

DaemonState::~DaemonState()
{
	// Don't delete m_pDiskMonitor and m_pDiskHandler, threads may need them
	// Since DaemonState is destroyed when the program exits, it's a leak we can live with
}

void DaemonState::disconnect(void)
{
	QueueManager::disconnect();

#ifdef HAVE_DBUS
	if (m_connectionId > 0)
	{
		Gio::DBus::unown_name(m_connectionId);
	}
#endif
}

void DaemonState::handle_power_properties_changed(const Gio::DBus::Proxy::MapChangedProperties &changed_properties,
	const vector<ustring> &invalidated_properties)
{
	if (changed_properties.find("OnBattery") != changed_properties.cend())
	{
		Variant<bool> boolValue;

		m_powerProxy->get_cached_property(boolValue, "OnBattery");

		if (boolValue)
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

		if (m_messageHandler.mustQuit() == true)
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
		// Flush
		pIndex->flush();

		// Signal
		emit_IndexFlushed(pIndex->getDocumentsCount());

		delete pIndex;
	}

	int inUse = Memory::getUsage();
	Memory::reclaim();
}

void DaemonState::register_session(void)
{
#ifdef HAVE_DBUS
	m_connectionId = Gio::DBus::own_name(
		Gio::DBus::BUS_TYPE_SESSION,
		PINOT_DBUS_SERVICE_NAME,
		[&](const RefPtr<Gio::DBus::Connection> &connection,
			const ustring & /* name */) {
			guint introId = m_introspectionHandler.register_object(m_refSessionBus,
				PINOT_DBUS_OBJECT_PATH);
			guint messageId = m_messageHandler.register_object(m_refSessionBus,
				PINOT_DBUS_OBJECT_PATH);
			m_searchProvider.register_object(m_refSessionBus,
				PINOT_DBUS_OBJECT_PATH);
#ifdef DEBUG
			clog << "DaemonState::register_object: registered on " << PINOT_DBUS_OBJECT_PATH
				<< " with IDs " << introId << " " << messageId << endl;
#endif
		},
		[&](const RefPtr<Gio::DBus::Connection> &connection,
			const ustring &name) {
#ifdef DEBUG
			clog << "DaemonState::register_object: acquired " << name << endl;
#endif
		},
		[&](const RefPtr<Gio::DBus::Connection> &connection,
			const ustring &name) {
#ifdef DEBUG
			clog << "DaemonState::register_object: lost " << name << endl;
#endif
			mustQuit(true);
		}
	);
#endif
}

void DaemonState::emit_IndexFlushed(unsigned int docsCount)
{
#ifdef HAVE_DBUS
	m_messageHandler.emit_IndexFlushed(docsCount);
#endif
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
	else if (type == "DBusEngineQueryThread")
	{
		DBusEngineQueryThread *pEngineQueryThread = dynamic_cast<DBusEngineQueryThread *>(pThread);
		if (pEngineQueryThread == NULL)
		{
			delete pThread;
			return;
		}

		RefPtr<Gio::DBus::MethodInvocation> refInvocation = pEngineQueryThread->getInvocation();
		const vector<DocumentInfo> &documentsList = pEngineQueryThread->getDocuments();
		unsigned int documentsCount = pEngineQueryThread->getDocumentsCount();
		bool simpleQuery = pEngineQueryThread->isSimpleQuery();
		bool pinotCall = pEngineQueryThread->isPinotCall();
		vector<ustring> idsList;
		vector<vector<tuple<ustring, ustring>>> docTuples;

		for (vector<DocumentInfo>::const_iterator docIter = documentsList.begin();
			docIter != documentsList.end(); ++docIter)
		{
			unsigned int indexId = 0;
			unsigned int docId = docIter->getIsIndexed(indexId);

			if (simpleQuery == false)
			{
				vector<tuple<ustring, ustring>> tuples;

				// The document ID isn't needed here
				DBusIndex::documentInfoToTuples(*docIter, tuples);

				docTuples.push_back(tuples);
			}
			else if (docId > 0)
			{
				stringstream docIdStr;

				// We only need the document ID
				docIdStr << docId;
				idsList.push_back(docIdStr.str().c_str());
			}
		}

		if (pinotCall == true)
		{
			com::github::fabricecolin::PinotStub::MethodInvocation pinotInvocation(refInvocation);

			if (simpleQuery == false)
			{
				pinotInvocation.ret(documentsCount, docTuples);
			}
			else
			{
				pinotInvocation.ret(idsList);
			}
		}
		else
		{
			org::gnome::Shell::SearchProvider2Stub::MethodInvocation shellInvocation(refInvocation);

			shellInvocation.ret(idsList);
		}
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

