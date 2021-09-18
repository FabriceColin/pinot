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

#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#ifdef __OpenBSD__
#include <sys/param.h>
#include <sys/sysctl.h>
#endif
#include <exception>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <glibmm/miscutils.h>
#include <glibmm/convert.h>
#include <glibmm/exception.h>

#include "config.h"
#include "NLS.h"
#include "Languages.h"
#include "MIMEScanner.h"
#include "TimeConverter.h"
#include "Timer.h"
#include "Url.h"
#include "HtmlFilter.h"
#include "FilterUtils.h"
#include "DownloaderFactory.h"
#include "FilterWrapper.h"
#include "ModuleFactory.h"
#include "WebEngine.h"
#include "WorkerThreads.h"

using namespace std;
using namespace Glib;

QueueManager::QueueManager(const string &defaultIndexLocation,
	unsigned int maxThreadsTime, bool scanLocalFiles) :
	ThreadsManager(defaultIndexLocation, maxThreadsTime),
	m_scanLocalFiles(scanLocalFiles),
	m_stopIndexing(false),
	m_actionQueue(PinotSettings::getInstance().getHistoryDatabaseName(), get_application_name())
{
}

QueueManager::~QueueManager()
{
}

ustring QueueManager::index_document(const DocumentInfo &docInfo)
{
	string location(docInfo.getLocation());

#ifdef DEBUG
	clog << "ThreadsManager::index_document: called with " << location << endl;
#endif
	if (m_stopIndexing == true)
	{
#ifdef DEBUG
		clog << "ThreadsManager::index_document: stopped indexing" << endl;
#endif
		return _("Indexing was stopped");
	}

	if (location.empty() == true)
	{
		// Nothing to do
		return "";
	}

	// If the document is a mail message, we can't index it again
	Url urlObj(location);
	if (urlObj.getProtocol() == "mailbox")
	{
		return _("Can't index mail here");
	}

	// Is the document being indexed/updated ?
	if (write_lock_lists() == true)
	{
		bool beingProcessed = true;

		if (m_beingIndexed.find(location) == m_beingIndexed.end())
		{
			m_beingIndexed.insert(location);
			beingProcessed = false;
		}

		unlock_lists();

		if (beingProcessed == true)
		{
			// FIXME: we may have to set labels on this document
			// FIXME: fix this for RTL languages
			ustring status(location);
			status += " ";
			status += _("is already being indexed");
			return status;
		}
	}

	// Is the document blacklisted ?
	if (PinotSettings::getInstance().isBlackListed(location) == true)
	{
		// FIXME: fix this for RTL languages
		ustring status(location);
		status += " ";
		status += _("is blacklisted");
		return status;
	}

	if ((m_scanLocalFiles == true) &&
		(urlObj.isLocal() == true))
	{
#ifdef DEBUG
		clog << "ThreadsManager::index_document: scanning " <<
			urlObj.getLocation() + "/" + urlObj.getFile() << endl;
#endif
		// This handles both directories and files
		start_thread(new DirectoryScannerThread(docInfo,
			m_defaultIndexLocation, 0, true, true));
	}
	else
	{
		start_thread(new IndexingThread(docInfo, m_defaultIndexLocation));
	}

	return "";
}

void QueueManager::clear_queues(void)
{
	if (write_lock_lists() == true)
	{
		m_beingIndexed.clear();

		unlock_lists();

		m_actionQueue.expireItems(time(NULL));
	}
}

ustring QueueManager::queue_index(const DocumentInfo &docInfo)
{
	bool addToQueue = false;

	if (get_threads_count() >= m_maxIndexThreads)
	{
#ifdef DEBUG
		clog << "QueueManager::queue_index: too many threads" << endl;
#endif
		addToQueue = true;
	}
#ifdef HAVE_GETLOADAVG
	// Get the load averaged over the last minute
	else
	{
		double averageLoad[3];

		if (getloadavg(averageLoad, 3) != -1)
		{
			// FIXME: is LOADAVG_1MIN Solaris specific ?
			if (averageLoad[0] >= (double)m_numCPUs * 4)
			{
				// Don't add to the load, queue this
				addToQueue = true;
			}
		}
	}
#endif

	if (addToQueue == true)
	{
		m_actionQueue.pushItem(ActionQueue::INDEX, docInfo);

		return "";
	}

	return index_document(docInfo);
}

bool QueueManager::pop_queue(const string &urlWasIndexed)
{
	bool getItem = true;
	bool emptyQueue = false;

#ifdef DEBUG
	clog << "QueueManager::pop_queue: called with " << urlWasIndexed << endl;
#endif
	if (get_threads_count() >= m_maxIndexThreads)
	{
#ifdef DEBUG
		clog << "QueueManager::pop_queue: too many threads" << endl;
#endif
		getItem = false;
	}

	if (write_lock_lists() == true)
	{
		// Update the in-progress list
		if (urlWasIndexed.empty() == false)
		{
			set<string>::iterator urlIter = m_beingIndexed.find(urlWasIndexed);
			if (urlIter != m_beingIndexed.end())
			{
				m_beingIndexed.erase(urlIter);
			}
		}

		unlock_lists();

		// Get an item ?
		if (getItem == true)
		{
			ActionQueue::ActionType type;
			DocumentInfo docInfo;
			string previousLocation;

			// Assume the queue is empty
			emptyQueue = true;

			while (m_actionQueue.popItem(type, docInfo) == true)
			{
				ustring status;

				if (type != ActionQueue::INDEX)
				{
					continue;
				}

				// The queue isn't actually empty
				emptyQueue = false;

				if (docInfo.getLocation() == previousLocation)
				{
					// Something dodgy is going on, we got the same item twice !
					// FIXME: fix this for RTL languages
					status = previousLocation;
					status += " ";
					status += _("is already being indexed");
				}
				else
				{
					status = index_document(docInfo);
				}

				if (status.empty() == true)
				{
					break;
				}

				previousLocation = docInfo.getLocation();
			}
		}
	}

	return emptyQueue;
}

