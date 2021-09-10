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

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#ifdef HAVE_PWD_H
#include <pwd.h>
#endif
#ifdef HAVE_FNMATCH_H
#include <fnmatch.h>
#endif
#include <algorithm>
#include <iostream>

#include <glibmm/convert.h>
#include <glibmm/date.h>
#include <libxml/parser.h>
#include <libxml/globals.h>
#include <libxml++/parsers/domparser.h>
#include <libxml++/nodes/node.h>
#include <libxml++/nodes/textnode.h>

#include "NLS.h"
#include "CommandLine.h"
#include "Languages.h"
#include "StringManip.h"
#include "ModuleFactory.h"
#include "PluginWebEngine.h"
#include "PinotSettings.h"

using namespace std;
using namespace Glib;
using namespace xmlpp;

static string getElementContent(const Element *pElem)
{
	if (pElem == NULL)
	{
		return "";
	}

#ifdef HAS_LIBXMLPP026
	const TextNode *pText = pElem->get_child_content();
#else
	const TextNode *pText = pElem->get_child_text();
#endif
	if (pText == NULL)
	{
		return "";
	}

	return pText->get_content();
}

static Element *addChildElement(Element *pElem, const string &nodeName, const string &nodeContent)
{
	if (pElem == NULL)
	{
		return NULL;
	}

	Element *pSubElem = pElem->add_child(nodeName);
	if (pSubElem != NULL)
	{
#ifdef HAS_LIBXMLPP026
		pSubElem->set_child_content(nodeContent);
#else
		pSubElem->set_child_text(nodeContent);
#endif
	}

	return pSubElem;
}

PinotSettings PinotSettings::m_instance;
bool PinotSettings::m_clientMode = false;

PinotSettings::PinotSettings() :
	m_warnAboutVersion(false),
	m_defaultBackend("xapian"),
	m_minimumDiskSpace(50),
	m_xPos(0),
	m_yPos(0),
	m_width(0),
	m_height(0),
	m_panePos(-1),
	m_showEngines(false),
	m_expandQueries(false),
	m_ignoreRobotsDirectives(false),
	m_suggestQueryTerms(true),
	m_newResultsColourRed(65535),
	m_newResultsColourGreen(0),
	m_newResultsColourBlue(0),
	m_proxyPort(8080),
	m_proxyEnabled(false),
	m_isBlackList(true),
	m_firstRun(false),
	m_indexCount(0)
{
	string directoryName(getConfigurationDirectory());
	struct stat fileStat;

	// Initialize libxml2 and check for potential ABI mismatches
	LIBXML_TEST_VERSION
	xmlParserDebugEntities = 0;

	// Find out if there is a .pinot directory
	if (stat(directoryName.c_str(), &fileStat) != 0)
	{
		// No, create it then
#ifdef WIN32
		if (mkdir(directoryName.c_str()) == 0)
#else
		if (mkdir(directoryName.c_str(), (mode_t)S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IWGRP|S_IXGRP|S_IROTH|S_IXOTH) == 0)
#endif
		{
			clog << "Created directory " << directoryName << endl;
			m_firstRun = true;
		}
		else
		{
			clog << "Couldn't create pinot directory at "
				<< directoryName << endl;
		}
	}

	// This is where the internal indices live
	m_docsIndexLocation = directoryName;
	m_docsIndexLocation += "/index";
	m_daemonIndexLocation = directoryName;
	m_daemonIndexLocation += "/daemon";

	// This is not set in the settings files
	char *pEnvVar = getenv("PINOT_MINIMUM_DISK_SPACE");
	if ((pEnvVar != NULL) &&
		(strlen(pEnvVar) > 0))
	{
		m_minimumDiskSpace = atof(pEnvVar);
	}
}

PinotSettings::~PinotSettings()
{
	xmlCleanupParser();
}

PinotSettings &PinotSettings::getInstance(void)
{
	return m_instance;
}

bool PinotSettings::enableClientMode(bool enable)
{
	bool isEnabled = m_clientMode;

	m_clientMode = enable;

	return isEnabled;
}

string PinotSettings::getHomeDirectory(void)
{
#ifdef HAVE_PWD_H
	struct passwd *pPasswd = getpwuid(geteuid());

	if ((pPasswd != NULL) &&
		(pPasswd->pw_dir != NULL))
	{
		return pPasswd->pw_dir;
	}
	else
	{
#endif
		char *homeDir = getenv("HOME");
		if (homeDir != NULL)
		{
			return homeDir;
		}
#ifdef HAVE_PWD_H
	}

	return "~";
#else

	return ".";
#endif
}

string PinotSettings::getConfigurationDirectory(void)
{
	string directoryName(getHomeDirectory());

	if (directoryName.empty() == true)
	{
		return "~/.pinot";
	}
	directoryName += "/.pinot";

	return directoryName;
}

string PinotSettings::getFileName(bool prefsOrUI)
{
	string configFileName(getConfigurationDirectory());

	if (prefsOrUI == true)
	{
		configFileName += "/prefs.xml";
	}
	else
	{
		configFileName += "/ui.xml";
	}

	return configFileName;
}

string PinotSettings::getCurrentUserName(void)
{
#ifdef HAVE_PWD_H
	struct passwd *pPasswd = getpwuid(geteuid());

	if ((pPasswd != NULL) &&
		(pPasswd->pw_name != NULL))
	{
		return pPasswd->pw_name;
	}
#endif

	return "";
}

void PinotSettings::checkHistoryDatabase(void)
{
	string uiHistoryDatabase(getConfigurationDirectory());
	string daemonHistoryDatabase(getConfigurationDirectory());
	struct stat fileStat;

	uiHistoryDatabase += "/history";
	daemonHistoryDatabase += "/history-daemon";

	// Copy the UI's over to the daemon's history if it doesn't exist
	if ((stat(uiHistoryDatabase.c_str(), &fileStat) == 0) &&
		((stat(daemonHistoryDatabase.c_str(), &fileStat) != 0) ||
		(!S_ISREG(fileStat.st_mode))))
	{
		string output;

		CommandLine::runSync(string("\\cp -f ") + uiHistoryDatabase + " " + daemonHistoryDatabase, output);
#ifdef DEBUG
		clog << "PinotSettings::checkHistoryDatabase: " << output << endl;
#endif
	}
}

string PinotSettings::getHistoryDatabaseName(bool needToQueryDaemonHistory)
{
	string historyDatabase(getConfigurationDirectory());

	if ((m_clientMode == false) ||
		(needToQueryDaemonHistory == true))
	{
		historyDatabase += "/history-daemon";
	}
	else
	{
		historyDatabase += "/history";
	}

	return historyDatabase;
}

bool PinotSettings::isFirstRun(void) const
{
	return m_firstRun;
}

