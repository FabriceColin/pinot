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

#include <stdlib.h>
#include <iostream>
#include <algorithm>
#include <glibmm/convert.h>
#include <gdkmm/color.h>
#include <gtkmm/colorselection.h>
#include <gtkmm/label.h>
#include <gtkmm/entry.h>
#include <gtkmm/menu.h>
#include <gtkmm/cellrenderertext.h>
#include <gtkmm/messagedialog.h>
#include <gtkmm/main.h>

#include "config.h"
#include "NLS.h"
#include "StringManip.h"
#ifdef HAVE_DBUS
#include "DBusIndex.h"
#endif
#include "ModuleFactory.h"
#include "PinotUtils.hh"
#include "prefsWindow.hh"

using namespace std;
using namespace Glib;
using namespace Gdk;
using namespace Gtk;

#ifdef HAVE_DBUS
class StartDaemonThread : public WorkerThread
{
	public:
		StartDaemonThread() :
			WorkerThread()
		{
		}
		virtual ~StartDaemonThread()
		{
		}

		virtual std::string getType(void) const
		{
			return "StartDaemonThread";
		}

	protected:
		virtual void doWork(void)
		{
			// Save the settings
			PinotSettings::getInstance().save(PinotSettings::SAVE_PREFS);

			// Ask the daemon to reload its configuration
			// Let D-Bus activate the service if necessary
			DBusIndex::reload();
		}

	private:
		StartDaemonThread(const StartDaemonThread &other);
		StartDaemonThread &operator=(const StartDaemonThread &other);

};
#endif

class GetLabelsThread : public WorkerThread
{
	public:
		GetLabelsThread() :
			WorkerThread()
		{
		}
		virtual ~GetLabelsThread()
		{
		}

		virtual std::string getType(void) const
		{
			return "GetLabelsThread";
		}

	protected:
		virtual void doWork(void)
		{
			set<string> &labels = PinotSettings::getInstance().m_labels;

			// Get labels directly from the daemon's index
			IndexInterface *pDaemonIndex = PinotSettings::getInstance().getIndex(PinotSettings::getInstance().m_daemonIndexLocation);
			if (pDaemonIndex != NULL)
			{
				set<string> indexLabels;

				// Nothing might be found if we are upgrading from an older version
				// and the daemon has not been run
				if (pDaemonIndex->getLabels(indexLabels) == true)
				{
					labels.clear();

					copy(indexLabels.begin(), indexLabels.end(),
						inserter(labels, labels.begin()));
				}
#ifdef DEBUG
				else clog << "GetLabelsThread::doWork: relying on configuration file" << endl;
#endif

				delete pDaemonIndex;
			}
		}

	private:
		GetLabelsThread(const GetLabelsThread &other);
		GetLabelsThread &operator=(const GetLabelsThread &other);

};

prefsWindow::InternalState::InternalState(prefsWindow *pWindow) :
        QueueManager(PinotSettings::getInstance().m_docsIndexLocation),
	m_savedPrefs(false)
{
        m_onThreadEndSignal.connect(sigc::mem_fun(*pWindow, &prefsWindow::on_thread_end));
}

prefsWindow::InternalState::~InternalState()
{
}

