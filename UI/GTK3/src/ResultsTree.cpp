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

#include <ctype.h>
#include <iostream>
#include <gdkmm/color.h>
#include <gtkmm/alignment.h>
#include <gtkmm/box.h>
#include <gtkmm/buttonbox.h>
#include <gtkmm/image.h>
#include <gtkmm/label.h>
#include <gtkmm/stock.h>
#include <gtkmm/cellrendererprogress.h>

#include "config.h"
#include "NLS.h"
#include "StringManip.h"
#include "TimeConverter.h"
#include "Url.h"
#include "QueryHistory.h"
#include "ViewHistory.h"
#include "PinotSettings.h"
#include "PinotUtils.h"
#include "ResultsTree.h"

using namespace std;
using namespace Glib;
using namespace Gdk;
using namespace Gtk;

ResultsTree::ResultsTree(const ustring &queryName, Menu *pPopupMenu,
	GroupByMode groupMode, PinotSettings &settings) :
	TreeView(),
	m_treeName(queryName),
	m_pPopupMenu(pPopupMenu),
	m_pResultsScrolledwindow(NULL),
	m_settings(settings),
	m_pExtractScrolledwindow(NULL),
	m_extractTextView(NULL),
	m_showExtract(true),
	m_groupMode(groupMode)
{
	TreeViewColumn *pColumn = NULL;

	m_pResultsScrolledwindow = manage(new ScrolledWindow());
	m_pExtractScrolledwindow = manage(new ScrolledWindow());
	m_extractTextView = manage(new TextView());

	// This is the actual results tree
	set_events(Gdk::BUTTON_PRESS_MASK);
	set_can_focus();
	set_headers_clickable(true);
	set_headers_visible(true);
	set_rules_hint(true);
	set_reorderable(false);
	set_enable_search(true);
	get_selection()->set_mode(SELECTION_MULTIPLE);
	m_pResultsScrolledwindow->set_can_focus();
	m_pResultsScrolledwindow->set_border_width(4);
	m_pResultsScrolledwindow->set_shadow_type(SHADOW_NONE);
	m_pResultsScrolledwindow->set_policy(POLICY_AUTOMATIC, POLICY_AUTOMATIC);
	m_pResultsScrolledwindow->property_window_placement().set_value(CORNER_TOP_LEFT);
	m_pResultsScrolledwindow->add(*this);

	// That's for the extract view
	m_extractTextView->set_can_focus();
	m_extractTextView->set_editable(false);
	m_extractTextView->set_cursor_visible(false);
	m_extractTextView->set_pixels_above_lines(0);
	m_extractTextView->set_pixels_below_lines(0);
	m_extractTextView->set_pixels_inside_wrap(0);
	m_extractTextView->set_left_margin(0);
	m_extractTextView->set_right_margin(0);
	m_extractTextView->set_indent(0);
	m_extractTextView->set_wrap_mode(WRAP_WORD);
	m_extractTextView->set_justification(JUSTIFY_LEFT);
	RefPtr<TextTag> refBoldTag = TextBuffer::Tag::create("bold");
	refBoldTag->property_weight() = 900;
	RefPtr<TextBuffer> refBuffer = m_extractTextView->get_buffer();
	if (refBuffer)
	{
		refBuffer->get_tag_table()->add(refBoldTag); 
	}
	m_pExtractScrolledwindow->set_can_focus();
	m_pExtractScrolledwindow->set_border_width(4);
	m_pExtractScrolledwindow->set_shadow_type(SHADOW_NONE);
	m_pExtractScrolledwindow->set_policy(POLICY_AUTOMATIC, POLICY_AUTOMATIC);
	m_pExtractScrolledwindow->property_window_placement().set_value(CORNER_TOP_LEFT);
	m_pExtractScrolledwindow->add(*m_extractTextView);

	// Associate the columns model to the results tree
	m_refStore = TreeStore::create(m_resultsColumns);
	set_model(m_refStore);

	if (m_groupMode != FLAT)
	{
		// The first column is for the status icons
		pColumn = new TreeViewColumn("");
		// Pack an icon renderer for the viewed status
		CellRendererPixbuf *pIconRenderer = new CellRendererPixbuf();
		pColumn->pack_start(*manage(pIconRenderer), false);
		pColumn->set_cell_data_func(*pIconRenderer, sigc::mem_fun(*this, &ResultsTree::renderViewStatus));
		// Pack a second icon renderer for the indexed status
		pIconRenderer = new CellRendererPixbuf();
		pColumn->pack_start(*manage(pIconRenderer), false);
		pColumn->set_cell_data_func(*pIconRenderer, sigc::mem_fun(*this, &ResultsTree::renderIndexStatus));
		// And a third one for the ranking
		pIconRenderer = new CellRendererPixbuf();
		pColumn->pack_start(*manage(pIconRenderer), false);
		pColumn->set_cell_data_func(*pIconRenderer, sigc::mem_fun(*this, &ResultsTree::renderRanking));
		pColumn->set_sizing(TREE_VIEW_COLUMN_AUTOSIZE);
		append_column(*manage(pColumn));

		// This is the score column
		pColumn = new TreeViewColumn(_("Score"));
		CellRendererProgress *pProgressRenderer = new CellRendererProgress();
		pColumn->pack_start(*manage(pProgressRenderer));
		pColumn->add_attribute(pProgressRenderer->property_text(), m_resultsColumns.m_scoreText);
		pColumn->add_attribute(pProgressRenderer->property_value(), m_resultsColumns.m_score);
		pColumn->set_sizing(TREE_VIEW_COLUMN_AUTOSIZE);
		pColumn->set_sort_column(m_resultsColumns.m_score);
		append_column(*manage(pColumn));
	}

	// This is the title column
	pColumn = new TreeViewColumn(_("Title"));
	CellRendererText *pTextRenderer = new CellRendererText();
	pColumn->pack_start(*manage(pTextRenderer));
	pColumn->set_cell_data_func(*pTextRenderer, sigc::mem_fun(*this, &ResultsTree::renderTitleColumn));
	pColumn->add_attribute(pTextRenderer->property_text(), m_resultsColumns.m_text);
	pColumn->set_resizable(true);
	pColumn->set_sort_column(m_resultsColumns.m_text);
	append_column(*manage(pColumn));

	// URL
	pColumn = create_column(_("URL"), m_resultsColumns.m_url, false, true, m_resultsColumns.m_url);
	if (pColumn != NULL)
	{
		pColumn->set_sizing(TREE_VIEW_COLUMN_AUTOSIZE);
		append_column(*manage(pColumn));
	}

	// The last column is for the timestamp
	pColumn = create_column(_("Date"), m_resultsColumns.m_timestamp, false, true, m_resultsColumns.m_timestampTime);
	if (pColumn != NULL)
	{
		pColumn->set_sizing(TREE_VIEW_COLUMN_AUTOSIZE);
		append_column(*manage(pColumn));
	}
	
	// Connect the signals
	signal_button_press_event().connect_notify(
		sigc::mem_fun(*this, &ResultsTree::onButtonPressEvent));
	get_selection()->signal_changed().connect(
		sigc::mem_fun(*this, &ResultsTree::onSelectionChanged));

	// Enable interactive search
	set_search_column(m_resultsColumns.m_text.index());
	// Control which rows can be selected
	get_selection()->set_select_function(sigc::mem_fun(*this, &ResultsTree::onSelectionSelect));
	// Listen for style changes
	signal_style_updated().connect_notify(sigc::mem_fun(*this, &ResultsTree::onStyleChanged));

	// Render the icons
	m_indexedIconPixbuf = render_icon_pixbuf(Stock::INDEX, ICON_SIZE_MENU);
	m_viewededIconPixbuf = render_icon_pixbuf(Stock::YES, ICON_SIZE_MENU);
	m_upIconPixbuf = render_icon_pixbuf(Stock::GO_UP, ICON_SIZE_MENU);
	m_downIconPixbuf = render_icon_pixbuf(Stock::GO_DOWN, ICON_SIZE_MENU);

	// Show all
	show();
	m_pResultsScrolledwindow->show();
	m_extractTextView->show();
	m_pExtractScrolledwindow->show();
}

