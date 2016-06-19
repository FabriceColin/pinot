/*
 *  Copyright 2005-2009 Fabrice Colin
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

#ifndef _IMPORTDIALOG_HH
#define _IMPORTDIALOG_HH

#include <string>
#include <set>
#include <sigc++/sigc++.h>
#include <gtkmm/entrycompletion.h>
#include <gtkmm/liststore.h>
#include <glibmm/ustring.h>

#include "DocumentInfo.h"
#include "ModelColumns.hh"
#include "importDialog_glade.hh"

class importDialog : public importDialog_glade
{  
public:
	importDialog();
	virtual ~importDialog();

	const DocumentInfo &getDocumentInfo(void) const;

protected:
	ComboModelColumns m_locationColumns;
	Glib::RefPtr<Gtk::ListStore> m_refLocationList;
	Glib::RefPtr<Gtk::EntryCompletion> m_refLocationCompletion;
	unsigned int m_locationLength;
	DocumentInfo m_docInfo;

	void on_data_received(const Glib::RefPtr<Gdk::DragContext> &context,
		int x, int y, const Gtk::SelectionData &data, guint info, guint time);
	void populate_comboboxes(void);

	virtual void on_importButton_clicked();
	virtual void on_locationEntry_changed();

};

#endif
