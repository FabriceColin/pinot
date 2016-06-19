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

#ifndef _QUERYDIALOG_HH
#define _QUERYDIALOG_HH

#include <string>
#include <glibmm/ustring.h>

#include "QueryProperties.h"
#include "PinotSettings.h"
#include "queryDialog_glade.hh"

class queryDialog : public queryDialog_glade
{
public:
	queryDialog(QueryProperties &properties);
	virtual ~queryDialog();

	bool badName(void) const;

protected:
	Glib::ustring m_name;
	QueryProperties &m_properties;
	bool m_badName;

	bool is_separator(const Glib::RefPtr<Gtk::TreeModel>& model,
		const Gtk::TreeModel::iterator& iter);

	void populate_comboboxes();

	virtual void on_queryOkbutton_clicked();
	virtual void on_nameEntry_changed();
	virtual void on_addFilterButton_clicked();
	virtual void on_indexCheckbutton_toggled();

};
#endif
