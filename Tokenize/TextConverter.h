/*
 *  Copyright 2008-2014 Fabrice Colin
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

#ifndef _TEXT_CONVERTER_H
#define _TEXT_CONVERTER_H

#include <string>

#include "Memory.h"
#include "Visibility.h"

class PINOT_EXPORT TextConverter
{
	public:
		TextConverter(unsigned int maxErrors = 10);
		virtual ~TextConverter();

		/// Converts to UTF-8.
		dstring toUTF8(const dstring &text,
			std::string &charset);

		/// Converts from UTF-8 the locale charset.
		std::string fromUTF8(const std::string &text);

		/// Converts from UTF-8.
		dstring fromUTF8(const dstring &text,
			const std::string &charset);

		/// Gets the number of conversion errors.
		unsigned int getErrorsCount(void) const;

	protected:
		std::string m_localeCharset;
		bool m_utf8Locale;
		unsigned int m_maxErrors;
		unsigned int m_conversionErrors;

		dstring convert(const dstring &text,
			std::string &fromCharset,
			const std::string &toCharset);

	private:
		TextConverter(const TextConverter &other);
		TextConverter& operator=(const TextConverter& other);

};

#endif // _TEXT_CONVERTER_H
