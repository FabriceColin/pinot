/*
 *  Copyright 2005-2012 Fabrice Colin
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

#include "config.h"
#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE
#include <unistd.h>
#undef _XOPEN_SOURCE
#else
#include <unistd.h>
#endif
#include <ctype.h>
#include <algorithm>

#include "StringManip.h"

using std::string;
using std::for_each;

static const unsigned int HASH_LEN = ((4 * 8 + 5) / 6);

// A function object to lower case strings with for_each()
struct ToLower
{
	public:
		void operator()(char &c)
		{
			c = (char)tolower((int)c);
		}
};

// A function object to upper case strings with for_each()
struct ToUpper
{
	public:
		void operator()(char &c)
		{
			c = (char)toupper((int)c);
		}
};

StringManip::StringManip()
{
}

/// Converts to lowercase.
string StringManip::toLowerCase(const string &str)
{
	string tmp(str);

	for_each(tmp.begin(), tmp.end(), ToLower());

	return tmp;
}

/// Converts to uppercase.
string StringManip::toUpperCase(const string &str)
{
	string tmp(str);

	for_each(tmp.begin(), tmp.end(), ToUpper());

	return tmp;
}

/// Extracts the sub-string between start and end.
string StringManip::extractField(const string &str, const string &start, const string &end, bool anyCharacterOfEnd)
{
	string::size_type endPos = 0;

	return extractField(str, start, end, endPos, anyCharacterOfEnd);
}

/// Extracts the sub-string between start and end.
string StringManip::extractField(const string &str, const string &start, const string &end, string::size_type &endPos, bool anyCharacterOfEnd)
{
	string fieldValue;
	string::size_type startPos = string::npos;

	if (start.empty() == true)
	{
		startPos = 0;
	}
	else
	{
		startPos = str.find(start, endPos);
	}

	if (startPos != string::npos)
	{
		startPos += start.length();

		if (end.empty() == true)
		{
			fieldValue = str.substr(startPos);
		}
		else
		{
			if (anyCharacterOfEnd == false)
			{
				endPos = str.find(end, startPos);
			}
			else
			{
				endPos = str.find_first_of(end, startPos);
			}
			if (endPos != string::npos)
			{
				fieldValue = str.substr(startPos, endPos - startPos);
			}
		}
	}

	return fieldValue;
}

/// Replaces a sub-string.
string StringManip::replaceSubString(const string &str, const std::string &substr, const std::string &rep)
{
	if (str.empty() == true)
	{
		return "";
	}

	string cleanStr(str);

	string::size_type startPos = cleanStr.find(substr);
	while (startPos != string::npos)
	{
		string::size_type endPos = startPos + substr.length();

		string tmp(cleanStr.substr(0, startPos));
		tmp += rep;
		tmp += cleanStr.substr(endPos);
		cleanStr = tmp;

		startPos += rep.length();
		if (startPos > cleanStr.length())
		{
			break;
		}

		startPos = cleanStr.find(substr, startPos);
	}

	return cleanStr;
}

/// Trims spaces at the start and end of a string.
unsigned int StringManip::trimSpaces(string &str)
{
	string::size_type pos = 0;
	unsigned int count = 0;

	while ((str.empty() == false) && (pos < str.length()))
	{
		if (isspace(str[pos]) == 0)
		{
			++pos;
			break;
		}

		str.erase(pos, 1);
		++count;
	}

	for (pos = str.length() - 1;
		(str.empty() == false) && (pos >= 0); --pos)
	{
		if (isspace(str[pos]) == 0)
		{
			break;
		}

		str.erase(pos, 1);
		++count;
	}

	return count;
}

/// Removes double and single quotes in links or any other attribute.
string StringManip::removeQuotes(const string &str)
{
	string unquotedText;

	if (str[0] == '"')
	{
		string::size_type closingQuotePos = str.find("\"", 1);
		if (closingQuotePos != string::npos)
		{
			unquotedText = str.substr(1, closingQuotePos - 1);
		}
	}
	else if (str[0] == '\'')
	{
		string::size_type closingQuotePos = str.find("'", 1);
		if (closingQuotePos != string::npos)
		{
			unquotedText = str.substr(1, closingQuotePos - 1);
		}
	}
	else
	{
		// There are no quotes, so just look for the first space, if any
		string::size_type spacePos = str.find(" ");
		if (spacePos != string::npos)
		{
			unquotedText = str.substr(0, spacePos);
		}
		else
		{
			unquotedText = str;
		}
	}

	return unquotedText;
}

/// Hashes a string.
string StringManip::hashString(const string &str)
{
	unsigned long int h = 1;

	if (str.empty() == true)
	{
		return "";
	}

	// The following was lifted from Xapian's xapian-applications/omega/hashterm.cc
	// and prettified slightly
	for (string::const_iterator i = str.begin(); i != str.end(); ++i)
	{
		h += (h << 5) + static_cast<unsigned char>(*i);
	}
	// In case sizeof(unsigned long) > 4
	h &= 0xffffffff;

	string hashedString(HASH_LEN, ' ');
	int i = 0;
	while (h != 0)
	{
		char ch = char((h & 63) + 33);
		hashedString[i++] = ch;
		h = h >> 6;
	}

	return hashedString;
}

/// Hashes a string so that it is no longer than maxLength.
string StringManip::hashString(const string &str, unsigned int maxLength)
{
	if (str.length() <= maxLength)
	{
		return str;
	}

	string result(str);
	maxLength -= HASH_LEN;
	result.replace(maxLength, string::npos,
		hashString(result.substr(maxLength)));

	return result;
}

