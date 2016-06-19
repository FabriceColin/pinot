/*
 *  Copyright 2005-2012 Fabrice Colin
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

#ifndef _DAEMONSTATE_HH
#define _DAEMONSTATE_HH

#include <sys/select.h>
#include <string>
#include <queue>
#include <set>
#include <sigc++/sigc++.h>

#include "CrawlHistory.h"
#ifdef HAVE_DBUS
#include "DBusIndex.h"
#endif
#include "MonitorInterface.h"
#include "MonitorHandler.h"
#include "PinotSettings.h"
#include "WorkerThreads.h"

#ifdef HAVE_DBUS
class DBusServletInfo
{
	public:
		DBusServletInfo(DBusConnection *pConnection, DBusMessage *pRequest);
		~DBusServletInfo();

		bool newReply(void);

		bool newErrorReply(const std::string &name, const std::string &message);

		bool newReplyWithArray(void);

		bool newQueryReply(const vector<DocumentInfo> &resultsList,
			unsigned int resultsEstimate);

		bool reply(void);

		DBusConnection *m_pConnection;
		DBusMessage *m_pRequest;
		DBusMessage *m_pReply;
		GPtrArray *m_pArray;
		bool m_simpleQuery;
		WorkerThread *m_pThread;

	protected:
		bool m_replied;

};
#endif

class DaemonState : public QueueManager
{
	public:
		DaemonState();
		virtual ~DaemonState();

		typedef enum { LOW_DISK_SPACE = 0, ON_BATTERY, CRAWLING, STOPPED, DISCONNECTED } StatusFlag;

		void start(bool isReindex);

		void reload(void);

		bool start_crawling(void);

		void stop_crawling(void);

		void on_thread_end(WorkerThread *pThread);

		void on_message_filefound(DocumentInfo docInfo, bool isDirectory);

		sigc::signal1<void, int>& getQuitSignal(void);

		void set_flag(StatusFlag flag);

		bool is_flag_set(StatusFlag flag);

		void reset_flag(StatusFlag flag);

	protected:
		bool m_isReindex;
		bool m_reload;
		bool m_flush;
		fd_set m_flagsSet;
		CrawlHistory m_crawlHistory;
		MonitorInterface *m_pDiskMonitor;
		MonitorHandler *m_pDiskHandler;
		sigc::connection m_timeoutConnection;
		sigc::signal1<void, int> m_signalQuit;
		unsigned int m_crawlers;
		std::queue<PinotSettings::IndexableLocation> m_crawlQueue;
#ifdef HAVE_DBUS
		std::set<DBusServletInfo *> m_servletsInfo;
#endif

		bool on_activity_timeout(void);

		void check_battery_state(void);

		bool crawl_location(const PinotSettings::IndexableLocation &location);

		void flush_and_reclaim(void);

};

#endif // _DAEMONSTATE_HH
