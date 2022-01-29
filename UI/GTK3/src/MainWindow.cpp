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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <string>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <list>
#include <glibmmconfig.h>
#include <glibmm/convert.h>
#include <glibmm/stringutils.h>
#include <glibmm/thread.h>
#include <glibmm/ustring.h>
#include <gtkmm/aboutdialog.h>
#include <gtkmm/checkbutton.h>
#include <gtkmm/clipboard.h>
#include <gtkmm/main.h>
#include <gtkmm/messagedialog.h>
#include <gtkmm/recentmanager.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/separatormenuitem.h>
#include <gtkmm/stock.h>
#include <gtkmm/targetlist.h>

#include "config.h"
#include "NLS.h"
#include "CommandLine.h"
#include "TimeConverter.h"
#include "MIMEScanner.h"
#include "Url.h"
#include "MonitorFactory.h"
#include "QueryHistory.h"
#include "ViewHistory.h"
#include "ModuleFactory.h"
#ifdef HAVE_DBUS
#include "DBusIndex.h"
#endif
#include "PinotUtils.h"
#include "MainWindow.h"
#include "ImportDialog.h"
#include "IndexDialog.h"
#include "LauncherDialog.h"
#include "PropertiesDialog.h"
#include "QueryDialog.h"
#include "StatisticsDialog.h"

using namespace std;
using namespace Glib;
using namespace Gdk;
using namespace Gtk;

// A function object to delete temporary files with for_each()
struct DeleteTemporaryFileFunc
{
	public:
		void operator()(const string fileName)
		{
			if (fileName.empty() == false)
			{
				unlink(fileName.c_str());
			}
		}
};

class ReloadHandler : public MonitorHandler
{
	public:
		ReloadHandler() : MonitorHandler()
		{
			// Get the list of files we will be handling
			MIMEScanner::listConfigurationFiles(PinotSettings::getHomeDirectory() + "/.local", m_fileNames);
			MIMEScanner::listConfigurationFiles(string(SHARED_MIME_INFO_PREFIX), m_fileNames);
		}
		virtual ~ReloadHandler() {}

		/// Handles file modified events.
		virtual bool fileModified(const std::string &fileName)
		{
			// Re-initialize the MIME sub-system
			return MIMEScanner::initialize(PinotSettings::getHomeDirectory() + "/.local",
				string(SHARED_MIME_INFO_PREFIX));
		}

	private:
		ReloadHandler(const ReloadHandler &other);
		ReloadHandler &operator=(const ReloadHandler &other);

};

// FIXME: this ought to be configurable
unsigned int MainWindow::m_maxDocsCount = 100;

MainWindow::ExpandSet::ExpandSet()
{
}

MainWindow::ExpandSet::~ExpandSet()
{
}

MainWindow::InternalState::InternalState(MainWindow *pWindow) :
	QueueManager(PinotSettings::getInstance().m_docsIndexLocation, 60, true),
#ifdef HAVE_DBUS
	m_refProxy(com::github::fabricecolin::PinotProxy::createForBus_sync(
		Gio::DBus::BUS_TYPE_SESSION,
		Gio::DBus::PROXY_FLAGS_NONE,
		PINOT_DBUS_SERVICE_NAME, PINOT_DBUS_OBJECT_PATH)),
	m_flushEpoch(time(NULL)),
#endif
	m_liveQueryLength(0),
	m_currentPage(0),
	m_browsingIndex(false)
{
	m_onThreadEndSignal.connect(sigc::mem_fun(*pWindow, &MainWindow::on_thread_end));
}

MainWindow::InternalState::~InternalState()
{
}

//
// Constructor
//
MainWindow::MainWindow(_GtkWindow *&pParent, RefPtr<Builder>& refBuilder,
	const Glib::ustring &statusText) :
	Window(pParent),
	m_refBuilder(refBuilder),
	enginesVbox(NULL),
	addIndexButton(NULL),
	removeIndexButton(NULL),
	leftVbox(NULL),
	enginesTogglebutton(NULL),
	liveQueryEntry(NULL),
	findButton(NULL),
	queryTreeview(NULL),
	addQueryButton(NULL),
	removeQueryButton(NULL),
	queryHistoryButton(NULL),
	findQueryButton(NULL),
	queryExpander(NULL),
	rightVbox(NULL),
	mainHpaned(NULL),
	mainHbox(NULL),
	mainProgressbar(NULL),
	mainStatusbar(NULL),
	mainVbox(NULL),
	mainMenubar(NULL),
	m_settings(PinotSettings::getInstance()),
	m_pEnginesTree(NULL),
	m_pNotebook(NULL),
	open1(NULL),
	opencache1(NULL),
	openparent1(NULL),
	addtoindex1(NULL),
	updateindex1(NULL),
	unindex1(NULL),
	morelikethis1(NULL),
	searchthisfor1(NULL),
	properties1(NULL),
	separatormenuitem1(NULL),
	quit1(NULL),
	fileMenuitem(NULL),
	cut1(NULL),
	copy1(NULL),
	paste1(NULL),
	delete1(NULL),
	preferences1(NULL),
	editMenuitem(NULL),
	listcontents1(NULL),
	searchenginegroup1(NULL),
	hostnamegroup1(NULL),
	groupresults1(NULL),
	showextracts1(NULL),
	import1(NULL),
	export1(NULL),
	about1(NULL),
	helpMenuitem(NULL),
	m_pIndexMenu(NULL),
	m_pCacheMenu(NULL),
	m_pFindMenu(NULL),
	m_pSettingsMonitor(MonitorFactory::getMonitor()),
	m_pSettingsHandler(NULL),
	m_state(this),
	m_maxResultsCount(10)
{
	m_refBuilder->get_widget("enginesVbox", enginesVbox);
	m_refBuilder->get_widget("addIndexButton", addIndexButton);
	m_refBuilder->get_widget("removeIndexButton", removeIndexButton);
	m_refBuilder->get_widget("leftVbox", leftVbox);
	m_refBuilder->get_widget("enginesTogglebutton", enginesTogglebutton);
	m_refBuilder->get_widget("liveQueryEntry", liveQueryEntry);
	m_refBuilder->get_widget("findButton", findButton);
	m_refBuilder->get_widget("queryTreeview", queryTreeview);
	m_refBuilder->get_widget("addQueryButton", addQueryButton);
	m_refBuilder->get_widget("removeQueryButton", removeQueryButton);
	m_refBuilder->get_widget("queryHistoryButton", queryHistoryButton);
	m_refBuilder->get_widget("findQueryButton", findQueryButton);
	// The Expander can't be looked up here for some reason...
	// m_refBuilder->get_widget("queryExpander". queryExpander);
	// ... so instead resolve it as it's this button's grand-parent
	queryExpander = dynamic_cast<Gtk::Expander*>(findQueryButton->get_parent()->get_parent()->get_parent());
	m_refBuilder->get_widget("rightVbox", rightVbox);
	m_refBuilder->get_widget("mainProgressbar", mainProgressbar);
	m_refBuilder->get_widget("mainStatusbar", mainStatusbar);
	m_refBuilder->get_widget("mainVbox", mainVbox);
	// We only need these two to insert the menu bar
	m_refBuilder->get_widget("mainHpaned", mainHpaned);
	m_refBuilder->get_widget("mainHbox", mainHbox);

	mainMenubar = Gtk::manage(new class Gtk::MenuBar());

	// Remove those, they will be added in later after the menu bar
	mainVbox->remove(*mainHpaned);
	mainVbox->remove(*mainHbox);

	addIndexButton->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_addIndexButton_clicked), false);
	removeIndexButton->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_removeIndexButton_clicked), false);
	enginesTogglebutton->signal_toggled().connect(sigc::mem_fun(*this, &MainWindow::on_enginesTogglebutton_toggled), false);
	liveQueryEntry->signal_focus_in_event().connect(sigc::mem_fun(*this, &MainWindow::on_liveQueryEntry_focus), false);
	liveQueryEntry->signal_focus_out_event().connect(sigc::mem_fun(*this, &MainWindow::on_liveQueryEntry_focus), false);
	liveQueryEntry->signal_changed().connect(sigc::mem_fun(*this, &MainWindow::on_liveQueryEntry_changed), false);
	liveQueryEntry->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_liveQueryEntry_activate), false);
	findButton->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_findButton_clicked), false);
	queryTreeview->signal_focus_in_event().connect(sigc::mem_fun(*this, &MainWindow::on_queryTreeview_focus), false);
	queryTreeview->signal_focus_out_event().connect(sigc::mem_fun(*this, &MainWindow::on_queryTreeview_focus), false);
	queryTreeview->signal_button_press_event().connect(sigc::mem_fun(*this, &MainWindow::on_queryTreeview_button_press_event), false);
	addQueryButton->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_addQueryButton_clicked), false);
	removeQueryButton->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_removeQueryButton_clicked), false);
	queryHistoryButton->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_queryHistoryButton_clicked), false);
	findQueryButton->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_findQueryButton_clicked), false);
	signal_delete_event().connect(sigc::mem_fun(*this, &MainWindow::on_mainWindow_delete_event), false);

	// Populate menu items
	populate_menuItems();

	// Override the number of query results ?
	char *pEnvVar = getenv("PINOT_MAXIMUM_QUERY_RESULTS");
	if ((pEnvVar != NULL) &&
		(strlen(pEnvVar) > 0))
	{
		int resultsNum = atoi(pEnvVar);

		if (resultsNum > 0)
		{
			m_maxResultsCount = (unsigned int)resultsNum;
		}
	}

	// Reposition and resize the window
	// Make sure the coordinates and sizes make sense
	if ((m_settings.m_xPos >= 0) &&
		(m_settings.m_yPos >= 0))
	{
		move(m_settings.m_xPos, m_settings.m_yPos);
	}
	if ((m_settings.m_width > 0) &&
		(m_settings.m_height > 0))
	{
		resize(m_settings.m_width, m_settings.m_height);
	}
	if (m_settings.m_panePos >= 0)
	{
		mainHpaned->set_position(m_settings.m_panePos);
	}
#ifdef DEBUG
	clog << "MainWindow::MainWindow: expanded ?" << m_settings.m_expandQueries << endl;
#endif
	queryExpander->set_expanded(m_settings.m_expandQueries);
	enginesTogglebutton->set_active(m_settings.m_showEngines);
	if (m_settings.m_showEngines == true)
	{
		leftVbox->show();
	}

	// Position the engine tree
	m_pEnginesTree = manage(new EnginesTree(enginesVbox, m_settings));
	// Connect to the "changed" signal
	m_pEnginesTree->get_selection()->signal_changed().connect(
		sigc::mem_fun(*this, &MainWindow::on_enginesTreeviewSelection_changed));
	// Connect to the edit index signal
	m_pEnginesTree->getDoubleClickSignal().connect(
		sigc::mem_fun(*this, &MainWindow::on_editindex));

	// Enable completion on the live query field
	m_refLiveQueryList = ListStore::create(m_liveQueryColumns);
	m_refLiveQueryCompletion = EntryCompletion::create();
	m_refLiveQueryCompletion->set_model(m_refLiveQueryList);
	m_refLiveQueryCompletion->set_text_column(m_liveQueryColumns.m_name);
	m_refLiveQueryCompletion->set_minimum_key_length(3);
	m_refLiveQueryCompletion->set_popup_completion(true);
	liveQueryEntry->set_completion(m_refLiveQueryCompletion);

	// Enable the find icon
	liveQueryEntry->set_icon_from_stock(Stock::FIND, ENTRY_ICON_SECONDARY);
	liveQueryEntry->signal_icon_press().connect(sigc::mem_fun(*this, &MainWindow::on_liveQueryEntry_icon), false);
	liveQueryEntry->set_icon_sensitive(ENTRY_ICON_SECONDARY, false);

	// Associate the columns model to the query tree
	m_refQueryTree = ListStore::create(m_queryColumns);
	queryTreeview->set_model(m_refQueryTree);
	TreeViewColumn *pColumn = create_column(_("Query Name"), m_queryColumns.m_name, true, true, m_queryColumns.m_name);
	if (pColumn != NULL)
	{
		queryTreeview->append_column(*manage(pColumn));
	}
	pColumn = create_column(_("Last Run"), m_queryColumns.m_lastRun, true, true, m_queryColumns.m_lastRunTime);
	if (pColumn != NULL)
	{
		queryTreeview->append_column(*manage(pColumn));
	}
	queryTreeview->append_column(_("Summary"), m_queryColumns.m_summary);
	// Allow only single selection
	queryTreeview->get_selection()->set_mode(SELECTION_SINGLE);
	// Connect to the "changed" signal
	queryTreeview->get_selection()->signal_changed().connect(
		sigc::mem_fun(*this, &MainWindow::on_queryTreeviewSelection_changed));

	// Specify drop targets
	queryTreeview->drag_dest_set(Gtk::DEST_DEFAULT_ALL, DragAction(GDK_ACTION_COPY|GDK_ACTION_MOVE));
	// FIXME: this causes some text files to be received as text, others as URIs !
	//queryTreeview->drag_dest_add_text_targets();
	queryTreeview->drag_dest_add_uri_targets();
	RefPtr<TargetList> targetList = queryTreeview->drag_dest_get_target_list();
	targetList->add("UTF8_STRING", TargetFlags(0), 0);
	targetList->add("text/plain", TargetFlags(0), 0);
	// Connect to the drag data received signal
	queryTreeview->signal_drag_data_received().connect(
		sigc::mem_fun(*this, &MainWindow::on_data_received));

	// Populate
	populate_queryTreeview("");

	// Create a notebook, without any page initially
	m_pNotebook = manage(new Notebook());
	m_pNotebook->set_can_focus();
	m_pNotebook->set_show_tabs(true);
	m_pNotebook->set_show_border(true);
	m_pNotebook->set_tab_pos(Gtk::POS_TOP);
	m_pNotebook->set_scrollable(true);
	rightVbox->pack_start(*m_pNotebook, Gtk::PACK_EXPAND_WIDGET, 4);
	m_pageSwitchConnection = m_pNotebook->signal_switch_page().connect(
		sigc::mem_fun(*this, &MainWindow::on_switch_page), false);

	// Gray out menu items
	show_pagebased_menuitems(false);
	show_selectionbased_menuitems(false);
	// ...and buttons
	removeIndexButton->set_sensitive(false);
	removeQueryButton->set_sensitive(false);
	queryHistoryButton->set_sensitive(false);

	// Set tooltips
	enginesTogglebutton->set_tooltip_text(_("Show all search engines"));
	liveQueryEntry->set_icon_tooltip_text(_("Find documents that match the query"), ENTRY_ICON_SECONDARY);
	addIndexButton->set_tooltip_text(_("Add an index"));
	removeIndexButton->set_tooltip_text(_("Remove an index"));
	addQueryButton->set_tooltip_text(_("Add a query"));
	removeQueryButton->set_tooltip_text(_("Remove a query"));
	queryHistoryButton->set_tooltip_text(_("Show the query's previous results"));
	findQueryButton->set_tooltip_text(_("Find documents that match the query"));

	// Connect to threads' finished signal
	m_state.connect();

	if (m_pSettingsMonitor != NULL)
	{
		// Run this in the background
		m_pSettingsHandler = new ReloadHandler();
		MonitorThread *pSettingsMonitorThread = new MonitorThread(m_pSettingsMonitor, m_pSettingsHandler);
		start_thread(pSettingsMonitorThread, true);
	}

	// Populate the menus
	populate_cacheMenu();
	populate_indexMenu();
	populate_findMenu();

	// Now we are ready
	if (statusText.empty() == true)
	{
		set_status(_("Ready"));
	}
	else
	{
		set_status(statusText);
	}
	m_pNotebook->show();
	show();
	liveQueryEntry->grab_focus();

	// Open the preferences on first run
	if (m_settings.isFirstRun() == true)
	{
		on_preferences_activate();
	}
	else if	(m_settings.m_warnAboutVersion == true)
	{
		ustring boxMsg(_("Updating all documents in My Web Pages is recommended"));
		MessageDialog msgDialog(boxMsg, false);
		CheckButton *warnCheckbutton = NULL;

		// The index version is not as expected
		Box *pVbox = msgDialog.get_vbox();
		if (pVbox != NULL)
		{
			warnCheckbutton = manage(new CheckButton(_("Don't warn me again")));
			warnCheckbutton->set_can_focus();
			warnCheckbutton->set_relief(Gtk::RELIEF_NORMAL);
			warnCheckbutton->set_mode(true);
			warnCheckbutton->set_active(false);
			pVbox->pack_end(*warnCheckbutton, Gtk::PACK_EXPAND_WIDGET);
			warnCheckbutton->show();
		}
		msgDialog.set_title(_("Update"));
		msgDialog.set_transient_for(*this);
		msgDialog.show();
		msgDialog.run();

		// Silence the warning ?
		if ((warnCheckbutton != NULL) &&
			(warnCheckbutton->get_active() == true))
		{
			m_settings.m_warnAboutVersion = false;
		}
	}
}

