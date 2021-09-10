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

#include <stdlib.h>
#include <strings.h>
#include <iostream>
#include <algorithm>
#include <utility>
#include <glibmm/convert.h>
#include <gtkmm/stock.h>
#include <gtkmm/targetlist.h>

#include "config.h"
#include "DocumentInfo.h"
#include "MIMEScanner.h"
#include "NLS.h"
#include "Url.h"
#include "QueryHistory.h"
#include "PinotSettings.h"
#include "PinotUtils.h"
#include "ImportDialog.h"

using namespace std;
using namespace Glib;
using namespace Gdk;
using namespace Gtk;

ImportDialog::ImportDialog(_GtkDialog *&pParent, RefPtr<Builder>& refBuilder) :
	Dialog(pParent),
	cancelButton(NULL),
	importButton(NULL),
	titleEntry(NULL),
	titleLabel(NULL),
	labelNameCombobox(NULL),
	locationEntry(NULL),
	docTable(NULL),
	m_locationLength(0)
{
	refBuilder->get_widget("cancelButton", cancelButton);
	refBuilder->get_widget("importButton", importButton);
	refBuilder->get_widget("docTitleEntry", titleEntry);
	refBuilder->get_widget("docTitleLabel", titleLabel);
	refBuilder->get_widget("docLabelNameCombobox", labelNameCombobox);
	refBuilder->get_widget("docLocationEntry", locationEntry);
	refBuilder->get_widget("docTable", docTable);

	cancelButton->signal_clicked().connect(sigc::mem_fun(*this, &ImportDialog::on_cancelButton_clicked), false);
	importButton->signal_clicked().connect(sigc::mem_fun(*this, &ImportDialog::on_importButton_clicked), false);
	locationEntry->signal_changed().connect(sigc::mem_fun(*this, &ImportDialog::on_locationEntry_changed), false);

	// Enable completion on the location field
	m_refLocationList = ListStore::create(m_locationColumns);
	m_refLocationCompletion = EntryCompletion::create();
	m_refLocationCompletion->set_model(m_refLocationList);
	m_refLocationCompletion->set_text_column(m_locationColumns.m_name);
	m_refLocationCompletion->set_minimum_key_length(8);
	m_refLocationCompletion->set_popup_completion(true);
	locationEntry->set_completion(m_refLocationCompletion);

	// Specify drop targets
	locationEntry->drag_dest_set(Gtk::DEST_DEFAULT_ALL, DragAction(GDK_ACTION_COPY|GDK_ACTION_MOVE));
	locationEntry->drag_dest_add_uri_targets();
	// Connect to the drag data received signal
	locationEntry->signal_drag_data_received().connect(
		sigc::mem_fun(*this, &ImportDialog::on_data_received));

	// Populate
	populate_comboboxes();

	// Disable this button as long the location entry field is empty
	// The title field may remain empty
	importButton->set_sensitive(false);
}

ImportDialog::~ImportDialog()
{
}

const DocumentInfo &ImportDialog::getDocumentInfo(void) const
{
	return m_docInfo;
}

void ImportDialog::on_data_received(const RefPtr<DragContext> &context,
	int x, int y, const SelectionData &data, guint info, guint time)
{
	vector<ustring> droppedUris = data.get_uris();
	bool goodDrop = false;

	clog << "ImportDialog::on_data_received: data type "
		<< data.get_data_type() << " in format " << data.get_format() << endl;

	if (droppedUris.empty() == false)
	{
		locationEntry->set_text(droppedUris.front());

		goodDrop = true;
	}

	context->drag_finish(goodDrop, false, time);
}

void ImportDialog::populate_comboboxes(void)
{
	labelNameCombobox->append(_("None"));
	labelNameCombobox->set_active(0);

	// Add all labels
	set<string> &labels = PinotSettings::getInstance().m_labels;
	for (set<string>::const_iterator labelIter = labels.begin();
		labelIter != labels.end(); ++labelIter)
	{
		labelNameCombobox->append(to_utf8(*labelIter));
	}
}

void ImportDialog::on_cancelButton_clicked()
{
#ifdef DEBUG
	clog << "ImportDialog::on_cancelButton_clicked: called" << endl;
#endif
	close();
}

void ImportDialog::on_importButton_clicked()
{
	string title(from_utf8(titleEntry->get_text()));
	string location(from_utf8(locationEntry->get_text()));
	string labelName;
#ifdef DEBUG
	clog << "ImportDialog::on_importButton_clicked: called" << endl;
#endif

	// Label
	int chosenLabel = labelNameCombobox->get_active_row_number();
	if (chosenLabel > 0)
	{
		labelName = from_utf8(labelNameCombobox->get_active_text());
	}

	Url urlObj(location);
	m_docInfo = DocumentInfo(title, location, MIMEScanner::scanUrl(urlObj), "");

	// Any label ?
	if (labelName.empty() == false)
	{
		set<string> labels;

		labels.insert(labelName);
		m_docInfo.setLabels(labels);
	}

	close();
}

void ImportDialog::on_locationEntry_changed()
{
	ustring fileName = from_utf8(locationEntry->get_text());
	bool enableImport = true;

	if (fileName.empty() == false)
	{
		Url urlObj(fileName);

		// Check the URL is valid
		if (urlObj.getProtocol().empty() == true)
		{
			enableImport = false;
		}
		// FIXME: be more thorough
	}
	else
	{
		enableImport = false;
	}

	unsigned int locationLength = fileName.length();
	if (locationLength > 0)
	{
		// Enable the import button
		importButton->set_sensitive(true);
	}
	else
	{
		// Disable the import button
		importButton->set_sensitive(false);
	}

	// Reset the list
	m_refLocationList->clear();

	// If characters are being deleted or if the location is too short, don't
	// attempt to offer suggestions
	if ((m_locationLength > locationLength) ||
		(fileName.empty() == true) ||
		(m_refLocationCompletion->get_minimum_key_length() > (int)fileName.length()))
	{
		m_locationLength = locationLength;
		return;
	}
	m_locationLength = locationLength;

	// Get 10 URLs like this one
	QueryHistory queryHistory(PinotSettings::getInstance().getHistoryDatabaseName());
	set<string> suggestedUrls;
	queryHistory.findUrlsLike(fileName, 10, suggestedUrls);
	// Populate the list
	for (set<string>::iterator urlIter = suggestedUrls.begin();
		urlIter != suggestedUrls.end(); ++urlIter)
	{
		TreeModel::iterator iter = m_refLocationList->append();
		TreeModel::Row row = *iter;

		row[m_locationColumns.m_name] = to_utf8(*urlIter);
	}
}