prefsWindow::prefsWindow() :
	prefsWindow_glade(),
	m_settings(PinotSettings::getInstance()),
	m_state(this)
{
	Color newColour;
	ustring desktopName(_("File Indexing and Search"));
	ustring desktopComment(_("Configure Pinot to index your files"));

	newColour.set_red(m_settings.m_newResultsColourRed);
	newColour.set_green(m_settings.m_newResultsColourGreen);
	newColour.set_blue(m_settings.m_newResultsColourBlue);

	// Initialize widgets
	// Ignore robots directives
	ignoreRobotsCheckbutton->set_active(m_settings.m_ignoreRobotsDirectives);
	// Google API key
	if (m_settings.m_googleAPIKey.empty() == false)
	{
		apiKeyEntry->set_text(m_settings.m_googleAPIKey);
	}
	// New results colour
	newResultsColorbutton->set_color(newColour);
	// Enable terms suggestion
	enableCompletionCheckbutton->set_active(m_settings.m_suggestQueryTerms);

	// Any plugin editable parameter ?
	if (m_settings.m_editablePluginValues.empty() == false)
	{
		Glib::PropertyProxy<guint> columnsProp(generalTable->property_n_columns());
		Glib::PropertyProxy<guint> rowsProp(generalTable->property_n_rows());
		guint rowsCount = rowsProp.get_value();

#ifdef DEBUG
		clog << "prefsWindow: adding " << m_settings.m_editablePluginValues.size() << " more rows" << endl;
#endif
		generalTable->resize(rowsCount + m_settings.m_editablePluginValues.size(), columnsProp.get_value());

		for (std::map<string, string>::const_iterator valueIter = m_settings.m_editablePluginValues.begin();
			valueIter != m_settings.m_editablePluginValues.end(); ++valueIter)
		{
			++rowsCount;
			attach_value_widgets(valueIter->first, valueIter->second, rowsCount);
		}
	}

	populate_proxyTypeCombobox();
	proxyRadiobutton->set_active(m_settings.m_proxyEnabled);
	proxyAddressEntry->set_text(m_settings.m_proxyAddress);
	proxyPortSpinbutton->set_value((double)m_settings.m_proxyPort);
	int proxyType = 0;
	if (m_settings.m_proxyType == "SOCKS4")
	{
		proxyType = 1;
	}
	else if (m_settings.m_proxyType == "SOCKS5")
	{
		proxyType = 2;
	}
	proxyTypeCombobox->set_active(proxyType);
	on_directConnectionRadiobutton_toggled();

	// Associate the columns model to the labels tree
	m_refLabelsTree = ListStore::create(m_labelsColumns);
	labelsTreeview->set_model(m_refLabelsTree);
	TreeViewColumn *pColumn = new TreeViewColumn(_("Name"));
	CellRendererText *pTextRenderer = new CellRendererText();
	pTextRenderer->signal_edited().connect(sigc::mem_fun(*this, &prefsWindow::updateLabelRow));
	pColumn->pack_start(*manage(pTextRenderer));
	pColumn->set_cell_data_func(*pTextRenderer, sigc::mem_fun(*this, &prefsWindow::renderLabelNameColumn));
	pColumn->add_attribute(pTextRenderer->property_text(), m_labelsColumns.m_name);
	pColumn->set_resizable(true);
	pColumn->set_sort_column(m_labelsColumns.m_name);
	labelsTreeview->append_column(*manage(pColumn));
	// Allow only single selection
	labelsTreeview->get_selection()->set_mode(SELECTION_SINGLE);
	// Prevent adding/removing labels until we have the list
	addLabelButton->set_sensitive(false);
	removeLabelButton->set_sensitive(false);

	// Associate the columns model to the directories tree
	m_refDirectoriesTree = ListStore::create(m_directoriesColumns);
	directoriesTreeview->set_model(m_refDirectoriesTree);
	directoriesTreeview->append_column_editable(_("Monitor"), m_directoriesColumns.m_monitor);
	directoriesTreeview->append_column(_("Location"), m_directoriesColumns.m_location);
	// Allow only single selection
	directoriesTreeview->get_selection()->set_mode(SELECTION_SINGLE);
	populate_directoriesTreeview();

	// Associate the columns model to the file patterns tree
	m_refPatternsTree = ListStore::create(m_patternsColumns);
	patternsTreeview->set_model(m_refPatternsTree);
	patternsTreeview->append_column_editable(_("Pattern"), m_patternsColumns.m_location);
	// Allow only single selection
	patternsTreeview->get_selection()->set_mode(SELECTION_SINGLE);
	populate_patternsCombobox();
	populate_patternsTreeview(m_settings.m_filePatternsList, m_settings.m_isBlackList);

	// Hide the Google API entry field ?
	if (ModuleFactory::isSupported("googleapi") == false)
	{
		apiKeyLabel->hide();
		apiKeyEntry->hide();
	}

	// Connect to threads' finished signal
	m_state.connect();

	// Get the labels
	GetLabelsThread *pThread = new GetLabelsThread();
	if (m_state.start_thread(pThread) == false)
	{
		delete pThread;

		populate_labelsTreeview();
	}
}

prefsWindow::~prefsWindow()
{
	if (m_state.m_savedPrefs == false)
	{
		// Save the settings
		PinotSettings::getInstance().save(PinotSettings::SAVE_PREFS);
	}
}