ListerThread::ListerThread(const PinotSettings::IndexProperties &indexProps,
	unsigned int startDoc) :
	WorkerThread(),
	m_indexProps(indexProps),
	m_startDoc(startDoc),
	m_documentsCount(0)
{
}

ListerThread::~ListerThread()
{
}

string ListerThread::getType(void) const
{
	return "ListerThread";
}

PinotSettings::IndexProperties ListerThread::getIndexProperties(void) const
{
	return m_indexProps;
}

unsigned int ListerThread::getStartDoc(void) const
{
	return m_startDoc;
}

const vector<DocumentInfo> &ListerThread::getDocuments(void) const
{
	return m_documentsList;
}

unsigned int ListerThread::getDocumentsCount(void) const
{
	return m_documentsCount;
}

QueryingThread::QueryingThread(const PinotSettings::IndexProperties &indexProps,
	const QueryProperties &queryProps, unsigned int startDoc, bool listingIndex) :
	ListerThread(indexProps, startDoc),
	m_engineName(PinotSettings::getInstance().m_defaultBackend),
	m_engineDisplayableName(indexProps.m_name),
	m_engineOption(indexProps.m_location),
	m_queryProps(queryProps),
	m_listingIndex(listingIndex),
	m_correctedSpelling(false),
	m_isLive(true)
{
#ifdef DEBUG
	clog << "QueryingThread: engine " << m_engineName << ", " << m_engineOption
		<< ", mode " << m_listingIndex << endl;
#endif
}

QueryingThread::QueryingThread(const string &engineName, const string &engineDisplayableName,
	const string &engineOption, const QueryProperties &queryProps,
	unsigned int startDoc) :
	ListerThread(PinotSettings::IndexProperties(engineDisplayableName, engineOption, 0, false), startDoc),
	m_engineName(engineName),
	m_engineDisplayableName(engineDisplayableName),
	m_engineOption(engineOption),
	m_queryProps(queryProps),
	m_listingIndex(false),
	m_correctedSpelling(false),
	m_isLive(true)
{
#ifdef DEBUG
	clog << "QueryingThread: engine " << m_engineName << ", " << m_engineOption
		<< ", mode 0" << endl;
#endif
}

QueryingThread::~QueryingThread()
{
}

string QueryingThread::getType(void) const
{
	if (m_listingIndex == true)
	{
		return ListerThread::getType();
	}

	return "QueryingThread";
}

bool QueryingThread::isLive(void) const
{
	return m_isLive;
}

string QueryingThread::getEngineName(void) const
{
	return m_engineDisplayableName;
}

QueryProperties QueryingThread::getQuery(bool &wasCorrected) const
{
	wasCorrected = m_correctedSpelling;
	return m_queryProps;
}

string QueryingThread::getCharset(void) const
{
	return m_resultsCharset;
}

bool QueryingThread::findPlugin(void)
{
	string pluginName;

	if ((m_engineName.empty() == true) &&
		(m_engineOption.empty() == false))
	{
		pluginName = m_engineOption;
	}
	else if ((m_engineName.empty() == false) &&
		(m_engineOption.empty() == true))
	{
		pluginName = m_engineName;
	}

	if (pluginName.empty() == false)
	{
		set<ModuleProperties> engines;
		PinotSettings::getInstance().getSearchEngines(engines, "");
#ifdef DEBUG
		clog << "QueryingThread::findPlugin: looking for a plugin named " << pluginName << endl;
#endif

		// Is there a plugin with such a name ?
		ModuleProperties modProps("sherlock", pluginName, "", "");
		set<ModuleProperties>::const_iterator engineIter = engines.find(modProps);
		if (engineIter == engines.end())
		{
			// Try again
			modProps.m_name = "opensearch";
			engineIter = engines.find(modProps);
		}

		if (engineIter != engines.end())
		{
			// Yes, there is !
			m_engineName = engineIter->m_name;
			m_engineDisplayableName = engineIter->m_longName;
			m_engineOption = engineIter->m_option;
#ifdef DEBUG
			clog << "QueryingThread::findPlugin: found " << m_engineName << ", " << m_engineDisplayableName << ", " << m_engineOption << endl;
#endif

			return true;
		}
	}

	return false;
}

EngineQueryThread::EngineQueryThread(const PinotSettings::IndexProperties &indexProps,
	const QueryProperties &queryProps, unsigned int startDoc, bool listingIndex) :
	QueryingThread(indexProps, queryProps, startDoc, listingIndex)
{
}

EngineQueryThread::EngineQueryThread(const PinotSettings::IndexProperties &indexProps,
	const QueryProperties &queryProps, const set<string> &limitToDocsSet,
	unsigned int startDoc) :
	QueryingThread(indexProps, queryProps, startDoc, false)
{
	copy(limitToDocsSet.begin(), limitToDocsSet.end(),
		inserter(m_limitToDocsSet, m_limitToDocsSet.begin()));
}

EngineQueryThread::EngineQueryThread(const string &engineName, const string &engineDisplayableName,
	const string &engineOption, const QueryProperties &queryProps, unsigned int startDoc) :
	QueryingThread(engineName, engineDisplayableName, engineOption, queryProps, startDoc)
{
}

EngineQueryThread::~EngineQueryThread()
{
}

