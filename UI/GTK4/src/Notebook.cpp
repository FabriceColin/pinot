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
#include <gtkmm.h>

#include "config.h"
#include "NLS.h"
#include "PinotUtils.h"
#include "Notebook.h"

using namespace std;
using namespace Glib;
using namespace Gtk;

NotebookPageBox::NotebookPageBox(const ustring &title, NotebookPageBox::PageType type,
	PinotSettings &settings) :
	VBox(),
	m_title(title),
	m_type(type),
	m_settings(settings),
	m_pTree(NULL)
{
}

NotebookPageBox::~NotebookPageBox()
{
}

//
// Returns the page title.
//
ustring NotebookPageBox::getTitle(void) const
{
	return m_title;
}

//
// Returns the page type.
//
NotebookPageBox::PageType NotebookPageBox::getType(void) const
{
	return m_type;
}

//
// Returns the page's tree.
//
ResultsTree *NotebookPageBox::getTree(void) const
{
	return m_pTree;
}

ResultsPage::ResultsPage(const ustring &queryName, ResultsTree *pTree,
	int parentHeight, PinotSettings &settings) :
	NotebookPageBox(queryName, NotebookPageBox::RESULTS_PAGE, settings),
	m_pLabel(NULL),
	m_pCombobox(NULL),
	m_pYesButton(NULL),
	m_pCloseImage(NULL),
	m_pCloseButton(NULL),
	m_pHBox(NULL),
	m_pVBox(NULL),
	m_pVPaned(NULL)
{
	if (pTree != NULL)
	{
		m_pTree = pTree;
		m_pLabel = manage(new Label(_("Did you mean ?")));
		m_pCombobox = manage(new ComboBoxText());
		m_pYesButton = manage(new Button(StockID("gtk-yes")));
		m_pCloseImage = manage(new Image(StockID("gtk-close"), IconSize(ICON_SIZE_MENU)));
		m_pCloseButton = manage(new Button());

		m_pCloseImage->set_alignment(0, 0);
		m_pCloseImage->set_padding(0, 0);
		m_pCloseButton->set_relief(RELIEF_NONE);
		m_pCloseButton->set_border_width(0);
		m_pCloseButton->set_name("pinot-tab-close-button");
		m_pCloseButton->set_tooltip_text(_("Close"));
		m_pCloseButton->set_alignment(0, 0);
		m_pCloseButton->add(*m_pCloseImage);
		m_pHBox = manage(new HBox(false, 0));
		m_pHBox->pack_start(*m_pLabel, Gtk::PACK_SHRINK, 4);
		m_pHBox->pack_start(*m_pCombobox, Gtk::PACK_EXPAND_WIDGET, 4);
		m_pHBox->pack_start(*m_pYesButton, Gtk::PACK_SHRINK, 4);
		m_pHBox->pack_start(*m_pCloseButton, Gtk::PACK_SHRINK, 4);

		m_pVBox = manage(new VBox(false, 0));
		m_pVBox->pack_start(*m_pHBox, Gtk::PACK_SHRINK);
		m_pVBox->pack_start(*pTree->getResultsScrolledWindow());

		m_pVPaned = manage(new VPaned());
		m_pVPaned->set_can_focus();
		m_pVPaned->set_position(105);
		m_pVPaned->pack1(*m_pVBox, Gtk::EXPAND|Gtk::SHRINK);
		m_pVPaned->pack2(*pTree->getExtractScrolledWindow(), Gtk::SHRINK);
		pack_start(*m_pVPaned, Gtk::PACK_EXPAND_WIDGET);

		// Give the extract 2/10th of the height
		m_pVPaned->set_position((parentHeight * 8) / 10);

		// Hide suggestions by default
		onCloseButtonClicked();
		m_pVBox->show();
		m_pVPaned->show();

		m_pYesButton->signal_clicked().connect(
			sigc::mem_fun(*this, &ResultsPage::onYesButtonClicked), false);
		m_pCloseButton->signal_clicked().connect(
			sigc::mem_fun(*this, &ResultsPage::onCloseButtonClicked), false);
	}

	show();
}

