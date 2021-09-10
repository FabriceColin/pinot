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
#include <glibmm/ustring.h>
#include <glibmm/convert.h>
#include <gtkmm/menu.h>

#include "config.h"
#include "NLS.h"
#include "Languages.h"
#include "TimeConverter.h"
#include "PinotUtils.hh"
#include "queryDialog.hh"

using namespace std;
using namespace Glib;
using namespace Gtk;

queryDialog::queryDialog(GtkDialog *&pParent, RefPtr<Builder>& refBuilder) :
	Dialog(pParent),
	queryCancelButton(NULL),
	queryOkButton(NULL),
	addFilterButton(NULL),
	nameEntry(NULL),
	filterCombobox(NULL),
	queryTextview(NULL),
	indexCheckbutton(NULL),
	labelNameCombobox(NULL),
	resultsCountSpinbutton(NULL),
	sortOrderCombobox(NULL),
	stemmingCombobox(NULL),
	indexNewCheckbutton(NULL),
	m_name(""),
	m_badName(true)
{
	refBuilder->get_widget("queryCancelButton", queryCancelButton);
	refBuilder->get_widget("queryOkButton", queryOkButton);
	refBuilder->get_widget("addFilterButton", addFilterButton);
	refBuilder->get_widget("queryNameEntry", nameEntry);
	refBuilder->get_widget("filterCombobox", filterCombobox);
	refBuilder->get_widget("queryTextview", queryTextview);
	refBuilder->get_widget("indexCheckbutton", indexCheckbutton);
	refBuilder->get_widget("labelNameCombobox", labelNameCombobox);
	refBuilder->get_widget("resultsCountSpinbutton", resultsCountSpinbutton);
	refBuilder->get_widget("sortOrderCombobox", sortOrderCombobox);
	refBuilder->get_widget("stemmingCombobox", stemmingCombobox);
	refBuilder->get_widget("indexNewCheckbutton", indexNewCheckbutton);

	queryCancelButton->signal_clicked().connect(sigc::mem_fun(*this, &queryDialog::on_queryCancelButton_clicked), false);
	queryOkButton->signal_clicked().connect(sigc::mem_fun(*this, &queryDialog::on_queryOkButton_clicked), false);
	nameEntry->signal_changed().connect(sigc::mem_fun(*this, &queryDialog::on_nameEntry_changed), false);
	addFilterButton->signal_clicked().connect(sigc::mem_fun(*this, &queryDialog::on_addFilterButton_clicked), false);
	indexCheckbutton->signal_toggled().connect(sigc::mem_fun(*this, &queryDialog::on_indexCheckbutton_toggled), false);
}

queryDialog::~queryDialog()
{
}

void queryDialog::setQueryProperties(const QueryProperties &properties)
{
	m_name = properties.getName();
	m_properties = properties;
#ifdef DEBUG
    clog << "queryDialog::setQueryProperties: editing " << m_properties.getName() << endl;
#endif

	// Name
	if (m_name.empty() == true)
	{
		queryOkButton->set_sensitive(false);
	}
	else
	{
		nameEntry->set_text(m_name);
	}
	// Query text
	RefPtr<TextBuffer> refBuffer = queryTextview->get_buffer();
	if (refBuffer)
	{
		refBuffer->set_text(m_properties.getFreeQuery());
	}
	// Maximum number of results
	resultsCountSpinbutton->set_value((double)m_properties.getMaximumResultsCount());
	// Index results
	QueryProperties::IndexWhat indexResults = m_properties.getIndexResults();
	if (indexResults == QueryProperties::NEW_RESULTS)
	{
		indexCheckbutton->set_active(true);
		indexNewCheckbutton->set_active(true);
	}
	else if (indexResults == QueryProperties::ALL_RESULTS)
	{
		indexCheckbutton->set_active(true);
		indexNewCheckbutton->set_active(false);
	}
	else
	{
		indexCheckbutton->set_active(false);
		indexNewCheckbutton->set_active(false);
		indexNewCheckbutton->set_sensitive(false);
	}

	// Populate
	populate_comboboxes();
}

