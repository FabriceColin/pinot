/*
 *  Copyright 2005-2009 Fabrice Colin
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

#ifndef _XAPIAN_DATABASE_H
#define _XAPIAN_DATABASE_H

#include <string>
#include <set>
#include <pthread.h>
#include <xapian.h>

#include "DocumentInfo.h"

/// Lockable Xapian database.
class XapianDatabase
{
	public:
		XapianDatabase(const std::string &databaseName,
			bool readOnly = true, bool overwrite = false);
		XapianDatabase(const std::string &databaseName,
			XapianDatabase *pFirst, XapianDatabase *pSecond);
		XapianDatabase(const XapianDatabase &other);
		virtual ~XapianDatabase();

		XapianDatabase &operator=(const XapianDatabase &other);

		/// Returns false if the database couldn't be opened.
		bool isOpen(void) const;

		/// Returns true if the database is a merge of other databases.
		bool isMerge(void) const;

		/// Returns false if the database isn't opened in write mode.
		bool isWritable(void) const;

		/// Returns true if the database supports spelling.
		bool withSpelling(void);

		/// Returns false if the database was of an obsolete format.
		bool wasObsoleteFormat(void) const;

		/// Reopens the database.
		void reopen(void);

		/// Attempts to lock and retrieve the database.
		Xapian::Database *readLock(void);

		/// Attempts to lock and retrieve the database.
		Xapian::WritableDatabase *writeLock(void);

		/// Unlocks the database.
		void unlock(void);

		/// Returns a record for the document's properties.
		static std::string propsToRecord(DocumentInfo *pDoc);

		/// Sets the document's properties acording to the record.
		static void recordToProps(const std::string &record, DocumentInfo *pDoc);

		/// Returns the URL for the given document in the given index.
		static std::string buildUrl(const std::string &database, unsigned int docId);

		/// Truncates or partially hashes a term.
		static std::string limitTermLength(const std::string &term, bool makeUnique = false);

	protected:
		static const unsigned int m_maxTermLength;
		std::string m_databaseName;
		bool m_withSpelling;
		bool m_readOnly;
		bool m_overwrite;
		bool m_obsoleteFormat;
		pthread_mutex_t m_rwLock;
		Xapian::Database *m_pDatabase;
		bool m_isOpen;
		bool m_merge;
		XapianDatabase *m_pFirst;
		XapianDatabase *m_pSecond;

		void initializeLock(void);

		void openDatabase(void);

		static bool badRecordField(const std::string &field);

};

#endif // _XAPIAN_DATABASE_H
