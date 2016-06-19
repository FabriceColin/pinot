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
 
#include <string.h>
#include <utility>

#include "Languages.h"

using std::string;
using std::map;
using std::pair;
using std::min;

const unsigned int Languages::m_count = 16;

const char *Languages::m_names[] = { "Unknown", "Danish", "Dutch", "English",
	"Finnish", "French", "German", "Hungarian", "Italian", "Norwegian",
	"Portuguese", "Romanian", "Russian", "Spanish", "Swedish", "Turkish" };

const char *Languages::m_codes[] = { "", "da", "nl", "en", "fi", "fr", "de", "hu", \
	"it", "nn", "pt", "ro", "ru", "es", "sv", "tr" };

map<unsigned int, string> Languages::m_intlNames;

Languages::Languages()
{
}

Languages::~Languages()
{
}

bool Languages::setIntlName(unsigned int num, const string &name)
{
		pair<map<unsigned int, string>::iterator, bool> insertPair = m_intlNames.insert(pair<unsigned int, string>(num, name));
		// Was it inserted ?
		return insertPair.second;
}

string Languages::getIntlName(unsigned int num)
{
	map<unsigned int, string>::iterator iter = m_intlNames.find(num);
	if (iter == m_intlNames.end())
	{
		return "";
	}

	return iter->second;
}

string Languages::toEnglish(const string &language)
{
	if (language.empty() == false)
	{
		for (unsigned int langNum = 0; langNum < Languages::m_count; ++langNum)
		{
			string intlLanguage = Languages::getIntlName(langNum);

			if (strncasecmp(language.c_str(), intlLanguage.c_str(),
				min(language.length(), intlLanguage.length())) == 0)
			{
				return Languages::m_names[langNum];
			}
		}
	}

	return language;
}

string Languages::toLocale(const string &language)
{
	if (language.empty() == false)
	{
		// Get the language name in the current locale
		for (unsigned int langNum = 0; langNum < Languages::m_count; ++langNum)
		{
			if (strncasecmp(language.c_str(), Languages::m_names[langNum],
				min(language.length(), strlen(Languages::m_names[langNum]))) == 0)
			{
				// That's the one !
				return Languages::getIntlName(langNum);
			}
		}
	}

	return language;
}

string Languages::toCode(const string &language)
{
	if (language.empty() == false)
	{
		// Get the language code
		for (unsigned int langNum = 0; langNum < Languages::m_count; ++langNum)
		{
			if (strncasecmp(language.c_str(), Languages::m_names[langNum],
				min(language.length(), strlen(Languages::m_names[langNum]))) == 0)
			{
				// That's the one !
				return Languages::m_codes[langNum];
			}
		}
	}

	return language;
}
