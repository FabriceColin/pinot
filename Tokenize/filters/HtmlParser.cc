/* htmlparse.cc: simple HTML parser for omega indexer
 *
 * Copyright 1999,2000,2001 BrightStation PLC
 * Copyright 2001 Ananova Ltd
 * Copyright 2002,2006,2007,2008 Olly Betts
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301
 * USA
 */

#include <ctype.h>
#include <cstring>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <algorithm>

#include "config.h"
#include "HtmlParser.h"

using namespace std;

inline void
lowercase_string(string &str)
{
    for (string::iterator i = str.begin(); i != str.end(); ++i) {
	*i = tolower(static_cast<unsigned char>(*i));
    }
}

map<string, unsigned int> HtmlParser::named_ents;

inline static bool
p_notdigit(char c)
{
    return !isdigit(static_cast<unsigned char>(c));
}

inline static bool
p_notxdigit(char c)
{
    return !isxdigit(static_cast<unsigned char>(c));
}

inline static bool
p_notalnum(char c)
{
    return !isalnum(static_cast<unsigned char>(c));
}

inline static bool
p_notwhitespace(char c)
{
    return !isspace(static_cast<unsigned char>(c));
}

inline static bool
p_nottag(char c)
{
    return !isalnum(static_cast<unsigned char>(c)) &&
	c != '.' && c != '-' && c != ':'; // ':' for XML namespaces.
}

inline static bool
p_whitespacegt(char c)
{
    return isspace(static_cast<unsigned char>(c)) || c == '>';
}

inline static bool
p_whitespaceeqgt(char c)
{
    return isspace(static_cast<unsigned char>(c)) || c == '=' || c == '>';
}

static unsigned
nonascii_to_utf8(unsigned ch, char * buf)
{
    // FIXME: use CJKVTokenizer's _unicode_to_char()
    if (ch < 0x800) {
	buf[0] = 0xc0 | (ch >> 6);
	buf[1] = 0x80 | (ch & 0x3f);
	return 2;
    }
    if (ch < 0x10000) {
	buf[0] = 0xe0 | (ch >> 12);
	buf[1] = 0x80 | ((ch >> 6) & 0x3f);
	buf[2] = 0x80 | (ch & 0x3f);
	return 3;
    }
    if (ch < 0x200000) {
	buf[0] = 0xf0 | (ch >> 18);
	buf[1] = 0x80 | ((ch >> 12) & 0x3f);
	buf[2] = 0x80 | ((ch >> 6) & 0x3f);
	buf[3] = 0x80 | (ch & 0x3f);
	return 4;
    }

    return 0;
}

bool
HtmlParser::get_parameter(const string & param, string & value)
{
    map<string, string>::const_iterator i = parameters.find(param);
    if (i == parameters.end()) return false;
    value = i->second;
    return true;
}

