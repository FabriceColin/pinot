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

#ifndef _DIJON_FILEOUTPUTFILTER_H
#define _DIJON_FILEOUTPUTFILTER_H

#include "Filter.h"

namespace Dijon
{
    class DIJON_FILTER_EXPORT FileOutputFilter : public Filter
    {
    public:
	/// Builds an empty filter.
	FileOutputFilter(const std::string &mime_type);
	/// Destroys the filter.
	virtual ~FileOutputFilter();

    protected:
	bool read_file(int fd, ssize_t maxSize, ssize_t &totalSize);

    };
}

#endif // _DIJON_FILEOUTPUTFILTER_H
