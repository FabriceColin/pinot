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

#ifndef _WORKERTHREADS_HH
#define _WORKERTHREADS_HH

#include <time.h>
#include <string>
#include <vector>
#include <queue>
#include <set>
#include <map>
#include <stack>
#include <pthread.h>
#include <sigc++/sigc++.h>
#include <glibmm/dispatcher.h>
#include <glibmm/thread.h>
#include <glibmm/ustring.h>

#include "Document.h"
#include "ActionQueue.h"
#include "CrawlHistory.h"
#include "DownloaderInterface.h"
#include "QueryProperties.h"
#include "PinotSettings.h"
#include "WorkerThread.h"

class QueueManager : public ThreadsManager
{
	public:
		QueueManager(const std::string &defaultIndexLocation,
			unsigned int maxThreadsTime = 300,
			bool scanLocalFiles = false);
		virtual ~QueueManager();

		virtual Glib::ustring queue_index(const DocumentInfo &docInfo);

		virtual bool pop_queue(const std::string &urlWasIndexed = "");

	protected:
		bool m_scanLocalFiles;
		bool m_stopIndexing;
		ActionQueue m_actionQueue;

		Glib::ustring index_document(const DocumentInfo &docInfo);

		virtual void clear_queues(void);

	private:
		QueueManager(const QueueManager &other);
		QueueManager &operator=(const QueueManager &other);

};

class ListerThread : public WorkerThread
{
	public:
		ListerThread(const PinotSettings::IndexProperties &indexProps,
			unsigned int startDoc);
		~ListerThread();

		std::string getType(void) const;

		PinotSettings::IndexProperties getIndexProperties(void) const;

		unsigned int getStartDoc(void) const;

		const std::vector<DocumentInfo> &getDocuments(void) const;

		unsigned int getDocumentsCount(void) const;

	protected:
		PinotSettings::IndexProperties m_indexProps;
		unsigned int m_startDoc;
		std::vector<DocumentInfo> m_documentsList;
		unsigned int m_documentsCount;

	private:
		ListerThread(const ListerThread &other);
		ListerThread &operator=(const ListerThread &other);

};

class QueryingThread : public ListerThread
{
	public:
		QueryingThread(const PinotSettings::IndexProperties &indexProps,
			const QueryProperties &queryProps, unsigned int startDoc = 0,
			bool listingIndex = false);
		QueryingThread(const std::string &engineName, const std::string &engineDisplayableName,
			const std::string &engineOption, const QueryProperties &queryProps,
			unsigned int startDoc = 0);
		virtual ~QueryingThread();

		virtual std::string getType(void) const;

		bool isLive(void) const;

		std::string getEngineName(void) const;

		QueryProperties getQuery(bool &wasCorrected) const;

		std::string getCharset(void) const;

	protected:
		std::string m_engineName;
		std::string m_engineDisplayableName;
		std::string m_engineOption;
		QueryProperties m_queryProps;
		std::string m_resultsCharset;
		bool m_listingIndex;
		bool m_correctedSpelling;
		bool m_isLive;

		bool findPlugin(void);

	private:
		QueryingThread(const QueryingThread &other);
		QueryingThread &operator=(const QueryingThread &other);

};

class EngineQueryThread : public QueryingThread
{
	public:
		EngineQueryThread(const PinotSettings::IndexProperties &indexProps,
			const QueryProperties &queryProps, unsigned int startDoc = 0,
			bool listingIndex = false);
		EngineQueryThread(const PinotSettings::IndexProperties &indexProps,
			const QueryProperties &queryProps,
			const std::set<std::string> &limitToDocsSet, unsigned int startDoc = 0);
		EngineQueryThread(const std::string &engineName, const std::string &engineDisplayableName,
			const std::string &engineOption, const QueryProperties &queryProps,
			unsigned int startDoc = 0);
		virtual ~EngineQueryThread();

	protected:
		std::set<std::string> m_limitToDocsSet;

		virtual void processResults(const std::vector<DocumentInfo> &resultsList);

