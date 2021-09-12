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

#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <iostream>

#include "TimeConverter.h"
#include "Url.h"
#include "QueryHistory.h"

using std::clog;
using std::endl;
using std::string;
using std::set;
using std::vector;

QueryHistory::QueryHistory(const string &database) :
	SQLiteBase(database)
{
}

QueryHistory::~QueryHistory()
{
}

/// Creates the QueryHistory table in the database.
bool QueryHistory::create(const string &database)
{
	// The specified path must be a file
	if (SQLiteBase::check(database) == false)
	{
		return false;
	}

	SQLiteBase db(database);

	// Does QueryHistory exist ?
	if (db.executeSimpleStatement("SELECT * FROM QueryHistory LIMIT 1;") == false)
	{
		// Create the table
		if (db.executeSimpleStatement("CREATE TABLE QueryHistory (QueryName VARCHAR(255), "
			"EngineName VARCHAR(255), HostName VARCHAR(255), Url VARCHAR(255), "
			"Title VARCHAR(255), Extract VARCHAR(255), Score FLOAT, Date INTEGER, "
			"PRIMARY KEY(QueryName, EngineName, Url, Date));") == false)
		{
			return false;
		}
	}

	return true;
}

/// Inserts an URL.
bool QueryHistory::insertItem(const string &queryName, const string &engineName,
	const string &url, const string &title, const string &extract, float score)
{
	Url urlObj(url);
	string hostName(urlObj.getHost());
	bool success = false;

	SQLResults *results = executeStatement("INSERT INTO QueryHistory "
		"VALUES('%q', '%q', '%q', '%q', '%q', '%q', '%f', '%d');",
		queryName.c_str(), engineName.c_str(), hostName.c_str(),
		Url::escapeUrl(url).c_str(), title.c_str(), extract.c_str(),
		score, time(NULL));
	if (results != NULL)
	{
		success = true;
		delete results;
	}

	return success;
}

/// Checks if an URL is in the history; returns its current score or 0 if not found.
float QueryHistory::hasItem(const string &queryName, const string &engineName, const string &url,
	float &previousScore)
{
	float score = 0;

	SQLResults *results = executeStatement("SELECT Score FROM QueryHistory "
		"WHERE QueryName='%q' AND EngineName='%q' AND Url='%q' ORDER BY Date DESC;",
		queryName.c_str(), engineName.c_str(), Url::escapeUrl(url).c_str());
	if (results != NULL)
	{
		previousScore = 0;

		SQLRow *row = results->nextRow();
		if (row != NULL)
		{
			score = (float)atof(row->getColumn(0).c_str());

			delete row;

			// Get the score of the second last run
			SQLRow *row = results->nextRow();
			if (row != NULL)
			{
				previousScore = (float)atof(row->getColumn(0).c_str());

				delete row;
			}
		}

		delete results;
	}

	return score;
}

/// Gets the list of engines the query was run on.
bool QueryHistory::getEngines(const string &queryName, set<string> &enginesList)
{
	bool success = false;

	SQLResults *results = executeStatement("SELECT EngineName FROM QueryHistory "
		"WHERE QueryName='%q' GROUP BY EngineName;",
		queryName.c_str());
	if (results != NULL)
	{
		while (results->hasMoreRows() == true)
		{
			SQLRow *row = results->nextRow();
			if (row == NULL)
			{
				break;
			}

			enginesList.insert(row->getColumn(0));
			success = true;

			delete row;
		}

		delete results;
	}

	return success;
}

/// Gets the first max items for the given query, engine pair.
bool QueryHistory::getItems(const string &queryName, const string &engineName,
	unsigned int max, vector<DocumentInfo> &resultsList)
{
	bool success = false;

	SQLResults *results = executeStatement("SELECT Title, Url, Extract, Score, Date "
		"FROM QueryHistory WHERE QueryName='%q' AND EngineName='%q' "
		"ORDER BY Date DESC, Score DESC LIMIT %u;",
		queryName.c_str(), engineName.c_str(), max);
	if (results != NULL)
	{
		while (results->hasMoreRows() == true)
		{
			SQLRow *row = results->nextRow();
			if (row == NULL)
			{
				break;
			}

			DocumentInfo result(row->getColumn(0),
				Url::unescapeUrl(row->getColumn(1)).c_str(),
				"", "");
			result.setExtract(row->getColumn(2));
			result.setScore((float)atof(row->getColumn(3).c_str()));
			int runDate = atoi(row->getColumn(4).c_str());
			result.setTimestamp(TimeConverter::toTimestamp((time_t)runDate));

			resultsList.push_back(result);
			success = true;

			delete row;
		}

		delete results;
	}

	return success;
}

