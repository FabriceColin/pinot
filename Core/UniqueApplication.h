/*
 *  Copyright 2008 Fabrice Colin
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

#ifndef _UNIQUEAPPLICATION_HH
#define _UNIQUEAPPLICATION_HH

#include <string>

#ifdef HAVE_UNIQUE
#include <unique/unique.h>
#endif

class UniqueApplication
{
	public:
		UniqueApplication(const std::string &name);
		~UniqueApplication();

		bool isRunning(void);

		bool isRunning(const std::string &pidFileName,
			const std::string &processName);

	private:
#ifdef HAVE_UNIQUE
		UniqueApp *m_pApp;
#endif

		UniqueApplication(const UniqueApplication &other);
		UniqueApplication &operator=(const UniqueApplication &other);

};

#endif // _UNIQUEAPPLICATION_HH
