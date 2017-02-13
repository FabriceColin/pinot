/*
 *  Copyright 2007-2016 Fabrice Colin
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
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <iostream>

#include "Filter.h"

using std::string;
using std::set;
using std::map;
using std::clog;
using std::endl;

using namespace Dijon;

MIMETypes::MIMETypes()
{
}

MIMETypes::~MIMETypes()
{
}

Filter::Filter() :
	m_deleteInputFile(false)
{
}

Filter::~Filter()
{
	deleteInputFile();
}

bool Filter::set_document_file(const string &file_path, bool unlink_when_done)
{
	if (file_path.empty() == true)
	{
		return false;
	}

	rewind();
	m_filePath = file_path;
	m_deleteInputFile = unlink_when_done;

	return true;
}

void Filter::set_mime_type(const string &mime_type)
{
	m_mimeType = mime_type;
}

string Filter::get_mime_type(void) const
{
	return m_mimeType;
}

const map<string, std::string> &Filter::get_meta_data(void) const
{
	return m_metaData;
}

const dstring &Filter::get_content(void) const
{
	return m_content;
}

void Filter::rewind(void)
{
	m_metaData.clear();
	m_content.clear();
	deleteInputFile();
	m_filePath.clear();
	m_deleteInputFile = false;
}

void Filter::deleteInputFile(void)
{
	if ((m_deleteInputFile == true) &&
		(m_filePath.empty() == false))
	{
		unlink(m_filePath.c_str());
	}
}
