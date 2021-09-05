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

#include <strings.h>
#include <unistd.h>
#include <glib.h>
#include <iostream>
#include <cstring>

#include "MIMEScanner.h"
#include "StringManip.h"
#include "Url.h"

#define APPLICATIONS_DIRECTORY	"/share/applications/"
#define MIME_DEFAULTS_LIST	"defaults.list"
#define MIME_DEFAULTS_SECTION	"Default Applications"
#define MIME_CACHE		"mimeinfo.cache"
#define MIME_CACHE_SECTION	"MIME Cache"
#define UNKNOWN_MIME_TYPE	"application/octet-stream";

using std::clog;
using std::endl;
using std::string;
using std::list;
using std::map;
using std::multimap;
using std::set;
using std::vector;
using std::pair;

MIMEAction::MIMEAction() :
	m_multipleArgs(false),
	m_localOnly(true),
	m_pAppInfo(NULL)
{
}

MIMEAction::MIMEAction(const string &name, const string &cmdLine) :
	m_multipleArgs(false),
	m_localOnly(true),
	m_name(name),
	m_exec(cmdLine),
	m_pAppInfo(NULL)
{
	GError *pError = NULL;
	m_pAppInfo = g_app_info_create_from_commandline(cmdLine.c_str(),
		name.c_str(), G_APP_INFO_CREATE_SUPPORTS_URIS, &pError);
	if (pError != NULL)
	{
		g_error_free(pError);
	}
}

MIMEAction::MIMEAction(GAppInfo *pAppInfo) :
	m_multipleArgs(false),
	m_localOnly(true),
	m_pAppInfo(NULL)
{
	if (pAppInfo != NULL)
	{
		const char *pInfo = g_app_info_get_name(pAppInfo);
		if (pInfo != NULL)
		{
			m_name = pInfo;
		}
		pInfo = g_app_info_get_executable(pAppInfo);
		if (pInfo != NULL)
		{
			m_exec = pInfo;
		}
		if (g_app_info_supports_uris(pAppInfo) == TRUE)
		{
			m_localOnly = false;
		}

		m_pAppInfo = g_app_info_dup(pAppInfo);
	}
}

MIMEAction::MIMEAction(const MIMEAction &other) :
	m_multipleArgs(other.m_multipleArgs),
	m_localOnly(other.m_localOnly),
	m_name(other.m_name),
	m_location(other.m_location),
	m_exec(other.m_exec),
	m_pAppInfo(NULL)
{
	if (other.m_pAppInfo != NULL)
	{
		m_pAppInfo = g_app_info_dup(other.m_pAppInfo);
	}
}

MIMEAction::~MIMEAction()
{
	if (m_pAppInfo != NULL)
	{
		g_object_unref(m_pAppInfo);
	}
}

bool MIMEAction::operator<(const MIMEAction &other) const
{
	if (m_name < other.m_name)
	{
		return true;
	}

	return false;
}

MIMEAction &MIMEAction::operator=(const MIMEAction &other)
{
	if (this != &other)
	{
		m_multipleArgs = other.m_multipleArgs;
		m_localOnly = other.m_localOnly;
		m_name = other.m_name;
		m_location = other.m_location;
		m_exec = other.m_exec;
		if (m_pAppInfo != NULL)
		{
			g_object_unref(m_pAppInfo);
			m_pAppInfo = NULL;
		}
		if (other.m_pAppInfo != NULL)
		{
			m_pAppInfo = g_app_info_dup(other.m_pAppInfo);
		}
	}

	return *this;
}

map<string, string> MIMEScanner::m_overrides;

MIMEScanner::MIMEScanner()
{
}

MIMEScanner::~MIMEScanner()
{
}

bool MIMEScanner::initialize(const string &userPrefix, const string &systemPrefix)
{
#if !GLIB_CHECK_VERSION(2,35,0)
	// Initialize the GType system
	g_type_init();
#endif

	return true;
}

void MIMEScanner::shutdown(void)
{
}

void MIMEScanner::listConfigurationFiles(const string &prefix, set<string> &files)
{
	if (prefix.empty() == false)
	{
		files.insert(prefix + APPLICATIONS_DIRECTORY + MIME_DEFAULTS_LIST);
		files.insert(prefix + APPLICATIONS_DIRECTORY + MIME_CACHE);
	}
}

