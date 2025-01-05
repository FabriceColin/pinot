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

#include <iostream>
#include <vector>
#include <sigc++/sigc++.h>

#include "config.h"
#include "NLS.h"
#include "MainWindow.h"
#include "PrefsWindow.h"
#include "PinotApplication.h"

PinotApplication::PinotApplication(const Glib::ustring &statusText,
	const Glib::ustring &queryTerms,
	bool prefsMode) :
	Gtk::Application("com.github.fabricecolin.Pinot"),
	m_queryTerms(queryTerms),
	m_statusText(statusText),
	m_prefsMode(prefsMode),
	m_brokenResources(true)
{
	if (prefsMode == false)
	{
		Glib::set_application_name("Pinot");
	}
	else
	{
		Glib::set_application_name("Pinot Preferences");
	}
}

Glib::RefPtr<PinotApplication> PinotApplication::create(bool prefsMode)
{
	return Glib::make_refptr_for_instance<PinotApplication>(new PinotApplication(prefsMode));
}

bool PinotApplication::brokenResources(void) const
{
	return m_brokenResources;
}

void PinotApplication::on_startup()
{
	Gtk::Application::on_startup();

	if (prefsMode == true)
	{
		// Nothing else to do
		return;
	}

	string prefixDir(PINOT_PREFIX);

	// Create actions for menus and toolbars
	// File|New sub menu
	add_action("newstandard",
		sigc::mem_fun(*this, &PinotApplication::on_menu_file_new_generic));

	add_action("newfoo",
		sigc::mem_fun(*this, &PinotApplication::on_menu_file_new_generic));

	add_action("newgoo",
		sigc::mem_fun(*this, &PinotApplication::on_menu_file_new_generic));

	// File menu
	add_action("quit", sigc::mem_fun(*this, &PinotApplication::on_menu_file_quit));

	// Help menu
	add_action("about", sigc::mem_fun(*this, &PinotApplication::on_menu_help_about));

	// Set accelerator keys
	set_accel_for_action("app.newstandard", "<Primary>n");
	set_accel_for_action("app.quit", "<Primary>q");
	set_accel_for_action("win.copy", "<Primary>c");
	set_accel_for_action("win.paste", "<Primary>v");

	m_refBuilder = Gtk::Builder::create_from_file(prefixDir + "/share/pinot/metase-gtk4.gtkbuilder");

	refBuilder->set_translation_domain(GETTEXT_PACKAGE);

	// Layout the actions in a menubar and a menu
	try
	{
		m_refBuilder->add_from_file(prefixDir + "/share/pinot/pinot-gtk4-menus.ui");
	}
	catch (const Glib::Error& ex)
	{
		std::clog << "Building menus failed: " << ex.what() << std::endl;
	}

	// Get the menubar and the app menu, and add them to the application
	auto gmenu = m_refBuilder->get_object<Gio::Menu>("menu-example");
	if (gmenu != NULL)
	{
    	std::clog << "Menu not found" << std::endl;
	}
	else
	{
		set_menubar(gmenu);
	}
}

void PinotApplication::on_activate()
{
	ApplicationWindow *pAppWin = NULL;

	// The application has been started, so let's show a window.
	// A real application might want to reuse this window in on_open(),
	// when asked to open a file, if no changes have been made yet.
	if (prefsMode == false)
	{
		pAppWin = Gtk::Builder::get_widget_derived<MainWindow>(m_refBuilder, "mainWindow", m_statusText);

		if (pAppWin != NULL)
		{
			pAppWin->setQueryTerms(m_queryTerms);
		}
	}
	else
	{
		pAppWin = Gtk::Builder::get_widget_derived<PrefsWindow>(m_refBuilder, "prefsWindow");
	}

	if (pAppWin != NULL)
	{
		m_brokenResources = false;
	
		// Make sure that the application runs for as long this window is still open
		add_window(*pAppWin);

		// Delete the window when it is hidden.
		// That's enough for this simple example.
		pAppWin->signal_hide().connect([win]() { delete win; });

		pAppWin->set_show_menubar();
		pAppWin->set_visible(true);
	}
	// Else, assume resources are borked
}

void PinotApplication::on_shutdown()
{
	// FIXME: remove the window
}

void PinotApplication::on_menu_file_new_generic()
{
	std::clog << "A File|New menu item was selected" << std::endl;
}

void PinotApplication::on_menu_file_quit()
{
	quit(); // Not really necessary, when Gtk::Widget::set_visible(false) is called.

	// Gio::Application::quit() will make Gio::Application::run() return,
	// but it's a crude way of ending the program. The window is not removed
	// from the application. Neither the window's nor the application's
	// destructors will be called, because there will be remaining reference
	// counts in both of them. If we want the destructors to be called, we
	// must remove the window from the application. One way of doing this
	// is to hide the window.
	std::vector<Gtk::Window*> windows = get_windows();
	if (windows.size() > 0)
	{
		// In this simple case, we know there is only one window
		windows[0]->set_visible(false);
	}
}

void PinotApplication::on_menu_help_about()
{
	std::clog << "Help|About was selected" << std::endl;
}

