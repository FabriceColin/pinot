/*
 *  Copyright 2005-2009 Fabrice Colin
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

#ifndef _PROPERTIESDIALOG_HH
#define _PROPERTIESDIALOG_HH

#include <string>
#include <vector>
#include <set>
#include <glibmm/refptr.h>
#include <gtkmm/liststore.h>

#include "DocumentInfo.h"
#include "ModelColumns.hh"
#include "propertiesDialog_glade.hh"

class propertiesDialog : public propertiesDialog_glade
{  
public:
	propertiesDialog(const std::string &indexLocation,
	        std::vector<DocumentInfo> &documentsList);
	virtual ~propertiesDialog();

	void setHeight(int maxHeight);

	const std::set<std::string> &getLabels(void) const;

	bool changedInfo(void) const;

	bool changedLabels(void) const;

protected:
	LabelModelColumns m_labelsColumns;
	Glib::RefPtr<Gtk::ListStore> m_refLabelsTree;
	std::string m_indexLocation;
	std::vector<DocumentInfo> &m_documentsList;
	std::set<std::string> m_labels;
	unsigned int m_docId;
	bool m_notALanguageName;
	bool m_editDocument;
	std::string m_infoHash;
	std::string m_labelsHash;
	bool m_changedInfo;
	bool m_changedLabels;

	void populate_languageCombobox(const std::string &language);

	void populate_labelsTreeview(const std::set<std::string> &docLabels);

	virtual void on_labelOkButton_clicked();
	virtual void on_saveTermsButton_clicked();

};
#endif