void EngineQueryThread::processResults(const vector<DocumentInfo> &resultsList)
{
	PinotSettings &settings = PinotSettings::getInstance();
	IndexInterface *pDocsIndex = NULL;
	IndexInterface *pDaemonIndex = NULL;
	unsigned int indexId = 0;
	bool isIndexQuery = false;

	// Are we querying an index ?
	if (ModuleFactory::isSupported(m_engineName, true) == true)
	{
		// Internal index ?
		if ((m_engineOption == settings.m_docsIndexLocation) ||
			(m_engineOption == settings.m_daemonIndexLocation))
		{
			indexId = settings.getIndexPropertiesByLocation(m_engineOption).m_id;
			isIndexQuery = true;
		}
	}

	// Will we have to query internal indices ?
	if (isIndexQuery == false)
	{
		pDocsIndex = settings.getIndex(settings.m_docsIndexLocation);
		pDaemonIndex = settings.getIndex(settings.m_daemonIndexLocation);
	}

	// Copy the results list
	for (vector<DocumentInfo>::const_iterator resultIter = resultsList.begin();
		resultIter != resultsList.end(); ++resultIter)
	{
		DocumentInfo currentDoc(*resultIter);
		string title(_("No title"));
		string location(currentDoc.getLocation(true));
		string language(currentDoc.getLanguage());
		unsigned int docId = 0;

		// The title may contain formatting
		if (currentDoc.getTitle().empty() == false)
		{
			title = FilterUtils::stripMarkup(currentDoc.getTitle());
		}
		currentDoc.setTitle(title);
#ifdef DEBUG
		clog << "EngineQueryThread::processResults: title is " << title << endl;
#endif

		// Use the query's language if the result's is unknown
		if (language.empty() == true)
		{
			language = m_queryProps.getStemmingLanguage();
		}
		currentDoc.setLanguage(language);

		if (isIndexQuery == true)
		{
			unsigned int tmpId = 0;

			// The index engine should have set this
			docId = currentDoc.getIsIndexed(tmpId);
		}

		// Is this in one of the indexes ?
		if ((pDocsIndex != NULL) &&
			(pDocsIndex->isGood() == true))
		{
			docId = pDocsIndex->hasDocument(location);
			if (docId > 0)
			{
				indexId = settings.getIndexPropertiesByName(_("My Web Pages")).m_id;
			}
		}
		if ((pDaemonIndex != NULL) &&
			(pDaemonIndex->isGood() == true) &&
			(docId == 0))
		{
			docId = pDaemonIndex->hasDocument(location);
			if (docId > 0)
			{
				indexId = settings.getIndexPropertiesByName(_("My Documents")).m_id;
			}
		}

		if (docId > 0)
		{
			currentDoc.setIsIndexed(indexId, docId);
#ifdef DEBUG
			clog << "EngineQueryThread::processResults: found in index " << indexId << endl;
#endif
		}
#ifdef DEBUG
		else clog << "EngineQueryThread::processResults: not found in any index" << endl;
#endif

		m_documentsList.push_back(currentDoc);
	}

	if (pDocsIndex != NULL)
	{
		delete pDocsIndex;
	}
	if (pDaemonIndex != NULL)
	{
		delete pDaemonIndex;
	}
}

void EngineQueryThread::processResults(const vector<DocumentInfo> &resultsList,
	unsigned int indexId)
{
	unsigned int zeroId = 0;

	// Copy the results list
	for (vector<DocumentInfo>::const_iterator resultIter = resultsList.begin();
		resultIter != resultsList.end(); ++resultIter)
	{
		DocumentInfo currentDoc(*resultIter);

		// The engine has no notion of index IDs
		unsigned int docId = currentDoc.getIsIndexed(zeroId);
		currentDoc.setIsIndexed(indexId, docId);

		m_documentsList.push_back(currentDoc);
	}
}

void EngineQueryThread::doWork(void)
{
	PinotSettings &settings = PinotSettings::getInstance();

	// Get the SearchEngine
	SearchEngineInterface *pEngine = ModuleFactory::getSearchEngine(m_engineName, m_engineOption);
	if (pEngine == NULL)
	{
		// Try again
		if (findPlugin() == true)
		{
			pEngine = ModuleFactory::getSearchEngine(m_engineName, m_engineOption);
		}

		if (pEngine == NULL)
		{
			m_errorNum = UNKNOWN_ENGINE;
			m_errorParam = m_engineDisplayableName;
			return;
		}
	}

	// Set up the proxy
	WebEngine *pWebEngine = dynamic_cast<WebEngine *>(pEngine);
	if (pWebEngine != NULL)
	{
		DownloaderInterface *pDownloader = pWebEngine->getDownloader();
		if ((pDownloader != NULL) &&
			(settings.m_proxyEnabled == true) &&
			(settings.m_proxyAddress.empty() == false))
		{
			char portStr[64];

			pDownloader->setSetting("proxyaddress", settings.m_proxyAddress);
			snprintf(portStr, 64, "%u", settings.m_proxyPort);
			pDownloader->setSetting("proxyport", portStr);
			pDownloader->setSetting("proxytype", settings.m_proxyType);
		}

		pWebEngine->setEditableValues(settings.m_editablePluginValues);
	}

	if (m_listingIndex == false)
	{
		pEngine->setLimitSet(m_limitToDocsSet);
	}

	// Run the query
	pEngine->setDefaultOperator(SearchEngineInterface::DEFAULT_OP_AND);
	if (pEngine->runQuery(m_queryProps, m_startDoc) == false)
	{
		m_errorNum = QUERY_FAILED;
		m_errorParam = m_engineDisplayableName;
	}
	else
	{
		const vector<DocumentInfo> &resultsList = pEngine->getResults();

		m_documentsList.clear();
		m_documentsList.reserve(resultsList.size());
		m_documentsCount = pEngine->getResultsCountEstimate();
#ifdef DEBUG
		clog << "EngineQueryThread::doWork: " << resultsList.size() << " off " << m_documentsCount
			<< " results to process, starting at position " << m_startDoc << endl;
#endif

		m_resultsCharset = pEngine->getResultsCharset();
		if (m_listingIndex == false)
		{
			processResults(resultsList);
		}
		else
		{
			processResults(resultsList,
				PinotSettings::getInstance().getIndexPropertiesByName(m_engineDisplayableName).m_id);
		}

		// Don't spellcheck if the query was modified in any way
		if (m_queryProps.getModified() == false)
		{
			string correctedFreeQuery(pEngine->getSpellingCorrection());

			// Any spelling correction ?
			if (correctedFreeQuery.empty() == false)
			{
				m_correctedSpelling = true;
				m_queryProps.setFreeQuery(correctedFreeQuery);
			}
		}
	}

	delete pEngine;
}

