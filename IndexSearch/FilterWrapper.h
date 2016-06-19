/*
 *  Copyright 2007-2012 Fabrice Colin
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

#ifndef _FILTER_WRAPPER_H
#define _FILTER_WRAPPER_H

#include <string>
#include <set>

#include "Document.h"
#include "Visibility.h"
#include "Filter.h"
#include "FilterUtils.h"
#include "IndexInterface.h"

/// Indexing action.
class PINOT_EXPORT IndexAction : public ReducedAction
{
	public:
		IndexAction(IndexInterface *pIndex);
		IndexAction(const IndexAction &other);
		virtual ~IndexAction();

		IndexAction &operator=(const IndexAction &other);

		void setIndexingMode(const std::set<std::string> &labels);

		void setUpdatingMode(unsigned int docId);

		virtual bool takeAction(Document &doc, bool isNested);

		unsigned int getId(void) const;

		virtual bool unindexNestedDocuments(const std::string &url);

		virtual bool unindexDocument(const std::string &location);

	public:
		IndexInterface *m_pIndex;
		std::set<std::string> m_labels;
		unsigned int m_docId;
		bool m_doUpdate;

};

/// A wrapper around Dijon filters.
class PINOT_EXPORT FilterWrapper
{
	public:
		FilterWrapper(IndexInterface *pIndex);
		FilterWrapper(IndexAction *pAction);
		virtual ~FilterWrapper();

		/// Indexes the given data.
		bool indexDocument(const Document &doc, const std::set<std::string> &labels,
			unsigned int &docId);

		/// Updates the given document.
		bool updateDocument(const Document &doc, unsigned int docId);

		/// Unindexes document(s) at the given location.
		bool unindexDocument(const std::string &location);

	protected:
		IndexAction *m_pAction;
		bool m_ownAction;

	private:
		FilterWrapper(const FilterWrapper &other);
		FilterWrapper &operator=(const FilterWrapper &other);

};

#endif // _FILTER_WRAPPER_H
