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

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <exception>
#include <iostream>

#include "config.h"
#include "NLS.h"
#include "QueryHistory.h"
#ifdef HAVE_DBUS
#include "DBusIndex.h"
#endif
#include "ModuleFactory.h"
#include "PinotSettings.h"
#include "UIThreads.h"

using namespace Glib;
using namespace std;

IndexBrowserThread::IndexBrowserThread(const PinotSettings::IndexProperties &indexProps,
	unsigned int maxDocsCount, unsigned int startDoc) :
	ListerThread(indexProps, startDoc),
	m_maxDocsCount(maxDocsCount)
{
}

IndexBrowserThread::~IndexBrowserThread()
{
}

void IndexBrowserThread::doWork(void)
{
	set<unsigned int> docIDList;
	set<string> docLabels;
	unsigned int numDocs = 0;

	if (m_indexProps.m_location.empty() == true)
	{
		m_errorNum = UNKNOWN_INDEX;
		m_errorParam = m_indexProps.m_name.c_str();
		return;
	}

	// Get the index at that location
	IndexInterface *pIndex = PinotSettings::getInstance().getIndex(m_indexProps.m_location);
	if ((pIndex == NULL) ||
		(pIndex->isGood() == false))
	{
		m_errorNum = INDEX_ERROR;
		m_errorParam = m_indexProps.m_location;
		if (pIndex != NULL)
		{
			delete pIndex;
		}
		return;
	}

	m_documentsCount = pIndex->getDocumentsCount();
	if (m_documentsCount == 0)
	{
#ifdef DEBUG
		clog << "IndexBrowserThread::doWork: no documents" << endl;
#endif
		return;
	}

#ifdef DEBUG
	clog << "IndexBrowserThread::doWork: " << m_maxDocsCount << " off " << m_documentsCount
		<< " documents to browse, starting at position " << m_startDoc << endl;
#endif
	// FIXME: this should provide documents' info
	pIndex->listDocuments(docIDList, m_maxDocsCount, m_startDoc);

	m_documentsList.clear();
	m_documentsList.reserve(m_maxDocsCount);

	for (set<unsigned int>::iterator iter = docIDList.begin(); iter != docIDList.end(); ++iter)
	{
		unsigned int docId = (*iter);

		if (m_done == true)
		{
			break;
		}

		DocumentInfo docInfo;
		if (pIndex->getDocumentInfo(docId, docInfo) == true)
		{
			string type(docInfo.getType());

			if (type.empty() == true)
			{
				docInfo.setType("text/html");
			}
			docInfo.setIsIndexed(m_indexProps.m_id, docId);

			// Insert that document
			m_documentsList.push_back(docInfo);
			++numDocs;
		}
#ifdef DEBUG
		else clog << "IndexBrowserThread::doWork: couldn't retrieve document " << docId << endl;
#endif
	}
	delete pIndex;
}

EngineHistoryThread::EngineHistoryThread(const string &engineDisplayableName,
	const QueryProperties &queryProps, unsigned int maxDocsCount) :
	QueryingThread("", engineDisplayableName, "", queryProps, 0),
	m_maxDocsCount(maxDocsCount)
{
	// Results are converted to UTF-8 prior to insertion in the history database
	m_resultsCharset = "UTF-8";
	m_isLive = false;
}

EngineHistoryThread::~EngineHistoryThread()
{
}

void EngineHistoryThread::doWork(void)
{
	QueryHistory queryHistory(PinotSettings::getInstance().getHistoryDatabaseName());

	if (queryHistory.getItems(m_queryProps.getName(), m_engineDisplayableName,
		m_maxDocsCount, m_documentsList) == false)
	{
		m_errorNum = HISTORY_FAILED;
		m_errorParam = m_engineDisplayableName;
	}
	else if (m_documentsList.empty() == false)
	{
		// Get the first result's charset
		queryHistory.getItemExtract(m_queryProps.getName(), m_engineDisplayableName,
			m_documentsList.front().getLocation(true));
	}
}