DownloadingThread::DownloadingThread(const DocumentInfo &docInfo) :
	WorkerThread(),
	m_docInfo(docInfo),
	m_pDoc(NULL),
	m_pDownloader(NULL)
{
}

DownloadingThread::DownloadingThread() :
	WorkerThread(),
	m_docInfo("", "", "", ""),
	m_pDoc(NULL),
	m_pDownloader(NULL)
{
}

DownloadingThread::~DownloadingThread()
{
	if (m_pDoc != NULL)
	{
		delete m_pDoc;
	}
	if (m_pDownloader != NULL)
	{
		delete m_pDownloader;
	}
}

string DownloadingThread::getType(void) const
{
	return "DownloadingThread";
}

string DownloadingThread::getURL(void) const
{
	return m_docInfo.getLocation();
}

const Document *DownloadingThread::getDocument(void) const
{
	return m_pDoc;
}

void DownloadingThread::doWork(void)
{
	Url thisUrl(m_docInfo.getLocation());
	bool getDownloader = true;

	if (m_pDoc != NULL)
	{
		delete m_pDoc;
		m_pDoc = NULL;
	}

	// Get a Downloader
	if (m_pDownloader != NULL)
	{
		// Same protocol as what we now need ?
		if (m_protocol == thisUrl.getProtocol())
		{
			getDownloader = false;
		}
		else
		{
			delete m_pDownloader;
			m_pDownloader = NULL;
			m_protocol.clear();
		}
	}
	if (getDownloader == true)
	{
		m_protocol = thisUrl.getProtocol();
		m_pDownloader = DownloaderFactory::getDownloader(m_protocol);
	}

	if (m_pDownloader == NULL)
	{
		m_errorNum = UNSUPPORTED_PROTOCOL;
		m_errorParam = thisUrl.getProtocol();
	}
	else if (m_done == false)
	{
		Timer collectTimer;
		PinotSettings &settings = PinotSettings::getInstance();

		// Set up the proxy
		if ((getDownloader == true) &&
			(settings.m_proxyEnabled == true) &&
			(settings.m_proxyAddress.empty() == false))
		{
			char portStr[64];

			m_pDownloader->setSetting("proxyaddress", settings.m_proxyAddress);
			snprintf(portStr, 64, "%u", settings.m_proxyPort);
			m_pDownloader->setSetting("proxyport", portStr);
			m_pDownloader->setSetting("proxytype", settings.m_proxyType);
		}

		collectTimer.start();

		m_pDoc = m_pDownloader->retrieveUrl(m_docInfo);

		clog << "Retrieved " << m_docInfo.getLocation() << " in " << collectTimer.stop() << " ms" << endl;
	}

	if (m_pDoc == NULL)
	{
		m_errorNum = DOWNLOAD_FAILED;
		m_errorParam = m_docInfo.getLocation();
	}
}

IndexingThread::IndexingThread(const DocumentInfo &docInfo, const string &indexLocation,
	bool allowAllMIMETypes) :
	DownloadingThread(docInfo),
	m_pIndex(NULL),
	m_indexLocation(indexLocation),
	m_allowAllMIMETypes(allowAllMIMETypes),
	m_update(false),
	m_docId(0)
{
}

IndexingThread::~IndexingThread()
{
	if (m_pIndex != NULL)
	{
		delete m_pIndex;
	}
}

string IndexingThread::getType(void) const
{
	return "IndexingThread";
}

const DocumentInfo &IndexingThread::getDocumentInfo(void) const
{
	return m_docInfo;
}

unsigned int IndexingThread::getDocumentID(void) const
{
	return m_docId;
}

bool IndexingThread::isNewDocument(void) const
{
	// If the thread is set to perform an update, the document isn't new
	if (m_update == true)
	{
		return false;
	}
	return true;
}

