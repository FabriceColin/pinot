/*
 *  Copyright 2005-2011 Fabrice Colin
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

#ifndef _RESULTSTREE_HH
#define _RESULTSTREE_HH

#include <string>
#include <vector>
#include <set>
#include <map>
#include <sigc++/sigc++.h>
#include <glibmm/refptr.h>
#include <glibmm/ustring.h>
#include <gdkmm/pixbuf.h>
#include <gtkmm/button.h>
#include <gtkmm/menu.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/treestore.h>
#include <gtkmm/textview.h>
#include <gtkmm/treeview.h>
#include <gtkmm/treeselection.h>

#include "ResultsExporter.h"
#include "PinotSettings.h"
#include "ModelColumns.hh"

class ResultsTree : public Gtk::TreeView
{
	public:
		typedef enum { BY_ENGINE = 0, BY_HOST, FLAT } GroupByMode;

		ResultsTree(const Glib::ustring &name, Gtk::Menu *pPopupMenu,
			GroupByMode groupMode, PinotSettings &settings);
		virtual ~ResultsTree();

		/// Returns the results scrolled window.
		Gtk::ScrolledWindow *getResultsScrolledWindow(void) const;

		/// Returns the extract scrolled window.
		Gtk::ScrolledWindow *getExtractScrolledWindow(void) const;

		/// Returns whether the extract text view has the focus.
		bool focusOnExtract(void) const;

		/// Returns the extract.
		Glib::ustring getExtract(void) const;

		/**
		  * Adds a set of results.
		  * Returns true if something was added to the tree.
		  */
		bool addResults(const std::string &engineName, const std::vector<DocumentInfo> &resultsList,
			const std::string &charset, bool updateHistory);

		/// Sets how results are grouped.
		void setGroupMode(GroupByMode groupMode);

		/// Gets the first selected item's URL.
		Glib::ustring getFirstSelectionURL(void);

		/// Gets a list of selected items.
		bool getSelection(std::vector<DocumentInfo> &resultsList, bool skipIndexed = false);

		/// Sets the selected items' state.
		void setSelectionState(bool viewed);

		/// Updates a result's properties.
		bool updateResult(const DocumentInfo &result);

		/**
		  * Deletes the current selection.
		  * Returns true if the tree is then empty.
		  */
		bool deleteSelection(void);

		/// Deletes results.
		bool deleteResults(const std::string &engineName);

		/// Returns the number of rows.
		unsigned int getRowsCount(void);

		/// Refreshes the tree.
		void refresh(void);

		/// Clears the tree.
		void clear(void);

		/// Shows or hides the extract field.
		void showExtract(bool showExtract = true);

		/// Exports results to a file.
		void exportResults(const std::string &fileName,
			const string &queryName, bool csvFormat);

		/// Returns the changed selection signal.
		sigc::signal1<void, Glib::ustring>& getSelectionChangedSignal(void);

		/// Returns the double-click signal.
		sigc::signal0<void>& getDoubleClickSignal(void);

	protected:
		Glib::ustring m_treeName;
		Gtk::Menu *m_pPopupMenu;
		Gtk::ScrolledWindow *m_pResultsScrolledwindow;
		Glib::RefPtr<Gtk::TreeStore> m_refStore;
		sigc::signal1<void, Glib::ustring> m_signalSelectionChanged;
		sigc::signal0<void> m_signalDoubleClick;
		PinotSettings &m_settings;
		Glib::RefPtr<Gdk::Pixbuf> m_indexedIconPixbuf;
		Glib::RefPtr<Gdk::Pixbuf> m_viewededIconPixbuf;
		Glib::RefPtr<Gdk::Pixbuf> m_upIconPixbuf;
		Glib::RefPtr<Gdk::Pixbuf> m_downIconPixbuf;
		std::map<std::string, Gtk::TreeModel::iterator> m_resultsGroups;
		ResultsModelColumns m_resultsColumns;
		Gtk::ScrolledWindow *m_pExtractScrolledwindow;
		Gtk::TextView *m_extractTextView;
		ComboModelColumns m_extractColumns;
		bool m_showExtract;
		GroupByMode m_groupMode;

		void renderViewStatus(Gtk::CellRenderer *pRenderer, const Gtk::TreeModel::iterator &iter);

		void renderIndexStatus(Gtk::CellRenderer *pRenderer, const Gtk::TreeModel::iterator &iter);

		void renderRanking(Gtk::CellRenderer *pRenderer, const Gtk::TreeModel::iterator &iter);

		void renderTitleColumn(Gtk::CellRenderer *pRenderer, const Gtk::TreeModel::iterator &iter);

		void onButtonPressEvent(GdkEventButton *ev);

		void onSelectionChanged(void);

		bool onSelectionSelect(const Glib::RefPtr<Gtk::TreeModel>& model,
			const Gtk::TreeModel::Path& node_path, bool path_currently_selected);

		/// Handles GTK style changes.
#if GTK_VERSION_LT(2, 90)
		void onStyleChanged(const Glib::RefPtr<Gtk::Style> &previous_style);
#else
		void onStyleChanged(void);
#endif

		/// Adds a results group.
		bool appendGroup(const std::string &groupName, ResultsModelColumns::RowType groupType,
			Gtk::TreeModel::iterator &groupIter);

		/// Adds a new row in the results tree.
		bool appendResult(const Glib::ustring &text, const Glib::ustring &url,
			int score, int rankDiff, bool isIndexed, bool wasViewed,
			unsigned int docId, const Glib::ustring &timestamp, const std::string &serial,
			unsigned int engineId, unsigned int indexId,
			Gtk::TreeModel::iterator &newRowIter,
			const Gtk::TreeModel::iterator &parentIter,
			bool noDuplicates = false);

		/// Updates a results group.
		void updateGroup(Gtk::TreeModel::iterator &groupIter);

		/// Updates a row.
		void updateRow(Gtk::TreeModel::Row &row, const Glib::ustring &text,
			const Glib::ustring &url, int score, unsigned int engineId, unsigned int indexId,
			unsigned int docId, const Glib::ustring &timestamp, const std::string &serial,
			ResultsModelColumns::RowType resultType, bool indexed, bool viewed, int rankDiff);

		/// Retrieves the extract to show for the given row.
		Glib::ustring findResultsExtract(const Gtk::TreeModel::Row &row);

		/// Exports results to a file.
		void exportResults(Gtk::TreeModel::Children &groupChildren,
			const string &queryName, ResultsExporter *pExporter);

	private:
		ResultsTree(const ResultsTree &other);
		ResultsTree &operator=(const ResultsTree &other);

};

#endif // _RESULTSTREE_HH
