/*
 *  Copyright 2005-2008 Fabrice Colin
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
 
#ifndef _INDEX_INTERFACE_H
#define _INDEX_INTERFACE_H

#include <string>
#include <set>
#include <map>

#include "Document.h"
#include "Visibility.h"

/// Interface implemented by indexes.
class PINOT_EXPORT IndexInterface
{
	public:
		IndexInterface(const IndexInterface &other) {};
		virtual ~IndexInterface() {};

		typedef enum { BY_LABEL = 0, BY_DIRECTORY, BY_FILE, BY_CONTAINER_FILE } NameType;

		/// Returns false if the index couldn't be opened.
		virtual bool isGood(void) const = 0;

		/// Gets metadata.
		virtual std::string getMetadata(const std::string &name) const = 0;

		/// Sets metadata.
		virtual bool setMetadata(const std::string &name, const std::string &value) const = 0;

		/// Gets the index location.
		virtual std::string getLocation(void) const = 0;

		/// Returns a document's properties.
		virtual bool getDocumentInfo(unsigned int docId, DocumentInfo &docInfo) const = 0;

		/// Returns a document's terms count.
		virtual unsigned int getDocumentTermsCount(unsigned int docId) const = 0;

		/// Returns a document's terms.
		virtual bool getDocumentTerms(unsigned int docId,
			std::map<unsigned int, std::string> &wordsBuffer) const = 0;

		/// Sets the list of known labels.
		virtual bool setLabels(const std::set<std::string> &labels, bool resetLabels) = 0;

		/// Gets the list of known labels.
		virtual bool getLabels(std::set<std::string> &labels) const = 0;

		/// Adds a label.
		virtual bool addLabel(const std::string &name) = 0;

		/// Deletes all references to a label.
		virtual bool deleteLabel(const std::string &name) = 0;

		/// Determines whether a document has a label.
		virtual bool hasLabel(unsigned int docId, const std::string &name) const = 0;

		/// Returns a document's labels.
		virtual bool getDocumentLabels(unsigned int docId, std::set<std::string> &labels) const = 0;

		/// Sets a document's labels.
		virtual bool setDocumentLabels(unsigned int docId, const std::set<std::string> &labels,
			bool resetLabels = true) = 0;

		/// Sets documents' labels.
		virtual bool setDocumentsLabels(const std::set<unsigned int> &docIds,
			const std::set<std::string> &labels, bool resetLabels = true) = 0;

		/// Checks whether the given URL is in the index.
		virtual unsigned int hasDocument(const std::string &url) const = 0;

		/// Gets terms with the same root.
		virtual unsigned int getCloseTerms(const std::string &term, std::set<std::string> &suggestions) = 0;

		/// Returns the ID of the last document.
		virtual unsigned int getLastDocumentID(void) const = 0;

		/// Returns the number of documents.
		virtual unsigned int getDocumentsCount(const std::string &labelName = "") const = 0;

		/// Lists documents.
		virtual unsigned int listDocuments(std::set<unsigned int> &docIDList,
			unsigned int maxDocsCount = 0, unsigned int startDoc = 0) const = 0;

		/// Lists documents.
		virtual bool listDocuments(const std::string &name, std::set<unsigned int> &docIds,
			NameType type, unsigned int maxDocsCount = 0, unsigned int startDoc = 0) const = 0;

		/// Indexes the given data.
		virtual bool indexDocument(const Document &doc, const std::set<std::string> &labels,
			unsigned int &docId) = 0;

		/// Updates the given document.
		virtual bool updateDocument(unsigned int docId, const Document &doc) = 0;

		/// Updates a document's properties.
		virtual bool updateDocumentInfo(unsigned int docId, const DocumentInfo &docInfo) = 0;

		/// Unindexes the given document.
		virtual bool unindexDocument(unsigned int docId) = 0;

		/// Unindexes the given document.
		virtual bool unindexDocument(const std::string &location) = 0;

		/// Unindexes documents.
		virtual bool unindexDocuments(const std::string &name, NameType type) = 0;

		/// Unindexes all documents.
		virtual bool unindexAllDocuments(void) = 0;

		/// Flushes recent changes to the disk.
		virtual bool flush(void) = 0;

		/// Reopens the index.
		virtual bool reopen(void) const = 0;

		/// Resets the index.
		virtual bool reset(void) = 0;

	protected:
		IndexInterface() { };

};

#endif // _INDEX_INTERFACE_H
