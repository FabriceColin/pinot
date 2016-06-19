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

#include "config.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#ifdef HAVE_DLFCN_H
#include <dlfcn.h>
#endif
#include <utility>
#include <iostream>

#ifdef HAVE_DBUS
#include "DBusIndex.h"
#endif
#include "PluginWebEngine.h"
#include "ModuleFactory.h"

#ifdef HAVE_DLFCN_H
#ifdef __CYGWIN__
#define DLOPEN_FLAGS RTLD_NOW
#else
#define DLOPEN_FLAGS (RTLD_NOW|RTLD_LOCAL)
#endif
#endif

#define GETMODULEPROPERTIESFUNC	"getModuleProperties"
#define OPENORCREATEINDEXFUNC	"openOrCreateIndex"
#define MERGEINDEXESFUNC	"mergeIndexes"
#define GETINDEXFUNC		"getIndex"
#define GETSEARCHENGINEFUNC	"getSearchEngine"
#define SETFIELDMAPPERFUNC	"setFieldMapper"
#define CLOSEALLFUNC		"closeAll"

typedef ModuleProperties *(getModulePropertiesFunc)(void);
typedef bool (openOrCreateIndexFunc)(const string &, bool &, bool, bool);
typedef bool (mergeIndexesFunc)(const string &, const string &, const string &);
typedef IndexInterface *(getIndexFunc)(const string &);
typedef SearchEngineInterface *(getSearchEngineFunc)(const string &);
typedef void (setFieldMapperFunc)(FieldMapperInterface *pMapper);
typedef void (closeAllFunc)(void);

using std::clog;
using std::clog;
using std::endl;
using std::string;
using std::map;
using std::set;
using std::pair;

LoadableModule::LoadableModule(ModuleProperties *pProperties,
	const string &location, void *pHandle) :
	m_pProperties(pProperties),
	m_location(location),
	m_canSearch(false),
	m_canIndex(false),
	m_pHandle(pHandle)
{
}

LoadableModule::LoadableModule(const LoadableModule &other) :
	m_pProperties(NULL),
	m_location(other.m_location),
	m_canSearch(other.m_canSearch),
	m_canIndex(other.m_canIndex),
	m_pHandle(other.m_pHandle)
{
	if (other.m_pProperties != NULL)
	{
		m_pProperties = new ModuleProperties(*other.m_pProperties);
	}
}

LoadableModule::~LoadableModule()
{
	if (m_pProperties != NULL)
	{
		delete m_pProperties;
	}
}

LoadableModule &LoadableModule::operator=(const LoadableModule &other)
{
	if (this != &other)
	{
		if (m_pProperties != NULL)
		{
			delete m_pProperties;
			m_pProperties = NULL;
		}
		m_pProperties = other.m_pProperties;
		m_location = other.m_location;
		m_canSearch = other.m_canSearch;
		m_canIndex = other.m_canIndex;
		m_pHandle = other.m_pHandle;
	}

	return *this;
}

map<string, LoadableModule> ModuleFactory::m_types;

ModuleFactory::ModuleFactory()
{
}

ModuleFactory::~ModuleFactory()
{
}

IndexInterface *ModuleFactory::getLibraryIndex(const string &type, const string &option)
{
	map<string, LoadableModule>::iterator typeIter = m_types.find(type);
	if ((typeIter == m_types.end()) ||
		(typeIter->second.m_canIndex == false))
	{
		// We don't know about this type, or doesn't support indexes
		return NULL;
	}

	void *pHandle = typeIter->second.m_pHandle;
	if (pHandle == NULL)
	{
		return NULL;
	}

#ifdef HAVE_DLFCN_H
	getIndexFunc *pFunc = (getIndexFunc *)dlsym(pHandle,
		GETINDEXFUNC);
	if (pFunc != NULL)
	{
		return (*pFunc)(option);
	}
#endif
#ifdef DEBUG
	clog << "ModuleFactory::getLibraryIndex: couldn't find export getIndex" << endl;
#endif

	return NULL;
}

SearchEngineInterface *ModuleFactory::getLibrarySearchEngine(const string &type, const string &option)
{
	map<string, LoadableModule>::iterator typeIter = m_types.find(type);
	if (typeIter == m_types.end())
	{
		// We don't know about this type
		return NULL;
	}

	void *pHandle = typeIter->second.m_pHandle;
	if (pHandle == NULL)
	{
		return NULL;
	}

#ifdef HAVE_DLFCN_H
	getSearchEngineFunc *pFunc = (getSearchEngineFunc *)dlsym(pHandle,
		GETSEARCHENGINEFUNC);
	if (pFunc != NULL)
	{
		return (*pFunc)(option);
	}
#endif
#ifdef DEBUG
	clog << "ModuleFactory::getLibrarySearchEngine: couldn't find export getSearchEngine" << endl;
#endif

	return NULL;
}

