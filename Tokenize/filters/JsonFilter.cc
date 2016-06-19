/*
 *  Copyright 2007-2015 Fabrice Colin
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
#include <iostream>

#include "JsonFilter.h"

using std::string;
using std::clog;
using std::endl;

using namespace Dijon;

DIJON_FILTER_EXPORT bool get_filter_types(std::set<std::string> &mime_types)
{
	mime_types.clear();
	mime_types.insert("application/json");

	return true;
}

DIJON_FILTER_EXPORT bool check_filter_data_input(int data_input)
{
	Filter::DataInput input = (Filter::DataInput)data_input;

	if ((input == Filter::DOCUMENT_DATA) ||
		(input == Filter::DOCUMENT_STRING))
	{
		return true;
	}

	return false;
}

DIJON_FILTER_EXPORT Filter *get_filter(const std::string &mime_type)
{
	return new JsonFilter(mime_type);
}

JsonFilter::JsonFilter(const string &mime_type) :
	Filter(mime_type),
	m_doneWithDocument(false)
{
}

JsonFilter::~JsonFilter()
{
	rewind();
}

bool JsonFilter::is_data_input_ok(DataInput input) const
{
	if ((input == DOCUMENT_DATA) ||
		(input == DOCUMENT_STRING))
	{
		return true;
	}

	return false;
}

bool JsonFilter::set_property(Properties prop_name, const string &prop_value)
{
	return true;
}

bool JsonFilter::set_document_data(const char *data_ptr, off_t data_length)
{
	if ((data_ptr == NULL) ||
		(data_length == 0))
	{
		return false;
	}

	string json_doc(data_ptr, data_length);
	return set_document_string(json_doc);
}

bool JsonFilter::set_document_string(const string &data_str)
{
	if (data_str.empty() == true)
	{
		return false;
	}

	rewind();

	if (parse_json(data_str) == true)
	{
		m_doneWithDocument = false;
		return true;
	}

	return false;
}

bool JsonFilter::set_document_file(const string &file_path, bool unlink_when_done)
{
	return false;
}

bool JsonFilter::set_document_uri(const string &uri)
{
	return false;
}

bool JsonFilter::has_documents(void) const
{
	if (m_doneWithDocument == false)
	{
		return true;
	}

	return false;
}

bool JsonFilter::next_document(void)
{
	if (m_doneWithDocument == false)
	{
		m_doneWithDocument = true;

		return true;
	}

	rewind();

	return false;
}

bool JsonFilter::skip_to_document(const string &ipath)
{
	if (ipath.empty() == true)
	{
		return next_document();
	}

	return false;
}

string JsonFilter::get_error(void) const
{
	return "";
}

void JsonFilter::rewind(void)
{
	Filter::rewind();

	m_doneWithDocument = false;
}

void JsonFilter::dump_object(const string &key,
	json_object *pObj)
{
	json_type type = json_object_get_type(pObj);

	if (type == json_type_object)
	{
		json_object_iter iter;

		json_object_object_foreachC(pObj, iter)
		{
			if (key.empty() == false)
			{
				dump_object(key + "/" + iter.key, iter.val);
			}
			else
			{
				dump_object(iter.key, iter.val);
			}
		}
	}
	else
	{
		const char *pValue = json_object_to_json_string_ext(pObj, JSON_C_TO_STRING_PLAIN);

		if (pValue != NULL)
		{
			string value(pValue);

			value.erase(0, 1);
			value.erase(value.length() - 1, 1);
#ifdef DEBUG
			clog << "Found JSON element " << key << " = " << value << endl;
#endif
			if (m_content.empty() == false)
			{
				m_content += " ";
			}
			// FIXME: what about the key ?
			m_content.append(value.c_str());
		}
	}
}

bool JsonFilter::parse_json(const string &json_doc)
{
	if (json_doc.empty() == true)
	{
		return false;
	}

	m_metaData.clear();

	struct json_tokener *pTokener = json_tokener_new();

	if (pTokener != NULL)
	{
		json_object *pRoot = json_tokener_parse_ex(pTokener, json_doc.c_str(), (int)json_doc.length());
		if (pRoot != NULL)
		{
			json_tokener_error err = json_tokener_get_error(pTokener);

			if (err == json_tokener_success)
			{
				dump_object("", pRoot);
			}
		}

		json_tokener_free(pTokener);
	}

	m_metaData["ipath"] = "";
	m_metaData["mimetype"] = "text/plain";

	return true;
}
