/*
 *  Copyright 2005-2014 Fabrice Colin
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

#ifndef _WORKERTHREAD_HH
#define _WORKERTHREAD_HH

#include <time.h>
#include <string>
#include <set>
#include <map>
#include <pthread.h>
#include <sigc++/sigc++.h>
#include <glibmm/dispatcher.h>
#include <glibmm/thread.h>
#include <glibmm/ustring.h>

#include "Document.h"
#include "MonitorInterface.h"
#include "MonitorHandler.h"

class WorkerThread
{
	public:
		WorkerThread();
		virtual ~WorkerThread();

		typedef enum { UNKNOWN_ERROR = 10000, INDEX_ERROR, INDEXING_FAILED, UPDATE_FAILED, UNINDEXING_FAILED, \
			QUERY_FAILED, HISTORY_FAILED, DOWNLOAD_FAILED, MONITORING_FAILED, OPENDIR_FAILED, \
			UNKNOWN_INDEX, UNKNOWN_ENGINE, UNSUPPORTED_TYPE, UNSUPPORTED_PROTOCOL, \
			ROBOTS_FORBIDDEN, NO_MONITORING } ThreadError;

		static std::string errorToString(int errorNum);

		static Glib::Dispatcher &getDispatcher(void);

		static void immediateFlush(bool doFlush);

		time_t getStartTime(void) const;

		void setId(unsigned int id);

		unsigned int getId(void) const;

		void inBackground(void);

		bool isBackground(void) const;

		bool operator<(const WorkerThread &other) const;

		Glib::Thread *start(void);

		virtual std::string getType(void) const = 0;

		virtual void stop(void);

		bool isStopped(void) const;

		bool isDone(void) const;

		int getErrorNum(void) const;

		std::string getStatus(void) const;

	protected:
		/// Use a Dispatcher for thread safety
		static Glib::Dispatcher m_dispatcher;
		static pthread_mutex_t m_dispatcherMutex;
		static bool m_immediateFlush;
		time_t m_startTime;
		unsigned int m_id;
		bool m_background;
		bool m_stopped;
		bool m_done;
		int m_errorNum;
		std::string m_errorParam;

		void threadHandler(void);

		virtual void doWork(void) = 0;

		void emitSignal(void);

	private:
		WorkerThread(const WorkerThread &other);
		WorkerThread &operator=(const WorkerThread &other);

};

class ThreadsManager : virtual public sigc::trackable
{
	public:
		ThreadsManager(const std::string &defaultIndexLocation,
			unsigned int maxThreadsTime = 300);
		virtual ~ThreadsManager();

		static unsigned int get_next_id(void);

		bool start_thread(WorkerThread *pWorkerThread, bool inBackground = false);

		unsigned int get_threads_count(void);

		void stop_threads(void);

		virtual void connect(void);

		virtual void disconnect(void);

		void on_thread_signal();

		bool read_lock_lists(void);

		bool write_lock_lists(void);

		void unlock_lists(void);

		bool mustQuit(bool quit = false);

	protected:
		static unsigned int m_nextThreadId;
		sigc::connection m_threadsEndConnection;
		pthread_rwlock_t m_threadsLock;
		pthread_rwlock_t m_listsLock;
		std::map<unsigned int, WorkerThread *> m_threads;
		bool m_mustQuit;
		std::string m_defaultIndexLocation;
		unsigned int m_maxIndexThreads;
		unsigned int m_backgroundThreadsCount;
		unsigned int m_foregroundThreadsMaxTime;
		long m_numCPUs;
		sigc::signal1<void, WorkerThread *> m_onThreadEndSignal;
		std::set<std::string> m_beingIndexed;

		bool read_lock_threads(void);

		bool write_lock_threads(void);

		void unlock_threads(void);

		WorkerThread *get_thread(void);

	private:
		ThreadsManager(const ThreadsManager &other);
		ThreadsManager &operator=(const ThreadsManager &other);

};

class MonitorThread : public WorkerThread
{
	public:
		MonitorThread(MonitorInterface *pMonitor, MonitorHandler *pHandler);
		virtual ~MonitorThread();

		virtual std::string getType(void) const;

		virtual void stop(void);

	protected:
		int m_ctrlReadPipe;
		int m_ctrlWritePipe;
		MonitorInterface *m_pMonitor;
		MonitorHandler *m_pHandler;

		virtual bool isFileBlacklisted(const std::string &location);
		virtual void fileModified(const std::string &location);
		void processEvents(void);
		virtual void doWork(void);

	private:
		MonitorThread(const MonitorThread &other);
		MonitorThread &operator=(const MonitorThread &other);

};

#endif // _WORKERTHREAD_HH
