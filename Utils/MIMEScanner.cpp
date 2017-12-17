/*
 *  Copyright 2005-2017 Fabrice Colin
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
#ifndef USE_GIO
#include "xdgmime/xdgmime.h"
#endif

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

#ifndef USE_GIO
static string getKeyValue(GKeyFile *pDesktopFile, const string &key)
{
	string value;

	if ((pDesktopFile == NULL) ||
		(key.empty() == true))
	{
		return "";
	}

	GError *pError = NULL;
	gchar *pKeyValue = g_key_file_get_string(pDesktopFile,
		"Desktop Entry", key.c_str(), &pError);

	if (pError == NULL)
	{
		if (pKeyValue != NULL)
		{
			value = pKeyValue;

			g_free(pKeyValue);
		}
	}
	else
	{
		g_error_free(pError);
	}

	return value;
}
#endif

MIMEAction::MIMEAction() :
	m_multipleArgs(false),
	m_localOnly(true),
#ifdef USE_GIO
	m_pAppInfo(NULL)
#else
	m_icon(""),
	m_device("")
#endif
{
}

MIMEAction::MIMEAction(const string &name, const string &cmdLine) :
	m_multipleArgs(false),
	m_localOnly(true),
	m_name(name),
	m_exec(cmdLine),
#ifdef USE_GIO
	m_pAppInfo(NULL)
#else
	m_icon(""),
	m_device("")
#endif
{
#ifdef USE_GIO
	GError *pError = NULL;
	m_pAppInfo = g_app_info_create_from_commandline(cmdLine.c_str(),
		name.c_str(), G_APP_INFO_CREATE_SUPPORTS_URIS, &pError);
	if (pError != NULL)
	{
		g_error_free(pError);
	}
#else
	parseExec();
#endif
}

#ifdef USE_GIO
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
#else
MIMEAction::MIMEAction(const string &desktopFile) :
	m_multipleArgs(false),
	m_localOnly(true),
	m_location(desktopFile)
{
	load();
}

#endif

MIMEAction::MIMEAction(const MIMEAction &other) :
	m_multipleArgs(other.m_multipleArgs),
	m_localOnly(other.m_localOnly),
	m_name(other.m_name),
	m_location(other.m_location),
	m_exec(other.m_exec),
#ifdef USE_GIO
	m_pAppInfo(NULL)
#else
	m_icon(other.m_icon),
	m_device(other.m_device)
#endif
{
#ifdef USE_GIO
	if (other.m_pAppInfo != NULL)
	{
		m_pAppInfo = g_app_info_dup(other.m_pAppInfo);
	}
#endif
}

MIMEAction::~MIMEAction()
{
#ifdef USE_GIO
	if (m_pAppInfo != NULL)
	{
		g_object_unref(m_pAppInfo);
	}
#endif
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
#ifdef USE_GIO
		if (m_pAppInfo != NULL)
		{
			g_object_unref(m_pAppInfo);
			m_pAppInfo = NULL;
		}
		if (other.m_pAppInfo != NULL)
		{
			m_pAppInfo = g_app_info_dup(other.m_pAppInfo);
		}
#else
		m_icon = other.m_icon;
		m_device = other.m_device;
#endif
	}

	return *this;
}

#ifndef USE_GIO
void MIMEAction::load(void)
{
	GKeyFile *pDesktopFile = g_key_file_new();
	GError *pError = NULL;

	if (pDesktopFile != NULL)
	{
		g_key_file_load_from_file(pDesktopFile, (const gchar *)m_location.c_str(),
			G_KEY_FILE_NONE, &pError);
		if (pError == NULL)
		{
			m_name = getKeyValue(pDesktopFile, "Name");
			// This may contain parameters described here :
			// http://standards.freedesktop.org/desktop-entry-spec/latest/ar01s06.html
			m_exec = getKeyValue(pDesktopFile, "Exec");
			m_icon = getKeyValue(pDesktopFile, "Icon");
			m_device = getKeyValue(pDesktopFile, "Device");
			parseExec();
		}
		else
		{
			g_error_free(pError);
		}

		g_key_file_free(pDesktopFile);
	}
}

void MIMEAction::parseExec(void)
{
	// Does it support multiple arguments ?
	if ((m_exec.find("%U") != string::npos) ||
		(m_exec.find("%F") != string::npos) ||
		(m_exec.find("%D") != string::npos) ||
		(m_exec.find("%N") != string::npos))
	{
		// Yes, it does
		m_multipleArgs = true;
	}
	// What about URLs as parameters ?
	if ((m_exec.find("%u") != string::npos) ||
		(m_exec.find("%U") != string::npos))
	{
		// It does, therefore it's not exclusively local
		m_localOnly = false;
	}
}

MIMECache::MIMECache()
{
}

MIMECache::MIMECache(const string &file, const string &section) :
	m_file(file),
	m_section(section)
{
}

MIMECache::MIMECache(const MIMECache &other) :
	m_file(other.m_file),
	m_section(other.m_section),
	m_defaultActions(other.m_defaultActions)
{
}

MIMECache::~MIMECache()
{
}

bool MIMECache::operator<(const MIMECache &other) const
{
	if (m_file < other.m_file)
	{
		return true;
	}

	return false;
}

MIMECache& MIMECache::operator=(const MIMECache &other)
{
	if (this != &other)
	{
		m_file = other.m_file;
		m_section = other.m_section;
		m_defaultActions = other.m_defaultActions;
	}

	return *this;
}

bool MIMECache::findDesktopFile(const string &desktopFile, const string &mimeType,
	map<string, MIMEAction> &loadedActions)
{
	// Has already been loaded ?
	map<string, MIMEAction>::const_iterator actionIter = loadedActions.find(desktopFile);
	if (actionIter != loadedActions.end())
	{
		// Yes, it was so just copy it
		m_defaultActions.insert(pair<string, MIMEAction>(mimeType, actionIter->second));

		return true;
	}
	// Does it exist in that path ?
	else if (access(desktopFile.c_str(), F_OK) == 0)
	{
		// It's here, we need to read it off the disk
		MIMEAction typeAction(desktopFile);
#ifdef DEBUG
		clog << "MIMECache::findDesktopFile: read " << desktopFile << endl;
#endif

		if (typeAction.m_exec.empty() == false)
		{
			m_defaultActions.insert(pair<string, MIMEAction>(mimeType, typeAction));
			loadedActions.insert(pair<string, MIMEAction>(desktopFile, typeAction));

			return true;
		}
	}

	return false;
}

void MIMECache::reload(const list<string> &desktopFilesPaths)
{
	if (m_file.empty() == true)
	{
		// We can't reload anything !
		return;
	}

	m_defaultActions.clear();
	load(desktopFilesPaths);
}

bool MIMECache::load(const list<string> &desktopFilesPaths)
{
	map<string, MIMEAction> loadedActions;
	GError *pError = NULL;
	bool foundActions = false;

	GKeyFile *pDefaults = g_key_file_new();
	if (pDefaults == NULL)
	{
		return false;
	}

	g_key_file_load_from_file(pDefaults, (const gchar *)m_file.c_str(),
		G_KEY_FILE_NONE, &pError);
	if (pError == NULL)
	{
#ifdef DEBUG
		clog << "MIMECache::load: loaded " << m_file << endl;
#endif

		gchar **pMimeTypes = g_key_file_get_keys(pDefaults, m_section.c_str(),
			NULL, &pError);
		if (pMimeTypes != NULL)
		{
			if (pError == NULL)
			{
				for (unsigned int i = 0; pMimeTypes[i] != NULL; ++i)
				{
					gchar **pDesktopFiles = g_key_file_get_string_list(pDefaults,
						m_section.c_str(), pMimeTypes[i], NULL, &pError);

					if (pDesktopFiles == NULL)
					{
						continue;
					}
					if (pError != NULL)
					{
						g_error_free(pError);
						continue;
					}

					// Register all applications for that type
					for (unsigned int j = 0; pDesktopFiles[j] != NULL; ++j)
					{
						// Search the paths for this desktop file
						for (list<string>::const_iterator iter = desktopFilesPaths.begin();
							iter != desktopFilesPaths.end(); ++iter)
						{
							if (findDesktopFile(*iter + pDesktopFiles[j], pMimeTypes[i],
								loadedActions) == true)
							{
								foundActions = true;
								break;
							}
						}
					}

					g_strfreev(pDesktopFiles);
				}
			}
			else
			{
				g_error_free(pError);
			}

			g_strfreev(pMimeTypes);
		}
	}
	else
	{
#ifdef DEBUG
		clog << "MIMECache::load: failed to load " << m_file << endl;
#endif
		g_error_free(pError);
	}
	g_key_file_free(pDefaults);

	return foundActions;
}

pthread_mutex_t MIMEScanner::m_xdgMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_rwlock_t MIMEScanner::m_cachesLock = PTHREAD_RWLOCK_INITIALIZER;
list<MIMECache> MIMEScanner::m_caches;
#endif
map<string, string> MIMEScanner::m_overrides;

MIMEScanner::MIMEScanner()
{
}

MIMEScanner::~MIMEScanner()
{
}

bool MIMEScanner::initialize(const string &userPrefix, const string &systemPrefix)
{
#ifdef USE_GIO
#if !GLIB_CHECK_VERSION(2,35,0)
	// Initialize the GType system
	g_type_init();
#endif
	return true;
#else
	list<string> desktopFilesPaths;
	string userDirectory(userPrefix + APPLICATIONS_DIRECTORY);
	string systemDirectory(systemPrefix + APPLICATIONS_DIRECTORY);
	bool foundActions = false;

	// This may be a re-initialize
	if (pthread_rwlock_wrlock(&m_cachesLock) == 0)
	{
		// Empty the caches list
		m_caches.clear();

		pthread_rwlock_unlock(&m_cachesLock);
	}

	if (systemDirectory.empty() == false)
	{
		// User-specific actions may point to desktop files in this path, so add it in
		desktopFilesPaths.push_front(systemDirectory);
	}

	// Load user-specific settings first
	if ((userPrefix.empty() == false) &&
		(userDirectory.empty() == false))
	{
		// Add the user's directory to the paths list
		desktopFilesPaths.push_front(userDirectory);

#ifdef DEBUG
		clog << "MIMEScanner::initialize: user-specific directory " << userDirectory << endl;
#endif
		foundActions |= addCache(userDirectory + MIME_DEFAULTS_LIST,
			MIME_DEFAULTS_SECTION, desktopFilesPaths);
		foundActions |= addCache(userDirectory + MIME_CACHE,
			MIME_CACHE_SECTION, desktopFilesPaths);
	}

	// Then load system-wide settings
	if ((systemPrefix.empty() == false) &&
		(systemDirectory.empty() == false))
	{
		// Make sure only the system directory is in the list
		desktopFilesPaths.clear();
		desktopFilesPaths.push_front(systemDirectory);

#ifdef DEBUG
		clog << "MIMEScanner::initialize: system-wide directory " << systemDirectory << endl;
#endif
		foundActions |= addCache(systemDirectory + MIME_DEFAULTS_LIST,
			MIME_DEFAULTS_SECTION, desktopFilesPaths);
		foundActions |= addCache(systemDirectory + MIME_CACHE,
			MIME_CACHE_SECTION, desktopFilesPaths);
	}

	return foundActions;
#endif
}

#ifndef USE_GIO
bool MIMEScanner::addCache(const string &file, const string &section,
	const list<string> &desktopFilesPaths)
{
	bool addedCache = false;

	if (pthread_rwlock_wrlock(&m_cachesLock) == 0)
	{
		m_caches.push_back(MIMECache(file, section));

		addedCache = m_caches.back().load(desktopFilesPaths);

		pthread_rwlock_unlock(&m_cachesLock);
	}

	return addedCache;
}
#endif

void MIMEScanner::shutdown(void)
{
#ifndef USE_GIO
	xdg_mime_shutdown();
#endif
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
#ifdef USE_GIO
	char *pType = g_content_type_guess(fileName.c_str(), NULL, 0, NULL);
#else
	const char *pType = NULL;

	if (pthread_mutex_lock(&m_xdgMutex) == 0)
	{
		pType = xdg_mime_get_mime_type_from_file_name(fileName.c_str());

		pthread_mutex_unlock(&m_xdgMutex);
	}
#endif

	if (pType == NULL)
	{
		return "";
	}

#ifdef USE_GIO
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
#else
	if (strncasecmp(pType, xdg_mime_type_unknown, strlen(pType)) == 0)
	{
		return "";
	}
#endif

	string mimeType(pType);

	// Quick and dirty fix to work-around shared-mime-info mistakenly identifying
	// HTML files as Mozilla bookmarks
	if ((fileName.find(".htm") != string::npos) &&
		(strncasecmp(pType, "application/x-mozilla-bookmarks", strlen(pType)) == 0))
	{
		mimeType = "text/html";
	}
#ifdef USE_GIO
	g_free(pType);
#endif
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

#ifdef USE_GIO
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
#else
	const char *pType = NULL;

	// Have a peek at the file
	if (pthread_mutex_lock(&m_xdgMutex) == 0)
	{
		pType = xdg_mime_get_mime_type_for_file(fileName.c_str(), NULL);

		pthread_mutex_unlock(&m_xdgMutex);
	}

	if (pType != NULL)
	{
		mimeType = pType;
	}
	else if (xdg_mime_type_unknown != NULL)
	{
		mimeType = xdg_mime_type_unknown;
	}
#endif
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
#ifdef USE_GIO
	else
	{
		mimeType = UNKNOWN_MIME_TYPE;
	}
#else
	else if (xdg_mime_type_unknown != NULL)
	{
		mimeType = xdg_mime_type_unknown;
	}
#endif
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

#ifdef USE_GIO
	char *pType = g_content_type_guess(NULL, (const guchar *)pData, (gsize)length, NULL);
#else
	const char *pType = NULL;

	if (pthread_mutex_lock(&m_xdgMutex) == 0)
	{
		pType = xdg_mime_get_mime_type_for_data((const void *)pData, (size_t)length, NULL);

		pthread_mutex_unlock(&m_xdgMutex);
	}
#endif

	if (pType == NULL)
	{
		return UNKNOWN_MIME_TYPE;
	}

#ifdef USE_GIO
	// Get the corresponding MIME type
	char *pMimeType = g_content_type_get_mime_type(pType);
	if (pMimeType == NULL)
	{
		g_free(pType);
		return UNKNOWN_MIME_TYPE;
	}

	g_free(pType);
	pType = pMimeType;
#endif

	string mimeType(pType);

#ifdef USE_GIO
	g_free(pType);
#endif
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

#ifdef USE_GIO
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
#else
	char **pParentTypes = xdg_mime_list_mime_parents(mimeType.c_str());
	if ((pParentTypes != NULL) &&
		(pParentTypes[0] != NULL))
	{
		for (unsigned int i = 0; pParentTypes[i] != NULL; ++i)
		{
			parentMimeTypes.insert(pParentTypes[i]);
		}

		free(pParentTypes);

		return true;
	}
#endif

	return false;
}

/// Adds a user-defined action for the given type.
void MIMEScanner::addDefaultAction(const string &mimeType, const MIMEAction &typeAction)
{
#ifdef USE_GIO
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
#else
	// Custom actions get stored in a cache object which is not connected to a file
	// We need to create this object the first time a custom action gets added
	if (pthread_rwlock_wrlock(&m_cachesLock) == 0)
	{
		if ((m_caches.empty() == true) ||
			(m_caches.front().m_file.empty() == false))
		{
			m_caches.push_front(MIMECache());
		}

		m_caches.front().m_defaultActions.insert(pair<string, MIMEAction>(mimeType, typeAction));

		pthread_rwlock_unlock(&m_cachesLock);
	}
#endif
}

bool MIMEScanner::getDefaultActionsForType(const string &mimeType, bool isLocal,
	set<string> &actionNames, vector<MIMEAction> &typeActions)
{
#ifdef USE_GIO
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
#else
	bool foundAction = false;

	if (pthread_rwlock_rdlock(&m_cachesLock) != 0)
	{
		return false;
	}

	// Each cache may have more than one action for this type
	for (list<MIMECache>::iterator cacheIter = m_caches.begin(); cacheIter != m_caches.end(); ++cacheIter)
	{
		MIMECache &cache = *cacheIter;
		multimap<string, MIMEAction> &defaultActions = cache.m_defaultActions;
		multimap<string, MIMEAction>::const_iterator actionIter = defaultActions.lower_bound(mimeType);
		multimap<string, MIMEAction>::const_iterator endActionIter = defaultActions.upper_bound(mimeType);

		// Found anything ?
		for (; actionIter != endActionIter; ++actionIter)
		{
			// The same action may be defined for another type
			if (actionNames.find(actionIter->second.m_name) == actionNames.end())
			{
#ifdef DEBUG
				clog << "MIMEScanner::getDefaultActionsForType: action " << actionIter->second.m_name
					<< " at " << actionIter->second.m_location << endl;
#endif
				actionNames.insert(actionIter->second.m_name);
				typeActions.push_back(actionIter->second);
				foundAction = true;
			}
		}
	}
  
	pthread_rwlock_unlock(&m_cachesLock);

	return foundAction;
#endif
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
	bool foundAction = getDefaultActionsForType(mimeType, isLocal, actionNames, typeActions);
#ifndef USE_GIO
	if (foundAction == false)
	{
		// Is there an action for any of this type's parents ?
		char **pParentTypes = xdg_mime_list_mime_parents(mimeType.c_str());
		if ((pParentTypes != NULL) &&
			(pParentTypes[0] != NULL))
		{
			for (unsigned int i = 0; pParentTypes[i] != NULL; ++i)
			{
				string parentType(pParentTypes[i]);

#ifdef DEBUG
				clog << "MIMEScanner::getDefaultActions: searching for parent type " << parentType << endl;
#endif
				foundAction = getDefaultActionsForType(parentType, isLocal, actionNames, typeActions);
				if (foundAction)
				{
					break;
				}
			}

			free(pParentTypes);
		}
#ifdef DEBUG
		else clog << "MIMEScanner::getDefaultActions: " << mimeType << " has no parent types" << endl;
#endif
	}
#endif

	return foundAction;
}

string MIMEScanner::getDescription(const string &mimeType)
{
#ifdef USE_GIO
	char *pDesc = g_content_type_get_description(mimeType.c_str());
	if (pDesc != NULL)
	{
		string description(pDesc);

		// Upper-case the first character
		description[0] = (char)toupper((int)pDesc[0]);
		g_free(pDesc);

		return description;
	}
#endif

	return mimeType;
}