void prefsWindow::updateLabelRow(const ustring &path_string, const ustring &text)
{
	Gtk::TreePath path(path_string);

	// Get the row
	TreeModel::iterator iter = m_refLabelsTree->get_iter(path);
	if (iter)
	{
		TreeRow row = *iter;

#ifdef DEBUG
		clog << "prefsWindow::updateLabelRow: set label to " << text << endl;
#endif
		// Set the value of the name column
		row.set_value(m_labelsColumns.m_name, (ustring)text);
	}
}

void prefsWindow::renderLabelNameColumn(CellRenderer *pRenderer, const TreeModel::iterator &iter)
{
	TreeModel::Row row = *iter;

	if (pRenderer == NULL)
	{
		return;
	}

	CellRendererText *pTextRenderer = dynamic_cast<CellRendererText*>(pRenderer);
	if (pTextRenderer != NULL)
	{
		bool isNewLabel = false;

		// Is this a new label ?
		if (row[m_labelsColumns.m_enabled] == false)
		{
			isNewLabel = true;
		}

		// Set the editable property
#ifdef GLIBMM_PROPERTIES_ENABLED
		pTextRenderer->property_editable() = isNewLabel;
#else
		pTextRenderer->set_property("editable", isNewLabel);
#endif
	}
}

void prefsWindow::on_thread_end(WorkerThread *pThread)
{
	bool canQuit = false;

	if (pThread == NULL)
	{
		return;
	}

	// Any thread still running ?
	unsigned int threadsCount = m_state.get_threads_count();

	// What type of thread was it ?
	string type = pThread->getType();
	// Did the thread fail ?
	string status = pThread->getStatus();
	if (status.empty() == false)
	{
#ifdef DEBUG
		clog << "prefsWindow::on_thread_end: " << status << endl;
#endif
		// FIXME: tell the user the thread failed
	}
	else if (type == "GetLabelsThread")
	{
		populate_labelsTreeview();
	}
	else if (type == "StartDaemonThread")
	{
		m_state.m_savedPrefs = true;

		if (threadsCount == 0)
		{
			canQuit = true;
		}
	}
	else if (type == "LabelUpdateThread")
	{
		if (threadsCount == 0)
		{
			canQuit = true;
		}
	}

	// Delete the thread
	delete pThread;

	if (canQuit == false)
	{
		// We might be able to run a queued action
		m_state.pop_queue();
	}
	else
	{
#ifdef DEBUG
		clog << "prefsWindow::on_thread_end: quitting" << endl;
#endif
		on_prefsWindow_delete_event(NULL);
	}
}

void prefsWindow::attach_value_widgets(const string &name, const string &value, guint rowNumber)
{
	Label *valueLabel = manage(new Label(name + ":"));
	Entry *valueEntry = manage(new Entry());

	// These settings are what Glade-- would use
	valueLabel->set_alignment(0,0.5); 
	valueLabel->set_padding(4,4);
	valueLabel->set_justify(Gtk::JUSTIFY_LEFT); 
	valueLabel->set_line_wrap(false);
	valueLabel->set_use_markup(false);
	valueLabel->set_selectable(false);
	valueLabel->set_ellipsize(Pango::ELLIPSIZE_NONE);
	valueLabel->set_width_chars(-1);
	valueLabel->set_angle(0);
	valueLabel->set_single_line_mode(false);

#if GTK_VERSION_LT(3, 0)
	valueEntry->set_flags(Gtk::CAN_FOCUS);
#else
	valueEntry->set_can_focus();
#endif
	valueEntry->set_visibility(true);
	valueEntry->set_editable(true);
	valueEntry->set_max_length(0);
	valueEntry->set_has_frame(true);
	valueEntry->set_activates_default(false);

	valueEntry->set_text(to_utf8(value));

	generalTable->attach(*valueLabel, 0, 1, rowNumber, rowNumber + 1, Gtk::FILL, Gtk::FILL, 0, 0);
	generalTable->attach(*valueEntry, 1, 2, rowNumber, rowNumber + 1, Gtk::EXPAND|Gtk::FILL, Gtk::FILL, 4, 4);

	m_editableValueEntries.push_back(valueEntry);

	valueLabel->show();
	valueEntry->show();
}