void IndexingThread::doWork(void)
{
	Url thisUrl(m_docInfo.getLocation());
	bool reliableType = false, doDownload = true;

	// First things first, get the index
	if (m_pIndex == NULL)
	{
		m_pIndex = PinotSettings::getInstance().getIndex(m_indexLocation);
	}
	if ((m_pIndex == NULL) ||
		(m_pIndex->isGood() == false))
	{
		m_errorNum = INDEX_ERROR;
		m_errorParam = m_indexLocation;
		return;
	}

	// Is it an update ?
	m_docId = m_pIndex->hasDocument(m_docInfo.getLocation(true));
	if (m_docId > 0)
	{
		// Ignore robots directives on updates
		m_update = true;
	}

	if (m_docInfo.getType().empty() == true)
	{
		m_docInfo.setType(MIMEScanner::scanUrl(thisUrl));
	}
	else if (thisUrl.isLocal() == true)
	{
		// There's a good chance the supplied type is accurate
		// if the document is a local file
		reliableType = true;
	}

	if (m_docInfo.getIsDirectory() == true)
	{
		doDownload = false;
#ifdef DEBUG
		clog << "IndexingThread::doWork: skipping download of directory " << m_docInfo.getLocation() << endl;
#endif
	}
	else if (FilterUtils::isSupportedType(m_docInfo.getType()) == false)
	{
		// Skip unsupported types ?
		if (m_allowAllMIMETypes == false)
		{
			m_errorNum = UNSUPPORTED_TYPE;
			m_errorParam = m_docInfo.getType();

			return;
		}

		if (reliableType == true)
		{
			doDownload = false;
#ifdef DEBUG
			clog << "IndexingThread::doWork: skipping download of unsupported type " << m_docInfo.getLocation() << endl;
#endif
		}
	}
	else
	{
		Dijon::Filter *pFilter = FilterUtils::getFilter(m_docInfo.getType());

		if (pFilter != NULL)
		{
			// We may be able to feed the document directly to the filter
			if (((pFilter->is_data_input_ok(Dijon::Filter::DOCUMENT_FILE_NAME) == true) &&
				(thisUrl.getProtocol() == "file")) ||
				((pFilter->is_data_input_ok(Dijon::Filter::DOCUMENT_URI) == true) &&
				(thisUrl.isLocal() == false)))
			{
				doDownload = false;
#ifdef DEBUG
				clog << "IndexingThread::doWork: let filter download " << m_docInfo.getLocation() << endl;
#endif
			}

			delete pFilter;
		}
	}

	// We may not have to download the document
	if (doDownload == true)
	{
		DownloadingThread::doWork();
	}
	else
	{
		if (m_pDoc != NULL)
		{
			delete m_pDoc;
			m_pDoc = NULL;
		}
		m_pDoc = new Document(m_docInfo);

		m_pDoc->setTimestamp(m_docInfo.getTimestamp());
		m_pDoc->setSize(m_docInfo.getSize());
	}

	if (m_pDoc != NULL)
	{
		Timer indexTimer;
		string docType(m_pDoc->getType());
		bool success = false;

		indexTimer.start();

		// The type may have been obtained when downloading
		if (docType.empty() == false)
		{
			// Use the document's type
			m_docInfo.setType(docType);
		}
		else
		{
			// Use the type we were supplied with
			m_pDoc->setType(m_docInfo.getType());
		}

		if (m_docInfo.getTitle().empty() == false)
		{
			// Use the title we were supplied with
			m_pDoc->setTitle(m_docInfo.getTitle());
		}
		else
		{
			// Use the document's
			m_docInfo.setTitle(m_pDoc->getTitle());
		}
#ifdef DEBUG
		clog << "IndexingThread::doWork: title is " << m_pDoc->getTitle() << endl;
#endif

		// Check again as the downloader may have altered the MIME type
		if (FilterUtils::isSupportedType(m_docInfo.getType()) == false)
		{
			// Skip unsupported types ?
			if (m_allowAllMIMETypes == false)
			{
				m_errorNum = UNSUPPORTED_TYPE;
				m_errorParam = m_docInfo.getType();

				return;
			}

			// Let FilterWrapper handle unspported documents
		}
		else if ((PinotSettings::getInstance().m_ignoreRobotsDirectives == false) &&
			(thisUrl.isLocal() == false) &&
			(m_docInfo.getType().length() >= 9) &&
			(m_docInfo.getType().substr(9) == "text/html"))
		{
			Dijon::HtmlFilter htmlFilter;

			htmlFilter.set_mime_type(m_docInfo.getType());

			if ((FilterUtils::feedFilter(*m_pDoc, &htmlFilter) == true) &&
				(htmlFilter.next_document() == true))
			{
				const map<string, string> &metaData = htmlFilter.get_meta_data();

				// See if the document has a ROBOTS META tag
				map<string, string>::const_iterator robotsIter = metaData.find("robots");
				if (robotsIter != metaData.end())
				{
					string robotsDirectives(robotsIter->second);
	
					// Is indexing allowed ?
					string::size_type pos1 = robotsDirectives.find("none");
					string::size_type pos2 = robotsDirectives.find("noindex");
					if ((pos1 != string::npos) ||
						(pos2 != string::npos))
					{
						// No, it isn't
						m_errorNum = ROBOTS_FORBIDDEN;
						m_errorParam = m_docInfo.getLocation();

						return;
					}
				}
			}
#ifdef DEBUG
			else clog << "IndexingThread::doWork: couldn't check document for ROBOTS directive" << endl;
#endif
		}

		if (m_done == false)
		{
			FilterWrapper wrapFilter(m_pIndex);

			// Update an existing document or add to the index ?
			if (m_update == true)
			{
				set<string> labels;

				// Make sure labels are preserved
				m_pIndex->getDocumentLabels(m_docId, labels);
				m_pDoc->setLabels(labels);

				// Update the document
				if (wrapFilter.updateDocument(*m_pDoc, m_docId) == true)
				{
#ifdef DEBUG
					clog << "IndexingThread::doWork: updated " << m_pDoc->getLocation()
						<< " at " << m_docId << endl;
#endif
					success = true;
				}
#ifdef DEBUG
				else clog << "IndexingThread::doWork: couldn't update " << m_pDoc->getLocation() << endl;
#endif
			}
			else
			{
				unsigned int docId = 0;
#ifdef DEBUG
				clog << "IndexingThread::doWork: " << m_docInfo.getLabels().size()
					<< " labels for URL " << m_pDoc->getLocation() << endl;
#endif

				// Index the document
				success = wrapFilter.indexDocument(*m_pDoc, m_docInfo.getLabels(), docId);
				if (success == true)
				{
					m_docId = docId;
#ifdef DEBUG
					clog << "IndexingThread::doWork: indexed " << m_pDoc->getLocation()
						<< " to " << m_docId << endl;
#endif
				}
#ifdef DEBUG
				else clog << "IndexingThread::doWork: couldn't index " << m_pDoc->getLocation() << endl;
#endif
			}

			if (success == false)
			{
				m_errorNum = INDEXING_FAILED;
				m_errorParam = m_docInfo.getLocation();
			}
			else
			{
				// Flush the index ?
				if (m_immediateFlush == true)
				{
					m_pIndex->flush();
				}

				// The document properties may have changed
				m_pIndex->getDocumentInfo(m_docId, m_docInfo);
				m_docInfo.setIsIndexed(
					PinotSettings::getInstance().getIndexPropertiesByLocation(m_indexLocation).m_id,
					m_docId);

				clog << "Indexed " << m_docInfo.getLocation() << " in " << indexTimer.stop() << " ms" << endl;
			}
		}
	}
#ifdef DEBUG
	else clog << "IndexingThread::doWork: couldn't download " << m_docInfo.getLocation() << endl;
#endif
}

