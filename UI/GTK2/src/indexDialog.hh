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

#ifndef _INDEXDIALOG_HH
#define _INDEXDIALOG_HH

#include <glibmm/ustring.h>

#include "ModelColumns.hh"
#include "indexDialog_glade.hh"

class indexDialog : public indexDialog_glade
{  
public:
	indexDialog();
	indexDialog(const Glib::ustring &name, const Glib::ustring &location);
	virtual ~indexDialog();

	bool badName(void) const;

	Glib::ustring getName(void) const;

	Glib::ustring getLocation(void) const;

protected:
	Glib::ustring m_name;
	Glib::ustring m_location;
	bool m_editIndex;
	bool m_badName;

	void populate_typeCombobox(void);
	void checkFields(void);

	virtual void on_indexOkbutton_clicked();
	virtual void on_locationEntry_changed();
	virtual void on_locationButton_clicked();
	virtual void on_typeCombobox_changed();
	virtual void on_nameEntry_changed();

};
#endif
