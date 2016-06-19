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

#include <iostream>

#include "XapianDatabaseFactory.h"

using std::clog;
using std::endl;
using std::string;
using std::map;
using std::pair;

pthread_mutex_t XapianDatabaseFactory::m_mutex = PTHREAD_MUTEX_INITIALIZER;
map<string, XapianDatabase *> XapianDatabaseFactory::m_databases;
bool XapianDatabaseFactory::m_closed = false;

XapianDatabaseFactory::XapianDatabaseFactory()
{
}

XapianDatabaseFactory::~XapianDatabaseFactory()
{
}

/// Merges two databases together and add the result to the list.
bool XapianDatabaseFactory::mergeDatabases(const string &name,
	XapianDatabase *pFirst, XapianDatabase *pSecond)
{
	if (m_closed == true)
	{
		return false;
	}

	map<string, XapianDatabase *>::iterator dbIter = m_databases.find(name);
	if (dbIter != m_databases.end())
	{
		return false;
	}

	// Create the new database
	XapianDatabase *pDb = new XapianDatabase(name, pFirst, pSecond);

	// Insert it into the map
	pair<map<string, XapianDatabase *>::iterator, bool> insertPair = m_databases.insert(pair<string, XapianDatabase *>(name, pDb));
	// Was it inserted ?
	if (insertPair.second == false)
	{
		// No, it wasn't : delete the object
		delete pDb;

		return false;
	}

	return true;
}

/// Returns a XapianDatabase pointer; NULL if unavailable.
XapianDatabase *XapianDatabaseFactory::getDatabase(const string &location,
	bool readOnly, bool overwrite)
{
	XapianDatabase *pDb = NULL;

	if ((m_closed == true) ||
		(location.empty() == true))
	{
		return NULL;
	}

	// Lock the map
	if (pthread_mutex_lock(&m_mutex) != 0)
	{
		return NULL;
	}

	// Is the database already open ?
	map<string, XapianDatabase *>::iterator dbIter = m_databases.find(location);
	if (dbIter != m_databases.end())
	{
		pDb = dbIter->second;

		// Overwrite the database ?
		if (overwrite == true)
		{
			dbIter->second = NULL;
#ifdef DEBUG
			clog << "XapianDatabaseFactory::getDatabase: closing " << dbIter->first << endl;
#endif
			m_databases.erase(dbIter);
			delete pDb;

			dbIter = m_databases.end();
		}
	}

	// Open the database ?
	if (dbIter == m_databases.end())
	{
		// Create a new instance
		pDb = new XapianDatabase(location, readOnly, overwrite);
		// Insert it into the map
		pair<map<string, XapianDatabase *>::iterator, bool> insertPair = m_databases.insert(pair<string, XapianDatabase *>(location, pDb));
		// Was it inserted ?
		if (insertPair.second == false)
		{
			// No, it wasn't : delete the object
			delete pDb;
			pDb = NULL;
		}
	}

	// Unlock the map
	pthread_mutex_unlock(&m_mutex);

	return pDb;
}

/// Closes all databases.
void XapianDatabaseFactory::closeAll(void)
{
	if (m_databases.empty() == true)
	{
		return;
	}

	// Lock the map
	// FIXME: another thread may have a database and try and lock it after the loop below deletes it
	if (pthread_mutex_lock(&m_mutex) != 0)
	{
		return;
	}
	m_closed = true;

	// Close merged databases first
	std::map<std::string, XapianDatabase *>::iterator dbIter = m_databases.begin();
	while (dbIter != m_databases.end())
	{
		XapianDatabase *pDb = dbIter->second;

		if (pDb->isMerge() == false)
		{
			++dbIter;
			continue;
		}

		std::map<std::string, XapianDatabase *>::iterator nextIter = dbIter;
		++nextIter;
#ifdef DEBUG
		clog << "XapianDatabaseFactory::closeAll: closing " << dbIter->first << endl;
#endif

		// Remove from the map
		dbIter->second = NULL;
		m_databases.erase(dbIter);

		Xapian::Database *pIndex = pDb->readLock();
		pDb->unlock();
		// Close the database
		delete pDb;

		dbIter = nextIter;
	}
	// Now close all other databases
	dbIter = m_databases.begin();
	while (dbIter != m_databases.end())
	{
		XapianDatabase *pDb = dbIter->second;
		Xapian::Database *pIndex = NULL;
#ifdef DEBUG
		clog << "XapianDatabaseFactory::closeAll: closing " << dbIter->first << endl;
#endif

		// Remove from the map
		dbIter->second = NULL;
		m_databases.erase(dbIter);

		if (pDb->isWritable() == true)
		{
			pIndex = pDb->writeLock();
		}
		else
		{
			pIndex = pDb->readLock();
		}
		pDb->unlock();
		// Close the database
		delete pDb;

		dbIter = m_databases.begin();
	}

	// Unlock the map
	pthread_mutex_unlock(&m_mutex);
}
