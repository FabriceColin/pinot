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

#ifndef _DIJON_CJKVTOKENIZER_H
#define _DIJON_CJKVTOKENIZER_H

#include <string>
#include <vector>
#include <glib.h>

#ifndef DIJON_CJKV_EXPORT
#if defined __GNUC__ && (__GNUC__ >= 4)
  #define DIJON_CJKV_EXPORT __attribute__ ((visibility("default")))
#else
  #define DIJON_CJKV_EXPORT
#endif
#endif

namespace Dijon
{
	class DIJON_CJKV_EXPORT CJKVTokenizer
	{
		public:
			CJKVTokenizer();
			~CJKVTokenizer();

			static std::string normalize(const std::string &str,
				bool normalizeAll = false);

			static std::string strip_marks(const std::string &str);

			class TokensHandler
			{
				public:
					TokensHandler() {}
					virtual ~TokensHandler() {}

					virtual bool handle_token(const std::string &tok, bool is_cjkv) = 0;
			};

			void set_ngram_size(unsigned int ngram_size);

			unsigned int get_ngram_size(void) const;

			void set_max_token_count(unsigned int max_token_count);

			unsigned int get_max_token_count(void) const;

			void set_max_text_size(unsigned int max_text_size);

			unsigned int get_max_text_size(void) const;

			void tokenize(const std::string &str,
				std::vector<std::string> &token_list,
				bool break_ascii_only_on_space = false);

			void tokenize(const std::string &str,
				TokensHandler &handler,
				bool break_ascii_only_on_space = false);

			void split(const std::string &str,
				std::vector<std::string> &string_list,
				std::vector<gunichar> &unicode_list);

			void segment(const std::string &str,
				std::vector<std::string> &token_segment);

			bool has_cjkv(const std::string &str);

			bool has_cjkv_only(const std::string &str);

		protected:
			unsigned int m_nGramSize;
			unsigned int m_maxTokenCount;
			unsigned int m_maxTextSize;

	};
};

#endif // _DIJON_CJKVTOKENIZER_H
