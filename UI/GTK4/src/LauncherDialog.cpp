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
#include <iostream>
#include <string>
#include <glibmm/ustring.h>
#include <sigc++/sigc++.h>

#include "config.h"
#include "NLS.h"
#include "Url.h"
#include "PinotSettings.h"
#include "PinotUtils.h"
#include "LauncherDialog.h"

using namespace std;
using namespace Glib;
using namespace Gtk;

LauncherDialog::LauncherDialog(_GtkDialog *&pParent, RefPtr<Builder>& refBuilder) :
	Dialog(pParent),
	cancelLaunchButton(NULL),
	openLaunchButton(NULL),
	launcherEntry(NULL),
	rememberCheckbutton(NULL)
{
	refBuilder->get_widget("cancelLaunchButton", cancelLaunchButton);
	refBuilder->get_widget("openLaunchButton", openLaunchButton);
	refBuilder->get_widget("launcherEntry", launcherEntry);;
	refBuilder->get_widget("rememberCheckbutton", rememberCheckbutton);

	cancelLaunchButton->signal_clicked().connect(sigc::mem_fun(*this, &LauncherDialog::on_cancelLaunchButton_clicked), false);
	openLaunchButton->signal_clicked().connect(sigc::mem_fun(*this, &LauncherDialog::on_openLaunchButton_clicked), false);

	launcherEntry->set_text("xdg-open");
}

LauncherDialog::~LauncherDialog()
{
}

void LauncherDialog::setUrl(const string &url)
{
	set_title(Url::prettifyUrl(url, 40));
}

bool LauncherDialog::getInput(MIMEAction &action, bool &remember)
{
	ustring cmdLine(launcherEntry->get_text());

	if (cmdLine.empty() == false)
	{
		action = MIMEAction("UserApp", from_utf8(launcherEntry->get_text()));
		// Assume this isn't local only 
		action.m_localOnly = false;
		remember = rememberCheckbutton->get_active();

		return true;
	}

	return false;
}

void LauncherDialog::on_cancelLaunchButton_clicked()
{
#ifdef DEBUG
	clog << "LauncherDialog::on_cancelLaunchButton_clicked: called" << endl;
#endif
	close();
}

void LauncherDialog::on_openLaunchButton_clicked()
{
#ifdef DEBUG
	clog << "LauncherDialog::on_openLaunchButton_clicked: called" << endl;
#endif
	close();
}

