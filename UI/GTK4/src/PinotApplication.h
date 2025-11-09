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
 
#ifndef _PINOTAPPLICATION_HH
#define _PINOTAPPLICATION_HH

#include <glibmm/refptr.h>
#include <glibmm/ustring.h>
#include <gtkmm/application.h>
#include <gtkmm/builder.h>

class PinotApplication : public Gtk::Application
{
	public:
		static Glib::RefPtr<PinotApplication> create(const Glib::ustring &statusText = Glib::ustring(""),
			const Glib::ustring &queryTerms = Glib::ustring(""),
			bool prefsMode = false);

		bool brokenResources(void) const;

	protected:
		Glib::RefPtr<Gtk::Builder> m_refBuilder;
		Glib::ustring m_queryTerms;
		Glib::ustring m_statusText;
		bool m_prefsMode;
		bool m_brokenResources;

		PinotApplication(const Glib::ustring &statusText = Glib::ustring(""),
			bool prefsMode = false);

		// Overrides
		void on_startup() override;
		void on_activate() override;
		void on_shutdown() override;

		// Menu handlers
		void on_menu_file_new_generic();
		void on_menu_file_quit();
		void on_menu_help_about();

	private:
		PinotApplication(const PinotApplication &other);
		PinotApplication &operator=(const PinotApplication &other);

};

#endif // _PINOTAPPLICATION_HH