HtmlParser::HtmlParser()
{
    static const struct ent { const char *n; unsigned int v; } ents[] = {
// Names and values from: "Character entity references in HTML 4"
// http://www.w3.org/TR/html4/sgml/entities.html
{ "quot", 34 },
{ "amp", 38 },
{ "apos", 39 }, // Not in HTML 4 list but used in OpenOffice XML.
{ "lt", 60 },
{ "gt", 62 },
{ "nbsp", 160 },
{ "iexcl", 161 },
{ "cent", 162 },
{ "pound", 163 },
{ "curren", 164 },
{ "yen", 165 },
{ "brvbar", 166 },
{ "sect", 167 },
{ "uml", 168 },
{ "copy", 169 },
{ "ordf", 170 },
{ "laquo", 171 },
{ "not", 172 },
{ "shy", 173 },
{ "reg", 174 },
{ "macr", 175 },
{ "deg", 176 },
{ "plusmn", 177 },
{ "sup2", 178 },
{ "sup3", 179 },
{ "acute", 180 },
{ "micro", 181 },
{ "para", 182 },
{ "middot", 183 },
{ "cedil", 184 },
{ "sup1", 185 },
{ "ordm", 186 },
{ "raquo", 187 },
{ "frac14", 188 },
{ "frac12", 189 },
{ "frac34", 190 },
{ "iquest", 191 },
{ "Agrave", 192 },
{ "Aacute", 193 },
{ "Acirc", 194 },
{ "Atilde", 195 },
{ "Auml", 196 },
{ "Aring", 197 },
{ "AElig", 198 },
{ "Ccedil", 199 },
{ "Egrave", 200 },
{ "Eacute", 201 },
{ "Ecirc", 202 },
{ "Euml", 203 },
{ "Igrave", 204 },
{ "Iacute", 205 },
{ "Icirc", 206 },
{ "Iuml", 207 },
{ "ETH", 208 },
{ "Ntilde", 209 },
{ "Ograve", 210 },
{ "Oacute", 211 },
{ "Ocirc", 212 },
{ "Otilde", 213 },
{ "Ouml", 214 },
{ "times", 215 },
{ "Oslash", 216 },
{ "Ugrave", 217 },
{ "Uacute", 218 },
{ "Ucirc", 219 },
{ "Uuml", 220 },
{ "Yacute", 221 },
{ "THORN", 222 },
{ "szlig", 223 },
{ "agrave", 224 },
{ "aacute", 225 },
{ "acirc", 226 },
{ "atilde", 227 },
{ "auml", 228 },
{ "aring", 229 },
{ "aelig", 230 },
{ "ccedil", 231 },
{ "egrave", 232 },
{ "eacute", 233 },
{ "ecirc", 234 },
{ "euml", 235 },
{ "igrave", 236 },
{ "iacute", 237 },
{ "icirc", 238 },
{ "iuml", 239 },
{ "eth", 240 },
{ "ntilde", 241 },
{ "ograve", 242 },
{ "oacute", 243 },
{ "ocirc", 244 },
{ "otilde", 245 },
{ "ouml", 246 },
{ "divide", 247 },
{ "oslash", 248 },
{ "ugrave", 249 },
{ "uacute", 250 },
{ "ucirc", 251 },
{ "uuml", 252 },
{ "yacute", 253 },
{ "thorn", 254 },
{ "yuml", 255 },
{ "OElig", 338 },
{ "oelig", 339 },
{ "Scaron", 352 },
{ "scaron", 353 },
{ "Yuml", 376 },
{ "fnof", 402 },
{ "circ", 710 },
{ "tilde", 732 },
{ "Alpha", 913 },
{ "Beta", 914 },
{ "Gamma", 915 },
{ "Delta", 916 },
{ "Epsilon", 917 },
{ "Zeta", 918 },
{ "Eta", 919 },
{ "Theta", 920 },
{ "Iota", 921 },
{ "Kappa", 922 },
{ "Lambda", 923 },
{ "Mu", 924 },
{ "Nu", 925 },
{ "Xi", 926 },
{ "Omicron", 927 },
{ "Pi", 928 },
{ "Rho", 929 },
{ "Sigma", 931 },
{ "Tau", 932 },
{ "Upsilon", 933 },
{ "Phi", 934 },
{ "Chi", 935 },
{ "Psi", 936 },
{ "Omega", 937 },
{ "alpha", 945 },
{ "beta", 946 },
{ "gamma", 947 },
{ "delta", 948 },
{ "epsilon", 949 },
{ "zeta", 950 },
{ "eta", 951 },
{ "theta", 952 },
{ "iota", 953 },
{ "kappa", 954 },
{ "lambda", 955 },
{ "mu", 956 },
{ "nu", 957 },
{ "xi", 958 },
{ "omicron", 959 },
{ "pi", 960 },
{ "rho", 961 },
{ "sigmaf", 962 },
{ "sigma", 963 },
{ "tau", 964 },
{ "upsilon", 965 },
{ "phi", 966 },
{ "chi", 967 },
{ "psi", 968 },
{ "omega", 969 },
{ "thetasym", 977 },
{ "upsih", 978 },
{ "piv", 982 },
{ "ensp", 8194 },
{ "emsp", 8195 },
{ "thinsp", 8201 },
{ "zwnj", 8204 },
{ "zwj", 8205 },
{ "lrm", 8206 },
{ "rlm", 8207 },
{ "ndash", 8211 },
{ "mdash", 8212 },
{ "lsquo", 8216 },
{ "rsquo", 8217 },
{ "sbquo", 8218 },
{ "ldquo", 8220 },
{ "rdquo", 8221 },
{ "bdquo", 8222 },
{ "dagger", 8224 },
{ "Dagger", 8225 },
{ "bull", 8226 },
{ "hellip", 8230 },
{ "permil", 8240 },
{ "prime", 8242 },
{ "Prime", 8243 },
{ "lsaquo", 8249 },
{ "rsaquo", 8250 },
{ "oline", 8254 },
{ "frasl", 8260 },
{ "euro", 8364 },
{ "image", 8465 },
{ "weierp", 8472 },
{ "real", 8476 },
{ "trade", 8482 },
{ "alefsym", 8501 },
{ "larr", 8592 },
{ "uarr", 8593 },
{ "rarr", 8594 },
{ "darr", 8595 },
{ "harr", 8596 },
{ "crarr", 8629 },
{ "lArr", 8656 },
{ "uArr", 8657 },
{ "rArr", 8658 },
{ "dArr", 8659 },
{ "hArr", 8660 },
{ "forall", 8704 },
{ "part", 8706 },
{ "exist", 8707 },
{ "empty", 8709 },
{ "nabla", 8711 },
{ "isin", 8712 },
{ "notin", 8713 },
{ "ni", 8715 },
{ "prod", 8719 },
{ "sum", 8721 },
{ "minus", 8722 },
{ "lowast", 8727 },
{ "radic", 8730 },
{ "prop", 8733 },
{ "infin", 8734 },
{ "ang", 8736 },
{ "and", 8743 },
{ "or", 8744 },
{ "cap", 8745 },
{ "cup", 8746 },
{ "int", 8747 },
{ "there4", 8756 },
{ "sim", 8764 },
{ "cong", 8773 },
{ "asymp", 8776 },
{ "ne", 8800 },
{ "equiv", 8801 },
{ "le", 8804 },
{ "ge", 8805 },
{ "sub", 8834 },
{ "sup", 8835 },
{ "nsub", 8836 },
{ "sube", 8838 },
{ "supe", 8839 },
{ "oplus", 8853 },
{ "otimes", 8855 },
{ "perp", 8869 },
{ "sdot", 8901 },
{ "lceil", 8968 },
{ "rceil", 8969 },
{ "lfloor", 8970 },
{ "rfloor", 8971 },
{ "lang", 9001 },
{ "rang", 9002 },
{ "loz", 9674 },
{ "spades", 9824 },
{ "clubs", 9827 },
{ "hearts", 9829 },
{ "diams", 9830 },
{ NULL, 0 }
    };
    if (named_ents.empty()) {
	const struct ent *i = ents;
	while (i->n) {
	    named_ents[string(i->n)] = i->v;
	    ++i;
	}
    }
}

