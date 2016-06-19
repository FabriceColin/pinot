/*
 *  Copyright 2007 Fabrice Colin
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

#ifndef _DIJON_FILTERFACTORY_H
#define _DIJON_FILTERFACTORY_H

#include <string>
#include <map>
#include <set>

#include "Filter.h"
#ifndef _DYNAMIC_DIJON_HTMLFILTER
#include "HtmlFilter.h"
#endif
#ifndef _DYNAMIC_DIJON_XMLFILTER
#include "XmlFilter.h"
#endif

namespace Dijon
{
    /// Factory for filters with related utility methods.
    class FilterFactory
    {
    public:
	virtual ~FilterFactory();

	/// Loads the filter libraries found in the given directory.
	static unsigned int loadFilters(const std::string &dir_name);

	/// Returns a Filter that handles the given MIME type.
	static Filter *getFilter(const std::string &mime_type);

	/// Returns all supported MIME types.
	static void getSupportedTypes(std::set<std::string> &mime_types);

	/// Indicates whether a MIME type is supported or not.
	static bool isSupportedType(const std::string &mime_type);

	/// Unloads all filter libraries.
	static void unloadFilters(void);

    protected:
	static std::map<std::string, std::string> m_types;
	static std::map<std::string, void *> m_handles;

	FilterFactory();

	static Filter *getLibraryFilter(const std::string &mime_type);

    private:
	FilterFactory(const FilterFactory &other);
	FilterFactory& operator=(const FilterFactory& other);

    };
}

#endif // _DIJON_FILTERFACTORY_H
