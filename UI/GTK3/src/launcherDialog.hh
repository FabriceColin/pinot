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

#ifndef _LAUNCHERDIALOG_HH
#define _LAUNCHERDIALOG_HH

#include <string>
#include <glibmm/refptr.h>
#include <gtkmm/checkbutton.h>
#include <gtkmm/builder.h>
#include <gtkmm/button.h>
#include <gtkmm/dialog.h>
#include <gtkmm/entry.h>

#include "MIMEScanner.h"

class launcherDialog : public Gtk::Dialog
{
public:
	launcherDialog(_GtkDialog *&pParent, Glib::RefPtr<Gtk::Builder>& refBuilder);
	virtual ~launcherDialog();

	void setUrl(const std::string &url);

	bool getInput(MIMEAction &action, bool &remember);

protected:
	Gtk::Button *cancelLaunchButton;
	Gtk::Button *openLaunchButton;
	Gtk::Entry *launcherEntry;
	Gtk::CheckButton *rememberCheckbutton;

	// Handlers defined in the XML file
	virtual void on_cancelLaunchButton_clicked();
	virtual void on_openLaunchButton_clicked();

};
#endif
