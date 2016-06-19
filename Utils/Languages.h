/*
 *  Copyright 2005-2008 Fabrice Colin
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

#ifndef _LANGUAGES_H
#define _LANGUAGES_H

#include <string>
#include <map>

#include "Visibility.h"

/// Utility class for manipulating languages.
class PINOT_EXPORT Languages
{
	public:
		virtual ~Languages();

		/// Sets the name for the given code.
		static bool setIntlName(unsigned int num, const std::string &name);

		/// Gets the name of the language at the given position.
		static std::string getIntlName(unsigned int num);

		/// Returns a language name, in English.
		static std::string toEnglish(const std::string &language);

		/// Returns a language name, in the current locale.
		static std::string toLocale(const std::string &language);

		/// Returns a language code.
		static std::string toCode(const std::string &language);

		static const unsigned int m_count;
		static const char *m_names[];

	protected:
		static const char *m_codes[];
		static std::map<unsigned int, std::string> m_intlNames;

		Languages();

	private:
		Languages(const Languages &other);
		Languages &operator=(const Languages &other);

};

#endif // _LANGUAGES_H
