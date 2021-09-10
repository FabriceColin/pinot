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
#include <glibmm/convert.h>
#include <gtkmm/stock.h>

#include "config.h"
#include "NLS.h"
#include "Url.h"
#include "PinotSettings.h"
#include "PinotUtils.h"

using namespace std;
using namespace Glib;
using namespace Gtk;

/// Open a FileChooserDialog.
bool select_file_name(const ustring &title, ustring &location,
	bool openOrCreate, bool directoriesOnly)
{
	FileChooserDialog fileChooser(title);

	if (title.empty() == true)
	{
		return false;
	}

	prepare_file_chooser(fileChooser, location, openOrCreate, directoriesOnly);
	fileChooser.show();

	int result = fileChooser.run();
	if (result == RESPONSE_OK)
	{
		// Retrieve the chosen location
		if (directoriesOnly == false)
		{
			location = filename_to_utf8(fileChooser.get_filename());
		}
		else
		{
			location = filename_to_utf8(fileChooser.get_current_folder());
		}

		return true;
	}

	return false;
}

bool prepare_file_chooser(FileChooserDialog &fileChooser, ustring &location,
	bool openOrCreate, bool directoriesOnly)
{
	FileChooserAction chooserAction = FILE_CHOOSER_ACTION_OPEN;
	StockID okButtonStockId = Stock::OPEN;
	bool isDirectory = false;

	if (openOrCreate == false)
	{
		okButtonStockId = Stock::SAVE;
		fileChooser.set_do_overwrite_confirmation(true);
	}

	// Have we been provided with an initial location ?
	if (location.empty() == true)
	{
		// No, get the location of the home directory then
		location = PinotSettings::getInstance().getHomeDirectory();
		isDirectory = true;
	}

	if (directoriesOnly == false)
	{
		if (openOrCreate == true)
		{
			chooserAction = FILE_CHOOSER_ACTION_OPEN;
		}
		else
		{
			chooserAction = FILE_CHOOSER_ACTION_SAVE;
		}
	}
	else
	{
		if (openOrCreate == true)
		{
			chooserAction = FILE_CHOOSER_ACTION_SELECT_FOLDER;
		}
		else
		{
			chooserAction = FILE_CHOOSER_ACTION_CREATE_FOLDER;
		}
		isDirectory = true;
	}

	fileChooser.set_action(chooserAction);
	Url urlObj(location);
	fileChooser.set_current_folder(filename_from_utf8(urlObj.getLocation()));
	if (isDirectory == false)
	{
		fileChooser.set_current_name(filename_from_utf8(urlObj.getFile()));
	}
	fileChooser.set_local_only();
	fileChooser.set_select_multiple(false);
	fileChooser.add_button(Stock::CANCEL, RESPONSE_CANCEL);
	fileChooser.add_button(okButtonStockId, RESPONSE_OK);
	fileChooser.set_show_hidden(true);

	return true;
}

/// Get a column height.
int get_column_height(TreeView *pTree)
{
	int height = 0;

	if (pTree == NULL)
	{
		return 0;
	}

	TreeViewColumn *pColumn = pTree->get_column(1);
	if (pColumn != NULL)
	{
		Gdk::Rectangle cellArea;
		int xOffset, yOffset, cellWidth, cellHeight;

		pColumn->cell_get_size(cellArea, xOffset, yOffset, cellWidth, cellHeight);
		height += cellHeight;
#ifdef DEBUG
		clog << "get_column_height: cell " << cellHeight << " " << yOffset << endl;
#endif
	}
#ifdef DEBUG
	clog << "get_column_height: " << height << endl;
#endif

	return height;
}

/// Create a text column.
TreeViewColumn *create_column(const ustring &title, const TreeModelColumnBase& modelColumn,
	bool isResizable, bool isSortable, const TreeModelColumnBase &sortColumn)
{
	TreeViewColumn *pColumn = new TreeViewColumn(title);

	CellRendererText *pTextRenderer = new CellRendererText();
	pColumn->pack_start(*manage(pTextRenderer));
	pColumn->add_attribute(pTextRenderer->property_text(), modelColumn);
	pColumn->set_resizable(isResizable);
	if (isSortable == true)
	{
		pColumn->set_sort_column(sortColumn);
	}

	return pColumn;
}

/// Create an icon and text column, rendered by renderTextAndIconCell.
TreeViewColumn *create_column_with_icon(const ustring &title, const TreeModelColumnBase& modelColumn,
	const TreeViewColumn::SlotCellData &renderTextAndIconCell,
	bool isResizable, bool isSortable, const TreeModelColumnBase &sortColumn)
{
	TreeViewColumn *pColumn = new TreeViewColumn(title);

	// Pack an icon renderer in the column
	CellRendererPixbuf *pIconRenderer = new CellRendererPixbuf();
	pColumn->pack_start(*manage(pIconRenderer), false);
	pColumn->set_cell_data_func(*pIconRenderer, renderTextAndIconCell);
	// ...followed by a text renderer
	CellRendererText *pTextRenderer = new CellRendererText();
	pColumn->pack_start(*manage(pTextRenderer));
	pColumn->set_cell_data_func(*pTextRenderer, renderTextAndIconCell);

	pColumn->add_attribute(pTextRenderer->property_text(), modelColumn);
	pColumn->set_resizable(isResizable);
	if (isSortable == true)
	{
		pColumn->set_sort_column(sortColumn);
	}

	return pColumn;
}

/// Converts to UTF-8.
ustring to_utf8(const string &text)
{
	std::string charset;

	// Get the locale charset
	get_charset(charset);
	// Call overload
	return to_utf8(text, charset);
}

/// Converts from the given charset to UTF-8.
ustring to_utf8(const string &text, const string &charset)
{
	if ((charset == "UTF-8") ||
		(charset == "utf-8"))
	{
		// No conversion necessary
		return text;
	}

	try
	{
		if (charset.empty() == false)
		{
			return convert_with_fallback(text, "UTF-8", charset, " ");
		}
		else
		{
			return locale_to_utf8(text);
		}
	}
	catch (Error &ce)
	{
#ifdef DEBUG
		clog << "to_utf8: cannot convert from " << charset << ": " << ce.what() << endl;
#endif
		if (charset.empty() == false)
		{
			return to_utf8(text);
		}
	}
	catch (...)
	{
#ifdef DEBUG
		clog << "to_utf8: unknown exception" << endl;
#endif
	}

	return "";
}

/// Converts from UTF-8.
string from_utf8(const ustring &text)
{
	try
	{
		return locale_from_utf8(text);
	}
	catch (Error &ce)
	{
#ifdef DEBUG
		clog << "from_utf8: " << ce.what() << endl;
#endif
	}
	catch (...)
	{
#ifdef DEBUG
		clog << "from_utf8: unknown exception" << endl;
#endif
	}

	return "";
}
