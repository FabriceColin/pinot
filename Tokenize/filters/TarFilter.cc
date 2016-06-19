/*
 *  Copyright 2009-2010 Fabrice Colin
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

#include "TarFilter.h"

using std::string;
using std::clog;
using std::endl;
using std::stringstream;
using namespace Dijon;

#ifdef _DYNAMIC_DIJON_FILTERS
DIJON_FILTER_EXPORT bool get_filter_types(std::set<std::string> &mime_types)
{
	mime_types.clear();
	mime_types.insert("application/x-tar");

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
	return new TarFilter(mime_type);
}
#endif

TarFilter::TarFilter(const string &mime_type) :
	Filter(mime_type),
	m_maxSize(0),
	m_parseDocument(false),
	m_pHandle(NULL)
{
}

TarFilter::~TarFilter()
{
	rewind();
}

bool TarFilter::is_data_input_ok(DataInput input) const
{
	if (input == DOCUMENT_FILE_NAME)
	{
		return true;
	}

	return false;
}

bool TarFilter::set_property(Properties prop_name, const string &prop_value)
{
	if ((prop_name == MAXIMUM_NESTED_SIZE) &&
		(prop_value.empty() == false))
	{
		m_maxSize = (size_t)atol(prop_value.c_str());
	}

	return false;
}

bool TarFilter::set_document_data(const char *data_ptr, off_t data_length)
{
	return false;
}

bool TarFilter::set_document_string(const string &data_str)
{
	return false;
}

bool TarFilter::set_document_file(const string &file_path, bool unlink_when_done)
{
	if ((Filter::set_document_file(file_path, unlink_when_done) == true) &&
		(tar_open(&m_pHandle, const_cast<char*>(file_path.c_str()), NULL, O_RDONLY, 0, TAR_GNU|TAR_NOOVERWRITE) == 0))
	{
		m_parseDocument = true;

		return true;
	}

	return false;
}

bool TarFilter::set_document_uri(const string &uri)
{
	return false;
}

bool TarFilter::has_documents(void) const
{
	return m_parseDocument;
}

bool TarFilter::next_document(void)
{
	return next_document("");
}

bool TarFilter::next_document(const std::string &ipath)
{
	char *pFileName = NULL;
	bool foundFile = false;

	if (m_parseDocument == false)
	{
		return false;
	}

	do
	{
		if (th_read(m_pHandle) != 0)
		{
			m_parseDocument = false;
			return false;
		}

		pFileName = th_get_pathname(m_pHandle);
		if (pFileName == NULL)
		{
			return false;
		}

		if (ipath.empty() == true)
		{
			foundFile = true;
		}
		else if (ipath != pFileName)
		{
			tar_skip_regfile(m_pHandle);
		}
		else
		{
			foundFile = true;
		}
	} while (foundFile == false);

	stringstream sizeStream;
	size_t size = th_get_size(m_pHandle);

	m_content.clear();
	m_metaData.clear();
	m_metaData["title"] = pFileName;
	m_metaData["ipath"] = pFileName;
	sizeStream << size;
	m_metaData["size"] = sizeStream.str();
#ifdef DEBUG
	clog << "TarFilter::next_document: found " << pFileName << ", size " << size << endl;
#endif

	if (TH_ISDIR(m_pHandle))
	{
		m_metaData["mimetype"] = "x-directory/normal";
	}
	else if (TH_ISSYM(m_pHandle))
	{
		m_metaData["mimetype"] = "inode/symlink";
	}
	else if (TH_ISREG(m_pHandle))
	{
		char pBuffer[T_BLOCKSIZE];
		size_t readSize = 0, totalSize = 0;
		size_t blockNum = size;
		bool readFile = true;

		m_metaData["mimetype"] = "SCANTITLE";

		while (blockNum > 0)
		{
			readSize = tar_block_read(m_pHandle, pBuffer);
			if (readSize != T_BLOCKSIZE)
			{
				return false;
			}
			totalSize += readSize;

			if ((readFile == true) &&
				(m_maxSize > 0) &&
				(totalSize > m_maxSize))
			{
#ifdef DEBUG
				clog << "TarFilter::next_document: stopping at " << totalSize << endl;
#endif
				readFile = false;
			}

			if (blockNum > T_BLOCKSIZE)
			{
				if (readFile == true)
				{
					m_content.append(pBuffer, T_BLOCKSIZE);
				}
				blockNum -= T_BLOCKSIZE;
			}
			else
			{
				if (readFile == true)
				{
					m_content.append(pBuffer, blockNum);
				}
				blockNum = 0;
			}
		}


		return true;
	}

	return true;
}

bool TarFilter::skip_to_document(const string &ipath)
{
	return next_document(ipath);
}

string TarFilter::get_error(void) const
{
	return "";
}

void TarFilter::rewind(void)
{
	Filter::rewind();

	m_parseDocument = false;
	if (m_pHandle != NULL)
	{
		tar_close(m_pHandle);
		m_pHandle = NULL;
	}
}