		virtual void processResults(const std::vector<DocumentInfo> &resultsList,
			unsigned int indexId);

		virtual void doWork(void);

	private:
		EngineQueryThread(const EngineQueryThread &other);
		EngineQueryThread &operator=(const EngineQueryThread &other);

};

class DownloadingThread : public WorkerThread
{
	public:
		DownloadingThread(const DocumentInfo &docInfo);
		virtual ~DownloadingThread();

		virtual std::string getType(void) const;

		std::string getURL(void) const;

		const Document *getDocument(void) const;

	protected:
		DocumentInfo m_docInfo;
		Document *m_pDoc;
		DownloaderInterface *m_pDownloader;
		std::string m_protocol;

		DownloadingThread();

		virtual void doWork(void);

	private:
		DownloadingThread(const DownloadingThread &other);
		DownloadingThread &operator=(const DownloadingThread &other);

};

class IndexingThread : public DownloadingThread
{
	public:
		IndexingThread(const DocumentInfo &docInfo, const std::string &indexLocation,
			bool allowAllMIMETypes = true);
		IndexingThread(const std::string &indexLocation);
		virtual ~IndexingThread();

		virtual std::string getType(void) const;

		const DocumentInfo &getDocumentInfo(void) const;

		std::string getLabelName(void) const;

		unsigned int getDocumentID(void) const;

		bool isNewDocument(void) const;

	protected:
		IndexInterface *m_pIndex;
		std::string m_indexLocation;
		bool m_allowAllMIMETypes;
		bool m_update;
		unsigned int m_docId;

		IndexingThread();

		virtual void doWork(void);

	private:
		IndexingThread(const IndexingThread &other);
		IndexingThread &operator=(const IndexingThread &other);

};

class UnindexingThread : public WorkerThread
{
	public:
		// Unindex documents from the internal index
		UnindexingThread(const std::set<unsigned int> &docIdList);
		// Unindex from the given index documents that have one of the labels
		UnindexingThread(const std::set<std::string> &labelNames, const std::string &indexLocation);
		virtual ~UnindexingThread();

		virtual std::string getType(void) const;

		unsigned int getDocumentsCount(void) const;

	protected:
		std::set<unsigned int> m_docIdList;
		std::set<std::string> m_labelNames;
		std::string m_indexLocation;
		unsigned int m_docsCount;

		virtual void doWork(void);

	private:
		UnindexingThread(const UnindexingThread &other);
		UnindexingThread &operator=(const UnindexingThread &other);

};

class HistoryMonitorThread : public MonitorThread
{
	public:
		HistoryMonitorThread(MonitorInterface *pMonitor, MonitorHandler *pHandler);
		virtual ~HistoryMonitorThread();

	protected:
		CrawlHistory m_crawlHistory;

		virtual bool isFileBlacklisted(const std::string &location);
		virtual void fileModified(const std::string &location);

	private:
		HistoryMonitorThread(const HistoryMonitorThread &other);
		HistoryMonitorThread &operator=(const HistoryMonitorThread &other);

};

class DirectoryScannerThread : public IndexingThread
{
	public:
		DirectoryScannerThread(const std::string &dirName,
			const std::string &indexLocation, unsigned int maxLevel = 0,
			bool inlineIndexing = false, bool followSymLinks = true);
		virtual ~DirectoryScannerThread();

		virtual std::string getType(void) const;

		virtual std::string getDirectory(void) const;

		virtual void stop(void);

		sigc::signal2<void, DocumentInfo, bool>& getFileFoundSignal(void);

	protected:
		std::string m_dirName;
		unsigned int m_currentLevel;
		unsigned int m_maxLevel;
		bool m_inlineIndexing;
		bool m_followSymLinks;
		sigc::signal2<void, DocumentInfo, bool> m_signalFileFound;
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

		bool scanEntry(const std::string &entryName,
			bool statLinks = true);
		virtual void doWork(void);

	private:
		DirectoryScannerThread(const DirectoryScannerThread &other);
		DirectoryScannerThread &operator=(const DirectoryScannerThread &other);

};

#endif // _WORKERTHREADS_HH
