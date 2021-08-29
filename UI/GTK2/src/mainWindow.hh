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

#ifndef _MAINWINDOW_HH
#define _MAINWINDOW_HH

#include <string>
#include <set>
#include <sigc++/sigc++.h>
#include <glibmm/refptr.h>
#include <gdkmm/pixbuf.h>
#include <gdkmm/color.h>
#include <gtkmm/entrycompletion.h>
#if GTK_VERSION_LT(2, 90)
#include <gtkmm/rc.h>
#endif
#include <gtkmm/notebook.h>
#include <gtkmm/liststore.h>
#include <gtkmm/treestore.h>
#include <gtkmm/treeselection.h>
#include <gtkmm/selectiondata.h>
#include <gtkmm/menu.h>
#include <gtkmm/menuitem.h>
#include <gtkmm/imagemenuitem.h>
#include <gtkmm/radiomenuitem.h>
#if GTK_VERSION_LT(2, 90)
#include <gtkmm/tooltips.h>
#endif

#include "DocumentInfo.h"
#include "QueryProperties.h"
#include "MonitorInterface.h"
#include "MonitorHandler.h"
#include "PinotSettings.h"
#include "EnginesTree.hh"
#include "IndexPage.hh"
#include "ModelColumns.hh"
#include "Notebook.hh"
#include "ResultsTree.hh"
#include "UIThreads.hh"
#include "mainWindow_glade.hh"

class mainWindow : public mainWindow_glade
{
public:
	mainWindow();
	virtual ~mainWindow();

	void set_status(const Glib::ustring &text, bool canBeSkipped = false);

protected:
	// Utility methods
	void populate_queryTreeview(const std::string &selectedQueryName);
	void save_queryTreeview();
	void populate_menuItems();
	void populate_cacheMenu();
	void populate_indexMenu();
	void populate_findMenu();
	void add_query(QueryProperties &queryProps, bool mergeQueries);
	bool get_results_page_details(const Glib::ustring &queryName,
		QueryProperties &queryProps, std::set<std::string> &locations,
		std::set<std::string> &locationsToIndex);

	// Handlers
	void on_data_received(const Glib::RefPtr<Gdk::DragContext> &context,
		int x, int y, const Gtk::SelectionData &data, guint info, guint dataTime);
	void on_enginesTreeviewSelection_changed();
	void on_queryTreeviewSelection_changed();
	void on_resultsTreeviewSelection_changed(Glib::ustring queryName);
	void on_indexTreeviewSelection_changed(Glib::ustring indexName);
	void on_document_changed(std::vector<DocumentInfo> &resultsList,
		bool isDocumentsIndex, bool editDocument, bool areResults);
	void on_index_changed(Glib::ustring indexName);
	void on_cache_changed(PinotSettings::CacheProvider cacheProvider);
	void on_searchthis_changed(Glib::ustring queryName);
	void on_query_changed(Glib::ustring indexName, Glib::ustring queryName);
#if GTK_VERSION_LT(2, 90)
	void on_switch_page(GtkNotebookPage *pPage, guint pageNum);
#else
	void on_switch_page(Gtk::Widget *pPage, guint pageNum);
#endif
	void on_close_page(Glib::ustring title, NotebookPageBox::PageType type);
	void on_thread_end(WorkerThread *pThread);
	void on_editindex(Glib::ustring indexName, Glib::ustring location);
	void on_suggestQueryButton_clicked(Glib::ustring queryName, Glib::ustring queryText);
	void on_indexBackButton_clicked(Glib::ustring indexName);
	void on_indexForwardButton_clicked(Glib::ustring indexName);

	// Handlers inherited from the base class
        virtual void on_open_activate();
        virtual void on_openparent_activate();
        virtual void on_addtoindex_activate();
        virtual void on_updateindex_activate();
        virtual void on_unindex_activate();
        virtual void on_morelikethis_activate();
        virtual void on_properties_activate();
        virtual void on_quit_activate();
        virtual void on_cut_activate();
        virtual void on_copy_activate();
        virtual void on_paste_activate();
        virtual void on_delete_activate();
        virtual void on_preferences_activate();
        virtual void on_groupresults_activate();
        virtual void on_showextracts_activate();
        virtual void on_import_activate();
        virtual void on_export_activate();
        virtual void on_status_activate();
        virtual void on_about_activate();
        virtual void on_addIndexButton_clicked();
        virtual void on_removeIndexButton_clicked();
        virtual void on_enginesTogglebutton_toggled();
        virtual void on_liveQueryEntry_changed();
        virtual void on_liveQueryEntry_activate();
	virtual void on_liveQueryEntry_icon(Gtk::EntryIconPosition position, const GdkEventButton *ev);
        virtual void on_findButton_clicked();
        virtual bool on_queryTreeview_button_press_event(GdkEventButton *ev);
        virtual void on_addQueryButton_clicked();
        virtual void on_removeQueryButton_clicked();
        virtual void on_queryHistoryButton_clicked();
        virtual void on_findQueryButton_clicked();
        virtual bool on_mainWindow_delete_event(GdkEventAny *ev);

