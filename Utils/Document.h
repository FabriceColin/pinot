/*
 *  Copyright 2005-2014 Fabrice Colin
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

#ifndef _DOCUMENT_H
#define _DOCUMENT_H

#include <string>

#include "DocumentInfo.h"
#include "Visibility.h"

/// Document wraps a document's properties and data.
class PINOT_EXPORT Document : public DocumentInfo
{
	public:
		Document();
		Document(const std::string &title, const std::string &location,
			const std::string &type, const std::string &language);
		Document(const DocumentInfo &info);
		Document(const Document &other);
		virtual ~Document();

		Document& operator=(const Document& other);

		bool operator<(const Document& other) const;

		/// Copies the given data in the document.
		virtual bool setData(const char *data, off_t length);

		/// Maps the given file.
		virtual bool setDataFromFile(const std::string &fileName);

		/// Returns the document's data; NULL if document is empty.
		virtual const char *getData(off_t &length) const;

		/// Resets the document's data.
		void resetData(void);

		/// Checks whether the document is binary.
		virtual bool isBinary(void) const;

	protected:
		char *m_pData;
		off_t m_dataLength;
		bool m_isMapped;

};

#endif // _DOCUMENT_H
