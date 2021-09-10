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

#ifndef _STATISTICSDIALOG_HH
#define _STATISTICSDIALOG_HH

#include <time.h>
#include <map>
#include <sigc++/sigc++.h>
#include <glibmm/refptr.h>
#include <gtkmm/builder.h>
#include <gtkmm/button.h>
#include <gtkmm/dialog.h>
#include <gtkmm/treestore.h>
#include <gtkmm/treeview.h>

#include "ModelColumns.h"
#include "UIThreads.h"

class StatisticsDialog : public Gtk::Dialog
{
public:
	StatisticsDialog(_GtkDialog *&pParent, Glib::RefPtr<Gtk::Builder>& refBuilder);
	virtual ~StatisticsDialog();

	sigc::signal5<void, bool, bool, bool, unsigned int, unsigned int>& getStatsSignal(void);

protected:
	Gtk::Button *closeStatisticsButton;
	Gtk::TreeView *statisticsTreeview;

	Glib::RefPtr<Gtk::TreeStore> m_refStore;
	ComboModelColumns m_statsColumns;
	Gtk::TreeModel::iterator m_myWebPagesIter;
	Gtk::TreeModel::iterator m_myDocumentsIter;
	Gtk::TreeModel::iterator m_viewStatIter;
	Gtk::TreeModel::iterator m_crawledStatIter;
	Gtk::TreeModel::iterator m_daemonIter;
	Gtk::TreeModel::iterator m_daemonProcIter;
	Gtk::TreeModel::iterator m_diskSpaceIter;
	Gtk::TreeModel::iterator m_batteryIter;
	Gtk::TreeModel::iterator m_crawlIter;
	sigc::signal5<void, bool, bool, bool, unsigned int, unsigned int> m_signalStats;
	std::string m_daemonDBusStatus;
	bool m_hasDiskSpace;
	bool m_hasBattery;
	bool m_hasCrawl;

	void populate(void);

	void populate_history(void);

	// Handlers defined in the XML file
	virtual void on_closeStatisticsButton_clicked();
	virtual bool on_statisticsDialog_delete_event(GdkEventAny *ev);

	virtual void on_stats_changed(unsigned int crawledCOunt, unsigned int docsCount,
		bool lowDiskSpace, bool onBattery, bool crawling);

};

#endif
