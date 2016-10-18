/*
 *  Copyright 2007-2014 Fabrice Colin
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
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#ifdef HAVE_DLFCN_H
#include <dlfcn.h>
#endif
#include <algorithm>
#include <iostream>

#include "Filter.h"
#include "TextFilter.h"
#include "FilterFactory.h"

#ifdef HAVE_DLFCN_H
#ifdef __CYGWIN__
#define DLOPEN_FLAGS RTLD_LAZY
#else
#define DLOPEN_FLAGS (RTLD_LAZY|RTLD_LOCAL)
#endif
#endif

#if defined _GLIBCXX_USE_CXX11_ABI && _GLIBCXX_USE_CXX11_ABI
#define GETFILTERTYPESFUNC	"_Z16get_filter_typesRNSt3__13setINS_12basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEENS_4lessIS6_EENS4_IS6_EEEE"
#define GETFILTERFUNC		"_Z10get_filterRKNSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEE"
#else
#define GETFILTERTYPESFUNC	"_Z16get_filter_typesRSt3setISsSt4lessISsESaISsEE"
#define GETFILTERFUNC		"_Z10get_filterRKSs"
#endif

using std::clog;
using std::clog;
using std::endl;
using std::string;
using std::set;
using std::map;
using std::copy;
using namespace Dijon;

map<string, string> FilterFactory::m_types;
map<string, void *> FilterFactory::m_handles;

FilterFactory::FilterFactory()
{
}

FilterFactory::~FilterFactory()
{
}

unsigned int FilterFactory::loadFilters(const string &dir_name)
{
	unsigned int count = 0;
#ifdef HAVE_DLFCN_H
	struct stat fileStat;

	if (dir_name.empty() == true)
	{
		return 0;
	}

	// Is it a directory ?
	if ((stat(dir_name.c_str(), &fileStat) == -1) ||
		(!S_ISDIR(fileStat.st_mode)))
	{
		clog << "FilterFactory::loadFilters: " << dir_name << " is not a directory" << endl;
		return 0;
	}

	// Scan it
	DIR *pDir = opendir(dir_name.c_str());
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

			fileName = dir_name;
			fileName += "/";
			fileName += pEntryName;

			// Check this entry
			if ((stat(fileName.c_str(), &fileStat) != 0) ||
				(!S_ISREG(fileStat.st_mode)))
			{
				clog << "FilterFactory::loadFilters: couldn't stat " << pEntryName << endl;

				// Next entry
				pDirEntry = readdir(pDir);
				continue;
			}

			void *pHandle = dlopen(fileName.c_str(), DLOPEN_FLAGS);
			if (pHandle == NULL)
			{
				clog << "FilterFactory::loadFilters: " << dlerror() << endl;

				// Next entry
				pDirEntry = readdir(pDir);
				continue;
			}

			// What type(s) does this support ?
			get_filter_types_func *pTypesFunc = (get_filter_types_func *)dlsym(pHandle,
				GETFILTERTYPESFUNC);
			if (pTypesFunc == NULL)
			{
				clog << "FilterFactory::loadFilters: " << dlerror() << endl;

				dlclose(pHandle);

				// Next entry
				pDirEntry = readdir(pDir);
				continue;
			}

			set<string> types;
			unsigned int typeCount = 0;
			bool filterOkay = (*pTypesFunc)(types);

			if (filterOkay == false)
			{
				clog << "FilterFactory::loadFilters: couldn't get types from " << pEntryName << endl;
			}
			else for (set<string>::iterator typeIter = types.begin();
				typeIter != types.end(); ++typeIter)
			{
				string newType(*typeIter);

				if (m_types.find(newType) == m_types.end())
				{
					// Add a record for this filter
					m_types[newType] = fileName;
					++typeCount;
#ifdef DEBUG
					clog << "FilterFactory::loadFilters: type " << newType
						<< " is supported by " << pEntryName << endl;
#endif
				}
			}

			if (typeCount > 0)
			{
				m_handles[fileName] = pHandle;
			}
			else
			{
#ifdef DEBUG
				clog << "FilterFactory::loadFilters: no useful types from " << fileName << endl;
#endif
				dlclose(pHandle);
			}
		}

		// Next entry
		pDirEntry = readdir(pDir);
	}
	closedir(pDir);
#endif

	return count;
}

Filter *FilterFactory::getLibraryFilter(const string &mime_type)
{
	void *pHandle = NULL;

	if (m_handles.empty() == true)
	{
#ifdef DEBUG
		clog << "FilterFactory::getLibraryFilter: no libraries" << endl;
#endif
		return NULL;
	}

	map<string, string>::iterator typeIter = m_types.find(mime_type);
	if (typeIter == m_types.end())
	{
		// We don't know about this type
		return NULL;
	}
	map<string, void *>::iterator handleIter = m_handles.find(typeIter->second);
	if (handleIter == m_handles.end())
	{
		// We don't know about this library
		return NULL;
	}
	pHandle = handleIter->second;
	if (pHandle == NULL)
	{
		return NULL;
	}

#ifdef HAVE_DLFCN_H
	// Get a filter object then
	get_filter_func *pFunc = (get_filter_func *)dlsym(pHandle,
		GETFILTERFUNC);
	if (pFunc != NULL)
	{
		return (*pFunc)(mime_type);
	}
#ifdef DEBUG
	clog << "FilterFactory::getLibraryFilter: couldn't find export getFilter" << endl;
#endif
#endif

	return NULL;
}

Filter *FilterFactory::getFilter(const string &mime_type)
{
	string typeOnly(mime_type);
	string::size_type semiColonPos = mime_type.find(";");

	// Remove the charset, if any
	if (semiColonPos != string::npos)
	{
		typeOnly = mime_type.substr(0, semiColonPos);
	}
#ifdef DEBUG
	clog << "FilterFactory::getFilter: file type is " << typeOnly << endl;
#endif

	if (typeOnly == "text/plain")
	{
		return new TextFilter(typeOnly);
	}
#ifndef _DYNAMIC_DIJON_HTMLFILTER
	else if (typeOnly == "text/html")
	{
		return new HtmlFilter(typeOnly);
	}
#endif
#ifndef _DYNAMIC_DIJON_XMLFILTER
	else if ((typeOnly == "text/xml") ||
		(typeOnly == "application/xml"))
	{
		return new XmlFilter(typeOnly);
	}
#endif

	return getLibraryFilter(typeOnly);
}

void FilterFactory::getSupportedTypes(set<string> &mime_types)
{
	mime_types.clear();

	// Built-in types
	mime_types.insert("text/plain");
#ifndef _DYNAMIC_DIJON_HTMLFILTER
	mime_types.insert("text/html");
#endif
#ifndef _DYNAMIC_DIJON_XMLFILTER
	mime_types.insert("text/xml");
	mime_types.insert("application/xml");
#endif
	// Library-handled types
	for (map<string, string>::iterator typeIter = m_types.begin();
		typeIter != m_types.end(); ++typeIter)
	{
		mime_types.insert(typeIter->first);
	}
}

bool FilterFactory::isSupportedType(const string &mime_type)
{
	string typeOnly(mime_type);
	string::size_type semiColonPos = mime_type.find(";");

	// Remove the charset, if any
	if (semiColonPos != string::npos)
	{
		typeOnly = mime_type.substr(0, semiColonPos);
	}

	// Is it a built-in type ?
	if ((typeOnly == "text/plain") ||
#ifndef _DYNAMIC_DIJON_HTMLFILTER
		(typeOnly == "text/html") ||
#endif
#ifndef _DYNAMIC_DIJON_XMLFILTER
		(typeOnly == "text/xml") ||
		(typeOnly == "application/xml") ||
#endif
		(m_types.find(typeOnly) != m_types.end()))
	{
		return true;
	}

	return false;
}

void FilterFactory::unloadFilters(void)
{
#ifdef HAVE_DLFCN_H
	for (map<string, void*>::iterator iter = m_handles.begin(); iter != m_handles.end(); ++iter)
	{
		if (dlclose(iter->second) != 0)
		{
#ifdef DEBUG
			clog << "FilterFactory::unloadFilters: failed on " << iter->first << endl;
#endif
		}
	}
#endif

	m_types.clear();
	m_handles.clear();
}