//
// Destructor
//
MainWindow::~MainWindow()
{
#ifdef DEBUG
	clog << "MainWindow::~MainWindow: called" << endl;
#endif
	// Don't delete m_pSettingsMonitor and m_pSettingsHandler, threads may need them
	// Since MainWindow is destroyed when the program exits, it's a leak we can live with
}

void MainWindow::setQueryTerms(const ustring &queryTerms)
{
	if (queryTerms.empty() == true)
	{
		return;
	}

	QueryProperties queryProps(_("Live query"), queryTerms);

	liveQueryEntry->set_text(queryTerms);
	queryProps.setMaximumResultsCount(m_maxResultsCount);

	run_search(queryProps);
}

//
// Load user-defined queries
//
void MainWindow::populate_queryTreeview(const string &selectedQueryName)
{
	QueryHistory queryHistory(m_settings.getHistoryDatabaseName());
	const std::map<string, QueryProperties> &queries = m_settings.getQueries();

	// Reset the whole tree
	queryTreeview->get_selection()->unselect_all();
	m_refQueryTree->clear();

	// Add all user-defined queries
	for (std::map<string, QueryProperties>::const_iterator queryIter = queries.begin();
		queryIter != queries.end(); ++queryIter)
	{
		TreeModel::iterator iter = m_refQueryTree->append();
		TreeModel::Row row = *iter;
		set<time_t> latestRuns;
		string queryName(queryIter->first);
		ustring lastRun;
		time_t lastRunTime = 0;

		if ((queryHistory.getLatestRuns(queryName, "", 1, latestRuns) == false) ||
			(latestRuns.empty() == true))
		{
			lastRun = _("N/A");
		}
		else
		{
			lastRunTime = *(latestRuns.begin());
			lastRun = to_utf8(TimeConverter::toTimestamp(lastRunTime));
		}

		row[m_queryColumns.m_name] = to_utf8(queryName);
		row[m_queryColumns.m_lastRun] = lastRun;
		row[m_queryColumns.m_lastRunTime] = lastRunTime;
		ustring summary(queryIter->second.getFreeQuery());
		if (summary.empty() == false)
		{
			row[m_queryColumns.m_summary] = summary;
		}
		else
		{
			row[m_queryColumns.m_summary] = _("Undefined");
		}
		row[m_queryColumns.m_properties] = queryIter->second;

		if ((selectedQueryName.empty() == false) &&
			(queryName == selectedQueryName))
		{
			// Select and scroll to this query
			TreeModel::Path queryPath = m_refQueryTree->get_path(iter);
			queryTreeview->get_selection()->select(queryPath);
			queryTreeview->scroll_to_row(queryPath, 0.5);
		}
	}
}

//
// Store defined queries into the settings object
//
void MainWindow::save_queryTreeview()
{
	// Clear the current queries
	m_settings.clearQueries();

	// Go through the query tree
	TreeModel::Children children = m_refQueryTree->children();
	if (children.empty() == false)
	{
		for (TreeModel::Children::iterator iter = children.begin();
			iter != children.end(); ++iter)
		{
			TreeModel::Row row = *iter;

			// Add this query to the settings
			string name = from_utf8(row[m_queryColumns.m_name]);
			QueryProperties queryProps = row[m_queryColumns.m_properties];
#ifdef DEBUG
			clog << "MainWindow::save_queryTreeview: " << name << endl;
#endif
			m_settings.addQuery(queryProps);
		}
	}
}

void MainWindow::populate_menuItems()
{
	Gtk::MenuItem *separator7 = NULL;
	Gtk::Image *image1007 = Gtk::manage(new class Gtk::Image(Gtk::StockID("gtk-go-up"), Gtk::IconSize(1)));
	Gtk::MenuItem *separator8 = NULL;
	Gtk::Image *image1008 = Gtk::manage(new class Gtk::Image(Gtk::StockID("gtk-add"), Gtk::IconSize(1)));
	Gtk::Image *image1009 = Gtk::manage(new class Gtk::Image(Gtk::StockID("gtk-refresh"), Gtk::IconSize(1)));
	Gtk::Image *image1010 = Gtk::manage(new class Gtk::Image(Gtk::StockID("gtk-delete"), Gtk::IconSize(1)));
	Gtk::MenuItem *separator9 = NULL;
	Gtk::MenuItem *separator10 = NULL;
	Gtk::Menu *fileMenuitem_menu = Gtk::manage(new class Gtk::Menu());
	Gtk::MenuItem *separator5 = NULL;
	Gtk::Menu *editMenuitem_menu = Gtk::manage(new class Gtk::Menu());
	Gtk::RadioMenuItem::Group _RadioMIGroup_searchenginegroup1;
	Gtk::Menu *groupresults1_menu = Gtk::manage(new class Gtk::Menu());
	Gtk::MenuItem *separator4 = NULL;
	Gtk::Image *image1011 = Gtk::manage(new class Gtk::Image(Gtk::StockID("gtk-open"), Gtk::IconSize(1)));
	Gtk::Image *image1012 = Gtk::manage(new class Gtk::Image(Gtk::StockID("gtk-save-as"), Gtk::IconSize(1)));
	Gtk::MenuItem *separator6 = NULL;
	Gtk::Image *image1013 = Gtk::manage(new class Gtk::Image(Gtk::StockID("gtk-info"), Gtk::IconSize(1)));
	Gtk::ImageMenuItem *status1 = NULL;
	Gtk::Menu *optionsMenuitem_menu = Gtk::manage(new class Gtk::Menu());
	Gtk::MenuItem *optionsMenuitem = NULL;
	Gtk::Menu *helpMenuitem_menu = Gtk::manage(new class Gtk::Menu());

	open1 = Gtk::manage(new class Gtk::ImageMenuItem(Gtk::Stock::OPEN));
	fileMenuitem_menu->append(*open1);

	separator7 = Gtk::manage(new class Gtk::SeparatorMenuItem());
	fileMenuitem_menu->append(*separator7);

	opencache1 = Gtk::manage(new class Gtk::MenuItem(_("Open Cache")));
	fileMenuitem_menu->append(*opencache1);

	openparent1 = Gtk::manage(new class Gtk::ImageMenuItem(*image1007, _("Open Parent")));
	fileMenuitem_menu->append(*openparent1);

	separator8 = Gtk::manage(new class Gtk::SeparatorMenuItem());
	fileMenuitem_menu->append(*separator8);

	addtoindex1 = Gtk::manage(new class Gtk::ImageMenuItem(*image1008, _("_Add To Index"), true));
	fileMenuitem_menu->append(*addtoindex1);

	updateindex1 = Gtk::manage(new class Gtk::ImageMenuItem(*image1009, _("Update")));
	fileMenuitem_menu->append(*updateindex1);

	unindex1 = Gtk::manage(new class Gtk::ImageMenuItem(*image1010, _("Unindex")));
	fileMenuitem_menu->append(*unindex1);

	separator9 = Gtk::manage(new class Gtk::SeparatorMenuItem());
	fileMenuitem_menu->append(*separator9);

	morelikethis1 = Gtk::manage(new class Gtk::MenuItem(_("More Like This")));
	fileMenuitem_menu->append(*morelikethis1);

	searchthisfor1 = Gtk::manage(new class Gtk::MenuItem(_("Search This For")));
	fileMenuitem_menu->append(*searchthisfor1);

	separator10 = Gtk::manage(new class Gtk::SeparatorMenuItem());
	fileMenuitem_menu->append(*separator10);

	properties1 = Gtk::manage(new class Gtk::ImageMenuItem(Gtk::Stock::PROPERTIES));
	fileMenuitem_menu->append(*properties1);

	separatormenuitem1 = Gtk::manage(new class Gtk::SeparatorMenuItem());
	fileMenuitem_menu->append(*separatormenuitem1);

	quit1 = Gtk::manage(new class Gtk::ImageMenuItem(Gtk::Stock::QUIT));
	fileMenuitem_menu->append(*quit1);

	cut1 = Gtk::manage(new class Gtk::ImageMenuItem(Gtk::Stock::CUT));
	editMenuitem_menu->append(*cut1);

	copy1 = Gtk::manage(new class Gtk::ImageMenuItem(Gtk::Stock::COPY));
	editMenuitem_menu->append(*copy1);

	paste1 = Gtk::manage(new class Gtk::ImageMenuItem(Gtk::Stock::PASTE));
	editMenuitem_menu->append(*paste1);

	delete1 = Gtk::manage(new class Gtk::ImageMenuItem(Gtk::Stock::DELETE));
	editMenuitem_menu->append(*delete1);

	separator5 = Gtk::manage(new class Gtk::SeparatorMenuItem());
	editMenuitem_menu->append(*separator5);

	preferences1 = Gtk::manage(new class Gtk::ImageMenuItem(Gtk::Stock::PREFERENCES));
	editMenuitem_menu->append(*preferences1);

	searchenginegroup1 = Gtk::manage(new class Gtk::RadioMenuItem(_RadioMIGroup_searchenginegroup1, _("Search Engine")));
	groupresults1_menu->append(*searchenginegroup1);

	hostnamegroup1 = Gtk::manage(new class Gtk::RadioMenuItem(_RadioMIGroup_searchenginegroup1, _("Host Name")));
	groupresults1_menu->append(*hostnamegroup1);

	listcontents1 = Gtk::manage(new class Gtk::MenuItem(_("List Contents Of")));
	optionsMenuitem_menu->append(*listcontents1);

	groupresults1 = Gtk::manage(new class Gtk::MenuItem(_("Group Results By")));
	groupresults1->set_submenu(*groupresults1_menu);
	optionsMenuitem_menu->append(*groupresults1);

	showextracts1 = Gtk::manage(new class Gtk::CheckMenuItem(_("Show Extracts")));
	optionsMenuitem_menu->append(*showextracts1);

	separator4 = Gtk::manage(new class Gtk::SeparatorMenuItem());
	optionsMenuitem_menu->append(*separator4);

	import1 = Gtk::manage(new class Gtk::ImageMenuItem(*image1011, _("Import URL")));
	optionsMenuitem_menu->append(*import1);

	export1 = Gtk::manage(new class Gtk::ImageMenuItem(*image1012, _("Export List")));
	optionsMenuitem_menu->append(*export1);

	separator6 = Gtk::manage(new class Gtk::SeparatorMenuItem());
	optionsMenuitem_menu->append(*separator6);

	status1 = Gtk::manage(new class Gtk::ImageMenuItem(*image1013, _("Status")));
	optionsMenuitem_menu->append(*status1);

	about1 = Gtk::manage(new class Gtk::ImageMenuItem(Gtk::Stock::ABOUT));
	helpMenuitem_menu->append(*about1);

	fileMenuitem = Gtk::manage(new class Gtk::MenuItem(_("_File"), true));
	fileMenuitem->set_submenu(*fileMenuitem_menu);
	mainMenubar->append(*fileMenuitem);

	editMenuitem = Gtk::manage(new class Gtk::MenuItem(_("_Edit"), true));
	editMenuitem->set_submenu(*editMenuitem_menu);
	mainMenubar->append(*editMenuitem);

	optionsMenuitem = Gtk::manage(new class Gtk::MenuItem(_("_Options"), true));
	optionsMenuitem->set_submenu(*optionsMenuitem_menu);
	mainMenubar->append(*optionsMenuitem);

	helpMenuitem = Gtk::manage(new class Gtk::MenuItem(_("_Help"), true));
	helpMenuitem->set_submenu(*helpMenuitem_menu);
	mainMenubar->append(*helpMenuitem);

	image1007->set_alignment(0.5,0.5);
	image1007->set_padding(0,0);
	image1008->set_alignment(0.5,0.5);
	image1008->set_padding(0,0);
	image1009->set_alignment(0.5,0.5);
	image1009->set_padding(0,0);
	image1010->set_alignment(0.5,0.5);
	image1010->set_padding(0,0);
	searchenginegroup1->set_active(true);
	hostnamegroup1->set_active(false);
	showextracts1->set_active(true);
	image1011->set_alignment(0.5,0.5);
	image1011->set_padding(0,0);
	image1012->set_alignment(0.5,0.5);
	image1012->set_padding(0,0);
	image1013->set_alignment(0.5,0.5);
	image1013->set_padding(0,0);

	mainMenubar->show();
	mainVbox->pack_start(*mainMenubar, Gtk::PACK_SHRINK, 0);
	mainVbox->pack_start(*mainHpaned, Gtk::PACK_EXPAND_WIDGET, 4);
	mainVbox->pack_start(*mainHbox, Gtk::PACK_SHRINK, 0);

	open1->show();
	separator7->show();
	opencache1->show();
	image1007->show();
	openparent1->show();
	separator8->show();
	image1008->show();
	addtoindex1->show();
	image1009->show();
	updateindex1->show();
	image1010->show();
	unindex1->show();
	separator9->show();
	morelikethis1->show();
	searchthisfor1->show();
	separator10->show();
	properties1->show();
	separatormenuitem1->show();
	quit1->show();
	fileMenuitem->show();
	cut1->show();
	copy1->show();
	paste1->show();
	delete1->show();
	separator5->show();
	preferences1->show();
	editMenuitem->show();
	listcontents1->show();
	searchenginegroup1->show();
	hostnamegroup1->show();
	groupresults1->show();
	showextracts1->show();
	separator4->show();
	image1011->show();
	import1->show();
	image1012->show();
	export1->show();
	separator6->show();
	image1013->show();
	status1->show();
	optionsMenuitem->show();
	about1->show();
	helpMenuitem->show();

	open1->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_open_activate), false);
	openparent1->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_openparent_activate), false);
	addtoindex1->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_addtoindex_activate), false);
	updateindex1->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_updateindex_activate), false);
	unindex1->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_unindex_activate), false);
	morelikethis1->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_morelikethis_activate), false);
	properties1->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_properties_activate), false);
	quit1->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_quit_activate), false);
	cut1->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_cut_activate), false);
	copy1->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_copy_activate), false);
	paste1->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_paste_activate), false);
	delete1->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_delete_activate), false);
	preferences1->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_preferences_activate), false);
	searchenginegroup1->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_groupresults_activate), false);
	showextracts1->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_showextracts_activate), false);
	import1->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_import_activate), false);
	export1->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_export_activate), false);
	status1->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_status_activate), false);
	about1->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_about_activate), false);

	cut1->set_sensitive(false);
	copy1->set_sensitive(false);
	paste1->set_sensitive(false);
	delete1->set_sensitive(false);
}