ResultsTree::~ResultsTree()
{
}

void ResultsTree::renderViewStatus(CellRenderer *pRenderer, const TreeModel::iterator &iter)
{
	TreeModel::Row row = *iter;

	if (pRenderer == NULL)
	{
		return;
	}

	CellRendererPixbuf *pIconRenderer = dynamic_cast<CellRendererPixbuf*>(pRenderer);
	if (pIconRenderer != NULL)
	{
		// Has this result been already viewed ?
		if ((row[m_resultsColumns.m_viewed] == true) &&
			(m_viewededIconPixbuf))
		{
			pIconRenderer->property_pixbuf() = m_viewededIconPixbuf;
		}
		else
		{
			pIconRenderer->property_pixbuf().reset_value();
		}
	}
}

void ResultsTree::renderIndexStatus(CellRenderer *pRenderer, const TreeModel::iterator &iter)
{
	TreeModel::Row row = *iter;

	if (pRenderer == NULL)
	{
		return;
	}

	CellRendererPixbuf *pIconRenderer = dynamic_cast<CellRendererPixbuf*>(pRenderer);
	if (pIconRenderer != NULL)
	{
		// Is this result indexed ?
		if ((row[m_resultsColumns.m_indexed] == true) &&
			(m_indexedIconPixbuf))
		{
			pIconRenderer->property_pixbuf() = m_indexedIconPixbuf;
		}
		else
		{
			pIconRenderer->property_pixbuf().reset_value();
		}
	}
}

void ResultsTree::renderRanking(CellRenderer *pRenderer, const TreeModel::iterator &iter)
{
	TreeModel::Row row = *iter;

	if (pRenderer == NULL)
	{
		return;
	}

	CellRendererPixbuf *pIconRenderer = dynamic_cast<CellRendererPixbuf*>(pRenderer);
	if (pIconRenderer != NULL)
	{
		int rankDiff = row[m_resultsColumns.m_rankDiff];

		// Has this result's score changed ?
		if ((rankDiff > 0) &&
			(rankDiff != 666))
		{
			pIconRenderer->property_pixbuf() = m_upIconPixbuf;
		}
		else if (rankDiff < 0)
		{
			pIconRenderer->property_pixbuf() = m_downIconPixbuf;
		}
		else
		{
			pIconRenderer->property_pixbuf().reset_value();
		}
	}
}

void ResultsTree::renderTitleColumn(CellRenderer *pRenderer, const TreeModel::iterator &iter)
{
	TreeModel::Row row = *iter;

	if (pRenderer == NULL)
	{
		return;
	}

	CellRendererText *pTextRenderer = dynamic_cast<CellRendererText*>(pRenderer);
	if (pTextRenderer != NULL)
	{
		// Is this result new ?
		if (row[m_resultsColumns.m_rankDiff] == 666)
		{
			Color newColour;

			newColour.set_red(m_settings.m_newResultsColourRed);
			newColour.set_green(m_settings.m_newResultsColourGreen);
			newColour.set_blue(m_settings.m_newResultsColourBlue);

			// Change the row's background
			pTextRenderer->property_background_gdk() = newColour;
		}
		else
		{
			pTextRenderer->property_background_gdk().reset_value();
		}

		ResultsModelColumns::RowType type = row[m_resultsColumns.m_resultType];
		if ((type == ResultsModelColumns::ROW_ENGINE) ||
			(type == ResultsModelColumns::ROW_HOST))
		{
			ustring markup("<b>");
			markup += row[m_resultsColumns.m_text];
			markup += "</b>";
			pTextRenderer->property_markup() = markup;
		}
	}
}

void ResultsTree::onButtonPressEvent(GdkEventButton *ev)
{
	// Check for popup click
	if ((ev->type == GDK_BUTTON_PRESS) &&
		(ev->button == 3) )
	{
		if (m_pPopupMenu != NULL)
		{
			m_pPopupMenu->popup(ev->button, ev->time);
		}
	}
	// Check for double clicks
	else if (ev->type == GDK_2BUTTON_PRESS)
	{
#ifdef DEBUG
		clog << "ResultsTree::onButtonPressEvent: double click on button " << ev->button << endl;
#endif
		m_signalDoubleClick();
	}
}

void ResultsTree::onSelectionChanged(void)
{
	m_signalSelectionChanged(m_treeName);
}

