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

#ifndef _INOTIFY_MONITOR_H
#define _INOTIFY_MONITOR_H

#include <stdint.h>
#include <pthread.h>
#include <string>
#include <map>
#include <queue>

#include "MonitorInterface.h"

/// Linux inotify monitor.
class INotifyMonitor : public MonitorInterface
{
	public:
		INotifyMonitor();
		virtual ~INotifyMonitor();

		/// Returns the maximum number of files that can be monitored.
		virtual unsigned int getLimit(void) const;

		/// Adds a watch for the specified location.
		virtual bool addLocation(const std::string &location, bool isDirectory);

		/// Removes the watch for the specified location.
		virtual bool removeLocation(const std::string &location);

		/// Removes watches for the specified location and all underneath.
		virtual bool removeLocations(const std::string &location);

		/// Retrieves pending events.
		virtual bool retrievePendingEvents(std::queue<MonitorEvent> &events);

	protected:
		pthread_mutex_t m_mutex;
		std::queue<MonitorEvent> m_internalEvents;
		std::map<std::string, int> m_locations;
		std::map<uint32_t, MonitorEvent> m_movedFrom;
		unsigned int m_maxUserWatches;
		unsigned int m_watchesCount;

		bool removeWatch(const std::string &location);

	private:
		INotifyMonitor(const INotifyMonitor &other);
		INotifyMonitor &operator=(const INotifyMonitor &other);

};

#endif // _INOTIFY_MONITOR_H