//
// Populate the cache menu
//
void MainWindow::populate_cacheMenu()
{
	bool setMenu = false;

	if (m_pCacheMenu == NULL)
	{
		m_pCacheMenu = manage(new Menu());
		setMenu = true;
	}
	else
	{
		// Clear the submenu
		// FIXME: m_pCacheMenu->unset_submenu(); ?
	}

	sigc::slot1<void, PinotSettings::CacheProvider> cacheSlot = sigc::mem_fun(*this, &MainWindow::on_cache_changed);

	if (m_settings.m_cacheProviders.empty() == true)
	{
		// Hide the submenu
		opencache1->hide();
	}
	else
	{
		// Populate the submenu
		for (vector<PinotSettings::CacheProvider>::iterator cacheIter = m_settings.m_cacheProviders.begin();
			cacheIter != m_settings.m_cacheProviders.end(); ++cacheIter)
		{
			MenuItem *pCacheMenuItem = manage(new MenuItem(cacheIter->m_name));

			// Bind the callback's parameter to the cache provider
			sigc::slot0<void> cachedDocumentsActivateSlot = sigc::bind(cacheSlot, (*cacheIter));
			pCacheMenuItem->signal_activate().connect(cachedDocumentsActivateSlot);
#if GTK_CHECK_VERSION(3, 0, 0)
			m_pCacheMenu->append(*pCacheMenuItem);
#endif
#ifdef DEBUG
			clog << "MainWindow::populate_cacheMenu: appended menuitem " << cacheIter->m_name << endl;
#endif
		}
	}

	if (setMenu == true)
	{
#if GTK_CHECK_VERSION(3, 0, 0)
		m_pCacheMenu->show_all();
#endif
		opencache1->set_submenu(*m_pCacheMenu);
		opencache1->show_all_children();
#ifdef DEBUG
		clog << "MainWindow::populate_cacheMenu: set submenu" << endl;
#endif
	}
}

//
// Populate the index menu
//
void MainWindow::populate_indexMenu()
{
	bool setMenu = false;

	if (m_pIndexMenu == NULL)
	{
		m_pIndexMenu = manage(new Menu());
		setMenu = true;
	}
	else
	{
		// Clear the submenu
		// FIXME: m_pIndexMenu->unset_submenu();
	}

	sigc::slot1<void, ustring> indexSlot = sigc::mem_fun(*this, &MainWindow::on_index_changed);

	set<PinotSettings::IndexProperties> indexes = m_settings.getIndexes();
	for (set<PinotSettings::IndexProperties>::const_iterator indexIter = indexes.begin();
		indexIter != indexes.end(); ++indexIter)
	{
		ustring indexName(indexIter->m_name);

		MenuItem *pIndexMenuItem = manage(new MenuItem(indexName));
		// Bind the callback's parameter to the index name
		sigc::slot0<void> menuItemSlot = sigc::bind(indexSlot, indexName);
		pIndexMenuItem->signal_activate().connect(menuItemSlot);
#if GTK_CHECK_VERSION(3, 0, 0)
		m_pIndexMenu->append(*pIndexMenuItem);
#endif
#ifdef DEBUG
		clog << "MainWindow::populate_indexMenu: appended menuitem " << indexIter->m_name << endl;
#endif
	}

	if (setMenu == true)
	{
#if GTK_CHECK_VERSION(3, 0, 0)
		m_pIndexMenu->show_all();
#endif
		listcontents1->set_submenu(*m_pIndexMenu);
		listcontents1->show_all_children();
#ifdef DEBUG
		clog << "MainWindow::populate_indexMenu: set submenu" << endl;
#endif
	}
}

//
// Populate the find menu
//
void MainWindow::populate_findMenu()
{
	bool setMenu = false;

	if (m_pFindMenu == NULL)
	{
		m_pFindMenu = manage(new Menu());
		setMenu = true;
	}
	else
	{
		// Clear the submenu
		// FIXME: m_pFindMenu->unset_submenu();
	}

	sigc::slot1<void, ustring> findSlot = sigc::mem_fun(*this, &MainWindow::on_searchthis_changed);

	TreeModel::Children children = m_refQueryTree->children();
	for (TreeModel::Children::iterator iter = children.begin();
		iter != children.end(); ++iter)
	{
		TreeModel::Row row = *iter;
		ustring queryName(row[m_queryColumns.m_name]);

		MenuItem *pFindMenuItem = manage(new MenuItem(queryName));
		// Bind the callback's parameter to the query name
		sigc::slot0<void> menuItemSlot = sigc::bind(findSlot, queryName);
		pFindMenuItem->signal_activate().connect(menuItemSlot);
#if GTK_CHECK_VERSION(3, 0, 0)
		m_pFindMenu->append(*pFindMenuItem);
#endif
#ifdef DEBUG
		clog << "MainWindow::populate_findMenu: appended menuitem " << queryName << endl;
#endif
	}

	if (setMenu == true)
	{
#if GTK_CHECK_VERSION(3, 0, 0)
		m_pFindMenu->show_all();
#endif
		searchthisfor1->set_submenu(*m_pFindMenu);
		searchthisfor1->show_all_children();
#ifdef DEBUG
		clog << "MainWindow::populate_findMenu: set submenu" << endl;
#endif
	}
}

//
// Add a query
//
void MainWindow::add_query(QueryProperties &queryProps, bool mergeQueries)
{
	ustring queryName(queryProps.getName());

	// Does such a query already exist ?
	TreeModel::Children children = m_refQueryTree->children();
	for (TreeModel::Children::iterator iter = children.begin();
		iter != children.end(); ++iter)
	{
		TreeModel::Row row = *iter;

		if (queryName == from_utf8(row[m_queryColumns.m_name]))
		{
			if (mergeQueries == true)
			{
				QueryProperties existingQueryProps = row[m_queryColumns.m_properties];
				ustring freeQuery(existingQueryProps.getFreeQuery());

				// Only merge if the new bit isn't in already
				if (freeQuery.find(queryProps.getFreeQuery()) == string::npos)
				{
					queryProps.setFreeQuery("( " + freeQuery + " ) OR ( " + queryProps.getFreeQuery() + " )");
				}
				else
				{
					// Leave the query as it is
					return;
				}
			}
			m_settings.removeQuery(queryName);
			break;
		}
	}

	// Update everything
	if (m_settings.addQuery(queryProps) == true)
	{
		populate_queryTreeview(queryName);
		queryExpander->set_expanded(true);
		populate_findMenu();
	}	
}

//
// Get selected results in the current results tab
//
bool MainWindow::get_results_page_details(const ustring &queryName,
	QueryProperties &queryProps, set<string> &locations,
	set<string> &locationsToIndex)
{
	vector<DocumentInfo> resultsList;
	ustring currentQueryName(queryName);
	bool foundQuery = false;

	locations.clear();

	if (currentQueryName.empty() == true)
	{
		NotebookPageBox *pNotebookPage = get_current_page();
		if (pNotebookPage != NULL)
		{
			ResultsTree *pResultsTree = pNotebookPage->getTree();
			if (pResultsTree != NULL)
			{
				pResultsTree->getSelectedResults(resultsList);
			}

			NotebookPageBox::PageType type = pNotebookPage->getType();
			if (type == NotebookPageBox::RESULTS_PAGE)
			{
				currentQueryName = pNotebookPage->getTitle();

				if (currentQueryName.empty() == true)
				{
					return false;
				}
			}
			else if (type == NotebookPageBox::INDEX_PAGE)
			{
				IndexPage *pIndexPage = dynamic_cast<IndexPage*>(pNotebookPage);
				if (pIndexPage != NULL)
				{
					currentQueryName = pIndexPage->getQueryName();
				}
			}
		}
	}

	// Find this query
	if (currentQueryName == _("Live query"))
	{
		queryProps.setName(currentQueryName);
		queryProps.setFreeQuery(liveQueryEntry->get_text());
	}
	else if (currentQueryName.empty() == false)
	{
		TreeModel::Children children = m_refQueryTree->children();
		for (TreeModel::Children::iterator iter = children.begin();
			iter != children.end(); ++iter)
		{
			TreeModel::Row row = *iter;

			if (currentQueryName == from_utf8(row[m_queryColumns.m_name]))
			{
				queryProps = row[m_queryColumns.m_properties];
				foundQuery = true;
				break;
			}
		}

		if (foundQuery == false)
		{
			queryProps.setName(currentQueryName);
			return false;
		}
	}

	for (vector<DocumentInfo>::const_iterator docIter = resultsList.begin();
		docIter != resultsList.end(); ++docIter)
	{
		if (docIter->getIsIndexed() == false)
		{
			locationsToIndex.insert(docIter->getLocation(true));
		}
		locations.insert(docIter->getLocation(true));
	}

	if (foundQuery == false)
	{
		Url urlObj(*locations.begin());
		string queryName(urlObj.getFile());

		if (locations.size() > 1)
		{
			queryName += "...";
		}

		queryProps.setName(queryName);
		queryProps.setFreeQuery("dir:/");
	}

	return true;
}

//
// Drag and drop
//
void MainWindow::on_data_received(const RefPtr<DragContext> &context,
	int x, int y, const SelectionData &data, guint info, guint dataTime)
{
	vector<ustring> droppedUris = data.get_uris();
	ustring droppedText(data.get_text());
	bool goodDrop = false;

	clog << "MainWindow::on_data_received: data type "
		<< data.get_data_type() << " in format " << data.get_format() << endl;

	if (droppedUris.empty() == false)
	{
		IndexInterface *pIndex = m_settings.getIndex("MERGED");
		set<string> locationsToIndex;
		ExpandSet expandSet;
		string queryName;

		expandSet.m_queryProps.setFreeQuery("dir:/");

		for (vector<ustring>::iterator uriIter = droppedUris.begin();
			uriIter != droppedUris.end(); ++uriIter)
		{
			string uri(*uriIter);

			clog << "MainWindow::on_data_received: received " << uri << endl; 

			// Query the merged index
			if (pIndex != NULL)
			{
				// Has this been indexed ?
				unsigned int docId = pIndex->hasDocument(uri);
				if (docId == 0)
				{
					locationsToIndex.insert(uri);
				}
			}

			// Name the query after the first document
			if (queryName.empty() == true)
			{
				Url urlObj(uri);
				queryName = urlObj.getFile();
			}

			expandSet.m_locations.insert(uri);
			goodDrop = true;
		}

		if (pIndex != NULL)
		{
			delete pIndex;
		}

		if (expandSet.m_locations.size() > 1)
		{
			queryName += "...";
		}

		if (goodDrop == true)
		{
			expandSet.m_queryProps.setName(queryName);
			m_expandSets.push_back(expandSet);

			for (set<string>::iterator locationIter = locationsToIndex.begin();
				locationIter != locationsToIndex.end(); ++locationIter)
			{
				DocumentInfo docInfo("", *locationIter, "", "");

				// Add this to My Web Pages
				m_state.queue_index(docInfo);
			}

			if (locationsToIndex.empty() == true)
			{
				// Expand on these documents right away
				expand_locations();
			}
			// Else, do it when indexing is completed
		}
	}
	else if (droppedText.empty() == false)
	{
		QueryProperties queryProps("", from_utf8(droppedText));
		edit_query(queryProps, true);

		clog << "MainWindow::on_data_received: received text \""
			<< droppedText << "\"" << endl;

		goodDrop = true;
	}

	context->drag_finish(goodDrop, false, dataTime);
}

//
// Engines tree selection changed
//
void MainWindow::on_enginesTreeviewSelection_changed()
{
	RefPtr<TreeSelection> refSelection = m_pEnginesTree->get_selection();
	vector<TreeModel::Path> selectedEngines = refSelection->get_selected_rows();

	// If there are more than one row selected, don't bother
	if (selectedEngines.size() != 1)
	{
		return;
	}

	vector<TreeModel::Path>::iterator enginePath = selectedEngines.begin();

	if (enginePath == selectedEngines.end())
	{
		return;
	}

	TreeModel::iterator engineIter = m_pEnginesTree->getIter(*enginePath);
	bool enableRemoveIndex = false;

	// Make sure it's a leaf node
	if (engineIter->children().empty() == true)
	{
		TreeModel::Row engineRow = *engineIter;

		// Is it an external index ?
		EnginesModelColumns &enginesColumns = m_pEnginesTree->getColumnRecord();
		EnginesModelColumns::EngineType engineType = engineRow[enginesColumns.m_type];
		if (engineType == EnginesModelColumns::INDEX_ENGINE)
		{
			// Yes, enable the remove index button
			enableRemoveIndex = true;
		}
	}
	removeIndexButton->set_sensitive(enableRemoveIndex);
}

//
// Query list selection changed
//
void MainWindow::on_queryTreeviewSelection_changed()
{
	bool enableButtons = false;

	TreeModel::iterator iter = queryTreeview->get_selection()->get_selected();
	// Anything selected ?
	if (iter)
	{
		// Enable all buttons
		enableButtons = true;
	}

	removeQueryButton->set_sensitive(enableButtons);
	queryHistoryButton->set_sensitive(enableButtons);
	findQueryButton->set_sensitive(enableButtons);
}

void MainWindow::on_resultsTreeviewSelection_changed(ustring queryName)
{
	vector<DocumentInfo> resultsList;
	bool hasSelection = false;

	NotebookPageBox *pNotebookPage = get_page(queryName, NotebookPageBox::RESULTS_PAGE);
	if (pNotebookPage != NULL)
	{
		ResultsTree *pResultsTree = pNotebookPage->getTree();
		if (pResultsTree != NULL)
		{
			hasSelection = pResultsTree->getSelectedResults(resultsList);
		}
	}

	on_document_changed(resultsList, false, false, true);
}

void MainWindow::on_indexTreeviewSelection_changed(ustring indexName)
{
	vector<DocumentInfo> resultsList;
	bool hasSelection = false;

	NotebookPageBox *pNotebookPage = get_page(indexName, NotebookPageBox::INDEX_PAGE);
	if (pNotebookPage != NULL)
	{
		ResultsTree *pResultsTree = pNotebookPage->getTree();
		if (pResultsTree != NULL)
		{
			hasSelection = pResultsTree->getSelectedResults(resultsList);
		}
	}

	bool isDocumentsIndex = true;

	if (indexName != _("My Web Pages"))
	{
		isDocumentsIndex = false;
	}

	on_document_changed(resultsList, isDocumentsIndex, true, false);
}

void MainWindow::on_document_changed(vector<DocumentInfo> &resultsList,
	bool isDocumentsIndex, bool editDocuments, bool areResults)
{
	if (resultsList.empty() == false)
	{
		bool firstResult = true, isViewable = true, isCached = false, isLocal = false, isIndexed = false, isIndexable = true;

		for (vector<DocumentInfo>::iterator resultIter = resultsList.begin();
			resultIter != resultsList.end(); ++resultIter)
		{
			string url(resultIter->getLocation(true));
			Url urlObj(url);
			string protocol(urlObj.getProtocol());

#ifdef DEBUG
			clog << "MainWindow::on_document_changed: " << url << endl;
#endif
			if (firstResult == true)
			{
				// Show the URL of the first document in the status bar
				ustring statusText = _("Document location is");
				statusText += " ";
				statusText += url;
				set_status(statusText, true);
				firstResult = false;
			}

			// Is this protocol specific to a backend ? This can't be viewed/indexed
			if (ModuleFactory::isSupported(protocol, true) == true)
			{
				isViewable = isCached = isIndexed = isIndexable = false;
				break;
			}
			else
			{
				if (m_settings.m_cacheProtocols.find(protocol) != m_settings.m_cacheProtocols.end())
				{
					// One document with a supported protocol is good enough
					isCached = true;
				}

				if (urlObj.isLocal() == true)
				{
					isLocal = true;
				}

				isIndexed = resultIter->getIsIndexed();
				isIndexable = !isIndexed;
			}
		}

		// Enable these menu items
		open1->set_sensitive(isViewable);
		opencache1->set_sensitive(isCached);
		openparent1->set_sensitive(isLocal);
		addtoindex1->set_sensitive(isIndexable);
		updateindex1->set_sensitive(isDocumentsIndex);
		unindex1->set_sensitive(isDocumentsIndex);
		morelikethis1->set_sensitive(true);
		searchthisfor1->set_sensitive(areResults);
		properties1->set_sensitive(editDocuments);
	}
	else
	{
		// Disable these menu items
		open1->set_sensitive(false);
		opencache1->set_sensitive(false);
		openparent1->set_sensitive(false);
		addtoindex1->set_sensitive(false);
		updateindex1->set_sensitive(false);
		unindex1->set_sensitive(false);
		morelikethis1->set_sensitive(false);
		searchthisfor1->set_sensitive(false);
		properties1->set_sensitive(false);

		// Reset
		set_status("");
	}
}