bool queryDialog::is_separator(const RefPtr<TreeModel>& model, const TreeModel::iterator& iter)
{
	if (iter)
	{
		const TreeModel::Path queryPath = model->get_path(iter);
		string rowPath(from_utf8(queryPath.to_string()));
		unsigned int rowPos = 0;

		// FIXME: this is extremely hacky !
		if ((sscanf(rowPath.c_str(), "%u", &rowPos) == 1) &&
			(rowPos == 12))
		{
			return true;
		}
	}

	return false;
}

void queryDialog::populate_comboboxes()
{
	unsigned int labelNum = 1;

	// All supported filters
	filterCombobox->set_row_separator_func(sigc::mem_fun(*this, &queryDialog::is_separator));
	filterCombobox->append(_("Host name"));
	filterCombobox->append(_("File name"));
	filterCombobox->append(_("File extension"));
	filterCombobox->append(_("Title"));
	filterCombobox->append(_("URL"));
	filterCombobox->append(_("Directory"));
	filterCombobox->append(_("In URL"));
	filterCombobox->append(_("Path"));
	filterCombobox->append(_("Language code"));
	filterCombobox->append(_("MIME type"));
	filterCombobox->append(_("MIME class"));
	filterCombobox->append(_("Label"));
	// And separate numerical ranges
	filterCombobox->append("===");
	filterCombobox->append(_("Date"));
	filterCombobox->append(_("Time"));
	filterCombobox->append(_("Size"));
	filterCombobox->set_active(0);

	// Sort order
	sortOrderCombobox->append(_("By relevance"));
	sortOrderCombobox->append(_("By date"));
	if (m_properties.getSortOrder() == QueryProperties::DATE_DESC)
	{
		sortOrderCombobox->set_active(1);
	}
	else
	{
		sortOrderCombobox->set_active(0);
	}

	// Stemming language
	stemmingCombobox->append(_("None"));
	stemmingCombobox->set_active(0);
	string language(m_properties.getStemmingLanguage());
	for (unsigned int languageNum = 1; languageNum < Languages::m_count; ++languageNum)
	{
		ustring languageName(Languages::getIntlName(languageNum));

		stemmingCombobox->append(languageName);
		// Is this the language we are looking for ?
		if (language == languageName)
		{
			stemmingCombobox->set_active(languageNum);
		}
	}

	// Labels
	labelNameCombobox->append(_("None"));
	labelNameCombobox->set_active(0);
	// Add all labels to the label combo and select the one defined for the query
	set<string> &labels = PinotSettings::getInstance().m_labels;
	for (set<string>::const_iterator labelIter = labels.begin(); labelIter != labels.end(); ++labelIter)
	{
		string labelName(*labelIter);

		labelNameCombobox->append(to_utf8(labelName));
		if (labelName == m_properties.getLabelName())
		{
			labelNameCombobox->set_active(labelNum);
		}

		++labelNum;
	}
}

const QueryProperties &queryDialog::getQueryProperties(void) const
{
	return m_properties;
}

bool queryDialog::badName(void) const
{
	return m_badName;
}

void queryDialog::on_queryCancelButton_clicked()
{
#ifdef DEBUG
	clog << "queryDialog::on_queryCancelButton_clicked: called" << endl;
#endif
	close();
}

