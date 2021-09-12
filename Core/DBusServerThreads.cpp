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

#include <iostream>
#include <sstream>
#include <tuple>
#include <vector>
#include <glibmm/ustring.h>

#include "config.h"
#include "NLS.h"
#include "DBusIndex.h"
#include "DBusServerThreads.h"

using namespace Glib;
using namespace std;

DBusEngineQueryThread::DBusEngineQueryThread(const RefPtr<Gio::DBus::MethodInvocation> &refInvocation,
	const string &engineName, const string &engineDisplayableName,
	const string &engineOption, const QueryProperties &queryProps,
	unsigned int startDoc, bool simpleQuery, bool pinotCall) :
	EngineQueryThread(engineName, engineDisplayableName,
		engineOption, queryProps, startDoc),
	m_refInvocation(refInvocation),
	m_simpleQuery(simpleQuery),
	m_pinotCall(pinotCall)
{
	stringstream queryNameStr;

	// Give the query a unique name
	queryNameStr << "DBUS " << m_id;

	m_queryProps.setName(queryNameStr.str());
}

DBusEngineQueryThread::~DBusEngineQueryThread()
{
}

string DBusEngineQueryThread::getType(void) const
{
	return "DBusEngineQueryThread";
}

RefPtr<Gio::DBus::MethodInvocation> DBusEngineQueryThread::getInvocation(void) const
{
	return m_refInvocation;
}

bool DBusEngineQueryThread::isSimpleQuery(void) const
{
	return m_simpleQuery;
}

bool DBusEngineQueryThread::isPinotCall(void) const
{
	return m_pinotCall;
}

DBusReloadThread::DBusReloadThread()
{
}

DBusReloadThread::~DBusReloadThread()
{
}

string DBusReloadThread::getType(void) const
{
	return "DBusReloadThread";
}

void DBusReloadThread::doWork(void)
{
	// Nothing to do here, we just want to inform the daemon
}

