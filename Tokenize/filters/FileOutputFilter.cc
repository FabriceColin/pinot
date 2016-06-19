/*
 *  Copyright 2011-2012 Fabrice Colin
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
#include <errno.h>
#include <iostream>
#include <sstream>

#include "FileOutputFilter.h"

using std::string;
using std::stringstream;
using std::set;
using std::map;
using std::clog;
using std::endl;

using namespace Dijon;

FileOutputFilter::FileOutputFilter(const string &mime_type) :
	Filter(mime_type)
{
}

FileOutputFilter::~FileOutputFilter()
{
}

bool FileOutputFilter::read_file(int fd, ssize_t maxSize, ssize_t &totalSize)
{
	ssize_t bytesRead = 0;
	bool gotOutput = true;

	do
	{
		if ((maxSize > 0) &&
			(totalSize >= maxSize))
		{
#ifdef DEBUG
			clog << "FileOutputFilter::read_file: stopping at " << totalSize << endl;
#endif
			break;
		}

		char readBuffer[4096];
		bytesRead = read(fd, readBuffer, 4096);
		if (bytesRead > 0)
		{
			m_content.append(readBuffer, bytesRead);
			totalSize += bytesRead;
		}
		else if (bytesRead == -1)
		{
			// An error occured
			if (errno != EINTR)
			{
				gotOutput = false;
				break;
			}

			// Try again
			bytesRead = 1;
		}
	} while (bytesRead > 0);

	if (gotOutput == true)
	{
	        stringstream numStream;

	        numStream << totalSize;
	        m_metaData["size"] = numStream.str();
	}

	return gotOutput;
}