/// Gets an item's extract.
string QueryHistory::getItemExtract(const string &queryName, const string &engineName,
	const string &url)
{
	string extract;

	SQLResults *results = executeStatement("SELECT Extract FROM QueryHistory "
		"WHERE QueryName='%q' AND EngineName='%q' AND Url='%q' ORDER BY Date DESC;",
		queryName.c_str(), engineName.c_str(), Url::escapeUrl(url).c_str());
	if (results != NULL)
	{
		SQLRow *row = results->nextRow();
		if (row != NULL)
		{
			extract = row->getColumn(0);

			delete row;
		}

		delete results;
	}

	return extract;
}

/// Finds URLs.
bool QueryHistory::findUrlsLike(const string &url, unsigned int count, set<string> &urls)
{
	bool success = false;

	if (url.empty() == true)
	{
		return false; 
	}

	SQLResults *results = executeStatement("SELECT Url FROM QueryHistory "
		"WHERE Url LIKE '%q%%' ORDER BY Url LIMIT %u;",
		Url::escapeUrl(url).c_str(), count);
	if (results != NULL)
	{
		while (results->hasMoreRows() == true)
		{
			SQLRow *row = results->nextRow();
			if (row == NULL)
			{
				break;
			}

			urls.insert(Url::unescapeUrl(row->getColumn(0)));
			success = true;

			delete row;
		}

		delete results;
	}

	return success;
}

/// Gets a query's latest run times.
bool QueryHistory::getLatestRuns(const string &queryName, const string &engineName,
	unsigned int runCount, set<time_t> &runTimes)
{
	SQLResults *results = NULL;
	bool success = false;

	if (queryName.empty() == true)
	{
		return false;
	}

	if (engineName.empty() == true)
	{
		results = executeStatement("SELECT Date FROM QueryHistory "
			"WHERE QueryName='%q' GROUP BY EngineName ORDER By Date DESC LIMIT %u;",
			queryName.c_str(), runCount);
	}
	else
	{
		results = executeStatement("SELECT Date FROM QueryHistory "
			"WHERE QueryName='%q' AND EngineName='%q' GROUP BY Date ORDER By Date DESC LIMIT %u;",
			queryName.c_str(), engineName.c_str(), runCount);
	}

	if (results != NULL)
	{
		while (results->hasMoreRows() == true)
		{
			SQLRow *row = results->nextRow();
			if (row == NULL)
			{
				break;
			}

			int runDate = atoi(row->getColumn(0).c_str());
			if (runDate > 0)
			{
				runTimes.insert((time_t)runDate);
			}
			success = true;

			delete row;
		}

		delete results;
	}

	return success;
}

/// Deletes items at least as old as the given date.
bool QueryHistory::deleteItems(const string &queryName, const string &engineName,
	time_t cutOffDate)
{
	if (cutOffDate == 0)
	{
		// Nothing to delete
		return true;
	}

	SQLResults *results = executeStatement("DELETE FROM QueryHistory "
		"WHERE QueryName='%q' AND EngineName='%q' AND Date<'%d';",
		queryName.c_str(), engineName.c_str(), cutOffDate);
	if (results != NULL)
	{
		delete results;

		return true;
	}

	return false;
}

/// Deletes items.
bool QueryHistory::deleteItems(const string &name, bool isQueryName)
{
	SQLResults *results = NULL;

	if (isQueryName == true)
	{
		results = executeStatement("DELETE FROM QueryHistory "
			"WHERE QueryName='%q';",
			name.c_str());
	}
	else
	{
		results = executeStatement("DELETE FROM QueryHistory "
			"WHERE EngineName='%q';",
			name.c_str());
	}

	if (results != NULL)
	{
		delete results;

		return true;
	}

	return false;
}

/// Expires items older than the given date.
bool QueryHistory::expireItems(time_t expiryDate)
{
	if (expiryDate == 0)
	{
		// Nothing to delete
		return true;
	}

	SQLResults *results = executeStatement("DELETE FROM QueryHistory "
		"WHERE Date<'%d';",
		expiryDate);
	if (results != NULL)
	{
		delete results;

		return true;
	}

	return false;
}