	// Action methods
	void show_pagebased_menuitems(bool showItems);
	void show_selectionbased_menuitems(bool showItems);
	NotebookPageBox *get_current_page(void);
	NotebookPageBox *get_page(const Glib::ustring &title,
		NotebookPageBox::PageType type);
	int get_page_number(const Glib::ustring &title,
		NotebookPageBox::PageType type);
	void edit_query(QueryProperties &queryProps, bool newQuery);
	void run_search(const QueryProperties &queryProps);
	void browse_index(const Glib::ustring &indexName, const Glib::ustring &labelName,
		unsigned int startDoc, bool changePage = true);
	void view_documents(const std::vector<DocumentInfo> &documentsList);
	bool start_thread(WorkerThread *pNewThread, bool inBackground = false);
	bool expand_locations(void);

	// Status methods
	bool on_activity_timeout(void);

private:
	// Global settings
	PinotSettings &m_settings;
	// Engine
	EnginesTree *m_pEnginesTree;
	// Query
	ComboModelColumns m_liveQueryColumns;
	Glib::RefPtr<Gtk::ListStore> m_refLiveQueryList;
	Glib::RefPtr<Gtk::EntryCompletion> m_refLiveQueryCompletion;
	QueryModelColumns m_queryColumns;
	Glib::RefPtr<Gtk::ListStore> m_refQueryTree;
	class ExpandSet
	{
		public:
			ExpandSet();
			~ExpandSet();

			QueryProperties m_queryProps;
			set<string> m_locations;

	};
	std::vector<ExpandSet> m_expandSets;
	// Notebook
	Gtk::Notebook *m_pNotebook;
	// Menus
	Gtk::ImageMenuItem * open1;
	Gtk::MenuItem * opencache1;
	Gtk::ImageMenuItem * openparent1;
	Gtk::ImageMenuItem * addtoindex1;
	Gtk::ImageMenuItem * updateindex1;
	Gtk::ImageMenuItem * unindex1;
	Gtk::MenuItem * morelikethis1;
	Gtk::MenuItem * searchthisfor1;
	Gtk::ImageMenuItem * properties1;
	Gtk::MenuItem * separatormenuitem1;
	Gtk::ImageMenuItem * quit1;
	Gtk::MenuItem * fileMenuitem;
	Gtk::ImageMenuItem * cut1;
	Gtk::ImageMenuItem * copy1;
	Gtk::ImageMenuItem * paste1;
	Gtk::ImageMenuItem * delete1;
	Gtk::ImageMenuItem * preferences1;
	Gtk::MenuItem * editMenuitem;
	Gtk::MenuItem * listcontents1;
	Gtk::RadioMenuItem * searchenginegroup1;
	Gtk::RadioMenuItem * hostnamegroup1;
	Gtk::MenuItem * groupresults1;
	Gtk::CheckMenuItem * showextracts1;
	Gtk::ImageMenuItem * import1;
	Gtk::ImageMenuItem * export1;
	Gtk::ImageMenuItem * about1;
	Gtk::MenuItem * helpMenuitem;
	Gtk::Menu *m_pIndexMenu;
	Gtk::Menu *m_pCacheMenu;
	Gtk::Menu *m_pFindMenu;
	// Index
	ComboModelColumns m_indexNameColumns;
	Glib::RefPtr<Gtk::ListStore> m_refIndexNameTree;
#if GTK_VERSION_LT(2, 90)
	// Tooltips
	Gtk::Tooltips m_tooltips;
#endif
	// Page switching
	sigc::connection m_pageSwitchConnection;
	// Activity timeout
	sigc::connection m_timeoutConnection;
	// Monitoring
	MonitorInterface *m_pSettingsMonitor;
	MonitorHandler *m_pSettingsHandler;
	// Viewing
	std::set<std::string> m_temporaryFiles;
	// Internal state
	class InternalState : public QueueManager
	{
		public:
			InternalState(mainWindow *pWindow);
			virtual ~InternalState();

			// Query
			unsigned int m_liveQueryLength;
			// Notebook pages
			unsigned int m_currentPage;
			// Current actions
			bool m_browsingIndex;

	} m_state;
	static unsigned int m_maxDocsCount;
	unsigned int m_maxResultsCount;

};

#endif
