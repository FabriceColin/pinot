/*
 *  Copyright 2007-2009 Fabrice Colin
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

#ifndef _DIJON_FILTER_H
#define _DIJON_FILTER_H

#include <string>
#include <set>
#include <map>

#ifndef DIJON_FILTER_EXPORT
#if defined __GNUC__ && (__GNUC__ >= 4)
  #define DIJON_FILTER_EXPORT     __attribute__ ((visibility("default")))
  #define DIJON_FILTER_INITIALIZE __attribute__((constructor))
  #define DIJON_FILTER_SHUTDOWN   __attribute__((destructor))
#else
  #define DIJON_FILTER_EXPORT
  #define DIJON_FILTER_INITIALIZE
  #define DIJON_FILTER_SHUTDOWN
#endif
#endif

#include "Memory.h"

namespace Dijon
{
    class Filter;

    /** Provides the list of MIME types supported by the filter(s).
     * The character string is allocated with new[].
     * This function is exported by dynamically loaded filter libraries.
     */
    typedef bool (get_filter_types_func)(std::set<std::string> &);
    /** Returns what data should be passed to the filter(s).
     * Output is cast from Filter::DataInput to int for convenience.
     * This function is exported by dynamically loaded filter libraries.
     * The aim is to let the client application know before-hand whether
     * it should load documents or not.
     */
    typedef bool (check_filter_data_input_func)(int);
    /** Returns a Filter that handles the given MIME type.
     * The Filter object is allocated with new.
     * This function is exported by dynamically loaded filter libraries
     * and serves as a factory for Filter objects, so that the client
     * application doesn't have to know which Filter sub-types handle
     * which MIME types.
     */
    typedef Filter *(get_filter_func)(const std::string &);
    /** Converts text to UTF-8.
     */
    typedef std::string (convert_to_utf8_func)(const char *,
        off_t, const std::string &);

    /// Filter interface.
    class DIJON_FILTER_EXPORT Filter
    {
    public:
	/// Builds an empty filter.
	Filter(const std::string &mime_type);
	/// Destroys the filter.
	virtual ~Filter();


	// Enumerations.

	/** What data a filter supports as input.
	 * It can be either the whole document data, its file name, or its URI.
	 */
	typedef enum { DOCUMENT_DATA = 0, DOCUMENT_STRING, DOCUMENT_FILE_NAME, DOCUMENT_URI } DataInput;

	/** Input properties supported by the filter.
	 * - PREFERRED_CHARSET is the charset preferred by the client application.
	 * The filter will convert document's content to this charset if possible.
	 * - OPERATING_MODE can be set to either view or index.
	 * - MAXIMUM_NESTED_SIZE is the maximum size in bytes of nested documents.
	 */
	typedef enum { PREFERRED_CHARSET = 0, OPERATING_MODE, MAXIMUM_NESTED_SIZE } Properties;


	// Information.

	/// Returns the MIME type handled by the filter.
	std::string get_mime_type(void) const;

	/// Returns what data the filter requires as input.
	virtual bool is_data_input_ok(DataInput input) const = 0;


	// Initialization.

	/** Sets a property, prior to calling set_document_XXX().
	 * Returns false if the property is not supported.
	 */
	virtual bool set_property(Properties prop_name, const std::string &prop_value) = 0;

	/** (Re)initializes the filter with the given data.
	 * Caller should ensure the given pointer is valid until the
	 * Filter object is destroyed, as some filters may not need to
	 * do a deep copy of the data.
	 * Call next_document() to position the filter onto the first document.
	 * Returns false if this input is not supported or an error occured.
	 */
	virtual bool set_document_data(const char *data_ptr, off_t data_length) = 0;

	/** (Re)initializes the filter with the given data.
	 * Call next_document() to position the filter onto the first document.
	 * Returns false if this input is not supported or an error occured.
	 */
	virtual bool set_document_string(const std::string &data_str) = 0;

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
	virtual bool set_document_uri(const std::string &uri) = 0;


	// Going from one nested document to the next.

	/** Returns true if there are nested documents left to extract.
	 * Returns false if the end of the parent document was reached
	 * or an error occured.
	 */
	virtual bool has_documents(void) const = 0;

	/** Moves to the next nested document.
	 * Returns false if there are none left.
	 */ 
	virtual bool next_document(void) = 0;

	/** Skips to the nested document with the given ipath.
	 * Returns false if no such document exists.
	 */
	virtual bool skip_to_document(const std::string &ipath) = 0;


	// Accessing documents' contents.

	/// Returns the message for the most recent error that has occured.
	virtual std::string get_error(void) const = 0;

	/** Returns a dictionary of metadata extracted from the current document.
	 * Metadata fields may include one or more of the following :
	 * title, ipath, mimetype, language, charset, author, creator,
	 * publisher, modificationdate, creationdate, size
	 * Special considerations apply :
	 * - ipath is an internal path to the nested document that can be
	 * later passed to skip_to_document(). It may be empty if the parent
	 * document's type doesn't allow embedding, in which case the filter
	 * should only return one document.
	 * - mimetype should be text/plain if the document could be handled
	 * internally, empty if unknown. If any other value, it is expected
	 * that the client application can pass the nested document's content
	 * to another filter that supports this particular type.
	 */
	const std::map<std::string, std::string> &get_meta_data(void) const;

	/// Returns content.
	const dstring &get_content(void) const;

    protected:
	/// The MIME type handled by the filter.
	std::string m_mimeType;
	/// Metadata dictionary.
	std::map<std::string, std::string> m_metaData;
	/// Content.
	dstring m_content;
	/// The name of the input file, if any.
	std::string m_filePath;

	/// Rewinds the filter.
	virtual void rewind(void);

    private:
	/// Whether the input file should be deleted when done.
	bool m_deleteInputFile;

	/// Filter objects cannot be copied.
	Filter(const Filter &other);
	/// Filter objects cannot be copied.
	Filter& operator=(const Filter& other);

	/// Deletes the input file.
	void deleteInputFile(void);

    };
}

#endif // _DIJON_FILTER_H
