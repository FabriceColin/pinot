/*
 *  Copyright 2005,2006 Fabrice Colin
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

#ifndef _GOOGLE_API_ENGINE_H
#define _GOOGLE_API_ENGINE_H

#include <string>

#include "Document.h"
#include "WebEngine.h"

using namespace std;

/// Engine that makes use of Google's SOAP API.
class GoogleAPIEngine : public WebEngine
{
	public:
		GoogleAPIEngine(const string &key);
		virtual ~GoogleAPIEngine();

		/// Runs a query; true if success.
		virtual bool runQuery(QueryProperties& queryProps,
			unsigned int startDoc = 0);

		/// Retrieves the specified URL from the cache; NULL if error. Caller deletes.
		Document *retrieveCachedUrl(const string &url);

		/// Checks spelling.
		string checkSpelling(const string &text);

	protected:
		string m_key;

	private:
		GoogleAPIEngine(const GoogleAPIEngine &other);
		GoogleAPIEngine &operator=(const GoogleAPIEngine &other);

};

#endif // _GOOGLE_API_ENGINE_H