void MainWindow::on_index_changed(ustring indexName)
{
	ResultsTree *pResultsTree = NULL;
	ustring queryName;
	bool foundPage = false;

#ifdef DEBUG
	clog << "MainWindow::on_index_changed: current index now " << indexName << endl;
#endif

	// Is there already a page for this index ?
	IndexPage *pIndexPage = dynamic_cast<IndexPage*>(get_page(indexName, NotebookPageBox::INDEX_PAGE));
	if (pIndexPage != NULL)
	{
		// Force a refresh
		pResultsTree = pIndexPage->getTree();
		if (pResultsTree != NULL)
		{
			pResultsTree->clear();
		}
		queryName = pIndexPage->getQueryName();
		foundPage = true;
	}

	if (foundPage == false)
	{
		NotebookTabBox *pTab = manage(new NotebookTabBox(indexName,
			NotebookPageBox::INDEX_PAGE));
		pTab->getCloseSignal().connect(
			sigc::mem_fun(*this, &MainWindow::on_close_page));

		// Position the index tree
		pResultsTree = manage(new ResultsTree(indexName, fileMenuitem->get_submenu(),
			ResultsTree::FLAT, m_settings));
		// Connect to the "focus" signals
		pResultsTree->connectFocusSignals(sigc::mem_fun(*this, &MainWindow::on_focus_page));
		pIndexPage = manage(new IndexPage(indexName, pResultsTree, m_settings));
		// Connect to the "changed" signal
		pResultsTree->getSelectionChangedSignal().connect(
			sigc::mem_fun(*this, &MainWindow::on_indexTreeviewSelection_changed));
		// Connect to the edit document signal
		pResultsTree->getDoubleClickSignal().connect(
			sigc::mem_fun(*this, &MainWindow::on_properties_activate));
		// Connect to the query changed signal
		pIndexPage->getQueryChangedSignal().connect(
			sigc::mem_fun(*this, &MainWindow::on_query_changed));
		// ...and to the buttons clicked signals
		pIndexPage->getBackClickedSignal().connect(
			sigc::mem_fun(*this, &MainWindow::on_indexBackButton_clicked));
		pIndexPage->getForwardClickedSignal().connect(
			sigc::mem_fun(*this, &MainWindow::on_indexForwardButton_clicked));

		// Append the page
		if (m_state.write_lock_lists() == true)
		{
			int pageNum = m_pNotebook->append_page(*pIndexPage, *pTab);
			if (pageNum >= 0)
			{
				// Allow reordering
				m_pNotebook->set_tab_reorderable(*pIndexPage);
				foundPage = true;
			}

			m_state.unlock_lists();
		}
	}

	if (foundPage == true)
	{
		browse_index(indexName, queryName, 0);
	}
}

//
// Results > View Cache menu selected
//
void MainWindow::on_cache_changed(PinotSettings::CacheProvider cacheProvider)
{
	if (cacheProvider.m_name.empty() == true)
	{
		return;
	}

	NotebookPageBox *pNotebookPage = get_current_page();
	if (pNotebookPage != NULL)
	{
		ResultsTree *pResultsTree = pNotebookPage->getTree();
		if (pResultsTree != NULL)
		{
			vector<DocumentInfo> resultsList;

			if (pResultsTree->getSelectedResults(resultsList) == true)
			{
				for (vector<DocumentInfo>::iterator resultIter = resultsList.begin();
					resultIter != resultsList.end(); ++resultIter)
				{
					string url(resultIter->getLocation(true));
					Url urlObj(url);
					string protocol(urlObj.getProtocol());

					// Is this protocol supported ?
					if (cacheProvider.m_protocols.find(protocol) == cacheProvider.m_protocols.end())
					{
						// No, it isn't... This document will be open in place
						continue;
					}

					// Rewrite the URL
					ustring location(cacheProvider.m_location);
					ustring::size_type argPos = location.find("%url0");
					if (argPos != ustring::npos)
					{
						string::size_type pos = url.find("://");
						if ((pos != string::npos) &&
							(pos + 3 < url.length()))
						{
							// URL without protocol
							location.replace(argPos, 5, url.substr(pos + 3));
						}
					}
					else
					{
						argPos = location.find("%url");
						if (argPos != ustring::npos)
						{
							// Full protocol
							location.replace(argPos, 4, url);
						}
					}

					resultIter->setLocation(location);
#ifdef DEBUG
					clog << "MainWindow::on_cache_changed: rewritten "
						<< url << " to " << location << endl;
#endif
				}

				view_documents(resultsList);

				// Update the rows right now
				pResultsTree->setSelectionState(true);
			}
		}
	}
}

//
// Results > Search This For menu selected
//
void MainWindow::on_searchthis_changed(ustring queryName)
{
	QueryProperties queryProps;
	QueryProperties currentQueryProps;
	set<string> locations, locationsToIndex;

	// Get the properties of the given query and
	// selected results on the current results page
	if ((get_results_page_details(queryName, queryProps, locations, locationsToIndex) == false) ||
		(get_results_page_details("", currentQueryProps, locations, locationsToIndex) == false))
	{
		set_status(_("Couldn't search in results"));

		return;
	}
	// FIXME: index locations that need to be indexed

	if (locations.empty() == false)
	{
		ustring queryName(queryProps.getName());

		queryProps.setName(queryName + " " + _("In Results"));
		queryProps.setModified(true);

		// Spawn new threads
		PinotSettings::IndexProperties indexProps(m_settings.getIndexPropertiesByName(_("My Documents")));
		if (indexProps.m_location.empty() == false)
		{
			start_thread(new EngineQueryThread(indexProps, queryProps, locations));
		}
		indexProps = m_settings.getIndexPropertiesByName(_("My Web Pages"));
		if (indexProps.m_location.empty() == false)
		{
			start_thread(new EngineQueryThread(indexProps, queryProps, locations));
		}
	}
}

//
// Index queries combo selection changed
//
void MainWindow::on_query_changed(ustring indexName, ustring queryName)
{
	NotebookPageBox *pNotebookPage = get_page(indexName, NotebookPageBox::INDEX_PAGE);
	if (pNotebookPage == NULL)
	{
		return;
	}

	browse_index(indexName, queryName, 0);
}

//
// Notebook page focus
//
bool MainWindow::on_focus_page(GdkEventFocus *ev)
{
	if (ev != NULL)
	{
		bool enableItems = (ev->in ? true : false);

#ifdef DEBUG
		clog << "MainWindow::on_focus_page: called with " << ev->in << endl;
#endif
		cut1->set_sensitive(false);
		copy1->set_sensitive(enableItems);
		paste1->set_sensitive(false);
		delete1->set_sensitive(false);
	}

	return false;
}

//
// Notebook page switch
//
void MainWindow::on_switch_page(Widget *pPage, guint pageNum)
{
	NotebookPageBox *pNotebookPage = dynamic_cast<NotebookPageBox*>(m_pNotebook->get_nth_page(pageNum));
	if (pNotebookPage != NULL)
	{
		NotebookPageBox::PageType type = pNotebookPage->getType();
		if (type == NotebookPageBox::RESULTS_PAGE)
		{
			// Show or hide the extract field
			ResultsTree *pResultsTree = pNotebookPage->getTree();
			if (pResultsTree != NULL)
			{
				// Sync the results tree with the menuitems
				pResultsTree->showExtract(showextracts1->get_active());
				if (searchenginegroup1->get_active() == true)
				{
					pResultsTree->setGroupMode(ResultsTree::BY_ENGINE);
				}
				else
				{
					pResultsTree->setGroupMode(ResultsTree::BY_HOST);
				}
			}
		}
#ifdef DEBUG
		clog << "MainWindow::on_switch_page: page " << pageNum << " has type " << type << endl;
#endif
	}

	show_pagebased_menuitems(true);

	// Did the page change ?
	if (m_state.m_currentPage != pageNum)
	{
		show_selectionbased_menuitems(false);
	}
	m_state.m_currentPage = pageNum;
}

//
// Notebook page closed
//
void MainWindow::on_close_page(ustring title, NotebookPageBox::PageType type)
{
	int pageDecrement = 0;

#ifdef DEBUG
	clog << "MainWindow::on_close_page: called for tab " << title << endl;
#endif
	int pageNum = get_page_number(title, type);
	if (pageNum >= 0)
	{
		if (m_state.write_lock_lists() == true)
		{
			// Remove the page
			m_pNotebook->remove_page(pageNum);

			m_state.unlock_lists();
		}
	}

	if (m_pNotebook->get_n_pages() - pageDecrement <= 0)
	{
		show_pagebased_menuitems(false);
		show_selectionbased_menuitems(false);
	}

	// Reset
	set_status("");
}