void prefsWindow::populate_proxyTypeCombobox()
{
#if GTK_VERSION_LT(3, 0)
	proxyTypeCombobox->append_text("HTTP");
	proxyTypeCombobox->append_text("SOCKS v4");
	proxyTypeCombobox->append_text("SOCKS v5");
#else
	proxyTypeCombobox->append("HTTP");
	proxyTypeCombobox->append("SOCKS v4");
	proxyTypeCombobox->append("SOCKS v5");
#endif
}

void prefsWindow::populate_labelsTreeview()
{
	TreeModel::iterator iter;
	TreeModel::Row row;
	set<string> &labels = m_settings.m_labels;

	// Now this can be enabled
	addLabelButton->set_sensitive(true);

	if (labels.empty() == true)
	{
		// This button will stay disabled until labels are added to the list
		removeLabelButton->set_sensitive(false);
		return;
	}

	// Populate the tree
	for (set<string>::const_iterator labelIter = labels.begin();
		labelIter != labels.end();
		++labelIter)
	{
		// Create a new row
		iter = m_refLabelsTree->append();
		row = *iter;
		// Set its name
		row[m_labelsColumns.m_name] = *labelIter;
		// This allows to differentiate existing labels from new labels the user may create
		row[m_labelsColumns.m_enabled] = true;
	}

	removeLabelButton->set_sensitive(true);
}

void prefsWindow::save_labelsTreeview()
{
	set<string> &labels = m_settings.m_labels;

	labels.clear();

	// Go through the labels tree
	TreeModel::Children children = m_refLabelsTree->children();
	if (children.empty() == false)
	{
		TreeModel::Children::iterator iter = children.begin();
		for (; iter != children.end(); ++iter)
		{
			TreeModel::Row row = *iter;
			ustring labelName(row[m_labelsColumns.m_name]);

			// Check user didn't recreate this label after having deleted it
			set<string>::iterator labelIter = m_deletedLabels.find(from_utf8(labelName));
			if (labelIter != m_deletedLabels.end())
			{
				m_deletedLabels.erase(labelIter);
			}
			// Is this a new label ?
			if (row[m_labelsColumns.m_enabled] == false)
			{
				m_addedLabels.insert(from_utf8(labelName));
			}

#ifdef DEBUG
			clog << "prefsWindow::save_labelsTreeview: " << labelName << endl;
#endif
			// Add this new label to the settings
			labels.insert(labelName);
		}
	}
}

void prefsWindow::populate_directoriesTreeview()
{
	TreeModel::iterator iter;
	TreeModel::Row row;
	ustring dirsString;

	if (m_settings.m_indexableLocations.empty() == true)
	{
		// This button will stay disabled until directories are added to the list
		removeDirectoryButton->set_sensitive(false);
		return;
	}

	// Populate the tree
	for (set<PinotSettings::IndexableLocation>::iterator dirIter = m_settings.m_indexableLocations.begin();
		dirIter != m_settings.m_indexableLocations.end();
		++dirIter)
	{
		// Create a new row
		iter = m_refDirectoriesTree->append();
		row = *iter;
		row[m_directoriesColumns.m_monitor] = dirIter->m_monitor;
		row[m_directoriesColumns.m_location] = dirIter->m_name;
		dirsString += dirIter->m_name + (dirIter->m_monitor == true ? "1" : "0") + "|";
	}

	m_directoriesHash = StringManip::hashString(dirsString);
	removeDirectoryButton->set_sensitive(true);
}

bool prefsWindow::save_directoriesTreeview()
{
	string dirsString;

	// Clear the current settings
	m_settings.m_indexableLocations.clear();

	// Go through the directories tree
	TreeModel::Children children = m_refDirectoriesTree->children();
	if (children.empty() == false)
	{
		TreeModel::Children::iterator iter = children.begin();
		for (; iter != children.end(); ++iter)
		{
			TreeModel::Row row = *iter;
			PinotSettings::IndexableLocation indexableLocation;

			// Add this new directory to the settings
			indexableLocation.m_monitor = row[m_directoriesColumns.m_monitor];
			indexableLocation.m_name = row[m_directoriesColumns.m_location];

			string dirLabel("file://");
			dirLabel += from_utf8(indexableLocation.m_name);

			// Check user didn't recreate this directory after having deleted it
			set<string>::iterator dirIter = m_deletedDirectories.find(dirLabel);
			if (dirIter != m_deletedDirectories.end())
			{
				m_deletedDirectories.erase(dirIter);
			}

#ifdef DEBUG
			clog << "prefsWindow::save_directoriesTreeview: " << indexableLocation.m_name << endl;
#endif
			m_settings.m_indexableLocations.insert(indexableLocation);
			dirsString += indexableLocation.m_name + (indexableLocation.m_monitor == true ? "1" : "0") + "|";
		}
	}

	if (m_directoriesHash != StringManip::hashString(dirsString))
	{
#ifdef DEBUG
		clog << "prefsWindow::save_directoriesTreeview: directories changed" << endl;
#endif
		return true;
	}

	return false;
}

