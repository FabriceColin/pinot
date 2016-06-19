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

#ifndef _QUERY_HISTORY_H
#define _QUERY_HISTORY_H

#include <string>
#include <vector>
#include <set>

#include "DocumentInfo.h"
#include "SQLiteBase.h"

/// Manages query history.
class QueryHistory : public SQLiteBase
{
	public:
		QueryHistory(const std::string &database);
		virtual ~QueryHistory();

		/// Creates the QueryHistory table in the database.
		static bool create(const std::string &database);

		/// Inserts an URL.
		bool insertItem(const std::string &queryName, const std::string &engineName,
			const std::string &url, const std::string &title, const std::string &extract, float score);

		/**
		  * Checks if an URL is in the query's history.
		  * If it is, it returns the current and previous scores; returns 0 if not found.
		  */
		float hasItem(const std::string &queryName, const std::string &engineName, const std::string &url,
			float &previousScore);

		/// Gets the list of engines the query was run on.
		bool getEngines(const std::string &queryName, std::set<std::string> &enginesList);

		/// Gets the first max items for the given query, engine pair.
		bool getItems(const std::string &queryName, const std::string &engineName,
			unsigned int max, std::vector<DocumentInfo> &resultsList);

		/// Gets an item's extract.
		std::string getItemExtract(const std::string &queryName, const std::string &engineName,
			const std::string &url);

		/// Finds URLs.
		bool findUrlsLike(const std::string &url, unsigned int count,
			std::set<std::string> &urls);

		/// Gets a query's latest run times.
		bool getLatestRuns(const std::string &queryName, const std::string &engineName,
			unsigned int runCount, std::set<time_t> &runTimes);

		/// Deletes items at least as old as the given date.
		bool deleteItems(const std::string &queryName, const std::string &engineName,
			time_t cutOffDate);

		/// Deletes items.
		bool deleteItems(const std::string &name, bool isQueryName);

		/// Expires items older than the given date.
		bool expireItems(time_t expiryDate);

	private:
		QueryHistory(const QueryHistory &other);
		QueryHistory &operator=(const QueryHistory &other);

};

#endif // _QUERY_HISTORY_H