ResultsPage::~ResultsPage()
{
}

void ResultsPage::onYesButtonClicked()
{
	if (m_pCombobox == NULL)
	{
		return;
	}

	m_signalSuggest(m_title, m_pCombobox->get_active_text());
}

void ResultsPage::onCloseButtonClicked()
{
	// Just hide these
	m_pLabel->hide();
	m_pCombobox->hide();
	m_pYesButton->hide();
	m_pCloseImage->hide();
	m_pCloseButton->hide();
	m_pHBox->hide();
}

// Returns the suggest signal.
sigc::signal2<void, ustring, ustring>& ResultsPage::getSuggestSignal(void)
{
	return m_signalSuggest;
}

//
// Append a suggestion.
//
bool ResultsPage::appendSuggestion(const ustring &text)
{
	bool appended = false;

	if ((text.empty() == false) &&
		(m_suggestions.find(text) == m_suggestions.end()))
	{
		ustring activeText(m_pCombobox->get_active_text());

#ifdef DEBUG
		clog << "ResultsPage::appendSuggestion: suggesting " << text << endl;
#endif
		m_suggestions.insert(text);
		m_pCombobox->prepend(text);
		if (activeText.empty() == true)
		{
			m_pCombobox->set_active(0);
		}
		else
		{
			m_pCombobox->set_active_text(activeText);
		}

		appended = true;
	}

	m_pLabel->show();
	m_pCombobox->show();
	m_pYesButton->show();
	m_pCloseImage->show();
	m_pCloseButton->show();
	m_pHBox->show();

	return appended;
}

bool NotebookTabBox::m_initialized = false;

NotebookTabBox::NotebookTabBox(const Glib::ustring &title, NotebookPageBox::PageType type) :
	HBox(),
	m_title(title),
	m_pageType(type),
	m_pTabLabel(NULL),
	m_pTabImage(NULL),
	m_pTabButton(NULL)
{
	if (m_initialized == false)
	{
		m_initialized = true;
	}

	m_pTabLabel = manage(new Label(title));
	m_pTabImage = manage(new Image(StockID("gtk-close"), IconSize(ICON_SIZE_MENU)));
	m_pTabButton = manage(new Button());

	m_pTabLabel->set_alignment(0, 0.5);
	m_pTabLabel->set_padding(0, 0);
	m_pTabLabel->set_justify(JUSTIFY_LEFT);
	m_pTabLabel->set_line_wrap(false);
	m_pTabLabel->set_use_markup(false);
	m_pTabLabel->set_selectable(false);
	m_pTabImage->set_alignment(0, 0);
	m_pTabImage->set_padding(0, 0);
	m_pTabButton->set_relief(RELIEF_NONE);
	m_pTabButton->set_border_width(0);
	m_pTabButton->set_name("pinot-tab-close-button");
	m_pTabButton->set_tooltip_text(_("Close"));
	m_pTabButton->set_alignment(0, 0);
	m_pTabButton->add(*m_pTabImage);
	pack_start(*m_pTabLabel);
	pack_start(*m_pTabButton, PACK_SHRINK);
	set_spacing(0);
	set_homogeneous(false);
	m_pTabLabel->show();
	m_pTabImage->show();
	m_pTabButton->show();
	show();

	m_pTabButton->signal_clicked().connect(
		sigc::mem_fun(*this, &NotebookTabBox::onButtonClicked));
}

NotebookTabBox::~NotebookTabBox()
{
}

void NotebookTabBox::onButtonClicked(void)
{
	m_signalClose(m_title, m_pageType);
}

//
// Returns the close signal.
//
sigc::signal2<void, ustring, NotebookPageBox::PageType>& NotebookTabBox::getCloseSignal(void)
{
	return m_signalClose;
}
