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

#ifndef _PINOTSETTINGS_HH
#define _PINOTSETTINGS_HH

#include <sys/types.h>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <vector>
#include <glibmm/ustring.h>
#include <libxml++/nodes/element.h>

#include "IndexInterface.h"
#include "ModuleProperties.h"
#include "QueryProperties.h"

class PinotSettings
{
	public:
		~PinotSettings();

		typedef enum { LOAD_ALL = 0, LOAD_GLOBAL, LOAD_LOCAL } LoadWhat;

		typedef enum { SAVE_PREFS = 0, SAVE_CONFIG } SaveWhat;

		class IndexProperties
		{
			public:
				IndexProperties();
				IndexProperties(const Glib::ustring &name,
		                        const std::string &location,
					unsigned int id, bool isInternal);
				IndexProperties(const IndexProperties &other);
				virtual ~IndexProperties();

				IndexProperties& operator=(const IndexProperties &other);
				bool operator<(const IndexProperties &other) const;
				bool operator==(const IndexProperties &other) const;

				Glib::ustring m_name;
				std::string m_location;
				unsigned int m_id;
				bool m_internal;

		};

		static PinotSettings &getInstance(void);

		static bool setClientMode(bool enable);

		static bool getClientMode(void);

		static std::string getHomeDirectory(void);

		static std::string getConfigurationDirectory(void);

		static std::string getFileName(bool prefsOrUI);

		static std::string getCurrentUserName(void);

		static void checkHistoryDatabase(void);

		static std::string getHistoryDatabaseName(bool needToQueryDaemonHistory = false);

		bool isFirstRun(void) const;

		void clear(void);

		bool load(LoadWhat what);

		bool save(SaveWhat what);

		/// Returns the indexes set.
		const std::set<IndexProperties> &getIndexes(void) const;

		/// Adds a new index.
		bool addIndex(const Glib::ustring &name, const std::string &location,
			bool isInternal = false);

		/// Removes an index.
		bool removeIndex(const IndexProperties &indexProps);

		/// Clears the indexes map.
		void clearIndexes(void);

		/// Returns properties of the given index.
		IndexProperties getIndexPropertiesByName(const std::string &name) const;

		/// Returns properties of the given index.
		IndexProperties getIndexPropertiesByLocation(const std::string &location) const;

		/// Returns the name(s) for the given ID.
		void getIndexNames(unsigned int id, std::set<std::string> &names);

		/// Returns an IndexInterface for the given index location.
		IndexInterface *getIndex(const std::string &location);

		/// Returns the search engines set.
		bool getSearchEngines(std::set<ModuleProperties> &engines, const std::string &channelName = "") const;

		/// Returns an ID that identifies the given engine name.
		unsigned int getEngineId(const std::string &name);

		/// Returns the name(s) for the given ID.
		void getEngineNames(unsigned int id, std::set<std::string> &names);

		/// Returns the search engines channels.
		std::map<std::string, bool> &getSearchEnginesChannels(void);

		/// Returns the queries map, keyed by name.
		const std::map<std::string, QueryProperties> &getQueries(void) const;

		/// Adds a new query.
		bool addQuery(const QueryProperties &properties);

		/// Removes a query.
		bool removeQuery(const std::string &name);

		/// Clears the queries map.
		void clearQueries(void);

		/// Gets default patterns.
		bool getDefaultPatterns(std::set<Glib::ustring> &defaultPatterns);

		/// Determines if a file matches the blacklist.
		bool isBlackListed(const std::string &fileName);

		class IndexableLocation 
		{
			public:
				IndexableLocation();
				IndexableLocation(const IndexableLocation &other);
				~IndexableLocation();

				IndexableLocation &operator=(const IndexableLocation &other);
				bool operator<(const IndexableLocation &other) const;
				bool operator==(const IndexableLocation &other) const;

				bool m_monitor;
				Glib::ustring m_name;
				bool m_isSource;

		};

		class CacheProvider
		{
			public:
				CacheProvider();
				CacheProvider(const CacheProvider &other);
				~CacheProvider();

				CacheProvider &operator=(const CacheProvider &other);
				bool operator<(const CacheProvider &other) const;
				bool operator==(const CacheProvider &other) const;

				Glib::ustring m_name;
				Glib::ustring m_location;
				std::set<Glib::ustring> m_protocols;
		};

		Glib::ustring m_version;
		bool m_warnAboutVersion;
		Glib::ustring m_defaultBackend;
		Glib::ustring m_docsIndexLocation;
		Glib::ustring m_daemonIndexLocation;
		double m_minimumDiskSpace;
		int m_xPos;
		int m_yPos;
		int m_width;
		int m_height;
		int m_panePos;
		bool m_showEngines;
		bool m_expandQueries;
		bool m_ignoreRobotsDirectives;
		bool m_suggestQueryTerms;
		unsigned short m_newResultsColourRed;
		unsigned short m_newResultsColourGreen;
		unsigned short m_newResultsColourBlue;
		Glib::ustring m_proxyAddress;
		unsigned int m_proxyPort;
		Glib::ustring  m_proxyType;
		bool m_proxyEnabled;
		std::set<std::string> m_labels;
		std::set<IndexableLocation> m_indexableLocations;
		std::set<Glib::ustring> m_filePatternsList;
		bool m_isBlackList;
		std::map<std::string, std::string> m_editablePluginValues;
		std::vector<CacheProvider> m_cacheProviders;
		std::set<Glib::ustring> m_cacheProtocols;

	protected:
		static PinotSettings m_instance;
		static bool m_clientMode;
		bool m_firstRun;
		std::set<IndexProperties> m_indexes;
		unsigned int m_indexCount;
		std::set<ModuleProperties> m_engines;
		std::map<unsigned int, std::string> m_engineIds;
		std::map<std::string, bool> m_engineChannels;
		std::map<std::string, QueryProperties> m_queries;

		PinotSettings();
		bool loadSearchEngines(const std::string &directoryName);
		bool loadConfiguration(const std::string &fileName, bool isGlobal);
		bool loadUi(const xmlpp::Element *pElem);
		bool loadIndexes(const xmlpp::Element *pElem);
		bool loadEngineChannels(const xmlpp::Element *pElem);
		bool loadQueries(const xmlpp::Element *pElem);
		bool loadLabels(const xmlpp::Element *pElem);
		bool loadColour(const xmlpp::Element *pElem);
		bool loadProxy(const xmlpp::Element *pElem);
		bool loadIndexableLocations(const xmlpp::Element *pElem);
		bool loadFilePatterns(const xmlpp::Element *pElem);
		bool loadPluginParameters(const xmlpp::Element *pElem);
		bool loadCacheProviders(const xmlpp::Element *pElem);

	private:
		PinotSettings(const PinotSettings &other);
		PinotSettings &operator=(const PinotSettings &other);

};

#endif // _PINOTSETTINGS_HH
