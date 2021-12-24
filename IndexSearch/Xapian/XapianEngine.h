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

#ifndef _XAPIAN_ENGINE_H
#define _XAPIAN_ENGINE_H

#include <string>
#include <set>
#include <vector>

#include <xapian.h>

#include "config.h"
#include "SearchEngineInterface.h"

/// Wraps Xapian's search funtionality.
class XapianEngine : public SearchEngineInterface
{
	public:
		XapianEngine(const std::string &database);
		virtual ~XapianEngine();

		/// Frees all objects.
		static void freeAll(void);

		/// Sets the set of documents to limit to.
		virtual bool setLimitSet(const std::set<std::string> &docsSet);

		/// Sets the set of documents to expand from.
		virtual bool setExpandSet(const std::set<std::string> &docsSet);

		/// Runs a query; true if success.
		virtual bool runQuery(QueryProperties& queryProps,
			unsigned int startDoc = 0);

	protected:
		std::string m_databaseName;
		std::set<std::string> m_limitDocuments;
		std::set<std::string> m_expandDocuments;
		Xapian::Stem m_stemmer;

		bool queryDatabase(Xapian::Database *pIndex, Xapian::Query &query,
			const string &stemLanguage, unsigned int startDoc,
			const QueryProperties &queryProps);

		Xapian::Query parseQuery(Xapian::Database *pIndex, const QueryProperties &queryProps,
			const string &stemLanguage, DefaultOperator defaultOperator,
			string &correctedFreeQuery, bool minimal = false);

	private:
		XapianEngine(const XapianEngine &other);
		XapianEngine &operator=(const XapianEngine &other);

};

#endif // _XAPIAN_ENGINE_H
