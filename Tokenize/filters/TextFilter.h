/*
 *  Copyright 2007-2008 Fabrice Colin
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

#ifndef _DIJON_TEXTFILTER_H
#define _DIJON_TEXTFILTER_H

#include <string>

#include  "Filter.h"

namespace Dijon
{
    class DIJON_FILTER_EXPORT TextFilter : public Filter
    {
    public:
	/// Builds an empty filter.
	TextFilter();
	/// Destroys the filter.
	virtual ~TextFilter();


	// Information.

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
	 * Returns false if this input is not supported or an error occurred.
	 */
	virtual bool set_document_data(const char *data_ptr, off_t data_length);

	/** (Re)initializes the filter with the given data.
	 * Call next_document() to position the filter onto the first document.
	 * Returns false if this input is not supported or an error occurred.
	 */
	virtual bool set_document_string(const std::string &data_str);

	/** (Re)initializes the filter with the given file.
	 * Call next_document() to position the filter onto the first document.
	 * Returns false if this input is not supported or an error occurred.
	 */
	virtual bool set_document_file(const std::string &file_path,
		bool unlink_when_done = false);

	/** (Re)initializes the filter with the given URI.
	 * Call next_document() to position the filter onto the first document.
	 * Returns false if this input is not supported or an error occurred.
	 */
	virtual bool set_document_uri(const std::string &uri);


	// Going from one nested document to the next.

	/** Returns true if there are nested documents left to extract.
	 * Returns false if the end of the parent document was reached
	 * or an error occurred.
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

	/// Returns the message for the most recent error that has occurred.
	virtual std::string get_error(void) const;

    protected:
	bool m_doneWithDocument;

	virtual void rewind(void);

    private:
	/// TextFilter objects cannot be copied.
	TextFilter(const TextFilter &other);
	/// TextFilter objects cannot be copied.
	TextFilter& operator=(const TextFilter& other);

    };
}

#endif // _DIJON_TEXTFILTER_H