bool ResultsTree::onSelectionSelect(const RefPtr<TreeModel>& model,
	const TreeModel::Path& node_path, bool path_currently_selected)
{
	const TreeModel::iterator iter = model->get_iter(node_path);
	const TreeModel::Row row = *iter;

	// In flat mode, don't bother about the extract
	if ((path_currently_selected == false) &&
		(m_groupMode != FLAT))
	{
		// Is this an actual result ?
		ResultsModelColumns::RowType type = row[m_resultsColumns.m_resultType];
		if (type != ResultsModelColumns::ROW_RESULT)
		{
			return true;
		}

		RefPtr<TextBuffer> refBuffer = m_extractTextView->get_buffer();
		if (refBuffer)
		{
			ustring extract(findResultsExtract(row));
			ustring::size_type textPos = 0, boldPos = extract.find("<b>");

			// Clear the extract
			refBuffer->set_text("");

#ifdef DEBUG
			clog << "ResultsTree::onSelectionSelect: extract for " << row[m_resultsColumns.m_url] << endl;
#endif
			if (boldPos == ustring::npos)
			{
				refBuffer->set_text(extract);
			}
			else
			{
				TextBuffer::iterator bufferPos = refBuffer->begin();

				while (boldPos != ustring::npos)
				{
					bufferPos = refBuffer->insert(bufferPos, extract.substr(textPos, boldPos - textPos));

					textPos = boldPos + 3;
					boldPos = extract.find("</b>", textPos);
					if (boldPos == ustring::npos)
					{
						continue;
					}
					bufferPos = refBuffer->insert_with_tag(bufferPos, extract.substr(textPos, boldPos - textPos), "bold");

					// Next
					textPos = boldPos + 4;
					boldPos = extract.find("<b>", textPos);
				}

				if (textPos + 1 < extract.length())
				{
					bufferPos = refBuffer->insert(bufferPos, extract.substr(textPos, boldPos - textPos));
				}
			}
		}
	}

	return true;
}

void ResultsTree::onStyleChanged(void)
{
#ifdef DEBUG
	clog << "ResultsTree::onStyleChanged: called" << endl;
#endif
	// FIXME: find better icons :-)
	m_indexedIconPixbuf = render_icon_pixbuf(Stock::INDEX, ICON_SIZE_MENU);
	m_viewededIconPixbuf = render_icon_pixbuf(Stock::YES, ICON_SIZE_MENU);
	m_upIconPixbuf = render_icon_pixbuf(Stock::GO_UP, ICON_SIZE_MENU);
	m_downIconPixbuf = render_icon_pixbuf(Stock::GO_DOWN, ICON_SIZE_MENU);
}

//
// Returns the results scrolled window.
//
ScrolledWindow *ResultsTree::getResultsScrolledWindow(void) const
{
	return m_pResultsScrolledwindow;
}

//
// Returns the extract scrolled window.
//
ScrolledWindow *ResultsTree::getExtractScrolledWindow(void) const
{
	return m_pExtractScrolledwindow;
}

// Connects the focus signals.
void ResultsTree::connectFocusSignals(sigc::slot1<bool, GdkEventFocus*> focusSlot)
{
	signal_focus_in_event().connect(focusSlot);
	signal_focus_out_event().connect(focusSlot);
	m_extractTextView->signal_focus_in_event().connect(focusSlot);
	m_extractTextView->signal_focus_out_event().connect(focusSlot);
}

//
// Returns the extract tree.
//
bool ResultsTree::focusOnExtract(void) const
{
	return m_extractTextView->is_focus();
}

//
// Returns the extract.
//
ustring ResultsTree::getExtract(void) const
{
	ustring text;

	RefPtr<TextBuffer> refBuffer = m_extractTextView->get_buffer();
	if (refBuffer)
	{
		text = refBuffer->get_text();
	}

	return text;
}