void PinotSettings::clear(void)
{
	m_version.clear();
	m_warnAboutVersion = false;
	m_defaultBackend = "xapian";
	m_xPos = 0;
	m_yPos = 0;
	m_width = 0;
	m_height = 0;
	m_panePos = -1;
	m_showEngines = false;
	m_expandQueries = false;
	m_ignoreRobotsDirectives = false;
	m_suggestQueryTerms = true;
	m_newResultsColourRed = 65535;
	m_newResultsColourGreen = 0;
	m_newResultsColourBlue = 0;
	m_proxyAddress.clear();
	m_proxyPort = 8080;
	m_proxyType.clear();
	m_proxyEnabled = false;
	m_indexableLocations.clear();
	m_filePatternsList.clear();
	m_isBlackList = true;
	m_editablePluginValues.clear();
	m_cacheProviders.clear();
	m_cacheProtocols.clear();

	m_firstRun = false;
	m_indexes.clear();
	m_indexCount = 0;
	m_engines.clear();
	m_engineIds.clear();
	m_engineChannels.clear();
	m_queries.clear();
	m_labels.clear();
}

bool PinotSettings::load(LoadWhat what)
{
	string fileName;
	bool loadedUIConfiguration = false;

	if ((what == LOAD_ALL) ||
		(what == LOAD_GLOBAL))
	{
		fileName = string(SYSCONFDIR) + "/pinot/globalconfig.xml";

		if (loadConfiguration(fileName, true) == false)
		{
			return false;
		}

		if (what == LOAD_GLOBAL)
		{
			// Stop here
			return true;
		}
	}

	// Load settings ?
	if (m_firstRun == false)
	{
		// Load 0.90 preferences first
		if (loadConfiguration(getFileName(true), false) == false)
		{
			fileName = getConfigurationDirectory() + "/config.xml";

			// We may have to migrate away from a pre-0.90 configuration
			clear();
			if (loadConfiguration(fileName, false) == true)
			{
				// Save settings now to the new format
				save(SAVE_PREFS);
				save(SAVE_CONFIG);

				clog << "Migrated settings to 0.90 format" << endl;
			}
			else
			{
				m_firstRun = true;
			}
		}
		else
		{
			loadedUIConfiguration = loadConfiguration(getFileName(false), false);
		}
	}

	if (what == LOAD_ALL)
	{
		// Load search engines
		loadSearchEngines(string(PREFIX) + "/share/pinot/engines");
		loadSearchEngines(getConfigurationDirectory() + "/engines");
	}

	map<ModuleProperties, bool> engines;
	string currentUserChannel("X-Current-User-Channel");

	// Some engines are available as back-ends
	ModuleFactory::getSupportedEngines(engines);
	for (map<ModuleProperties, bool>::const_iterator engineIter = engines.begin();
		engineIter != engines.end(); ++engineIter)
	{
		if (engineIter->second == true)
		{
			string channelName(engineIter->first.m_channel);

			m_engineIds[1 << m_engines.size()] = engineIter->first.m_name;

			// Is a channel specified ?
			if (engineIter->first.m_channel.empty() == true)
			{
				ModuleProperties modProps(engineIter->first);

				channelName = modProps.m_channel = currentUserChannel;

#ifdef DEBUG
				clog << "PinotSettings::load: no channel for back-end " << engineIter->first.m_name << endl;
#endif
				m_engines.insert(modProps);
			}
			else
			{
				m_engines.insert(engineIter->first);
			}

			if (m_engineChannels.find(channelName) == m_engineChannels.end())
			{
				m_engineChannels.insert(pair<string, bool>(channelName, true));
			}
		}
	}

	// Internal indices
	addIndex(_("My Web Pages"), m_docsIndexLocation, true);
	addIndex(_("My Documents"), m_daemonIndexLocation, true);

	if (loadedUIConfiguration == false)
	{
		// Add default labels
		m_labels.insert(_("Important"));
		m_labels.insert(_("New"));
		m_labels.insert(_("Personal"));
		m_isBlackList = getDefaultPatterns(m_filePatternsList);

		// Create default queries
#ifdef HAVE_PWD_H
		struct passwd *pPasswd = getpwuid(geteuid());
		if (pPasswd != NULL)
		{
			string userName;

			if ((pPasswd->pw_gecos != NULL) &&
				(strlen(pPasswd->pw_gecos) > 0))
			{
				userName = pPasswd->pw_gecos;
			}
			else if (pPasswd->pw_name != NULL)
			{
				userName = pPasswd->pw_name;
			}

			if (userName.empty() == false)
			{
				QueryProperties queryProps(_("Me"), string("\"") + userName + string("\""));

				queryProps.setSortOrder(QueryProperties::DATE_DESC);
				addQuery(queryProps);
			}
		}
#endif

		QueryProperties queryProps(_("Latest First"), "dir:/");
		queryProps.setSortOrder(QueryProperties::DATE_DESC);
		addQuery(queryProps);
		queryProps = QueryProperties(_("10kb And Smaller"), "0..10240b");
		queryProps.setSortOrder(QueryProperties::SIZE_DESC);
		addQuery(queryProps);
		addQuery(QueryProperties(_("Home Stuff"), string("dir:") + getHomeDirectory()));
		addQuery(QueryProperties(_("With Label New"), "label:New"));
		addQuery(QueryProperties(_("Have CJKV"), "tokens:CJKV"));
		addQuery(QueryProperties(_("In English"), "lang:en"));
		addQuery(QueryProperties("Pinot search", "pinot search"));
	}

	return true;
}

bool PinotSettings::loadSearchEngines(const string &directoryName)
{
	if (directoryName.empty() == true)
	{
		return true;
	}

	DIR *pDir = opendir(directoryName.c_str());
	if (pDir == NULL)
	{
		return false;
	}

	// Iterate through this directory's entries
	struct dirent *pDirEntry = readdir(pDir);
	while (pDirEntry != NULL)
	{
		char *pEntryName = pDirEntry->d_name;
		if (pEntryName != NULL)
		{
			struct stat fileStat;
			string location = directoryName;
			location += "/";
			location += pEntryName;

			// Is that a file ?
			if ((stat(location.c_str(), &fileStat) == 0) &&
				(S_ISREG(fileStat.st_mode)))
			{
				SearchPluginProperties properties;

				if ((PluginWebEngine::getDetails(location, properties) == true) &&
					(properties.m_name.empty() == false) &&
					(properties.m_longName.empty() == false))
				{
					m_engineIds[1 << m_engines.size()] = properties.m_longName;
					if (properties.m_channel.empty() == true)
					{
						properties.m_channel = _("Unclassified");
					}
					// SearchPluginProperties derives ModuleProperties
					m_engines.insert(properties);
					if (m_engineChannels.find(properties.m_channel) == m_engineChannels.end())
					{
						m_engineChannels.insert(pair<string, bool>(properties.m_channel, true));
					}

					// Any editable parameters in this plugin ?
					for (map<string, string>::const_iterator editableIter = properties.m_editableParameters.begin();
						editableIter != properties.m_editableParameters.end(); ++editableIter)
					{
						// This may have been created when loading settings
						if (m_editablePluginValues.find(editableIter->second) == m_editablePluginValues.end())
						{
							m_editablePluginValues[editableIter->second] = "";
						}
					}
#ifdef DEBUG
					clog << "PinotSettings::loadSearchEngines: " << properties.m_name
						<< ", " << properties.m_longName << ", " << properties.m_option
						<< " has " << properties.m_editableParameters.size() << " editable values" << endl;
#endif
				}
			}
		}

		// Next entry
		pDirEntry = readdir(pDir);
	}
	closedir(pDir);

	return true;
}

