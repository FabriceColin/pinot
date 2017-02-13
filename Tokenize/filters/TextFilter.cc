/*
 *  Copyright 2007-2009 Fabrice Colin
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

#include <unistd.h>

#include "TextFilter.h"

using std::string;

using namespace std;
using namespace Dijon;

TextFilter::TextFilter() :
	Filter(),
	m_doneWithDocument(false)
{
}

TextFilter::~TextFilter()
{
	rewind();
}

bool TextFilter::is_data_input_ok(DataInput input) const
{
	if ((input == DOCUMENT_DATA) ||
		(input == DOCUMENT_STRING))
	{
		return true;
	}

	return false;
}

bool TextFilter::set_property(Properties prop_name, const string &prop_value)
{
	return true;
}

bool TextFilter::set_document_data(const char *data_ptr, off_t data_length)
{
	if ((data_ptr == NULL) ||
		(data_length == 0))
	{
		return false;
	}

	string text_doc(data_ptr, data_length);
	return set_document_string(text_doc);
}

bool TextFilter::set_document_string(const string &data_str)
{
	if (data_str.empty() == true)
	{
		return false;
	}

	rewind();

#ifdef DEBUG
	clog << "TextFilter::set_document_string: " << data_str.length() << " bytes of text" << endl;
#endif
	m_content.reserve(data_str.length());
	m_content.append(data_str.c_str(), data_str.length());
	m_metaData["ipath"] = "";
	m_metaData["mimetype"] = "text/plain";

	return true;
}

bool TextFilter::set_document_file(const string &file_path, bool unlink_when_done)
{
	return false;
}

bool TextFilter::set_document_uri(const string &uri)
{
	return false;
}

bool TextFilter::has_documents(void) const
{
	if (m_doneWithDocument == false)
	{
		return true;
	}

	return false;
}

bool TextFilter::next_document(void)
{
	if (m_doneWithDocument == false)
	{
		m_doneWithDocument = true;

		return true;
	}

	rewind();

	return false;
}

bool TextFilter::skip_to_document(const string &ipath)
{
	if (ipath.empty() == true)
	{
		return next_document();
	}

	return false;
}

string TextFilter::get_error(void) const
{
	return "";
}

void TextFilter::rewind(void)
{
	Filter::rewind();

	m_doneWithDocument = false;
}