//
// Adds a set of results.
// Returns true if something was added to the tree.
//
bool ResultsTree::addResults(const string &engineName, const vector<DocumentInfo> &resultsList,
	const string &charset, bool updateHistory)
{
	std::map<string, TreeModel::iterator> updatedGroups;
	ResultsModelColumns::RowType rootType;
	unsigned int count = 0;

	// What's the grouping criteria ?
	if (m_groupMode == BY_ENGINE)
	{
		// By search engine
		rootType = ResultsModelColumns::ROW_ENGINE;
	}
	else if (m_groupMode == BY_HOST)
	{
		// By host
		rootType = ResultsModelColumns::ROW_HOST;
	}

	unsigned int indexId = 0;
	unsigned int engineId = 0;

	// Find out what the search engine ID is
	if (engineName.empty() == false)
	{
		engineId = m_settings.getEngineId(engineName);
		if (engineId == 0)
		{
			// Chances are this engine is an index
			PinotSettings::IndexProperties indexProps = m_settings.getIndexPropertiesByName(engineName);
			if (indexProps.m_location.empty() == false)
			{
				// Yes, it is
				indexId = indexProps.m_id;
				engineId = m_settings.getEngineId(m_settings.m_defaultBackend);
#ifdef DEBUG
				clog << "ResultsTree::addResults: engine is index " << engineName << " " << indexId << " " << engineId << endl;
#endif
			}
#ifdef DEBUG
			else clog << "ResultsTree::addResults: " << engineName << " is not an index" <<  endl;
#endif
		}
#ifdef DEBUG
		else clog << "ResultsTree::addResults: ID for engine " << engineName << " is " << engineId <<  endl;
#endif
	}

	QueryHistory queryHistory(m_settings.getHistoryDatabaseName());
	ViewHistory viewHistory(m_settings.getHistoryDatabaseName());
	set<time_t> latestRuns;
	time_t secondLastRunTime = 0;
	bool isNewQuery = false;

	// Is this a new query ?
	if ((queryHistory.getLatestRuns(m_treeName, engineName, 2, latestRuns) == false) ||
		(latestRuns.empty() == true))
	{
		isNewQuery = true;
	}
	else
	{
		set<time_t>::const_iterator runIter = latestRuns.begin();

		// We only need to keep the last two runs
		if (runIter != latestRuns.end())
		{
			++runIter;
			if (runIter != latestRuns.end())
			{
				secondLastRunTime = (*runIter);
			}
		}
	}
#ifdef DEBUG
	clog << "ResultsTree::addResults: " << resultsList.size() << " results with charset " << charset
		<< ", second last run " << secondLastRunTime << endl;
#endif

	// Look at the results list
	for (vector<DocumentInfo>::const_iterator resultIter = resultsList.begin();
		resultIter != resultsList.end(); ++resultIter)
	{
		ustring title(to_utf8(resultIter->getTitle(), charset));
		ustring location(to_utf8(resultIter->getLocation(true), charset));
		ustring timestamp(to_utf8(resultIter->getTimestamp()));
		ustring extract(to_utf8(resultIter->getExtract(), charset));
		string groupName;
		TreeModel::iterator groupIter;
		float currentScore = resultIter->getScore();
		int rankDiff = 0;

		if (m_groupMode != FLAT)
		{
			// What group should the result go to ?
			if (rootType == ResultsModelColumns::ROW_HOST)
			{
				Url urlObj(location);
				groupName = urlObj.getHost();
			}
			else
			{
				groupName = engineName;
			}
			// Add the group or get its position if it's already in
			appendGroup(groupName, rootType, groupIter);

			// Has the result's ranking changed ?
			float oldestScore = 0;
			float previousScore = queryHistory.hasItem(m_treeName, engineName,
				location, oldestScore);
#ifdef DEBUG
			clog << "ResultsTree::addResults: " << location << " has scores "
				<< previousScore << ", " << oldestScore << endl;
#endif
			if (previousScore > 0)
			{
				// Yes, it has
				rankDiff = (int)(currentScore - previousScore);
			}
			else
			{
				// New results are displayed as such only if the query has already been run on the engine
				if (isNewQuery == false)
				{
					// This is a magic value :-)
					rankDiff = 666;
				}
			}

			if (updateHistory == true)
			{
				queryHistory.insertItem(m_treeName, engineName, location,
					title, extract, currentScore);
			}
		}

		++count;

		// We already got indexId from PinotSettings
		unsigned int docIndexId = 0;
		unsigned int docId = resultIter->getIsIndexed(docIndexId);
		bool isIndexed = false;

		if (docId > 0)
		{
			isIndexed = true;
		}

		// Has it been already viewed ?
		bool wasViewed = viewHistory.hasItem(location);

		// OK, add a row for this result within the group
		TreeModel::iterator titleIter;
		if (appendResult(title, location, (int)currentScore, rankDiff, isIndexed, wasViewed,
			docId, timestamp, resultIter->serialize(),
			engineId, indexId, titleIter, groupIter, true) == true)
		{
#ifdef DEBUG
			clog << "ResultsTree::addResults: added row for result " << count
				<< ", " << currentScore << ", " << isIndexed << " " << docId
				<< " " << indexId << endl;
#endif

			if (groupIter)
			{
				// Update this map, so that we know which groups need updating
				updatedGroups[groupName] = groupIter;
			}
		}
	}

	// Remove older items ?
	if ((isNewQuery == false) &&
		(updateHistory == true))
	{
#ifdef DEBUG
		clog << "ResultsTree::addResults: removing items for " << m_treeName
			<< ", " << engineName << " older than " << secondLastRunTime << endl;
#endif
		queryHistory.deleteItems(m_treeName, engineName, secondLastRunTime);
	}

	if (count > 0)
	{
#ifdef DEBUG
		clog << "ResultsTree::addResults: " << updatedGroups.size() << " groups to update" << endl;
#endif
		// Update the groups to which we have added results
		for (std::map<string, TreeModel::iterator>::iterator mapIter = updatedGroups.begin();
			mapIter != updatedGroups.end(); mapIter++)
		{
			TreeModel::iterator groupIter = mapIter->second;
			updateGroup(groupIter);
		}

		return true;
	}
	else if (m_groupMode == BY_ENGINE)
	{
		// If this didn't return any result, add an empty group
		TreeModel::iterator groupIter;
		appendGroup(engineName, rootType, groupIter);
		updateGroup(groupIter);

		return true;
	}

	return false;
}