//
// End of worker thread
//
void MainWindow::on_thread_end(WorkerThread *pThread)
{
	ustring status;
	string indexedUrl;

	if (pThread == NULL)
	{
		return;
	}

	string type(pThread->getType());
	string threadStatus(pThread->getStatus());
#ifdef DEBUG
	clog << "MainWindow::on_thread_end: end of " << type << " " << pThread->getId() << endl;
#endif

	// Did the thread fail for some reason ?
	if (threadStatus.empty() == false)
	{
		// Yep, it did
		set_status(to_utf8(threadStatus));
		// Better reset that flag if an error occurred while browsing an index
		if (type == "ListerThread")
		{
			m_state.m_browsingIndex = false;
		}
		else if (type == "IndexingThread")
		{
			IndexingThread *pIndexThread = dynamic_cast<IndexingThread *>(pThread);
			if (pIndexThread != NULL)
			{
				// Get the URL we failed to index
				indexedUrl = pIndexThread->getURL();
			}
		}
	}
	// Based on type, take the appropriate action...
	else if (type == "ListerThread")
	{
		char docsCountStr[64];

		ListerThread *pListThread = dynamic_cast<ListerThread *>(pThread);
		if (pListThread == NULL)
		{
			delete pThread;
			return;
		}

		// Find the page for this index
		// It may have been closed by the user
		PinotSettings::IndexProperties indexProps(pListThread->getIndexProperties());
		IndexPage *pIndexPage = dynamic_cast<IndexPage*>(get_page(indexProps.m_name, NotebookPageBox::INDEX_PAGE));
		if (pIndexPage != NULL)
		{
			ResultsTree *pResultsTree = pIndexPage->getTree();
			if (pResultsTree != NULL)
			{
				const vector<DocumentInfo> &docsList = pListThread->getDocuments();

				// Add the documents to the tree
				pResultsTree->addResults("", docsList, "UTF-8", false);

				pIndexPage->setDocumentsCount(pListThread->getDocumentsCount());
				pIndexPage->updateButtonsState(m_maxDocsCount);

				// FIXME: fix this for RTL languages
				status = _("Showing");
				status += " ";
				snprintf(docsCountStr, 64, "%u", pIndexPage->getFirstDocument());
				status += docsCountStr;
				status += " - ";
				snprintf(docsCountStr, 64, "%u", pIndexPage->getFirstDocument() + pResultsTree->getRowsCount());
				status += docsCountStr;
				status += " ";
				status += _("of");
				status += " ";
				snprintf(docsCountStr, 64, "%u", pIndexPage->getDocumentsCount());
				status += docsCountStr;
				set_status(status);

				if (pIndexPage->getDocumentsCount() > 0)
				{
					// Refresh the tree
					pResultsTree->refresh();
				}
				m_state.m_browsingIndex = false;
			}
		}
	}
	else if (type == "QueryingThread")
	{
		set<DocumentInfo> docsToIndex;
		int pageNum = -1;

		QueryingThread *pQueryThread = dynamic_cast<QueryingThread *>(pThread);
		if (pQueryThread == NULL)
		{
			delete pThread;
			return;
		}

		bool wasCorrected = false;
		QueryProperties queryProps(pQueryThread->getQuery(wasCorrected));
		ustring queryName(queryProps.getName());
		string labelName(queryProps.getLabelName());
		ustring engineName(to_utf8(pQueryThread->getEngineName()));
		string resultsCharset(pQueryThread->getCharset());
		set<string> labels;
		const vector<DocumentInfo> &resultsList = pQueryThread->getDocuments();

		// These are the labels that need to be applied to results
		if (labelName.empty() == false)
		{
			labels.insert(labelName);
		}

		// FIXME: fix this for RTL languages
		status = _("Query");
		status += " ";
		status += queryName;
		status += " ";
		status += _("on");
		status += " ";
		status += engineName;
		status += " ";
		status += _("ended");
		set_status(status);

		// Index results ?
		QueryProperties::IndexWhat indexResults = queryProps.getIndexResults();
		if ((indexResults != QueryProperties::NOTHING) &&
			(resultsList.empty() == false))
		{
			QueryHistory queryHistory(m_settings.getHistoryDatabaseName());
			set<unsigned int> docsIds, daemonIds;
			unsigned int docsIndexId = m_settings.getIndexPropertiesByName(_("My Web Pages")).m_id;
			unsigned int daemonIndexId = m_settings.getIndexPropertiesByName(_("My Documents")).m_id;

			for (vector<DocumentInfo>::const_iterator resultIter = resultsList.begin();
				resultIter != resultsList.end(); ++resultIter)
			{
				unsigned int indexId = 0;
				unsigned int docId = resultIter->getIsIndexed(indexId);
				float oldestScore = 0;
				bool isNewResult = false;

				// Is this a new result or a new query's result ?
				if (queryHistory.hasItem(queryName, engineName, resultIter->getLocation(true), oldestScore) <= 0)
				{
#ifdef DEBUG
					clog << "MainWindow::on_thread_end: new result " << resultIter->getLocation(true) << endl;
#endif
					isNewResult = true;
				}
				
				if ((indexResults == QueryProperties::ALL_RESULTS) ||
					((indexResults == QueryProperties::NEW_RESULTS) && (isNewResult == true)))
				{
					// Don't bother indexing what's already indexed
					if (docId == 0)
					{
						// This document will be indexed or updated
						docsToIndex.insert(*resultIter);
					}
#ifdef DEBUG
					else clog << "MainWindow::on_thread_end: result " << resultIter->getLocation(true)
						<< " from index " << indexId << endl;
#endif
				}
				else if ((docId > 0) &&
					(labels.empty() == false))
				{
					// Apply this label
					if (indexId == docsIndexId)
					{
						docsIds.insert(docId);
					}
					else if (indexId == daemonIndexId)
					{
						daemonIds.insert(docId);
					}
				}
			}

			if ((docsIds.empty() == false) ||
				(daemonIds.empty() == false))
			{
				// Apply the new label to existing documents
				start_thread(new LabelUpdateThread(labels, docsIds, daemonIds, false));
			}
		}

		// Find the page for this query
		ResultsTree *pResultsTree = NULL;
		ResultsPage *pResultsPage = dynamic_cast<ResultsPage*>(get_page(queryName, NotebookPageBox::RESULTS_PAGE));
		if (pResultsPage != NULL)
		{
			pResultsTree = pResultsPage->getTree();

			pageNum = get_page_number(queryName, NotebookPageBox::RESULTS_PAGE);
		}
		else
		{
			// There is none, create one
			NotebookTabBox *pTab = manage(new NotebookTabBox(queryName,
				NotebookPageBox::RESULTS_PAGE));
			pTab->getCloseSignal().connect(
				sigc::mem_fun(*this, &MainWindow::on_close_page));

			ResultsTree::GroupByMode groupMode = ResultsTree::BY_ENGINE;
			if (searchenginegroup1->get_active() == false)
			{
				groupMode = ResultsTree::BY_HOST;
			}
			// Position the results tree
			pResultsTree = manage(new ResultsTree(queryName, fileMenuitem->get_submenu(),
				groupMode, m_settings));
			// Connect to the "focus" signals
			pResultsTree->connectFocusSignals(sigc::mem_fun(*this, &MainWindow::on_focus_page));
			pResultsPage = manage(new ResultsPage(queryName, pResultsTree,
				m_pNotebook->get_height(), m_settings));
			// Sync the results tree with the menuitems
			pResultsTree->showExtract(showextracts1->get_active());
			// Connect to the "changed" signal
			pResultsTree->getSelectionChangedSignal().connect(
				sigc::mem_fun(*this, &MainWindow::on_resultsTreeviewSelection_changed));
			// Connect to the view results signal
			pResultsTree->getDoubleClickSignal().connect(
				sigc::mem_fun(*this, &MainWindow::on_open_activate));

			// Append the page
			if (m_state.write_lock_lists() == true)
			{
				pageNum = m_pNotebook->append_page(*pResultsPage, *pTab);
				if (pageNum >= 0)
				{
					// Allow reordering
					m_pNotebook->set_tab_reorderable(*pResultsPage);
					// Switch to this new page
					m_pNotebook->set_current_page(pageNum);
				}

				m_state.unlock_lists();
			}
		}

		if ((pageNum >= 0) &&
			(pResultsTree != NULL))
		{
			// Add the results to the tree, using the current group by mode
			pResultsTree->deleteResults(engineName);
			pResultsTree->addResults(engineName, resultsList,
				resultsCharset, pQueryThread->isLive());
		}

		// Suggest the correction to the user
		if ((pResultsPage != NULL) &&
			(wasCorrected == true) &&
			(pResultsPage->appendSuggestion(queryProps.getFreeQuery()) == true))
		{
			pResultsPage->getSuggestSignal().connect(sigc::mem_fun(*this, &MainWindow::on_suggestQueryButton_clicked));
		}

		// Now that results are displayed, go ahead and index documents
		for (set<DocumentInfo>::const_iterator docIter = docsToIndex.begin();
			docIter != docsToIndex.end(); ++docIter)
		{
			DocumentInfo docInfo(*docIter);

			// Set/reset labels
			docInfo.setLabels(labels);
#ifdef DEBUG
			clog << "MainWindow::on_thread_end: indexing results with label " << labelName << endl;
#endif
			m_state.queue_index(docInfo);
		}
	}
	else if (type == "ExpandQueryThread")
	{
		ExpandQueryThread *pExpandThread = dynamic_cast<ExpandQueryThread *>(pThread);
		if (pExpandThread == NULL)
		{
			delete pThread;
			return;
		}

		QueryProperties queryProps = pExpandThread->getQuery();
		const set<string> &expandTerms = pExpandThread->getExpandTerms();
		string queryName(_("More Like"));
		ustring moreLike;

		if (queryProps.getName().empty() == true)
		{
			queryName += "...";
		}
		else
		{
			queryName += " ";
			queryName += queryProps.getName();
		}
		queryProps.setName(queryName);

		if (queryProps.getFreeQuery().empty() == false)
		{
			moreLike = " ";
		}

		// Add the expand terms
		for (set<string>::const_iterator termIter = expandTerms.begin();
			termIter != expandTerms.end(); ++termIter)
		{
			if (moreLike.empty() == false)
			{
				moreLike += " ";
			}
			moreLike += *termIter;
		}
		queryProps.setFreeQuery(queryProps.getFreeQuery() + moreLike);
		queryProps.setModified(true);

		add_query(queryProps, false);
	}
	else if (type == "LabelUpdateThread")
	{
		// This could refresh index lists but refreshing My Documents won't help
		// as the daemon may not have flushed the changes
		status = _("Updated label(s)");
		set_status(status);
	}
	else if (type == "DownloadingThread")
	{
		DownloadingThread *pDownloadThread = dynamic_cast<DownloadingThread *>(pThread);
		if (pDownloadThread == NULL)
		{
			delete pThread;
			return;
		}

		string url = pDownloadThread->getURL();
		const Document *pDoc = pDownloadThread->getDocument();
		if (pDoc != NULL)
		{
			off_t dataLength = 0;
			const char *pData = pDoc->getData(dataLength);

			if ((pData != NULL) &&
				(dataLength > 0))
			{
				char inTemplate[21] = "/tmp/pinottempXXXXXX";
#ifdef HAVE_MKSTEMP
				int inFd = mkstemp(inTemplate);
#else
				int inFd = -1;
				char *pInFile = mktemp(inTemplate);
				if (pInFile != NULL)
				{
					inFd = open(pInFile, O_RDONLY);
				}
#endif
				bool viewDoc = false;

				if (inFd != -1)
				{
					// Save the data into a temporary file
					if (write(inFd, (const void*)pData, dataLength) != -1)
					{
						viewDoc = true;
					}

					::close(inFd);
				}

				if (viewDoc == true)
				{
					DocumentInfo docInfo(*pDoc);
					string fileName("file://");

					fileName += inTemplate;
					docInfo.setLocation(fileName);
					docInfo.setInternalPath("");

					// View this document
					vector<DocumentInfo> documentsList;
					documentsList.push_back(docInfo);
					view_documents(documentsList);
					// FIXME: how do we know when to delete this document ?
				}

				m_temporaryFiles.insert(inTemplate);
			}
		}
	}
	else if (type == "IndexingThread")
	{
		IndexingThread *pIndexThread = dynamic_cast<IndexingThread *>(pThread);
		if (pIndexThread == NULL)
		{
			delete pThread;
			return;
		}

		// Get the URL we have just indexed
		indexedUrl = pIndexThread->getURL();

		// Get the document properties
		DocumentInfo docInfo = pIndexThread->getDocumentInfo();

		// Is the index still being shown ?
		ResultsTree *pResultsTree = NULL;
		IndexPage *pIndexPage = dynamic_cast<IndexPage*>(get_page(_("My Web Pages"), NotebookPageBox::INDEX_PAGE));
		if (pIndexPage != NULL)
		{
			pResultsTree = pIndexPage->getTree();
		}

		// Did the thread perform an update ?
		if (pIndexThread->isNewDocument() == false)
		{
			// Yes, it did
			// FIXME: fix this for RTL languages
			status = _("Updated document");
			status += " ";
			status += to_utf8(indexedUrl);

			if (pResultsTree != NULL)
			{
				// Update the index tree
				pResultsTree->updateResult(docInfo);
			}
		}
		else
		{
			// FIXME: fix this for RTL languages
			status = _("Indexed");
			status += " ";
			status += to_utf8(indexedUrl);

			if (pResultsTree != NULL)
			{
				unsigned int rowsCount = pResultsTree->getRowsCount();

				// Refresh the index list if the last page is being displayed
				if (pIndexPage->getFirstDocument() + rowsCount == pIndexPage->getDocumentsCount()) 
				{
					browse_index(_("My Web Pages"), pIndexPage->getQueryName(),
						pIndexPage->getFirstDocument(), false);
#ifdef DEBUG
					clog << "MainWindow::on_thread_end: refreshed My Web Pages" << endl;
#endif
				}
				pIndexPage->updateButtonsState(m_maxDocsCount);
			}

			// FIXME: update the result's indexed status in results lists !
		}

		set_status(status);
	}
	else if (type == "UnindexingThread")
	{
		UnindexingThread *pUnindexThread = dynamic_cast<UnindexingThread *>(pThread);
		if (pUnindexThread == NULL)
		{
			delete pThread;
			return;
		}

		if (pUnindexThread->getDocumentsCount() > 0)
		{
			ustring indexName(_("My Web Pages"));
			status = _("Unindexed document(s)");
			set_status(status);

			IndexPage *pIndexPage = dynamic_cast<IndexPage*>(get_page(indexName, NotebookPageBox::INDEX_PAGE));
			if (pIndexPage != NULL)
			{
				unsigned int docsCount = pIndexPage->getDocumentsCount();

				--docsCount;
				pIndexPage->setDocumentsCount(docsCount);
				pIndexPage->updateButtonsState(m_maxDocsCount);

				if ((docsCount >= pIndexPage->getFirstDocument() + m_maxDocsCount))
				{
					// Refresh this page
					browse_index(indexName, pIndexPage->getQueryName(), pIndexPage->getFirstDocument());
				}
			}
		}
		// Else, stay silent
	}
	else if (type == "UpdateDocumentThread")
	{
		UpdateDocumentThread *pUpdateThread = dynamic_cast<UpdateDocumentThread *>(pThread);
		if (pUpdateThread == NULL)
		{
			delete pThread;
			return;
		}

		DocumentInfo docInfo(pUpdateThread->getDocumentInfo());

		// FIXME: fix this for RTL languages
		status = _("Updated document");
		status += " ";
		status += docInfo.getTitle();
		set_status(status);
	}
#ifdef HAVE_DBUS
	else if (type == "DaemonStatusThread")
	{
		DaemonStatusThread *pStatusThread = dynamic_cast<DaemonStatusThread*>(pThread);
		if (pStatusThread == NULL)
		{
			delete pThread;
			return;
		}

		StatisticsDialog *pStatisticsBox = NULL;
		m_refBuilder->get_widget_derived<StatisticsDialog>("statisticsDialog", pStatisticsBox);

		if (pStatisticsBox != NULL)
		{
			pStatisticsBox->getStatsSignal()(pStatusThread->m_crawledCount, pStatusThread->m_docsCount,
				pStatusThread->m_lowDiskSpace, pStatusThread->m_onBattery, pStatusThread->m_crawling);
		}
	}
#endif

	// Delete the thread
	delete pThread;

	// We might be able to run a queued action
	if (m_state.pop_queue(indexedUrl) == true)
	{
		expand_locations();
	}

	// Any threads left to return ?
	if (m_state.get_threads_count() == 0)
	{
		if (m_timeoutConnection.connected() == true)
		{
			m_timeoutConnection.block();
			m_timeoutConnection.disconnect();
			mainProgressbar->set_fraction(0.0);
		}
	}
}

//
// Message reception by EnginesTree
//
void MainWindow::on_editindex(ustring indexName, ustring location)
{
	IndexDialog *pIndexBox = NULL;
	m_refBuilder->get_widget_derived<IndexDialog>("indexDialog", pIndexBox);
#ifdef DEBUG
	clog << "MainWindow::on_editindex: called" << endl;
#endif

	if (pIndexBox == NULL)
	{
		return;
	}

	pIndexBox->setNameAndLocation(indexName, location);
	pIndexBox->show();
	if (pIndexBox->run() != RESPONSE_OK)
	{
		return;
	}

	if ((indexName != pIndexBox->getName()) ||
		(location != pIndexBox->getLocation()))
	{
		ustring newName(pIndexBox->getName());
		ustring newLocation(pIndexBox->getLocation());

		// Is the name okay ?
		if (pIndexBox->badName() == true)
		{
			ustring statusText = _("Couldn't rename index, name");
			statusText += " ";
			statusText += newName;
			statusText += " ";
			statusText +=  _("is already in use");

			// Tell user name is bad
			set_status(statusText);
			return;
		}

		// The only way to edit an index right now is to remove it
		// first, then add it again
		if ((m_settings.removeIndex(m_settings.getIndexPropertiesByName(indexName)) == false) ||
			(m_settings.addIndex(newName, from_utf8(newLocation)) == false))
		{
			ustring statusText = _("Couldn't rename index");
			statusText += " ";
			statusText += indexName;

			// An error occurred
			set_status(statusText);
			return;
		}

		// Refresh the engines list
		m_pEnginesTree->populate();
		// ...and the index menu
		populate_indexMenu();
	}

	set_status(_("Edited index"));
}

//
// Suggest query button click
//
void MainWindow::on_suggestQueryButton_clicked(ustring queryName, ustring queryText)
{
	QueryProperties queryProps;

	// Find the query
	const std::map<string, QueryProperties> &queriesMap = m_settings.getQueries();
	std::map<string, QueryProperties>::const_iterator queryIter = queriesMap.find(queryName);
	if (queryIter != queriesMap.end())
	{
		queryProps = queryIter->second;
		queryProps.setName("");
	}

	queryProps.setFreeQuery(queryText);
	queryProps.setModified(true);

	edit_query(queryProps, true);
}

//
// Index back button click
//
void MainWindow::on_indexBackButton_clicked(ustring indexName)
{
	IndexPage *pIndexPage = dynamic_cast<IndexPage*>(get_page(indexName, NotebookPageBox::INDEX_PAGE));
	if (pIndexPage != NULL)
	{
		if (pIndexPage->getFirstDocument() >= m_maxDocsCount)
		{
			browse_index(indexName, pIndexPage->getQueryName(), pIndexPage->getFirstDocument() - m_maxDocsCount);
		}
	}
}

//
// Index forward button click
//
void MainWindow::on_indexForwardButton_clicked(ustring indexName)
{
	IndexPage *pIndexPage = dynamic_cast<IndexPage*>(get_page(indexName, NotebookPageBox::INDEX_PAGE));
	if (pIndexPage != NULL)
	{
		if (pIndexPage->getDocumentsCount() == 0)
		{
#ifdef DEBUG
			clog << "MainWindow::on_indexForwardButton_clicked: first" << endl;
#endif
			browse_index(indexName, pIndexPage->getQueryName(), 0);
		}
		else if (pIndexPage->getDocumentsCount() >= pIndexPage->getFirstDocument() + m_maxDocsCount)
		{
#ifdef DEBUG
			clog << "MainWindow::on_indexForwardButton_clicked: next" << endl;
#endif
			browse_index(indexName, pIndexPage->getQueryName(), pIndexPage->getFirstDocument() + m_maxDocsCount);
		}
#ifdef DEBUG
		clog << "MainWindow::on_indexForwardButton_clicked: counts "
			<< pIndexPage->getFirstDocument() << " " << pIndexPage->getDocumentsCount() << endl;
#endif
	}
}

void MainWindow::on_status_activate()
{
	StatisticsDialog *pStatisticsBox = NULL;
	m_refBuilder->get_widget_derived<StatisticsDialog>("statisticsDialog", pStatisticsBox);

	if (pStatisticsBox == NULL)
	{
		return;
	}

#ifdef HAVE_DBUS
	start_thread(new DaemonStatusThread());
#endif

	pStatisticsBox->resize(250, 350);
	pStatisticsBox->show();
	pStatisticsBox->run();
}

void MainWindow::on_preferences_activate()
{
	MIMEAction prefsAction("pinot-prefs", "pinot-prefs");
	vector<string> arguments;

	// Save the settings first as any change in preferences will make the daemon reload the whole thing
	m_settings.save(PinotSettings::SAVE_CONFIG);
#ifdef DEBUG
	clog << "MainWindow::on_preferences_activate: saved config" << endl;
#endif

	CommandLine::runAsync(prefsAction, arguments);
}

void MainWindow::on_quit_activate()
{
	on_mainWindow_delete_event(NULL);
}

void MainWindow::on_cut_activate()
{
	// Copy
	on_copy_activate();
	// ...and delete
	on_delete_activate();
}

void MainWindow::on_copy_activate()
{
	ustring text;

	if (liveQueryEntry->is_focus() == true)
	{
		liveQueryEntry->copy_clipboard();
		return;
	}
	else if (queryTreeview->is_focus() == true)
	{
#ifdef DEBUG
		clog << "MainWindow::on_copy_activate: query tree" << endl;
#endif
		TreeModel::iterator iter = queryTreeview->get_selection()->get_selected();
		TreeModel::Row row = *iter;
		// Copy the query
		text = row[m_queryColumns.m_summary];
	}
	else
	{
		// The focus may be on one of the notebook pages
		NotebookPageBox *pNotebookPage = get_current_page();
		if (pNotebookPage != NULL)
		{
			vector<DocumentInfo> documentsList;
			bool firstItem = true;

			ResultsTree *pResultsTree = pNotebookPage->getTree();
			if (pResultsTree != NULL)
			{
				if (pResultsTree->focusOnExtract() == true)
				{
					// Retrieve the extract
					text = pResultsTree->getExtract();
				}
				else
				{
					// Get the current results selection
					pResultsTree->getSelectedResults(documentsList);
				}
			}

			for (vector<DocumentInfo>::const_iterator docIter = documentsList.begin();
				docIter != documentsList.end(); ++docIter)
			{
				string location(docIter->getLocation());

				if (firstItem == false)
				{
					text += "\n";
				}
				text += docIter->getTitle();
				if (location.empty() == false)
				{
					text += " ";
					text += location;
				}
				firstItem = false;
			}
		}

		if (text.empty() == true)
		{
			// Only rows from the query, results and index trees can be copied
#ifdef DEBUG
			clog << "MainWindow::on_copy_activate: other" << endl;
#endif
			return;
		}
	}
	
	RefPtr<Clipboard> refClipboard = Clipboard::get();
	if (refClipboard)
	{
		refClipboard->set_text(text);
	}
}

