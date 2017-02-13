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

#include <stdio.h>
#include <unistd.h>
#include <fileref.h>
#include <tfile.h>
#include <tag.h>
#include <iostream>

#include "TagLibMusicFilter.h"

using std::string;
using std::clog;
using std::endl;

using namespace Dijon;

#ifdef _DYNAMIC_DIJON_FILTERS
DIJON_FILTER_EXPORT bool get_filter_types(MIMETypes &mime_types)
{
	mime_types.m_mimeTypes.clear();
	mime_types.m_mimeTypes.insert("audio/mpeg");
	mime_types.m_mimeTypes.insert("audio/x-mp3");
	mime_types.m_mimeTypes.insert("application/ogg");
	mime_types.m_mimeTypes.insert("audio/x-flac+ogg");
	mime_types.m_mimeTypes.insert("audio/x-flac");

	return true;
}

DIJON_FILTER_EXPORT bool check_filter_data_input(int data_input)
{
	Filter::DataInput input = (Filter::DataInput)data_input;

	if (input == Filter::DOCUMENT_FILE_NAME)
	{
		return true;
	}

	return false;
}

DIJON_FILTER_EXPORT Filter *get_filter(void)
{
	return new TagLibMusicFilter();
}
#endif

TagLibMusicFilter::TagLibMusicFilter() :
	Filter(),
	m_parseDocument(false)
{
}

TagLibMusicFilter::~TagLibMusicFilter()
{
	rewind();
}

bool TagLibMusicFilter::is_data_input_ok(DataInput input) const
{
	if (input == DOCUMENT_FILE_NAME)
	{
		return true;
	}

	return false;
}

bool TagLibMusicFilter::set_property(Properties prop_name, const string &prop_value)
{
	return false;
}

bool TagLibMusicFilter::set_document_data(const char *data_ptr, off_t data_length)
{
	return false;
}

bool TagLibMusicFilter::set_document_string(const string &data_str)
{
	return false;
}

bool TagLibMusicFilter::set_document_file(const string &file_path, bool unlink_when_done)
{
	if (Filter::set_document_file(file_path, unlink_when_done) == true)
	{
		m_parseDocument = true;

		return true;
	}

	return false;
}

bool TagLibMusicFilter::set_document_uri(const string &uri)
{
	return false;
}

bool TagLibMusicFilter::has_documents(void) const
{
	return m_parseDocument;
}

bool TagLibMusicFilter::next_document(void)
{
	if (m_parseDocument == true)
	{
		m_parseDocument = false;

		m_content.clear();
		m_metaData.clear();

		TagLib::FileRef fileRef(m_filePath.c_str(), false);
		if (fileRef.isNull() == false)
		{
			TagLib::Tag *pTag = fileRef.tag();

			if ((pTag != NULL) &&
				(pTag->isEmpty() == false))
			{
				char yearStr[64];

				string trackTitle(pTag->title().toCString(true));
				trackTitle += " ";
				trackTitle += pTag->artist().toCString(true);

#ifdef DEBUG
				clog << "TagLibMusicFilter::next_document: " << trackTitle.length() << " bytes of text" << endl;
#endif
				m_content.append(trackTitle.c_str(), trackTitle.length());
				m_content += " ";
				m_content += pTag->album().toCString(true);
				m_content += " ";
				m_content += pTag->comment().toCString(true);
				m_content += " ";
				m_content += pTag->genre().toCString(true);
				snprintf(yearStr, 64, " %u", pTag->year());
				m_content += yearStr;

				m_metaData["title"] = trackTitle;
				m_metaData["ipath"] = "";
				m_metaData["mimetype"] = "text/plain";
				m_metaData["charset"] = "utf-8";
				m_metaData["author"] = pTag->artist().toCString(true);
			}
			else
			{
				// This file doesn't have any tag
				string::size_type filePos = m_filePath.find_last_of("/");
				if ((filePos != string::npos) &&
					(m_filePath.length() - filePos > 1))
				{
					m_metaData["title"] = m_filePath.substr(filePos + 1);
				}
				else
				{
					m_metaData["title"] = m_filePath;
				}
				m_metaData["ipath"] = "";
				m_metaData["mimetype"] = "text/plain";
				m_metaData["charset"] = "utf-8";
			}

			return true;
		}
	}

	return false;
}

bool TagLibMusicFilter::skip_to_document(const string &ipath)
{
	if (ipath.empty() == true)
	{
		return next_document();
	}

	return false;
}

string TagLibMusicFilter::get_error(void) const
{
	return "";
}

void TagLibMusicFilter::rewind(void)
{
	Filter::rewind();

	m_parseDocument = false;
}
