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

#ifndef _MODULE_FACTORY_H
#define _MODULE_FACTORY_H

#include <string>
#include <map>

#include "FieldMapperInterface.h"
#include "IndexInterface.h"
#include "SearchPluginProperties.h"
#include "SearchEngineInterface.h"

/// Loadable module.
class LoadableModule
{
	public:
		LoadableModule(ModuleProperties *pProperties,
			const std::string &location, void *pHandle);
		LoadableModule(const LoadableModule &other);
		virtual ~LoadableModule();

		LoadableModule &operator=(const LoadableModule &other);

		ModuleProperties *m_pProperties;
		std::string m_location;
		bool m_canSearch;
		bool m_canIndex;
		void *m_pHandle;

};

/// Factory for search engines.
class ModuleFactory
{
	public:
		virtual ~ModuleFactory();

		/// Loads the libraries found in the given directory.
		static unsigned int loadModules(const std::string &directory);

		/// Makes sure the index exists in the desired mode.
		static bool openOrCreateIndex(const std::string &type, const std::string &option,
			bool &obsoleteFormat, bool readOnly = true, bool overwrite = false);

		/// Merges two physical indexes in a logical one.
		static bool mergeIndexes(const std::string &type, const std::string &option0,
			const std::string &option1, const std::string &option2);

		/// Returns an index of the specified type; NULL if unavailable.
		static IndexInterface *getIndex(const std::string &type, const std::string &option);

		/// Returns a SearchEngine of the specified type; NULL if unavailable.
		static SearchEngineInterface *getSearchEngine(const std::string &type, const std::string &option);

		/// Returns the name of the given engine.
		static string getSearchEngineName(const std::string &type, const std::string &option);

		/// Returns all supported engines.
		static void getSupportedEngines(std::map<ModuleProperties, bool> &engines);

		/// Indicates whether a search engine or index is supported or not.
		static bool isSupported(const std::string &type, bool asIndex = false);

		/// Sets a field mapper.
		static void setFieldMapper(FieldMapperInterface *pMapper);

		/// Unloads all libraries.
		static void unloadModules(void);

	protected:
		static std::map<std::string, LoadableModule> m_types;

		ModuleFactory();

		static IndexInterface *getLibraryIndex(const std::string &type, const std::string &option);

		static SearchEngineInterface *getLibrarySearchEngine(const std::string &type, const std::string &option);

	private:
		ModuleFactory(const ModuleFactory &other);
		ModuleFactory &operator=(const ModuleFactory &other);

};

#endif // _MODULE_FACTORY_H
