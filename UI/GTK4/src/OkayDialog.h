/*
 *  Copyright 2025 Fabrice Colin
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

#ifndef _OKAYDIALOG_HH
#define _OKAYDIALOG_HH

#include <string>
#include <glibmm/refptr.h>
#include <gtkmm/builder.h>
#include <gtkmm/dialog.h>

class OkayDialog : public Gtk::Dialog
{
public:
	OkayDialog(_GtkDialog *&pParent);
	virtual ~OkayDialog();

	bool userAcked(void) const;

protected:
	bool m_userAcked;

	virtual void on_response(int responseId);

};

#endif
