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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <set>
#include <iostream>
#include <sstream>

#include "Url.h"
#include "StringManip.h"
#include "TimeConverter.h"
#include "ActionQueue.h"

using std::string;
using std::set;
using std::vector;
using std::stringstream;
using std::clog;
using std::endl;

ActionQueue::ActionQueue(const string &database, const string queueId) :
	SQLiteBase(database, false, false),
	m_queueId(queueId)
{
	prepareStatement("select-url",
		"SELECT Url FROM ActionQueue WHERE QueueId=? AND Url=?;");
	prepareStatement("push-item-insert",
		"INSERT INTO ActionQueue VALUES(?, ?, ?, ?, ?);");
	prepareStatement("push-item-update",
		"UPDATE ActionQueue SET Type=?, Date=?, Info=? WHERE QueueId=? AND Url=?;");
	prepareStatement("pop-item",
		"DELETE FROM ActionQueue WHERE QueueId=? AND Url=?;");
	prepareStatement("select-oldest-url",
		"SELECT Type, Info FROM ActionQueue "
		"WHERE QueueId=? ORDER BY Date DESC LIMIT 1;");
	prepareStatement("expire-items",
		"DELETE FROM ActionQueue WHERE QueueId=? AND Date<?;");
}

ActionQueue::~ActionQueue()
{
}

string ActionQueue::typeToText(ActionType type)
{
	string text;

	switch (type)
	{
		case INDEX:
			text = "INDEX";
			break;
		case UNINDEX:
			text = "UNINDEX";
			break;
		default:
			break;
	}

	return text;
}

ActionQueue::ActionType ActionQueue::textToType(const string &text)
{
	ActionType type = INDEX;

	if (text == "UNINDEX")
	{
		type = UNINDEX;
	}

	return type;
}

/// Creates the ActionQueue table in the database.
bool ActionQueue::create(const string &database)
{
	bool success = true;

	// The specified path must be a file
	if (SQLiteBase::check(database) == false)
	{
		return false;
	}

	SQLiteBase db(database);

	// Does ActionQueue exist ?
	if (db.executeSimpleStatement("SELECT * FROM ActionQueue LIMIT 1;") == false)
	{
#ifdef DEBUG
		clog << "ActionQueue::create: ActionQueue doesn't exist" << endl;
#endif
		// Create the table
		if (db.executeSimpleStatement("CREATE TABLE ActionQueue (QueueId VARCHAR(255), "
			"Url VARCHAR(255), Type VARCHAR(255), Date INTEGER, Info TEXT, "
			"PRIMARY KEY(QueueId, Url));") == false)
		{
			success = false;
		}
	}

	return success;
}

/// Pushes an item.
bool ActionQueue::pushItem(ActionType type, const DocumentInfo &docInfo)
{
	vector<string> values;
	string url(docInfo.getLocation());
	string info(docInfo.serialize());
	stringstream numStr;
	bool update = false, success = false;

	// Is there already an item for this URL ?
	values.push_back(m_queueId);
	values.push_back(Url::escapeUrl(url));
	SQLResults *results = executePreparedStatement("select-url", values);
	if (results != NULL)
	{
		SQLRow *row = results->nextRow();
		if (row != NULL)
		{
#ifdef DEBUG
			clog << "ActionQueue::pushItem: item "
				<< Url::unescapeUrl(row->getColumn(0)) << " exists" << endl;
#endif
			update = true;

			delete row;
		}

		delete results;
	}

	numStr << time(NULL);

	if (update == false)
	{
		values.push_back(typeToText(type));
		values.push_back(numStr.str());
		values.push_back(info);

		results = executePreparedStatement("push-item-insert", values);
	}
	else
	{
		values.clear();
		values.push_back(typeToText(type));
		values.push_back(numStr.str());
		values.push_back(info);
		values.push_back(m_queueId);
		values.push_back(Url::escapeUrl(url));

		results = executePreparedStatement("push-item-update", values);
	}
	if (results != NULL)
	{
#ifdef DEBUG
		clog << "ActionQueue::pushItem: queue " << m_queueId
			<< ": " << type << " on " << url << ", " << update << endl;
#endif
		success = true;

		delete results;
	}

	return success;
}

/// Pops and deletes the oldest item.
bool ActionQueue::popItem(ActionType &type, DocumentInfo &docInfo)
{
	vector<string> values;
	string url;
	bool success = false;

	if (getOldestItem(type, docInfo) == false)
	{
		return false;
	}
	url = docInfo.getLocation();
#ifdef DEBUG
	clog << "ActionQueue::popItem: queue " << m_queueId
		<< ": " << type << " on " << url << endl;
#endif

	values.push_back(m_queueId);
	values.push_back(Url::escapeUrl(url));

	// Delete from ActionQueue
	SQLResults *results = executePreparedStatement("pop-item", values);
	if (results != NULL)
	{
		success = true;
		delete results;
	}

	return success;
}

bool ActionQueue::getOldestItem(ActionType &type, DocumentInfo &docInfo)
{
	vector<string> values;
	bool success = false;

	values.push_back(m_queueId);
	SQLResults *results = executePreparedStatement("select-oldest-url", values);
	if (results != NULL)
	{
		SQLRow *row = results->nextRow();
		if (row != NULL)
		{
			type = textToType(row->getColumn(0));
			success = true;

			// Deserialize DocumentInfo
			docInfo.deserialize(row->getColumn(1));

			delete row;
		}

		delete results;
	}

	return success;
}

/// Returns the number of items of a particular type.
unsigned int ActionQueue::getItemsCount(ActionType type)
{
	unsigned int count = 0;

	SQLResults *results = executeStatement("SELECT COUNT(*) FROM ActionQueue "
		"WHERE Type='%q';",
		typeToText(type).c_str());
	if (results != NULL)
	{
		count = (unsigned int)results->getIntCount();

		delete results;
	}

	return count;
}

/// Deletes all items under a given URL.
bool ActionQueue::deleteItems(const string &url)
{
	bool success = false;

	if (beginTransaction() == false)
	{
		return false;
	}

	SQLResults *results = executeStatement("DELETE FROM ActionQueue "
		"WHERE Url LIKE '%q%%';",
		Url::escapeUrl(url).c_str());
	if (results != NULL)
	{
		success = true;
		delete results;
	}

	if (endTransaction() == false)
	{
		return false;
	}

	return success;
}

/// Expires items older than the given date.
bool ActionQueue::expireItems(time_t expiryDate)
{
	vector<string> values;
	stringstream numStr;
	bool success = false;

	if (beginTransaction() == false)
	{
		return false;
	}

	values.push_back(m_queueId);
	numStr << expiryDate;
	values.push_back(numStr.str());

	SQLResults *results = executePreparedStatement("expire-items", values);
	if (results != NULL)
	{
		success = true;
		delete results;
	}

	if (endTransaction() == false)
	{
		return false;
	}

	return success;
}