void prefsWindow::populate_patternsCombobox()
{
#if GTK_VERSION_LT(3, 0)
	patternsCombobox->append_text(_("Exclude these patterns from indexing"));
	patternsCombobox->append_text(_("Only index these patterns"));
#else
	patternsCombobox->append(_("Exclude these patterns from indexing"));
	patternsCombobox->append(_("Only index these patterns"));
#endif
}

void prefsWindow::populate_patternsTreeview(const set<ustring> &patternsList, bool isBlackList)
{
	TreeModel::iterator iter;
	TreeModel::Row row;
	ustring patternsString;

	if (patternsList.empty() == true)
	{
		// This button will stay disabled until a ppatern is added to the list
		removePatternButton->set_sensitive(false);
		return;
	}

	// Populate the tree
	for (set<ustring>::iterator patternIter = patternsList.begin();
		patternIter != patternsList.end();
		++patternIter)
	{
		ustring pattern(*patternIter);

		// Create a new row
		iter = m_refPatternsTree->append();
		row = *iter;
		// Set its name
		row[m_patternsColumns.m_location] = pattern;
		patternsString += pattern + "|";
	}

	removePatternButton->set_sensitive(true);

	// Is it a black or white list ?
	if (isBlackList == true)
	{
		patternsCombobox->set_active(0);
		patternsString += "0";
	}
	else
	{
		patternsCombobox->set_active(1);
		patternsString += "1";
	}

	m_patternsHash = StringManip::hashString(patternsString);
}

bool prefsWindow::save_patternsTreeview()
{
	ustring patternsString;

	// Clear the current settings
	m_settings.m_filePatternsList.clear();

	// Go through the file patterns tree
	TreeModel::Children children = m_refPatternsTree->children();
	if (children.empty() == false)
	{
		TreeModel::Children::iterator iter = children.begin();
		for (; iter != children.end(); ++iter)
		{
			TreeModel::Row row = *iter;
			ustring pattern(row[m_patternsColumns.m_location]);

			if (pattern.empty() == false)
			{
				m_settings.m_filePatternsList.insert(pattern);
				patternsString += pattern + "|";
			}
		}
	}
	if (patternsCombobox->get_active_row_number() == 0)
	{
		m_settings.m_isBlackList = true;
		patternsString += "0";
	}
	else
	{
		m_settings.m_isBlackList = false;
		patternsString += "1";
	}

	if (m_patternsHash != StringManip::hashString(patternsString))
	{
#ifdef DEBUG
		clog << "prefsWindow::save_patternsTreeview: patterns changed" << endl;
#endif
		return true;
	}

	return false;
}

void prefsWindow::on_prefsCancelbutton_clicked()
{
	on_prefsWindow_delete_event(NULL);
}

