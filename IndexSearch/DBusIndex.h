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

#ifndef _DBUS_INDEX_H
#define _DBUS_INDEX_H

#include <string>
#include <set>
#include <map>
#include "config.h"
extern "C"
{
#if DBUS_VERSION < 1000000
#define DBUS_API_SUBJECT_TO_CHANGE
#endif
#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>
}

#include "IndexInterface.h"

#define PINOT_DBUS_SERVICE_NAME "de.berlios.Pinot"
#define PINOT_DBUS_OBJECT_PATH "/de/berlios/Pinot"

/// Allows to write to the daemon index via D-Bus. 
class DBusIndex : public IndexInterface
{
	public:
		DBusIndex(IndexInterface *pROIndex);
		DBusIndex(const DBusIndex &other);
		virtual ~DBusIndex();

		DBusIndex &operator=(const DBusIndex &other);

		/// Extracts docId and docInfo from a dbus message.
		static bool documentInfoFromDBus(DBusMessageIter *iter, unsigned int &docId,
			DocumentInfo &docInfo);

		/// Converts docId and docInfo to a dbus message.
		static bool documentInfoToDBus(DBusMessageIter *iter, unsigned int docId,
			const DocumentInfo &docInfo);

		/// Asks the D-Bus service to reload its configuration.
		static bool reload(void);

		/// Gets some statistics from the D-Bus service.
		static bool getStatistics(unsigned int crawledCount, unsigned int docsCount,
			bool &lowDiskSpace, bool &onBattery, bool &crawling);

		/// Returns false if the index couldn't be opened.
		virtual bool isGood(void) const;

		/// Gets metadata.
		virtual std::string getMetadata(const std::string &name) const;

		/// Sets metadata.
		virtual bool setMetadata(const std::string &name, const std::string &value) const;

		/// Gets the index location.
		virtual std::string getLocation(void) const;

		/// Returns a document's properties.
		virtual bool getDocumentInfo(unsigned int docId, DocumentInfo &docInfo) const;

		/// Returns a document's terms count.
		virtual unsigned int getDocumentTermsCount(unsigned int docId) const;

		/// Returns a document's terms.
		virtual bool getDocumentTerms(unsigned int docId,
			std::map<unsigned int, std::string> &wordsBuffer) const;

		/// Sets the list of known labels.
		virtual bool setLabels(const std::set<std::string> &labels, bool resetLabels);

		/// Gets the list of known labels.
		virtual bool getLabels(std::set<std::string> &labels) const;

		/// Adds a label.
		virtual bool addLabel(const std::string &name);

		/// Deletes all references to a label.
		virtual bool deleteLabel(const std::string &name);

		/// Determines whether a document has a label.
		virtual bool hasLabel(unsigned int docId, const std::string &name) const;

		/// Returns a document's labels.
		virtual bool getDocumentLabels(unsigned int docId, std::set<std::string> &labels) const;

		/// Sets a document's labels.
		virtual bool setDocumentLabels(unsigned int docId, const std::set<std::string> &labels,
			bool resetLabels = true);

		/// Sets documents' labels.
		virtual bool setDocumentsLabels(const std::set<unsigned int> &docIds,
			const std::set<std::string> &labels, bool resetLabels = true);

		/// Checks whether the given URL is in the index.
		virtual unsigned int hasDocument(const std::string &url) const;

		/// Gets terms with the same root.
		virtual unsigned int getCloseTerms(const std::string &term, std::set<std::string> &suggestions);

		/// Returns the ID of the last document.
		virtual unsigned int getLastDocumentID(void) const;

		/// Returns the number of documents.
		virtual unsigned int getDocumentsCount(const std::string &labelName = "") const;

		/// Lists documents.
		virtual unsigned int listDocuments(std::set<unsigned int> &docIDList,
			unsigned int maxDocsCount = 0, unsigned int startDoc = 0) const;

		/// Lists documents.
		virtual bool listDocuments(const std::string &name, std::set<unsigned int> &docIds,
			NameType type, unsigned int maxDocsCount = 0, unsigned int startDoc = 0) const;

		/// Indexes the given data.
		virtual bool indexDocument(const Document &doc, const std::set<std::string> &labels,
			unsigned int &docId);

		/// Updates the given document.
		virtual bool updateDocument(unsigned int docId, const Document &doc);

		/// Updates a document's properties.
		virtual bool updateDocumentInfo(unsigned int docId, const DocumentInfo &docInfo);

		/// Unindexes the given document.
		virtual bool unindexDocument(unsigned int docId);

		/// Unindexes the given document.
		virtual bool unindexDocument(const std::string &location);

		/// Unindexes documents.
		virtual bool unindexDocuments(const std::string &name, NameType type);

		/// Unindexes all documents.
		virtual bool unindexAllDocuments(void);

		/// Flushes recent changes to the disk.
		virtual bool flush(void);

		/// Reopens the index.
		virtual bool reopen(void) const;

		/// Resets the index.
		virtual bool reset(void);

	protected:
		IndexInterface *m_pROIndex;

};

#endif // _DBUS_INDEX_H
