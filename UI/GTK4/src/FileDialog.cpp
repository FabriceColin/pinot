/*
 *  Copyright 2025 Fabrice Colin
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

#include <glibmm/convert.h>

#include "config.h"
#include "NLS.h"
#include "Url.h"
#include "FileDialog.h"
#include "PinotSettings.h"

using namespace Glib;
using namespace Gtk;

FileDialog::FileDialog(const ustring &title, const ustring &location,
	bool openOrCreate, bool directoriesOnly) :
	FileChooserDialog(title),
	m_okResponse(false)
{
	FileChooser::Action chooserAction = FileChooser::Action::OPEN;
	bool isDirectory = false;

	if (directoriesOnly == false)
	{
		if (openOrCreate == true)
		{
			chooserAction = FileChooser::Action::OPEN;
		}
		else
		{
			chooserAction = FileChooser::Action::SAVE;
		}
	}
	else
	{
		chooserAction = FileChooser::Action::SELECT_FOLDER;
		if (openOrCreate == false)
		{
			set_create_folders(true);
		}
		isDirectory = true;
	}

	set_action(chooserAction);
	// Have we been provided with an initial location ?
	if (location.empty() == true)
	{
		// No, get the location of the home directory then
		Url urlObj(PinotSettings::getInstance().getHomeDirectory());
		RefPtr<const Gio::File> refFile = Gio::File::create_for_path(filename_from_utf8(urlObj.getLocation()));

		set_current_folder(refFile);
	}
	else
	{
		Url urlObj(location);
		RefPtr<const Gio::File> refFile = Gio::File::create_for_path(filename_from_utf8(urlObj.getLocation()));

		set_current_folder(refFile);
		if (isDirectory == false)
		{
			set_current_name(filename_from_utf8(urlObj.getFile()));
		}
	}
	set_select_multiple(false);
}

FileDialog::~FileDialog()
{
}

bool FileDialog::okResponse(void) const
{
	return m_okResponse;
}

ustring FileDialog::getPath(void) const
{
	RefPtr<const Gio::File> refFile = get_current_folder();

	return filename_to_utf8(refFile->get_path());
}

void FileDialog::on_response(int responseId)
{
	Dialog::on_response(responseId);

	if (responseId == ResponseType::OK)
	{
		m_okResponse = true;
	}
}

