/*
 *  Copyright 2005-2013 Fabrice Colin
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

#ifndef _DOWNLOADER_INTERFACE_H
#define _DOWNLOADER_INTERFACE_H

#include <string>

#include "Document.h"

/// Interface implemented by downloaders.
class DownloaderInterface
{
	public:
		virtual ~DownloaderInterface();

		/// Initializes downloaders.
		static void initialize(void);

		/// Shutdowns downloaders.
		static void shutdown(void);

		/**
		  * Sets a (name, value) setting. Setting names include :
		  * proxyaddress - the address of the proxy to use
		  * proxyport - the port of the proxy to use (positive integer)
		  * proxytype - the type of the proxy to use
		  * timeout - timeout in seconds 
		  * method - GET or POST
		  * postfields - data to post
		  * Returns true if success.
		  */
		virtual bool setSetting(const std::string &name, const std::string &value);

		/// Retrieves the specified document; NULL if error. Caller deletes.
		virtual Document *retrieveUrl(const DocumentInfo &docInfo) = 0;

		/// Retrieves the specified document; NULL if error. Caller deletes.
		virtual Document *retrieveUrl(const DocumentInfo &docInfo,
			const std::map<std::string, std::string> &headers) = 0;

	protected:
		std::string m_userAgent;
		std::string m_proxyAddress;
		unsigned int m_proxyPort;
		std::string m_proxyType;
		unsigned int m_timeout;
		std::string m_method;
		std::string m_postFields;

		DownloaderInterface();

};

#endif // _DOWNLOADER_INTERFACE_H