bool PinotSettings::loadConfiguration(const string &fileName, bool isGlobal)
{
	struct stat fileStat;
	bool success = true;

	if ((stat(fileName.c_str(), &fileStat) != 0) ||
		(!S_ISREG(fileStat.st_mode)))
	{
		clog << "Couldn't open settings file " << fileName << endl;
		return false;
	}

	try
	{
		// Parse the settings file
		DomParser parser;
		parser.set_substitute_entities(true);
		parser.parse_file(fileName);
		xmlpp::Document *pDocument = parser.get_document();
		if (pDocument == NULL)
		{
			return false;
		}

		Element *pRootElem = pDocument->get_root_node();
		if (pRootElem == NULL)
		{
			return false;
		}

		// Check the top-level element is what we expect
		ustring rootNodeName = pRootElem->get_name();
		if (rootNodeName != "pinot")
		{
			return false;
		}

		// Go through the subnodes
		const Node::NodeList childNodes = pRootElem->get_children();
		if (childNodes.empty() == false)
		{
			for (Node::NodeList::const_iterator iter = childNodes.begin(); iter != childNodes.end(); ++iter)
			{
				Node *pNode = (*iter);
				// All nodes should be elements
				Element *pElem = dynamic_cast<Element*>(pNode);
				if (pElem == NULL)
				{
					continue;
				}

				string nodeName(pElem->get_name());
				string nodeContent(getElementContent(pElem));
				if (isGlobal == true)
				{
					if (nodeName == "cache")
					{
						loadCacheProviders(pElem);
					}
					else
					{
						// Unsupported element
						continue;
					}
				}
				else if (nodeName == "version")
				{
					m_version = nodeContent;
				}
				else if (nodeName == "warnaboutversion")
				{
					if (nodeContent == "YES")
					{
						m_warnAboutVersion = true;
					}
					else
					{
						m_warnAboutVersion = false;
					}
				}
				else if (nodeName == "backend")
				{
					m_defaultBackend = nodeContent;
				}
				else if (nodeName == "ui")
				{
					loadUi(pElem);
				}
				else if (nodeName == "extraindex")
				{
					loadIndexes(pElem);
				}
				else if (nodeName == "channel")
				{
					loadEngineChannels(pElem);
				}
				else if (nodeName == "storedquery")
				{
					loadQueries(pElem);
				}
				else if (nodeName == "label")
				{
					loadLabels(pElem);
				}
				else if (nodeName == "robots")
				{
					if (nodeContent == "IGNORE")
					{
						m_ignoreRobotsDirectives = true;
					}
					else
					{
						m_ignoreRobotsDirectives = false;
					}
				}
				else if (nodeName == "suggestterms")
				{
					if (nodeContent == "YES")
					{
						m_suggestQueryTerms = true;
					}
					else
					{
						m_suggestQueryTerms = false;
					}
				}
				else if (nodeName == "newresults")
				{
					loadColour(pElem);
				}
				else if (nodeName == "proxy")
				{
					loadProxy(pElem);
				}
				else if (nodeName == "indexable")
				{
					loadIndexableLocations(pElem);
				}
				else if ((nodeName == "blacklist") ||
					(nodeName == "patterns"))
				{
					loadFilePatterns(pElem);
				}
				else if (nodeName == "pluginparameters")
				{
					loadPluginParameters(pElem);
				}
			}
		}
	}
	catch (const std::exception& ex)
	{
		clog << "Couldn't parse settings file: "
			<< ex.what() << endl;
		success = false;
	}

	return success;
}

bool PinotSettings::loadUi(const Element *pElem)
{
	if (pElem == NULL)
	{
		return false;
	}

	Node::NodeList childNodes = pElem->get_children();
	if (childNodes.empty() == true)
	{
		return false;
	}

	for (Node::NodeList::iterator iter = childNodes.begin(); iter != childNodes.end(); ++iter)
	{
		Node *pNode = (*iter);
		Element *pChildElem = dynamic_cast<Element*>(pNode);
		if (pChildElem == NULL)
		{
			continue;
		}

		string nodeName(pChildElem->get_name());
		string nodeContent(getElementContent(pChildElem));
		if (nodeName == "xpos")
		{
			m_xPos = atoi(nodeContent.c_str());
		}
		else if (nodeName == "ypos")
		{
			m_yPos = atoi(nodeContent.c_str());
		}
		else if (nodeName == "width")
		{
			m_width = atoi(nodeContent.c_str());
		}
		else if (nodeName == "height")
		{
			m_height = atoi(nodeContent.c_str());
		}
		else if (nodeName == "panepos")
		{
			m_panePos = atoi(nodeContent.c_str());
		}
		else if (nodeName == "expandqueries")
		{
			if (nodeContent == "YES")
			{
				m_expandQueries = true;
			}
			else
			{
				m_expandQueries = false;
			}
		}
		else if (nodeName == "showengines")
		{
			if (nodeContent == "YES")
			{
				m_showEngines = true;
			}
			else
			{
				m_showEngines = false;
			}
		}
	}

	return true;
}

bool PinotSettings::loadIndexes(const Element *pElem)
{
	if (pElem == NULL)
	{
		return false;
	}

	Node::NodeList childNodes = pElem->get_children();
	if (childNodes.empty() == true)
	{
		return false;
	}

	string indexName, indexLocation;

	for (Node::NodeList::iterator iter = childNodes.begin(); iter != childNodes.end(); ++iter)
	{
		Node *pNode = (*iter);
		Element *pChildElem = dynamic_cast<Element*>(pNode);
		if (pChildElem == NULL)
		{
			continue;
		}

		string nodeName(pChildElem->get_name());
		string nodeContent(getElementContent(pChildElem));
		if (nodeName == "name")
		{
			indexName = nodeContent;
		}
		else if (nodeName == "location")
		{
			indexLocation = nodeContent;
		}
	}

	if ((indexName.empty() == false) &&
		(indexLocation.empty() == false))
	{
		addIndex(indexName, indexLocation, false);
	}

	return true;
}

bool PinotSettings::loadEngineChannels(const Element *pElem)
{
	if (pElem == NULL)
	{
		return false;
	}

	Node::NodeList childNodes = pElem->get_children();
	if (childNodes.empty() == true)
	{
		return false;
	}

	for (Node::NodeList::iterator iter = childNodes.begin(); iter != childNodes.end(); ++iter)
	{
		Node *pNode = (*iter);
		Element *pChildElem = dynamic_cast<Element*>(pNode);
		if (pChildElem == NULL)
		{
			continue;
		}

		string nodeName(pChildElem->get_name());
		string nodeContent(getElementContent(pChildElem));
		if (nodeName == "name")
		{
			std::map<string, bool>::iterator channelIter = m_engineChannels.find(nodeContent);

			if (channelIter != m_engineChannels.end())
			{
				channelIter->second = false;
			}
			else
			{
				m_engineChannels.insert(pair<string, bool>(nodeContent, false));
			}
		}
	}

	return true;
}

