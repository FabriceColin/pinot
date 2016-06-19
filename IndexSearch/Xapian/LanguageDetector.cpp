/*
 *  Copyright 2005-2011 Fabrice Colin
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
#include <strings.h>
#include <sys/time.h>
extern "C"
{
#define USE_TEXTCAT 1
#ifdef HAVE_LIBEXTTEXTCAT_TEXTCAT_H
#include <libexttextcat/textcat.h>
#else
#ifdef HAVE_LIBTEXTCAT_TEXTCAT_H
#include <libtextcat/textcat.h>
#else
#ifdef HAVE_TEXTCAT_H
#include <textcat.h>
#else
#undef USE_TEXTCAT
#endif
#endif
#endif
}
#include <iostream>
#include <utility>
#include <cstring>

#include "StringManip.h"
#include "Timer.h"
#include "LanguageDetector.h"
#include "config.h"

using std::clog;
using std::clog;
using std::endl;
using std::string;
using std::vector;
using std::min;

#define MAX_TEXT_SIZE 1000

LanguageDetector LanguageDetector::m_instance;

LanguageDetector::LanguageDetector() :
	m_pHandle(NULL)
{
#ifdef USE_TEXTCAT
	string confFile(SYSCONFDIR);
	const char *textCatVersion = textcat_Version();

	// What configuration file should we use ?
	confFile += "/pinot/";
#ifdef DEBUG
	clog << "LanguageDetector::guessLanguage: detected " << textCatVersion << endl;
#endif
	if (strncasecmp(textCatVersion, "TextCat 3", 9) == 0)
	{
		// Version 3
		confFile += "textcat3_conf.txt";
	}
	else if (strncasecmp(textCatVersion, "3.1", 3) == 0)
	{
		// Version 3.1
		confFile += "textcat31_conf.txt";
	}
	else if (strncasecmp(textCatVersion, "3.", 2) == 0)
	{
		// Version 3.2 and above
		confFile += "textcat32_conf.txt";
	}
	else
	{
		confFile += "textcat_conf.txt";
	}

	// Initialize
	pthread_mutex_init(&m_mutex, NULL);
	m_pHandle = textcat_Init(confFile.c_str());
#endif
}

LanguageDetector::~LanguageDetector()
{
#ifdef USE_TEXTCAT
	if (m_pHandle != NULL)
	{
		// Close the descriptor
		textcat_Done(m_pHandle);
	}
	pthread_mutex_destroy(&m_mutex);
#endif
}

LanguageDetector &LanguageDetector::getInstance(void)
{
	return m_instance;
}

/**
  * Attempts to guess the language.
  * Returns a list of candidates, or "unknown" if detection failed.
  */
void LanguageDetector::guessLanguage(const char *pData, unsigned int dataLength,
	vector<string> &candidates)
{
#ifdef HAVE_TEXTCAT_CAT
	const char *catResults[10];
#endif

	candidates.clear();
#ifdef USE_TEXTCAT
	if (m_pHandle == NULL)
	{
		candidates.push_back("unknown");
		return;
	}

#ifdef DEBUG
	Timer timer;
	timer.start();
#endif
	// Lock the handle
	if (pthread_mutex_lock(&m_mutex) != 0)
	{
		return;
	}

	// Classify
#ifdef HAVE_TEXTCAT_CAT
	unsigned int resultNum = textcat_Cat(m_pHandle, pData,
		min(dataLength, (unsigned int)MAX_TEXT_SIZE), catResults, 10);
	if (resultNum == 0 )
	{
		candidates.push_back("unknown");
	}
	else
	{
		for (unsigned int i=0; i<resultNum; ++i)
		{
			string language(StringManip::toLowerCase(catResults[i]));

			// Remove the charset information
			string::size_type dashPos = language.find('-');
			if (dashPos != string::npos)
			{
				language.resize(dashPos);
			}
#ifdef DEBUG
			clog << "LanguageDetector::guessLanguage: found language " << language << endl;
#endif
			candidates.push_back(language);
		}
	}
#else
	const char *languages = textcat_Classify(m_pHandle, pData,
		min(dataLength, (unsigned int)MAX_TEXT_SIZE));
	if (languages == NULL)
	{
		candidates.push_back("unknown");
	}
	else
	{
		// The output will be either SHORT, or UNKNOWN or a list of languages in []
		if ((strncasecmp(languages, "SHORT", 5) == 0) ||
			(strncasecmp(languages, "UNKNOWN", 7) == 0))
		{
			candidates.push_back("unknown");
		}
		else
		{
			string languageList(languages);
			string::size_type lastPos = 0, pos = languageList.find_first_of("[");

			while (pos != string::npos)
			{
				++pos;
				lastPos = languageList.find_first_of("]", pos);
				if (lastPos == string::npos)
				{
					break;
				}

				string language(StringManip::toLowerCase(languageList.substr(pos, lastPos - pos)));
				// Remove the charset information
				string::size_type dashPos = language.find('-');
				if (dashPos != string::npos)
				{
					language.resize(dashPos);
				}
#ifdef DEBUG
				clog << "LanguageDetector::guessLanguage: found language " << language << endl;
#endif
				candidates.push_back(language);

				// Next
				pos = languageList.find_first_of("[", lastPos);
			}
		}
	}
#endif

	// Unlock
	pthread_mutex_unlock(&m_mutex);
#ifdef DEBUG
	clog << "LanguageDetector::guessLanguage: language guessing took "
		<< timer.stop() << " ms" << endl;
#endif
#else
	candidates.push_back("unknown");
#endif
}