UnindexingThread::UnindexingThread(const set<unsigned int> &docIdList) :
	WorkerThread(),
	m_indexLocation(PinotSettings::getInstance().m_docsIndexLocation),
	m_docsCount(0)
{
	copy(docIdList.begin(), docIdList.end(), inserter(m_docIdList, m_docIdList.begin()));
}

UnindexingThread::UnindexingThread(const set<string> &labelNames, const string &indexLocation) :
	WorkerThread(),
	m_indexLocation(indexLocation),
	m_docsCount(0)
{
	copy(labelNames.begin(), labelNames.end(), inserter(m_labelNames, m_labelNames.begin()));
	if (indexLocation.empty() == true)
	{
		m_indexLocation = PinotSettings::getInstance().m_docsIndexLocation;
	}
}

UnindexingThread::~UnindexingThread()
{
}

string UnindexingThread::getType(void) const
{
	return "UnindexingThread";
}

unsigned int UnindexingThread::getDocumentsCount(void) const
{
	return m_docsCount;
}

void UnindexingThread::doWork(void)
{
	IndexInterface *pIndex = PinotSettings::getInstance().getIndex(m_indexLocation);

	if ((pIndex == NULL) ||
		(pIndex->isGood() == false))
	{
		m_errorNum = INDEX_ERROR;
		m_errorParam = m_indexLocation;
		if (pIndex != NULL)
		{
			delete pIndex;
		}
		return;
	}

	// Be pessimistic and assume something will go wrong ;-)
	m_errorNum = UNINDEXING_FAILED;

	// Are we supposed to remove documents based on labels ?
	if (m_docIdList.empty() == true)
	{
		// Yep, delete documents one label at a time
		for (set<string>::iterator iter = m_labelNames.begin(); iter != m_labelNames.end(); ++iter)
		{
			string labelName = (*iter);

			// By unindexing all documents that match the label,
			// we effectively delete the label from the index
			if (pIndex->unindexDocuments(labelName, IndexInterface::BY_LABEL) == true)
			{
#ifdef DEBUG
				clog << "UnindexingThread::doWork: removed label " << labelName << endl;
#endif
				// OK
				++m_docsCount;
			}
#ifdef DEBUG
			else clog << "UnindexingThread::doWork: couldn't remove label " << labelName << endl;
#endif
		}

		// Nothing to report
		m_errorNum = 0;
	}
	else
	{
		for (set<unsigned int>::iterator iter = m_docIdList.begin(); iter != m_docIdList.end(); ++iter)
		{
			unsigned int docId = (*iter);

			if (pIndex->unindexDocument(docId) == true)
			{
#ifdef DEBUG
				clog << "UnindexingThread::doWork: removed " << docId << endl;
#endif
				// OK
				++m_docsCount;
			}
#ifdef DEBUG
			else clog << "UnindexingThread::doWork: couldn't remove " << docId << endl;
#endif
		}
#ifdef DEBUG
		clog << "UnindexingThread::doWork: removed " << m_docsCount << " documents" << endl;
#endif
	}

	if (m_docsCount > 0)
	{
		// Flush the index ?
		if (m_immediateFlush == true)
		{
			pIndex->flush();
		}

		// Nothing to report
		m_errorNum = 0;
	}

	delete pIndex;
}

HistoryMonitorThread::HistoryMonitorThread(MonitorInterface *pMonitor, MonitorHandler *pHandler) :
	MonitorThread(pMonitor, pHandler),
	m_crawlHistory(PinotSettings::getInstance().getHistoryDatabaseName())
{
}

HistoryMonitorThread::~HistoryMonitorThread()
{
}

bool HistoryMonitorThread::isFileBlacklisted(const string &location)
{
	return PinotSettings::getInstance().isBlackListed(location);
}

void HistoryMonitorThread::fileModified(const string &location)
{ 
	CrawlHistory::CrawlStatus status = CrawlHistory::UNKNOWN;
	struct stat fileStat;
	time_t itemDate = 0;

	if (m_crawlHistory.hasItem("file://" + location, status, itemDate) == true)
	{
		// Was the file actually modified ?
		if ((stat(location.c_str(), &fileStat) == 0) &&
				(itemDate < fileStat.st_mtime))
		{
			m_pHandler->fileModified(location);
		}
#ifdef DEBUG
		else clog << "HistoryMonitorThread::fileModified: file wasn't modified" << endl;
#endif
	}
#ifdef DEBUG
	else clog << "HistoryMonitorThread::fileModified: file wasn't crawled" << endl;
#endif
}

DirectoryScannerThread::DirectoryScannerThread(const DocumentInfo &docInfo,
	const string &indexLocation, unsigned int maxLevel,
	bool inlineIndexing, bool followSymLinks) :
	IndexingThread(docInfo, indexLocation),
	m_currentLevel(0),
	m_maxLevel(maxLevel),
	m_inlineIndexing(inlineIndexing),
	m_followSymLinks(followSymLinks)
{
	Url urlObj(docInfo.getLocation());

	m_dirName = urlObj.getLocation() + "/" + urlObj.getFile();
}

DirectoryScannerThread::~DirectoryScannerThread()
{
}

string DirectoryScannerThread::getType(void) const
{
	if (m_inlineIndexing == true)
	{
		return IndexingThread::getType();
	}

	return "DirectoryScannerThread";
}

string DirectoryScannerThread::getDirectory(void) const
{
	return m_dirName;
}

void DirectoryScannerThread::stop(void)
{
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
	WorkerThread::stop();
}

