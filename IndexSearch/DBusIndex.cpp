/*
 *  Copyright 2007-2021 Fabrice Colin
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

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <sstream>
#include <typeinfo>

#include "Languages.h"
#include "DBusIndex.h"

using std::clog;
using std::clog;
using std::endl;
using std::string;
using std::stringstream;
using std::vector;
using std::set;
using std::map;
using std::min;
using std::tuple;

using namespace Gio;
using namespace Glib;

static const char *g_fieldNames[] = { "caption", "url", "type", "language", "modtime", "size", "extract", "score", NULL };

DBusIndex::DBusIndex(IndexInterface *pROIndex) :
	IndexInterface(),
	m_refProxy(com::github::fabricecolin::PinotProxy::createForBus_sync(
		DBus::BUS_TYPE_SESSION,
		Gio::DBus::PROXY_FLAGS_DO_NOT_AUTO_START_AT_CONSTRUCTION,
		PINOT_DBUS_SERVICE_NAME, PINOT_DBUS_OBJECT_PATH)),
	m_pROIndex(pROIndex)
{
}

DBusIndex::DBusIndex(const DBusIndex &other) :
	IndexInterface(other),
	m_pROIndex(other.m_pROIndex)
{
}

DBusIndex::~DBusIndex()
{
	if (m_pROIndex != NULL)
	{
		// Noone else is going to delete this
		delete m_pROIndex;
	}
}

DBusIndex &DBusIndex::operator=(const DBusIndex &other)
{
	if (this != &other)
	{
		IndexInterface::operator=(other);
		m_pROIndex = other.m_pROIndex;
	}

	return *this;
}

/// Extracts docInfo from tuples.
void DBusIndex::documentInfoFromTuples(const vector<tuple<ustring, ustring>> &tuples,
	DocumentInfo &docInfo)
{
	for (vector<tuple<ustring, ustring>>::const_iterator tupleIter = tuples.begin(); tupleIter != tuples.end(); ++tupleIter)
	{
		ustring fieldName, fieldValue;

		std::tie(fieldName, fieldValue) = *tupleIter;

		// Populate docInfo
		if (fieldName == g_fieldNames[0])
		{
			docInfo.setTitle(fieldValue.c_str());
		}
		else if (fieldName == g_fieldNames[1])
		{
			docInfo.setLocation(fieldValue.c_str());
		}
		else if (fieldName == g_fieldNames[2])
		{
			docInfo.setType(fieldValue.c_str());
		}
		else if (fieldName == g_fieldNames[3])
		{
			docInfo.setLanguage(Languages::toLocale(fieldValue.c_str()));
		}
		else if (fieldName == g_fieldNames[4])
		{
			docInfo.setTimestamp(fieldValue.c_str());
		}
		else if (fieldName == g_fieldNames[5])
		{
			docInfo.setSize((off_t )atoi(fieldValue.c_str()));
		}
		else if (fieldName == g_fieldNames[6])
		{
			docInfo.setExtract(fieldValue.c_str());
		}
		else if (fieldName == g_fieldNames[7])
		{
			docInfo.setScore((float)atof(fieldValue.c_str()));
		}
	}
}

/// Converts docInfo to tuples.
void DBusIndex::documentInfoToTuples(const DocumentInfo &docInfo,
	vector<tuple<ustring, ustring>> &tuples)
{
	for (unsigned int fieldNum = 0; g_fieldNames[fieldNum] != NULL; ++fieldNum)
	{
		string value;
		stringstream numStr;

		switch (fieldNum)
		{
			case 0:
				value = docInfo.getTitle();
				break;
			case 1:
				value = docInfo.getLocation(true);
				break;
			case 2:
				value = docInfo.getType();
				break;
			case 3:
				value = Languages::toEnglish(docInfo.getLanguage());
				break;
			case 4:
				value = docInfo.getTimestamp();
				break;
			case 5:
				numStr << docInfo.getSize();
				value = numStr.str();
				break;
			case 6:
				value = docInfo.getExtract();
				break;
			case 7:
				numStr << docInfo.getScore();
				value = numStr.str();
				break;
			default:
				break;
		}

		if (value.empty() == true)
		{
			continue;
		}

		tuples.push_back(make_tuple(g_fieldNames[fieldNum], value));
	}
}

/// Asks the D-Bus service to reload its configuration.
bool DBusIndex::reload(void)
{
	try
	{
		return m_refProxy->Reload_sync();
	}
	catch (const Glib::Error &ex)
	{
		clog << "DBusIndex::reload: " << ex.what() << endl;
	}

	return false;
}

/// Gets some statistics from the D-Bus service.
bool DBusIndex::getStatistics(unsigned int &crawledCount, unsigned int &docsCount,
	bool &lowDiskSpace, bool &onBattery, bool &crawling)
{
	try
	{
		std::tie(crawledCount, docsCount, lowDiskSpace, onBattery, crawling) = m_refProxy->GetStatistics_sync();
	}
	catch (const Glib::Error &ex)
	{
		clog << "DBusIndex::getStatistics: " << ex.what() << endl;
	}

	return true;
}

//
// Implementation of IndexInterface
//

/// Returns false if the index couldn't be opened.
bool DBusIndex::isGood(void) const
{
	if (m_pROIndex == NULL)
	{
		return false;
	}

	return m_pROIndex->isGood();
}

/// Gets metadata.
string DBusIndex::getMetadata(const string &name) const
{
	if (m_pROIndex == NULL)
	{
		return "";
	}

	return m_pROIndex->getMetadata(name);
}

/// Sets metadata.
bool DBusIndex::setMetadata(const string &name, const string &value) const
{
	clog << "DBusIndex::setMetadata: not allowed" << endl;
	return false;
}

/// Gets the index location.
string DBusIndex::getLocation(void) const
{
	if (m_pROIndex == NULL)
	{
		return "";
	}

	return m_pROIndex->getLocation();
}

/// Returns a document's properties.
bool DBusIndex::getDocumentInfo(unsigned int docId, DocumentInfo &docInfo) const
{
	if (m_pROIndex == NULL)
	{
		return false;
	}

	reopen();

	return m_pROIndex->getDocumentInfo(docId, docInfo);
}

/// Returns a document's terms count.
unsigned int DBusIndex::getDocumentTermsCount(unsigned int docId) const
{
	if (m_pROIndex == NULL)
	{
		return false;
	}

	reopen();

	return m_pROIndex->getDocumentTermsCount(docId);
}

/// Returns a document's terms.
bool DBusIndex::getDocumentTerms(unsigned int docId,
	map<unsigned int, string> &wordsBuffer) const
{
	if (m_pROIndex == NULL)
	{
		return false;
	}

	reopen();

	return m_pROIndex->getDocumentTerms(docId, wordsBuffer);
}

/// Sets the list of known labels.
bool DBusIndex::setLabels(const set<string> &labels, bool resetLabels)
{
	// Not allowed here
	return false;
}

/// Gets the list of known labels.
bool DBusIndex::getLabels(set<string> &labels) const
{
	vector<ustring> labelsList;

	try
	{
		labelsList = m_refProxy->GetLabels_sync();
	}
	catch (const Glib::Error &ex)
	{
		clog << "DBusIndex::getLabels: " << ex.what() << endl;
	}

	for (vector<ustring>::const_iterator labelIter = labelsList.begin(); labelIter != labelsList.end(); ++labelIter)
	{
		labels.insert(labelIter->c_str());
	}

	return true;
}

/// Adds a label.
bool DBusIndex::addLabel(const string &name)
{
	ustring labelName(name.c_str());
	ustring newLabelName;

	try
	{
		newLabelName = m_refProxy->AddLabel_sync(labelName);
	}
	catch (const Glib::Error &ex)
	{
		clog << "DBusIndex::addLabel: " << ex.what() << endl;
	}

	return (newLabelName == labelName);
}

/// Deletes all references to a label.
bool DBusIndex::deleteLabel(const string &name)
{
	ustring labelName(name.c_str());
	ustring deletedLabelName;

	try
	{
		deletedLabelName = m_refProxy->DeleteLabel_sync(labelName);
	}
	catch (const Glib::Error &ex)
	{
		clog << "DBusIndex::deleteLabel: " << ex.what() << endl;
	}

	return (deletedLabelName == labelName);
}

/// Determines whether a document has a label.
bool DBusIndex::hasLabel(unsigned int docId, const string &name) const
{
	if (m_pROIndex == NULL)
	{
		return false;
	}

	reopen();

	return m_pROIndex->hasLabel(docId, name);
}

/// Returns a document's labels.
bool DBusIndex::getDocumentLabels(unsigned int docId, set<string> &labels) const
{
	vector<ustring> labelsList;

	try
	{
		labelsList = m_refProxy->GetDocumentLabels_sync(docId);
	}
	catch (const Glib::Error &ex)
	{
		clog << "DBusIndex::getDocumentLabels: " << ex.what() << endl;
	}
	for (vector<ustring>::const_iterator labelIter = labelsList.begin(); labelIter != labelsList.end(); ++labelIter)
	{
		labels.insert(labelIter->c_str());
	}

	return true;
}

/// Sets a document's labels.
bool DBusIndex::setDocumentLabels(unsigned int docId, const set<string> &labels,
	bool resetLabels)
{
	vector<ustring> labelsList;

	labelsList.reserve(labels.size());
	for (set<string>::const_iterator labelIter = labels.begin(); labelIter != labels.end(); ++labelIter)
	{
		labelsList.push_back(labelIter->c_str());
	}
	try
	{
		m_refProxy->SetDocumentLabels_sync(docId, labelsList, resetLabels);
	}
	catch (const Glib::Error &ex)
	{
		clog << "DBusIndex::setDocumentLabels: " << ex.what() << endl;
	}

	return true;
}

/// Sets documents' labels.
bool DBusIndex::setDocumentsLabels(const set<unsigned int> &docIds,
	const set<string> &labels, bool resetLabels)
{
	vector<ustring> idsList;
	vector<ustring> labelsList;

	idsList.reserve(docIds.size());
	for (set<unsigned int>::const_iterator idIter = docIds.begin(); idIter != docIds.end(); ++idIter)
	{
		stringstream numStr;

		numStr << *idIter;
		idsList.push_back(numStr.str().c_str());
	}
	labelsList.reserve(labels.size());
	for (set<string>::const_iterator labelIter = labels.begin(); labelIter != labels.end(); ++labelIter)
	{
		labelsList.push_back(labelIter->c_str());
	}

	try
	{
		return m_refProxy->SetDocumentsLabels_sync(idsList, labelsList, resetLabels);
	}
	catch (const Glib::Error &ex)
	{
		clog << "DBusIndex::setDocumentsLabels: " << ex.what() << endl;
	}

	return false;
}

/// Checks whether the given URL is in the index.
unsigned int DBusIndex::hasDocument(const string &url) const
{
	try
	{
		return m_refProxy->HasDocument_sync(url.c_str());
	}
	catch (const Glib::Error &ex)
	{
		clog << "DBusIndex::hasDocument: " << ex.what() << endl;
	}

	return 0;
}

/// Gets terms with the same root.
unsigned int DBusIndex::getCloseTerms(const string &term, set<string> &suggestions)
{
	if (m_pROIndex == NULL)
	{
		return false;
	}

	reopen();

	return m_pROIndex->getCloseTerms(term, suggestions);
}

/// Returns the ID of the last document.
unsigned int DBusIndex::getLastDocumentID(void) const
{
	if (m_pROIndex == NULL)
	{
		return false;
	}

	reopen();

	return m_pROIndex->getLastDocumentID();
}

/// Returns the number of documents.
unsigned int DBusIndex::getDocumentsCount(const string &labelName) const
{
	if (m_pROIndex == NULL)
	{
		return false;
	}

	reopen();

	return m_pROIndex->getDocumentsCount(labelName);
}

/// Lists documents.
unsigned int DBusIndex::listDocuments(set<unsigned int> &docIds,
	unsigned int maxDocsCount, unsigned int startDoc) const
{
	if (m_pROIndex == NULL)
	{
		return false;
	}

	reopen();

	return m_pROIndex->listDocuments(docIds, maxDocsCount, startDoc);
}

/// Lists documents.
bool DBusIndex::listDocuments(const string &name, set<unsigned int> &docIds,
	NameType type, unsigned int maxDocsCount, unsigned int startDoc) const
{
	if (m_pROIndex == NULL)
	{
		return false;
	}

	reopen();

	return m_pROIndex->listDocuments(name, docIds, type, maxDocsCount, startDoc);
}

/// Indexes the given data.
bool DBusIndex::indexDocument(const Document &doc, const set<string> &labels,
	unsigned int &docId)
{
	clog << "DBusIndex::indexDocument: not allowed" << endl;
	return false;
}

/// Updates the given document; true if success.
bool DBusIndex::updateDocument(unsigned int docId, const Document &doc)
{
	unsigned updatedDocId;

	try
	{
		updatedDocId = m_refProxy->UpdateDocument_sync(docId);
	}
	catch (const Glib::Error &ex)
	{
		clog << "DBusIndex::updateDocument: " << ex.what() << endl;
	}

	return (updatedDocId == docId);
}

/// Updates a document's properties.
bool DBusIndex::updateDocumentInfo(unsigned int docId, const DocumentInfo &docInfo)
{
	vector<tuple<ustring, ustring>> tuples;
	unsigned updatedDocId = 0;

	documentInfoToTuples(docInfo, tuples);

	try
	{
		updatedDocId = m_refProxy->SetDocumentInfo_sync(docId, tuples);
	}
	catch (const Glib::Error &ex)
	{
		clog << "DBusIndex::updateDocument: " << ex.what() << endl;
	}

	return (updatedDocId == docId);
}

/// Unindexes the given document; true if success.
bool DBusIndex::unindexDocument(unsigned int docId)
{
	clog << "DBusIndex::unindexDocument: not allowed" << endl;
	return false;
}

/// Unindexes the given document.
bool DBusIndex::unindexDocument(const string &location)
{
	clog << "DBusIndex::unindexDocument: not allowed" << endl;
	return false;
}

/// Unindexes documents.
bool DBusIndex::unindexDocuments(const string &name, NameType type)
{
	clog << "DBusIndex::unindexDocuments: not allowed" << endl;
	return false;
}

/// Unindexes all documents.
bool DBusIndex::unindexAllDocuments(void)
{
	clog << "DBusIndex::unindexDocuments: not allowed" << endl;
	return false;
}

/// Flushes recent changes to the disk.
bool DBusIndex::flush(void)
{
	// The daemon knows best when to flush
	return true;
}

/// Reopens the index.
bool DBusIndex::reopen(void) const
{
	if (m_pROIndex == NULL)
	{
		return false;
	}

	return m_pROIndex->reopen();
}

/// Resets the index.
bool DBusIndex::reset(void)
{
	// This can't be done here
	return false;
}