void queryDialog::on_queryOkButton_clicked()
{
	ustring newName(nameEntry->get_text());
#ifdef DEBUG
	clog << "queryDialog::on_queryOkButton_clicked: called" << endl;
#endif

	// Name
	m_properties.setName(newName);
	m_badName = false;
	// Did the name change ?
	if (m_name != newName)
	{
		const std::map<string, QueryProperties> &queries = PinotSettings::getInstance().getQueries();

		// Is it already used ?
		std::map<string, QueryProperties>::const_iterator queryIter = queries.find(newName);
		if (queryIter != queries.end())
		{
			// Yes, it is
			m_badName = true;
#ifdef DEBUG
			clog << "queryDialog::on_queryOkButton_clicked: name in use" << endl;
#endif
		}
	}

	// Query text
	RefPtr<TextBuffer> refBuffer = queryTextview->get_buffer();
	if (refBuffer)
	{
		m_properties.setFreeQuery(refBuffer->get_text());
	}
	// Maximum number of results
	m_properties.setMaximumResultsCount((unsigned int)resultsCountSpinbutton->get_value());
	// Sort order
	if (sortOrderCombobox->get_active_row_number() == 1)
	{
		m_properties.setSortOrder(QueryProperties::DATE_DESC);
	}
	else
	{
		m_properties.setSortOrder(QueryProperties::RELEVANCE);
	}
	// Stemming language
	string languageName(from_utf8(stemmingCombobox->get_active_text()));
	int chosenLanguagePos = stemmingCombobox->get_active_row_number();
	if (chosenLanguagePos == 0)
	{
		languageName.clear();
	}
	m_properties.setStemmingLanguage(languageName);
	// Index results
	bool indexResults = indexCheckbutton->get_active();
	bool indexNewResults = indexNewCheckbutton->get_active();
	if ((indexResults == true) &&
		(indexNewResults == true))
	{
		m_properties.setIndexResults(QueryProperties::NEW_RESULTS);
	}
	else if ((indexResults == true) &&
		(indexNewResults == false))
	{
		m_properties.setIndexResults(QueryProperties::ALL_RESULTS);
	}
	else
	{
		m_properties.setIndexResults(QueryProperties::NOTHING);
	}

	// Index label
	m_properties.setLabelName("");
	int chosenLabel = labelNameCombobox->get_active_row_number();
	if (chosenLabel > 0)
	{
		m_properties.setLabelName(from_utf8(labelNameCombobox->get_active_text()));
	}

	close();
}

void queryDialog::on_nameEntry_changed()
{
	ustring name = nameEntry->get_text();
	if (name.empty() == false)
	{
		queryOkButton->set_sensitive(true);
	}
	else
	{
		queryOkButton->set_sensitive(false);
	}
}

void queryDialog::on_addFilterButton_clicked()
{
	ustring filter;
	time_t timeT = time(NULL);
	struct tm *tm = localtime(&timeT);

	// What's the corresponding filter ?
	int chosenFilter = filterCombobox->get_active_row_number();
	// FIXME: should the filters be localized ?
	switch (chosenFilter)
	{
		case 0:
			filter = "site:localhost";
			break;
		case 1:
			filter = "file:xxx.txt";
			break;
		case 2:
			filter = "ext:txt";
			break;
		case 3:
			filter = "title:pinot";
			break;
		case 4:
			filter = "url:file:///home/xxx/yyy.txt";
			break;
		case 5:
			filter = "dir:/home/xxx";
			break;
		case 6:
			filter = "inurl:file:///home/xxx/yyy.tar.gz";
			break;
		case 7:
			filter = "path:Documents";
			break;
		case 8:
			filter = "lang:en";
			break;
		case 9:
			filter = "type:text/plain";
			break;
		case 10:
			filter = "class:text";
			break;
		case 11:
			filter = "label:New";
			break;
		case 12:
			// Separator
			break;
		case 13:
			filter = TimeConverter::toYYYYMMDDString(tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday);
			filter += "..20991231";
			break;
		case 14:
			filter = TimeConverter::toHHMMSSString(tm->tm_hour, tm->tm_min, tm->tm_sec);
			filter += "..235959";
			break;
		case 15:
			filter += "0..10240b";
			break;
		default:
			break;
	}

	RefPtr<TextBuffer> refBuffer = queryTextview->get_buffer();
	if (refBuffer)
	{
		ustring queryText = refBuffer->get_text();
		queryText += " ";
		queryText += filter;
		refBuffer->set_text(queryText);
	}
}

void queryDialog::on_indexCheckbutton_toggled()
{
	if (indexCheckbutton->get_active() == false)
	{
		indexNewCheckbutton->set_active(false);
		indexNewCheckbutton->set_sensitive(false);
	}
	else
	{
		indexNewCheckbutton->set_sensitive(true);
	}
}

