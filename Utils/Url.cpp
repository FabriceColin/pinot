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

#include <stdio.h>
#include <unistd.h>
#include <iostream>
#include <glibmm/miscutils.h>

#include "StringManip.h"
#include "Url.h"

using std::string;
using std::clog;
using std::endl;

static const int g_rfc2396Encoded[] = {
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  /* 0x00 - 0x0f */
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  /* 0x10 - 0x1f */
	1, 0, 1, 1, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0,  /*  ' ' - '/'  */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0,  /*  '0' - '?'  */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /*  '@' - 'O'  */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0,  /*  'P' - '_'  */
	1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /*  '`' - 'o'  */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1,  /*  'p' - 0x7f */
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
};

Url::Url(const string &url)
{
	parse(url);
}

Url::Url(const string &path, const string &parentPath)
{
	string absoluteUrl;

	// Is this a relative path ?
	string::size_type pos = path.find("://");
	if ((pos == string::npos) &&
		(Glib::path_is_absolute(path) == false))
	{
		if (parentPath.empty() == true)
		{
			char *pCurrentDir = (char *)malloc(sizeof(char) * PATH_MAX);

			if (pCurrentDir != NULL)
			{
				if (getcwd(pCurrentDir, PATH_MAX) != NULL)
				{
					absoluteUrl = Url::resolvePath(pCurrentDir, path);
				}
				free(pCurrentDir);
			}
		}
		else
		{
			absoluteUrl = Url::resolvePath(parentPath, path);
		}
	}

	if (absoluteUrl.empty() == false)
	{
		parse(absoluteUrl);
	}
	else
	{
		parse(path);
	}
}

Url::Url(const Url &other) :
	m_protocol(other.m_protocol),
	m_user(other.m_user),
	m_password(other.m_password),
	m_host(other.m_host),
	m_location(other.m_location),
	m_file(other.m_file),
	m_parameters(other.m_parameters)
{
}

Url::~Url()
{
}

Url& Url::operator=(const Url& other)
{
	if (this != &other)
	{
		m_protocol = other.m_protocol;
		m_user = other.m_user;
		m_password = other.m_password;
		m_host = other.m_host;
		m_location = other.m_location;
		m_file = other.m_file;
		m_parameters = other.m_parameters;
	}

	return *this;
}

void Url::parse(const string &url)
{
	string::size_type pos1 = 0, pos2 = 0;
	bool hasHostName = true, hasParameters = true;

	if ((url[0] == '/') ||
		(url[0] == '.'))
	{
		if ((url.length() > 2) &&
				(url.substr(0, 2) == "./"))
		{
			pos2 = 2;
		}

		// Assume default protocol
		m_protocol = "file";
		hasHostName = false;
	}
	else
	{
		// Protocol
		pos1 = url.find("://");
		if (pos1 != string::npos)
		{
			m_protocol = StringManip::toLowerCase(url.substr(0, pos1));
			pos1 += 3;
		}
		else
		{
			// Assume default protocol
			m_protocol = "file";
			pos1 = 0;
		}

		if (isLocal(m_protocol) == true)
		{
			hasHostName = false;
			pos2 = pos1;
		}
	}

	if (m_protocol == "file")
	{
		hasParameters = false;
	}

	if (hasHostName == true)
	{
		string userAndPassword;

		// User and password
		string::size_type atPos = url.find_first_of("@", pos1);
		if (atPos != string::npos)
		{
			userAndPassword = url.substr(pos1, atPos - pos1);
		}

		pos2 = userAndPassword.find_first_of(":");
		if (pos2 != string::npos)
		{
			bool isPartOfLocation = false;

			string::size_type firstSlash = userAndPassword.find_first_of("/");
			if (firstSlash != string::npos)
			{
				// The : is part of the location if it follows the /, eg like in this URL :
				// http://216.239.39.100/search?q=cache:X8L8R9AazsAJ:eastenwest.free.fr/site/php/download.php%3Ftype%3Darticles%26ID%3D193+fabrice+colin&hl=en&ie=UTF-8
				if (pos2 > firstSlash)
				{
					isPartOfLocation = true;
				}
			}

			if (isPartOfLocation == false)
			{
				m_user = userAndPassword.substr(0, pos2);
				pos1 = pos2 + 1;

				pos2 = userAndPassword.find_first_of("@", pos1);
				if (pos2 != string::npos)
				{
					m_password = userAndPassword.substr(pos2 + 1);
					pos1 = atPos + 1;
				}
			}
		}

		// Host name
		pos2 = url.find_first_of("/", pos1);
		if (pos2 != string::npos)
		{
			m_host = url.substr(pos1, pos2 - pos1);
			pos2++;
		}
		else
		{
			if (url.find_first_of("?", pos1) == string::npos)
			{
				m_host = url.substr(pos1);
				return;
			}
			pos2 = 0;
		}
		// FIXME: what about the port number ?
	}
	else
	{
		m_host = "localhost";
	}

	string locationAndFile(url.substr(pos2));
	// Parameters
	if (hasParameters == true)
	{
		pos2 = locationAndFile.find("?");
		if (pos2 != string::npos)
		{
			m_parameters = locationAndFile.substr(pos2+1);
			locationAndFile.resize(pos2);
		}
	}

	// Location and file
	pos1 = locationAndFile.find_last_of("/");
	if (pos1 != string::npos)
	{
		m_location = locationAndFile.substr(0, pos1);
		m_file = locationAndFile.substr(pos1+1);
	}
	else
	{
		// No slash found, what we have got is either a directory
		// directly under the root or a file name
		// Assume this is a directory unless there's a dot
		if (locationAndFile.find('.') == string::npos)
		{
			m_location = locationAndFile;
			m_file = "";
		}
		else
		{
			m_location = "";
			m_file = locationAndFile;
		}
	}
}