void
HtmlParser::decode_entities(string &s)
{
    // We need a const_iterator version of s.end() - otherwise the
    // find() and find_if() templates don't work...
    string::const_iterator amp = s.begin(), s_end = s.end();
    while ((amp = find(amp, s_end, '&')) != s_end) {
	unsigned int val = 0;
	string::const_iterator end, p = amp + 1;
	if (p != s_end && *p == '#') {
	    p++;
	    if (p != s_end && (*p == 'x' || *p == 'X')) {
		// hex
		p++;
		end = find_if(p, s_end, p_notxdigit);
		sscanf(s.substr(p - s.begin(), end - p).c_str(), "%x", &val);
	    } else {
		// number
		end = find_if(p, s_end, p_notdigit);
		val = atoi(s.substr(p - s.begin(), end - p).c_str());
	    }
	} else {
	    end = find_if(p, s_end, p_notalnum);
	    string code = s.substr(p - s.begin(), end - p);
	    map<string, unsigned int>::const_iterator i;
	    i = named_ents.find(code);
	    if (i != named_ents.end()) val = i->second;
	}
	if (end < s_end && *end == ';') end++;
	if (val) {
	    string::size_type amp_pos = amp - s.begin();
	    if (val < 0x80) {
		s.replace(amp_pos, end - amp, 1u, char(val));
	    } else {
		// Convert unicode value val to UTF-8.
		char seq[4];
		unsigned len = nonascii_to_utf8(val, seq);
		s.replace(amp_pos, end - amp, seq, len);
	    }
	    s_end = s.end();
	    // We've modified the string, so the iterators are no longer
	    // valid...
	    amp = s.begin() + amp_pos + 1;
	} else {
	    amp = end;
	}
    }
}

