/*
 *  Copyright 2005-2009 Fabrice Colin
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

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_ATTR_XATTR_H
#include <attr/xattr.h>
#endif
#include <set>
#include <iostream>
#include <sstream>

#include "Url.h"
#include "StringManip.h"
#include "TimeConverter.h"
#include "MetaDataBackup.h"

using std::clog;
using std::endl;
using std::string;
using std::set;

MetaDataBackup::MetaDataBackup(const string &database) :
	SQLiteBase(database)
{
}

MetaDataBackup::~MetaDataBackup()
{
}

bool MetaDataBackup::setAttribute(const DocumentInfo &docInfo,
	const string &name, const string &value, bool noXAttr)
{
	string url(docInfo.getLocation());
	string urlWithIPath(docInfo.getLocation(true));
#ifdef HAVE_ATTR_XATTR_H
	Url urlObj(url);

	// If the file is local and isn't a nested document, use an extended attribute
	if ((noXAttr == false) &&
		(urlObj.isLocal() == true) &&
		(docInfo.getInternalPath().empty() == true))
	{
		string fileName(url.substr(urlObj.getProtocol().length() + 3));
		string attrName("pinot." + name);

		// Set an attribute, and add an entry in the table
		if (setxattr(fileName.c_str(), attrName.c_str(),
			value.c_str(), (size_t)value.length(), 0) != 0)
		{
#ifdef DEBUG
			clog << "MetaDataBackup::setAttribute: setxattr failed with " << strerror(errno) << endl;
#endif
		}
	}
#endif
	bool update = false, success = false;

	// Is there already such an item for this URL ?
	SQLResults *results = executeStatement("SELECT Url FROM MetaDataBackup \
		WHERE Url='%q' AND Name='%q';",
		Url::escapeUrl(urlWithIPath).c_str(), name.c_str());
	if (results != NULL)
	{
		SQLRow *row = results->nextRow();
		if (row != NULL)
		{
			// Yes, there is
			update = true;

			delete row;
		}

		delete results;
	}

	if (update == false)
	{
		results = executeStatement("INSERT INTO MetaDataBackup \
			VALUES('%q', '%q', '%q');",
			Url::escapeUrl(urlWithIPath).c_str(), name.c_str(), value.c_str());
	}
	else
	{
		results = executeStatement("UPDATE MetaDataBackup \
			SET Value='%q' WHERE Url='%q' AND Name='%q';",
			value.c_str(), Url::escapeUrl(urlWithIPath).c_str(), name.c_str());
	}
	if (results != NULL)
	{
		success = true;

		delete results;
	}

	return success;

}

bool MetaDataBackup::getAttribute(const DocumentInfo &docInfo,
	const string &name, string &value, bool noXAttr)
{
	string url(docInfo.getLocation());
	string urlWithIPath(docInfo.getLocation(true));
	bool success = false;
#ifdef HAVE_ATTR_XATTR_H
	Url urlObj(url);

	// If the file is local and isn't a nested document, use an extended attribute
	if ((noXAttr == false) &&
		(urlObj.isLocal() == true) &&
		(docInfo.getInternalPath().empty() == true))
	{
		string fileName(url.substr(urlObj.getProtocol().length() + 3));
		string attrName("pinot." + name);

		ssize_t attrSize = getxattr(fileName.c_str(), attrName.c_str(), NULL, 0);
		if (attrSize > 0)
		{
			char *pAttr = new char[attrSize];

			if (getxattr(fileName.c_str(), attrName.c_str(), pAttr, attrSize) > 0)
			{
				value = string(pAttr, attrSize);
				success = true;
			}
			else if (errno != ENOTSUP)
			{
				// Extended attributes are supported, but this one doesn't exist
				delete[] pAttr;
				return false;
			}

			delete[] pAttr;
		}
	}
#endif

	SQLResults *results = executeStatement("SELECT Value FROM MetaDataBackup \
		WHERE Url='%q' AND Name='%q';",
		Url::escapeUrl(urlWithIPath).c_str(), name.c_str());
	if (results != NULL)
	{
		SQLRow *row = results->nextRow();
		if (row != NULL)
		{
			value = row->getColumn(0);
			success = true;

			delete row;
		}

		delete results;
	}

	return success;
}

bool MetaDataBackup::getAttributes(const DocumentInfo &docInfo,
	const string &name, set<string> &values)
{
	string url(docInfo.getLocation());
	string urlWithIPath(docInfo.getLocation(true));
	bool success = false;
#if 0
	Url urlObj(url);

	// If the file is local and isn't a nested document, use an extended attribute
	if ((urlObj.isLocal() == true) &&
		(docInfo.getInternalPath().empty() == true))
	{
		string likeName("pinot." + name);

		ssize_t listSize = flistxattr(fd, NULL, 0);
		if (listSize > 0)
		{
			char *pList = new char[listSize];

			if ((pList != NULL) &&
				(flistxattr(fd, pList, listSize) > 0))
			{
				string attrList(pList, listSize);
				string::size_type startPos = 0, endPos = attrList.find('\0');

				while (endPos != string::npos)
				{
					string attrName(attrList.substr(startPos, endPos - startPos));

					if ((attrName.length() > likeName.length()) &&
						(attrName.substr(0, likeName.length()) == likeName))
					{
						string value;

						if (getAttribute(url, attrName.substr(6), value, true) == true)
						{
							values.insert(value);
						}
					}

					// Next
					startPos = endPos + 1;
					if (startPos < listSize)
					{
						endPos = attrList.find('\0', startPos);
					}
					else
					{
						endPos = string::npos;
					}
				}
			}

			delete[] pList;
		}
	}
#endif

	SQLResults *results = executeStatement("SELECT Value FROM MetaDataBackup \
		WHERE Url='%q' AND Name LIKE '%q%%';",
		Url::escapeUrl(urlWithIPath).c_str(), name.c_str());
	if (results != NULL)
	{
		while (results->hasMoreRows() == true)
                {
			SQLRow *row = results->nextRow();
			if (row == NULL)
			{
				continue;
			}

			values.insert(row->getColumn(0));
			success = true;

			delete row;
		}

		delete results;
	}

	return success;
}

bool MetaDataBackup::removeAttribute(const DocumentInfo &docInfo, 
	const string &name, bool noXAttr, bool likeName)
{
	string url(docInfo.getLocation());
	string urlWithIPath(docInfo.getLocation(true));
	bool success = false;
#ifdef HAVE_ATTR_XATTR_H
	Url urlObj(url);

	// If the file is local and isn't a nested document, use an extended attribute
	if ((noXAttr == false) &&
		(url.empty() == false) &&
		(urlObj.isLocal() == true) &&
		(docInfo.getInternalPath().empty() == true))
	{
		string fileName(url.substr(urlObj.getProtocol().length() + 3));
		string attrName("pinot." + name);

		if (removexattr(fileName.c_str(), attrName.c_str()) > 0)
		{
			return true;
		}
		else if (errno != ENOTSUP)
		{
			// Extended attributes are supported, but this one doesn't exist
			return false;
		}
	}
#endif

	// Delete from MetaDataBackup
	SQLResults *results = NULL;

	if (urlWithIPath.empty() == false)
	{
		if (likeName == false)
		{
			results = executeStatement("DELETE FROM MetaDataBackup \
				WHERE Url='%q' AND NAME='%q';",
				Url::escapeUrl(urlWithIPath).c_str(), name.c_str());
		}
		else
		{
			results = executeStatement("DELETE FROM MetaDataBackup \
				WHERE Url='%q' AND NAME LIKE '%q%%';",
				Url::escapeUrl(urlWithIPath).c_str(), name.c_str());
		}
	}
	else
	{
		results = executeStatement("DELETE FROM MetaDataBackup \
			WHERE NAME='%q';",
			name.c_str());
	}
	if (results != NULL)
	{
		success = true;
		delete results;
	}

	return success;
}

/// Creates the MetaDataBackup table in the database.
bool MetaDataBackup::create(const string &database)
{
	bool success = true;

	// The specified path must be a file
	if (SQLiteBase::check(database) == false)
	{
		return false;
	}

	SQLiteBase db(database);

	// Does MetaDataBackup exist ?
	if (db.executeSimpleStatement("SELECT * FROM MetaDataBackup LIMIT 1;") == false)
	{
#ifdef DEBUG
		clog << "MetaDataBackup::create: MetaDataBackup doesn't exist" << endl;
#endif
		// Create the table
		if (db.executeSimpleStatement("CREATE TABLE MetaDataBackup (Url VARCHAR(255), \
			Name VARCHAR(255), Value TEXT, PRIMARY KEY(Url, Value));") == false)
		{
			success = false;
		}
	}

	return success;
}

/// Adds an item.
bool MetaDataBackup::addItem(const DocumentInfo &docInfo, DocumentInfo::SerialExtent extent)
{
	bool success = false;

	if ((extent == DocumentInfo::SERIAL_FIELDS) ||
		(extent == DocumentInfo::SERIAL_ALL))
	{
		if (setAttribute(docInfo, "fields",
			docInfo.serialize(DocumentInfo::SERIAL_FIELDS)) == false)
		{
			return false;
		}

		success = true;
	}
	if ((extent == DocumentInfo::SERIAL_LABELS) ||
		(extent == DocumentInfo::SERIAL_ALL))
	{
		success = true;

		const set<string> &labels = docInfo.getLabels();
		for (set<string>::const_iterator labelIter = labels.begin();
			labelIter != labels.end(); ++labelIter)
		{
			// Skip internal labels
			if (labelIter->substr(0, 2) == "X-")
			{
				continue;
			}

			if (setAttribute(docInfo, string("label.") + *labelIter, *labelIter, true) == false)
			{
				success = false;
			}
		}
	}

	return success;
}

/// Gets an item.
bool MetaDataBackup::getItem(DocumentInfo &docInfo, DocumentInfo::SerialExtent extent)
{
	string value;
	bool success = false;

	if ((extent == DocumentInfo::SERIAL_FIELDS) ||
		(extent == DocumentInfo::SERIAL_ALL))
	{
		if (getAttribute(docInfo, "fields", value) == true)
		{
			docInfo.deserialize(value, DocumentInfo::SERIAL_FIELDS);
			success = true;
		}
	}
	if ((extent == DocumentInfo::SERIAL_LABELS) ||
		(extent == DocumentInfo::SERIAL_ALL))
	{
		set<string> labels;

		if (getAttributes(docInfo, "label.", labels) == false)
		{
			success = false;
		}
		else
		{
			docInfo.setLabels(labels);
			success = true;
		}
	}

	return success;
}

/// Gets items.
bool MetaDataBackup::getItems(const string &likeUrl, set<string> &urls,
	unsigned long min, unsigned long max)
{
	SQLResults *results = NULL;
	bool success = false;

	// Even when attributes are used, an entry is always added to the table
	if (likeUrl.empty() == true)
	{
		results = executeStatement("SELECT Url FROM MetaDataBackup \
			LIMIT %u OFFSET %u;",
			max - min, min);
	}
	else
	{
		results = executeStatement("SELECT Url FROM MetaDataBackup \
			WHERE Url LIKE '%q%%' LIMIT %u OFFSET %u;",
			likeUrl.c_str(), max - min, min);
	}
	if (results != NULL)
	{
		while (results->hasMoreRows() == true)
                {
			SQLRow *row = results->nextRow();
			if (row == NULL)
			{
				continue;
			}

			urls.insert(Url::unescapeUrl(row->getColumn(0)));
			success = true;

			delete row;
		}

		delete results;
	}

	return success;
}

/// Deletes an item.
bool MetaDataBackup::deleteItem(const DocumentInfo &docInfo, DocumentInfo::SerialExtent extent,
	const string &value)
{
	bool success = false;

	if ((extent == DocumentInfo::SERIAL_FIELDS) ||
		(extent == DocumentInfo::SERIAL_ALL))
	{
		if (removeAttribute(docInfo, "fields") == false)
		{
			return false;
		}

		success = true;
	}
	if ((extent == DocumentInfo::SERIAL_LABELS) ||
		(extent == DocumentInfo::SERIAL_ALL))
	{
		if (value.empty() == false)
		{
			success = removeAttribute(docInfo, string("label.") + value, true);
		}
		else
		{
			success = removeAttribute(docInfo, "label.", true, true);
		}
	}

	return success;
}

/// Deletes a label.
bool MetaDataBackup::deleteLabel(const string &value)
{
	if ((value.empty() == true) ||
		(removeAttribute(DocumentInfo("", "", "", ""),
			string("label.") + value, true) == false))
	{
		return false;
	}

	return true;
}