ExpandQueryThread::ExpandQueryThread(const QueryProperties &queryProps,
	const set<string> &expandFromDocsSet) :
	WorkerThread(),
	m_queryProps(queryProps)
{
	copy(expandFromDocsSet.begin(), expandFromDocsSet.end(),
		inserter(m_expandFromDocsSet, m_expandFromDocsSet.begin()));
}

ExpandQueryThread::~ExpandQueryThread()
{
}

string ExpandQueryThread::getType(void) const
{
	return "ExpandQueryThread";
}

QueryProperties ExpandQueryThread::getQuery(void) const
{
	return m_queryProps;
}

const set<string> &ExpandQueryThread::getExpandTerms(void) const
{
	return m_expandTerms;
}

void ExpandQueryThread::doWork(void)
{
	// Get the SearchEngine
	SearchEngineInterface *pEngine = ModuleFactory::getSearchEngine(PinotSettings::getInstance().m_defaultBackend, "MERGED");
	if (pEngine == NULL)
	{
		m_errorNum = UNKNOWN_ENGINE;
		m_errorParam = m_queryProps.getName();
		return;
	}

	// Expand the query
	pEngine->setExpandSet(m_expandFromDocsSet);

	// Run the query
	pEngine->setDefaultOperator(SearchEngineInterface::DEFAULT_OP_AND);
	if (pEngine->runQuery(m_queryProps) == false)
	{
		m_errorNum = QUERY_FAILED;
	}
	else
	{
		// Copy the expand terms
		const set<string> &expandTerms = pEngine->getExpandTerms();
		copy(expandTerms.begin(), expandTerms.end(),
			inserter(m_expandTerms, m_expandTerms.begin()));
	}

	delete pEngine;
}

LabelUpdateThread::LabelUpdateThread(const set<string> &labelsToAdd,
	const set<string> &labelsToDelete) :
	WorkerThread(),
	m_resetLabels(false)
{
	copy(labelsToAdd.begin(), labelsToAdd.end(),
		inserter(m_labelsToAdd, m_labelsToAdd.begin()));
	copy(labelsToDelete.begin(), labelsToDelete.end(),
		inserter(m_labelsToDelete, m_labelsToDelete.begin()));
}

LabelUpdateThread::LabelUpdateThread(const set<string> &labelsToAdd,
	const set<unsigned int> &docsIds, const set<unsigned int> &daemonIds,
	bool resetLabels) :
	WorkerThread(),
	m_resetLabels(resetLabels)
{
	copy(labelsToAdd.begin(), labelsToAdd.end(),
		inserter(m_labelsToAdd, m_labelsToAdd.begin()));
	copy(docsIds.begin(), docsIds.end(),
		inserter(m_docsIds, m_docsIds.begin()));
	copy(daemonIds.begin(), daemonIds.end(),
		inserter(m_daemonIds, m_daemonIds.begin()));
}

LabelUpdateThread::~LabelUpdateThread()
{
}

string LabelUpdateThread::getType(void) const
{
	return "LabelUpdateThread";
}

const set<unsigned int> &LabelUpdateThread::getDocIds(void) const
{
	return m_docsIds;
}

const set<unsigned int> &LabelUpdateThread::getDaemonIds(void) const
{
	return m_daemonIds;
}

const set<string> &LabelUpdateThread::getLabels(void) const
{
	return m_labelsToAdd;
}

bool LabelUpdateThread::resetLabels(void) const
{
	return m_resetLabels;
}

