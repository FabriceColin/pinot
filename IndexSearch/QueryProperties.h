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

#ifndef _QUERY_PROPERTIES_H
#define _QUERY_PROPERTIES_H

#include <string>
#include <set>

#include "Visibility.h"

using namespace std;

/// This represents a query.
class PINOT_EXPORT QueryProperties
{
	public:
		typedef enum { RELEVANCE = 0, DATE_DESC, DATE_ASC, SIZE_DESC } SortOrder;
		typedef enum { NOTHING = 0, ALL_RESULTS, NEW_RESULTS } IndexWhat;

		QueryProperties();
		QueryProperties(const string &name, const string &freeQuery);
		QueryProperties(const QueryProperties &other);
		~QueryProperties();

		QueryProperties &operator=(const QueryProperties &other);
		bool operator==(const QueryProperties &other) const;
		bool operator<(const QueryProperties &other) const;

		/// Sets the name.
		void setName(const string &name);
		/// Gets the name.
		string getName(void) const;

		/// Sets the sort order.
		void setSortOrder(SortOrder order);
		/// Gets the sort order.
		SortOrder getSortOrder(void) const;

		/// Sets the language to use for stemming.
		void setStemmingLanguage(const string &language);
		/// Gets the language to use for stemming.
		string getStemmingLanguage(void) const;

		/// Sets the query string.
		void setFreeQuery(const string &freeQuery);
		/// Gets the query string.
		string getFreeQuery(bool withoutFilters = false) const;

		/// Sets the maximum number of results.
		void setMaximumResultsCount(unsigned int count);
		/// Gets the maximum number of results.
		unsigned int getMaximumResultsCount(void) const;

		/// Sets whether results should be indexed.
		void setIndexResults(IndexWhat indexResults);
		/// Gets whether results should be indexed
		IndexWhat getIndexResults(void) const;

		/// Sets the name of the label to use for indexed documents.
		void setLabelName(const string &labelName);
		/// Gets the name of the label to use for indexed documents.
		string getLabelName(void) const;

		/// Sets whether the query was modified in some way.
		void setModified(bool isModified);
		/// Gets whether the query was modified in some way.
		bool getModified(void) const;

		/// Returns the query's terms.
		void getTerms(set<string> &terms) const;

		/// Returns whether the query is empty.
		bool isEmpty() const;

	protected:
		string m_name;
		SortOrder m_order;
		string m_language;
		string m_freeQuery;
		string m_freeQueryWithoutFilters;
		unsigned int m_resultsCount;
		IndexWhat m_indexResults;
		string m_labelName;
		bool m_modified;

		void removeFilters(void);

};

#endif // _QUERY_PROPERTIES_H
