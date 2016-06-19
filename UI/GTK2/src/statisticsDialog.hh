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

#ifndef _STATISTICSDIALOG_HH
#define _STATISTICSDIALOG_HH

#include <time.h>
#include <map>
#include <sigc++/sigc++.h>
#include <glibmm/refptr.h>
#include <gtkmm/treestore.h>

#include "ModelColumns.hh"
#include "UIThreads.hh"
#include "statisticsDialog_glade.hh"

class statisticsDialog : public statisticsDialog_glade
{
public:
	statisticsDialog();
	virtual ~statisticsDialog();

protected:
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
	Gtk::TreeModel::iterator m_errorsTopIter;
	std::map<int, Gtk::TreeModel::iterator> m_errorsIters;
	std::map<unsigned int, time_t> m_latestErrorDates;
	bool m_hasErrors;
	bool m_hasDiskSpace;
	bool m_hasBattery;
	bool m_hasCrawl;
	sigc::connection m_idleConnection;
	class InternalState : public QueueManager
	{
	public:
		InternalState(statisticsDialog *pWindow);
		~InternalState();

		bool m_getStats;
		bool m_gettingStats;
		bool m_lowDiskSpace;
		bool m_onBattery;
		bool m_crawling;

	} m_state;

	void populate(void);

	void populate_history(void);

	bool on_activity_timeout(void);

	void on_thread_end(WorkerThread *pThread);

};

#endif
