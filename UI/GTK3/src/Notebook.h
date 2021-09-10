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

#ifndef _NOTEBOOK_HH
#define _NOTEBOOK_HH

#include <set>
#include <sigc++/sigc++.h>
#include <glibmm/ustring.h>
#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/comboboxtext.h>
#include <gtkmm/image.h>
#include <gtkmm/label.h>
#include <gtkmm/paned.h>
#include <gtkmm/textview.h>

#include "PinotSettings.h"
#include "ResultsTree.h"

class NotebookPageBox : public Gtk::VBox
{
	public:
		typedef enum { RESULTS_PAGE = 0, INDEX_PAGE } PageType;

		NotebookPageBox(const Glib::ustring &title, PageType type,
			PinotSettings &settings);
		virtual ~NotebookPageBox();

		/// Returns the page title.
		Glib::ustring getTitle(void) const;

		/// Returns the page type.
		PageType getType(void) const;

		/// Returns the page's tree.
		virtual ResultsTree *getTree(void) const;

	protected:
		Glib::ustring m_title;
		PageType m_type;
		PinotSettings &m_settings;
		ResultsTree *m_pTree;

};

class ResultsPage : public NotebookPageBox
{
	public:
		ResultsPage(const Glib::ustring &queryName, ResultsTree *pTree,
			int parentHeight, PinotSettings &settings);
		virtual ~ResultsPage();

		/// Returns the suggest signal.
		sigc::signal2<void, Glib::ustring, Glib::ustring>& getSuggestSignal(void);

		/// Append a suggestion.
		bool appendSuggestion(const Glib::ustring &text);

	protected:
		Gtk::Label *m_pLabel;
		Gtk::ComboBoxText *m_pCombobox;
		Gtk::Button *m_pYesButton;
		Gtk::Image *m_pCloseImage;
		Gtk::Button *m_pCloseButton;
		Gtk::HBox *m_pHBox;
		Gtk::VBox *m_pVBox;
		Gtk::VPaned *m_pVPaned;
		sigc::signal2<void, Glib::ustring, Glib::ustring> m_signalSuggest;
		std::set<Glib::ustring> m_suggestions;
		void onYesButtonClicked();

		void onCloseButtonClicked();

};

class NotebookTabBox : public Gtk::HBox
{
	public:
		NotebookTabBox(const Glib::ustring &title, NotebookPageBox::PageType type);
		virtual ~NotebookTabBox();

		/// Returns the close signal.
		sigc::signal2<void, Glib::ustring, NotebookPageBox::PageType>& getCloseSignal(void);

	protected:
		static bool m_initialized;
		Glib::ustring m_title;
		NotebookPageBox::PageType m_pageType;
		Gtk::Label *m_pTabLabel;
		Gtk::Image *m_pTabImage;
		Gtk::Button *m_pTabButton;
		sigc::signal2<void, Glib::ustring, NotebookPageBox::PageType> m_signalClose;

		void onButtonClicked(void);

};

#endif // _NOTEBOOK_HH
