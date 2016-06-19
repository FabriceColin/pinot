/*
 *  Copyright 2005-2008 Fabrice Colin
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

#ifndef _SEARCH_ENGINE_INTERFACE_H
#define _SEARCH_ENGINE_INTERFACE_H

#include <time.h>
#include <string>
#include <set>
#include <vector>

#include "DocumentInfo.h"
#include "Visibility.h"
#include "QueryProperties.h"

using namespace std;

/// Interface implemented by search engines.
class PINOT_EXPORT SearchEngineInterface
{
	public:
		typedef enum { DEFAULT_OP_AND = 0, DEFAULT_OP_OR } DefaultOperator;

		virtual ~SearchEngineInterface();

		/// Sets whether AND is the default operator.
		virtual void setDefaultOperator(DefaultOperator op);

		/// Sets the set of documents to limit to.
		virtual bool setLimitSet(const set<string> &docsSet);

		/// Sets the set of documents to expand from.
		virtual bool setExpandSet(const set<string> &docsSet);

		/// Runs a query; true if success.
		virtual bool runQuery(QueryProperties& queryProps,
			unsigned int startDoc = 0) = 0;

		/// Returns the results for the previous query.
		virtual const vector<DocumentInfo> &getResults(void) const;

		/// Returns an estimate of the total number of results for the previous query.
		virtual unsigned int getResultsCountEstimate(void) const;

		/// Returns the charset for the previous query's results.
		virtual string getResultsCharset(void) const;

		/// Suggests a spelling correction.
		virtual string getSpellingCorrection(void) const;

		/// Returns expand terms from the previous query.
		virtual const set<string> &getExpandTerms(void) const;

	protected:
		DefaultOperator m_defaultOperator;
		vector<DocumentInfo> m_resultsList;
		unsigned int m_resultsCountEstimate;
		string m_charset;
		string m_correctedFreeQuery;
		set<string> m_expandTerms;

		SearchEngineInterface();

	private:
		SearchEngineInterface(const SearchEngineInterface &other);
		SearchEngineInterface &operator=(const SearchEngineInterface &other);

};

#endif // _SEARCH_ENGINE_INTERFACE_H