//
// Sets how results are grouped.
//
void ResultsTree::setGroupMode(GroupByMode groupMode)
{
	ResultsModelColumns::RowType newType;

	if (m_groupMode == FLAT)
	{
		// No change possible
		return;
	}

	if (m_groupMode == groupMode)
	{
		// No change
		return;
	}
#ifdef DEBUG
	clog << "ResultsTree::setGroupMode: set to " << groupMode << endl;
#endif
	m_groupMode = groupMode;

	// Do we need to update the tree ?
	TreeModel::Children children = m_refStore->children();
	if (children.empty() == true)
	{
		return;
	}

	// What's the new grouping criteria ?
	if (m_groupMode == BY_ENGINE)
	{
		// By search engine
		newType = ResultsModelColumns::ROW_ENGINE;
	}
	else
	{
		// By host
		newType = ResultsModelColumns::ROW_HOST;
	}

	// Clear the map
	m_resultsGroups.clear();

	// Unselect results
	get_selection()->unselect_all();

	TreeModel::Children::iterator iter = children.begin();
	while (iter != children.end())
	{
		TreeModel::Row row = *iter;
#ifdef DEBUG
		clog << "ResultsTree::setGroupMode: looking at " << row[m_resultsColumns.m_url] << endl;
#endif
		ResultsModelColumns::RowType type = row[m_resultsColumns.m_resultType];
		// Skip new type and other rows
		if ((type == newType) ||
			(type == ResultsModelColumns::ROW_OTHER))
		{
			iter++;
			continue;
		}

		TreeModel::Children groupChildren = iter->children();
		TreeModel::Children::iterator childIter = groupChildren.begin();
		// Type ROW_RESULT
		while (childIter != groupChildren.end())
		{
			TreeModel::Row childRow = *childIter;
			TreeModel::iterator groupIter, newIter;
			bool success = false;

			type = childRow[m_resultsColumns.m_resultType];
			if (type == ResultsModelColumns::ROW_OTHER)
			{
				TreeModel::Children::iterator nextChildIter = childIter;
				++nextChildIter;

				// Erase this row
				m_refStore->erase(childIter);
				childIter = nextChildIter;
				continue;
			}

			// We will need the URL and engines columns in all cases
			string url(from_utf8(childRow[m_resultsColumns.m_url]));
			unsigned int engineIds = childRow[m_resultsColumns.m_engines];
			unsigned int indexIds = childRow[m_resultsColumns.m_indexes];

			// Get the name of the group this should go into
			if (m_groupMode == BY_HOST)
			{
				Url urlObj(url);
#ifdef DEBUG
				clog << "ResultsTree::setGroupMode: row " << url << endl;
#endif
				// Add group
				if (appendGroup(urlObj.getHost(), newType, groupIter) == true)
				{
					// Add result
					success = appendResult(childRow[m_resultsColumns.m_text],
						childRow[m_resultsColumns.m_url],
						childRow[m_resultsColumns.m_score],
						childRow[m_resultsColumns.m_rankDiff],
						childRow[m_resultsColumns.m_indexed],
						childRow[m_resultsColumns.m_viewed],
						childRow[m_resultsColumns.m_docId],
						childRow[m_resultsColumns.m_timestamp],
						childRow[m_resultsColumns.m_serial],
						engineIds, indexIds,
						newIter, groupIter, true);
				}
			}
			else
			{
				// Look at the engines column and see which engines this result is for
				set<string> engineNames;
				m_settings.getEngineNames(engineIds, engineNames);
				if (engineNames.empty() == false)
				{
#ifdef DEBUG
					clog << "ResultsTree::setGroupMode: row is for " << engineNames.size() << " engine(s)" << endl;
#endif
					// Are there indexes in the list ?
					set<string>::iterator backendIter = engineNames.find(m_settings.m_defaultBackend);
					if ((backendIter != engineNames.end()) &&
						(indexIds > 0))
					{
						// Erase this
						engineNames.erase(backendIter);
#ifdef DEBUG
						clog << "ResultsTree::setGroupMode: row is for index(es) " << indexIds << endl;
#endif

						// Add entries for each index name so that we can loop once on engine names
						set<string> indexNames;
						m_settings.getIndexNames(indexIds, indexNames);
						for (set<string>::iterator indexIter = indexNames.begin();
							indexIter != indexNames.end(); ++indexIter)
						{
							string indexName(*indexIter);
							engineNames.insert(indexName);
#ifdef DEBUG
							clog << "ResultsTree::setGroupMode: row is for index " << indexName << endl;
#endif
						}
					}

					for (set<string>::iterator engineIter = engineNames.begin();
						engineIter != engineNames.end(); ++engineIter)
					{
						string engineName(*engineIter);
						unsigned int indexId = 0;
						unsigned int engineId = m_settings.getEngineId(engineName);

						if (engineId == 0)
						{
							// This is actually an index, not an engine...
							PinotSettings::IndexProperties indexProps = m_settings.getIndexPropertiesByName(engineName);
							if (indexProps.m_location.empty() == false)
							{
								engineId = m_settings.getEngineId(m_settings.m_defaultBackend);
							}
#ifdef DEBUG
							clog << "ResultsTree::setGroupMode: index " << indexId << endl;
#endif
						}
#ifdef DEBUG
						else clog << "ResultsTree::setGroupMode: no index" << endl;
#endif

						// Add group
						if (appendGroup(engineName, newType, groupIter) == true)
						{
							// Add result
							appendResult(childRow[m_resultsColumns.m_text],
								childRow[m_resultsColumns.m_url],
								childRow[m_resultsColumns.m_score],
								childRow[m_resultsColumns.m_rankDiff],
								childRow[m_resultsColumns.m_indexed],
								childRow[m_resultsColumns.m_viewed],
								childRow[m_resultsColumns.m_docId],
								childRow[m_resultsColumns.m_timestamp],
								childRow[m_resultsColumns.m_serial],
								engineId, indexId,
								newIter, groupIter, true);
#ifdef DEBUG
							clog << "ResultsTree::setGroupMode: row for " << engineName << endl;
#endif
						}
					}

					// FIXME: make sure at least one row was added
					success = true;
				}
			}

			if (success == true)
			{
				// Delete it
				m_refStore->erase(*childIter);
				childIter = groupChildren.begin();
			}
			else
			{
				// Don't delete anything then, just go to the next child
				childIter++;
			}
		}

		// Erase this row
		m_refStore->erase(*iter);

		// Get the new first row, that way we don't have to worry about iterators validity
		iter = children.begin();
	}

	for (std::map<string, TreeModel::iterator>::iterator mapIter = m_resultsGroups.begin();
		mapIter != m_resultsGroups.end(); mapIter++)
	{
		TreeModel::iterator groupIter = mapIter->second;
		updateGroup(groupIter);
	}

	onSelectionChanged();
}

//
// Gets the first selected item's URL.
//
ustring ResultsTree::getFirstSelectionURL(void)
{
	vector<TreeModel::Path> selectedItems = get_selection()->get_selected_rows();

	if (selectedItems.empty() == true)
	{
		return "";
	}

	vector<TreeModel::Path>::iterator itemPath = selectedItems.begin();
	TreeModel::iterator iter = m_refStore->get_iter(*itemPath);
	TreeModel::Row row = *iter;

	return row[m_resultsColumns.m_url];
}

//
// Gets a list of selected items.
//
bool ResultsTree::getSelection(vector<DocumentInfo> &resultsList, bool skipIndexed)
{
	vector<TreeModel::Path> selectedItems = get_selection()->get_selected_rows();

	if (selectedItems.empty() == true)
	{
		return false;
	}

	// Go through selected items
	for (vector<TreeModel::Path>::iterator itemPath = selectedItems.begin();
		itemPath != selectedItems.end(); ++itemPath)
	{
		TreeModel::iterator iter = m_refStore->get_iter(*itemPath);
		TreeModel::Row row = *iter;

		ResultsModelColumns::RowType type = row[m_resultsColumns.m_resultType];
		if (type != ResultsModelColumns::ROW_RESULT)
		{
			continue;
		}

		bool isIndexed = row[m_resultsColumns.m_indexed];
		if ((skipIndexed == false) ||
			(isIndexed == false))
		{
			DocumentInfo current;
			string serial(row[m_resultsColumns.m_serial]);

			current.deserialize(serial);

			if (isIndexed == true)
			{
				set<string> indexNames;
				unsigned int indexIds = row[m_resultsColumns.m_indexes];

				m_settings.getIndexNames(indexIds, indexNames);
				// Any internal index in there ?
				for (set<string>::iterator indexIter = indexNames.begin(); indexIter != indexNames.end(); ++indexIter)
				{
					PinotSettings::IndexProperties indexProps = m_settings.getIndexPropertiesByName(*indexIter);
					if (indexProps.m_internal == true)
					{
#ifdef DEBUG
						clog << "ResultsTree::getSelection: result in internal index " << *indexIter << endl;
#endif
						current.setIsIndexed(indexProps.m_id, row[m_resultsColumns.m_docId]);
						break;
					}
				}
			}

			resultsList.push_back(current);
		}
	}
#ifdef DEBUG
	clog << "ResultsTree::getSelection: " << resultsList.size() << " results selected" << endl;
#endif

	return true;
}

