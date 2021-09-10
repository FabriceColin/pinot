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

#include <string>
#include <fstream>
#include <map>
#include <set>
#include <iostream>
#include <sstream>
#include <glibmm/convert.h>
#include <gtkmm/stock.h>

#include "config.h"
#include "NLS.h"
#include "Url.h"
#include "CrawlHistory.h"
#include "ViewHistory.h"
#include "ModuleFactory.h"
#include "PinotSettings.h"
#include "PinotUtils.hh"
#include "statisticsDialog.hh"

using namespace std;
using namespace Glib;
using namespace Gtk;

statisticsDialog::statisticsDialog(GtkDialog *&pParent, RefPtr<Builder>& refBuilder) :
	Dialog(pParent),
	closeStatisticsButton(NULL),
	statisticsTreeview(NULL),
	m_hasDiskSpace(false),
	m_hasBattery(false),
	m_hasCrawl(false)
{
	refBuilder->get_widget("closeStatisticsButton", closeStatisticsButton);
	refBuilder->get_widget("statisticsTreeview", statisticsTreeview);

	closeStatisticsButton->signal_clicked().connect(sigc::mem_fun(*this, &statisticsDialog::on_closeStatisticsButton_clicked), false);
	signal_delete_event().connect(sigc::mem_fun(*this, &statisticsDialog::on_statisticsDialog_delete_event), false);

	m_signalStats.connect(sigc::mem_fun(*this, &statisticsDialog::on_stats_changed));

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
}

statisticsDialog::~statisticsDialog()
{
}

sigc::signal5<void, bool, bool, bool, unsigned int, unsigned int>& statisticsDialog::getStatsSignal(void)
{
	return m_signalStats;
}

void statisticsDialog::populate(void)
{
	TreeModel::iterator folderIter = m_refStore->append();
	TreeModel::Row row = *folderIter;
	std::map<ModuleProperties, bool> engines;
	stringstream countStr;

	// Indexes
	statisticsTreeview->get_selection()->select(folderIter);
	row[m_statsColumns.m_name] = _("Indexes");
	TreeModel::iterator statIter = m_refStore->append(folderIter->children());
	row = *statIter;
	row[m_statsColumns.m_name] = _("My Web Pages");
	m_myWebPagesIter = m_refStore->append(statIter->children());
	row = *m_myWebPagesIter;
	IndexInterface *pIndex = PinotSettings::getInstance().getIndex(PinotSettings::getInstance().m_docsIndexLocation);
	if (pIndex != NULL)
	{
		unsigned int docsCount = pIndex->getDocumentsCount();

		countStr << docsCount;
		row[m_statsColumns.m_name] = ustring(countStr.str()) + " " + _("documents");

		delete pIndex;
	}
	else
	{
		row[m_statsColumns.m_name] = _("Unknown error");
	}
	statIter = m_refStore->append(folderIter->children());
	row = *statIter;
	row[m_statsColumns.m_name] = _("My Documents");
	m_myDocumentsIter = m_refStore->append(statIter->children());
	row = *m_myDocumentsIter;
	pIndex = PinotSettings::getInstance().getIndex(PinotSettings::getInstance().m_daemonIndexLocation);
	if (pIndex != NULL)
	{
		unsigned int docsCount = pIndex->getDocumentsCount();

		countStr << docsCount;
		row[m_statsColumns.m_name] = ustring(countStr.str()) + " " + _("documents");

		m_daemonDBusStatus = pIndex->getMetadata("dbus-status");

		delete pIndex;
	}
	else
	{
		row[m_statsColumns.m_name] = _("Unknown error");
	}

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
	stringstream countStr;

	// Show view statistics
	unsigned int viewCount = viewHistory.getItemsCount();
	countStr << viewCount;
	row = *m_viewStatIter;
	row[m_statsColumns.m_name] = ustring(_("Viewed")) + " " + countStr.str() + " " + _("results");
	countStr.clear();

	// Show crawler statistics
	unsigned int crawledFilesCount = crawlHistory.getItemsCount(CrawlHistory::CRAWLED);
	countStr << crawledFilesCount;
	row = *m_crawledStatIter;
	row[m_statsColumns.m_name] = ustring(_("Crawled")) + " " + countStr.str() + " " + _("files");
}

void statisticsDialog::on_closeStatisticsButton_clicked()
{
#ifdef DEBUG
	clog << "statisticsDialog::on_closeStatisticsButton_clicked: called" << endl;
#endif
	close();
}

bool statisticsDialog::on_statisticsDialog_delete_event(GdkEventAny *ev)
{
#ifdef DEBUG
	clog << "statisticsDialog::on_statisticsDialog_delete_event: called" << endl;
#endif
	return false;
}

void statisticsDialog::on_stats_changed(unsigned int crawledCOunt, unsigned int docsCount,
	bool lowDiskSpace, bool onBattery, bool crawling)
{
	TreeModel::Row row;
	stringstream countStr;
#ifdef HAVE_DBUS
	string programName("pinot-dbus-daemon");
#else
	string programName("pinot-daemon");
#endif
#ifdef DEBUG
	clog << "statisticsDialog::on_stats_changed: called" << endl;
#endif

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
	countStr << daemonPID;

	row = *m_daemonProcIter;
	if (daemonPID > 0)
	{
		// FIXME: check whether it's actually running !
		row[m_statsColumns.m_name] = ustring(_("Running under PID")) + " " + countStr.str();
	}
	else if (m_daemonDBusStatus == "Disconnected")
	{
		row[m_statsColumns.m_name] = _("Disconnected from D-Bus");
	}
	else if (m_daemonDBusStatus == "Stopped")
	{
		row[m_statsColumns.m_name] = _("Stopped");
	}
	else
	{
		row[m_statsColumns.m_name] = _("Currently not running");
	}

	// Show status
	if (lowDiskSpace == true)
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
	if (onBattery == true)
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
	if (crawling == true)
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
}

