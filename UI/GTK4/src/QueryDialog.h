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

#ifndef _QUERYDIALOG_HH
#define _QUERYDIALOG_HH

#include <string>
#include <glibmm/refptr.h>
#include <glibmm/ustring.h>
#include <gtkmm/builder.h>
#include <gtkmm/button.h>
#include <gtkmm/checkbutton.h>
#include <gtkmm/comboboxtext.h>
#include <gtkmm/dialog.h>
#include <gtkmm/entry.h>
#include <gtkmm/spinbutton.h>
#include <gtkmm/textview.h>

#include "QueryProperties.h"
#include "PinotSettings.h"

class QueryDialog : public Gtk::Dialog
{
public:
	QueryDialog(_GtkDialog *&pParent, Glib::RefPtr<Gtk::Builder>& refBuilder);
	virtual ~QueryDialog();

	void setQueryProperties(const QueryProperties &properties);

	const QueryProperties &getQueryProperties(void) const;

	bool badName(void) const;

protected:
	Gtk::Button *queryCancelButton;
	Gtk::Button *queryOkButton;
	Gtk::Button *addFilterButton;
	Gtk::Entry *nameEntry;
	Gtk::ComboBoxText *filterCombobox;
	Gtk::TextView *queryTextview;
	Gtk::CheckButton *indexCheckbutton;
	Gtk::ComboBoxText *labelNameCombobox;
	Gtk::SpinButton *resultsCountSpinbutton;
	Gtk::ComboBoxText *sortOrderCombobox;
	Gtk::ComboBoxText *stemmingCombobox;
	Gtk::CheckButton *indexNewCheckbutton;

	Glib::ustring m_name;
	QueryProperties m_properties;
	bool m_badName;

	bool is_separator(const Glib::RefPtr<Gtk::TreeModel>& model,
		const Gtk::TreeModel::iterator& iter);

	void populate_comboboxes();

	// Handlers defined in the XML file
	virtual void on_queryCancelButton_clicked();
	virtual void on_queryOkButton_clicked();
	virtual void on_nameEntry_changed();
	virtual void on_addFilterButton_clicked();
	virtual void on_indexCheckbutton_toggled();

};
#endif
