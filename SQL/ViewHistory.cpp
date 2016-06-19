/*
 *  Copyright 2005-2013 Fabrice Colin
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
#include <unistd.h>
#include <iostream>
#include <cstdlib>

#include "Url.h"
#include "ViewHistory.h"

using std::clog;
using std::endl;
using std::string;

ViewHistory::ViewHistory(const string &database) :
	SQLiteBase(database)
{
}

ViewHistory::~ViewHistory()
{
}

/// Creates the ViewHistory table in the database.
bool ViewHistory::create(const string &database)
{
	bool createTable = false;

	// The specified path must be a file
	if (SQLiteBase::check(database) == false)
	{
		return false;
	}

	SQLiteBase db(database);

	// Does ViewHistory exist ?
	if (db.executeSimpleStatement("SELECT * FROM ViewHistory LIMIT 1;") == false)
	{
#ifdef DEBUG
		clog << "ViewHistory::create: ViewHistory doesn't exist" << endl;
#endif
		createTable = true;
	}
	else
	{
		// Previous versions didn't include a Date field, so check for it
		if (db.executeSimpleStatement("SELECT Date FROM ViewHistory LIMIT 1;") == false)
		{
#ifdef DEBUG
			clog << "ViewHistory::create: ViewHistory needs updating" << endl;
#endif
			// Ideally, we would use ALTER TABLE but it's not supported by SQLite
			if (db.executeSimpleStatement("DROP TABLE ViewHistory; VACUUM;") == true)
			{
				createTable = true;
			}
		}
	}

	// Create the table ?
	if (createTable == true)
	{
		if (db.executeSimpleStatement("CREATE TABLE ViewHistory (Url VARCHAR(255) \
			PRIMARY KEY, Status INTEGER, DATE INTEGER);") == false)
		{
			return false;
		}
	}

	return true;
}

/// Inserts an URL.
bool ViewHistory::insertItem(const string &url)
{
	bool success = false;

	SQLResults *results = executeStatement("INSERT INTO ViewHistory \
		VALUES('%q', '1', '%d');", Url::escapeUrl(url).c_str(), time(NULL));
	if (results != NULL)
	{
		success = true;
		delete results;
	}

	return success;
}

/// Checks if an URL is in the history.
bool ViewHistory::hasItem(const string &url)
{
	bool success = false;

	SQLResults *results = executeStatement("SELECT Url FROM ViewHistory \
		WHERE Url='%q';", Url::escapeUrl(url).c_str());
	if (results != NULL)
	{
		SQLRow *row = results->nextRow();
		if (row != NULL)
		{
			// If this returns anything, it's the URL we are looking for
#ifdef DEBUG
			clog << "ViewHistory::hasItem: URL " << row->getColumn(0) << endl;
#endif
			success = true;

			delete row;
		}

		delete results;
	}

	return success;
}

/// Returns the number of items.
unsigned int ViewHistory::getItemsCount(void)
{
	unsigned int count = 0;

	SQLResults *results = executeStatement("SELECT COUNT(*) FROM ViewHistory;");
	if (results != NULL)
	{
		count = (unsigned int)results->getIntCount();

		delete results;
	}

	return count;
}

/// Deletes an URL.
bool ViewHistory::deleteItem(const string &url)
{
	bool success = false;

	SQLResults *results = executeStatement("DELETE FROM ViewHistory \
		WHERE Url='%q';", Url::escapeUrl(url).c_str());
	if (results != NULL)
	{
		success = true;
		delete results;
	}

	return success;
}

/// Expires items older than the given date.
bool ViewHistory::expireItems(time_t expiryDate)
{
	bool success = false;

	SQLResults *results = executeStatement("DELETE FROM ViewHistory \
		WHERE Date<'%d';", expiryDate);
	if (results != NULL)
	{
		success = true;
		delete results;
	}

	return success;
}