bool PinotSettings::loadQueries(const Element *pElem)
{
	if (pElem == NULL)
	{
		return false;
	}

	Node::NodeList childNodes = pElem->get_children();
	if (childNodes.empty() == true)
	{
		return false;
	}

	QueryProperties queryProps;
	Date minDate, maxDate;
	string freeQuery;
	bool enableMinDate = false, enableMaxDate = false;

	// Load the query's properties
	for (Node::NodeList::iterator iter = childNodes.begin(); iter != childNodes.end(); ++iter)
	{
		Node *pNode = (*iter);
		Element *pChildElem = dynamic_cast<Element*>(pNode);
		if (pChildElem == NULL)
		{
			continue;
		}

		string nodeName(pChildElem->get_name());
		string nodeContent(getElementContent(pChildElem));
		if (nodeName == "name")
		{
			queryProps.setName(nodeContent);
		}
		else if (nodeName == "sortorder")
		{
			if ((nodeContent == "DATE") ||
				(nodeContent == "DATE_DESC"))
			{
				queryProps.setSortOrder(QueryProperties::DATE_DESC);
			}
			else if (nodeContent == "DATE_ASC")
			{
				queryProps.setSortOrder(QueryProperties::DATE_ASC);
			}
			else if (nodeContent == "SIZE_DESC")
			{
				queryProps.setSortOrder(QueryProperties::SIZE_DESC);
			}
			else
			{
				queryProps.setSortOrder(QueryProperties::RELEVANCE);
			}
		}
		else if (nodeName == "text")
		{
			freeQuery = nodeContent;
		}
		else if ((nodeName == "and") &&
			(nodeContent.empty() == false))
		{
			if (freeQuery.empty() == false)
			{
				freeQuery += " ";
			}
			freeQuery += nodeContent;
		}
		else if ((nodeName == "phrase") &&
			(nodeContent.empty() == false))
		{
			if (freeQuery.empty() == false)
			{
				freeQuery += " ";
			}
			freeQuery += "\"";
			freeQuery += nodeContent;
			freeQuery += "\"";
		}
		else if ((nodeName == "any") &&
			(nodeContent.empty() == false))
		{
			// FIXME: don't be lazy and add those correctly
			if (freeQuery.empty() == false)
			{
				freeQuery += " ";
			}
			freeQuery += nodeContent;
		}
		else if ((nodeName == "not") &&
			(nodeContent.empty() == false))
		{
			if (freeQuery.empty() == false)
			{
				freeQuery += " ";
			}
			freeQuery += "-(";
			freeQuery += nodeContent;
			freeQuery += ")";
		}
		else if ((nodeName == "language") &&
			(nodeContent.empty() == false))
		{
			if (freeQuery.empty() == false)
			{
				freeQuery += " ";
			}
			freeQuery += "lang:";
			freeQuery += nodeContent;
		}
		else if ((nodeName == "stemlanguage") &&
			(nodeContent.empty() == false))
		{
			queryProps.setStemmingLanguage(Languages::toLocale(nodeContent));
		}
		else if ((nodeName == "hostfilter") &&
			(nodeContent.empty() == false))
		{
			if (freeQuery.empty() == false)
			{
				freeQuery += " ";
			}
			freeQuery += "site:";
			freeQuery += nodeContent;
		}
		else if ((nodeName == "filefilter") &&
			(nodeContent.empty() == false))
		{
			if (freeQuery.empty() == false)
			{
				freeQuery += " ";
			}
			freeQuery += "file:";
			freeQuery += nodeContent;
		}
		else if ((nodeName == "labelfilter") &&
			(nodeContent.empty() == false))
		{
			if (freeQuery.empty() == false)
			{
				freeQuery += " ";
			}
			freeQuery += "label:";
			freeQuery += nodeContent;
		}
		else if (nodeName == "maxresults")
		{
			int count = atoi(nodeContent.c_str());
			queryProps.setMaximumResultsCount((unsigned int)max(count, 10));
		}
		else if (nodeName == "enablemindate")
		{
			if (nodeContent == "YES")
			{
				enableMinDate = true;
			}
		}
		else if (nodeName == "mindate")
		{
			minDate.set_parse(nodeContent);
		}
		else if (nodeName == "enablemaxdate")
		{
			if (nodeContent == "YES")
			{
				enableMaxDate = true;
			}
		}
		else if (nodeName == "maxdate")
		{
			maxDate.set_parse(nodeContent);
		}
		else if (nodeName == "index")
		{
			if (nodeContent == "NEW")
			{
				queryProps.setIndexResults(QueryProperties::NEW_RESULTS);
			}
			else if (nodeContent == "ALL")
			{
				queryProps.setIndexResults(QueryProperties::ALL_RESULTS);
			}
			else
			{
				queryProps.setIndexResults(QueryProperties::NOTHING);
			}
		}
		else if (nodeName == "label")
		{
			queryProps.setLabelName(nodeContent);
		}
		else if (nodeName == "modified")
		{
			if (nodeContent == "YES")
			{
				queryProps.setModified(true);
			}
		}
	}

	// Are pre-0.80 dates specified ?
	if ((enableMinDate == true) ||
		(enableMaxDate == true))
	{
		// Provide reasonable defaults
		if (enableMinDate == false)
		{
			minDate.set_day(1);
			minDate.set_month(Date::JANUARY);
			minDate.set_year(1970);
		}
		if (enableMaxDate == false)
		{
			maxDate.set_day(31);
			maxDate.set_month(Date::DECEMBER);
			maxDate.set_year(2099);
		}

		ustring minDateStr(minDate.format_string("%Y%m%d"));
		ustring maxDateStr(maxDate.format_string("%Y%m%d"));

#ifdef DEBUG
		clog << "PinotSettings::loadQueries: date range " << minDateStr << " to " << maxDateStr << endl;
#endif
		freeQuery += " ";
		freeQuery += minDateStr;
		freeQuery += "..";
		freeQuery += maxDateStr;
	}

	// We need at least a name
	if (queryProps.getName().empty() == false)
	{
		if (freeQuery.empty() == false)
		{
			queryProps.setFreeQuery(freeQuery);
		}
		m_queries[queryProps.getName()] = queryProps;
	}

	return true;
}

