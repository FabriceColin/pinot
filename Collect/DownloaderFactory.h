/*
 *  Copyright 2005,2006 Fabrice Colin
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

#ifndef _DOWNLOADER_FACTORY_H
#define _DOWNLOADER_FACTORY_H

#include <string>

#include "DownloaderInterface.h"

/// Downloader factory class.
class DownloaderFactory
{
	public:
		virtual ~DownloaderFactory();

		/// Returns a Downloader; NULL if unavailable.
		static DownloaderInterface *getDownloader(const std::string &protocol);

	protected:
		DownloaderFactory();

	private:
		DownloaderFactory(const DownloaderFactory &other);
		DownloaderFactory &operator=(const DownloaderFactory &other);

};

#endif // _DOWNLOADER_FACTORY_H
