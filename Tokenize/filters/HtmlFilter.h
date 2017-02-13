/*
 *  Copyright 2007-2016 Fabrice Colin
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

#ifndef _DIJON_HTMLFILTER_H
#define _DIJON_HTMLFILTER_H

#include <string>
#include <set>
#include <map>

#include "HtmlParser.h"
#include "Filter.h"

namespace Dijon
{
    /// A link in an HTML page.
    class DIJON_FILTER_EXPORT Link
    {
    public:
		Link();
		Link(const Link &other);
		~Link();

		Link& operator=(const Link& other);
		bool operator==(const Link &other) const;
		bool operator<(const Link &other) const;

		std::string m_url;
		std::string m_name;
		unsigned int m_index;
		unsigned int m_startPos;
		unsigned int m_endPos;
    };

    class DIJON_FILTER_EXPORT HtmlFilter : public Filter
    {
    public:
	/// Builds an empty filter.
	HtmlFilter();
	/// Destroys the filter.
	virtual ~HtmlFilter();


	/// Returns what data the filter requires as input.
	virtual bool is_data_input_ok(DataInput input) const;


	// Initialization.

	/** Sets a property, prior to calling set_document_XXX().
	 * Returns false if the property is not supported.
	 */
	virtual bool set_property(Properties prop_name, const std::string &prop_value);

	/** (Re)initializes the filter with the given data.
	 * Caller should ensure the given pointer is valid until the
	 * Filter object is destroyed, as some filters may not need to
	 * do a deep copy of the data.
	 * Call next_document() to position the filter onto the first document.
	 * Returns false if this input is not supported or an error occured.
	 */
	virtual bool set_document_data(const char *data_ptr, off_t data_length);

	/** (Re)initializes the filter with the given data.
	 * Call next_document() to position the filter onto the first document.
	 * Returns false if this input is not supported or an error occured.
	 */
	virtual bool set_document_string(const std::string &data_str);

	/** (Re)initializes the filter with the given file.
	 * Call next_document() to position the filter onto the first document.
	 * Returns false if this input is not supported or an error occured.
	 */
	virtual bool set_document_file(const std::string &file_path,
		bool unlink_when_done = false);

	/** (Re)initializes the filter with the given URI.
	 * Call next_document() to position the filter onto the first document.
	 * Returns false if this input is not supported or an error occured.
	 */
	virtual bool set_document_uri(const std::string &uri);


	// Going from one nested document to the next.

	/** Returns true if there are nested documents left to extract.
	 * Returns false if the end of the parent document was reached
	 * or an error occured.
	 */
	virtual bool has_documents(void) const;

	/** Moves to the next nested document.
	 * Returns false if there are none left.
	 */ 
	virtual bool next_document(void);

	/** Skips to the nested document with the given ipath.
	 * Returns false if no such document exists.
	 */
	virtual bool skip_to_document(const std::string &ipath);


	// Accessing documents' contents.

	/// Returns the message for the most recent error that has occured.
	virtual std::string get_error(void) const;

	/// Returns the links set.
	bool get_links(std::set<Link> &links) const;

	class ParserState : public HtmlParser
	{
		public:
			ParserState(dstring &text);
			virtual ~ParserState();

			virtual void process_text(const string &text);
			virtual void opening_tag(const string &tag);
			virtual void closing_tag(const string &tag);

			bool get_links_text(unsigned int currentLinkIndex);

			bool m_isValid;
			bool m_findAbstract;
			unsigned int m_textPos;
			bool m_inHead;
			bool m_foundHead;
			bool m_appendToTitle;
			bool m_appendToText;
			bool m_appendToLink;
			unsigned int m_skip;
			std::string m_charset;
			std::string m_title;
			dstring &m_text;
			std::string m_abstract;
			Link m_currentLink;
			std::set<Link> m_links;
			std::set<Link> m_frames;
			std::map<std::string, std::string> m_metaTags;

		protected:
			void append_whitespace(void);
			void append_text(const string &text);

	};

    protected:
	ParserState *m_pParserState;
	std::string m_error;
	bool m_skipText;
	bool m_findAbstract;

	virtual void rewind(void);

	bool parse_html(const string &html);

    private:
	/// HtmlFilter objects cannot be copied.
	HtmlFilter(const HtmlFilter &other);
	/// HtmlFilter objects cannot be copied.
	HtmlFilter& operator=(const HtmlFilter& other);

    };
}

#endif // _DIJON_HTMLFILTER_H
