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

#include <string>
#include <map>

#include "config.h"
#include "Visibility.h"
#include "FieldMapperInterface.h"
#include "ModuleProperties.h"
#include "XapianDatabaseFactory.h"
#include "XapianEngine.h"
#include "XapianIndex.h"

using std::string;

extern "C"
{
	PINOT_EXPORT ModuleProperties *getModuleProperties(void);
	PINOT_EXPORT bool openOrCreateIndex(const string &databaseName, bool &obsoleteFormat,
		bool readOnly, bool overwrite);
	PINOT_EXPORT bool mergeIndexes(const string &mergedDatabaseName,
		const string &firstDatabaseName, const string &secondDatabaseName);
	PINOT_EXPORT IndexInterface *getIndex(const string &databaseName);
	PINOT_EXPORT SearchEngineInterface *getSearchEngine(const string &databaseName);
	PINOT_EXPORT void setFieldMapper(FieldMapperInterface *pMapper);
	PINOT_EXPORT void closeAll(void);
}

FieldMapperInterface *g_pMapper = NULL;

ModuleProperties *getModuleProperties(void)
{
	return new ModuleProperties("xapian", "Xapian", "", "");
}

bool openOrCreateIndex(const string &databaseName, bool &obsoleteFormat,
	bool readOnly, bool overwrite)
{
	XapianDatabase *pDb = XapianDatabaseFactory::getDatabase(databaseName, readOnly, overwrite);
	if (pDb == NULL)
	{
		obsoleteFormat = false;
		return false;
	}
	obsoleteFormat = pDb->wasObsoleteFormat();

	return true;
}

bool mergeIndexes(const string &mergedDatabaseName,
	const string &firstDatabaseName, const string &secondDatabaseName)
{
	// Assume both have already been open
	XapianDatabase *pFirstDb = XapianDatabaseFactory::getDatabase(firstDatabaseName);
	if ((pFirstDb == NULL) ||
		(pFirstDb->isOpen() == false))
	{
		return false;
	}
	XapianDatabase *pSecondDb = XapianDatabaseFactory::getDatabase(secondDatabaseName);
	if ((pSecondDb == NULL) ||
		(pSecondDb->isOpen() == false))
	{
		return false;
	}

	// Merge them
	return XapianDatabaseFactory::mergeDatabases(mergedDatabaseName, pFirstDb, pSecondDb);
}

IndexInterface *getIndex(const string &databaseName)
{
	return new XapianIndex(databaseName);
}

SearchEngineInterface *getSearchEngine(const string &databaseName)
{
	return new XapianEngine(databaseName);
}

void setFieldMapper(FieldMapperInterface *pMapper)
{
	g_pMapper = pMapper;
}

void closeAll(void)
{
	XapianEngine::freeAll();
	XapianDatabaseFactory::closeAll();
}