bool PinotSettings::loadLabels(const Element *pElem)
{
	if (pElem == NULL)
	{
		return false;
	}

	Node::NodeList childNodes = pElem->get_children();
	if (childNodes.empty() == true)
	{
		return false;
	}

	// Load the label's properties
	for (Node::NodeList::iterator iter = childNodes.begin(); iter != childNodes.end(); ++iter)
	{
		Node *pNode = (*iter);
		Element *pChildElem = dynamic_cast<Element*>(pNode);
		if (pChildElem == NULL)
		{
			continue;
		}

		string nodeName(pChildElem->get_name());
		string nodeContent(getElementContent(pChildElem));

		if (nodeName == "name")
		{
			m_labels.insert(nodeContent);
		}
		// Labels used to have colours...
	}

	return true;
}

bool PinotSettings::loadColour(const Element *pElem)
{
	if (pElem == NULL)
	{
		return false;
	}

	Node::NodeList childNodes = pElem->get_children();
	if (childNodes.empty() == true)
	{
		return false;
	}

	// Load the colour RGB components
	for (Node::NodeList::iterator iter = childNodes.begin(); iter != childNodes.end(); ++iter)
	{
		Node *pNode = (*iter);
		Element *pChildElem = dynamic_cast<Element*>(pNode);
		if (pChildElem == NULL)
		{
			continue;
		}

		string nodeName(pChildElem->get_name());
		string nodeContent(getElementContent(pChildElem));
		gushort value = (gushort)atoi(nodeContent.c_str());

		if (nodeName == "red")
		{
			m_newResultsColourRed = value;
		}
		else if (nodeName == "green")
		{
			m_newResultsColourGreen = value;
		}
		else if (nodeName == "blue")
		{
			m_newResultsColourBlue = value;
		}
	}

	return true;
}

bool PinotSettings::loadProxy(const Element *pElem)
{
	if (pElem == NULL)
	{
		return false;
	}

	Node::NodeList childNodes = pElem->get_children();
	if (childNodes.empty() == true)
	{
		return false;
	}

	for (Node::NodeList::iterator iter = childNodes.begin(); iter != childNodes.end(); ++iter)
	{
		Node *pNode = (*iter);
		Element *pChildElem = dynamic_cast<Element*>(pNode);
		if (pChildElem == NULL)
		{
			continue;
		}

		string nodeName(pChildElem->get_name());
		string nodeContent(getElementContent(pChildElem));

		if (nodeName == "address")
		{
			m_proxyAddress = nodeContent;
		}
		else if (nodeName == "port")
		{
			m_proxyPort = (unsigned int)atoi(nodeContent.c_str());
		}
		else if (nodeName == "type")
		{
			m_proxyType = nodeContent;
		}
		else if (nodeName == "enable")
		{
			if (nodeContent == "YES")
			{
				m_proxyEnabled = true;
			}
			else
			{
				m_proxyEnabled = false;
			}
		}	
	}

	return true;
}

bool PinotSettings::loadIndexableLocations(const Element *pElem)
{
	if (pElem == NULL)
	{
		return false;
	}

	Node::NodeList childNodes = pElem->get_children();
	if (childNodes.empty() == true)
	{
		return false;
	}

	IndexableLocation location;

	// Load the indexable location's properties
	for (Node::NodeList::iterator iter = childNodes.begin(); iter != childNodes.end(); ++iter)
	{
		Node *pNode = (*iter);
		Element *pChildElem = dynamic_cast<Element*>(pNode);
		if (pChildElem == NULL)
		{
			continue;
		}

		string nodeName(pChildElem->get_name());
		string nodeContent(getElementContent(pChildElem));

		if (nodeName == "name")
		{
			location.m_name = nodeContent;
		}
		else if (nodeName == "monitor")
		{
			if (nodeContent == "YES")
			{
				location.m_monitor = true;
			}
			else
			{
				location.m_monitor = false;
			}
		}
	}

	if (location.m_name.empty() == false)
	{
		location.m_isSource = true;

		m_indexableLocations.insert(location);
	}

	return true;
}

bool PinotSettings::loadFilePatterns(const Element *pElem)
{
	if (pElem == NULL)
	{
		return false;
	}

	Node::NodeList childNodes = pElem->get_children();
	if (childNodes.empty() == true)
	{
		return false;
	}

	// Load the file patterns list
	for (Node::NodeList::iterator iter = childNodes.begin(); iter != childNodes.end(); ++iter)
	{
		Node *pNode = (*iter);
		Element *pChildElem = dynamic_cast<Element*>(pNode);
		if (pChildElem == NULL)
		{
			continue;
		}

		string nodeName(pChildElem->get_name());
		string nodeContent(getElementContent(pChildElem));

		if (nodeName == "pattern")
		{
			m_filePatternsList.insert(nodeContent);
		}
		else if (nodeName == "forbid")
		{
			if (nodeContent == "YES")
			{
				m_isBlackList = true;
			}
			else
			{
				m_isBlackList = false;
			}
		}
	}

	return true;
}

bool PinotSettings::loadPluginParameters(const Element *pElem)
{
	if (pElem == NULL)
	{
		return false;
	}

	Node::NodeList childNodes = pElem->get_children();
	if (childNodes.empty() == true)
	{
		return false;
	}

	string name, value;

	// Load the plugin parameters' values 
	for (Node::NodeList::iterator iter = childNodes.begin(); iter != childNodes.end(); ++iter)
	{
		Node *pNode = (*iter);
		Element *pChildElem = dynamic_cast<Element*>(pNode);
		if (pChildElem == NULL)
		{
			continue;
		}

		string nodeName(pChildElem->get_name());
		string nodeContent(getElementContent(pChildElem));

		if (nodeName == "name")
		{
			name = nodeContent;
		}
		else if (nodeName == "value")
		{
			value = nodeContent;
		}
	}

	m_editablePluginValues[name] = value;

	return true;
}

bool PinotSettings::loadCacheProviders(const Element *pElem)
{
	if (pElem == NULL)
	{
		return false;
	}

	Node::NodeList childNodes = pElem->get_children();
	if (childNodes.empty() == true)
	{
		return false;
	}

	CacheProvider cacheProvider;

	// Load the cache provider's properties
	for (Node::NodeList::iterator iter = childNodes.begin(); iter != childNodes.end(); ++iter)
	{
		Node *pNode = (*iter);
		Element *pChildElem = dynamic_cast<Element*>(pNode);
		if (pChildElem == NULL)
		{
			continue;
		}

		string nodeName(pChildElem->get_name());
		string nodeContent(getElementContent(pChildElem));

		if (nodeName == "name")
		{
			cacheProvider.m_name = nodeContent;
		}
		else if (nodeName == "location")
		{
			cacheProvider.m_location = nodeContent;
		}
		else if (nodeName == "protocols")
		{
			nodeContent += ",";

			ustring::size_type previousPos = 0, commaPos = nodeContent.find(",");
			while (commaPos != ustring::npos)
			{
				string protocol(nodeContent.substr(previousPos,
                                        commaPos - previousPos));

				StringManip::trimSpaces(protocol);
				cacheProvider.m_protocols.insert(protocol);

				// Next
				previousPos = commaPos + 1;
				commaPos = nodeContent.find(",", previousPos);
			}
		}
	}

	if ((cacheProvider.m_name.empty() == false) &&
		(cacheProvider.m_location.empty() == false))
	{
		m_cacheProviders.push_back(cacheProvider);

		// Copy the list of protocols supported by this cache provider
		copy(cacheProvider.m_protocols.begin(), cacheProvider.m_protocols.end(),
			inserter(m_cacheProtocols, m_cacheProtocols.begin()));
	}

	return true;
}