sigc::signal2<void, DocumentInfo, bool>& DirectoryScannerThread::getFileFoundSignal(void)
{
	return m_signalFileFound;
}

void DirectoryScannerThread::recordCrawled(const string &location, time_t itemDate)
{
	// Nothing to do by default
}

bool DirectoryScannerThread::isIndexable(const string &entryName) const
{
	string entryDir(path_get_dirname(entryName) + "/");

	// Is this under the directory being scanned ?
	if ((entryDir.length() >= m_dirName.length()) &&
		(entryDir.substr(0, m_dirName.length()) == m_dirName))
	{
		// Yes, it is
#ifdef DEBUG
		clog << "DirectoryScannerThread::isIndexable: under " << m_dirName << endl;
#endif
		return true;
	}

	return false;
}

bool DirectoryScannerThread::wasCrawled(const string &location, time_t &itemDate)
{
	// This information is unknown
	return false;
}

void DirectoryScannerThread::recordCrawling(const string &location, bool itemExists, time_t &itemDate)
{
	// Nothing to do by default
}

void DirectoryScannerThread::recordError(const string &location, int errorCode)
{
	// Nothing to do by default
}

void DirectoryScannerThread::recordSymlink(const string &location, time_t itemDate)
{
	// Nothing to do by default
}

bool DirectoryScannerThread::monitorEntry(const string &entryName)
{
	// Nothing to do by default
	return true;
}

void DirectoryScannerThread::unmonitorEntry(const string &entryName)
{
	// Nothing to do by default
}

void DirectoryScannerThread::foundFile(const DocumentInfo &docInfo)
{
	if ((docInfo.getLocation().empty() == true) ||
		(m_done == true))
	{
		return;
	}

	if (m_inlineIndexing == true)
	{
		// Reset base class members
		m_docInfo = docInfo;
		m_docId = 0;
		m_update = false;

		IndexingThread::doWork();
#ifdef DEBUG
		clog << "DirectoryScannerThread::foundFile: indexed " << docInfo.getLocation() << " to " << m_docId << endl;
#endif
	}
	else
	{
		// Delegate indexing
		// Report everything as file to avoid triggering another crawl
		m_signalFileFound(docInfo, false);
	}
}