//
// Sets the selected items' state.
//
void ResultsTree::setSelectionState(bool viewed)
{
	vector<TreeModel::Path> selectedItems = get_selection()->get_selected_rows();

	if (selectedItems.empty() == true)
	{
		return;
	}

	// Go through selected items
	for (vector<TreeModel::Path>::iterator itemPath = selectedItems.begin();
		itemPath != selectedItems.end(); ++itemPath)
	{
		TreeModel::iterator iter = m_refStore->get_iter(*itemPath);
		TreeModel::Row row = *iter;
  
		ResultsModelColumns::RowType type = row[m_resultsColumns.m_resultType];
		if (type != ResultsModelColumns::ROW_RESULT)
		{
			continue;
		}

		if (viewed == true)
		{
			row[m_resultsColumns.m_viewed] = true;
		}
	}
}

//
// Updates a result's properties.
//
bool ResultsTree::updateResult(const DocumentInfo &result)
{
	unsigned int indexId = 0;
	unsigned int docId = result.getIsIndexed(indexId);

	if (docId == 0)
	{
		return false;
	}

	// Go through the list
	TreeModel::Children children = m_refStore->children();
	for (TreeModel::Children::iterator iter = children.begin(); iter != children.end(); ++iter)
	{
		TreeModel::Row row = *iter;

		if (docId == row[m_resultsColumns.m_docId])
		{
			// FIXME: title, location should be converted based on the result's charset !
			updateRow(row, result.getTitle(), result.getLocation(true),
				row[m_resultsColumns.m_score], row[m_resultsColumns.m_engines],
				row[m_resultsColumns.m_indexes], docId,
				to_utf8(result.getTimestamp()), result.serialize(),
				ResultsModelColumns::ROW_RESULT,
				row[m_resultsColumns.m_indexed], row[m_resultsColumns.m_viewed],
				row[m_resultsColumns.m_rankDiff]);

			return true;
		}
	}

	return false;
}

//
// Deletes the current selection.
//
bool ResultsTree::deleteSelection(void)
{
	bool empty = false;

	// Go through selected items
	vector<TreeModel::Path> selectedItems = get_selection()->get_selected_rows();
	vector<TreeModel::Path>::iterator itemPath = selectedItems.begin();

	while (itemPath != selectedItems.end())
	{
		TreeModel::iterator iter = m_refStore->get_iter(*itemPath);
		TreeModel::Row row = *iter;
		TreeModel::iterator parentIter;
		bool updateParent = false;

		// This could be a group that's in the map and should be removed first
		ResultsModelColumns::RowType type = row[m_resultsColumns.m_resultType];
		if (type != ResultsModelColumns::ROW_RESULT)
		{
			string groupName(from_utf8(row[m_resultsColumns.m_text]));
			std::map<string, TreeModel::iterator>::iterator mapIter = m_resultsGroups.find(groupName);
			if (mapIter != m_resultsGroups.end())
			{
				m_resultsGroups.erase(mapIter);
#ifdef DEBUG
				clog << "ResultsTree::deleteSelection: erased group " << groupName << endl;
#endif
			}
		}
		else if (m_groupMode != FLAT)
		{
			// This item is a result
			parentIter = row.parent();
			updateParent = true;
		}

		// Unselect and erase
		get_selection()->unselect(iter);
		m_refStore->erase(row);

		// Update group ?
		if (updateParent == true)
		{
			// Update the group this result belongs to
			updateGroup(parentIter);
		}

		selectedItems = get_selection()->get_selected_rows();
		itemPath = selectedItems.begin();
	}

	TreeModel::Children children = m_refStore->children();
	empty = children.empty();

	refresh();

	return empty;
}

//
// Deletes results.
//
bool ResultsTree::deleteResults(const string &engineName)
{
	unsigned int indexId = 0;
	unsigned int engineId = m_settings.getEngineId(engineName);
	unsigned int count = 0;

	if (engineId == 0)
	{
		// Chances are this engine is an index
		PinotSettings::IndexProperties indexProps = m_settings.getIndexPropertiesByName(engineName);
		if (indexProps.m_location.empty() == false)
		{
			// Yes, it is
			indexId = indexProps.m_id;
			engineId = m_settings.getEngineId(m_settings.m_defaultBackend);
		}
	}

	TreeModel::Children groups = m_refStore->children();
	for (TreeModel::Children::iterator parentIter = groups.begin();
		parentIter != groups.end(); ++parentIter)
	{
		TreeModel::Row row = *parentIter;

		ResultsModelColumns::RowType type = row[m_resultsColumns.m_resultType];
		if ((type != ResultsModelColumns::ROW_ENGINE) &&
			(type != ResultsModelColumns::ROW_HOST))
		{
			continue;
		}

		TreeModel::Children children = parentIter->children();
		TreeModel::Children::iterator iter = children.begin();
		while (iter != children.end())
		{
			row = *iter;

			type = row[m_resultsColumns.m_resultType];
			if (((type == ResultsModelColumns::ROW_RESULT) &&
				(row[m_resultsColumns.m_engines] == engineId) &&
				(row[m_resultsColumns.m_indexes] == indexId)) ||
				(type == ResultsModelColumns::ROW_OTHER))
			{
				TreeModel::Children::iterator nextIter = iter;
				++nextIter;
				++count;

				// Erase this row
				m_refStore->erase(*iter);
				iter = nextIter;
				continue;
			}

			// Next
			++iter;
		}
	}

	if (count > 0)
	{
		onSelectionChanged();
#ifdef DEBUG
		clog << "ResultsTree::deleteResults: erased " << count << " rows" << endl;
#endif
		return true;
	}

	return false;
}

//
// Returns the number of rows.
//
unsigned int ResultsTree::getRowsCount(void)
{
	return m_refStore->children().size();
}

//
// Refreshes the tree.
//
void ResultsTree::refresh(void)
{
	// FIXME: not sure why, but this helps with refreshing the tree
	columns_autosize();
}