bool PinotSettings::save(SaveWhat what)
{
	Element *pRootElem = NULL;
	Element *pElem = NULL;
	char numStr[64];
	bool prefsOrUI = true;

	if (what == SAVE_CONFIG)
	{
		prefsOrUI = false;
	}

	try
	{
		xmlpp::Document doc("1.0");

		// Create a new node
		pRootElem = doc.create_root_node("pinot");
		if (pRootElem == NULL)
		{
			return false;
		}
		// ...with text children nodes
		addChildElement(pRootElem, "version", VERSION);
		if (what == SAVE_CONFIG)
		{
			addChildElement(pRootElem, "warnaboutversion", (m_warnAboutVersion ? "YES" : "NO"));
			// User interface position and size
			pElem = pRootElem->add_child("ui");
			if (pElem == NULL)
			{
				return false;
			}
			snprintf(numStr, 64, "%d", m_xPos);
			addChildElement(pElem, "xpos", numStr);
			snprintf(numStr, 64, "%d", m_yPos);
			addChildElement(pElem, "ypos", numStr);
			snprintf(numStr, 64, "%d", m_width);
			addChildElement(pElem, "width", numStr);
			snprintf(numStr, 64, "%d", m_height);
			addChildElement(pElem, "height", numStr);
			snprintf(numStr, 64, "%d", m_panePos);
			addChildElement(pElem, "panepos", numStr);
			addChildElement(pElem, "expandqueries", (m_expandQueries ? "YES" : "NO"));
			addChildElement(pElem, "showengines", (m_showEngines ? "YES" : "NO"));
			// User-defined indexes
			for (set<IndexProperties>::iterator indexIter = m_indexes.begin(); indexIter != m_indexes.end(); ++indexIter)
			{
				if (indexIter->m_internal == true)
				{
					continue;
				}

				pElem = pRootElem->add_child("extraindex");
				if (pElem == NULL)
				{
					return false;
				}
				addChildElement(pElem, "name", indexIter->m_name);
				addChildElement(pElem, "location", indexIter->m_location);
			}
			// Engine channels
			for (map<string, bool>::iterator channelIter = m_engineChannels.begin();
				channelIter != m_engineChannels.end(); ++channelIter)
			{
				// Only save those whose group was collapsed
				if (channelIter->second == false)
				{
					pElem = pRootElem->add_child("channel");
					if (pElem == NULL)
					{
						return false;
					}
					addChildElement(pElem, "name", channelIter->first);
				}
			}
			// User-defined queries
			for (map<string, QueryProperties>::iterator queryIter = m_queries.begin();
				queryIter != m_queries.end(); ++queryIter)
			{
				pElem = pRootElem->add_child("storedquery");
				if (pElem == NULL)
				{
					return false;
				}

				string sortOrder("RELEVANCE");

				if (queryIter->second.getSortOrder() == QueryProperties::DATE_DESC)
				{
					sortOrder = "DATE";
				}
				else if (queryIter->second.getSortOrder() == QueryProperties::DATE_ASC)
				{
					sortOrder = "DATE_ASC";
				}
				else if (queryIter->second.getSortOrder() == QueryProperties::SIZE_DESC)
				{
					sortOrder = "SIZE_DESC";
				}
				addChildElement(pElem, "name", queryIter->first);
				addChildElement(pElem, "sortorder", sortOrder);
				addChildElement(pElem, "text", queryIter->second.getFreeQuery());
				addChildElement(pElem, "stemlanguage", Languages::toEnglish(queryIter->second.getStemmingLanguage()));
				snprintf(numStr, 64, "%u", queryIter->second.getMaximumResultsCount());
				addChildElement(pElem, "maxresults", numStr);
				QueryProperties::IndexWhat indexResults = queryIter->second.getIndexResults();
				if (indexResults == QueryProperties::NEW_RESULTS)
				{
					addChildElement(pElem, "index", "NEW");
				}
				else if (indexResults == QueryProperties::ALL_RESULTS)
				{
					addChildElement(pElem, "index", "ALL");
				}
				else
				{
					addChildElement(pElem, "index", "NONE");
				}
				addChildElement(pElem, "label", queryIter->second.getLabelName());
				addChildElement(pElem, "modified", (queryIter->second.getModified() ? "YES" : "NO"));
			}
		}
		if (what == SAVE_PREFS)
		{
			addChildElement(pRootElem, "backend", m_defaultBackend);
			// Labels
			for (set<string>::iterator labelIter = m_labels.begin(); labelIter != m_labels.end(); ++labelIter)
			{
				pElem = pRootElem->add_child("label");
				if (pElem == NULL)
				{
					return false;
				}
				addChildElement(pElem, "name", *labelIter);
			}
			// Ignore robots directives
			addChildElement(pRootElem, "robots", (m_ignoreRobotsDirectives ? "IGNORE" : "OBEY"));
			// Enable terms suggestion
			addChildElement(pRootElem, "suggestterms", (m_suggestQueryTerms ? "YES" : "NO"));
			// New results colour
			pElem = pRootElem->add_child("newresults");
			if (pElem == NULL)
			{
				return false;
			}
			snprintf(numStr, 64, "%u", m_newResultsColourRed);
			addChildElement(pElem, "red", numStr);
			snprintf(numStr, 64, "%u", m_newResultsColourGreen);
			addChildElement(pElem, "green", numStr);
			snprintf(numStr, 64, "%u", m_newResultsColourBlue);
			addChildElement(pElem, "blue", numStr);
			// Proxy
			pElem = pRootElem->add_child("proxy");
			if (pElem == NULL)
			{
				return false;
			}
			addChildElement(pElem, "address", m_proxyAddress);
			snprintf(numStr, 64, "%u", m_proxyPort);
			addChildElement(pElem, "port", numStr);
			addChildElement(pElem, "type", m_proxyType);
			addChildElement(pElem, "enable", (m_proxyEnabled ? "YES" : "NO"));
			// Locations to index 
			for (set<IndexableLocation>::iterator locationIter = m_indexableLocations.begin();
				locationIter != m_indexableLocations.end(); ++locationIter)
			{
				pElem = pRootElem->add_child("indexable");
				if (pElem == NULL)
				{
					return false;
				}
				addChildElement(pElem, "name", locationIter->m_name);
				addChildElement(pElem, "monitor", (locationIter->m_monitor ? "YES" : "NO"));
			}
			// File patterns
			pElem = pRootElem->add_child("patterns");
			if (pElem == NULL)
			{
				return false;
			}
			for (set<ustring>::iterator patternIter = m_filePatternsList.begin();
				patternIter != m_filePatternsList.end() ; ++patternIter)
			{
				addChildElement(pElem, "pattern", *patternIter);
			}
			addChildElement(pElem, "forbid", (m_isBlackList ? "YES" : "NO"));
			// Values of editable plugin parameters
			for (map<string, string>::iterator editableIter = m_editablePluginValues.begin();
				editableIter != m_editablePluginValues.end() ; ++editableIter)
			{
				if (editableIter->second.empty() == true)
				{
					continue;
				}

				pElem = pRootElem->add_child("pluginparameters");
				if (pElem == NULL)
				{
					return false;
				}
				addChildElement(pElem, "name", editableIter->first);
				addChildElement(pElem, "value", editableIter->second);
			}
		}
#ifdef DEBUG
		clog << "PinotSettings::save: saving to " << getFileName(prefsOrUI) << endl;
#endif

		// Save to file
		doc.write_to_file_formatted(getFileName(prefsOrUI));
	}
	catch (const std::exception& ex)
	{
		clog << "Couldn't save settings file: "
			<< ex.what() << endl;
		return false;
	}

	return true;
}