bool Url::isLocal(const string &protocol) const
{
	if (protocol == "file")
	{
		return true;
	}

	return false;
}

/// Canonicalizes an URL.
string Url::canonicalizeUrl(const string &url)
{
	if (url.empty() == true)
	{
		return "";
	}

	Url urlObj(url);
	string canonicalUrl(url);
	string location = urlObj.getLocation();
	string file = urlObj.getFile();

	if (urlObj.isLocal() == false)
	{
		string host = urlObj.getHost();

		// Lower-case the host name
		string::size_type pos = canonicalUrl.find(host);
		if (pos != string::npos)
		{
			canonicalUrl.replace(pos, host.length(), StringManip::toLowerCase(host));
		}
	}

	// Get rid of the last directory's slash
	if ((file.empty() == true) &&
		(location.empty() == false) &&
		(canonicalUrl[canonicalUrl.length() - 1] == '/'))
	{
		return canonicalUrl.substr(0, url.length() - 1);
	}

	return canonicalUrl;
}

/// Truncates an URL to the given length by discarding characters in the middle.
string Url::prettifyUrl(const string &url, unsigned int maxLen)
{
	if (maxLen >= url.length())
	{
		// Don't change anything...
		return url;
	}

	unsigned int diffLen = url.length() - maxLen;
	Url urlObj(url);
	string protocol = urlObj.getProtocol();
	string user = urlObj.getUser();
	string password = urlObj.getPassword();
	string host = urlObj.getHost();
	string location = urlObj.getLocation();
	string file = urlObj.getFile();
	
	string prettyUrl = protocol;
	prettyUrl += "://";
	if (user.empty() == false)
	{
		prettyUrl += user;
		prettyUrl += ":";
		prettyUrl += password;
	}
	if (urlObj.isLocal() == false)
	{
		prettyUrl += host;
	}
	prettyUrl += "/";

	if (url.length() <= diffLen)
	{
		// That's the bare minimum...
		prettyUrl = protocol;
		prettyUrl += "://";
		if (urlObj.isLocal() == false)
		{
			prettyUrl += host;
		}
		prettyUrl += "/...";
	}
	else if (location.length() > diffLen + 3)
	{
		// Truncate the location and keep the rest intact
		prettyUrl += location.substr(0, location.length() - (diffLen + 3));
		prettyUrl += ".../";
		prettyUrl += file;
	}
	else
	{
		// Cut somewhere in the middle of the URL then
		prettyUrl += location;
		prettyUrl += "/";
		prettyUrl += file;
		unsigned int urlLen = prettyUrl.length();
		string::size_type startPos = 0;
		if (urlLen - diffLen > 0)
		{
			startPos = (urlLen - diffLen) / 2;
		}
		string tmp = prettyUrl;
		prettyUrl = tmp.substr(0, startPos);
		prettyUrl += "...";
		prettyUrl += tmp.substr(startPos + diffLen);
	}

	return prettyUrl;
}