void MainWindow::on_paste_activate()
{
	RefPtr<Clipboard> refClipboard = Clipboard::get();

	if (!refClipboard)
	{
		return;
	}

	if (refClipboard->wait_is_text_available() == false)
	{
		return;
	}

	ustring clipText = refClipboard->wait_for_text();
	if (liveQueryEntry->is_focus() == true)
	{
		int currentPosition = liveQueryEntry->get_position();

		// Paste where the cursor is
		liveQueryEntry->insert_text(clipText, clipText.length(), currentPosition);
	}
	else if (queryTreeview->is_focus() == true)
	{
#ifdef DEBUG
		clog << "MainWindow::on_paste_activate: query tree" << endl;
#endif
		// Use whatever text is in the clipboard as free query
		QueryProperties queryProps("", from_utf8(clipText));
		edit_query(queryProps, true);
	}
	// Only the query tree can be pasted into, others are read-only
}

void MainWindow::on_delete_activate()
{
	if (liveQueryEntry->is_focus() == true)
	{
		liveQueryEntry->delete_selection();
	}
	// Nothing else can be deleted
}

void MainWindow::on_showextracts_activate()
{
	NotebookPageBox *pNotebookPage = get_current_page();
	if (pNotebookPage != NULL)
	{
		ResultsPage *pResultsPage = dynamic_cast<ResultsPage*>(pNotebookPage);
		if (pResultsPage != NULL)
		{
			ResultsTree *pResultsTree = pResultsPage->getTree();
			if (pResultsTree != NULL)
			{
				pResultsTree->showExtract(showextracts1->get_active());
			}
		}
	}
}

void MainWindow::on_groupresults_activate()
{
	// What's the new grouping criteria ?
	NotebookPageBox *pNotebookPage = get_current_page();
	if (pNotebookPage != NULL)
	{
		ResultsPage *pResultsPage = dynamic_cast<ResultsPage*>(pNotebookPage);
		if (pResultsPage != NULL)
		{
			ResultsTree *pResultsTree = pResultsPage->getTree();
			if (pResultsTree != NULL)
			{
				if (searchenginegroup1->get_active() == true)
				{
					pResultsTree->setGroupMode(ResultsTree::BY_ENGINE);
				}
				else
				{
					pResultsTree->setGroupMode(ResultsTree::BY_HOST);
				}
			}
		}
	}
}

void MainWindow::on_export_activate()
{
	NotebookPageBox *pNotebookPage = get_current_page();
	if (pNotebookPage != NULL)
	{
		string queryName = from_utf8(pNotebookPage->getTitle());

		IndexPage *pIndexPage = dynamic_cast<IndexPage*>(get_current_page());
		if (pIndexPage != NULL)
		{
			queryName = from_utf8(pIndexPage->getQueryName());
		}

		ResultsTree *pResultsTree = pNotebookPage->getTree();
		if (pResultsTree != NULL)
		{
			FileChooserDialog fileChooser(_("Export"));
			RefPtr<FileFilter> csvFilter = FileFilter::create();
			RefPtr<FileFilter> xmlFilter = FileFilter::create();
			ustring location(m_settings.getHomeDirectory());
			bool exportToCSV = true;

			location += "/";
			location += pNotebookPage->getTitle();
			location += ".csv";
			prepare_file_chooser(fileChooser, location, false);

			// Add filters
			csvFilter->add_mime_type("application/csv");
			csvFilter->set_name("CSV");
			fileChooser.add_filter(csvFilter);
			xmlFilter->add_mime_type("application/xml");
			xmlFilter->set_name("OpenSearch response");
			fileChooser.add_filter(xmlFilter);

			fileChooser.show();
			if (fileChooser.run() == RESPONSE_OK)
			{
				// Retrieve the chosen location
				location = filename_to_utf8(fileChooser.get_filename());
				// What file format was selected ?
				RefPtr<FileFilter> pFilter = fileChooser.get_filter();
				if (pFilter->get_name() != "CSV")
				{
					exportToCSV = false;
				}

				pResultsTree->exportResults(location, queryName, exportToCSV);
			}
		}
	}
}

void MainWindow::on_morelikethis_activate()
{
	set<string> locationsToIndex;
	ExpandSet expandSet;

	// Get the current page's details
	if (get_results_page_details("", expandSet.m_queryProps, expandSet.m_locations, locationsToIndex) == false)
	{
		set_status(_("Couldn't find similar documents"));

		return;
	}
	m_expandSets.push_back(expandSet);

	for (set<string>::iterator locationIter = locationsToIndex.begin();
		locationIter != locationsToIndex.end(); ++locationIter)
	{
		DocumentInfo docInfo("", *locationIter, "", "");

		// Add this to My Web Pages
		m_state.queue_index(docInfo);
	}

	if (locationsToIndex.empty() == true)
	{
		// Expand on these documents right away
		expand_locations();
	}
	// Else, do it when indexing is completed
}

void MainWindow::on_addtoindex_activate()
{
	vector<DocumentInfo> resultsList;

	NotebookPageBox *pNotebookPage = get_current_page();
	if (pNotebookPage != NULL)
	{
		ResultsPage *pResultsPage = dynamic_cast<ResultsPage*>(pNotebookPage);
		if (pResultsPage != NULL)
		{
			ResultsTree *pResultsTree = pResultsPage->getTree();
			if (pResultsTree != NULL)
			{
				pResultsTree->getSelectedResults(resultsList, true);

				// Go through selected results
				for (vector<DocumentInfo>::const_iterator resultIter = resultsList.begin();
					resultIter != resultsList.end(); ++resultIter)
				{
#ifdef DEBUG
					clog << "MainWindow::on_addtoindex_activate: URL is " << resultIter->getLocation() << endl;
#endif
					ustring status = m_state.queue_index(*resultIter);
					if (status.empty() == false)
					{
						set_status(status);
					}
				}
			}
		}
	}
}

void MainWindow::on_import_activate()
{
	ImportDialog *pImportBox = NULL;
	m_refBuilder->get_widget_derived<ImportDialog>("importDialog", pImportBox);

	if (pImportBox == NULL)
	{
		return;
	}

	pImportBox->resetLocation();
	pImportBox->show();
	if (pImportBox->run() != RESPONSE_OK)
	{
		return;
	}

	// Anything to import ?
	const DocumentInfo &docInfo = pImportBox->getDocumentInfo();
	if (docInfo.getLocation().empty() == false)
	{
#ifdef DEBUG
		clog << "MainWindow::on_import_activate: URL is " << docInfo.getLocation() << endl;
#endif
		ustring status = m_state.queue_index(docInfo);
		if (status.empty() == false)
		{
			set_status(status);
		}
	}
}

void MainWindow::on_open_activate()
{
	NotebookPageBox *pNotebookPage = get_current_page();
	if (pNotebookPage != NULL)
	{
		ResultsTree *pResultsTree = pNotebookPage->getTree();
		bool updateSelectionState = false;

		ResultsPage *pResultsPage = dynamic_cast<ResultsPage*>(pNotebookPage);
		if (pResultsPage != NULL)
		{
			updateSelectionState = true;
		}

		if (pResultsTree != NULL)
		{
			vector<DocumentInfo> resultsList;

			if (pResultsTree->getSelectedResults(resultsList) == true)
			{
				view_documents(resultsList);

				if (updateSelectionState == true)
				{
					// We can update the rows right now
					pResultsTree->setSelectionState(true);
				}
			}
		}
	}
}

void MainWindow::on_openparent_activate()
{
	NotebookPageBox *pNotebookPage = get_current_page();
	if (pNotebookPage != NULL)
	{
		ResultsTree *pResultsTree = pNotebookPage->getTree();
		if (pResultsTree != NULL)
		{
			vector<DocumentInfo> resultsList;

			if (pResultsTree->getSelectedResults(resultsList) == true)
			{
				vector<DocumentInfo>::iterator resultIter = resultsList.begin();
				while (resultIter != resultsList.end())
				{
					string location(resultIter->getLocation());
					Url urlObj(location);
					string::size_type slashPos = location.find_last_of("/");

					if ((urlObj.isLocal() == true) &&
						(slashPos != string::npos))
					{
						location.erase(slashPos);
#ifdef DEBUG
						clog << "MainWindow::on_openparent_activate: " << location << endl;
#endif
						resultIter->setLocation(location);
						resultIter->setType("x-directory/normal");

						// Next
						++resultIter;
					}
					else
					{
						vector<DocumentInfo>::iterator nextIter = resultIter;
						++nextIter;

						resultsList.erase(resultIter);

						// Next
						resultIter = nextIter;
					}
				}

				view_documents(resultsList);
			}
		}
	}
}

void MainWindow::on_updateindex_activate()
{
	vector<DocumentInfo> documentsList;

	// Get the current documents selection
	IndexPage *pIndexPage = dynamic_cast<IndexPage*>(get_current_page());
	if (pIndexPage != NULL)
	{
		ResultsTree *pResultsTree = pIndexPage->getTree();

		if (pResultsTree != NULL)
		{
			if ((pResultsTree->getSelectedResults(documentsList) == false) ||
				(documentsList.empty() == true))
			{
				// No selection
				return;
			}
		}
	}

	for (vector<DocumentInfo>::const_iterator docIter = documentsList.begin();
		docIter != documentsList.end(); ++docIter)
	{
		unsigned int indexId = 0;
		unsigned int docId = docIter->getIsIndexed(indexId);

		if (docId == 0)
		{
			continue;
		}
#ifdef DEBUG
		clog << "MainWindow::on_updateindex_activate: URL is " << docIter->getLocation() << endl;
#endif

		// Add this action to the queue
		ustring status = m_state.queue_index(*docIter);
		if (status.empty() == false)
		{
			set_status(status);
		}
	}
}

void MainWindow::on_properties_activate()
{
	vector<DocumentInfo> documentsList;
	set<unsigned int> docIds;
	string indexName;
	int width, height;
	bool docsIndex = false, daemonIndex = false, readOnlyProps = false;

#ifdef DEBUG
	clog << "MainWindow::on_properties_activate: called" << endl;
#endif
	ResultsTree *pResultsTree = NULL;
	IndexPage *pIndexPage = dynamic_cast<IndexPage*>(get_current_page());
	if (pIndexPage != NULL)
	{
		indexName = pIndexPage->getTitle();
		pResultsTree = pIndexPage->getTree();
	}

	// Is this an internal index ?
	if (indexName == _("My Web Pages"))
	{
		docsIndex = true;
	}
	else if (indexName == _("My Documents"))
	{
		daemonIndex = true;
	}
	else
	{
		readOnlyProps = true;
	}

	PinotSettings::IndexProperties indexProps(m_settings.getIndexPropertiesByName(indexName));
	if (indexProps.m_location.empty() == true)
	{
		// FIXME: fix this for RTL languages
		ustring statusText = _("Index");
		statusText += " ";
		statusText += indexName;
		statusText += " ";
		statusText += _("doesn't exist");
		set_status(statusText);
		return;
	}

	// Get the current documents selection
	if ((pResultsTree == NULL) ||
		(pResultsTree->getSelectedResults(documentsList) == false) ||
		(documentsList.empty() == true))
	{
		// No selection
		return;
	}

	get_size(width, height);

	PropertiesDialog *pPropertiesBox = NULL;
	m_refBuilder->get_widget_derived<PropertiesDialog>("propertiesDialog", pPropertiesBox);

	if (pPropertiesBox == NULL)
	{
		return;
	}

	pPropertiesBox->setDocuments(indexProps.m_location, documentsList);
	pPropertiesBox->setHeight(height / 2);
	pPropertiesBox->show();
	if ((pPropertiesBox->run() != RESPONSE_OK) ||
		(readOnlyProps == true))
	{
		return;
	}

	documentsList = pPropertiesBox->getDocuments();
	const set<string> &labels = pPropertiesBox->getLabels();

	// Update documents properties
	for (vector<DocumentInfo>::iterator docIter = documentsList.begin();
		docIter != documentsList.end(); ++docIter)
	{
		DocumentInfo docInfo(*docIter);
		unsigned int indexId = 0;
		unsigned int docId = docInfo.getIsIndexed(indexId);
		bool updateResult = false;

		// ... only if something was modified
		if (pPropertiesBox->changedInfo() == true)
		{
			start_thread(new UpdateDocumentThread(indexProps, docId, docInfo, false));
			updateResult = true;
		}
		if (pPropertiesBox->changedLabels() == true)
		{
			docInfo.setLabels(labels);
			updateResult = true;
		}

		if (updateResult == true)
		{
			pResultsTree->updateResult(docInfo);
		}

		docIds.insert(docId);
	}

	// Apply labels
	if (pPropertiesBox->changedLabels() == true)
	{
		LabelUpdateThread *pThread = NULL;
		set<unsigned int> empty;

		// Apply the labels en masse
		if (docsIndex == true)
		{
			pThread = new LabelUpdateThread(labels, docIds, empty, true);
		}
		else if (daemonIndex == true)
		{
			pThread = new LabelUpdateThread(labels, empty, docIds, true);
		}

		if (pThread != NULL)
		{
			start_thread(pThread);
		}
	}
}

void MainWindow::on_unindex_activate()
{
	vector<DocumentInfo> documentsList;
	ustring boxMsg(_("Remove this document from the index ?"));

	ResultsTree *pResultsTree = NULL;
	IndexPage *pIndexPage = dynamic_cast<IndexPage*>(get_current_page());
	if (pIndexPage != NULL)
	{
		pResultsTree = pIndexPage->getTree();
	}

	// Get the current documents selection
	if ((pResultsTree == NULL) ||
		(pResultsTree->getSelectedResults(documentsList) == false) ||
		(documentsList.empty() == true))
	{
		return;
	}

	if (documentsList.size() > 1)
	{
		boxMsg = _("Remove these documents from the index ?");
	}

	// Ask for confirmation
	MessageDialog msgDialog(boxMsg, false, MESSAGE_QUESTION, BUTTONS_YES_NO);
	msgDialog.set_title(_("Unindex"));
	msgDialog.set_transient_for(*this);
	msgDialog.show();
	int result = msgDialog.run();
	if (result == RESPONSE_NO)
	{
		return;
	}

	// Remove these documents from the tree
	pResultsTree->deleteSelection();

	set<unsigned int> docIdList;
	for (vector<DocumentInfo>::const_iterator docIter = documentsList.begin();
		docIter != documentsList.end(); ++docIter)
	{
		unsigned int indexId = 0;
		unsigned int docId = docIter->getIsIndexed(indexId);

		if (docId > 0)
		{
			docIdList.insert(docId);
		}
	}

	if (docIdList.empty() == false)
	{
		// Queue this action
#ifdef DEBUG
		clog << "MainWindow::on_unindex_activate: " << docIdList.size() << " documents to unindex" << endl;
#endif
		start_thread(new UnindexingThread(docIdList));
	}
}

void MainWindow::on_about_activate()
{
	AboutDialog aboutBox;

	aboutBox.set_comments(_("A metasearch tool for the Free Desktop") + string(".\n") +
		_("Search the Web and your documents !"));
	aboutBox.set_copyright("(C) 2005-2022 Fabrice Colin");
	aboutBox.set_name("Pinot");
	aboutBox.set_version(VERSION);
	aboutBox.set_website("https://github.com/FabriceColin/pinot");
	aboutBox.set_website_label("https://github.com/FabriceColin/pinot");
	aboutBox.show();
	aboutBox.run();
}

//
// Activity timeout elapsed
//
bool MainWindow::on_activity_timeout()
{
	if (m_timeoutConnection.blocked() == false)
	{
		mainProgressbar->pulse();
	}
#ifdef DEBUG
	else clog << "MainWindow::on_activity_timeout: blocked" << endl;
#endif

	return true;
}

