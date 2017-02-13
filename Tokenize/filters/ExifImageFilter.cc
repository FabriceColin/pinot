/*
 *  Copyright 2008-2016 Fabrice Colin
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
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <iostream>
#include <libexif/exif-data.h>
#include <libexif/exif-ifd.h>
#include <libexif/exif-loader.h>
#include <libexif/exif-utils.h>

#include "config.h"
#include "ExifImageFilter.h"

using std::string;
using std::clog;
using std::clog;
using std::endl;
using namespace Dijon;

#ifdef _DYNAMIC_DIJON_FILTERS
DIJON_FILTER_EXPORT bool get_filter_types(MIMETypes &mime_types)
{
	mime_types.m_mimeTypes.clear();
	mime_types.m_mimeTypes.insert("image/jpeg");

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
	return new ExifImageFilter();
}
#endif

class ExifMetaData
{
	public:
		ExifMetaData(dstring &content) :
			m_content(content)
		{
		}

		string m_title;
		string m_date;
		dstring &m_content;
};

static void entryCallback(ExifEntry *pEntry, void *pData)
{
	if ((pEntry == NULL) ||
		(pData == NULL))
	{
		return;
	}

	ExifMetaData *pMetaData = (ExifMetaData *)pData;
	struct tm timeTm;
	char value[1024];

	// Initialize the structure
	timeTm.tm_sec = timeTm.tm_min = timeTm.tm_hour = timeTm.tm_mday = 0;
	timeTm.tm_mon = timeTm.tm_year = timeTm.tm_wday = timeTm.tm_yday = timeTm.tm_isdst = 0;

	exif_entry_get_value(pEntry, value, 1024);
	switch (pEntry->tag)
	{
		case EXIF_TAG_DOCUMENT_NAME:
			pMetaData->m_title = value;
			break;
		case EXIF_TAG_DATE_TIME:
#ifdef HAVE_STRPTIME
			if (strptime(value, "%Y:%m:%d %H:%M:%S", &timeTm) != NULL)
#else
			{
				string valueStr(value);

				timeTm.tm_year = atoi(valueStr.substr(0, 4).c_str());
				timeTm.tm_mon = atoi(valueStr.substr(5, 2).c_str());
				timeTm.tm_mday = atoi(valueStr.substr(8, 2).c_str());
				timeTm.tm_hour = atoi(valueStr.substr(11, 2).c_str());
				timeTm.tm_min = atoi(valueStr.substr(14, 2).c_str());
				timeTm.tm_sec = atoi(valueStr.substr(17, 2).c_str());
			}
			if (timeTm.tm_mday > 0)
#endif
			{
				char timeStr[64];

#if defined(__GNU_LIBRARY__)
				// %z is a GNU extension
				if (strftime(timeStr, 64, "%a, %d %b %Y %H:%M:%S %z", &timeTm) > 0)
#else
				if (strftime(timeStr, 64, "%a, %d %b %Y %H:%M:%S %Z", &timeTm) > 0)
#endif
				{
					pMetaData->m_date = timeStr;
				}
			}
			break;
		default:
			pMetaData->m_content += " ";
			pMetaData->m_content.append(value, strlen(value));
			break;
	}
#ifdef DEBUG
	clog << "ExifImageFilter: tag " << exif_tag_get_name(pEntry->tag) << ": " << value << endl;
#endif
}

static void contentCallback(ExifContent *pContent, void *pData)
{
	exif_content_foreach_entry(pContent, entryCallback, pData);
}

ExifImageFilter::ExifImageFilter() :
	Filter(),
	m_parseDocument(false)
{
}

ExifImageFilter::~ExifImageFilter()
{
	rewind();
}

bool ExifImageFilter::is_data_input_ok(DataInput input) const
{
	if (input == DOCUMENT_FILE_NAME)
	{
		return true;
	}

	return false;
}

bool ExifImageFilter::set_property(Properties prop_name, const string &prop_value)
{
	return false;
}

bool ExifImageFilter::set_document_data(const char *data_ptr, off_t data_length)
{
	return false;
}

bool ExifImageFilter::set_document_string(const string &data_str)
{
	return false;
}

bool ExifImageFilter::set_document_file(const string &file_path, bool unlink_when_done)
{
	if (Filter::set_document_file(file_path, unlink_when_done) == true)
	{
		m_parseDocument = true;

		return true;
	}

	return false;
}

bool ExifImageFilter::set_document_uri(const string &uri)
{
	return false;
}

bool ExifImageFilter::has_documents(void) const
{
	return m_parseDocument;
}

bool ExifImageFilter::next_document(void)
{
	if (m_parseDocument == true)
	{
#ifdef DEBUG
		clog << "ExifImageFilter::next_document: " << m_filePath << endl;
#endif
		m_parseDocument = false;

		m_metaData["mimetype"] = "text/plain";
		m_metaData["charset"] = "utf-8";

		ExifData *pData = exif_data_new_from_file(m_filePath.c_str());
		if (pData == NULL)
		{
			clog << "No EXIF data in " << m_filePath.c_str() << endl;
		}
		else
		{
			ExifMetaData *pMetaData = new ExifMetaData(m_content);

			// Get it all
			exif_data_foreach_content(pData, contentCallback, pMetaData);

			m_metaData["title"] = pMetaData->m_title;
			if (pMetaData->m_date.empty() == false)
			{
				m_metaData["date"] = pMetaData->m_date;
			}

			delete pMetaData;
			exif_data_unref(pData);
		}

		return true;
	}

	return false;
}

bool ExifImageFilter::skip_to_document(const string &ipath)
{
	if (ipath.empty() == true)
	{
		return next_document();
	}

	return false;
}

string ExifImageFilter::get_error(void) const
{
	return "";
}

void ExifImageFilter::rewind(void)
{
	Filter::rewind();

	m_parseDocument = false;
}
