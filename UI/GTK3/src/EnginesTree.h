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

#ifndef _ENGINESTREE_HH
#define _ENGINESTREE_HH

#include <string>
#include <vector>
#include <set>
#include <map>
#include <sigc++/sigc++.h>
#include <glibmm/refptr.h>
#include <glibmm/ustring.h>
#include <gdkmm/pixbuf.h>
#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/menu.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/textview.h>
#include <gtkmm/treestore.h>
#include <gtkmm/treeview.h>
#include <gtkmm/treeselection.h>

#include "QueryProperties.h"
#include "PinotSettings.h"
#include "ModelColumns.h"

class EnginesTree : public Gtk::TreeView
{
	public:
		EnginesTree(Gtk::VBox *enginesVbox, PinotSettings &settings);
		virtual ~EnginesTree();

		/// Gets a list of selected items.
		std::vector<Gtk::TreeModel::Path> getSelection(void);

		/// Gets an iterator.
		Gtk::TreeModel::iterator getIter(Gtk::TreeModel::Path enginePath);

		/// Gets the column record.
		EnginesModelColumns &getColumnRecord(void);

		/// Populate the tree.
		void populate(bool indexesOnly = false);

		/// Clear the tree.
		void clear(void);

		/// Save the tree's state.
		void save(void);

		/// Returns the double-click signal.
		sigc::signal2<void, std::string, std::string>& getDoubleClickSignal(void);

	protected:
		Glib::RefPtr<Gtk::TreeStore> m_refStore;
		PinotSettings &m_settings;
		Glib::RefPtr<Gdk::Pixbuf> m_engineFolderIconPixbuf;
		EnginesModelColumns m_enginesColumns;
		sigc::signal2<void, std::string, std::string> m_signalDoubleClick;

		void renderEngineIcon(Gtk::CellRenderer *renderer, const Gtk::TreeModel::iterator &iter);

		/// Handles button presses.
		void onButtonPressEvent(GdkEventButton *ev);

		/// Handles attempts to select rows.
		bool onSelectionSelect(const Glib::RefPtr<Gtk::TreeModel>& model,
			const Gtk::TreeModel::Path& node_path, bool path_currently_selected);

		/// Handles GTK style changes.
		void onStyleChanged(void);

	private:
		EnginesTree(const EnginesTree &other);
		EnginesTree &operator=(const EnginesTree &other);

};

#endif // _ENGINESTREE_HH