//
// Add index button click
//
void MainWindow::on_addIndexButton_clicked()
{
	IndexDialog *pIndexBox = NULL;
	m_refBuilder->get_widget_derived<IndexDialog>("indexDialog", pIndexBox);

	if (pIndexBox == NULL)
	{
		return;
	}

	pIndexBox->setNameAndLocation();
	pIndexBox->show();
	if (pIndexBox->run() != RESPONSE_OK)
	{
		return;
	}

	ustring name = pIndexBox->getName();
	ustring location = pIndexBox->getLocation();

	// Is the name okay ?
	if (pIndexBox->badName() == true)
	{
		ustring statusText = _("Index name");
		statusText += " ";
		statusText += name;
		statusText += " ";
		statusText +=  _("is already in use");

		// Tell user name is bad
		set_status(statusText);
		return;
	}

	// Add the new index
	if (m_settings.addIndex(name, from_utf8(location)) == false)
	{
		ustring statusText = _("Couldn't add index");
		statusText += " ";
		statusText += name;

		// An error occurred
		set_status(statusText);
	}
	else
	{
		// Refresh the indexes list
		removeIndexButton->set_sensitive(false);
		m_pEnginesTree->populate();
		// ...and the index menu
		populate_indexMenu();
	}

	set_status(_("Added new index"));
}

//
// Remove index button click
//
void MainWindow::on_removeIndexButton_clicked()
{
	RefPtr<TreeSelection> refSelection = m_pEnginesTree->get_selection();
	vector<TreeModel::Path> selectedEngines = refSelection->get_selected_rows();

	// If there are more than one row selected, don't bother
	if (selectedEngines.size() != 1)
	{
		return;
	}

	vector<TreeModel::Path>::iterator enginePath = selectedEngines.begin();

	if (enginePath == selectedEngines.end())
	{
		return;
	}

	TreeModel::iterator engineIter = m_pEnginesTree->getIter(*enginePath);
	TreeModel::Row engineRow = *engineIter;

	// Make sure the engine is an external index
	EnginesModelColumns &engineColumns = m_pEnginesTree->getColumnRecord();
	EnginesModelColumns::EngineType engineType = engineRow[engineColumns.m_type];
	if (engineType == EnginesModelColumns::INDEX_ENGINE)
	{
		ustring indexName(engineRow[engineColumns.m_name]);

		// Remove it
		// FIXME: ask for confirmation ?
		if (m_settings.removeIndex(m_settings.getIndexPropertiesByName(indexName)) == false)
		{
			ustring statusText = _("Couldn't remove index");
			statusText += " ";
			statusText += indexName;

			// An error occurred
			set_status(statusText);
		}
		else
		{
			// Refresh the indexes list
			removeIndexButton->set_sensitive(false);
			m_pEnginesTree->populate();
			// ...and the index menu
			populate_indexMenu();
		}
	}
}

//
// Show or hide the engines list
//
void MainWindow::on_enginesTogglebutton_toggled()
{
	if (enginesTogglebutton->get_active() == true)
	{
		leftVbox->show();
	}
	else
	{
		leftVbox->hide();
	}
}

//
// Live query entry focus
//
bool MainWindow::on_liveQueryEntry_focus(GdkEventFocus *ev)
{
	if (ev != NULL)
	{
		bool enableItems = (ev->in ? true : false);

#ifdef DEBUG
		clog << "MainWindow::on_liveQueryEntry_focus: called with " << ev->in << endl;
#endif
		cut1->set_sensitive(enableItems);
		copy1->set_sensitive(enableItems);
		paste1->set_sensitive(enableItems);
		delete1->set_sensitive(enableItems);
	}

	return false;
}

//
// Live query entry change
//
void MainWindow::on_liveQueryEntry_changed()
{
	ustring term(liveQueryEntry->get_text());
	bool enableFindButton = true;

	if (term.empty() == true)
	{
		enableFindButton = false;
	}
	liveQueryEntry->set_icon_sensitive(ENTRY_ICON_SECONDARY, enableFindButton);

	if (m_settings.m_suggestQueryTerms == false)
	{
		// Suggestions are disabled
		return;
	}

	ustring prefix;
	unsigned int liveQueryLength = term.length();

	// Reset the list
	m_refLiveQueryList->clear();
	// Get the last term from the entry field
	ustring::size_type pos = term.find_last_of(" ");
	if (pos != ustring::npos)
	{
		ustring liveQuery(term);

		prefix = liveQuery.substr(0, pos);
		prefix += " ";
		term = liveQuery.substr(pos + 1);
	}

	// If characters are being deleted, if the term is too short or
	// if it's a filter or a range, don't offer suggestions
	if ((m_state.m_liveQueryLength > liveQueryLength) ||
		(term.empty() == true) ||
		(m_refLiveQueryCompletion->get_minimum_key_length() > (int)term.length()) ||
		(term.find(':') != string::npos) ||
		(term.find("..") != string::npos))
	{
		m_state.m_liveQueryLength = liveQueryLength;
		return;
	}
	m_state.m_liveQueryLength = liveQueryLength;

	// Query the merged index
	IndexInterface *pIndex = m_settings.getIndex("MERGED");
	if (pIndex != NULL)
	{
		set<string> suggestedTerms;
		int termIndex = 0;

		// Get a list of suggestions
		pIndex->getCloseTerms(from_utf8(term), suggestedTerms);

		// Populate the list
		for (set<string>::iterator termIter = suggestedTerms.begin();
			termIter != suggestedTerms.end(); ++termIter)
		{
			TreeModel::iterator iter = m_refLiveQueryList->append();
			TreeModel::Row row = *iter;

			row[m_liveQueryColumns.m_name] = prefix + to_utf8(*termIter);
			++termIndex;
		}
#ifdef DEBUG
		clog << "MainWindow::on_liveQueryEntry_changed: " << termIndex << " suggestions" << endl;
#endif

		delete pIndex;
	}
}

//
// Live query entry activate
//
void MainWindow::on_liveQueryEntry_activate()
{
	on_findButton_clicked();
}

//
// Live query entry icon press
//
void MainWindow::on_liveQueryEntry_icon(EntryIconPosition position, const GdkEventButton *ev)
{
	on_findButton_clicked();
}

//
// Find button click
//
void MainWindow::on_findButton_clicked()
{
	QueryProperties queryProps(_("Live query"), liveQueryEntry->get_text());

	queryProps.setMaximumResultsCount(m_maxResultsCount);

	run_search(queryProps);
}

//
// Add query button click
//
void MainWindow::on_addQueryButton_clicked()
{
	// Even though live queries terms are now ANDed together,
	// use them as OR terms when creating a new stored query
	QueryProperties queryProps("", liveQueryEntry->get_text());

	edit_query(queryProps, true);
}

//
// Remove query button click
//
void MainWindow::on_removeQueryButton_clicked()
{
	TreeModel::iterator iter = queryTreeview->get_selection()->get_selected();
	// Anything selected ?
	if (iter)
	{
		TreeModel::Row row = *iter;
		string queryName = from_utf8(row[m_queryColumns.m_name]);

		if (m_settings.removeQuery(queryName) == true)
		{
			QueryHistory queryHistory(m_settings.getHistoryDatabaseName());

			// Remove records from QueryHistory
			queryHistory.deleteItems(queryName, true);

			// Select another row
			queryTreeview->get_selection()->unselect(iter);
			TreeModel::Path queryPath = m_refQueryTree->get_path(iter);
			queryPath.next();
			queryTreeview->get_selection()->select(queryPath);
			// Erase
			m_refQueryTree->erase(row);

			queryTreeview->columns_autosize();

			populate_findMenu();
		}

		if (m_state.read_lock_lists() == true)
		{
			for (int pageNum = 0; pageNum < m_pNotebook->get_n_pages(); ++pageNum)
			{
				Widget *pPage = m_pNotebook->get_nth_page(pageNum);
				if (pPage != NULL)
				{
					IndexPage *pIndexPage = dynamic_cast<IndexPage*>(pPage);
					if (pIndexPage != NULL)
					{
						// Synchronize the queries list with the new list 
						// This will trigger a list refresh
						pIndexPage->populateQueryCombobox("");
					}
				}
			}

			m_state.unlock_lists();
		}
	}
}

//
// Previous Results button click
//
void MainWindow::on_queryHistoryButton_clicked()
{
	TreeModel::iterator queryIter = queryTreeview->get_selection()->get_selected();
	// Any query selected ?
	if (queryIter)
	{
		QueryHistory queryHistory(m_settings.getHistoryDatabaseName());
		TreeModel::Row queryRow = *queryIter;
		set<string> engineNames;
		QueryProperties queryProps = queryRow[m_queryColumns.m_properties];
		ustring queryName(queryRow[m_queryColumns.m_name]);

		if ((queryHistory.getEngines(queryName, engineNames) == false) ||
			(engineNames.empty() == true))
		{
			ustring statusText = _("No history for");
			statusText += " ";
			statusText += queryName;

			set_status(statusText);
			return;
		}

		for (set<string>::const_iterator engineIter = engineNames.begin();
			engineIter != engineNames.end(); ++engineIter)
		{
			// Spawn a new thread
			start_thread(new EngineHistoryThread(*engineIter, queryProps, m_maxDocsCount));
		}
	}
}

//
// Find query button click
//
void MainWindow::on_findQueryButton_clicked()
{
	TreeModel::iterator queryIter = queryTreeview->get_selection()->get_selected();
	// Any query selected ?
	if (queryIter)
	{
		TreeModel::Row queryRow = *queryIter;
		time_t lastRunTime = time(NULL);

		QueryProperties queryProps = queryRow[m_queryColumns.m_properties];
		run_search(queryProps);

		// Update the Last Run column
		queryRow[m_queryColumns.m_lastRun] = to_utf8(TimeConverter::toTimestamp(lastRunTime));
		queryRow[m_queryColumns.m_lastRunTime] = lastRunTime;
	}
}

//
// Query list focus
//
bool MainWindow::on_queryTreeview_focus(GdkEventFocus *ev)
{
	if (ev != NULL)
	{
		bool enableItems = (ev->in ? true : false);

#ifdef DEBUG
		clog << "MainWindow::on_queryTreeview_focus: called with " << ev->in << endl;
#endif
		cut1->set_sensitive(false);
		copy1->set_sensitive(enableItems);
		paste1->set_sensitive(enableItems);
		delete1->set_sensitive(false);
	}

	return false;
}

//
// Query list mouse click
//
bool MainWindow::on_queryTreeview_button_press_event(GdkEventButton *ev)
{
	// Check for double clicks
	if (ev->type == GDK_2BUTTON_PRESS)
	{
		TreeModel::iterator iter = queryTreeview->get_selection()->get_selected();
		// Anything selected ?
		if (iter)
		{
			TreeModel::Row row = *iter;
#ifdef DEBUG
			clog << "MainWindow::on_queryTreeview_button_press_event: selected " << row[m_queryColumns.m_name] << endl;
#endif

			// Edit this query's properties
			QueryProperties queryProps(row[m_queryColumns.m_properties]);
			edit_query(queryProps, false);
		}
	}

	return false;
}

//
// Main window deleted
//
bool MainWindow::on_mainWindow_delete_event(GdkEventAny *ev)
{
	// Any thread still running ?
	if (m_state.get_threads_count() > 0)
	{
		ustring boxMsg(_("At least one task hasn't completed yet. Quit now ?"));
		MessageDialog msgDialog(boxMsg, false, MESSAGE_QUESTION, BUTTONS_YES_NO);
		msgDialog.set_title(_("Quit"));
		msgDialog.set_transient_for(*this);
		msgDialog.show();
		int result = msgDialog.run();
		if (result == RESPONSE_NO)
		{
			return true;
		}
	}

	m_state.disconnect();
	m_state.mustQuit(true);

	// Disconnect UI signals
	m_pageSwitchConnection.disconnect();

	// Save the window's position and dimensions now
	get_position(m_settings.m_xPos, m_settings.m_yPos);
	get_size(m_settings.m_width, m_settings.m_height);
	m_settings.m_panePos = mainHpaned->get_position();
	m_settings.m_expandQueries = queryExpander->get_expanded();
#ifdef DEBUG
	clog << "MainWindow::on_mainWindow_delete_event: expanded ?" << m_settings.m_expandQueries << endl;
#endif
	m_settings.m_showEngines = enginesTogglebutton->get_active();
#ifdef DEBUG
	clog << "MainWindow::on_mainWindow_delete_event: quitting" << endl;
#endif

	// Save engines
	m_pEnginesTree->save();

	// Save queries
	save_queryTreeview();

	// Save the settings
	m_settings.save(PinotSettings::SAVE_CONFIG);
#ifdef DEBUG
	clog << "MainWindow::on_mainWindow_delete_event: saved config" << endl;
#endif

	// Delete temporary files created for viewing documents
	for_each(m_temporaryFiles.begin(), m_temporaryFiles.end(), DeleteTemporaryFileFunc());

	Main::quit();

	return false;
}

//
// Show or hide menuitems.
//
void MainWindow::show_pagebased_menuitems(bool showItems)
{
	// Results menuitems that depend on the page
	export1->set_sensitive(showItems);
}

//
// Show or hide menuitems.
//
void MainWindow::show_selectionbased_menuitems(bool showItems)
{
	// Menuitems that depend on selection
	open1->set_sensitive(showItems);
	opencache1->set_sensitive(showItems);
	openparent1->set_sensitive(showItems);
	morelikethis1->set_sensitive(showItems);
	searchthisfor1->set_sensitive(showItems);
	addtoindex1->set_sensitive(showItems);
	updateindex1->set_sensitive(showItems);
	unindex1->set_sensitive(showItems);
	properties1->set_sensitive(showItems);
}

//
// Returns the current page.
//
NotebookPageBox *MainWindow::get_current_page(void)
{
	NotebookPageBox *pNotebookPage = NULL;

	if (m_state.read_lock_lists() == true)
	{
		Widget *pPage = m_pNotebook->get_nth_page(m_pNotebook->get_current_page());
		if (pPage != NULL)
		{
			pNotebookPage = dynamic_cast<NotebookPageBox*>(pPage);
		}

		m_state.unlock_lists();
	}

	return pNotebookPage;
}

//
// Returns the page with the given title.
//
NotebookPageBox *MainWindow::get_page(const ustring &title, NotebookPageBox::PageType type)
{
	NotebookPageBox *pNotebookPage = NULL;

#ifdef DEBUG
	clog << "MainWindow::get_page: looking for " << title << " " << type << endl;
#endif
	if (m_state.read_lock_lists() == true)
	{
		for (int pageNum = 0; pageNum < m_pNotebook->get_n_pages(); ++pageNum)
		{
			Widget *pPage = m_pNotebook->get_nth_page(pageNum);
			if (pPage != NULL)
			{
				pNotebookPage = dynamic_cast<NotebookPageBox*>(pPage);
				if (pNotebookPage != NULL)
				{
#ifdef DEBUG
					clog << "MainWindow::get_page: " << pNotebookPage->getTitle()
						<< " " << pNotebookPage->getType() << endl;
#endif
					if ((title == pNotebookPage->getTitle()) &&
						(type == pNotebookPage->getType()))
					{
						// That's the page we are looking for
						break;
					}
					pNotebookPage = NULL;
				}
			}
		}

		m_state.unlock_lists();
	}

	return pNotebookPage;
}

//
// Returns the number of the page with the given title.
//
int MainWindow::get_page_number(const ustring &title, NotebookPageBox::PageType type)
{
	int pageNumber = -1;

	if (m_state.read_lock_lists() == true)
	{
		for (int pageNum = 0; pageNum < m_pNotebook->get_n_pages(); ++pageNum)
		{
			Widget *pPage = m_pNotebook->get_nth_page(pageNum);
			if (pPage != NULL)
			{
				NotebookPageBox *pNotebookPage = dynamic_cast<NotebookPageBox*>(pPage);
				if (pNotebookPage != NULL)
				{
#ifdef DEBUG
					clog << "MainWindow::get_page_number: " << pNotebookPage->getTitle() << endl;
#endif
					if ((title == pNotebookPage->getTitle()) &&
						(type == pNotebookPage->getType()))
					{
						// That's the page we are looking for
						pageNumber = pageNum;
						break;
					}
				}
			}
		}

		m_state.unlock_lists();
	}

	return pageNumber;
}

