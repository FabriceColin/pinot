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

#ifndef _WEB_ENGINE_H
#define _WEB_ENGINE_H

#include <string>
#include <set>
#include <map>

#include "Document.h"
#include "Visibility.h"
#include "DownloaderInterface.h"
#include "QueryProperties.h"
#include "SearchEngineInterface.h"

/// Base class for all Web search engines.
class PINOT_EXPORT WebEngine : public SearchEngineInterface
{
	public:
		WebEngine();
		virtual ~WebEngine();

		/// Returns the downloader used if any.
		DownloaderInterface *getDownloader(void);

		/// Specifies values for editable parameters.
		void setEditableValues(const std::map<std::string, std::string> &editableValues);

	protected:
		DownloaderInterface *m_pDownloader;
		std::map<std::string, std::string> m_editableValues;
		std::set<std::string> m_queryTerms;

		Document *downloadPage(const DocumentInfo &docInfo);

		void setHostNameFilter(const string &filter);

		void setFileNameFilter(const string &filter);

		void setQuery(const QueryProperties &queryProps);

		virtual bool processResult(const string &queryUrl, DocumentInfo &result);

	private:
		WebEngine(const WebEngine &other);
		WebEngine &operator=(const WebEngine &other);

};

#endif // _WEB_ENGINE_H