void prefsWindow::on_prefsOkbutton_clicked()
{
	bool startedThread = false;

	// Disable the buttons
	prefsCancelbutton->set_sensitive(false);
	prefsOkbutton->set_sensitive(false);

	// Synchronise widgets with settings
	m_settings.m_ignoreRobotsDirectives = ignoreRobotsCheckbutton->get_active();
	Color newColour = newResultsColorbutton->get_color();
	m_settings.m_newResultsColourRed = newColour.get_red();
	m_settings.m_newResultsColourGreen = newColour.get_green();
	m_settings.m_newResultsColourBlue = newColour.get_blue();
	m_settings.m_suggestQueryTerms = enableCompletionCheckbutton->get_active();
	m_settings.m_googleAPIKey = apiKeyEntry->get_text();
	// Any plugin editable parameter ?
	if (m_settings.m_editablePluginValues.empty() == false)
	{
		std::map<string, string>::iterator valueIter = m_settings.m_editablePluginValues.begin();
		vector<Entry *>::const_iterator entryIter = m_editableValueEntries.begin();
		while ((valueIter != m_settings.m_editablePluginValues.end()) &&
			(entryIter != m_editableValueEntries.end()))
		{
			ustring value((*entryIter)->get_text());

			valueIter->second = from_utf8(value);

			// Next
			++valueIter;
			++entryIter;
		}
	}

	m_settings.m_proxyEnabled = proxyRadiobutton->get_active();
	m_settings.m_proxyAddress = proxyAddressEntry->get_text();
	m_settings.m_proxyPort = (unsigned int)proxyPortSpinbutton->get_value();
	int proxyType = proxyTypeCombobox->get_active_row_number();
	if (proxyType == 1)
	{
		m_settings.m_proxyType = "SOCKS4";
	}
	else if (proxyType == 2)
	{
		m_settings.m_proxyType = "SOCKS5";
	}
	else
	{
		m_settings.m_proxyType = "HTTP";
	}

	// Validate the current lists
	save_labelsTreeview();
	bool startForDirectories = save_directoriesTreeview();
	bool startForPatterns = save_patternsTreeview();
#ifdef HAVE_DBUS
	if ((startForDirectories == true) ||
		(startForPatterns == true))
	{
		StartDaemonThread *pThread = new StartDaemonThread();

		if (m_state.start_thread(pThread) == false)
		{
			delete pThread;
		}
		else
		{
			startedThread = true;
		}
	}
#endif
	if ((m_addedLabels.empty() == false) ||
		(m_deletedLabels.empty() == false))
	{
		LabelUpdateThread *pThread = new LabelUpdateThread(m_addedLabels, m_deletedLabels);

		if (m_state.start_thread(pThread) == false)
		{
			delete pThread;
		}
		else
		{
			startedThread = true;
		}
	}

	if (startedThread == false)
	{
		on_prefsWindow_delete_event(NULL);
	}
	// FIXME: else, disable all buttons or provide some visual feedback, until threads are done
}

void prefsWindow::on_addDirectoryButton_clicked()
{
	ustring dirName;

	TreeModel::Children children = m_refDirectoriesTree->children();
	bool wasEmpty = children.empty();

	if (select_file_name(_("Directory to index"), dirName, true, true) == true)
	{
#ifdef DEBUG
		clog << "prefsWindow::on_addDirectoryButton_clicked: "
			<< dirName << endl;
#endif
		// Create a new entry in the directories list
		TreeModel::iterator iter = m_refDirectoriesTree->append();
		TreeModel::Row row = *iter;
	
		row[m_directoriesColumns.m_monitor] = false;
		row[m_directoriesColumns.m_location] = to_utf8(dirName);

		if (wasEmpty == true)
		{
			// Enable this button
			removeDirectoryButton->set_sensitive(true);
		}
	}
}

void prefsWindow::on_removeDirectoryButton_clicked()
{
	// Get the selected directory in the list
	TreeModel::iterator iter = directoriesTreeview->get_selection()->get_selected();
	if (iter)
	{
		string dirLabel("file://");

		// Unselect
		directoriesTreeview->get_selection()->unselect(iter);
		// Select another row
		TreeModel::Path dirPath = m_refDirectoriesTree->get_path(iter);
		dirPath.next();
		directoriesTreeview->get_selection()->select(dirPath);

		// Erase
		TreeModel::Row row = *iter;
		dirLabel += from_utf8(row[m_directoriesColumns.m_location]);
		m_deletedDirectories.insert(dirLabel);
		m_refDirectoriesTree->erase(row);

		TreeModel::Children children = m_refDirectoriesTree->children();
		if (children.empty() == true)
		{
			// Disable this button
			removeDirectoryButton->set_sensitive(false);
		}
	}
}

void prefsWindow::on_patternsCombobox_changed()
{
	int activeRow = patternsCombobox->get_active_row_number();

	if (((activeRow == 0) && (m_settings.m_isBlackList == true)) ||
		((activeRow > 0) && (m_settings.m_isBlackList == false)))
	{
		// No change
		return;
	}

	// Unselect
	patternsTreeview->get_selection()->unselect_all();
	// Remove all patterns in order to make sure the user enters a new bunch
	m_refPatternsTree->clear();
}

