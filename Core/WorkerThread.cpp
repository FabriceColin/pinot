/*
 *  Copyright 2005-2024 Fabrice Colin
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

#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#ifdef __OpenBSD__
#include <sys/param.h>
#include <sys/sysctl.h>
#endif
#include <exception>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <glibmm/convert.h>
#include <glibmm/exception.h>
#include <glibmm/miscutils.h>
#include <glibmm/stringutils.h>

#include "config.h"
#include "NLS.h"
#include "Memory.h"
#include "Url.h"
#include "WorkerThread.h"

using namespace std;
using namespace Glib;

// A function object to stop threads with for_each()
struct StopThreadFunc
{
public:
	void operator()(map<unsigned int, WorkerThread *>::value_type &p)
	{
		p.second->stop();
#ifdef DEBUG
		clog << "StopThreadFunc: stopped thread " << p.second->getId() << endl;
#endif
		Thread::yield();
	}
};

Dispatcher WorkerThread::m_dispatcher;
pthread_mutex_t WorkerThread::m_dispatcherMutex = PTHREAD_MUTEX_INITIALIZER;
bool WorkerThread::m_immediateFlush = true;

string WorkerThread::errorToString(int errorNum)
{
	if (errorNum == 0)
	{
		return "";
	}

	if (errorNum < INDEX_ERROR)
	{
		ustring errorText(Glib::strerror(errorNum));

		return errorText.c_str();
	}

	// Internal error codes
	switch (errorNum)
	{
		case INDEX_ERROR:
			return _("Index error");
		case INDEXING_FAILED:
			return _("Couldn't index document");
		case UPDATE_FAILED:
			return _("Couldn't update document");
		case UNINDEXING_FAILED:
			return _("Couldn't unindex document(s)");
		case QUERY_FAILED:
			return _("Couldn't run query on search engine");
		case HISTORY_FAILED:
			return _("Couldn't get history for search engine");
		case DOWNLOAD_FAILED:
			return _("Couldn't retrieve document");
		case MONITORING_FAILED:
			return _("File monitor error");
		case OPENDIR_FAILED:
			return _("Couldn't open directory");
		case UNKNOWN_INDEX:
			return _("Index doesn't exist");
		case UNKNOWN_ENGINE:
			return  _("Couldn't create search engine");
		case UNSUPPORTED_TYPE:
			return _("Cannot index document type");
		case UNSUPPORTED_PROTOCOL:
			return _("No downloader for this protocol");
		case ROBOTS_FORBIDDEN:
			return _("Robots META tag forbids indexing");
		case NO_MONITORING:
			return _("No monitoring handler");
		default:
			break;
	}

	return _("Unknown error");
}

Dispatcher &WorkerThread::getDispatcher(void)
{
	return m_dispatcher;
}

void WorkerThread::immediateFlush(bool doFlush)
{
	m_immediateFlush = doFlush;
}

WorkerThread::WorkerThread() :
	m_startTime(time(NULL)),
	m_id(ThreadsManager::get_next_id()),
	m_background(false),
	m_stopped(false),
	m_done(false),
	m_errorNum(0)
{
}

WorkerThread::~WorkerThread()
{
}

time_t WorkerThread::getStartTime(void) const
{
	return m_startTime;
}

void WorkerThread::setId(unsigned int id)
{
	m_id = id;
}

unsigned int WorkerThread::getId(void) const
{
	return m_id;
}

void WorkerThread::inBackground(void)
{
	m_background = true;
}

bool WorkerThread::isBackground(void) const
{
	return m_background;
}

bool WorkerThread::operator<(const WorkerThread &other) const
{
	return m_id < other.m_id;
}

Glib::Thread *WorkerThread::start(void)
{
#ifdef DEBUG
	clog << "WorkerThread::start: " << getType() << " " << m_id << endl;
#endif
	// Create non-joinable threads
	return Thread::create(sigc::mem_fun(*this, &WorkerThread::threadHandler), false);
}

void WorkerThread::stop(void)
{
	m_stopped = m_done = true;
}

bool WorkerThread::isStopped(void) const
{
	return m_stopped;
}

bool WorkerThread::isDone(void) const
{
	return m_done;
}

int WorkerThread::getErrorNum(void) const
{
	return m_errorNum;
}

string WorkerThread::getStatus(void) const
{
	string status(errorToString(m_errorNum));

	if ((status.empty() == false) &&
		(m_errorParam.empty() == false))
	{
		status += " (";
		status += m_errorParam;
		status += ")";
	}

	return status;
}

void WorkerThread::threadHandler(void)
{
#ifdef DEBUG
	clog << "WorkerThread::threadHandler: thread " << m_id << endl;
#endif
	try
	{
		doWork();
	}
	catch (Glib::Exception &ex)
	{
		clog << "Glib exception in thread " << m_id << ", type " << getType()
			<< ":" << ex.what() << endl;
		m_errorNum = UNKNOWN_ERROR;
	}
	catch (std::exception &ex)
	{
		clog << "STL exception in thread " << m_id << ", type " << getType()
			<< ":" << ex.what() << endl;
		m_errorNum = UNKNOWN_ERROR;
	}
	catch (...)
	{
		clog << "Unknown exception in thread " << m_id << ", type " << getType() << endl;
		m_errorNum = UNKNOWN_ERROR;
	}

	emitSignal();
}

void WorkerThread::emitSignal(void)
{
	m_done = true;
	if (pthread_mutex_lock(&m_dispatcherMutex) == 0)
	{
#ifdef DEBUG
		clog << "WorkerThread::emitSignal: signaling end of thread " << m_id << endl;
#endif
		m_dispatcher();

		pthread_mutex_unlock(&m_dispatcherMutex);
	}
}

unsigned int ThreadsManager::m_nextThreadId = 1;

ThreadsManager::ThreadsManager(const string &defaultIndexLocation,
	unsigned int maxThreadsTime) :
	m_mustQuit(false),
	m_defaultIndexLocation(defaultIndexLocation),
	m_maxIndexThreads(1),
	m_backgroundThreadsCount(0),
	m_foregroundThreadsMaxTime(maxThreadsTime),
	m_numCPUs(1)
{
	pthread_rwlock_init(&m_threadsLock, NULL);
	pthread_rwlock_init(&m_listsLock, NULL);

	// Override the number of indexing threads ?
	char *pEnvVar = getenv("PINOT_MAXIMUM_INDEX_THREADS");
	if ((pEnvVar != NULL) &&
		(strlen(pEnvVar) > 0))
	{
		int threadsNum = atoi(pEnvVar);

		if (threadsNum > 0)
		{
			m_maxIndexThreads = (unsigned int)threadsNum;
		}
	}

#ifdef __OpenBSD__
	int mib[2], ncpus;

	mib[0] = CTL_HW;
	mib[1] = HW_NCPU;
	size_t len = sizeof(ncpus);
	if (sysctl(mib, 2, &ncpus, &len, NULL, 0) > 0)
	{
		m_numCPUs = ncpus;
	}
#else
#ifdef HAVE_SYSCONF
	m_numCPUs = sysconf(_SC_NPROCESSORS_ONLN);
#endif
#endif
}

ThreadsManager::~ThreadsManager()
{
	stop_threads();
	// Destroy the read/write locks
	pthread_rwlock_destroy(&m_listsLock);
	pthread_rwlock_destroy(&m_threadsLock);
}

bool ThreadsManager::read_lock_threads(void)
{
	if (pthread_rwlock_rdlock(&m_threadsLock) == 0)
	{
		return true;
	}

	return false;
}

bool ThreadsManager::write_lock_threads(void)
{
	if (pthread_rwlock_wrlock(&m_threadsLock) == 0)
	{
		return true;
	}

	return false;
}

void ThreadsManager::unlock_threads(void)
{
	pthread_rwlock_unlock(&m_threadsLock);
}

bool ThreadsManager::read_lock_lists(void)
{
	if (pthread_rwlock_rdlock(&m_listsLock) == 0)
	{
		return true;
	}

	return false;
}

bool ThreadsManager::write_lock_lists(void)
{
	if (pthread_rwlock_wrlock(&m_listsLock) == 0)
	{
		return true;
	}

	return false;
}

void ThreadsManager::unlock_lists(void)
{
	pthread_rwlock_unlock(&m_listsLock);
}

WorkerThread *ThreadsManager::get_thread(void)
{
	time_t timeNow = time(NULL);
	WorkerThread *pWorkerThread = NULL;

	// Get the first thread that's finished
	if (write_lock_threads() == true)
	{
		for (map<unsigned int, WorkerThread *>::iterator threadIter = m_threads.begin();
			threadIter != m_threads.end(); ++threadIter)
		{
			unsigned int threadId = threadIter->first;

			if (threadIter->second->isDone() == false)
			{
#ifdef DEBUG
				clog << "ThreadsManager::get_thread: thread "
					<< threadId << " is not done" << endl;
#endif

				// Foreground threads ought not to run very long
				if ((threadIter->second->isBackground() == false) &&
					(threadIter->second->getStartTime() + m_foregroundThreadsMaxTime < timeNow))
				{
					// This thread has been running for too long !
					threadIter->second->stop();

					clog << "Stopped long-running thread " << threadId << endl;
				}
			}
			else
			{
				// This one will do...
				pWorkerThread = threadIter->second;
				// Remove it
				m_threads.erase(threadIter);
#ifdef DEBUG
				clog << "ThreadsManager::get_thread: thread " << threadId
					<< " is done, " << m_threads.size() << " left" << endl;
#endif
				break;
			}
		}

		unlock_threads();
	}

	if (pWorkerThread == NULL)
	{
		return NULL;
	}

	if (pWorkerThread->isBackground() == true)
	{
#ifdef DEBUG
		clog << "ThreadsManager::get_thread: thread " << pWorkerThread->getId()
			<< " was running in the background" << endl;
#endif
		--m_backgroundThreadsCount;
	}

	return pWorkerThread;
}

unsigned int ThreadsManager::get_next_id(void)
{
	unsigned int nextThreadId = ++m_nextThreadId;

	// Reclaim memory on a regular basis
	if (nextThreadId % 100 == 0)
	{
		int inUse = Memory::getUsage();
		Memory::reclaim();
	}

	return nextThreadId;
}

bool ThreadsManager::start_thread(WorkerThread *pWorkerThread, bool inBackground)
{
	bool createdThread = false;

	if (pWorkerThread == NULL)
	{
		return false;
	}

	if (inBackground == true)
	{
#ifdef DEBUG
		clog << "ThreadsManager::start_thread: thread " << pWorkerThread->getId()
			<< " will run in the background" << endl;
#endif
		pWorkerThread->inBackground();
		++m_backgroundThreadsCount;
	}
#ifdef DEBUG
	else clog << "ThreadsManager::start_thread: thread " << pWorkerThread->getId()
			<< " will run in the foreground" << endl;
#endif

	// Insert
	pair<map<unsigned int, WorkerThread *>::iterator, bool> threadPair;
	if (write_lock_threads() == true)
	{
		threadPair = m_threads.insert(pair<unsigned int, WorkerThread *>(pWorkerThread->getId(), pWorkerThread));
		if (threadPair.second == false)
		{
			delete pWorkerThread;
			pWorkerThread = NULL;
		}

		unlock_threads();
	}

	// Start the thread
	if (pWorkerThread != NULL)
	{
		Thread *pThread = pWorkerThread->start();
		if (pThread != NULL)
		{
			createdThread = true;
		}
		else
		{
			// Erase
			if (write_lock_threads() == true)
			{
				m_threads.erase(threadPair.first);

				unlock_threads();
			}
			delete pWorkerThread;
		}
	}

	return createdThread;
}

unsigned int ThreadsManager::get_threads_count(void)
{
	int count = 0;

	if (read_lock_threads() == true)
	{
		count = m_threads.size() - m_backgroundThreadsCount;

		unlock_threads();
	}
#ifdef DEBUG
	clog << "ThreadsManager::get_threads_count: " << count << "/"
		<< m_backgroundThreadsCount << " threads left" << endl;
#endif

	// A negative count would mean that a background thread
	// exited without signaling
	return (unsigned int)max(count , 0);
}

void ThreadsManager::stop_threads(void)
{
	if (m_threads.empty() == false)
	{
		if (write_lock_threads() == true)
		{
			// Stop threads
			for_each(m_threads.begin(), m_threads.end(), StopThreadFunc());

			unlock_threads();
		}
	}
}

void ThreadsManager::connect(void)
{
	// The previous manager may have been signalled by our threads
	WorkerThread *pThread = get_thread();
	while (pThread != NULL)
	{
		m_onThreadEndSignal(pThread);

		// Next
		pThread = get_thread();
	}
#ifdef DEBUG
	clog << "ThreadsManager::connect: connecting" << endl;
#endif

	// Connect the dispatcher
	m_threadsEndConnection = WorkerThread::getDispatcher().connect(
		sigc::mem_fun(*this, &ThreadsManager::on_thread_signal));
#ifdef DEBUG
	clog << "ThreadsManager::connect: connected" << endl;
#endif
}

void ThreadsManager::disconnect(void)
{
	m_threadsEndConnection.block();
	m_threadsEndConnection.disconnect();
#ifdef DEBUG
	clog << "ThreadsManager::disconnect: disconnected" << endl;
#endif
}

void ThreadsManager::on_thread_signal()
{
	WorkerThread *pThread = get_thread();
	if (pThread == NULL)
	{
#ifdef DEBUG
		clog << "ThreadsManager::on_thread_signal: foreign thread" << endl;
#endif
		return;
	}
	m_onThreadEndSignal(pThread);
}

bool ThreadsManager::mustQuit(bool quit)
{
	if (quit == true)
	{
		m_mustQuit = true;
		stop_threads();
	}

	return m_mustQuit;
}

MonitorThread::MonitorThread(MonitorInterface *pMonitor, MonitorHandler *pHandler) :
	WorkerThread(),
	m_ctrlReadPipe(-1),
	m_ctrlWritePipe(-1),
	m_pMonitor(pMonitor),
	m_pHandler(pHandler)
{
	int pipeFds[2];

#ifdef HAVE_PIPE
	if (pipe(pipeFds) == 0)
	{
		// This pipe will allow to stop select()
		m_ctrlReadPipe = pipeFds[0];
		m_ctrlWritePipe = pipeFds[1];
	}
#endif
}

MonitorThread::~MonitorThread()
{
	if (m_ctrlReadPipe >= 0)
	{
		close(m_ctrlReadPipe);
	}
	if (m_ctrlWritePipe >= 0)
	{
		close(m_ctrlWritePipe);
	}
}

string MonitorThread::getType(void) const
{
	return "MonitorThread";
}

void MonitorThread::stop(void)
{
	WorkerThread::stop();
	if (m_ctrlWritePipe >= 0)
	{
		if (write(m_ctrlWritePipe, "X", 1) == -1)
		{
			clog << "Couldn't signal thread " << m_id << " to stop" << endl;
		}
	}
}

bool MonitorThread::isFileBlacklisted(const string &location)
{
	return false;
}

void MonitorThread::fileModified(const string &location)
{
	// Pass this event directly to the handler 
	m_pHandler->fileModified(location);
}

void MonitorThread::processEvents(void)
{
	queue<MonitorEvent> events;

#ifdef DEBUG
	clog << "MonitorThread::processEvents: checking for events" << endl;
#endif
	if ((m_pMonitor == NULL) ||
		(m_pMonitor->retrievePendingEvents(events) == false))
	{
		return;
	}
#ifdef DEBUG
	clog << "MonitorThread::processEvents: retrieved " << events.size() << " events" << endl;
#endif

	while ((events.empty() == false) &&
		(m_done == false))
	{
		MonitorEvent &event = events.front();

		if ((event.m_location.empty() == true) ||
			(event.m_type == MonitorEvent::UNKNOWN))
		{
			// Next
			events.pop();
			continue;
		}
#ifdef DEBUG
		clog << "MonitorThread::processEvents: event " << event.m_type << " on "
			<< event.m_location << " " << event.m_isDirectory << endl;
#endif

		// Skip dotfiles and blacklisted files
		Url urlObj("file://" + event.m_location);
		if ((urlObj.getFile()[0] == '.') ||
			(isFileBlacklisted(event.m_location) == true))
		{
			// Next
			events.pop();
			continue;
		}

		// What's the event code ?
		if (event.m_type == MonitorEvent::EXISTS)
		{
			if (event.m_isDirectory == false)
			{
				m_pHandler->fileExists(event.m_location);
			}
		}
		else if (event.m_type == MonitorEvent::CREATED)
		{
			if (event.m_isDirectory == false)
			{
				m_pHandler->fileCreated(event.m_location);
			}
			else
			{
				m_pHandler->directoryCreated(event.m_location);
			}
		}
		else if (event.m_type == MonitorEvent::WRITE_CLOSED)
		{
			if (event.m_isDirectory == false)
			{
				fileModified(event.m_location);
			}
		}
		else if (event.m_type == MonitorEvent::MOVED)
		{
			if (event.m_isDirectory == false)
			{
				m_pHandler->fileMoved(event.m_location, event.m_previousLocation);
			}
			else
			{
				// We should receive this only if the destination directory is monitored too
				m_pHandler->directoryMoved(event.m_location, event.m_previousLocation);
			}
		}
		else if (event.m_type == MonitorEvent::DELETED)
		{
			if (event.m_isDirectory == false)
			{
				m_pHandler->fileDeleted(event.m_location);
			}
			else
			{
				// The monitor should have stopped monitoring this
				// In practice, events for the files in this directory will already have been received 
				m_pHandler->directoryDeleted(event.m_location);
			}
		}

		// Next
		events.pop();
	}
}

void MonitorThread::doWork(void)
{
	if ((m_pHandler == NULL) ||
		(m_pMonitor == NULL))
	{
		m_errorNum = NO_MONITORING;
		return;
	}

	// Initialize the handler
	m_pHandler->initialize();

	// Get the list of files to monitor
	const set<string> &fileNames = m_pHandler->getFileNames();
	for (set<string>::const_iterator fileIter = fileNames.begin();
		fileIter != fileNames.end(); ++fileIter)
	{
		m_pMonitor->addLocation(*fileIter, false);
	}
	// Directories, if any, are set elsewhere
	// In the case of OnDiskHandler, they are set by DirectoryScannerThread

	// There might already be events that need processing
	processEvents();

	// Wait for something to happen
	while (m_done == false)
	{
		struct timeval selectTimeout;
		fd_set listenSet;

		selectTimeout.tv_sec = 60;
		selectTimeout.tv_usec = 0;

		FD_ZERO(&listenSet);
		if (m_ctrlReadPipe >= 0)
		{
			FD_SET(m_ctrlReadPipe, &listenSet);
		}

		// The file descriptor may change over time
		int monitorFd = m_pMonitor->getFileDescriptor();
		FD_SET(monitorFd, &listenSet);
		if (monitorFd < 0)
		{
			m_errorNum = MONITORING_FAILED;
			return;
		}

		int fdCount = select(max(monitorFd, m_ctrlReadPipe) + 1, &listenSet, NULL, NULL, &selectTimeout);
		if ((fdCount < 0) &&
			(errno != EINTR))
		{
#ifdef DEBUG
			clog << "MonitorThread::doWork: select() failed" << endl;
#endif
			break;
		}
		else if (FD_ISSET(monitorFd, &listenSet))
		{
			processEvents();
		}
	}
}

