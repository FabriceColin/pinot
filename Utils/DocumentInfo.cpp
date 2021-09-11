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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sstream>
#include <algorithm>

#include "StringManip.h"
#include "TimeConverter.h"
#include "DocumentInfo.h"
#include "Url.h"

using std::string;
using std::stringstream;
using std::map;
using std::set;
using std::copy;
using std::inserter;

DocumentInfo::DocumentInfo() :
	m_score(0.0),
	m_indexId(0),
	m_docId(0)
{
	setField("modtime", TimeConverter::toTimestamp(time(NULL)));
}

DocumentInfo::DocumentInfo(const string &title, const string &location,
	const string &type, const string &language) :
	m_score(0.0),
	m_indexId(0),
	m_docId(0)
{
	setField("caption", title);
	setField("url", location);
	setField("type", type);
	setField("language", language);
	setField("modtime", TimeConverter::toTimestamp(time(NULL)));
}

DocumentInfo::DocumentInfo(const DocumentInfo &other) :
	m_fields(other.m_fields),
	m_extract(other.m_extract),
	m_score(other.m_score),
	m_labels(other.m_labels),
	m_indexId(other.m_indexId),
	m_docId(other.m_docId)
{
}

DocumentInfo::~DocumentInfo()
{
}

DocumentInfo& DocumentInfo::operator=(const DocumentInfo& other)
{
	if (this != &other)
	{
		m_fields = other.m_fields;
		m_extract = other.m_extract;
		m_score = other.m_score;
		m_labels = other.m_labels;
		m_indexId = other.m_indexId;
		m_docId = other.m_docId;
	}

	return *this;
}

bool DocumentInfo::operator<(const DocumentInfo& other) const
{
	string thisUrl(getField("url"));
	string otherUrl(other.getField("url"));

	if (thisUrl < otherUrl)
	{
		return true;
	}
	else if (thisUrl == otherUrl)
	{
		if (getField("ipath") < other.getField("ipath"))
		{
			return true;
		}
	}

	return false;
}

/// Serializes the document.
string DocumentInfo::serialize(SerialExtent extent) const
{
	string info;
	char numStr[64];

	// Serialize DocumentInfo into a string
	if ((extent == SERIAL_ALL) ||
		(extent == SERIAL_FIELDS))
	{
		for (map<string, string>::const_iterator fieldIter = m_fields.begin();
			fieldIter != m_fields.end(); ++fieldIter)
		{
			info += "\n";
			info += fieldIter->first;
			info += "=";
			info += fieldIter->second;
		}
		info += "\n";
	}
	if ((extent == SERIAL_ALL) ||
		(extent == SERIAL_LABELS))
	{
		info += "labels=";
		for (set<string>::const_iterator labelIter = m_labels.begin();
			labelIter != m_labels.end(); ++labelIter)
		{
			info += "[" + Url::escapeUrl(*labelIter) + "]";
		}
		info += "\n";
	}
	if (extent == SERIAL_ALL)
	{
		info += "extract=";
		info += m_extract;
		info += "\nscore=";
		snprintf(numStr, 64, "%f", m_score);
		info += numStr;
		info += "\nindexid=";
		snprintf(numStr, 64, "%u", m_indexId);
		info += numStr;
		info += "\ndocid=";
		snprintf(numStr, 64, "%u", m_docId);
		info += numStr;
		info += "\n";
	}

	return Url::escapeUrl(info);
}

/// Deserializes the document.
void DocumentInfo::deserialize(const string &info, SerialExtent extent)
{
	string unescapedInfo(Url::unescapeUrl(info));

	// Deserialize DocumentInfo
	if ((extent == SERIAL_ALL) ||
		(extent == SERIAL_FIELDS))
	{
		setField("caption", StringManip::extractField(unescapedInfo, "caption=", "\n"));
		setField("url", StringManip::extractField(unescapedInfo, "url=", "\n"));
		setField("ipath", StringManip::extractField(unescapedInfo, "ipath=", "\n"));
		setField("type", StringManip::extractField(unescapedInfo, "type=", "\n"));
		setField("language", StringManip::extractField(unescapedInfo, "language=", "\n"));
		setField("modtime", StringManip::extractField(unescapedInfo, "modtime=", "\n"));
		setField("size", StringManip::extractField(unescapedInfo, "size=", "\n"));
	}
	if ((extent == SERIAL_ALL) ||
		(extent == SERIAL_LABELS))
	{
		string labelsString(StringManip::extractField(unescapedInfo, "labels=", "\n"));
		if (labelsString.empty() == false)
		{
			string::size_type endPos = 0;
			string labelName(StringManip::extractField(labelsString, "[", "]", endPos));

			m_labels.clear();

			// Parse labels
			while (labelName.empty() == false)
			{
				m_labels.insert(Url::unescapeUrl(labelName));

				if (endPos == string::npos)
				{
					break;
				}
				labelName = StringManip::extractField(labelsString, "[", "]", endPos);
			}
		}
	}
	if (extent == SERIAL_ALL)
	{
		m_extract = StringManip::extractField(unescapedInfo, "extract=", "\n");
		m_score = (float)atof(StringManip::extractField(unescapedInfo, "score=", "\n").c_str());
		m_indexId = (unsigned int)atoi(StringManip::extractField(unescapedInfo, "indexid=", "\n").c_str());
		m_docId = (unsigned int)atoi(StringManip::extractField(unescapedInfo, "docid=", "\n").c_str());
	}
}