//
// Edits a query
//
void MainWindow::edit_query(QueryProperties &queryProps, bool newQuery)
{
	ustring queryName;

	if (newQuery == false)
	{
		// Backup the current name
		queryName = queryProps.getName();
	}
	else
	{
		// Initialize the number of results
		queryProps.setMaximumResultsCount(m_maxResultsCount);
	}
#ifdef DEBUG
	clog << "MainWindow::edit_query: editing " << queryProps.getName() << endl;
#endif

	QueryDialog *pQueryBox = NULL;
	m_refBuilder->get_widget_derived<QueryDialog>("queryDialog", pQueryBox);

	if (pQueryBox == NULL)
	{
		return;
	}

	pQueryBox->setQueryProperties(queryProps);
	pQueryBox->show();
	if (pQueryBox->run() != RESPONSE_OK)
	{
		// Nothing to do
		return;
	}

	queryProps = pQueryBox->getQueryProperties();

	// Is the name okay ?
	if (pQueryBox->badName() == true)
	{
		ustring statusText = _("Query name");
		statusText += " ";
		statusText += queryProps.getName();
		statusText += " ";
		statusText +=  _("is already in use");

		// Tell user the name is bad
		set_status(statusText);
		return;
	}

	if (newQuery == false)
	{
		// Did the name change ?
		ustring newQueryName(queryProps.getName());
		if (newQueryName != queryName)
		{
			QueryHistory queryHistory(m_settings.getHistoryDatabaseName());

			// Remove records from QueryHistory
			queryHistory.deleteItems(queryName, true);
		}

		// Update the query properties
		if ((m_settings.removeQuery(queryName) == false) ||
			(m_settings.addQuery(queryProps) == false))
		{
			ustring statusText = _("Couldn't update query");
			statusText += " ";
			statusText += queryName;
	
			set_status(statusText);
			return;
		}

		// User edits reset the automatically modified flag
		queryProps.setModified(false);

		set_status(_("Edited query"));
	}
	else
	{
		// Add the new query
		if (m_settings.addQuery(queryProps) == false)
		{
			ustring statusText = _("Couldn't add query");
			statusText += " ";
			statusText += queryProps.getName();

			set_status(statusText);
			return;
		}

		set_status(_("Added new query"));
	}

	populate_queryTreeview(queryProps.getName());
	populate_findMenu();

	if (m_state.read_lock_lists() == true)
	{
		for (int pageNum = 0; pageNum < m_pNotebook->get_n_pages(); ++pageNum)
		{
			Widget *pPage = m_pNotebook->get_nth_page(pageNum);
			if (pPage != NULL)
			{
				IndexPage *pIndexPage = dynamic_cast<IndexPage*>(pPage);
				if (pIndexPage != NULL)
				{
					// Synchronize the queries list with the new list
					// This will trigger a list refresh
					pIndexPage->populateQueryCombobox(pIndexPage->getQueryName());
				}
			}
		}

		m_state.unlock_lists();
	}
}

//
// Runs a search
//
void MainWindow::run_search(const QueryProperties &queryProps)
{
	if (queryProps.isEmpty() == true)
	{
		set_status(_("Query is not set"));
		return;
	}
#ifdef DEBUG
	clog << "MainWindow::run_search: query name is " << queryProps.getName() << endl;
#endif

	// Check a search engine has been selected
	RefPtr<TreeSelection> refSelection = m_pEnginesTree->get_selection();
	vector<TreeModel::Path> selectedEngines = refSelection->get_selected_rows();

	if (selectedEngines.empty() == true)
	{
		set_status(_("No search engine selected"));
		return;
	}

	// Go through the tree and check selected nodes
	vector<TreeModel::iterator> engineIters;

	EnginesModelColumns &engineColumns = m_pEnginesTree->getColumnRecord();
	for (vector<TreeModel::Path>::iterator enginePath = selectedEngines.begin();
		enginePath != selectedEngines.end(); ++enginePath)
	{
		TreeModel::iterator engineIter = m_pEnginesTree->getIter(*enginePath);
		TreeModel::Row engineRow = *engineIter;
		EnginesModelColumns::EngineType engineType = engineRow[engineColumns.m_type];

		if ((refSelection->is_selected(*enginePath) == false) ||
			(engineType < EnginesModelColumns::ENGINE_FOLDER))
		{
			// Skip
			continue;
		}

		// Is it a folder ?
		if (engineType == EnginesModelColumns::ENGINE_FOLDER)
		{
			TreeModel::Children children = engineIter->children();
			for (TreeModel::Children::iterator folderEngineIter = children.begin();
				folderEngineIter != children.end(); ++folderEngineIter)
			{
				TreeModel::Row folderEngineRow = *folderEngineIter;

				EnginesModelColumns::EngineType folderEngineType = folderEngineRow[engineColumns.m_type];
				if (folderEngineType < EnginesModelColumns::ENGINE_FOLDER)
				{
					// Skip
					continue;
				}

				engineIters.push_back(folderEngineIter);
			}
		}
		else
		{
			engineIters.push_back(engineIter);
		}
	}
#ifdef DEBUG
	clog << "MainWindow::run_search: selected " << engineIters.size()
		<< " engines" << endl;
#endif

	// Now go through the selected search engines
	set<ustring> engineDisplayableNames;
	for (vector<TreeModel::iterator>::iterator iter = engineIters.begin();
		iter != engineIters.end(); ++iter)
	{
		TreeModel::Row engineRow = **iter;

		// Check whether this engine has already been done
		// Using a set<TreeModel::iterator/Row> would be preferable
		// but is not helpful here
		ustring engineDisplayableName = engineRow[engineColumns.m_name];
		if (engineDisplayableNames.find(engineDisplayableName) != engineDisplayableNames.end())
		{
			continue;
		}
		engineDisplayableNames.insert(engineDisplayableName);

		ustring engineName = engineRow[engineColumns.m_engineName];
		ustring engineOption = engineRow[engineColumns.m_option];
#ifdef DEBUG
		clog << "MainWindow::run_search: engine " << engineDisplayableName << endl;
#endif

		// FIXME: fix this for RTL languages
		ustring status = _("Running query");
		status += " \"";
		status += queryProps.getName();
		status += "\" ";
		status += _("on");
		status += " ";
		status += engineDisplayableName;
		set_status(status);

		// Spawn a new thread
		start_thread(new EngineQueryThread(from_utf8(engineName),
			from_utf8(engineDisplayableName), engineOption, queryProps));
	}
}

//
// Browse an index
//
void MainWindow::browse_index(const ustring &indexName, const ustring &queryName,
	unsigned int startDoc, bool changePage)
{
#ifdef DEBUG
	clog << "MainWindow::browse_index: called on " << indexName << ", " << queryName << endl;
#endif
	// Rudimentary lock
	if (m_state.m_browsingIndex == true)
	{
		return;
	}
	m_state.m_browsingIndex = true;

	IndexPage *pIndexPage = dynamic_cast<IndexPage*>(get_page(indexName, NotebookPageBox::INDEX_PAGE));
	if (pIndexPage != NULL)
	{
		ResultsTree *pResultsTree = pIndexPage->getTree();
		if (pResultsTree != NULL)
		{
			// Remove existing rows in the index tree
			pResultsTree->clear();
		}
		pIndexPage->setFirstDocument(startDoc);

		if (changePage == true)
		{
			// Switch to that index page
			m_pNotebook->set_current_page(get_page_number(indexName, NotebookPageBox::INDEX_PAGE));
		}
	}

	PinotSettings::IndexProperties indexProps(m_settings.getIndexPropertiesByName(indexName));
	if (indexProps.m_location.empty() == true)
	{
#ifdef DEBUG
		clog << "MainWindow::browse_index: couldn't find index " << indexName << endl;
#endif
		return;
	}

	// Spawn a new thread to browse the index
	if (queryName.empty() == true)
	{
		start_thread(new IndexBrowserThread(indexProps, m_maxDocsCount, startDoc));
	}
	else
	{
		// Find the query
		const std::map<string, QueryProperties> &queriesMap = m_settings.getQueries();
		std::map<string, QueryProperties>::const_iterator queryIter = queriesMap.find(queryName);
		if (queryIter != queriesMap.end())
		{
			QueryProperties queryProps(queryIter->second);

			// Override the query's default number of results
			queryProps.setMaximumResultsCount(m_maxDocsCount);

			// ... and the index
			start_thread(new EngineQueryThread(indexProps, queryProps, startDoc, true));
		}
#ifdef DEBUG
		else clog << "MainWindow::browse_index: couldn't find query " << queryName << endl;
#endif
	}
}

//
// View documents
//
void MainWindow::view_documents(const vector<DocumentInfo> &documentsList)
{
	ViewHistory viewHistory(m_settings.getHistoryDatabaseName());
	RefPtr<RecentManager> recentManager = RecentManager::get_default();
	multimap<string, string> locationsByType;

	for (vector<DocumentInfo>::const_iterator docIter = documentsList.begin();
		docIter != documentsList.end(); ++docIter)
	{
		string url(docIter->getLocation());
		string mimeType(docIter->getType());

#ifdef DEBUG
		clog << "MainWindow::view_documents: " << url << "?" << docIter->getInternalPath() << endl;
#endif
		if (url.empty() == true)
		{
			continue;
		}

		// FIXME: there should be a way to know which protocols can be viewed/indexed
		if (docIter->getInternalPath().empty() == false)
		{
			// Get that message
			start_thread(new DownloadingThread(*docIter));

			// Record this into the history now, even though it may fail
			if (viewHistory.hasItem(docIter->getLocation(true)) == false)
			{
				viewHistory.insertItem(docIter->getLocation(true));
			}
			continue;
		}

		// What's the MIME type ?
		if (mimeType.empty() == true)
		{
			Url urlObj(docIter->getLocation());

			// Scan for the MIME type
			mimeType = MIMEScanner::scanUrl(urlObj);
		}

#ifdef DEBUG
		clog << "MainWindow::view_documents: " << url << " has type " << mimeType << endl;
#endif
		locationsByType.insert(pair<string, string>(mimeType, url));
	}

	vector<MIMEAction> actionsList;
	vector<string> arguments;
	MIMEAction action;
	string currentType;

	arguments.reserve(documentsList.size());

	for (multimap<string, string>::iterator locationIter = locationsByType.begin();
		locationIter != locationsByType.end(); ++locationIter)
	{
		string type(locationIter->first);
		string url(locationIter->second);

		if (type != currentType)
		{
			Url urlObj(url);

			if ((action.m_exec.empty() == false) &&
				(arguments.empty() == false))
			{
				// Run the default program for this type
				if (CommandLine::runAsync(action, arguments) == false)
				{
#ifdef DEBUG
					clog << "MainWindow::view_documents: couldn't view type "
						<< currentType << endl;
#endif
				}

				// Clear the list of arguments
				arguments.clear();
			}

			// Get the action for this MIME type
			if ((urlObj.getProtocol() == "http") ||
				(urlObj.getProtocol() == "https"))
			{
				// Chances are the web browser will be able to open this
				type = "text/html";
#ifdef DEBUG
				clog << "MainWindow::view_documents: defaulting to text/html" << endl;
#endif
			}
			bool foundAction = MIMEScanner::getDefaultActions(type, urlObj.isLocal(), actionsList);
			if (foundAction == false)
			{
				if ((type.length() > 5) &&
					(type.substr(0, 5) == "text/"))
				{
					// It's a subtype of text
					// FIXME: MIMEScanner should return parent types !
					type = "text/plain";
					foundAction = MIMEScanner::getDefaultActions(type, urlObj.isLocal(), actionsList);
#ifdef DEBUG
					clog << "MainWindow::view_documents: defaulting to text/plain" << endl;
#endif
				}

				if (foundAction == false)
				{
					LauncherDialog *pLauncherBox = NULL;
					m_refBuilder->get_widget_derived<LauncherDialog>("launcherDialog", pLauncherBox);

					if (pLauncherBox != NULL)
					{
						bool remember = true;

						pLauncherBox->setUrl(url);
						pLauncherBox->show();
						if (pLauncherBox->run() == RESPONSE_OK)
						{
							foundAction = pLauncherBox->getInput(action, remember);
						}

						if (foundAction == false)
						{
							ustring statusText = _("No default application defined for type");
							statusText += " ";
							statusText += type;
							set_status(statusText);
							continue;
						}
						else if (remember == true)
						{
							// Add this to MIMESCanner's list
#ifdef DEBUG
							clog << "MainWindow::view_documents: adding user-defined action for type " << type << endl;
#endif
							MIMEScanner::addDefaultAction(type, action);
							// FIXME: save this in the settings ?
						}
					}
				}
			}

			if ((foundAction == true) &&
				(actionsList.empty() == false))
			{
				action = actionsList.front();
			}
		}
		currentType = type;

		arguments.push_back(url);

		// Record this into the history now, even though it may fail
		if (viewHistory.hasItem(url) == false)
		{
			viewHistory.insertItem(url);

			// ...as well as in the recently used files list
			RecentManager::Data itemData;
			for (vector<DocumentInfo>::const_iterator docIter = documentsList.begin();
				docIter != documentsList.end(); ++docIter)
			{
				if (docIter->getLocation() == url)
				{
					itemData.display_name = docIter->getTitle();
					break;
				}
			}
			itemData.mime_type = type;
			itemData.is_private = false;
			recentManager->add_item(url, itemData);
		}
	}

	// Run the default program for this type
	if ((action.m_exec.empty() == true) ||
		(arguments.empty() == true) ||
		(CommandLine::runAsync(action, arguments) == false))
	{
#ifdef DEBUG
		clog << "MainWindow::view_documents: couldn't view type "
			<< currentType << endl;
#endif
	}
}

//
// Start of worker thread
//
bool MainWindow::start_thread(WorkerThread *pNewThread, bool inBackground)
{
#ifdef HAVE_DBUS
	time_t flushEpoch = (time_t)m_state.m_refProxy->IndexFlushEpoch_get();

#ifdef DEBUG
	clog << "MainWindow::start_thread: index flushed at " << flushEpoch
		<< " versus " << m_state.m_flushEpoch << endl;
#endif
	if (flushEpoch > m_state.m_flushEpoch)
	{
		m_state.m_flushEpoch = flushEpoch;

		IndexInterface *pIndex = m_settings.getIndex(m_settings.m_daemonIndexLocation);
		if (pIndex != NULL)
		{
			pIndex->reopen();

			delete pIndex;
		}
	}
#endif

	if (m_state.start_thread(pNewThread, inBackground) == false)
	{
		// Delete the object
		delete pNewThread;
		return false;
	}
#ifdef DEBUG
	clog << "MainWindow::start_thread: started thread " << pNewThread->getId() << endl;
#endif

	if (inBackground == false)
	{
		// Enable the activity progress bar
		m_timeoutConnection.block();
		m_timeoutConnection.disconnect();
		m_timeoutConnection = Glib::signal_timeout().connect(sigc::mem_fun(*this,
			&MainWindow::on_activity_timeout), 1000);
		m_timeoutConnection.unblock();
	}

	return true;
}

bool MainWindow::expand_locations(void)
{
	ExpandQueryThread *pExpandQueryThread = NULL;

	// Grab the first set
	vector<ExpandSet>::iterator expandIter = m_expandSets.begin();
	if (expandIter != m_expandSets.end())
	{
		pExpandQueryThread = new ExpandQueryThread(expandIter->m_queryProps, expandIter->m_locations);
#ifdef DEBUG
		clog << "MainWindow::expand_locations: " << expandIter->m_locations.size() << " locations in set" << endl;
#endif

		m_expandSets.erase(expandIter);
	}

	if (pExpandQueryThread != NULL)
	{
		// Run a catch-all query and expand on these documents
		return start_thread(pExpandQueryThread);
	}

	return false;
}

//
// Sets the status bar text.
//
void MainWindow::set_status(const ustring &text, bool canBeSkipped)
{
	static time_t lastTime = time(NULL);

	time_t now = time(NULL);
	if ((difftime(now, lastTime) < 1) &&
		(canBeSkipped == true))
	{
		// Skip this
		return;
	}
	lastTime = now;
	
	unsigned int threadsCount = m_state.get_threads_count();
	if (threadsCount > 0)
	{
		char threadsCountStr[64];

		snprintf(threadsCountStr, 64, "%u", threadsCount);
		// Display the number of threads
		mainProgressbar->set_text(threadsCountStr);
	}
	else
	{
		// Reset
		mainProgressbar->set_text("");
	}
	// Pop the previous message
	mainStatusbar->pop();
	// Push
	mainStatusbar->push(text);
}