/// Reduces a host name to the given TLD level.
string Url::reduceHost(const string &hostName, unsigned int level)
{
	string reducedHost;
	unsigned int currentLevel = 0;

	if (hostName.empty() == true)
	{
		return "";
	}

	string::size_type endPos = string::npos;
	string::size_type pos = hostName.find_last_of(".");
	while ((pos != string::npos) &&
		(currentLevel < level))
	{
		if (endPos == string::npos)
		{
			reducedHost = hostName.substr(pos + 1);
		}
		else
		{
			string levelled(hostName.substr(pos + 1, endPos - pos));
			levelled += reducedHost;
			reducedHost = levelled;
		}

		// Next
		pos = hostName.find_last_of(".", pos - 1);
		++currentLevel;
	}

	return reducedHost;
}

/// Escapes an URL.
string Url::escapeUrl(const string &url)
{
	string escapedUrl;

	if (url.empty() == true)
	{
		return "";
	}

	for (unsigned int pos = 0; pos < url.length(); ++pos)
	{
		// Encode this character ?
		if (g_rfc2396Encoded[url[pos]] == 1)
		{
			char currentChar = url[pos];
			char encodedStr[4];

			snprintf(encodedStr, 4, "%%%02X", (int)currentChar);
			escapedUrl += encodedStr;
		}
		else
		{
			escapedUrl += url[pos];
		}
	}

	return escapedUrl;
}

/// Unescapes an URL.
string Url::unescapeUrl(const string &escapedUrl)
{
	string unescapedUrl;
	unsigned int pos = 0;

	if (escapedUrl.empty() == true)
	{
		return "";
	}

	while (pos < escapedUrl.length())
	{
		if (escapedUrl[pos] == '%')
		{
			char numberStr[3];
			unsigned int number;

			numberStr[0] = escapedUrl[pos + 1];
			numberStr[1] = escapedUrl[pos + 2];
			numberStr[2] = '\0';
			if ((sscanf(numberStr, "%X", &number) == 1) ||
				(sscanf(numberStr, "%x", &number) == 1))
			{
				unescapedUrl += (char)(0x0ff & number);
				pos += 3;
			}
		}
		else
		{
			unescapedUrl += escapedUrl[pos];
			++pos;
		}
	}

	return unescapedUrl;
}

/// Resolves a path.
string Url::resolvePath(const string &currentDir, const string &location)
{
	string currentLocation(currentDir);
	string::size_type prevSlashPos = 0, slashPos = location.find('/');

	if (currentDir.empty() == true)
	{
		return "";
	}

	while (slashPos != string::npos)
	{
		string path(location.substr(prevSlashPos, slashPos - prevSlashPos));

		if (path == "..")
		{
			string upDir(Glib::path_get_dirname(currentLocation));
			currentLocation = upDir;
		}
		else if (path != ".")
		{
			currentLocation += "/";
			currentLocation += path;
		}
#ifdef DEBUG
		clog << "Url::resolvePath: partially resolved to " << currentLocation << endl;
#endif

		if (slashPos + 1 >= location.length())
		{
			// Nothing behind
			prevSlashPos = string::npos;
			break;
		}

		// Next
		prevSlashPos = slashPos + 1;
		slashPos = location.find('/', prevSlashPos);
	}

	// Remainder
	if (prevSlashPos != string::npos)
	{
		string path(location.substr(prevSlashPos));

		if (path == "..")
		{
			string upDir(Glib::path_get_dirname(currentLocation));
			currentLocation = upDir;
		}
		else if (path != ".")
		{
			currentLocation += "/";
			currentLocation += path;
		}
	}
#ifdef DEBUG
	clog << "Url::resolvePath: resolved to " << currentLocation << endl;
#endif

	return currentLocation;
}

string Url::getProtocol(void) const
{
	return m_protocol;
}

string Url::getUser(void) const
{
	return m_user;
}

string Url::getPassword(void) const
{
	return m_password;
}

string Url::getHost(void) const
{
	return m_host;
}

string Url::getLocation(void) const
{
	return m_location;
}

string Url::getFile(void) const
{
	return m_file;
}

string Url::getParameters(void) const
{
	return m_parameters;
}

bool Url::isLocal(void) const
{
	return isLocal(m_protocol);
}

