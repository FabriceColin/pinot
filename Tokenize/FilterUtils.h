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

#ifndef _FILTER_UTILS_H
#define _FILTER_UTILS_H

#include <string>
#include <map>

#include "Document.h"
#include "Visibility.h"
#include "filters/Filter.h"

/// Drives document reduction and takes action on the final document.
class PINOT_EXPORT ReducedAction
{
	public:
		ReducedAction();
		ReducedAction(const ReducedAction &other);
		virtual ~ReducedAction();

		ReducedAction &operator=(const ReducedAction &other);

		virtual bool positionFilter(const Document &doc, Dijon::Filter *pFilter);

		virtual bool isReduced(const Document &doc);

		virtual bool takeAction(Document &doc, bool isNested) = 0;

};

/// Utility functions for dealing with Dijon filters.
class PINOT_EXPORT FilterUtils
{
	public:
		virtual ~FilterUtils();

		/// Returns a Filter that handles the given MIME type, or one of its parents.
		static Dijon::Filter *getFilter(const std::string &mimeType);

		/// Indicates whether a MIME type is supported or not.
		static bool isSupportedType(const std::string &mimeType);

		/// Feeds a document's data to a filter.
		static bool feedFilter(const Document &doc, Dijon::Filter *pFilter);

		/// Populates a document based on metadata extracted by the filter.
		static bool populateDocument(Document &doc, Dijon::Filter *pFilter);

		/// Filters a document until reduced to the minimum.
		static bool filterDocument(const Document &doc, const std::string &originalType,
			ReducedAction &action);

		/// Convenient front-end for filterDocument() to reduce documents.
		static bool reduceDocument(const Document &doc, ReducedAction &action);

		/// Strips markup from a piece of text.
		static std::string stripMarkup(const std::string &text);

	protected:
		static std::set<std::string> m_types;
		static std::map<std::string, std::string> m_typeAliases;
		static std::string m_maxNestedSize;

		FilterUtils();

	private:
		FilterUtils(const FilterUtils &other);
		FilterUtils &operator=(const FilterUtils &other);

};

#endif // _FILTER_UTILS_H
