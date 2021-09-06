/*
 *  Copyright 2021 Fabrice Colin
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

#ifndef _DBUSSERVERTHREADS_HH
#define _DBUSSERVERTHREADS_HH

#include <glibmm/refptr.h>
#include <giomm/dbusmethodinvocation.h>

#include "WorkerThreads.h"

class DBusEngineQueryThread : public EngineQueryThread
{
	public:
		DBusEngineQueryThread(const Glib::RefPtr<Gio::DBus::MethodInvocation> &refInvocation,
			const std::string &engineName, const std::string &engineDisplayableName,
			const std::string &engineOption, const QueryProperties &queryProps,
			unsigned int startDoc, bool simpleQuery, bool pinotCall = true);
		virtual ~DBusEngineQueryThread();

		virtual std::string getType(void) const;

		Glib::RefPtr<Gio::DBus::MethodInvocation> getInvocation(void) const;

		bool isSimpleQuery(void) const;

		bool isPinotCall(void) const;

	protected:
		Glib::RefPtr<Gio::DBus::MethodInvocation> m_refInvocation;
		bool m_simpleQuery;
		bool m_pinotCall;

	private:
		DBusEngineQueryThread(const DBusEngineQueryThread &other);
		DBusEngineQueryThread &operator=(const DBusEngineQueryThread &other);

};

#endif // _DBUSSERVERTHREADS_HH