string MIMEScanner::scanFileType(const string &fileName)
{
	if (fileName.empty() == true)
	{
		return "";
	}

	// Does it have an obvious extension ?
	for (map<string, string>::const_iterator overrideIter = m_overrides.begin();
		overrideIter != m_overrides.end(); ++overrideIter)
	{
		string ext(overrideIter->second);

		if (fileName.find(ext) == fileName.length() - ext.length())
		{
			// This extension matches
#ifdef DEBUG
			clog << "MIMEScanner::scanFileType: " << fileName << " has extension "
				<< ext << ", is type " << overrideIter->first << endl;
#endif
			return overrideIter->first;
		}
	}
	char *pType = g_content_type_guess(fileName.c_str(), NULL, 0, NULL);

	if (pType == NULL)
	{
		return "";
	}

	if (g_content_type_is_unknown(pType) == TRUE)
	{
		g_free(pType);
		return "";
	}

	// Get the corresponding MIME type
	char *pMimeType = g_content_type_get_mime_type(pType);
	if (pMimeType == NULL)
	{
		g_free(pType);
		return "";
	}

	g_free(pType);
	pType = pMimeType;

	string mimeType(pType);

	// Quick and dirty fix to work-around shared-mime-info mistakenly identifying
	// HTML files as Mozilla bookmarks
	if ((fileName.find(".htm") != string::npos) &&
		(strncasecmp(pType, "application/x-mozilla-bookmarks", strlen(pType)) == 0))
	{
		mimeType = "text/html";
	}
	g_free(pType);
#ifdef DEBUG
	clog << "MIMEScanner::scanFileType: " << fileName << " " << mimeType << endl;
#endif

	return mimeType;
}

/// Adds a MIME type override.
void MIMEScanner::addOverride(const string &mimeType, const string &extension)
{
	m_overrides.insert(pair<string, string>(mimeType, extension));
}

/// Finds out the given file's MIME type.
string MIMEScanner::scanFile(const string &fileName)
{
	if (fileName.empty() == true)
	{
		return "";
	}

	string mimeType(scanFileType(fileName));

	if (mimeType.empty() == false)
	{
		return mimeType;
	}

	string uri("file://");
	uri += fileName;

	// GFile will sniff the file if necessary
	GFile *pFile = g_file_new_for_uri(uri.c_str());
	if (pFile == NULL)
	{
		return UNKNOWN_MIME_TYPE;
	}

	GFileInfo *pFileInfo = g_file_query_info(pFile,
		G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,
		(GFileQueryInfoFlags)0, NULL, NULL);
	if (pFileInfo == NULL)
	{
		g_object_unref(pFile);
		return UNKNOWN_MIME_TYPE;
	}

	const char *pType = g_file_info_get_content_type(pFileInfo);
	if (pType == NULL)
	{
		g_object_unref(pFileInfo);
		g_object_unref(pFile);
		return UNKNOWN_MIME_TYPE;
	}

	// Get the corresponding MIME type
	char *pMimeType = g_content_type_get_mime_type(pType);
	if (pMimeType == NULL)
	{
		g_object_unref(pFileInfo);
		g_object_unref(pFile);
		return UNKNOWN_MIME_TYPE;
	}

	mimeType = pMimeType;
	g_free(pMimeType);
	g_object_unref(pFileInfo);
	g_object_unref(pFile);
#ifdef DEBUG
	clog << "MIMEScanner::scanFile: " << fileName << " " << mimeType << endl;
#endif

	return mimeType;
}

/// Finds out the given URL's MIME type.
string MIMEScanner::scanUrl(const Url &urlObj)
{
	string mimeType;

	// Is it a local file ?
	if (urlObj.getProtocol() == "file")
	{
		string fileName(urlObj.getLocation());
		fileName += "/";
		fileName += urlObj.getFile();

		mimeType = scanFile(fileName);
	}
	else
	{
		mimeType = scanFileType(urlObj.getFile());
	}

	if (mimeType.empty() == false)
	{
		return mimeType;
	}

	if (urlObj.getProtocol() == "http")
	{
		mimeType = "text/html";
	}
	else
	{
		mimeType = UNKNOWN_MIME_TYPE;
	}
#ifdef DEBUG
	clog << "MIMEScanner::scanUrl: " << urlObj.getFile() << " " << mimeType << endl;
#endif

	return mimeType;
}

