/*
 *  Copyright 2009-2021 Fabrice Colin
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

#ifndef _UITHREADS_HH
#define _UITHREADS_HH

#include <string>
#include <set>
#include <glibmm/ustring.h>

#include "DocumentInfo.h"
#include "QueryProperties.h"
#include "WorkerThreads.h"

class IndexBrowserThread : public ListerThread
{
	public:
		IndexBrowserThread(const PinotSettings::IndexProperties &indexProps,
			unsigned int maxDocsCount, unsigned int startDoc = 0);
		~IndexBrowserThread();

		std::string getLabelName(void) const;

	protected:
		unsigned int m_maxDocsCount;

		virtual void doWork(void);

	private:
		IndexBrowserThread(const IndexBrowserThread &other);
		IndexBrowserThread &operator=(const IndexBrowserThread &other);

};

class EngineHistoryThread : public QueryingThread
{
	public:
		EngineHistoryThread(const std::string &engineDisplayableName,
			const QueryProperties &queryProps, unsigned int maxDocsCount);
		virtual ~EngineHistoryThread();

	protected:
		unsigned int m_maxDocsCount;

		virtual void doWork(void);

	private:
		EngineHistoryThread(const EngineHistoryThread &other);
		EngineHistoryThread &operator=(const EngineHistoryThread &other);

};

class ExpandQueryThread : public WorkerThread
{
	public:
		ExpandQueryThread(const QueryProperties &queryProps,
			const std::set<std::string> &expandFromDocsSet);
		virtual ~ExpandQueryThread();

		virtual std::string getType(void) const;

		QueryProperties getQuery(void) const;

		const std::set<std::string> &getExpandTerms(void) const;

	protected:
		QueryProperties m_queryProps;
		std::set<std::string> m_expandFromDocsSet;
		std::set<std::string> m_expandTerms;

		virtual void doWork(void);

	private:
		ExpandQueryThread(const ExpandQueryThread &other);
		ExpandQueryThread &operator=(const ExpandQueryThread &other);

};

class LabelUpdateThread : public WorkerThread
{
	public:
		LabelUpdateThread(const std::set<std::string> &labelsToAdd,
			const std::set<std::string> &labelsToDelete);
		LabelUpdateThread(const std::set<std::string> &labelsToAdd,
			const std::set<unsigned int> &docsIds,
			const std::set<unsigned int> &daemonIds,
			bool resetLabels);

		virtual ~LabelUpdateThread();

		virtual std::string getType(void) const;

		bool modifiedDocsIndex(void) const;

		bool modifiedDaemonIndex(void) const;

	protected:
		std::set<std::string> m_labelsToAdd;
		std::set<std::string> m_labelsToDelete;
		std::set<unsigned int> m_docsIds;
		std::set<unsigned int> m_daemonIds;
		bool m_resetLabels;

		virtual void doWork(void);

	private:
		LabelUpdateThread(const LabelUpdateThread &other);
		LabelUpdateThread &operator=(const LabelUpdateThread &other);

};

class UpdateDocumentThread : public WorkerThread
{
	public:
		// Update a document's properties
		UpdateDocumentThread(const PinotSettings::IndexProperties &indexProps,
			unsigned int docId, const DocumentInfo &docInfo,
			bool updateLabels);
		virtual ~UpdateDocumentThread();

		virtual std::string getType(void) const;

		PinotSettings::IndexProperties getIndexProperties(void) const;

		unsigned int getDocumentID(void) const;

		const DocumentInfo &getDocumentInfo(void) const;

	protected:
		PinotSettings::IndexProperties m_indexProps;
		unsigned int m_docId;
		DocumentInfo m_docInfo;
		bool m_updateLabels;

		virtual void doWork(void);

	private:
		UpdateDocumentThread(const UpdateDocumentThread &other);
		UpdateDocumentThread &operator=(const UpdateDocumentThread &other);

};

#ifdef HAVE_DBUS
class DaemonStatusThread : public WorkerThread
{
	public:
		DaemonStatusThread();
		virtual ~DaemonStatusThread();

		virtual std::string getType(void) const;

		bool m_gotStats;
		bool m_lowDiskSpace;
		bool m_onBattery;
		bool m_crawling;
		unsigned int m_crawledCount;
		unsigned int m_docsCount;

	protected:
		virtual void doWork(void);

	private:
		DaemonStatusThread(const DaemonStatusThread &other);
		DaemonStatusThread &operator=(const DaemonStatusThread &other);

};
#endif

#endif // _UITHREADS_HH