//
// Clears the tree.
//
void ResultsTree::clear(void)
{
	// Unselect results
	get_selection()->unselect_all();

	// Remove existing rows in the tree
	TreeModel::Children children = m_refStore->children();
	if (children.empty() == false)
	{
		// Clear the groups map
		m_resultsGroups.clear();

		TreeModel::Children::iterator iter = children.begin();
		while (iter != children.end())
		{
			// Erase this row
			m_refStore->erase(*iter);

			// Get the new first row
			children = m_refStore->children();
			iter = children.begin();
		}
		m_refStore->clear();

		// Clear the extract
		RefPtr<TextBuffer> refBuffer = m_extractTextView->get_buffer();
		if (refBuffer)
		{
			refBuffer->set_text("");
		}

		onSelectionChanged();
	}
}

//
// Shows or hides the extract field.
//
void ResultsTree::showExtract(bool showExtract)
{
	m_showExtract = showExtract;
	if (m_showExtract == true)
	{
		// Show the extract
		m_pExtractScrolledwindow->show();
	}
	else
	{
		// Hide
		m_pExtractScrolledwindow->hide();
	}
}

//
// Exports results to a file.
//
void ResultsTree::exportResults(const string &fileName,
	const string &queryName, bool csvFormat)
{
	QueryProperties queryProps(queryName, "");
	ResultsExporter *pExporter = NULL;
	unsigned int maxResultsCount = 0;

	if (fileName.empty() == true)
	{
		return;
	}

	if (csvFormat == true)
	{
		pExporter = new CSVExporter(fileName,
			queryProps);
	}
	else
	{
		pExporter = new OpenSearchExporter(fileName,
			queryProps);
	}

	// How many results are there altogether ?
	TreeModel::Children children = m_refStore->children();
	if (m_groupMode == FLAT)
	{
		maxResultsCount = children.size();
	}
	else for (TreeModel::Children::iterator iter = children.begin();
		iter != children.end(); ++iter)
	{
		TreeModel::Row row = *iter;
		ResultsModelColumns::RowType type = row[m_resultsColumns.m_resultType];

		if ((type != ResultsModelColumns::ROW_ENGINE) &&
			(type != ResultsModelColumns::ROW_HOST))
		{
			continue;
		}

		TreeModel::Children groupChildren = iter->children();
		maxResultsCount += groupChildren.size();
	}
#ifdef DEBUG
	clog << "ResultsTree::exportResults: " << maxResultsCount << " results to export" << endl;
#endif

	// Start
	pExporter->exportStart("", maxResultsCount);

	if (m_groupMode == FLAT)
	{
		exportResults(children, queryName, pExporter);
	}
	else for (TreeModel::Children::iterator iter = children.begin();
		iter != children.end(); ++iter)
	{
		TreeModel::Row row = *iter;
		ResultsModelColumns::RowType type = row[m_resultsColumns.m_resultType];

		if ((type != ResultsModelColumns::ROW_ENGINE) &&
			(type != ResultsModelColumns::ROW_HOST))
		{
			continue;
		}

		TreeModel::Children groupChildren = iter->children();
		exportResults(groupChildren, queryName, pExporter);
	}

	// End
	pExporter->exportEnd();

	delete pExporter;
}

//
// Exports results to a file.
//
void ResultsTree::exportResults(TreeModel::Children &groupChildren,
	const string &queryName, ResultsExporter *pExporter)
{
	QueryHistory queryHistory(m_settings.getHistoryDatabaseName());

	for (TreeModel::Children::iterator childIter = groupChildren.begin();
		childIter != groupChildren.end(); ++childIter)
	{
		TreeModel::Row childRow = *childIter;
		ResultsModelColumns::RowType type = childRow[m_resultsColumns.m_resultType];

		if (type == ResultsModelColumns::ROW_OTHER)
		{
			continue;
		}

		set<string> engineNames, indexNames;
		DocumentInfo result;
		string engineName, serial(childRow[m_resultsColumns.m_serial]);
		unsigned int engineIds = childRow[m_resultsColumns.m_engines];
		unsigned int indexIds = childRow[m_resultsColumns.m_indexes];

#ifdef DEBUG
		clog << "ResultsTree::exportResults: engines " << engineIds << ", indexes " << indexIds << endl;
#endif
		result.deserialize(serial);
		m_settings.getEngineNames(engineIds, engineNames);
		if (engineNames.empty() == false)
		{
			// Get the first engine this result was obtained from
			engineName = *engineNames.begin();
			if (engineName == m_settings.m_defaultBackend)
			{
				m_settings.getIndexNames(indexIds, indexNames);
				if (indexNames.empty() == false)
				{
					// Use the name of the first index as engine name
					engineName = (*indexNames.begin());
				}
			}
		}
		if (m_groupMode != FLAT)
		{
			result.setExtract(queryHistory.getItemExtract(from_utf8(queryName),
				engineName, result.getLocation(true)));
		}
		else
		{
			engineName = m_treeName;
		}
		result.setTimestamp(from_utf8(childRow[m_resultsColumns.m_timestamp]));

		// Export this
		if (pExporter != NULL)
		{
			pExporter->exportResult(engineName, result);
		}
	}
}

//
// Returns the changed selection signal.
//
sigc::signal1<void, ustring>& ResultsTree::getSelectionChangedSignal(void)
{
	return m_signalSelectionChanged;
}

//
// Returns the double-click signal.
//
sigc::signal0<void>& ResultsTree::getDoubleClickSignal(void)
{
	return m_signalDoubleClick;
}

//
// Adds a new row in the results tree.
//
bool ResultsTree::appendResult(const ustring &text, const ustring &url,
	int score, int rankDiff, bool isIndexed, bool wasViewed,
	unsigned int docId, const ustring &timestamp,
	const string &serial, unsigned int engineId,
	unsigned int indexId, TreeModel::iterator &newRowIter,
	const TreeModel::iterator &parentIter, bool noDuplicates)
{
	if (!parentIter)
	{
#ifdef DEBUG
		clog << "ResultsTree::appendResult: no parent" << endl;
#endif
		newRowIter = m_refStore->append();
	}
	else
	{
		const TreeModel::Row parentRow = *parentIter;

		// Merge duplicates within groups ?
		if (noDuplicates == true)
		{
			// Look for a row with the same URL and query. For instance, in group
			// by host mode, if a page is returned by several search engines, it
			// should appear only once
			TreeModel::Children children = parentRow.children();
			if (children.empty() == false)
			{
				for (TreeModel::Children::iterator childIter = children.begin();
					childIter != children.end(); ++childIter)
				{
					TreeModel::Row row = *childIter;
					if (row[m_resultsColumns.m_url] == url)
					{
						// Update the engines column...
						row[m_resultsColumns.m_engines] = row[m_resultsColumns.m_engines] | engineId;
						// ...and the indexes column too
						row[m_resultsColumns.m_indexes] = row[m_resultsColumns.m_indexes] | engineId;

						newRowIter = childIter;
						return true;
					}
				}
			}
		}

		newRowIter = m_refStore->append(parentRow.children());
	}

	TreeModel::Row childRow = *newRowIter;
	updateRow(childRow, text, url, score, engineId, indexId,
		docId, timestamp, serial, ResultsModelColumns::ROW_RESULT, isIndexed,
		wasViewed, rankDiff);

	return true;
}

