/*
 *  Copyright 2011-2019 Fabrice Colin
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

#include "config.h"
#include <unistd.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <iostream>
#include <exiv2/image.hpp>
#include <exiv2/exif.hpp>
#include <exiv2/iptc.hpp>
#ifdef HAVE_EXIV2_XMP_EXIV2_HPP
#include <exiv2/xmp_exiv2.hpp>
#include <exiv2/error.hpp>
#else
#include <exiv2/xmp.hpp>
#endif

#include "config.h"
#include "Exiv2ImageFilter.h"

using std::string;
using std::clog;
using std::clog;
using std::endl;
using namespace Dijon;

#ifdef _DYNAMIC_DIJON_FILTERS
DIJON_FILTER_EXPORT bool get_filter_types(MIMETypes &mime_types)
{
	mime_types.m_mimeTypes.clear();
	// List from http://dev.exiv2.org/wiki/exiv2/Supported_image_formats
	// without application/rdf+xml
	mime_types.m_mimeTypes.insert("image/jpeg");
	mime_types.m_mimeTypes.insert("image/x-exv");
	mime_types.m_mimeTypes.insert("image/x-canon-cr2");
	mime_types.m_mimeTypes.insert("image/x-canon-crw");
	mime_types.m_mimeTypes.insert("image/x-minolta-mrw");
	mime_types.m_mimeTypes.insert("image/tiff");
	mime_types.m_mimeTypes.insert("image/x-nikon-nef");
	mime_types.m_mimeTypes.insert("image/x-pentax-pef");
	mime_types.m_mimeTypes.insert("image/x-panasonic-rw2");
	mime_types.m_mimeTypes.insert("image/x-samsung-srw");
	mime_types.m_mimeTypes.insert("image/x-olympus-orf");
	mime_types.m_mimeTypes.insert("image/png");
	mime_types.m_mimeTypes.insert("image/pgf");
	mime_types.m_mimeTypes.insert("image/x-fuji-raf");
	mime_types.m_mimeTypes.insert("image/x-photoshop");
	mime_types.m_mimeTypes.insert("image/targa");
	mime_types.m_mimeTypes.insert("image/x-ms-bmp");
	mime_types.m_mimeTypes.insert("image/jp2");

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
	return new Exiv2ImageFilter();
}
#endif

static string iptcDateTime(const string &ccyymmdd, const string &hhmmss)
{
	struct tm timeTm;

	// Initialize the structure
	timeTm.tm_sec = timeTm.tm_min = timeTm.tm_hour = timeTm.tm_mday = 0;
	timeTm.tm_mon = timeTm.tm_year = timeTm.tm_wday = timeTm.tm_yday = timeTm.tm_isdst = 0;

#ifdef HAVE_STRPTIME
	if ((strptime(ccyymmdd.c_str(), "%C%Y%m%d", &timeTm) != NULL) &&
		(strptime(hhmmss.c_str(), "%H%M%S", &timeTm) != NULL))
#else
	timeTm.tm_year = atoi(ccyymmdd.substr(2, 4).c_str());
	timeTm.tm_mon = atoi(ccyymmdd.substr(6, 2).c_str());
	timeTm.tm_mday = atoi(ccyymmdd.substr(8, 2).c_str());
	timeTm.tm_hour = atoi(hhmmss.substr(0, 2).c_str());
	timeTm.tm_min = atoi(hhmmss.substr(2, 2).c_str());
	timeTm.tm_sec = atoi(hhmmss.substr(4, 2).c_str());

	if (timeTm.tm_yday > 0)
#endif
	{
		char timeStr[64];

		if (strftime(timeStr, 64, "%a, %d %b %Y %H:%M:%S", &timeTm) > 0)
		{
#ifdef DEBUG
			clog << "IPTC " << ccyymmdd << " " << hhmmss << " is " << timeStr << endl;
#endif
			return timeStr;
		}
	}

	return "";
}

static string exifDateTime(const string &value)
{
	struct tm timeTm;

	// Initialize the structure
	timeTm.tm_sec = timeTm.tm_min = timeTm.tm_hour = timeTm.tm_mday = 0;
	timeTm.tm_mon = timeTm.tm_year = timeTm.tm_wday = timeTm.tm_yday = timeTm.tm_isdst = 0;

#ifdef HAVE_STRPTIME
	if (strptime(value.c_str(), "%Y:%m:%d %H:%M:%S", &timeTm) != NULL)
#else
	timeTm.tm_year = atoi(value.substr(0, 4).c_str());
	timeTm.tm_mon = atoi(value.substr(5, 2).c_str());
	timeTm.tm_mday = atoi(value.substr(8, 2).c_str());
	timeTm.tm_hour = atoi(value.substr(11, 2).c_str());
	timeTm.tm_min = atoi(value.substr(14, 2).c_str());
	timeTm.tm_sec = atoi(value.substr(17, 2).c_str());

	if (timeTm.tm_mday > 0)
#endif
	{
		char timeStr[64];

		if (strftime(timeStr, 64, "%a, %d %b %Y %H:%M:%S", &timeTm) > 0)
		{
#ifdef DEBUG
			clog << "EXIF " << value << " is " << timeStr << endl;
#endif
			return timeStr;
		}
	}

	return "";
}

Exiv2ImageFilter::Exiv2ImageFilter() :
	Filter(),
	m_parseDocument(false)
{
}

Exiv2ImageFilter::~Exiv2ImageFilter()
{
	rewind();
}

bool Exiv2ImageFilter::is_data_input_ok(DataInput input) const
{
	if (input == DOCUMENT_FILE_NAME)
	{
		return true;
	}

	return false;
}

bool Exiv2ImageFilter::set_property(Properties prop_name, const string &prop_value)
{
	return false;
}

bool Exiv2ImageFilter::set_document_data(const char *data_ptr, off_t data_length)
{
	return false;
}

bool Exiv2ImageFilter::set_document_string(const string &data_str)
{
	return false;
}

bool Exiv2ImageFilter::set_document_file(const string &file_path, bool unlink_when_done)
{
	if (Filter::set_document_file(file_path, unlink_when_done) == true)
	{
		m_parseDocument = true;

		return true;
	}

	return false;
}

bool Exiv2ImageFilter::set_document_uri(const string &uri)
{
	return false;
}

bool Exiv2ImageFilter::has_documents(void) const
{
	return m_parseDocument;
}

bool Exiv2ImageFilter::next_document(void)
{
	bool foundData = true;

	if (m_parseDocument == false)
	{
		return false;
	}
#ifdef DEBUG
	clog << "Exiv2ImageFilter::next_document: " << m_filePath << endl;
#endif

	m_parseDocument = false;
	m_metaData["mimetype"] = "text/plain";
	m_metaData["charset"] = "utf-8";
	m_metaData["title"] = m_filePath;

	try
	{
		Exiv2::Image::AutoPtr image = Exiv2::ImageFactory::open(m_filePath);
		if (image.get() == NULL)
		{
			clog << m_filePath.c_str() << " is not an image" << endl;
			return false;
		}

		image->readMetadata();

		// Tag reference at http://www.exiv2.org/metadata.html
		Exiv2::XmpData &xmpData = image->xmpData();
		if (xmpData.empty() == false)
		{
#ifdef DEBUG
			clog << "Exiv2ImageFilter::next_document: XMP data in " << m_filePath << endl;
#endif
			for (Exiv2::XmpData::const_iterator tagIter = xmpData.begin();
				tagIter != xmpData.end(); ++tagIter)
			{
				const char *pTypeName = tagIter->typeName();

				if ((pTypeName == NULL) ||
					(strncasecmp(pTypeName, "Text", 4) != 0))
				{
					continue;
				}

				const Exiv2::Value &value = tagIter->value();
				string key(tagIter->key());
				string valueStr(value.toString());

				if (valueStr.empty() == false)
				{
					m_content += " ";
					m_content.append(key.c_str(), key.length());
					m_content += " ";
					m_content.append(valueStr.c_str(), valueStr.length());
				}
#ifdef DEBUG
				clog << "Exiv2ImageFilter::next_document: " << key << "=" << value << endl;
#endif
			}
		}
		Exiv2::IptcData &iptcData = image->iptcData();
		if (iptcData.empty() == false)
		{
			string iptcDate, iptcTime;

#ifdef DEBUG
			clog << "Exiv2ImageFilter::next_document: IPTC data in " << m_filePath << endl;
#endif
			for (Exiv2::IptcData::const_iterator tagIter = iptcData.begin();
				tagIter != iptcData.end(); ++tagIter)
			{
				const char *pTypeName = tagIter->typeName();

				if (pTypeName == NULL)
				{
					continue;
				}

				const Exiv2::Value &value = tagIter->value();
				string key(tagIter->key());
				string valueStr(value.toString());

#ifdef DEBUG
				clog << "Exiv2ImageFilter::next_document: " << key << "=" << value << endl;
#endif
				if ((strncasecmp(pTypeName, "Date", 4) == 0) &&
					(key == "Iptc.Application2.DateCreated"))
				{
					iptcDate = valueStr;
				}
				else if ((strncasecmp(pTypeName, "Time", 4) == 0) &&
					(key == "Iptc.Application2.TimeCreated"))
				{
					iptcTime = valueStr;
				}
				else if (strncasecmp(pTypeName, "String", 6) != 0)
				{
					continue;
				}

				if (key.find(".ObjectName") != string::npos)
				{
					m_metaData["title"] = valueStr;
				}
				else if (valueStr.empty() == false)
				{
					m_content += " ";
					m_content.append(key.c_str(), key.length());
					m_content += " ";
					m_content.append(valueStr.c_str(), valueStr.length());
				}
			}

			if ((iptcDate.empty() == false) ||
				(iptcTime.empty() == false))
			{
				m_metaData["date"] = iptcDateTime(iptcDate, iptcTime);
			}
		}
		Exiv2::ExifData &exifData = image->exifData();
		if (exifData.empty() == false)
		{
			bool foundDate = false;

#ifdef DEBUG
			clog << "Exiv2ImageFilter::next_document: EXIF data in " << m_filePath << endl;
#endif
			for (Exiv2::ExifData::const_iterator tagIter = exifData.begin();
				tagIter != exifData.end(); ++tagIter)
			{
				const char *pTypeName = tagIter->typeName();

				if ((pTypeName == NULL) ||
					(strncasecmp(pTypeName, "Ascii", 5) != 0))
				{
					continue;
				}

				const Exiv2::Value &value = tagIter->value();
				string key(tagIter->key());
				string valueStr(value.toString());

#ifdef DEBUG
				clog << "Exiv2ImageFilter::next_document: " << key << "=" << value << endl;
#endif
				if (key == "Exif.Image.DocumentName")
				{
					m_metaData["title"] = valueStr;
				}
				else if (key.find("Date") != string::npos)
				{
					if (((key == "Exif.Photo.DateTimeOriginal") ||
						(key == "Exif.Image.DateTimeOriginal")) &&
						(foundDate == false))
					{
						m_metaData["date"] = exifDateTime(valueStr);
						foundDate = true;
					}
				}
				else if (valueStr.empty() == false)
				{
					m_content += " ";
					m_content.append(key.c_str(), key.length());
					m_content += " ";
					m_content.append(valueStr.c_str(), valueStr.length());
				}
			}
		}
	}
	catch (Exiv2::AnyError &e)
	{
		clog << "Caught exiv2 exception: " << e << endl;
		foundData = false;
	}
	catch (...)
	{
		clog << "Caught unknown exception" << endl;
		foundData = false;
	}

	return foundData;
}

bool Exiv2ImageFilter::skip_to_document(const string &ipath)
{
	if (ipath.empty() == true)
	{
		return next_document();
	}

	return false;
}

string Exiv2ImageFilter::get_error(void) const
{
	return "";
}

void Exiv2ImageFilter::rewind(void)
{
	Filter::rewind();

	m_parseDocument = false;
}