bool DirectoryScannerThread::scanEntry(const string &entryName,
	int &entryStatus, bool statLinks)
{
	string location("file://" + entryName);
	DocumentInfo docInfo("", location, "", "");
	time_t itemDate = time(NULL);
	struct stat fileStat;
	bool scanSuccess = true, reportFile = false, itemExists = false;

	if (entryName.empty() == true)
	{
		return false;
	}

	// Skip . .. and dotfiles
	Url urlObj(location);
	if (urlObj.getFile()[0] == '.')
	{
#ifdef DEBUG
		clog << "DirectoryScannerThread::scanEntry: skipped dotfile " << urlObj.getFile() << endl;
#endif
		return false;
	}
#ifdef DEBUG
	clog << "DirectoryScannerThread::scanEntry: checking " << entryName << endl;
#endif

#ifdef HAVE_LSTAT
	// Stat links, or the stuff it refers to ?
	if (statLinks == true)
	{
		entryStatus = lstat(entryName.c_str(), &fileStat);
	}
	else
	{
#endif
		entryStatus = stat(entryName.c_str(), &fileStat);
#ifdef HAVE_LSTAT
	}
#endif

	if (entryStatus == -1)
	{
		entryStatus = errno;
		scanSuccess = false;
#ifdef DEBUG
		clog << "DirectoryScannerThread::scanEntry: stat failed with error " << entryStatus << endl;
#endif
	}
#ifdef HAVE_LSTAT
	// Special processing applies if it's a symlink
	else if (S_ISLNK(fileStat.st_mode))
	{
		string realEntryName(entryName);
		string entryNameReferree;
		bool isInIndexableLocation = false;

		// If symlinks are followed, check if this symlink is blacklisted
		if ((m_followSymLinks == false) ||
			(PinotSettings::getInstance().isBlackListed(entryName) == true))
		{
#ifdef DEBUG
			clog << "DirectoryScannerThread::scanEntry: skipped symlink " << entryName << endl;
#endif
			return false;
		}

		// Are we already following a symlink to a directory ?
		if (m_currentLinks.empty() == false)
		{
			string linkToDir(m_currentLinks.top() + "/");

			// Yes, we are
			if ((entryName.length() > linkToDir.length()) &&
				(entryName.substr(0, linkToDir.length()) == linkToDir))
			{
				// ...and this entry is below it
				realEntryName.replace(0, linkToDir.length() - 1, m_currentLinkReferrees.top());
#ifdef DEBUG
				clog << "DirectoryScannerThread::scanEntry: really at " << realEntryName << endl;
#endif
				isInIndexableLocation = isIndexable(realEntryName);
			}
		}

		char *pBuf = g_file_read_link(realEntryName.c_str(), NULL);
		if (pBuf != NULL)
		{
			string linkLocation(filename_to_utf8(pBuf));
			if (path_is_absolute(linkLocation) == true)
			{
				entryNameReferree = linkLocation;
			}
			else
			{
				string entryDir(path_get_dirname(realEntryName));

				entryNameReferree = Url::resolvePath(entryDir, linkLocation);
			}

			if (entryNameReferree[entryNameReferree.length() - 1] == '/')
			{
				// Drop the terminating slash
				entryNameReferree.resize(entryNameReferree.length() - 1);
			}
#ifdef DEBUG
			clog << "DirectoryScannerThread::scanEntry: symlink resolved to " << entryNameReferree << endl;
#endif

			g_free(pBuf);
		}

		string referreeLocation("file://" + entryNameReferree);
		time_t referreeItemDate;

		// Check whether this will be, or has already been crawled
		// Referrees in indexable locations will be indexed later on
		if ((isInIndexableLocation == false) &&
			(isIndexable(entryNameReferree) == false) &&
			(wasCrawled(referreeLocation, referreeItemDate) == false))
		{
			m_currentLinks.push(entryName);
			m_currentLinkReferrees.push(entryNameReferree);

			// Add a dummy entry for this referree
			// It will ensure it's not indexed more than once and it shouldn't do any harm
			recordSymlink(referreeLocation, itemDate);

			// Do it again, this time by stat'ing what the link refers to
			bool scannedReferree = scanEntry(entryName, entryStatus, false);

			m_currentLinks.pop();
			m_currentLinkReferrees.pop();

			return scannedReferree;
		}
		else
		{
			clog << "Skipping " << entryName << ": it links to " << entryNameReferree
				<< " which will be crawled, or has already been crawled" << endl;

			// This should ensure that only metadata is indexed
			docInfo.setType("inode/symlink");
			reportFile = true;
		}
	}
#endif

	// Is this item in the database already ?
	itemExists = wasCrawled(location, itemDate);
	// Put it in if necessary
	recordCrawling(location, itemExists, itemDate);

	// If stat'ing didn't fail, see if it's a file or a directory
	if ((entryStatus == 0) &&
		(S_ISREG(fileStat.st_mode)))
	{
		// Is this file blacklisted ?
		// We have to check early so that if necessary the file's status stays at TO_CRAWL
		// and it is removed from the index at the end of this crawl
		if (PinotSettings::getInstance().isBlackListed(entryName) == false)
		{
			reportFile = true;
		}
	}
	else if ((entryStatus == 0) &&
		(S_ISDIR(fileStat.st_mode)))
	{
		docInfo.setType("x-directory/normal");

		// Can we scan this directory ?
		if (((m_maxLevel == 0) ||
			(m_currentLevel < m_maxLevel)) &&
			(PinotSettings::getInstance().isBlackListed(entryName) == false))
		{
			++m_currentLevel;

			// Open the directory
			DIR *pDir = opendir(entryName.c_str());
			if (pDir != NULL)
			{
				// Monitor first so that we don't miss events
				// If monitoring is not possible, record the first case
				if ((monitorEntry(entryName) == false) &&
					(entryStatus != MONITORING_FAILED))
				{
					entryStatus = MONITORING_FAILED;
				}
#ifdef DEBUG
				clog << "DirectoryScannerThread::scanEntry: entering " << entryName << endl;
#endif

				// Iterate through this directory's entries
				struct dirent *pDirEntry = readdir(pDir);
				while ((m_done == false) &&
					(pDirEntry != NULL))
				{
					char *pEntryName = pDirEntry->d_name;

					// Skip . .. and dotfiles
					if ((pEntryName != NULL) &&
						(pEntryName[0] != '.'))
					{
						string subEntryName(entryName);
						int subEntryStatus = 0;

						if (entryName[entryName.length() - 1] != '/')
						{
							subEntryName += "/";
						}
						subEntryName += pEntryName;

						// Scan this entry
						scanEntry(subEntryName, subEntryStatus);
					}

					// Next entry
					pDirEntry = readdir(pDir);
				}
#ifdef DEBUG
				clog << "DirectoryScannerThread::scanEntry: leaving " << entryName << endl;
#endif

				// Close the directory
				closedir(pDir);
				--m_currentLevel;
				reportFile = true;
			}
			else
			{
				entryStatus = errno;
				scanSuccess = false;
#ifdef DEBUG
				clog << "DirectoryScannerThread::scanEntry: opendir failed with error " << entryStatus << endl;
#endif
			}
		}
	}
	// Is it some unknown type ?
	else if ((entryStatus == 0)
#ifdef HAVE_LSTAT
		&& (!S_ISLNK(fileStat.st_mode))
#endif
		)
	{
#ifdef DEBUG
		clog << "DirectoryScannerThread::scanEntry: unknown entry type" << endl;
#endif
		entryStatus = ENOENT;
		scanSuccess = false;
	}

	// Was it modified after the last crawl ?
	if ((itemExists == true) &&
		(itemDate >= fileStat.st_mtime))
	{
		// No, it wasn't
#ifdef DEBUG
		clog << "DirectoryScannerThread::scanEntry: no change to " << location << endl;
#endif
		reportFile = false;
	}

	if (m_done == true)
	{
		// Don't record or report the file
		reportFile = false;
	}
	// Did an error occur ?
	else if (entryStatus != 0)
	{
		// Record this error
		recordError(location, entryStatus);

		if (scanSuccess == false)
		{
			return scanSuccess;
		}
	}
	// History of new or modified files, especially their timestamp, is always updated
	// Others' are updated only if we are doing a full scan because
	// the status has to be reset to CRAWLED, so that they are not unindexed
	else if ((itemExists == false) ||
		(reportFile == true))
	{
		recordCrawled(location, fileStat.st_mtime);
	}

	// If a major error occurred, this won't be true
	if (reportFile == true)
	{
		if (docInfo.getType().empty() == true)
		{
			// Scan the file
			docInfo.setType(MIMEScanner::scanFile(entryName));
		}
		docInfo.setTimestamp(TimeConverter::toTimestamp(fileStat.st_mtime));
		docInfo.setSize(fileStat.st_size);

		foundFile(docInfo);
	}

	return scanSuccess;
}

void DirectoryScannerThread::doWork(void)
{
	Timer scanTimer;
	int entryStatus = 0;

	if (m_dirName.empty() == true)
	{
		return;
	}
	scanTimer.start();

	if (scanEntry(m_dirName, entryStatus) == false)
	{
		if (entryStatus == 0)
		{
			m_errorNum = OPENDIR_FAILED;
		}
		else
		{
			m_errorNum = entryStatus;
		}
		m_errorParam = m_dirName;
	}
	clog << "Scanned " << m_dirName << " in " << scanTimer.stop() << " ms" << endl;
}

