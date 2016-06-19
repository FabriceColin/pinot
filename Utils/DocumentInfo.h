/*
 *  Copyright 2005-2012 Fabrice Colin
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

#ifndef _DOCUMENT_INFO_H
#define _DOCUMENT_INFO_H

#include <sys/types.h>
#include <string>
#include <map>
#include <set>

#include "Visibility.h"

/// This represents all the properties of a document.
class PINOT_EXPORT DocumentInfo
{
	public:
		DocumentInfo();
		DocumentInfo(const std::string &title, const std::string &location,
			const std::string &type, const std::string &language);
		DocumentInfo(const DocumentInfo &other);
		virtual ~DocumentInfo();

		DocumentInfo& operator=(const DocumentInfo& other);

		bool operator<(const DocumentInfo& other) const;

		typedef enum { SERIAL_ALL = 0, SERIAL_FIELDS, SERIAL_LABELS } SerialExtent;

		/// Serializes the document.
		std::string serialize(SerialExtent extent = SERIAL_ALL) const;

		/// Deserializes the document.
		void deserialize(const std::string &info, SerialExtent extent = SERIAL_ALL);

		/// Sets the title of the document.
		virtual void setTitle(const std::string &title);

		/// Returns the title of the document.
		virtual std::string getTitle(void) const;

		/// Sets the original location of the document.
		virtual void setLocation(const std::string &location);

		/// Returns the original location of the document.
		virtual std::string getLocation(bool withIPath = false) const;

		/// Sets the internal path to the document.
		void setInternalPath(const std::string &ipath);

		/// Returns the internal path to the document.
		std::string getInternalPath(void) const;

		/// Sets the type of the document.
		virtual void setType(const std::string &type);

		/// Returns the type of the document.
		virtual std::string getType(void) const;

		/// Sets the language of the document.
		virtual void setLanguage(const std::string &language);

		/// Returns the document's language.
		virtual std::string getLanguage(void) const;

		/// Sets the document's timestamp.
		virtual void setTimestamp(const std::string &timestamp);

		/// Returns the document's timestamp.
		virtual std::string getTimestamp(void) const;

		/// Sets the document's size in bytes.
		virtual void setSize(off_t size);

		/// Returns the document's size in bytes.
		virtual off_t getSize(void) const;

		/// Sets the document's extract.
		virtual void setExtract(const std::string &extract);

		/// Returns the document's extract.
		virtual std::string getExtract(void) const;

		/// Sets the document's score.
		void setScore(float score);

		/// Returns the document's score.
		float getScore(void) const;

		/// Sets a foreign field.
		void setOther(const std::string &name,
			const std::string &value);

		/// Returns a foreign field.
		std::string getOther(const std::string &name) const;

		/// Sets the document's labels.
		virtual void setLabels(const std::set<std::string> &labels);

		/// Returns the document's labels.
		virtual const std::set<std::string> &getLabels(void) const;

		/// Sets that the document is indexed.
		void setIsIndexed(unsigned int indexId, unsigned int docId);

		/// Sets that the document is not indexed.
		void setIsNotIndexed(void);

		/// Gets whether the document is indexed.
		bool getIsIndexed(void) const;

		/// Gets whether the document is indexed.
		unsigned int getIsIndexed(unsigned int &indexId) const;

	protected:
		std::map<std::string, std::string> m_fields;
		std::string m_extract;
		float m_score;
		std::set<std::string> m_labels;
		unsigned int m_indexId;
		unsigned int m_docId;

		void setField(const std::string &name, const std::string &value);

		std::string getField(const std::string &name) const;

};

#endif // _DOCUMENT_INFO_H
