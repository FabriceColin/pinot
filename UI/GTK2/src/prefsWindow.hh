/*
 *  Copyright 2008-2012 Fabrice Colin
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

#ifndef _PREFSWINDOW_HH
#define _PREFSWINDOW_HH

#include <string>
#include <map>
#include <set>
#include <vector>
#include <glibmm/refptr.h>
#include <glibmm/ustring.h>
#include <gtkmm/liststore.h>

#include "PinotSettings.h"
#include "ModelColumns.hh"
#include "UIThreads.hh"
#include "prefsWindow_glade.hh"

class prefsWindow : public prefsWindow_glade
{  
public:
	prefsWindow();
	virtual ~prefsWindow();

protected:
	PinotSettings &m_settings;
	Glib::RefPtr<Gtk::ListStore> m_refViewTree;
	LabelModelColumns m_labelsColumns;
	Glib::RefPtr<Gtk::ListStore> m_refLabelsTree;
	IndexableModelColumns m_directoriesColumns;
	Glib::RefPtr<Gtk::ListStore> m_refDirectoriesTree;
	TimestampedModelColumns m_mailColumns;
	TimestampedModelColumns m_patternsColumns;
	Glib::RefPtr<Gtk::ListStore> m_refPatternsTree;
	std::vector<Gtk::Entry *> m_editableValueEntries;
	std::set<std::string> m_addedLabels;
	std::set<std::string> m_deletedLabels;
	std::set<std::string> m_deletedDirectories;
	std::string m_directoriesHash;
	std::string m_patternsHash;
	class InternalState : public QueueManager
	{
	public:
		InternalState(prefsWindow *pWindow);
		~InternalState();

		bool m_savedPrefs;

	} m_state;

	virtual void on_prefsCancelbutton_clicked();
	virtual void on_prefsOkbutton_clicked();
	virtual void on_addDirectoryButton_clicked();
	virtual void on_removeDirectoryButton_clicked();
	virtual void on_patternsCombobox_changed();
	virtual void on_addPatternButton_clicked();
	virtual void on_removePatternButton_clicked();
	virtual void on_resetPatternsButton_clicked();
	virtual void on_addLabelButton_clicked();
	virtual void on_removeLabelButton_clicked();
	virtual void on_directConnectionRadiobutton_toggled();
	virtual bool on_prefsWindow_delete_event(GdkEventAny *ev);

	void on_thread_end(WorkerThread *pThread);
	void updateLabelRow(const Glib::ustring &path_string, const Glib::ustring &text);
	void renderLabelNameColumn(Gtk::CellRenderer *pRenderer, const Gtk::TreeModel::iterator &iter);
	void attach_value_widgets(const std::string &name, const std::string &value, guint rowNumber);
	void populate_proxyTypeCombobox();
	void populate_labelsTreeview();
	void save_labelsTreeview();
	void populate_directoriesTreeview();
	bool save_directoriesTreeview();
	void populate_patternsCombobox();
	void populate_patternsTreeview(const std::set<Glib::ustring> &patternsList, bool isBlackList);
	bool save_patternsTreeview();

};

#endif