unsigned int ModuleFactory::loadModules(const string &directory)
{
	unsigned int count = 0;
#ifdef HAVE_DLFCN_H
	struct stat fileStat;

	if (directory.empty() == true)
	{
		return 0;
	}

	// Is it a directory ?
	if ((stat(directory.c_str(), &fileStat) == -1) ||
		(!S_ISDIR(fileStat.st_mode)))
	{
		clog << "ModuleFactory::loadModules: " << directory << " is not a directory" << endl;
		return 0;
	}

	// Scan it
	DIR *pDir = opendir(directory.c_str());
	if (pDir == NULL)
	{
		return 0;
	}

	// Iterate through this directory's entries
	struct dirent *pDirEntry = readdir(pDir);
	while (pDirEntry != NULL)
	{
		char *pEntryName = pDirEntry->d_name;
		if (pEntryName != NULL)
		{
			string fileName = pEntryName;
			string::size_type extPos = fileName.find_last_of(".");

			if ((extPos == string::npos) ||
				(fileName.substr(extPos) != ".so"))
			{
				// Next entry
				pDirEntry = readdir(pDir);
				continue;
			}

			fileName = directory;
			fileName += "/";
			fileName += pEntryName;

			// Check this entry
			if ((stat(fileName.c_str(), &fileStat) == 0) &&
				(S_ISREG(fileStat.st_mode)))
			{
				void *pHandle = dlopen(fileName.c_str(), DLOPEN_FLAGS);
				if (pHandle != NULL)
				{
					// What type does this export ?
					getModulePropertiesFunc *pPropsFunc = (getModulePropertiesFunc *)dlsym(pHandle,
						GETMODULEPROPERTIESFUNC);
					if (pPropsFunc != NULL)
					{
						LoadableModule module((*pPropsFunc)(), fileName, pHandle);

						if (module.m_pProperties != NULL)
						{
							string moduleType(module.m_pProperties->m_name);

							// Can it search ?
							getSearchEngineFunc *pSearchFunc = (getSearchEngineFunc *)dlsym(pHandle,
								GETSEARCHENGINEFUNC);
							if (pSearchFunc != NULL)
							{
								module.m_canSearch = true;
							}

							// Can it index ?
							getIndexFunc *pIndexFunc = (getIndexFunc *)dlsym(pHandle,
								GETINDEXFUNC);
							if (pIndexFunc != NULL)
							{
								module.m_canIndex = true;
							}

							// Add a record for this module
							m_types.insert(pair<string, LoadableModule>(moduleType, module));
#ifdef DEBUG
							clog << "ModuleFactory::loadModules: " << moduleType
								<< " is supported by " << pEntryName << endl;
#endif
						}
					}
					else clog << "ModuleFactory::loadModules: " << dlerror() << endl;
				}
				else clog << "ModuleFactory::loadModules: " << dlerror() << endl;
			}
#ifdef DEBUG
			else clog << "ModuleFactory::loadModules: "
				<< pEntryName << " is not a file" << endl;
#endif
		}

		// Next entry
		pDirEntry = readdir(pDir);
	}
	closedir(pDir);
#endif

	return count;
}

bool ModuleFactory::openOrCreateIndex(const string &type, const string &option,
	bool &obsoleteFormat, bool readOnly, bool overwrite)
{
	map<string, LoadableModule>::iterator typeIter = m_types.find(type);
	if ((typeIter == m_types.end()) ||
		(typeIter->second.m_canIndex == false))
	{
		// We don't know about this type, or doesn't support indexes
		return false;
	}

	void *pHandle = typeIter->second.m_pHandle;
	if (pHandle == NULL)
	{
		return false;
	}

#ifdef HAVE_DLFCN_H
	openOrCreateIndexFunc *pFunc = (openOrCreateIndexFunc *)dlsym(pHandle,
		OPENORCREATEINDEXFUNC);
	if (pFunc != NULL)
	{
		return (*pFunc)(option, obsoleteFormat, readOnly, overwrite);
	}
#endif
#ifdef DEBUG
	clog << "ModuleFactory::openOrCreateIndex: couldn't find export openOrCreateIndex" << endl;
#endif

	return false;
}

bool ModuleFactory::mergeIndexes(const string &type, const string &option0,
	const string &option1, const string &option2)
{
	map<string, LoadableModule>::iterator typeIter = m_types.find(type);
	if ((typeIter == m_types.end()) ||
		(typeIter->second.m_canIndex == false))
	{
		// We don't know about this type, or doesn't support indexes
		return false;
	}

	void *pHandle = typeIter->second.m_pHandle;
	if (pHandle == NULL)
	{
		return false;
	}

#ifdef HAVE_DLFCN_H
	mergeIndexesFunc *pFunc = (mergeIndexesFunc *)dlsym(pHandle,
		MERGEINDEXESFUNC);
	if (pFunc != NULL)
	{
		return (*pFunc)(option0, option1, option2);
	}
#endif
#ifdef DEBUG
	clog << "ModuleFactory::mergeIndexes: couldn't find export mergeIndexes" << endl;
#endif

	return false;
}