/// Sets the title of the document.
void DocumentInfo::setTitle(const string &title)
{
	setField("caption", title);
}

/// Returns the title of the document.
string DocumentInfo::getTitle(void) const
{
	return getField("caption");
}

/// Sets the original location of the document.
void DocumentInfo::setLocation(const string &location)
{
	setField("url", location);
}

/// Returns the original location of the document.
string DocumentInfo::getLocation(bool withIPath) const
{
	string url(getField("url"));

	if (withIPath == false)
	{
		return url;
	}

	string ipath(getField("ipath"));

	if (ipath.empty() == false)
	{
		url += "?";
		url += ipath;
	}

	return url;
}

/// Sets the internal path to the document.
void DocumentInfo::setInternalPath(const string &ipath)
{
	setField("ipath", ipath);
}

/// Returns the internal path to the document.
string DocumentInfo::getInternalPath(void) const
{
	return getField("ipath");
}

/// Sets the type of the document.
void DocumentInfo::setType(const string &type)
{
	setField("type", type);
}

/// Returns the type of the document.
string DocumentInfo::getType(bool withCharset) const
{
	string type(getField("type"));

	if (withCharset == false)
	{
		// Drop the charset, if any
		string::size_type semiColonPos = type.find(";");

		if (semiColonPos != string::npos)
		{
			 type.erase(semiColonPos);
		}
	}

	return type;
}

/// Returns whether the document is a directory.
bool DocumentInfo::getIsDirectory(void) const
{
	string type(getField("type"));

	if (type.find("x-directory") == string::npos)
	{
		return false;
	}

	return true;
}

/// Sets the language of the document.
void DocumentInfo::setLanguage(const string &language)
{
	setField("language", language);
}

/// Returns the document's language.
string DocumentInfo::getLanguage(void) const
{
	return getField("language");
}

/// Sets the document's timestamp.
void DocumentInfo::setTimestamp(const string &timestamp)
{
	setField("modtime", timestamp);
}

/// Returns the document's timestamp.
string DocumentInfo::getTimestamp(void) const
{
	return getField("modtime");
}

/// Sets the document's size in bytes.
void DocumentInfo::setSize(off_t size)
{
	stringstream sizeStr;

	sizeStr << size;
	setField("size", sizeStr.str());
}

/// Returns the document's size in bytes.
off_t DocumentInfo::getSize(void) const
{
	string sizeStr(getField("size"));

	if (sizeStr.empty() == true)
	{
		return 0;
	}

	return (off_t)atoll(sizeStr.c_str());
}

/// Sets the document's extract.
void DocumentInfo::setExtract(const string &extract)
{
	m_extract = extract;
}

/// Returns the document's extract.
string DocumentInfo::getExtract(void) const
{
	return m_extract;
}

/// Sets the document's score.
void DocumentInfo::setScore(float score)
{
	m_score = score;
}

/// Returns the document's score.
float DocumentInfo::getScore(void) const
{
	return m_score;
}

/// Sets a foreign field.
void DocumentInfo::setOther(const string &name,
	const string &value)
{
	setField(name, value);
}

/// Returns a foreign field.
string DocumentInfo::getOther(const string &name) const
{
	return getField(name);
}

/// Sets the document's labels.
void DocumentInfo::setLabels(const set<string> &labels)
{
	copy(labels.begin(), labels.end(),
		inserter(m_labels, m_labels.begin()));
}

/// Returns the document's labels.
const set<string> &DocumentInfo::getLabels(void) const
{
	return m_labels;
}

/// Sets that the document is indexed.
void DocumentInfo::setIsIndexed(unsigned int indexId, unsigned int docId)
{
	m_indexId = indexId;
	m_docId = docId;
}

/// Sets that the document is not indexed.
void DocumentInfo::setIsNotIndexed(void)
{
	m_indexId = 0;
	m_docId = 0;
}

/// Gets whether the document is indexed.
bool DocumentInfo::getIsIndexed(void) const
{
	if (m_docId > 0)
	{
		return true;
	}

	return false;
}

/// Gets whether the documentt is indexed.
unsigned int DocumentInfo::getIsIndexed(unsigned int &indexId) const
{
	indexId = m_indexId;

	return m_docId;
}

void DocumentInfo::setField(const string &name, const string &value)
{
	m_fields[name] = value;
}

string DocumentInfo::getField(const string &name) const
{
	map<string, string>::const_iterator fieldIter = m_fields.find(name);
	if (fieldIter != m_fields.end())
	{
		return fieldIter->second;
	}

	return "";
}

