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

#include <errno.h>
#include <iostream>
#include <glibmm/convert.h>
#include <glibmm/ustring.h>

#include "StringManip.h"
#include "TextConverter.h"

using std::clog;
using std::endl;
using std::string;
using namespace Glib;

TextConverter::TextConverter(unsigned int maxErrors) :
	m_utf8Locale(false),
	m_maxErrors(maxErrors),
	m_conversionErrors(0)
{
	// Get the locale charset
	m_utf8Locale = get_charset(m_localeCharset);
}

TextConverter::~TextConverter()
{
}

dstring TextConverter::convert(const dstring &text,
	string &fromCharset, const string &toCharset)
{
	dstring outputText;
	char outputBuffer[8192];
	char *pInput = const_cast<char *>(text.c_str());
	gsize inputSize = (gsize)text.length();
	bool invalidSequence = false;

	outputText.clear();
	try
	{
		IConv converter(toCharset, fromCharset);
 
		while (inputSize > 0)
		{
			char *pOutput = outputBuffer;
			gsize outputSize = 8192;

			size_t conversions = converter.iconv(&pInput, &inputSize, &pOutput, &outputSize);
			int errorCode = errno;
			if (conversions == static_cast<size_t>(-1))
			{
				if (errorCode == EILSEQ)
				{
					// Conversion was only partially successful
					++m_conversionErrors;
#ifdef DEBUG
					clog << "TextConverter::convert: invalid sequence" << endl;
#endif
					if (m_conversionErrors >= m_maxErrors)
					{
						// Give up
						return text;
					}
					converter.reset();

					outputText.append(outputBuffer, 8192 - outputSize);
					if (invalidSequence == false)
					{
						outputText += "?";
						invalidSequence = true;
					}

					// Skip that
					++pInput;
					--inputSize;
					continue;
				}
				else if (errorCode != E2BIG)
				{
#ifdef DEBUG
					clog << "TextConverter::convert: unknown error " << errorCode << endl;
#endif
					return text;
				}
			}
			else
			{
				invalidSequence = false;
			}

			// Append what was successfully converted
			outputText.append(outputBuffer, 8192 - outputSize);
		}

#ifdef DEBUG
		clog << "TextConverter::convert: " << m_conversionErrors << " conversion errors" << endl;
#endif
	}
	catch (Error &ce)
	{
#ifdef DEBUG
		clog << "TextConverter::convert: " << ce.what() << endl;
#endif
		outputText.clear();

		string::size_type pos = fromCharset.find('_');
		if (pos != string::npos)
		{
			string fixedCharset(StringManip::replaceSubString(fromCharset, "_", "-"));

#ifdef DEBUG
			clog << "TextConverter::convert: trying with charset " << fixedCharset << endl;
#endif
			fromCharset = fixedCharset;
			outputText = convert(text, fromCharset, toCharset);
		}
	}
	catch (...)
	{
#ifdef DEBUG
		clog << "TextConverter::convert: unknown exception" << endl;
#endif
		outputText.clear();
	}

	return outputText;
}

dstring TextConverter::toUTF8(const dstring &text,
	string &charset)
{
	string textCharset(StringManip::toLowerCase(charset));

	m_conversionErrors = 0;

	if ((text.empty() == true) ||
		(textCharset == "utf-8"))
	{
		// No conversion necessary
		return text;
	}

	if (textCharset.empty() == true)
	{
		if (m_utf8Locale == true)
		{
			// The current locale uses UTF-8
			return text;
		}

		textCharset = m_localeCharset;
	}

	return convert(text, textCharset, "UTF-8");
}

dstring TextConverter::fromUTF8(const dstring &text,
	const string &charset)
{
	string fromCharset("UTF-8");

	return convert(text, fromCharset, charset);
}

string TextConverter::fromUTF8(const string &text)
{
	try
	{
		return locale_from_utf8(text);
	}
	catch (Error &ce)
	{
#ifdef DEBUG
		clog << "TextConverter::fromUTF8: " << ce.what() << endl;
#endif
	}
	catch (...)
	{
#ifdef DEBUG
		clog << "TextConverter::fromUTF8: unknown exception" << endl;
#endif
	}
 
       return "";
}

unsigned int TextConverter::getErrorsCount(void) const
{
	return m_conversionErrors;
}

