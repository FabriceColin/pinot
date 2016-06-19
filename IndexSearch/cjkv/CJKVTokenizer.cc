/*
 *  Copyright 2007-2008 林永忠 Yung-Chung Lin
 *  Copyright 2008-2013 Fabrice Colin
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <ctype.h>
#include <string.h>
#include <string.h>
#include <iostream>

#include "CJKVTokenizer.h"

static char *unicode_get_utf8(const char *p, gunichar *result)
{
	*result = g_utf8_get_char(p);

	return (*result == (gunichar)-1) ? NULL : g_utf8_next_char(p);
}

// 2E80..2EFF; CJK Radicals Supplement
// 3000..303F; CJK Symbols and Punctuation
// 3040..309F; Hiragana
// 30A0..30FF; Katakana
// 3100..312F; Bopomofo
// 3130..318F; Hangul Compatibility Jamo
// 3190..319F; Kanbun
// 31A0..31BF; Bopomofo Extended
// 31C0..31EF; CJK Strokes
// 31F0..31FF; Katakana Phonetic Extensions
// 3200..32FF; Enclosed CJK Letters and Months
// 3300..33FF; CJK Compatibility
// 3400..4DBF; CJK Unified Ideographs Extension A
// 4DC0..4DFF; Yijing Hexagram Symbols
// 4E00..9FFF; CJK Unified Ideographs
// A700..A71F; Modifier Tone Letters
// AC00..D7AF; Hangul Syllables
// F900..FAFF; CJK Compatibility Ideographs
// FE30..FE4F; CJK Compatibility Forms
// FF00..FFEF; Halfwidth and Fullwidth Forms
// 20000..2A6DF; CJK Unified Ideographs Extension B
// 2F800..2FA1F; CJK Compatibility Ideographs Supplement
#define UTF8_IS_CJKV(p)                                                 \
    (((p) >= 0x2E80 && (p) <= 0x2EFF)                                   \
     || ((p) >= 0x3000 && (p) <= 0x303F)                                \
     || ((p) >= 0x3040 && (p) <= 0x309F)                                \
     || ((p) >= 0x30A0 && (p) <= 0x30FF)                                \
     || ((p) >= 0x3100 && (p) <= 0x312F)                                \
     || ((p) >= 0x3130 && (p) <= 0x318F)                                \
     || ((p) >= 0x3190 && (p) <= 0x319F)                                \
     || ((p) >= 0x31A0 && (p) <= 0x31BF)                                \
     || ((p) >= 0x31C0 && (p) <= 0x31EF)                                \
     || ((p) >= 0x31F0 && (p) <= 0x31FF)                                \
     || ((p) >= 0x3200 && (p) <= 0x32FF)                                \
     || ((p) >= 0x3300 && (p) <= 0x33FF)                                \
     || ((p) >= 0x3400 && (p) <= 0x4DBF)                                \
     || ((p) >= 0x4DC0 && (p) <= 0x4DFF)                                \
     || ((p) >= 0x4E00 && (p) <= 0x9FFF)                                \
     || ((p) >= 0xA700 && (p) <= 0xA71F)                                \
     || ((p) >= 0xAC00 && (p) <= 0xD7AF)                                \
     || ((p) >= 0xF900 && (p) <= 0xFAFF)                                \
     || ((p) >= 0xFE30 && (p) <= 0xFE4F)                                \
     || ((p) >= 0xFF00 && (p) <= 0xFFEF)                                \
     || ((p) >= 0x20000 && (p) <= 0x2A6DF)                              \
     || ((p) >= 0x2F800 && (p) <= 0x2FA1F)                              \
     || ((p) >= 0x2F800 && (p) <= 0x2FA1F))
// Combining Marks
// 0300..036F; Basic range
// 1DC0..1DFF; Supplements
// 20D0..20FF; Symbols
// FE20..FE2F; Half marks
#define UTF8_IS_CM(p)                                                   \
    (((p) >= 0x0300 && (p) <= 0x036F)                                   \
     || ((p) >= 0x1DC0 && (p) <= 0x1DFF)                                \
     || ((p) >= 0x20D0 && (p) <= 0x20FF)                                \
     || ((p) >= 0xFE20 && (p) <= 0xFE2F))

using namespace std;
using namespace Dijon;

static void _split_string(string str, const string &delim,
	vector<string> &list)
{
	list.clear();

	string::size_type cut_at = 0;
	while ((cut_at = str.find_first_of(delim)) != str.npos)
	{
		if (cut_at > 0)
		{
			list.push_back(str.substr(0,cut_at));
		}
		str = str.substr(cut_at+1);
	}

	if (str.empty() == false)
	{
		list.push_back(str);
	}
}

static inline unsigned char *_unicode_to_char(gunichar &uchar,
	unsigned char *p) 
{
	if (p == NULL)
	{
		return NULL;
	}

	memset(p, 0, sizeof(gunichar) + 1);
	if (g_unichar_isspace(uchar) ||
		(g_unichar_ispunct(uchar) && (uchar != '.')))
	{
		p[0] = ' ';
	}
	else if (uchar < 0x80)
	{
		p[0] = uchar;
	}
	else if (uchar < 0x800)
	{
		p[0] = (0xC0 | uchar >> 6);
		p[1] = (0x80 | uchar & 0x3F);
	}
	else if (uchar < 0x10000)
	{
		p[0] = (0xE0 | uchar >> 12);
		p[1] = (0x80 | uchar >> 6 & 0x3F);
		p[2] = (0x80 | uchar & 0x3F);
	}
	else if (uchar < 0x200000)
	{
		p[0] = (0xF0 | uchar >> 18);
		p[1] = (0x80 | uchar >> 12 & 0x3F);
		p[2] = (0x80 | uchar >> 6 & 0x3F);
		p[3] = (0x80 | uchar & 0x3F);
	}

	return p;
}

class VectorTokensHandler : public CJKVTokenizer::TokensHandler
{
	public:
		VectorTokensHandler(vector<string> &token_list) :
			CJKVTokenizer::TokensHandler(),
			m_token_list(token_list)
		{
		}

		virtual ~VectorTokensHandler()
		{
		}

		virtual bool handle_token(const string &tok, bool is_cjkv)
		{
			m_token_list.push_back(tok);
			return true;
		}

	protected:
		vector<string> &m_token_list;

};

CJKVTokenizer::CJKVTokenizer() :
	m_nGramSize(2),
	m_maxTokenCount(0),
	m_maxTextSize(5242880)
{
}

CJKVTokenizer::~CJKVTokenizer()
{
}

string CJKVTokenizer::normalize(const string &str,
	bool normalizeAll)
{
	// Normalize the string
	gchar *normalized = g_utf8_normalize(str.c_str(), str.length(),
		(normalizeAll == true ? G_NORMALIZE_ALL : G_NORMALIZE_DEFAULT_COMPOSE));
	if (normalized == NULL)
	{
		return "";
	}

	string normalized_str(normalized, strlen(normalized));

	g_free(normalized);

	return normalized_str;
}

string CJKVTokenizer::strip_marks(const string &str)
{
	if (str.empty() == true)
	{
		return "";
	}

	gchar *stripped = g_strdup(normalize(str, true).c_str());
	gsize input_pos = 0, output_pos = 0;

	if (stripped == NULL)
	{
		return "";
	}

	while (input_pos < strlen(stripped))
	{
		gunichar unichar = g_utf8_get_char_validated(&stripped[input_pos], -1);

		if ((unichar == (gunichar)-1) ||
			(unichar == (gunichar)-2))
		{
			break;
		}

		gchar *next_utf8 = g_utf8_next_char(&stripped[input_pos]);
		gint utf8_len = next_utf8 - &stripped[input_pos];

		// Is this a Combining Mark ?
		if (!UTF8_IS_CM((guint32)unichar))
		{
			// No, it's not
			if (input_pos != output_pos)
			{
				memmove(&stripped[output_pos], &stripped[input_pos], utf8_len);
			}

			output_pos += utf8_len;
		}
		input_pos += utf8_len;
	}
	stripped[output_pos] = '\0';

	string stripped_str(stripped, output_pos);

	g_free(stripped);

	return stripped_str;
}

void CJKVTokenizer::set_ngram_size(unsigned int ngram_size)
{
	m_nGramSize = ngram_size;
}

unsigned int CJKVTokenizer::get_ngram_size(void) const
{
	return m_nGramSize;
}

void CJKVTokenizer::set_max_token_count(unsigned int max_token_count)
{
	m_maxTokenCount = max_token_count;
}

unsigned int CJKVTokenizer::get_max_token_count(void) const
{
	return m_maxTokenCount;
}

void CJKVTokenizer::set_max_text_size(unsigned int max_text_size)
{
	m_maxTextSize = max_text_size;
}

unsigned int CJKVTokenizer::get_max_text_size(void) const
{
	return m_maxTextSize;
}

void CJKVTokenizer::tokenize(const string &str, vector<string> &token_list,
	bool break_ascii_only_on_space)
{
	VectorTokensHandler handler(token_list);

	tokenize(str, handler, break_ascii_only_on_space);
}

void CJKVTokenizer::tokenize(const string &str, TokensHandler &handler,
	bool break_ascii_only_on_space)
{
	string token_str;
	vector<string> temp_token_list;
	vector<gunichar> temp_uchar_list;
	unsigned int tokens_count = 0;

	split(str, temp_token_list, temp_uchar_list);

	for (unsigned int i = 0; i < temp_token_list.size();)
	{
		if ((m_maxTokenCount > 0) &&
			(tokens_count >= m_maxTokenCount))
		{
			break;
		}
		token_str.resize(0);
		if (UTF8_IS_CJKV(temp_uchar_list[i]))
		{
			for (unsigned int j = i; j < i + m_nGramSize; j++)
			{
				if ((m_maxTokenCount > 0) &&
					(tokens_count >= m_maxTokenCount))
				{
					break;
				}
				if (j == temp_token_list.size())
				{
					break;
				}
				if (UTF8_IS_CJKV(temp_uchar_list[j]))
				{
					string token(temp_token_list[j]);

					if ((token.length() == 1) &&
						(isspace(token[0]) != 0))
					{
						break;
					}
					token_str += token;
					if (handler.handle_token(normalize(token_str), true) == true)
					{
						++tokens_count;
					}
				}
			}
			i++;
		}
		else
		{
			unsigned int j = i;

			while (j < temp_token_list.size())
			{
				unsigned char *p = (unsigned char*) temp_token_list[j].c_str();
				bool break_ascii = false;

				if (isascii((int)p[0]) != 0)
				{
					if (break_ascii_only_on_space == true)
					{
						if (isspace((int)p[0]) != 0)
						{
							break_ascii = true;
						}
					}
					else if (isalnum((int)p[0]) == 0)
					{
						break_ascii = true;
					}
				}

				if (break_ascii == true)
				{
					j++;
					break;
				}
				else if (UTF8_IS_CJKV(temp_uchar_list[j]))
				{
					break;
				}

				token_str += temp_token_list[j];
				j++;
			}
			i = j;
			if ((m_maxTokenCount > 0) &&
				(tokens_count >= m_maxTokenCount))
			{
				break;
			}
			if (token_str.empty() == false)
			{
				if (handler.handle_token(normalize(token_str), false) == true)
				{
					++tokens_count;
				}
			}
		}
	}
}

void CJKVTokenizer::split(const string &str,
	vector<string> &string_list,
	vector<gunichar> &unicode_list)
{
	gunichar uchar;
	const char *str_ptr = str.c_str();
	glong str_utf8_len = g_utf8_strlen(str_ptr, str.length());
	unsigned char p[sizeof(gunichar) + 1];

	for (glong i = 0; i < str_utf8_len; i++)
	{
		str_ptr = unicode_get_utf8(str_ptr, &uchar);
		if (str_ptr == NULL)
		{
			break;
		}

		if (i >= m_maxTextSize)
		{
			break;
		}

		string_list.push_back((const char*)_unicode_to_char(uchar, p));
		unicode_list.push_back(uchar);
	}
}

void CJKVTokenizer::segment(const string &str, vector<string> &token_segment)
{
	vector<string> token_list;
	string onlySpacesStr(str);

	for (string::iterator it = onlySpacesStr.begin(); it != onlySpacesStr.end(); ++it)
	{
		if (isspace((int)*it) != 0)
		{
			*it = ' ';
		}
	}

	_split_string(onlySpacesStr, " ", token_segment);
}

bool CJKVTokenizer::has_cjkv(const string &str)
{
	vector<string> temp_token_list;
	vector<gunichar> temp_uchar_list;

	split(str, temp_token_list, temp_uchar_list);

	for (unsigned int i = 0; i < temp_uchar_list.size(); i++)
	{
		if (UTF8_IS_CJKV(temp_uchar_list[i]))
		{
			return true;
		}
	}
	return false;
}

bool CJKVTokenizer::has_cjkv_only(const string &str)
{
	vector<string> temp_token_list;
	vector<gunichar> temp_uchar_list;

	split(str, temp_token_list, temp_uchar_list);

	for (unsigned int i = 0; i < temp_uchar_list.size(); i++)
	{
		if (!(UTF8_IS_CJKV(temp_uchar_list[i])))
		{
			unsigned char p[sizeof(gunichar) + 1];

			_unicode_to_char(temp_uchar_list[i], p);
			if (isspace((int)p[0]) == 0)
			{
				return false;
			}
		}
	}
	return true;
}