/// Finds out the given data buffer's MIME type.
string MIMEScanner::scanData(const char *pData, unsigned int length)
{
	if ((pData == NULL) ||
		(length == 0))
	{
		return "";
	}

	char *pType = g_content_type_guess(NULL, (const guchar *)pData, (gsize)length, NULL);

	if (pType == NULL)
	{
		return UNKNOWN_MIME_TYPE;
	}

	// Get the corresponding MIME type
	char *pMimeType = g_content_type_get_mime_type(pType);
	if (pMimeType == NULL)
	{
		g_free(pType);
		return UNKNOWN_MIME_TYPE;
	}

	g_free(pType);
	pType = pMimeType;

	string mimeType(pType);

	g_free(pType);
#ifdef DEBUG
	clog << "MIMEScanner::scanData: " << mimeType << endl;
#endif

	return mimeType;
}

/// Gets parent MIME types.
bool MIMEScanner::getParentTypes(const string &mimeType,
	const set<string> &allTypes, set<string> &parentMimeTypes)
{
	if (mimeType.empty() == true)
	{
		return false;
	}

	for (set<string>::const_iterator typeIter = allTypes.begin(); typeIter != allTypes.end(); ++typeIter)
	{
		// FIXME: this function deals with content types, not MIME types !
		// Check whether g_content_type_from_mime_type() exists
		if (g_content_type_is_a(mimeType.c_str(), typeIter->c_str()) == TRUE)
		{
#ifdef DEBUG
			clog << "MIMEScanner::getParentTypes: " << mimeType << " is a " << *typeIter << endl;
#endif
			parentMimeTypes.insert(*typeIter);
		}
	}

	return false;
}

/// Adds a user-defined action for the given type.
void MIMEScanner::addDefaultAction(const string &mimeType, const MIMEAction &typeAction)
{
	if (typeAction.m_pAppInfo == NULL)
	{
		return;
	}

	GError *pError = NULL;
	g_app_info_set_as_default_for_type(typeAction.m_pAppInfo, mimeType.c_str(), &pError);
	if (pError != NULL)
	{
		g_error_free(pError);
	}
}

bool MIMEScanner::getDefaultActionsForType(const string &mimeType, bool isLocal,
	set<string> &actionNames, vector<MIMEAction> &typeActions)
{
	// Get default actions first
	GAppInfo *pDefAppInfo1 = g_app_info_get_default_for_type(mimeType.c_str(), (isLocal == false ? TRUE : FALSE));
	if (pDefAppInfo1 != NULL)
	{
		MIMEAction action(pDefAppInfo1);

#ifdef DEBUG
		clog << "MIMEScanner::getDefaultActionsForType: default action "
			<< isLocal << " " << action.m_name << endl;
#endif
		actionNames.insert(action.m_name);
		typeActions.push_back(action);

		g_object_unref(pDefAppInfo1);
	}

	// Get all other actions
	GList *pAppInfoList = g_app_info_get_all_for_type(mimeType.c_str());
	if (pAppInfoList == NULL)
	{
		return !typeActions.empty();
	}

	typeActions.reserve(g_list_length(pAppInfoList));
	for (GList *pList = pAppInfoList; pList != NULL; pList = pList->next)
	{
		if (pList->data == NULL)
		{
			continue;
		}

		GAppInfo *pAppInfo = G_APP_INFO(pList->data);
		if (pAppInfo == NULL)
		{
			continue;
		}

		MIMEAction action(pAppInfo);

		// Skip defaults
		if (actionNames.find(action.m_name) == actionNames.end())
		{
#ifdef DEBUG
			clog << "MIMEScanner::getDefaultActionsForType: action " << action.m_name << endl;
#endif
			actionNames.insert(action.m_name);
			typeActions.push_back(action);
		}
	}
	g_list_foreach(pAppInfoList, (GFunc)g_object_unref, NULL);
	g_list_free(pAppInfoList);

	return true;
}

/// Determines the default action(s) for the given type.
bool MIMEScanner::getDefaultActions(const string &mimeType, bool isLocal,
	vector<MIMEAction> &typeActions)
{
	set<string> actionNames;
	
	typeActions.clear();
  
#ifdef DEBUG
	clog << "MIMEScanner::getDefaultActions: searching for " << mimeType << endl;
#endif

	return getDefaultActionsForType(mimeType, isLocal, actionNames, typeActions);
}

string MIMEScanner::getDescription(const string &mimeType)
{
	char *pDesc = g_content_type_get_description(mimeType.c_str());
	if (pDesc != NULL)
	{
		string description(pDesc);

		// Upper-case the first character
		description[0] = (char)toupper((int)pDesc[0]);
		g_free(pDesc);

		return description;
	}

	return mimeType;
}

