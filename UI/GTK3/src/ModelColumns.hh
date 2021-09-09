/*
 *  Copyright 2005-2011 Fabrice Colin
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

#ifndef _MODELCOLUMNS_HH
#define _MODELCOLUMNS_HH

#include <time.h>
#include <sys/types.h>
#include <glibmm/refptr.h>
#include <glibmm/ustring.h>
#include <gdkmm/event.h>
#include <gtkmm/treemodel.h>
#include <gtkmm/treemodelcolumn.h>

#include "QueryProperties.h"

/// Model column for text combo boxes.
class ComboModelColumns : public Gtk::TreeModel::ColumnRecord
{
	public:
		ComboModelColumns();
		virtual ~ComboModelColumns();

		Gtk::TreeModelColumn<Glib::ustring> m_name;

};

/// Main window, model column for the search engines tree.
class EnginesModelColumns : public Gtk::TreeModel::ColumnRecord
{
	public:
		EnginesModelColumns();
		virtual ~EnginesModelColumns();

		Gtk::TreeModelColumn<Glib::ustring> m_name;
		Gtk::TreeModelColumn<Glib::ustring> m_engineName;
		Gtk::TreeModelColumn<Glib::ustring> m_option;
		typedef enum { ENGINE_SEPARATOR = 0, ENGINE_FOLDER,
			WEB_ENGINE, INTERNAL_INDEX_ENGINE, INDEX_ENGINE } EngineType;
		Gtk::TreeModelColumn<EngineType> m_type;

};

/// Main window, model column for the queries tree.
class QueryModelColumns : public Gtk::TreeModel::ColumnRecord
{
public:
	QueryModelColumns();
	virtual ~QueryModelColumns();

	Gtk::TreeModelColumn<Glib::ustring> m_name;
	Gtk::TreeModelColumn<Glib::ustring> m_lastRun;
	Gtk::TreeModelColumn<time_t> m_lastRunTime;
	Gtk::TreeModelColumn<Glib::ustring> m_summary;
	Gtk::TreeModelColumn<QueryProperties> m_properties;

};

/// Main window, model column for the search results tree.
class ResultsModelColumns : public Gtk::TreeModel::ColumnRecord
{
public:
	ResultsModelColumns();
	virtual ~ResultsModelColumns();

	Gtk::TreeModelColumn<Glib::ustring> m_text;
	Gtk::TreeModelColumn<Glib::ustring> m_url;
	Gtk::TreeModelColumn<bool> m_indexed;
	Gtk::TreeModelColumn<bool> m_viewed;
	Gtk::TreeModelColumn<int> m_rankDiff;
	Gtk::TreeModelColumn<int> m_score;
	Gtk::TreeModelColumn<Glib::ustring> m_scoreText;
	Gtk::TreeModelColumn<unsigned int> m_engines;
	Gtk::TreeModelColumn<unsigned int> m_indexes;
	Gtk::TreeModelColumn<unsigned int> m_docId;
	typedef enum { ROW_ENGINE = 0, ROW_HOST, ROW_RESULT, ROW_OTHER } RowType;
	Gtk::TreeModelColumn<RowType> m_resultType;
	Gtk::TreeModelColumn<Glib::ustring> m_timestamp;
	Gtk::TreeModelColumn<time_t> m_timestampTime;
	Gtk::TreeModelColumn<std::string> m_serial;

};

/// Preferences window, model column for the Xapian indexes tree.
class OtherIndexModelColumns : public Gtk::TreeModel::ColumnRecord
{
public:
	OtherIndexModelColumns();
	virtual ~OtherIndexModelColumns();

	Gtk::TreeModelColumn<Glib::ustring> m_name;
	Gtk::TreeModelColumn<Glib::ustring> m_location;

};

/// Preferences window, model column for the labels tree.
/// Export/import window, model column for the labels tree.
class LabelModelColumns : public Gtk::TreeModel::ColumnRecord
{
public:
	LabelModelColumns();
	virtual ~LabelModelColumns();

	Gtk::TreeModelColumn<bool> m_enabled;
	Gtk::TreeModelColumn<Glib::ustring> m_name;

};

/// Preferences window, model column for the mail accounts tree.
class TimestampedModelColumns : public Gtk::TreeModel::ColumnRecord
{
public:
	TimestampedModelColumns();
	virtual ~TimestampedModelColumns();

	Gtk::TreeModelColumn<Glib::ustring> m_location;
	Gtk::TreeModelColumn<time_t> m_mTime;

};

/// Preferences window, model column for the directories tree.
class IndexableModelColumns : public Gtk::TreeModel::ColumnRecord
{
public:
	IndexableModelColumns();
	virtual ~IndexableModelColumns();

	Gtk::TreeModelColumn<bool> m_monitor;
	Gtk::TreeModelColumn<Glib::ustring> m_location;

};

#endif // _MODELCOLUMNS_HH
