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
#include "PinotUtils.h"
#include "PropertiesDialog.h"

using namespace std;
using namespace Glib;
using namespace Gtk;

PropertiesDialog::PropertiesDialog(_GtkDialog *&pParent, RefPtr<Builder>& refBuilder) :
	Dialog(pParent),
	cancelPropertiesButton(NULL),
	applyPropertiesButton(NULL),
	titleLabel(NULL),
	languageLabel(NULL),
	typeLabel(NULL),
	sizeLabel(NULL),
	termsLabel(NULL),
	titleEntry(NULL),
	typeEntry(NULL),
	languageCombobox(NULL),
	sizeEntry(NULL),
	termsEntry(NULL),
	bytesLabel(NULL),
	saveTermsButton(NULL),
	labelsTreeview(NULL),
	m_indexLocation(""),
	m_docId(0),
	m_notALanguageName(false),
	m_editDocument(false),
	m_changedInfo(false),
	m_changedLabels(false)
{
	refBuilder->get_widget("cancelPropertiesButton", cancelPropertiesButton);
	refBuilder->get_widget("applyPropertiesButton", applyPropertiesButton);
	refBuilder->get_widget("titleLabel", titleLabel);
	refBuilder->get_widget("languageLabel", languageLabel);
	refBuilder->get_widget("mimeTypeLabel", typeLabel);
	refBuilder->get_widget("sizeLabel", sizeLabel);
	refBuilder->get_widget("termsLabel", termsLabel);
	refBuilder->get_widget("titleEntry", titleEntry);
	refBuilder->get_widget("typeEntry", typeEntry);
	refBuilder->get_widget("languageCombobox", languageCombobox);
	refBuilder->get_widget("sizeEntry", sizeEntry);
	refBuilder->get_widget("termsEntry", termsEntry);
	refBuilder->get_widget("bytesLabel", bytesLabel);
	refBuilder->get_widget("saveTermsButton", saveTermsButton);
	refBuilder->get_widget("propLabelsTreeview", labelsTreeview);

	cancelPropertiesButton->signal_clicked().connect(sigc::mem_fun(*this, &PropertiesDialog::on_cancelPropertiesButton_clicked), false);
	applyPropertiesButton->signal_clicked().connect(sigc::mem_fun(*this, &PropertiesDialog::on_applyPropertiesButton_clicked), false);
	saveTermsButton->signal_clicked().connect(sigc::mem_fun(*this, &PropertiesDialog::on_saveTermsButton_clicked), false);

	// Associate the columns model to the labels tree
	m_refLabelsTree = ListStore::create(m_labelsColumns);
	labelsTreeview->set_model(m_refLabelsTree);
	labelsTreeview->append_column_editable(" ", m_labelsColumns.m_enabled);
	labelsTreeview->append_column(_("Labels"), m_labelsColumns.m_name);
	// Allow only single selection
	labelsTreeview->get_selection()->set_mode(SELECTION_SINGLE);
}

PropertiesDialog::~PropertiesDialog()
{
}

void PropertiesDialog::setDocuments(const string &indexLocation,
	const vector<DocumentInfo> &documentsList)
{
	set<string> docLabels;
	string title, language;
	char numStr[128];

	m_indexLocation = indexLocation;
	m_documentsList = documentsList;

	// If there's only one document selected, get its labels
	if (m_documentsList.size() == 1)
	{
		DocumentInfo docInfo(m_documentsList.front());
		unsigned int indexId = 0;
		unsigned int termsCount = 0;

		// Get the document ID
		m_docId = docInfo.getIsIndexed(indexId);
#ifdef DEBUG
		clog << "PropertiesDialog::PropertiesDialog: document " << m_docId << " in index " << indexId << endl;
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
			clog << "PropertiesDialog::PropertiesDialog: language " << language << endl;
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

void PropertiesDialog::populate_languageCombobox(const string &language)
{
	int unknownLanguagePos = 0;
	bool foundLanguage = false;

	if (m_notALanguageName == true)
	{
		languageCombobox->append(language);
		languageCombobox->set_active(0);
		unknownLanguagePos = 1;
	}

	// Add all supported languages
	for (unsigned int languageNum = 0; languageNum < Languages::m_count; ++languageNum)
	{
		string languageName(Languages::getIntlName(languageNum));

		languageCombobox->append(languageName);

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

void PropertiesDialog::populate_labelsTreeview(const set<string> &docLabels)
{
	string labelsString;

	// Populate the tree
	m_refLabelsTree->clear();
	set<string> &sysLabels = PinotSettings::getInstance().m_labels;
	for (set<string>::const_iterator labelIter = sysLabels.begin(); labelIter != sysLabels.end(); ++labelIter)
	{
		string labelName(*labelIter);

		// Create a new row
		TreeModel::iterator iter = m_refLabelsTree->append();
		TreeModel::Row row = *iter;

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
	clog << "PropertiesDialog::populate_labelsTreeview: showing " << docLabels.size()
		<< "/" << sysLabels.size() << " labels" << endl;
#endif

	// This will help determining if changes were made
	m_labelsHash = StringManip::hashString(labelsString);
}

const vector<DocumentInfo> &PropertiesDialog::getDocuments(void) const
{
	return m_documentsList;
}

void PropertiesDialog::setHeight(int maxHeight)
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

const set<string> &PropertiesDialog::getLabels(void) const
{
	return m_labels;
}

bool PropertiesDialog::changedInfo(void) const
{
	return m_changedInfo;
}

bool PropertiesDialog::changedLabels(void) const
{
	return m_changedLabels;
}

void PropertiesDialog::on_cancelPropertiesButton_clicked()
{
#ifdef DEBUG
	clog << "PropertiesDialog::on_cancelPropertiesButton_clicked: called" << endl;
#endif
	close();
}

void PropertiesDialog::on_applyPropertiesButton_clicked()
{
	string title, languageName(from_utf8(languageCombobox->get_active_text())), labelsString;
	int unknownLanguagePos = 0;
#ifdef DEBUG
	clog << "PropertiesDialog::on_applyPropertiesButton_clicked: called" << endl;
#endif

	// If only one document was edited, set its title
	if (m_editDocument == true)
	{
		vector<DocumentInfo>::iterator docIter = m_documentsList.begin();

		title = from_utf8(titleEntry->get_text());
		docIter->setTitle(title);
		// FIXME: find out if changes were actually made 
	}
#ifdef DEBUG
	clog << "PropertiesDialog::on_applyPropertiesButton_clicked: chosen title " << title << endl;
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
	clog << "PropertiesDialog::on_applyPropertiesButton_clicked: chosen language " << languageName << endl;
#endif
	if (m_infoHash != StringManip::hashString(title + languageName))
	{
#ifdef DEBUG
		clog << "PropertiesDialog::on_applyPropertiesButton_clicked: properties changed" << endl;
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
	clog << "PropertiesDialog::on_applyPropertiesButton_clicked: chosen " << m_labels.size() << " labels" << endl;
#endif
	if (m_labelsHash != StringManip::hashString(labelsString))
	{
#ifdef DEBUG
		clog << "PropertiesDialog::on_applyPropertiesButton_clicked: labels changed" << endl;
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

	close();
}

void PropertiesDialog::on_saveTermsButton_clicked()
{
	ustring location(PinotSettings::getInstance().getHomeDirectory());

	location += "/terms.txt";
#ifdef DEBUG
	clog << "PropertiesDialog::on_saveTermsButton_clicked: " << location << endl;
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

