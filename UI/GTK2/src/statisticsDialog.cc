/*
 *  Copyright 2005-2020 Fabrice Colin
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

#include <string>
#include <fstream>
#include <map>
#include <set>
#include <iostream>
#include <glibmm/convert.h>
#include <gtkmm/stock.h>

#include "config.h"
#include "NLS.h"
#include "Url.h"
#include "CrawlHistory.h"
#include "ViewHistory.h"
#ifdef HAVE_DBUS
#include "DBusIndex.h"
#endif
#include "ModuleFactory.h"
#include "PinotSettings.h"
#include "PinotUtils.hh"
#include "statisticsDialog.hh"

using namespace std;
using namespace Glib;
using namespace Gtk;

#ifdef HAVE_DBUS
class DaemonStatusThread : public WorkerThread
{
        public:
                DaemonStatusThread() :
			WorkerThread(),
			m_gotStats(false),
			m_lowDiskSpace(false),
			m_onBattery(false),
			m_crawling(false),
			m_crawledCount(0),
			m_docsCount(0)
		{
		}
                virtual ~DaemonStatusThread()
		{
		}

                virtual std::string getType(void) const
		{
			return "DaemonStatusThread";
		}

		bool m_gotStats;
		bool m_lowDiskSpace;
		bool m_onBattery;
		bool m_crawling;
		unsigned int m_crawledCount;
		unsigned int m_docsCount;

        protected:
                virtual void doWork(void)
		{
			if (DBusIndex::getStatistics(m_crawledCount, m_docsCount,
				m_lowDiskSpace, m_onBattery, m_crawling) == true)
			{
				m_gotStats = true;
			}
#ifdef DEBUG
			else clog << "DaemonStatusThread::doWork: failed to get statistics" << endl;
#endif
		}

        private:
                DaemonStatusThread(const DaemonStatusThread &other);
                DaemonStatusThread &operator=(const DaemonStatusThread &other);

};
#endif

statisticsDialog::InternalState::InternalState(statisticsDialog *pWindow) :
        QueueManager(PinotSettings::getInstance().m_docsIndexLocation),
	m_getStats(true),
	m_gettingStats(false),
	m_lowDiskSpace(false),
	m_onBattery(false),
	m_crawling(false)
{
        m_onThreadEndSignal.connect(sigc::mem_fun(*pWindow, &statisticsDialog::on_thread_end));
}

statisticsDialog::InternalState::~InternalState()
{
}

statisticsDialog::statisticsDialog() :
	statisticsDialog_glade(),
	m_hasErrors(false),
	m_hasDiskSpace(false),
	m_hasBattery(false),
	m_hasCrawl(false),
	m_state(this)
{
	// Associate the columns model to the engines tree
	m_refStore = TreeStore::create(m_statsColumns);
	statisticsTreeview->set_model(m_refStore);

	TreeViewColumn *pColumn = create_column(_("Status"), m_statsColumns.m_name, true, true, m_statsColumns.m_name);
	if (pColumn != NULL)
	{
		statisticsTreeview->append_column(*manage(pColumn));
	}

	// Resize the window
	resize((int )(get_width() * 2), (int )(get_height() * 1.5));

	// Populate
	populate();
	// ...and update regularly
	m_idleConnection = Glib::signal_timeout().connect(sigc::mem_fun(*this,
		&statisticsDialog::on_activity_timeout), 10000);

	// Connect to threads' finished signal
	m_state.connect();
}

statisticsDialog::~statisticsDialog()
{
	m_idleConnection.disconnect();
	m_state.disconnect();
}

void statisticsDialog::populate(void)
{
	TreeModel::iterator folderIter = m_refStore->append();
	TreeModel::Row row = *folderIter;
	std::map<ModuleProperties, bool> engines;

	// Indexes
	statisticsTreeview->get_selection()->select(folderIter);
	row[m_statsColumns.m_name] = _("Indexes");
	TreeModel::iterator statIter = m_refStore->append(folderIter->children());
	row = *statIter;
	row[m_statsColumns.m_name] = _("My Web Pages");
	m_myWebPagesIter = m_refStore->append(statIter->children());
	row = *m_myWebPagesIter;
	row[m_statsColumns.m_name] = ustring(_("Checking"));
	statIter = m_refStore->append(folderIter->children());
	row = *statIter;
	row[m_statsColumns.m_name] = _("My Documents");
	m_myDocumentsIter = m_refStore->append(statIter->children());
	row = *m_myDocumentsIter;
	row[m_statsColumns.m_name] = ustring(_("Checking"));

	// Search engines
	TreeModel::iterator enginesIter = m_refStore->append();
	row = *enginesIter;
	row[m_statsColumns.m_name] = _("Search Engines");
	ModuleFactory::getSupportedEngines(engines);
	for (std::map<ModuleProperties, bool>::const_iterator engineIter = engines.begin();
		engineIter != engines.end(); ++engineIter)
	{
		TreeModel::iterator statIter = m_refStore->append(enginesIter->children());
		row = *statIter;
		row[m_statsColumns.m_name] = engineIter->first.m_name;
	}

	// History
	folderIter = m_refStore->append();
	row = *folderIter;
	row[m_statsColumns.m_name] = _("History");
	m_viewStatIter = m_refStore->append(folderIter->children());
	row = *m_viewStatIter;
	m_crawledStatIter = m_refStore->append(folderIter->children());
	row = *m_crawledStatIter;
	populate_history();

	// Daemon
	m_daemonIter = m_refStore->append();
	row = *m_daemonIter;
	row[m_statsColumns.m_name] = _("Daemon");
	m_daemonProcIter = m_refStore->append(m_daemonIter->children());
	row = *m_daemonProcIter;
	row[m_statsColumns.m_name] = ustring(_("Checking"));

	// Expand everything
	statisticsTreeview->expand_all();
	TreeModel::Path enginesPath = m_refStore->get_path(enginesIter);
	statisticsTreeview->collapse_row(enginesPath);
}

void statisticsDialog::populate_history(void)
{
	CrawlHistory crawlHistory(PinotSettings::getInstance().getHistoryDatabaseName(true));
	ViewHistory viewHistory(PinotSettings::getInstance().getHistoryDatabaseName());
	TreeModel::Row row;
	char countStr[64];

	// Show view statistics
	unsigned int viewCount = viewHistory.getItemsCount();
	snprintf(countStr, 64, "%u", viewCount);
	row = *m_viewStatIter;
	row[m_statsColumns.m_name] = ustring(_("Viewed")) + " " + countStr + " " + _("results");

	// Show crawler statistics
	unsigned int crawledFilesCount = crawlHistory.getItemsCount(CrawlHistory::CRAWLED);
	snprintf(countStr, 64, "%u", crawledFilesCount);
	row = *m_crawledStatIter;
	row[m_statsColumns.m_name] = ustring(_("Crawled")) + " " + countStr + " " + _("files");
}

bool statisticsDialog::on_activity_timeout(void)
{
	TreeModel::Row row;
	std::map<unsigned int, string> sources;
	string daemonDBusStatus;
#ifdef HAVE_DBUS
	string programName("pinot-dbus-daemon");
#else
	string programName("pinot-daemon");
#endif
	char countStr[64];

	row = *m_myWebPagesIter;
	IndexInterface *pIndex = PinotSettings::getInstance().getIndex(PinotSettings::getInstance().m_docsIndexLocation);
	if (pIndex != NULL)
	{
		unsigned int docsCount = pIndex->getDocumentsCount();

		snprintf(countStr, 64, "%u", docsCount);
		row[m_statsColumns.m_name] = ustring(countStr) + " " + _("documents");

		delete pIndex;
	}
	else
	{
		row[m_statsColumns.m_name] = _("Unknown error");
	}

	row = *m_myDocumentsIter;
	pIndex = PinotSettings::getInstance().getIndex(PinotSettings::getInstance().m_daemonIndexLocation);
	if (pIndex != NULL)
	{
		unsigned int docsCount = pIndex->getDocumentsCount();

		snprintf(countStr, 64, "%u", docsCount);
		row[m_statsColumns.m_name] = ustring(countStr) + " " + _("documents");

		daemonDBusStatus = pIndex->getMetadata("dbus-status");

		delete pIndex;
	}
	else
	{
		row[m_statsColumns.m_name] = _("Unknown error");
	}

	populate_history();

	// Is the daemon still running ?
	string pidFileName(PinotSettings::getInstance().getConfigurationDirectory() + "/" + programName + ".pid");
	pid_t daemonPID = 0;
	ifstream pidFile;
	pidFile.open(pidFileName.c_str());
	if (pidFile.is_open() == true)
	{
		pidFile >> daemonPID;
		pidFile.close();
	}
	snprintf(countStr, 64, "%u", daemonPID);

	row = *m_daemonProcIter;
	if (daemonPID > 0)
	{
		// FIXME: check whether it's actually running !
		row[m_statsColumns.m_name] = ustring(_("Running under PID")) + " " + countStr;

#ifdef HAVE_DBUS
		if ((m_state.m_getStats == true) &&
			(m_state.m_gettingStats == false))
		{
			m_state.m_gettingStats = true;

			DaemonStatusThread *pThread = new DaemonStatusThread();

			if (m_state.start_thread(pThread, false) == false)
			{
				delete pThread;
				m_state.m_getStats = m_state.m_gettingStats = false;
			}
		}
#endif
	}
	else
	{
		if (daemonDBusStatus == "Disconnected")
		{
			row[m_statsColumns.m_name] = _("Disconnected from D-Bus");
		}
		else if (daemonDBusStatus == "Stopped")
		{
			row[m_statsColumns.m_name] = _("Stopped");
		}
		else
		{
			row[m_statsColumns.m_name] = _("Currently not running");
		}
	}

	// Show status
	if (m_state.m_lowDiskSpace == true)
	{
		if (m_hasDiskSpace == false)
		{
			m_diskSpaceIter = m_refStore->insert_after(m_daemonProcIter);

			m_hasDiskSpace = true;
		}
		row = *m_diskSpaceIter;
		row[m_statsColumns.m_name] = ustring(_("Low disk space"));
	}
	else if (m_hasDiskSpace == true)
	{
		m_refStore->erase(m_diskSpaceIter);

		m_hasDiskSpace = false;
	}
	if (m_state.m_onBattery == true)
	{
		if (m_hasBattery == false)
		{
			m_batteryIter = m_refStore->insert_after(m_daemonProcIter);

			m_hasBattery = true;
		}
		row = *m_batteryIter;
		row[m_statsColumns.m_name] = ustring(_("System on battery"));
	}
	else if (m_hasBattery == true)
	{
		m_refStore->erase(m_batteryIter);

		m_hasBattery = false;
	}
	if (m_state.m_crawling == true)
	{
		if (m_hasCrawl == false)
		{
			m_crawlIter = m_refStore->insert_after(m_daemonProcIter);

			m_hasCrawl = true;
		}

		row = *m_crawlIter;
		row[m_statsColumns.m_name] = ustring(_("Crawling"));
	}
	else if (m_hasCrawl == true)
	{
		m_refStore->erase(m_crawlIter);

		m_hasCrawl = false;
	}

	CrawlHistory crawlHistory(PinotSettings::getInstance().getHistoryDatabaseName(true));

	// Show errors
	crawlHistory.getSources(sources);
	for (std::map<unsigned int, string>::iterator sourceIter = sources.begin();
		sourceIter != sources.end(); ++sourceIter)
	{
		unsigned int sourceNum(sourceIter->first);
		set<string> errors;
		time_t latestErrorDate = 0;
		unsigned int currentOffset = 0;

		std::map<unsigned int, time_t>::const_iterator dateIter = m_latestErrorDates.find(sourceNum);
		if (dateIter != m_latestErrorDates.end())
		{
			latestErrorDate = dateIter->second;
		}

		// Did any error occur on this source ?
		unsigned int errorCount = crawlHistory.getSourceItems(sourceNum,
			CrawlHistory::CRAWL_ERROR, errors, currentOffset, currentOffset + 100,
			latestErrorDate);
		while ((errorCount > 0) &&
			(errors.empty() == false))
		{
			// Add an errors row
			if (m_hasErrors == false)
			{
				m_errorsTopIter = m_refStore->append(m_daemonIter->children());
				row = *m_errorsTopIter;
				row[m_statsColumns.m_name] = _("Errors");

				m_hasErrors = true;
			}

			// List them
			for (set<string>::const_iterator errorIter = errors.begin();
				errorIter != errors.end(); ++errorIter)
			{
				string locationWithError(*errorIter);
				TreeModel::iterator errIter;
				time_t errorDate;
				int errorNum = crawlHistory.getErrorDetails(locationWithError, errorDate);

				if (errorDate > latestErrorDate)
				{
					latestErrorDate = errorDate;
				}

				// Find or create the iterator for this particular kind of error
				std::map<int, TreeModel::iterator>::const_iterator errorTreeIter = m_errorsIters.find(errorNum);
				if (errorTreeIter == m_errorsIters.end())
				{
					string errorText(WorkerThread::errorToString(errorNum));

					errIter = m_refStore->append(m_errorsTopIter->children());
					row = *errIter;
					row[m_statsColumns.m_name] = errorText;

					m_errorsIters.insert(pair<int, TreeModel::iterator>(errorNum, errIter));
				}
				else
				{
					errIter = errorTreeIter->second;
				}

				// Display the location itself
				TreeModel::iterator locIter = m_refStore->append(errIter->children());
				row = *locIter;
				row[m_statsColumns.m_name] = locationWithError;
			}

			// Expand errors
			TreeModel::Path errPath = m_refStore->get_path(m_errorsTopIter);
			statisticsTreeview->expand_to_path(errPath);

			// Next
			if (errors.size() < 100)
			{
				break;
			}
			currentOffset += 100;
			errorCount = crawlHistory.getSourceItems(sourceNum,
				CrawlHistory::CRAWL_ERROR, errors, currentOffset, 100, // currentOffset + 100,
				latestErrorDate);
		}

		// The next check will ignore errors older than this
		m_latestErrorDates[sourceNum] = latestErrorDate;
	}

	return true;
}

void statisticsDialog::on_thread_end(WorkerThread *pThread)
{
	ustring status;
	bool success = true;

	if (pThread == NULL)
	{
		return;
	}

	// Did the thread fail ?
	status = pThread->getStatus();
	if (status.empty() == false)
	{
#ifdef DEBUG
		clog << "statisticsDialog::on_thread_end: " << status << endl;
#endif
		success = false;
	}

	// What type of thread was it ?
	string type = pThread->getType();
#ifdef HAVE_DBUS
	if (type == "DaemonStatusThread")
	{
		m_state.m_gettingStats = false;

		// Did it succeed ?
		if (success == true)
		{
			DaemonStatusThread *pStatusThread = dynamic_cast<DaemonStatusThread*>(pThread);
			if (pStatusThread != NULL)
			{
				// Yes, it did
				m_state.m_getStats = pStatusThread->m_gotStats;
				m_state.m_lowDiskSpace = pStatusThread->m_lowDiskSpace;
				m_state.m_onBattery = pStatusThread->m_onBattery;
				m_state.m_crawling = pStatusThread->m_crawling;
#ifdef DEBUG
				clog << "statisticsDialog::on_thread_end: refreshed stats" << endl;
#endif
			}
		}
	}
#endif

	// Delete the thread
	delete pThread;

	// We might be able to run a queued action
	m_state.pop_queue();
}