void LabelUpdateThread::doWork(void)
{
	bool actOnDocuments = false;

	IndexInterface *pDocsIndex = PinotSettings::getInstance().getIndex(PinotSettings::getInstance().m_docsIndexLocation);
	if (pDocsIndex == NULL)
	{
		m_errorNum = INDEX_ERROR;
		m_errorParam = PinotSettings::getInstance().m_docsIndexLocation;
		return;
	}

	IndexInterface *pDaemonIndex = PinotSettings::getInstance().getIndex(PinotSettings::getInstance().m_daemonIndexLocation);
	if (pDaemonIndex == NULL)
	{
		m_errorNum = INDEX_ERROR;
		m_errorParam = PinotSettings::getInstance().m_daemonIndexLocation;
		delete pDocsIndex;
		return;
	}

	// Apply the labels to existing documents
	if (m_docsIds.empty() == false)
	{
		pDocsIndex->setDocumentsLabels(m_docsIds, m_labelsToAdd, m_resetLabels);
		actOnDocuments = true;
	}
	if (m_daemonIds.empty() == false)
	{
		pDaemonIndex->setDocumentsLabels(m_daemonIds, m_labelsToAdd, m_resetLabels);
		actOnDocuments = true;
	}

	if (actOnDocuments == false)
	{
		// Add and/or delete labels on the daemon's index only
		// The documents index is not required to have labels set
		for (set<string>::iterator iter = m_labelsToAdd.begin(); iter != m_labelsToAdd.end(); ++iter)
		{
			pDaemonIndex->addLabel(*iter);
		}
		for (set<string>::iterator iter = m_labelsToDelete.begin(); iter != m_labelsToDelete.end(); ++iter)
		{
			pDaemonIndex->deleteLabel(*iter);
		}
	}

	delete pDaemonIndex;
	delete pDocsIndex;
}

UpdateDocumentThread::UpdateDocumentThread(const PinotSettings::IndexProperties &indexProps, unsigned int docId,
	const DocumentInfo &docInfo, bool updateLabels) :
	WorkerThread(),
	m_indexProps(indexProps),
	m_docId(docId),
	m_docInfo(docInfo),
	m_updateLabels(updateLabels)
{
}

UpdateDocumentThread::~UpdateDocumentThread()
{
}

string UpdateDocumentThread::getType(void) const
{
	return "UpdateDocumentThread";
}

PinotSettings::IndexProperties UpdateDocumentThread::getIndexProperties(void) const
{
	return m_indexProps;
}

const DocumentInfo &UpdateDocumentThread::getDocumentInfo(void) const
{
	return m_docInfo;
}

void UpdateDocumentThread::doWork(void)
{
	if (m_done == false)
	{
		if (m_indexProps.m_location.empty() == true)
		{
			m_errorNum = UNKNOWN_INDEX;
			m_errorParam = m_indexProps.m_name.c_str();
			return;
		}

		// Get the index at that location
		IndexInterface *pIndex = PinotSettings::getInstance().getIndex(m_indexProps.m_location);
		if ((pIndex == NULL) ||
			(pIndex->isGood() == false))
		{
			m_errorNum = INDEX_ERROR;
			m_errorParam = m_indexProps.m_location;
			if (pIndex != NULL)
			{
				delete pIndex;
			}
			return;
		}

		// Update the DocumentInfo
		if (pIndex->updateDocumentInfo(m_docId, m_docInfo) == false)
		{
			m_errorNum = UPDATE_FAILED;
			m_errorParam = m_docInfo.getLocation(true);
			return;
		}
		// ...and the labels if necessary
		if (m_updateLabels == true)
		{
			if (pIndex->setDocumentLabels(m_docId, m_docInfo.getLabels()) == false)
			{
				m_errorNum = UPDATE_FAILED;
				m_errorParam = m_docInfo.getLocation(true);
				return;
			}
		}

		// Flush the index ?
		if (m_immediateFlush == true)
		{
			pIndex->flush();
		}

		delete pIndex;
	}
}

#ifdef HAVE_DBUS
DaemonStatusThread::DaemonStatusThread() :
	WorkerThread(),
	m_gotStats(false),
	m_lowDiskSpace(false),
	m_onBattery(false),
	m_crawling(false),
	m_crawledCount(0),
	m_docsCount(0)
{
}

DaemonStatusThread::~DaemonStatusThread()
{
}

string DaemonStatusThread::getType(void) const
{
	return "DaemonStatusThread";
}

void DaemonStatusThread::doWork(void)
{
	// We need a pure DBusIndex object
	DBusIndex index(NULL);

	if (index.getStatistics(m_crawledCount, m_docsCount,
		m_lowDiskSpace, m_onBattery, m_crawling) == true)
	{
		m_gotStats = true;
	}
#ifdef DEBUG
	else clog << "DaemonStatusThread::doWork: failed to get statistics" << endl;
#endif
}
#endif

