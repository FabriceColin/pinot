/*
 *  Copyright 2008 Fabrice Colin
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

#ifndef _XESAM_ENGINE_H
#define _XESAM_ENGINE_H

#include <string>

#include "QueryProperties.h"
#include "SearchEngineInterface.h"

/// A search engine that acts as a client to the Xesam D-Bus interface.
class XesamEngine : public SearchEngineInterface
{
	public:
		XesamEngine(const std::string &dbusObject);
		virtual ~XesamEngine();

		/// Runs a query; true if success.
		virtual bool runQuery(QueryProperties& queryProps,
			unsigned int startDoc = 0);

	protected:
		std::string m_dbusObject;

	private:
		XesamEngine(const XesamEngine &other);
		XesamEngine &operator=(const XesamEngine &other);

};

#endif // _XESAM_ENGINE_H
