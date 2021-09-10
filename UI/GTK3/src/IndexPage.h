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

#ifndef _INDEXPAGE_HH
#define _INDEXPAGE_HH

#include <string>
#include <vector>
#include <sigc++/sigc++.h>
#include <glibmm/refptr.h>
#include <glibmm/ustring.h>
#include <gtkmm/menu.h>
#include <gdkmm/pixbuf.h>
#include <gtkmm/comboboxtext.h>
#include <gtkmm/button.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/textview.h>
#include <gtkmm/treeview.h>
#include <gtkmm/treeselection.h>

#include "QueryProperties.h"
#include "PinotSettings.h"
#include "ResultsTree.h"
#include "Notebook.h"

class IndexPage : public NotebookPageBox
{
	public:
		IndexPage(const Glib::ustring &indexName, ResultsTree *pTree,
			PinotSettings &settings);
		virtual ~IndexPage();

		/// Returns the name of the current query.
		Glib::ustring getQueryName(void) const;

		/// Populates the queries list.
		void populateQueryCombobox(const std::string &queryName);

		/// Updates the state of the index buttons.
		void updateButtonsState(unsigned int maxDocsCount);

		/// Gets the number of documents.
		unsigned int getDocumentsCount(void) const;

		/// Sets the number of documents.
		void setDocumentsCount(unsigned int docsCount);

		/// Gets the first document.
		unsigned int getFirstDocument(void) const;

		/// Sets the first document.
		void setFirstDocument(unsigned int startDoc);

		/// Returns the changed query signal.
		sigc::signal2<void, Glib::ustring, Glib::ustring>& getQueryChangedSignal(void);

		/// Returns the back button clicked signal.
		sigc::signal1<void, Glib::ustring>& getBackClickedSignal(void);

		/// Returns the forward button clicked signal.
		sigc::signal1<void, Glib::ustring>& getForwardClickedSignal(void);

	protected:
		Glib::ustring m_indexName;
		Glib::ustring m_queryName;
		Gtk::ComboBoxText *m_pQueryCombobox;
		Gtk::Button *m_pBackButton;
		Gtk::Button *m_pForwardButton;
		unsigned int m_docsCount;
		unsigned int m_firstDoc;
		sigc::signal2<void, Glib::ustring, Glib::ustring> m_signalQueryChanged;
		sigc::signal1<void, Glib::ustring> m_signalBackClicked;
		sigc::signal1<void, Glib::ustring> m_signalForwardClicked;
		sigc::connection m_queryChangedConnection;

		void onQueryChanged(void);

		void onBackClicked(void);

		void onForwardClicked(void);

	private:
		IndexPage(const IndexPage &other);
		IndexPage &operator=(const IndexPage &other);

};

#endif // _INDEXPAGE_HH
