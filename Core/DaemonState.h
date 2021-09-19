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

#ifndef _DAEMONSTATE_HH
#define _DAEMONSTATE_HH

#include <sys/select.h>
#include <string>
#include <queue>
#include <set>
#include <tuple>
#include <vector>
#include <sigc++/sigc++.h>
#ifdef HAVE_DBUS
#include <giomm/dbusconnection.h>
#include <giomm/dbusproxy.h>
#endif

#include "CrawlHistory.h"
#ifdef HAVE_DBUS
#include "DBusIndex.h"
#endif
#include "MonitorInterface.h"
#include "MonitorHandler.h"
#include "PinotSettings.h"
#include "PinotDBus_stub.h"
#include "SearchProvider_stub.h"
#include "WorkerThreads.h"

class DaemonState : public QueueManager
{
	public:
		DaemonState();
		virtual ~DaemonState();

		typedef enum { LOW_DISK_SPACE = 0, ON_BATTERY, CRAWLING, STOPPED, DISCONNECTED, SHOULD_FLUSH } StatusFlag;

		virtual void disconnect(void);

		void register_session(void);

		void start(bool isReindex);

		bool start_crawling(void);

		void stop_crawling(void);

		void on_thread_end(WorkerThread *pThread);

		void on_message_filefound(DocumentInfo docInfo, bool isDirectory);

		sigc::signal1<void, int>& getQuitSignal(void);

		void set_flag(StatusFlag flag);

		bool is_flag_set(StatusFlag flag);

		void reset_flag(StatusFlag flag);

		void flush_and_reclaim(void);

	protected:
#ifdef HAVE_DBUS
		class DBusIntrospectHandler : public org::freedesktop::DBus::IntrospectableStub
		{
			public:
				DBusIntrospectHandler();
				virtual ~DBusIntrospectHandler();

			protected:
				virtual void Introspect(IntrospectableStub::MethodInvocation &invocation);

		};

		class DBusMessageHandler : public com::github::fabricecolin::PinotStub
		{
			public:
				DBusMessageHandler(DaemonState *pServer);
				virtual ~DBusMessageHandler();

				bool mustQuit(void) const;

			protected:
				DaemonState *m_pServer;
				time_t m_flushTime;
				bool m_mustQuit;

			    virtual void GetStatistics(PinotStub::MethodInvocation &invocation);

			    virtual void Reload(PinotStub::MethodInvocation &invocation);

			    virtual void Stop(PinotStub::MethodInvocation &invocation);

			    virtual void GetDocumentInfo(guint32 docId,
					PinotStub::MethodInvocation &invocation);

				virtual void GetDocumentTermsCount(guint32 docId,
					PinotStub::MethodInvocation &invocation);

				virtual void GetDocumentTerms(guint32 docId,
					PinotStub::MethodInvocation &invocation);

			    virtual void GetLabels(PinotStub::MethodInvocation &invocation);

			    virtual void AddLabel(const Glib::ustring &label,
					PinotStub::MethodInvocation &invocation);

			    virtual void DeleteLabel(const Glib::ustring &label,
					PinotStub::MethodInvocation &invocation);

				virtual void HasLabel(guint32 docId,
					const Glib::ustring &label,
					PinotStub::MethodInvocation &invocation);

			    virtual void GetDocumentLabels(guint32 docId,
					PinotStub::MethodInvocation &invocation);

			    virtual void SetDocumentLabels(guint32 docId,
					const std::vector<Glib::ustring> &labels,
					bool resetLabels,
					PinotStub::MethodInvocation &invocation);

			    virtual void SetDocumentsLabels(const std::vector<Glib::ustring> &docIds,
					const std::vector<Glib::ustring> &labels,
					bool resetLabels,
					PinotStub::MethodInvocation &invocation);

			    virtual void HasDocument(const Glib::ustring &url,
					PinotStub::MethodInvocation &invocation);

				virtual void GetCloseTerms(const Glib::ustring &term,
					PinotStub::MethodInvocation &invocation);

				virtual void GetDocumentsCount(const Glib::ustring &label,
					PinotStub::MethodInvocation &invocation);

				virtual void ListDocuments(const Glib::ustring &term,
					guint32 termType, guint32 maxCount, guint32 startOffset,
					PinotStub::MethodInvocation &invocation);

			    virtual void UpdateDocument(guint32 docId,
					PinotStub::MethodInvocation &invocation);

			    virtual void SetDocumentInfo(guint32 docId,
					const std::vector<std::tuple<Glib::ustring,Glib::ustring>> &fields,
					PinotStub::MethodInvocation &invocation);

			    virtual void Query(const Glib::ustring &engineType,
					const Glib::ustring &engineName,
					const Glib::ustring &searchText,
					guint32 startDoc,
					guint32 maxHits,
					PinotStub::MethodInvocation &invocation);

			    virtual void SimpleQuery(const Glib::ustring &searchText,
					guint32 maxHits,
					PinotStub::MethodInvocation &invocation);

				virtual bool DaemonVersion_setHandler(const Glib::ustring &value);

				virtual Glib::ustring DaemonVersion_get();

				virtual bool IndexFlushEpoch_setHandler(guint32 value);

				virtual guint32 IndexFlushEpoch_get();

		};
		class DBusSearchProvider : public org::gnome::Shell::SearchProvider2Stub
		{
			public:
				DBusSearchProvider(DaemonState *pServer);
				virtual ~DBusSearchProvider();

			protected:
				DaemonState *m_pServer;

				virtual void GetInitialResultSet(const std::vector<Glib::ustring> &terms,
					MethodInvocation &invocation);

				virtual void GetSubsearchResultSet(const std::vector<Glib::ustring> &previous_results,
					const std::vector<Glib::ustring> &terms,
					MethodInvocation &invocation);

				virtual void GetResultMetas(const std::vector<Glib::ustring> &identifiers,
					MethodInvocation &invocation);

				virtual void ActivateResult(const Glib::ustring &identifier,
					const std::vector<Glib::ustring> &terms,
					guint32 timestamp,
					MethodInvocation &invocation);

				virtual void LaunchSearch(const std::vector<Glib::ustring> &terms,
					guint32 timestamp,
					MethodInvocation &invocation);

		};
		Glib::RefPtr<Gio::DBus::Connection> m_refSessionBus;
		DBusIntrospectHandler m_introspectionHandler;
		DBusMessageHandler m_messageHandler;
		DBusSearchProvider m_searchProvider;
		Glib::RefPtr<Gio::DBus::Proxy> m_powerProxy;
		guint m_connectionId;
#endif
		bool m_isReindex;
		bool m_tryReload;
		bool m_readyToReload;
		fd_set m_flagsSet;
		CrawlHistory m_crawlHistory;
		MonitorInterface *m_pDiskMonitor;
		MonitorHandler *m_pDiskHandler;
		sigc::connection m_timeoutConnection;
		sigc::signal1<void, int> m_signalQuit;
		unsigned int m_crawlers;
		std::queue<PinotSettings::IndexableLocation> m_crawlQueue;

#ifdef HAVE_DBUS
		void handle_power_properties_changed(const Gio::DBus::Proxy::MapChangedProperties &changed_properties,
			const std::vector<Glib::ustring> &invalidated_properties);
#endif

		bool on_activity_timeout(void);

		void check_battery_state(void);

		bool crawl_location(const PinotSettings::IndexableLocation &location);

};

#endif // _DAEMONSTATE_HH
