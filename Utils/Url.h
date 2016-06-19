/*
 *  Copyright 2005-2009 Fabrice Colin
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

#ifndef _URL_H
#define _URL_H

#include <string>

#include "Visibility.h"

/// This dissects and manipulates URLs.
class PINOT_EXPORT Url
{
	public:
		Url(const std::string &url);
		Url(const std::string &path, const std::string &parentPath);
		Url(const Url &other);
		virtual ~Url();

		Url& operator=(const Url& other);

		/// Canonicalizes an URL.
		static std::string canonicalizeUrl(const std::string &url);

		/// Truncates an URL to the given length by discarding characters in the middle.
		static std::string prettifyUrl(const std::string &url, unsigned int maxLen);

		/// Reduces a host name to the given TLD level.
		static std::string reduceHost(const std::string &hostName, unsigned int level);

		/// Escapes an URL.
		static std::string escapeUrl(const std::string &url);

		/// Unescapes an URL.
		static std::string unescapeUrl(const std::string &escapedUrl);

		/// Resolves a path.
		static std::string resolvePath(const std::string &currentDir, const std::string &location);

		/// Returns the protocol, eg "file".
		std::string getProtocol(void) const;

		/// Returns the user name if any.
		std::string getUser(void) const;

		/// Returns the password if any.
		std::string getPassword(void) const;

		/// Returns the host name.
		std::string getHost(void) const;

		/// Returns the directory that leads to the file.
		std::string getLocation(void) const;

		/// Returns the file name.
		std::string getFile(void) const;

		/// Returns parameters that may follow the file name.
		std::string getParameters(void) const;

		/// Returns whether the Url points to a local file.
		bool isLocal(void) const;

	protected:
		std::string m_protocol;
		std::string m_user;
		std::string m_password;
		std::string m_host;
		std::string m_location;
		std::string m_file;
		std::string m_parameters;

		void parse(const std::string &url);

		bool isLocal(const std::string &protocol) const;

};

#endif // _URL_H