/// Returns the indexes set.
const set<PinotSettings::IndexProperties> &PinotSettings::getIndexes(void) const
{
	return m_indexes;
}

/// Adds a new index.
bool PinotSettings::addIndex(const ustring &name, const string &location, bool isInternal)
{
	unsigned int indexId(1 << m_indexCount);
	m_indexes.insert(IndexProperties(name, location, indexId, isInternal));
#ifdef DEBUG
	clog << "PinotSettings::addIndex: index " << m_indexCount << " is " << name << " with ID " << indexId << endl;
#endif
	++m_indexCount;

	return true;
}

/// Removes an index.
bool PinotSettings::removeIndex(const IndexProperties &indexProps)
{
	// Remove from the names map
	set<IndexProperties>::iterator namesMapIter = m_indexes.find(indexProps);
	if (namesMapIter != m_indexes.end())
	{
		m_indexes.erase(namesMapIter);

		return true;
	}

	return false;
}

/// Clears the indexes map.
void PinotSettings::clearIndexes(void)
{
	// Clear and reinsert the internal indexes
	m_indexes.clear();
	m_indexCount = 0;
	addIndex(_("My Web Pages"), m_docsIndexLocation, true);
	addIndex(_("My Documents"), m_daemonIndexLocation, true);
}

/// Returns properties of the given index.
PinotSettings::IndexProperties PinotSettings::getIndexPropertiesByName(const string &name) const
{
	for (set<IndexProperties>::const_iterator propsIter = m_indexes.begin();
		propsIter != m_indexes.end(); ++propsIter)
	{
		if (propsIter->m_name == name)
		{
			return *propsIter;
		}
	}

	return IndexProperties();
}

/// Returns properties of the given index.
PinotSettings::IndexProperties PinotSettings::getIndexPropertiesByLocation(const string &location) const
{
	for (set<IndexProperties>::const_iterator propsIter = m_indexes.begin();
		propsIter != m_indexes.end(); ++propsIter)
	{
		if (propsIter->m_location == location)
		{
			return *propsIter;
		}
	}

	return IndexProperties();
}

/// Returns the name(s) for the given ID.
void PinotSettings::getIndexNames(unsigned int id, set<string> &names)
{
	names.clear();

	// Make sure indexes are or were defined
	if (m_indexCount == 0)
	{
		return;
	}

	unsigned indexId = 1 << (m_indexCount - 1);
	do
	{
		if (id & indexId)
		{
			for (set<IndexProperties>::const_iterator propsIter = m_indexes.begin();
				propsIter != m_indexes.end(); ++propsIter)
			{
				if (propsIter->m_id == indexId)
				{
#ifdef DEBUG
					clog << "PinotSettings::getIndexNames: index " << indexId << " is " << propsIter->m_name << endl;
#endif
					// Get the associated name
					names.insert(propsIter->m_name);
				}
			}
		}
		// Shift to the right
		indexId = indexId >> 1;
	} while (indexId > 0);
}

/// Returns an IndexInterface for the given index location.
IndexInterface *PinotSettings::getIndex(const string &location)
{
	if (location == m_docsIndexLocation)
	{
		return ModuleFactory::getIndex(m_defaultBackend, m_docsIndexLocation);
	}
	else if ((m_clientMode == true) &&
		(location == m_daemonIndexLocation))
	{
		return ModuleFactory::getIndex("dbus-" + m_defaultBackend, m_daemonIndexLocation);
	}

	return ModuleFactory::getIndex(m_defaultBackend, location);
}

/// Returns the search engines set.
bool PinotSettings::getSearchEngines(set<ModuleProperties> &engines, const string &channelName) const
{
	if (channelName.empty() == true)
	{
		// Copy the whole list of search engines
		copy(m_engines.begin(), m_engines.end(), inserter(engines, engines.begin()));
	}
	else
	{
		if (m_engineChannels.find(channelName) == m_engineChannels.end())
		{
			// Unknown channel name
			return false;
		}

		// Copy engines that belong to the given channel
		for (set<ModuleProperties>::iterator engineIter = m_engines.begin(); engineIter != m_engines.end(); ++engineIter)
		{
			if (engineIter->m_channel == channelName)
			{
#ifdef DEBUG
				clog << "PinotSettings::getSearchEngines: engine " << engineIter->m_longName << " in channel " << channelName << endl;
#endif
				engines.insert(*engineIter);
			}
		}
	}

	return true;
}

/// Returns an ID that identifies the given engine name.
unsigned int PinotSettings::getEngineId(const string &name)
{
	unsigned int engineId = 0;

	for (map<unsigned int, string>::iterator mapIter = m_engineIds.begin();
		mapIter != m_engineIds.end(); ++mapIter)
	{
		if (mapIter->second == name)
		{
			engineId = mapIter->first;
			break;
		}
	}
#ifdef DEBUG
	clog << "PinotSettings::getEngineId: " << name << ", ID " << engineId << endl;
#endif

	return engineId;
}

/// Returns the name for the given ID.
void PinotSettings::getEngineNames(unsigned int id, set<string> &names)
{
	names.clear();

	// Make sure there are search engines defined
	if (m_engines.empty() == true)
	{
		return;
	}

	unsigned engineId = 1 << (m_engines.size() - 1);
	do
	{
		if (id & engineId)
		{
			map<unsigned int, string>::iterator mapIter = m_engineIds.find(engineId);
			if (mapIter != m_engineIds.end())
			{
				// Get the associated name
				names.insert(mapIter->second);
			}
		}
		// Shift to the right
		engineId = engineId >> 1;
	} while (engineId > 0);
}

/// Returns the search engines channels.
map<string, bool> &PinotSettings::getSearchEnginesChannels(void)
{
	return m_engineChannels;
}

/// Returns the queries map, keyed by name.
const map<string, QueryProperties> &PinotSettings::getQueries(void) const
{
	return m_queries;
}