//
// Adds a results group
//
bool ResultsTree::appendGroup(const string &groupName, ResultsModelColumns::RowType groupType,
	TreeModel::iterator &groupIter)
{
	bool success = false;

	// Is this group already in ?
	std::map<string, TreeModel::iterator>::iterator mapIter = m_resultsGroups.find(groupName);
	if (mapIter == m_resultsGroups.end())
	{
		// No, it isn't: insert a new group in the tree
		groupIter = m_refStore->append();
		TreeModel::Row groupRow = *groupIter;
		updateRow(groupRow, groupName,
			"", 0, 0, 0, 0, "", "", groupType,
			false, false, 0);

		// Update the map
		m_resultsGroups[groupName] = groupIter;
		success = true;
#ifdef DEBUG
		clog << "ResultsTree::appendGroup: updated map with " << groupName << endl;
#endif
	}
	else
	{
		// Yes, it is
		groupIter = mapIter->second;
#ifdef DEBUG
		clog << "ResultsTree::appendGroup: found " << groupName << " in map" << endl;
#endif
		success = true;
	}

	return success;
}

//
// Updates a results group.
//
void ResultsTree::updateGroup(TreeModel::iterator &groupIter)
{
	TreeModel::Row groupRow = (*groupIter);
	int averageScore = 0;

	// Check the iterator doesn't point to a result
	ResultsModelColumns::RowType type = groupRow[m_resultsColumns.m_resultType];
	if (type == ResultsModelColumns::ROW_RESULT)
	{
		return;
	}

	// Modify the "score" column to indicate the number of results in that group
	TreeModel::Children groupChildren = groupIter->children();
	if (groupChildren.empty() == false)
	{
		for (TreeModel::Children::iterator childIter = groupChildren.begin();
			childIter != groupChildren.end(); ++childIter)
		{
			TreeModel::Row row = *childIter;

			averageScore += row[m_resultsColumns.m_score];
		}

		averageScore = (int)(averageScore / groupChildren.size());
	}
	else
	{
		TreeModel::Row groupRow = *groupIter;
		TreeModel::iterator childIter = m_refStore->append(groupRow.children());
		TreeModel::Row childRow = *childIter;

		updateRow(childRow, _("No results"), "", 0, 0, 0, 0,
			"", "", ResultsModelColumns::ROW_OTHER, false, false, 0);
	}
	groupRow[m_resultsColumns.m_score] = averageScore;

	// Expand this group
	TreeModel::Path groupPath = m_refStore->get_path(groupIter);
	expand_row(groupPath, true);
}

//
// Updates a row.
//
void ResultsTree::updateRow(TreeModel::Row &row, const ustring &text,
	const ustring &url, int score, 	unsigned int engineId, unsigned int indexId,
	unsigned int docId, const ustring &timestamp, const string &serial,
	ResultsModelColumns::RowType resultType, bool indexed, bool viewed, int rankDiff)
{
	try
	{
		row[m_resultsColumns.m_text] = text;
		row[m_resultsColumns.m_url] = url;
		row[m_resultsColumns.m_score] = score;
		row[m_resultsColumns.m_scoreText] = "";
		row[m_resultsColumns.m_engines] = engineId;
		row[m_resultsColumns.m_indexes] = indexId;
		row[m_resultsColumns.m_docId] = docId;
		row[m_resultsColumns.m_resultType] = resultType;
		row[m_resultsColumns.m_timestamp] = timestamp;
		row[m_resultsColumns.m_timestampTime] = TimeConverter::fromTimestamp(from_utf8(timestamp));
		row[m_resultsColumns.m_serial] = serial;

		row[m_resultsColumns.m_indexed] = indexed;
		row[m_resultsColumns.m_viewed] = viewed;
		row[m_resultsColumns.m_rankDiff] = rankDiff;
	}
	catch (Error &error)
	{
#ifdef DEBUG
		clog << "ResultsTree::updateRow: " << error.what() << endl;
#endif
	}
	catch (...)
	{
#ifdef DEBUG
		clog << "ResultsTree::updateRow: caught unknown exception" << endl;
#endif
	}
}

//
// Retrieves the extract to show for the given row.
//
ustring ResultsTree::findResultsExtract(const Gtk::TreeModel::Row &row)
{
	QueryHistory queryHistory(m_settings.getHistoryDatabaseName());
	set<string> engineNames, indexNames;
	string url(from_utf8(row[m_resultsColumns.m_url]));
	string extract;
	unsigned int engineIds = row[m_resultsColumns.m_engines];
	unsigned int indexIds = row[m_resultsColumns.m_indexes];

#ifdef DEBUG
	clog << "ResultsTree::findResultsExtract: " << url << " has engines " << engineIds << ", indexes " << indexIds << endl;
#endif
	m_settings.getEngineNames(engineIds, engineNames);
	for (set<string>::const_iterator engineIter = engineNames.begin();
		engineIter != engineNames.end(); ++engineIter)
	{
		string engineName(*engineIter);

		indexNames.clear();
		if (engineName == m_settings.m_defaultBackend)
		{
			m_settings.getIndexNames(indexIds, indexNames);
		}
		else
		{
			// That's not an index but pretend it is
			indexNames.insert(engineName);
		}

		for (set<string>::const_iterator indexIter = indexNames.begin();
			indexIter != indexNames.end(); ++indexIter)
		{
			// Use the name of this index as engine name
			engineName = (*indexNames.begin());

#ifdef DEBUG
			clog << "ResultsTree::findResultsExtract: engine or index " << engineName << endl;
#endif
			extract = queryHistory.getItemExtract(from_utf8(m_treeName), engineName, url);
			if (extract.empty() == false)
			{
				// Stop here
				return extract;
			}
		}
	}

	return "";
}
