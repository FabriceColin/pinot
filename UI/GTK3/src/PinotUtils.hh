/*
 *  Copyright 2005,2006 Fabrice Colin
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
 
#ifndef _PINOTUTILS_HH
#define _PINOTUTILS_HH

#include <string>
#include <gtkmmconfig.h>
#include <glibmm/ustring.h>
#include <gtkmm/window.h>
#include <gtkmm/filechooserdialog.h>
#include <gtkmm/treeview.h>
#include <gtkmm/treemodel.h>
#include <gtkmm/filechooserdialog.h>

/// Open a FileChooserDialog.
bool select_file_name(const Glib::ustring &title, Glib::ustring &location,
	bool openOrCreate, bool directoriesOnly = false);

/// Prepare a FileChooserDialog.
bool prepare_file_chooser(Gtk::FileChooserDialog &fileChooser, Glib::ustring &location,
	bool openOrCreate, bool directoriesOnly = false);

/// Get a column height.
int get_column_height(Gtk::TreeView *pTree);

/// Create a text column.
Gtk::TreeViewColumn *create_column(const Glib::ustring &title,
	const Gtk::TreeModelColumnBase& modelColumn,
	bool isResizable, bool isSortable, const Gtk::TreeModelColumnBase &sortColumn);

/// Create an icon and text column, rendered by renderTextAndIconCell.
Gtk::TreeViewColumn *create_column_with_icon(const Glib::ustring &title,
	const Gtk::TreeModelColumnBase& modelColumn,
	const Gtk::TreeViewColumn::SlotCellData &renderTextAndIconCell,
	bool isResizable, bool isSortable, const Gtk::TreeModelColumnBase &sortColumn);

/// Converts to UTF-8.
Glib::ustring to_utf8(const std::string &text);

/// Converts from the given charset to UTF-8.
Glib::ustring to_utf8(const std::string &text, const std::string &charset);

/// Converts from UTF-8.
std::string from_utf8(const Glib::ustring &text);

#endif // _PINOTUTILS_HH
