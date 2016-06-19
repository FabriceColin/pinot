/*
 *  Copyright 2005-2012 Fabrice Colin
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

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <utility>
#include <cstring>

#include "config.h"
#include "Languages.h"
#include "MIMEScanner.h"
#include "StringManip.h"
#include "NLS.h"
#include "PinotSettings.h"
#include "PinotUtils.hh"
#include "propertiesDialog.hh"

using namespace std;
using namespace Glib;
using namespace Gtk;

propertiesDialog::propertiesDialog(const string &indexLocation,
	vector<DocumentInfo> &documentsList) :
	propertiesDialog_glade(),
	m_indexLocation(indexLocation),
	m_documentsList(documentsList),
	m_docId(0),
	m_notALanguageName(false),
	m_editDocument(false),
	m_changedInfo(false),
	m_changedLabels(false)
{
	set<string> docLabels;
	string title, language;
	char numStr[128];

	// Associate the columns model to the labels tree
	m_refLabelsTree = ListStore::create(m_labelsColumns);
	labelsTreeview->set_model(m_refLabelsTree);
	labelsTreeview->append_column_editable(" ", m_labelsColumns.m_enabled);
	labelsTreeview->append_column(_("Labels"), m_labelsColumns.m_name);
	// Allow only single selection
	labelsTreeview->get_selection()->set_mode(SELECTION_SINGLE);

	// If there's only one document selected, get its labels
	if (m_documentsList.size() == 1)
	{
		DocumentInfo docInfo(m_documentsList.front());
		unsigned int indexId = 0;
		unsigned int termsCount = 0;

		// Get the document ID
		m_docId = docInfo.getIsIndexed(indexId);
#ifdef DEBUG
		clog << "propertiesDialog::propertiesDialog: document " << m_docId << " in index " << indexId << endl;
#endif
		if (m_docId > 0)
		{
			IndexInterface *pIndex = PinotSettings::getInstance().getIndex(m_indexLocation);
			if (pIndex != NULL)
			{
				// Get the document's labels
				pIndex->getDocumentLabels(m_docId, docLabels);
				// Get the number of terms
				termsCount = pIndex->getDocumentTermsCount(m_docId);

				delete pIndex;
			}
			title = docInfo.getTitle();
			language = docInfo.getLanguage();

			snprintf(numStr, 128, "%u", m_docId);
			set_title(get_title() + " (ID " + numStr + ")");

			titleEntry->set_text(to_utf8(title));
			typeEntry->set_text(to_utf8(MIMEScanner::getDescription(docInfo.getType())));

			unsigned int size = docInfo.getSize();
			snprintf(numStr, 128, "%u", size);
			sizeEntry->set_text(numStr);

			snprintf(numStr, 128, "%u", termsCount);
			termsEntry->set_text(numStr);

			m_editDocument = true;
		}
	}
	else
	{
		string perDoc(_("Per document"));
		bool firstDoc = true;

		// If all documents are of the same language, show it
		for (vector<DocumentInfo>::const_iterator docIter = m_documentsList.begin();
			docIter != m_documentsList.end(); ++docIter)
		{
#ifdef DEBUG
			clog << "propertiesDialog::propertiesDialog: language " << language << endl;
#endif
			if (firstDoc == true)
			{
				language = docIter->getLanguage();
				firstDoc = false;
			}
			else if (language != docIter->getLanguage())
			{
				language = perDoc;
				m_notALanguageName = true;
			}
		}
		if (language.empty() == true)
		{
			language = perDoc;
			m_notALanguageName = true;
		}

		// Hide these widgets
		titleLabel->hide();
		titleEntry->hide();
		languageLabel->hide();
		languageCombobox->hide();
		typeLabel->hide();
		typeEntry->hide();
		sizeLabel->hide();
		bytesLabel->hide();
		sizeEntry->hide();
		termsLabel->hide();
		termsEntry->hide();
		saveTermsButton->hide();
	}

	populate_languageCombobox(language);
	m_infoHash = StringManip::hashString(title + language);
	populate_labelsTreeview(docLabels);
}

propertiesDialog::~propertiesDialog()
{
}

void propertiesDialog::populate_languageCombobox(const string &language)
{
	int unknownLanguagePos = 0;
	bool foundLanguage = false;

	if (m_notALanguageName == true)
	{
#if GTK_VERSION_LT(3, 0)
		languageCombobox->append_text(language);
#else
		languageCombobox->append(language);
#endif
		languageCombobox->set_active(0);
		unknownLanguagePos = 1;
	}

	// Add all supported languages
	for (unsigned int languageNum = 0; languageNum < Languages::m_count; ++languageNum)
	{
		string languageName(Languages::getIntlName(languageNum));

#if GTK_VERSION_LT(3, 0)
		languageCombobox->append_text(languageName);
#else
		languageCombobox->append(languageName);
#endif
		// Is this the language we are looking for ?
		if ((m_notALanguageName == false) &&
			(language.empty() == false) &&
			(strncasecmp(languageName.c_str(), language.c_str(),
			min(languageName.length(), language.length())) == 0))
		{
			languageCombobox->set_active(languageNum + unknownLanguagePos);
			foundLanguage = true;
		}
	}

	// Did we find the given language ?
	if ((m_notALanguageName == false) &&
		(foundLanguage == false))
	{
		// The first language in the list should be unknown
		languageCombobox->set_active(unknownLanguagePos);
	}
}

void propertiesDialog::populate_labelsTreeview(const set<string> &docLabels)
{
	TreeModel::iterator iter;
	TreeModel::Row row;
	string labelsString;

	// Populate the tree
	set<string> &sysLabels = PinotSettings::getInstance().m_labels;
	for (set<string>::const_iterator labelIter = sysLabels.begin(); labelIter != sysLabels.end(); ++labelIter)
	{
		string labelName(*labelIter);

		// Create a new row
		iter = m_refLabelsTree->append();
		row = *iter;

		row[m_labelsColumns.m_name] = labelName;
		// Is it in the document labels list ?
		set<string>::const_iterator docLabelIter = find(docLabels.begin(), docLabels.end(), labelName);
		if (docLabelIter != docLabels.end())
		{
			// Yup
			row[m_labelsColumns.m_enabled] = true;
			labelsString += labelName + "|";
			m_labels.insert(labelName);
		}
		else
		{
			row[m_labelsColumns.m_enabled] = false;
		}
	}
#ifdef DEBUG
	clog << "propertiesDialog::populate_labelsTreeview: showing " << docLabels.size()
		<< "/" << sysLabels.size() << " labels" << endl;
#endif

	// This will help determining if changes were made
	m_labelsHash = StringManip::hashString(labelsString);
}

void propertiesDialog::setHeight(int maxHeight)
{
	// FIXME: there must be a better way to determine how high the tree should be
	// for all rows to be visible !
	int rowsCount = m_refLabelsTree->children().size();
	// By default, the tree is high enough for two rows
	if (rowsCount > 2)
	{
		int width, height;

		// What's the current size ?
		get_size(width, height);
		// Add enough room for the rows we need to show
		height += get_column_height(labelsTreeview) * (rowsCount - 1);
		// Resize
		resize(width, min(maxHeight, height));
	}
}

const set<string> &propertiesDialog::getLabels(void) const
{
	return m_labels;
}

bool propertiesDialog::changedInfo(void) const
{
	return m_changedInfo;
}

bool propertiesDialog::changedLabels(void) const
{
	return m_changedLabels;
}

void propertiesDialog::on_labelOkButton_clicked()
{
	string title, languageName(from_utf8(languageCombobox->get_active_text())), labelsString;
	int unknownLanguagePos = 0;

	// If only one document was edited, set its title
	if (m_editDocument == true)
	{
		vector<DocumentInfo>::iterator docIter = m_documentsList.begin();

		title = from_utf8(titleEntry->get_text());
		docIter->setTitle(title);
		// FIXME: find out if changes were actually made 
	}
#ifdef DEBUG
	clog << "propertiesDialog::on_labelOkButton_clicked: chosen title " << title << endl;
#endif

	// Did we add an extra string to the languages list ?
	int chosenLanguagePos = languageCombobox->get_active_row_number();
	if (m_notALanguageName == true)
	{
		unknownLanguagePos = 1;
	}
	if (chosenLanguagePos == unknownLanguagePos)
	{
		languageName.clear();
	}
#ifdef DEBUG
	clog << "propertiesDialog::on_labelOkButton_clicked: chosen language " << languageName << endl;
#endif
	if (m_infoHash != StringManip::hashString(title + languageName))
	{
#ifdef DEBUG
		clog << "propertiesDialog::on_labelOkButton_clicked: properties changed" << endl;
#endif
		m_changedInfo = true;
	}

	// Go through the labels tree
	m_labels.clear();
	TreeModel::Children children = m_refLabelsTree->children();
	if (children.empty() == false)
	{
		for (TreeModel::Children::iterator iter = children.begin(); iter != children.end(); ++iter)
		{
			TreeModel::Row row = *iter;

			bool enabled = row[m_labelsColumns.m_enabled];
			if (enabled == true)
			{
				ustring labelName = row[m_labelsColumns.m_name];
				labelsString += labelName + "|";
				m_labels.insert(from_utf8(labelName));
			}
		}
	}
#ifdef DEBUG
	clog << "propertiesDialog::on_labelOkButton_clicked: chosen " << m_labels.size() << " labels" << endl;
#endif
	if (m_labelsHash != StringManip::hashString(labelsString))
	{
#ifdef DEBUG
		clog << "propertiesDialog::on_labelOkButton_clicked: labels changed" << endl;
#endif
		m_changedLabels = true;
	}

	for (vector<DocumentInfo>::iterator docIter = m_documentsList.begin();
		docIter != m_documentsList.end(); ++docIter)
	{
		// Apply the new language if necessary
		if (chosenLanguagePos >= unknownLanguagePos)
		{
			docIter->setLanguage(from_utf8(languageName));
		}

		// Apply labels
		docIter->setLabels(m_labels);
	}
}

void propertiesDialog::on_saveTermsButton_clicked()
{
	ustring location(PinotSettings::getInstance().getHomeDirectory());

	location += "/terms.txt";
#ifdef DEBUG
	clog << "propertiesDialog::on_saveTermsButton_clicked: " << location << endl;
#endif

	IndexInterface *pIndex = PinotSettings::getInstance().getIndex(m_indexLocation);
	if (pIndex == NULL)
	{
		return;
	}

	if (select_file_name(_("Export Terms"), location, false) == true)
	{
		std::map<unsigned int, string> wordsMap;

		if ((location.empty() == false) &&
			(pIndex->getDocumentTerms(m_docId, wordsMap) == true))
		{
			ofstream termsFile;

			termsFile.open(location.c_str());
			if (termsFile.good() == true)
			{
				for (std::map<unsigned int, string>::const_iterator termIter = wordsMap.begin();
					termIter != wordsMap.end(); ++termIter)
				{
					termsFile << termIter->second << " ";
				}
			}
			termsFile.close();
		}
	}

	delete pIndex;
}