void
HtmlParser::parse_html(const string &body)
{
    in_script = false;

    parameters.clear();
    string::const_iterator start = body.begin();

    while (true) {
	// Skip through until we find an HTML tag, a comment, or the end of
	// document.  Ignore isolated occurrences of `<' which don't start
	// a tag or comment.
	string::const_iterator p = start;
	while (true) {
	    p = find(p, body.end(), '<');
	    if (p == body.end()) break;
	    unsigned char ch = *(p + 1);

	    // Tag, closing tag, or comment (or SGML declaration).
	    if ((!in_script && isalpha(ch)) || ch == '/' || ch == '!') break;

	    if (ch == '?') {
		// PHP code or XML declaration.
		// XML declaration is only valid at the start of the first line.
		// FIXME: need to deal with BOMs...
		if (p != body.begin() || body.size() < 20) break;

		// XML declaration looks something like this:
		// <?xml version="1.0" encoding="UTF-8"?>
		if (p[2] != 'x' || p[3] != 'm' || p[4] != 'l') break;
		if (strchr(" \t\r\n", p[5]) == NULL) break;

		string::const_iterator decl_end = find(p + 6, body.end(), '?');
		if (decl_end == body.end()) break;

		// Default charset for XML is UTF-8.
		charset = "UTF-8";

		string decl(p + 6, decl_end);
		size_t enc = decl.find("encoding");
		if (enc == string::npos) break;

		enc = decl.find_first_not_of(" \t\r\n", enc + 8);
		if (enc == string::npos || enc == decl.size()) break;

		if (decl[enc] != '=') break;
		
		enc = decl.find_first_not_of(" \t\r\n", enc + 1);
		if (enc == string::npos || enc == decl.size()) break;

		if (decl[enc] != '"' && decl[enc] != '\'') break;

		char quote = decl[enc++];
		size_t enc_end = decl.find(quote, enc);

		if (enc != string::npos)
		    charset = decl.substr(enc, enc_end - enc);

		break;
	    }
	    p++;
	}

	// Process text up to start of tag.
	if (p > start) {
	    string text = body.substr(start - body.begin(), p - start);
#if 0
	    convert_to_utf8(text, charset);
#endif
	    decode_entities(text);
	    process_text(text);
	}

	if (p == body.end()) break;

	start = p + 1;

	if (start == body.end()) break;

	if (*start == '!') {
	    if (++start == body.end()) break;
	    if (++start == body.end()) break;
	    // comment or SGML declaration
	    if (*(start - 1) == '-' && *start == '-') {
		++start;
		string::const_iterator close = find(start, body.end(), '>');
		// An unterminated comment swallows rest of document
		// (like Netscape, but unlike MSIE IIRC)
		if (close == body.end()) break;

		p = close;
		// look for -->
		while (p != body.end() && (*(p - 1) != '-' || *(p - 2) != '-'))
		    p = find(p + 1, body.end(), '>');

		if (p != body.end()) {
		    // Check for htdig's "ignore this bit" comments.
		    if (p - start == 15 && string(start, p - 2) == "htdig_noindex") {
			string::size_type i;
			i = body.find("<!--/htdig_noindex-->", p + 1 - body.begin());
			if (i == string::npos) break;
			start = body.begin() + i + 21;
			continue;
		    }
		    // If we found --> skip to there.
		    start = p;
		} else {
		    // Otherwise skip to the first > we found (as Netscape does).
		    start = close;
		}
	    } else {
		// just an SGML declaration, perhaps giving the DTD - ignore it
		start = find(start - 1, body.end(), '>');
		if (start == body.end()) break;
	    }
	    ++start;
	} else if (*start == '?') {
	    if (++start == body.end()) break;
	    // PHP - swallow until ?> or EOF
	    start = find(start + 1, body.end(), '>');

	    // look for ?>
	    while (start != body.end() && *(start - 1) != '?')
		start = find(start + 1, body.end(), '>');

	    // unterminated PHP swallows rest of document (rather arbitrarily
	    // but it avoids polluting the database when things go wrong)
	    if (start != body.end()) ++start;
	} else {
	    // opening or closing tag
	    int closing = 0;

	    if (*start == '/') {
		closing = 1;
		start = find_if(start + 1, body.end(), p_notwhitespace);
	    }

	    p = start;
	    start = find_if(start, body.end(), p_nottag);
	    string tag = body.substr(p - body.begin(), start - p);
	    // convert tagname to lowercase
	    lowercase_string(tag);

	    if (closing) {
		closing_tag(tag);
		if (in_script && tag == "script") in_script = false;

		/* ignore any bogus parameters on closing tags */
		p = find(start, body.end(), '>');
		if (p == body.end()) break;
		start = p + 1;
	    } else {
		// FIXME: parse parameters lazily.
		while (start < body.end() && *start != '>') {
		    string name, value;

		    p = find_if(start, body.end(), p_whitespaceeqgt);

		    name.assign(body, start - body.begin(), p - start);

		    p = find_if(p, body.end(), p_notwhitespace);

		    start = p;
		    if (start != body.end() && *start == '=') {
			start = find_if(start + 1, body.end(), p_notwhitespace);

			p = body.end();

			int quote = *start;
			if (quote == '"' || quote == '\'') {
			    start++;
			    p = find(start, body.end(), quote);
			}

			if (p == body.end()) {
			    // unquoted or no closing quote
			    p = find_if(start, body.end(), p_whitespacegt);
			}
			value.assign(body, start - body.begin(), p - start);
			start = find_if(p, body.end(), p_notwhitespace);

			if (!name.empty()) {
			    // convert parameter name to lowercase
			    lowercase_string(name);
			    // in case of multiple entries, use the first
			    // (as Netscape does)
			    parameters.insert(make_pair(name, value));
			}
		    }
		}
		opening_tag(tag);
		parameters.clear();

		// In <script> tags we ignore opening tags to avoid problems
		// with "a<b".
		if (tag == "script") in_script = true;

		if (start != body.end() && *start == '>') ++start;
	    }
	}
    }
}
