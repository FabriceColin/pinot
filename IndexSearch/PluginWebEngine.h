/*
 *  Copyright 2005-2008 Fabrice Colin
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

#ifndef _PLUGIN_WEB_ENGINE_H
#define _PLUGIN_WEB_ENGINE_H

#include <string>

#include "PluginParsers.h"
#include "SearchPluginProperties.h"
#include "WebEngine.h"

/// A plugin-based search engine.
class PluginWebEngine : public WebEngine
{
	public:
		PluginWebEngine(const std::string &fileName);
		virtual ~PluginWebEngine();

		/// Utility method that returns a search plugin's name and channel.
		static bool getDetails(const std::string &fileName, SearchPluginProperties &properties);

		/// Runs a query; true if success.
		virtual bool runQuery(QueryProperties& queryProps,
			unsigned int startDoc = 0);

	protected:
		SearchPluginProperties m_properties;
		ResponseParserInterface *m_pResponseParser;

		void load(const std::string &fileName);

		bool getPage(const std::string &formattedQuery, unsigned int maxResultsCount);

		static PluginParserInterface *getPluginParser(const std::string &fileName,
			std::string &pluginType);

	private:
		PluginWebEngine(const PluginWebEngine &other);
		PluginWebEngine &operator=(const PluginWebEngine &other);

};

#endif // _PLUGIN_WEB_ENGINE_H
