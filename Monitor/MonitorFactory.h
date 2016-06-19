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

#ifndef _MONITOR_FACTORY_H
#define _MONITOR_FACTORY_H

#include "MonitorInterface.h"

/// Factory for monitors.
class MonitorFactory
{
	public:
		virtual ~MonitorFactory();

		/// Returns a Monitor.
		static MonitorInterface *getMonitor(void);

	protected:
		MonitorFactory();

	private:
		MonitorFactory(const MonitorFactory &other);
		MonitorFactory& operator=(const MonitorFactory& other);

};

#endif // _MONITOR_FACTORY_H