/// Adds a new query.
bool PinotSettings::addQuery(const QueryProperties &properties)
{
	string name(properties.getName());

	map<string, QueryProperties>::iterator queryIter = m_queries.find(name);
	if (queryIter == m_queries.end())
	{
		// Okay, no such query exists
		m_queries[name] = properties;

		return true;
	}

	return false;
}

/// Removes a query.
bool PinotSettings::removeQuery(const string &name)
{
	// Remove from the map
	map<string, QueryProperties>::iterator queryIter = m_queries.find(name);
	if (queryIter != m_queries.end())
	{
		m_queries.erase(queryIter);

		return true;
	}

	return false;
}

/// Clears the queries map.
void PinotSettings::clearQueries(void)
{
	m_queries.clear();
}

/// Gets default patterns.
bool PinotSettings::getDefaultPatterns(set<ustring> &defaultPatterns)
{
	defaultPatterns.clear();

	// Skip common image, video and archive types
	defaultPatterns.insert("*~");
	defaultPatterns.insert("*.a");
	defaultPatterns.insert("*.asf");
	defaultPatterns.insert("*.avi");
	defaultPatterns.insert("*.aux");
	defaultPatterns.insert("*CVS");
	defaultPatterns.insert("*.cap");
	defaultPatterns.insert("*.divx");
	defaultPatterns.insert("*.flv");
	defaultPatterns.insert("*.git");
	defaultPatterns.insert("*.gmo");
	defaultPatterns.insert("*.iso");
	defaultPatterns.insert("*.la");
	defaultPatterns.insert("*.lha");
	defaultPatterns.insert("*.lo");
	defaultPatterns.insert("*.loT");
	defaultPatterns.insert("*.m4");
	defaultPatterns.insert("*.mov");
	defaultPatterns.insert("*.msf");
	defaultPatterns.insert("*.mpeg");
	defaultPatterns.insert("*.mp4");
	defaultPatterns.insert("*.mpg");
	defaultPatterns.insert("*.mo");
	defaultPatterns.insert("*.o");
	defaultPatterns.insert("*.omf");
	defaultPatterns.insert("*.orig");
	defaultPatterns.insert("*.part");
	defaultPatterns.insert("*.pc");
	defaultPatterns.insert("*.po");
	defaultPatterns.insert("*.rar");
	defaultPatterns.insert("*.rej");
	defaultPatterns.insert("*.sh");
	defaultPatterns.insert("*.so");
	defaultPatterns.insert("*.svn");
	defaultPatterns.insert("*.tmp");
	defaultPatterns.insert("*.torrent");
	defaultPatterns.insert("*.vm*");
	defaultPatterns.insert("*.wmv");
	defaultPatterns.insert("*.xbm");
	defaultPatterns.insert("*.xpm");

	return true;
}

/// Determines if a file matches the blacklist.
bool PinotSettings::isBlackListed(const string &fileName)
{
	if (m_filePatternsList.empty() == true)
	{
		if (m_isBlackList == true)
		{
			// There is no black-list
			return false;
		}

		// The file is not in the (empty) whitelist
		return true;
	}

#ifdef HAVE_FNMATCH_H
	// Any pattern matches this file name ?
	for (set<ustring>::iterator patternIter = m_filePatternsList.begin(); patternIter != m_filePatternsList.end() ; ++patternIter)
	{
		if (fnmatch(patternIter->c_str(), fileName.c_str(), FNM_NOESCAPE) == 0)
		{
			// Fail if it's in the blacklist, let the file through otherwise
			return m_isBlackList;
		}
	}
#endif

	return !m_isBlackList;
}

PinotSettings::IndexableLocation::IndexableLocation() :
	m_monitor(false),
	m_isSource(true)
{
}

PinotSettings::IndexableLocation::IndexableLocation(const IndexableLocation &other) :
	m_monitor(other.m_monitor),
	m_name(other.m_name),
	m_isSource(other.m_isSource)
{
}

PinotSettings::IndexableLocation::~IndexableLocation()
{
}

PinotSettings::IndexableLocation &PinotSettings::IndexableLocation::operator=(const IndexableLocation &other)
{
	if (this != &other)
	{
		m_monitor = other.m_monitor;
		m_name = other.m_name;
		m_isSource = other.m_isSource;
	}

	return *this;
}

bool PinotSettings::IndexableLocation::operator<(const IndexableLocation &other) const
{
	if (m_name < other.m_name)
	{
		return true;
	}

	return false;
}

bool PinotSettings::IndexableLocation::operator==(const IndexableLocation &other) const
{
	if (m_name == other.m_name)
	{
		return true;
	}

	return false;
}

PinotSettings::CacheProvider::CacheProvider()
{
}

PinotSettings::CacheProvider::CacheProvider(const CacheProvider &other) :
	m_name(other.m_name),
	m_location(other.m_location)
{
	m_protocols.clear();
	copy(other.m_protocols.begin(), other.m_protocols.end(),
		inserter(m_protocols, m_protocols.begin()));
}

PinotSettings::CacheProvider::~CacheProvider()
{
}

PinotSettings::CacheProvider &PinotSettings::CacheProvider::operator=(const CacheProvider &other)
{
	if (this != &other)
	{
		m_name = other.m_name;
		m_location = other.m_location;
		m_protocols.clear();
		copy(other.m_protocols.begin(), other.m_protocols.end(),
			inserter(m_protocols, m_protocols.begin()));
	}

	return *this;
}

bool PinotSettings::CacheProvider::operator<(const CacheProvider &other) const
{
	if (m_name < other.m_name)
	{
		return true;
	}

	return false;
}

bool PinotSettings::CacheProvider::operator==(const CacheProvider &other) const
{
	if (m_name == other.m_name)
	{
		return true;
	}

	return false;
}

PinotSettings::IndexProperties::IndexProperties() :
	m_id(0),
	m_internal(false)
{
}

PinotSettings::IndexProperties::IndexProperties(const ustring &name,
	const string &location, unsigned int id, bool isInternal) :
	m_name(name),
	m_location(location),
	m_id(id),
	m_internal(isInternal)
{
}

PinotSettings::IndexProperties::IndexProperties(const IndexProperties &other) :
	m_name(other.m_name),
	m_location(other.m_location),
	m_id(other.m_id),
	m_internal(other.m_internal)
{
}

PinotSettings::IndexProperties::~IndexProperties()
{
}

PinotSettings::IndexProperties& PinotSettings::IndexProperties::operator=(const IndexProperties &other)
{
	if (this != &other)
	{
		m_name = other.m_name;
		m_location = other.m_location;
		m_id = other.m_id;
		m_internal = other.m_internal;
	}

	return *this;
}

bool PinotSettings::IndexProperties::operator<(const IndexProperties &other) const
{
	if (m_id < other.m_id)
	{
		return true;
	}
	else if (m_id == other.m_id)
	{
		if (m_name < other.m_name)
		{
			return true;
		}
	}

	return false;
}

bool PinotSettings::IndexProperties::operator==(const IndexProperties &other) const
{
	if (m_id == other.m_id)
	{
		return true;
	}

	return false;
}