IndexInterface *ModuleFactory::getIndex(const string &type, const string &option)
{
	IndexInterface *pIndex = NULL;

	// Choice by type
	// Do we need to nest it in a DBusIndex ?
	if (type.substr(0, 5) == "dbus-")
	{
#ifdef DEBUG
		clog << "ModuleFactory::mergeIndexes: sub-type " << type.substr(5) << endl;
#endif
		pIndex = getLibraryIndex(type.substr(5), option);
		if (pIndex != NULL)
		{
#ifdef HAVE_DBUS
			return new DBusIndex(pIndex);
#else
			return pIndex;
#endif
		}

		return NULL;
	}

	return getLibraryIndex(type, option);
}

SearchEngineInterface *ModuleFactory::getSearchEngine(const string &type, const string &option)
{
	SearchEngineInterface *pEngine = NULL;

	// Choice by type
	if (
#ifdef HAVE_BOOST_SPIRIT
		(type == "sherlock") ||
#endif
		(type == "opensearch"))
	{
		pEngine = new PluginWebEngine(option);
	}

	if (pEngine != NULL)
	{
		return pEngine;
	}

	return getLibrarySearchEngine(type, option);
}

string ModuleFactory::getSearchEngineName(const string &type, const string &option)
{
	if (
#ifdef HAVE_BOOST_SPIRIT
		(type == "sherlock") ||
#endif
		(type == "opensearch"))
	{
		SearchPluginProperties properties;

		if (PluginWebEngine::getDetails(option, properties) == true)
		{
			return properties.m_name;
		}

		return "";
	}
	else
	{
		return option;
	}

	return type;
}

void ModuleFactory::getSupportedEngines(map<ModuleProperties, bool> &engines)
{
	engines.clear();

	// Built-in engines
#ifdef HAVE_BOOST_SPIRIT
	engines.insert(pair<ModuleProperties, bool>(ModuleProperties("sherlock", "Sherlock", "", ""), false));
#endif
	engines.insert(pair<ModuleProperties, bool>(ModuleProperties("opensearch", "OpenSearch", "", ""), false));

	// Library-handled engines
	for (map<string, LoadableModule>::iterator typeIter = m_types.begin();
		typeIter != m_types.end(); ++typeIter)
	{
		ModuleProperties *pProps = typeIter->second.m_pProperties;

		if (pProps != NULL)
		{
			engines.insert(pair<ModuleProperties, bool>(*pProps, true));
		}
	}
}

bool ModuleFactory::isSupported(const string &type, bool asIndex)
{
	if (asIndex == true)
	{
		// Only backends implement index functionality
		map<string, LoadableModule>::const_iterator typeIter = m_types.find(type);
		if (typeIter != m_types.end())
		{
			return typeIter->second.m_canIndex;
		}

		return false;
	}

	if (
#ifdef HAVE_BOOST_SPIRIT
		(type == "sherlock") ||
#endif
		(type == "opensearch"))
	{
		return true;
	}
	else
	{
		// Does this backend implement search functionality ?
		map<string, LoadableModule>::const_iterator typeIter = m_types.find(type);
		if (typeIter != m_types.end())
		{
			return typeIter->second.m_canSearch;
		}
	}

	return false;	
}

void ModuleFactory::setFieldMapper(FieldMapperInterface *pMapper)
{
	for (map<string, LoadableModule>::iterator typeIter = m_types.begin(); typeIter != m_types.end(); ++typeIter)
	{
		void *pHandle = typeIter->second.m_pHandle;
		if (pHandle == NULL)
		{
			continue;
		}

#ifdef HAVE_DLFCN_H
		setFieldMapperFunc *pFunc = (setFieldMapperFunc *)dlsym(pHandle, SETFIELDMAPPERFUNC);
		if (pFunc != NULL)
		{
			(*pFunc)(pMapper);
		}
#ifdef DEBUG
		else clog << "ModuleFactory::setFieldMapper: couldn't find export setFieldMapper" << endl;
#endif
#endif
	}
}

void ModuleFactory::unloadModules(void)
{
	for (map<string, LoadableModule>::iterator typeIter = m_types.begin(); typeIter != m_types.end(); ++typeIter)
	{
		void *pHandle = typeIter->second.m_pHandle;
		if (pHandle == NULL)
		{
			continue;
		}

#ifdef HAVE_DLFCN_H
		closeAllFunc *pFunc = (closeAllFunc *)dlsym(pHandle, CLOSEALLFUNC);
		if (pFunc != NULL)
		{
			(*pFunc)();
		}
#ifdef DEBUG
		else clog << "ModuleFactory::unloadModules: couldn't find export closeAll" << endl;
#endif

		if (dlclose(pHandle) != 0)
		{
#ifdef DEBUG
			clog << "ModuleFactory::unloadModules: failed on " << typeIter->first << endl;
#endif
		}
#endif
	}

	m_types.clear();
}

