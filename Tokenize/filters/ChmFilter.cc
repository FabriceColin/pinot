/*
 *  Copyright 2011 Fabrice Colin
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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include <sstream>

#include "ChmFilter.h"

using std::string;
using std::vector;
using std::clog;
using std::endl;
using std::stringstream;
using namespace Dijon;

#ifdef _DYNAMIC_DIJON_FILTERS
DIJON_FILTER_EXPORT bool get_filter_types(std::set<std::string> &mime_types)
{
	mime_types.clear();
	mime_types.insert("application/x-chm");

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

DIJON_FILTER_EXPORT Filter *get_filter(const std::string &mime_type)
{
	return new ChmFilter(mime_type);
}
#endif

static int enumerator(struct chmFile *pHandle,
	struct chmUnitInfo *pUnitInfo,
	void *pContext)
{
	if (pContext == NULL)
	{
		return CHM_ENUMERATOR_CONTINUE;
	}
	ChmFilter *pFilter = static_cast<ChmFilter*>(pContext);

	if ((pUnitInfo != NULL) &&
		(pUnitInfo->flags & CHM_ENUMERATE_FILES) &&
		(pUnitInfo->length > 0) &&
		(pUnitInfo->path != NULL))
	{
#ifdef DEBUG
		clog << "ChmFilter: found " << pUnitInfo->path << ", size " << pUnitInfo->length << endl;
#endif
		pFilter->add_unit(pUnitInfo);
	}

	return CHM_ENUMERATOR_CONTINUE;
}

ChmFilter::ChmFilter(const string &mime_type) :
	Filter(mime_type),
	m_maxSize(0),
	m_pHandle(NULL),
	m_doneAll(false)
{
}

ChmFilter::~ChmFilter()
{
	rewind();
}

bool ChmFilter::is_data_input_ok(DataInput input) const
{
	if (input == DOCUMENT_FILE_NAME)
	{
		return true;
	}

	return false;
}

bool ChmFilter::set_property(Properties prop_name, const string &prop_value)
{
	if ((prop_name == MAXIMUM_NESTED_SIZE) &&
		(prop_value.empty() == false))
	{
		m_maxSize = (size_t)atol(prop_value.c_str());
	}

	return false;
}

bool ChmFilter::set_document_data(const char *data_ptr, off_t data_length)
{
	return false;
}

bool ChmFilter::set_document_string(const string &data_str)
{
	return false;
}

bool ChmFilter::set_document_file(const string &file_path, bool unlink_when_done)
{
	if ((Filter::set_document_file(file_path, unlink_when_done) == true) &&
		((m_pHandle = chm_open(file_path.c_str())) != NULL))
	{
		return true;
	}

	return false;
}

bool ChmFilter::set_document_uri(const string &uri)
{
	return false;
}

bool ChmFilter::has_documents(void) const
{
	if ((m_pHandle != NULL) &&
		(m_doneAll == false))
	{
		return true;
	}

	return false;
}

bool ChmFilter::next_document(void)
{
	return next_document("");
}

bool ChmFilter::next_document(const std::string &ipath)
{
	struct chmUnitInfo unitInfo;
	struct chmUnitInfo *pUnitInfo = NULL;
	bool deleteUnitInfo = false;

	if (m_pHandle == NULL)
	{
		return false;
	}

	m_content.clear();
	m_metaData.clear();

	if (ipath.empty() == false)
	{
		// Resolve this
		if (chm_resolve_object(m_pHandle,
			ipath.c_str(),
			&unitInfo) != CHM_RESOLVE_SUCCESS)
		{
			return false;
		}

		pUnitInfo = &unitInfo;
		m_doneAll = true;
	}
	else
	{
		if (m_units.empty() == true)
		{
			// Enumerate content
			if ((chm_enumerate(m_pHandle,
				CHM_ENUMERATE_ALL,
				enumerator, this) == 0))
			{
				return false;
			}
		}

		vector<struct chmUnitInfo*>::iterator unitIter = m_units.begin();
		if (unitIter == m_units.end())
		{
			return false;
		}

		pUnitInfo = *unitIter;
		deleteUnitInfo = true;
		m_units.erase(unitIter);
		m_doneAll = m_units.empty();
	}

	if (pUnitInfo != NULL)
	{
		char *pBuffer = NULL;

		if (pUnitInfo->length > 0)
		{
			pBuffer = Memory::allocateBuffer(pUnitInfo->length + 1);
		}
		if (pBuffer != NULL)
		{
			if ((pUnitInfo->path != NULL) &&
				(chm_retrieve_object(m_pHandle, pUnitInfo,
					(unsigned char*)pBuffer, 0, pUnitInfo->length) != 0))
			{
				stringstream sizeStream;

				m_content = pBuffer;

				m_metaData["title"] = pUnitInfo->path;
				m_metaData["ipath"] = pUnitInfo->path;
				sizeStream << pUnitInfo->length;
				m_metaData["size"] = sizeStream.str();
				m_metaData["mimetype"] = "SCAN";
#ifdef DEBUG
				clog << "ChmFilter::next_document: returning "
					<< pUnitInfo->path << ", size " << m_content.size() << endl;
#endif
			}

			Memory::freeBuffer(pBuffer, pUnitInfo->length + 1);
		}

		if (deleteUnitInfo == true)
		{
			delete pUnitInfo;
		}
	}

	return !m_content.empty();
}

bool ChmFilter::skip_to_document(const string &ipath)
{
	return next_document(ipath);
}

string ChmFilter::get_error(void) const
{
	return "";
}

void ChmFilter::rewind(void)
{
	Filter::rewind();

	for (vector<struct chmUnitInfo*>::iterator unitIter = m_units.begin();
		unitIter != m_units.end(); ++unitIter)
	{
		struct chmUnitInfo *pUnitInfo = *unitIter;
		delete pUnitInfo;
	}
	m_units.clear();
	if (m_pHandle != NULL)
	{
		chm_close(m_pHandle);
		m_pHandle = NULL;
	}
	m_doneAll = false;
}

void ChmFilter::add_unit(struct chmUnitInfo *pUnitInfo)
{
	if (pUnitInfo == NULL)
	{
		return;
	}

	struct chmUnitInfo *pCopy = new struct chmUnitInfo;
	memcpy((void*)pCopy, pUnitInfo, sizeof(struct chmUnitInfo));

	m_units.push_back(pCopy);
}
