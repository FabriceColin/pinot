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

#include <time.h>
#include <iostream>

#include "Url.h"
#include "FilterFactory.h"
#include "TextFilter.h"
#include "FilterWrapper.h"

using std::clog;
using std::endl;
using std::string;
using std::set;
using namespace Dijon;

IndexAction::IndexAction(IndexInterface *pIndex) :
	ReducedAction(),
	m_pIndex(pIndex),
	m_docId(0),
	m_doUpdate(false)
{
}

IndexAction::IndexAction(const IndexAction &other) :
	ReducedAction(other),
	m_pIndex(other.m_pIndex),
	m_labels(other.m_labels),
	m_docId(other.m_docId),
	m_doUpdate(other.m_doUpdate)
{
	
}

IndexAction::~IndexAction()
{
}

IndexAction &IndexAction::operator=(const IndexAction &other)
{
	ReducedAction::operator=(other);

	if (this != &other)
	{
		m_pIndex = other.m_pIndex;
		m_labels = other.m_labels;
		m_docId = other.m_docId;
		m_doUpdate = other.m_doUpdate;
	}

	return *this;
}

void IndexAction::setIndexingMode(const set<string> &labels)
{
	m_labels = labels;
	m_docId = 0;
	m_doUpdate = false;
}

void IndexAction::setUpdatingMode(unsigned int docId)
{
	m_labels.clear();
	m_docId = docId;
	m_doUpdate = true;
}

bool IndexAction::takeAction(Document &doc, bool isNested)
{
	bool docSuccess = false;

	if (m_pIndex == NULL)
	{
		return false;
	}

	// Nested documents can't be updated because they are unindexed
	// and the ID is that of the base document anyway
	if ((m_doUpdate == true) &&
		(isNested == false))
	{
		docSuccess = m_pIndex->updateDocument(m_docId, doc);
	}
	else
	{
		unsigned int newDocId = m_docId;

		docSuccess = m_pIndex->indexDocument(doc, m_labels, newDocId);
		// Make sure we return the base document's ID, not the last nested document's ID
		if (isNested == false)
		{
			m_docId = newDocId;
		}
	}

	return docSuccess;
}

unsigned int IndexAction::getId(void) const
{
	return m_docId;
}

bool IndexAction::unindexNestedDocuments(const string &url)
{
	if (m_pIndex == NULL)
	{
		return false;
	}

	// Unindex all documents that stem from this file
	return m_pIndex->unindexDocuments(url, IndexInterface::BY_CONTAINER_FILE);
}

bool IndexAction::unindexDocument(const string &location)
{
	if (m_pIndex == NULL)
	{
		return false;
	}

	unindexNestedDocuments(location);

	return m_pIndex->unindexDocument(location);
}

FilterWrapper::FilterWrapper(IndexInterface *pIndex) :
	m_pAction(new IndexAction(pIndex)),
	m_ownAction(true)
{
}

FilterWrapper::FilterWrapper(IndexAction *pAction) :
	m_pAction(pAction),
	m_ownAction(false)
{
}

FilterWrapper::~FilterWrapper()
{
	if ((m_ownAction == true) &&
		(m_pAction != NULL))
	{
		delete m_pAction;
	}
}

bool FilterWrapper::indexDocument(const Document &doc, const set<string> &labels, unsigned int &docId)
{
	string originalType(doc.getType());

	if (m_pAction == NULL)
	{
		return false;
	}

	m_pAction->unindexNestedDocuments(doc.getLocation());
	m_pAction->setIndexingMode(labels);

	bool filteredDoc = FilterUtils::filterDocument(doc, originalType, *m_pAction);
	docId = m_pAction->getId();

	return filteredDoc;
}

bool FilterWrapper::updateDocument(const Document &doc, unsigned int docId)
{
	string originalType(doc.getType());

	if (m_pAction == NULL)
	{
		return false;
	}

	m_pAction->unindexNestedDocuments(doc.getLocation());
	m_pAction->setUpdatingMode(docId);

	return FilterUtils::filterDocument(doc, originalType, *m_pAction);
}

bool FilterWrapper::unindexDocument(const string &location)
{
	if (m_pAction == NULL)
	{
		return false;
	}

	return m_pAction->unindexDocument(location);
}