void prefsWindow::on_addPatternButton_clicked()
{
	TreeModel::Children children = m_refPatternsTree->children();
	bool wasEmpty = children.empty();

	// Create a new entry in the file patterns list
	TreeModel::iterator iter = m_refPatternsTree->append();
	TreeModel::Row row = *iter;

	row[m_patternsColumns.m_location] = "";
	row[m_patternsColumns.m_mTime] = time(NULL);

	// Select and start editing the row
	TreeViewColumn *pColumn = patternsTreeview->get_column(0);
	if (pColumn != NULL)
	{
		TreeModel::Path patternPath = m_refPatternsTree->get_path(iter);
		patternsTreeview->set_cursor(patternPath, *pColumn, true);
	}

	if (wasEmpty == true)
	{
		// Enable this button
		removePatternButton->set_sensitive(true);
	}
}

void prefsWindow::on_removePatternButton_clicked()
{
	// Get the selected file pattern in the list
	TreeModel::iterator iter = patternsTreeview->get_selection()->get_selected();
	if (iter)
	{
		// Unselect
		patternsTreeview->get_selection()->unselect(iter);
		// Select another row
		TreeModel::Path patternPath = m_refPatternsTree->get_path(iter);
		patternPath.next();
		patternsTreeview->get_selection()->select(patternPath);

		// Erase
		TreeModel::Row row = *iter;
		m_refPatternsTree->erase(row);

		TreeModel::Children children = m_refPatternsTree->children();
		if (children.empty() == true)
		{
			// Disable this button
			removePatternButton->set_sensitive(false);
		}
	}
}

void prefsWindow::on_resetPatternsButton_clicked()
{
	set<ustring> defaultPatterns;
	bool isBlackList = m_settings.getDefaultPatterns(defaultPatterns);

	// Unselect
	patternsTreeview->get_selection()->unselect_all();
	// Remove all patterns
	m_refPatternsTree->clear();

	// Repopulate with defaults
	populate_patternsTreeview(defaultPatterns, isBlackList);
}

void prefsWindow::on_addLabelButton_clicked()
{
	// Now create a new entry in the labels list
	TreeModel::iterator iter = m_refLabelsTree->append();
	TreeModel::Row row = *iter;
	row[m_labelsColumns.m_name] = to_utf8(_("New Label"));
	// This marks the label as new
	row[m_labelsColumns.m_enabled] = false;

	// Enable this button
	removeLabelButton->set_sensitive(true);
}

void prefsWindow::on_removeLabelButton_clicked()
{
	// Get the selected label in the list
	TreeModel::iterator iter = labelsTreeview->get_selection()->get_selected();
	if (iter)
	{
		// Unselect
		labelsTreeview->get_selection()->unselect(iter);
		// Select another row
		TreeModel::Path labelPath = m_refLabelsTree->get_path(iter);
		labelPath.next();
		labelsTreeview->get_selection()->select(labelPath);
		// Erase
		TreeModel::Row row = *iter;
		m_deletedLabels.insert(from_utf8(row[m_labelsColumns.m_name]));
		m_refLabelsTree->erase(row);

		TreeModel::Children children = m_refLabelsTree->children();
		if (children.empty() == true)
		{
			// Disable this button
			removeLabelButton->set_sensitive(false);
		}
	}
}

void prefsWindow::on_directConnectionRadiobutton_toggled()
{
	bool enabled = proxyRadiobutton->get_active();

	proxyAddressEntry->set_sensitive(enabled);
	proxyPortSpinbutton->set_sensitive(enabled);
	proxyTypeCombobox->set_sensitive(enabled);
}

bool prefsWindow::on_prefsWindow_delete_event(GdkEventAny *ev)
{
	// Any thread still running ?
	if (m_state.get_threads_count() > 0)
	{
		ustring boxMsg(_("At least one task hasn't completed yet. Quit now ?"));
		MessageDialog msgDialog(boxMsg, false, MESSAGE_QUESTION, BUTTONS_YES_NO);
		msgDialog.set_title(_("Quit"));
		msgDialog.set_transient_for(*this);
		msgDialog.show();
		int result = msgDialog.run();
		if (result == RESPONSE_NO)
		{
			return true;
		}

		m_state.disconnect();
		m_state.mustQuit(true);
	}
	else
	{
		m_state.disconnect();
	}

	Main::quit();
	return false;
}

