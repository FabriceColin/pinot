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

#include <iostream>

#include "config.h"
#include "NLS.h"
#include "Url.h"
#include "PinotSettings.h"
#include "PinotUtils.hh"
#include "indexDialog.hh"

using namespace std;
using namespace Glib;
using namespace Gtk;

indexDialog::indexDialog() :
	indexDialog_glade(),
	m_editIndex(false),
	m_badName(true)
{
	// Populate the type combo
	populate_typeCombobox();

	// By default, type is set to local
	typeCombobox->set_active(0);
	hostEntry->set_sensitive(false);
	portSpinbutton->set_sensitive(false);
	// ...and both name and location are empty
	indexOkbutton->set_sensitive(false);
}

indexDialog::indexDialog(const ustring &name, const ustring &location) :
	indexDialog_glade(),
	m_name(name),
	m_location(location),
	m_editIndex(false),
	m_badName(true)
{
	unsigned int port = 1024;

	// Populate the type combo
	populate_typeCombobox();

	// Name
	nameEntry->set_text(name);

	// Type and location
	ustring::size_type slashPos = location.find("/");
	if (slashPos == 0) 
	{
		// Local index
		typeCombobox->set_active(0);
		locationEntry->set_text(location);
		hostEntry->set_sensitive(false);
		portSpinbutton->set_sensitive(false);
	}
	else
	{
		Url urlObj(location);

		if (location.find("://") == ustring::npos)
		{
			// It's an old style remote specification without the protocol
			urlObj = Url("tcpsrv://" + location);
		}

		// Remote index
		ustring hostName(urlObj.getHost());
		// A port number should be included
		ustring::size_type colonPos = hostName.find(":");
		if (colonPos != ustring::npos)
		{
			port = (unsigned int)atoi(hostName.substr(colonPos + 1).c_str());
			hostName.resize(colonPos);
		}

#ifdef _SSH_TUNNEL
		if (urlObj.getProtocol() == "progsrv+ssh")
		{
			// SSH tunnel
			typeCombobox->set_active(2);
			locationEntry->set_text("/" + urlObj.getLocation());
			locationButton->set_sensitive(false);
		}
		else
#endif
		{
			// Assume vanilla TCP/IP
			typeCombobox->set_active(1);
			locationEntry->set_sensitive(false);
			locationButton->set_sensitive(false);
		}
		hostEntry->set_text(hostName);
		portSpinbutton->set_value((double)port);
	}

	m_editIndex = true;
}

indexDialog::~indexDialog()
{
}

void indexDialog::populate_typeCombobox(void)
{
	ustring servedBy(_("Served by"));

#if GTK_VERSION_LT(3, 0)
	typeCombobox->append_text(_("Local"));
	typeCombobox->append_text(servedBy + " xapian-tcpsrv");
#ifdef _SSH_TUNNEL
	typeCombobox->append_text(servedBy + " xapian-progsrv+ssh");
#endif
#else
	typeCombobox->append(_("Local"));
	typeCombobox->append(servedBy + " xapian-tcpsrv");
#ifdef _SSH_TUNNEL
	typeCombobox->append(servedBy + " xapian-progsrv+ssh");
#endif
#endif
}

void indexDialog::checkFields(void)
{
	bool needsLocation = false, needsHost = false, enableOkButton = true;

	if (typeCombobox->get_active_row_number() == 0)
	{
		// Local index : location only
		locationEntry->set_sensitive(true);
		locationButton->set_sensitive(true);
		hostEntry->set_sensitive(false);
		portSpinbutton->set_sensitive(false);
		needsLocation = true;
	}
	else if (typeCombobox->get_active_row_number() == 1)
	{
		// Vanilla TCP/IP : host and port only
		locationEntry->set_sensitive(false);
		locationButton->set_sensitive(false);
		hostEntry->set_sensitive(true);
		portSpinbutton->set_sensitive(true);
		needsHost = true;
	}
#ifdef _SSH_TUNNEL
	else
	{
		// SSH tunnel : location, host and port 
		locationEntry->set_sensitive(true);
		locationButton->set_sensitive(false);
		hostEntry->set_sensitive(true);
		portSpinbutton->set_sensitive(true);
		needsLocation = true;
		needsHost = true;
	}
#endif

	// Enable the OK button only if host name and location make sense
	if (needsLocation == true)
	{
		ustring location(locationEntry->get_text());
		if (location.empty() == true)
		{
			enableOkButton = false;
		}
		else
		{
			ustring::size_type slashPos = location.find("/");
			if (slashPos != 0)
			{
				enableOkButton = false;
			}
		}
	}
	if (needsHost == true)
	{
		ustring hostName(hostEntry->get_text());
		if (hostName.empty() == true)
		{
			enableOkButton = false;
		}
	}
	indexOkbutton->set_sensitive(enableOkButton);
}

bool indexDialog::badName(void) const
{
	return m_badName;
}

ustring indexDialog::getName(void) const
{
	return m_name;
}

ustring indexDialog::getLocation(void) const
{
	return m_location;
}

void indexDialog::on_indexOkbutton_clicked()
{
	PinotSettings &settings = PinotSettings::getInstance();
	// The changed() methods ensure name and location are set
	ustring location, name = nameEntry->get_text();

	m_badName = false;

	// Is it a remote index ?
	if (typeCombobox->get_active_row_number() == 0)
	{
		location = locationEntry->get_text();
	}
	else
	{
		char portStr[64];
		int port = portSpinbutton->get_value_as_int();

		snprintf(portStr, 64, "%d", port);
		if (typeCombobox->get_active_row_number() == 1)
		{
			// Vanilla TCP/IP
			location = "tcpsrv://";
			location += hostEntry->get_text();
			location += ":";
			location += portStr;
		}
#ifdef _SSH_TUNNEL
		else
		{
			// SSH tunnel
			location = "progsrv+ssh://";
			location += hostEntry->get_text();
			location += ":";
			location += portStr;
			location += locationEntry->get_text();
		}
#endif
	}
#ifdef DEBUG
	clog << "indexDialog::on_indexOkbutton_clicked: " << name << ", " << location << endl;
#endif

	// Look up that index name in the map
	const set<PinotSettings::IndexProperties> &indexes = settings.getIndexes();
	for (set<PinotSettings::IndexProperties>::const_iterator indexIter = indexes.begin();
		indexIter != indexes.end(); ++indexIter)
	{
		if (indexIter->m_name == name)
		{
			// This name is in use
			m_badName = true;
#ifdef DEBUG
			clog << "indexDialog::on_indexOkbutton_clicked: name in use" << endl;
#endif
			break;
		}
	}

	if ((m_editIndex == true) &&
		(name == m_name))
	{
		// ... but that's okay, because it's the original name
		m_badName = false;
#ifdef DEBUG
		clog << "indexDialog::on_indexOkbutton_clicked: old name" << endl;
#endif
	}

	m_name = name;
	m_location = location;
}

void indexDialog::on_locationEntry_changed()
{
	checkFields();
}

void indexDialog::on_locationButton_clicked()
{
	ustring dirName = locationEntry->get_text();
	if (select_file_name(_("Index location"), dirName, true, true) == true)
	{
		locationEntry->set_text(dirName);
	}
}

void indexDialog::on_typeCombobox_changed()
{
	checkFields();

}

void indexDialog::on_nameEntry_changed()
{
	checkFields();
}
