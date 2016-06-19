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

#ifndef _XAPIAN_INDEX_H
#define _XAPIAN_INDEX_H

#include <string>
#include <set>
#include <map>

#include "config.h"
#include "CJKVTokenizer.h"
#include "XapianDatabase.h"
#include "IndexInterface.h"

#if !ENABLE_XAPIAN_DB_METADATA
// Database metadata is only available in Xapian > 1.0.2
#if XAPIAN_NUM_VERSION > 1000002
#define ENABLE_XAPIAN_DB_METADATA 1
#else
#define ENABLE_XAPIAN_DB_METADATA 0
#endif
#endif

/// A Xapian-based index.
class XapianIndex : public IndexInterface
{
	public:
		XapianIndex(const std::string &indexName);
		XapianIndex(const XapianIndex &other);
		virtual ~XapianIndex();

		XapianIndex &operator=(const XapianIndex &other);

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
		std::string m_databaseName;
		bool m_goodIndex;
		bool m_doSpelling;
		std::string m_stemLanguage;

		bool listDocumentsWithTerm(const std::string &term, std::set<unsigned int> &docIds,
			unsigned int maxDocsCount = 0, unsigned int startDoc = 0) const;

		void addPostingsToDocument(const Xapian::Utf8Iterator &itor, Xapian::Document &doc,
			const Xapian::WritableDatabase &db, const std::string &prefix,
			bool noStemming, bool &doSpelling,  Xapian::termcount &termPos) const;

		void addPostingsToDocument(Dijon::CJKVTokenizer &tokenizer, Xapian::Stem *pStemmer,
			const std::string &text, Xapian::Document &doc,
			const Xapian::WritableDatabase &db, const std::string &prefix,
			bool &doSpelling, Xapian::termcount &termPos) const;

		static void addLabelsToDocument(Xapian::Document &doc,
			const std::set<std::string> &labels, bool skipInternals);

		void removePostingsFromDocument(const Xapian::Utf8Iterator &itor, Xapian::Document &doc,
			const Xapian::WritableDatabase &db, const std::string &prefix,
			bool noStemming, bool &doSpelling) const;

		void addCommonTerms(const DocumentInfo &info, Xapian::Document &doc,
			const Xapian::WritableDatabase &db, Xapian::termcount &termPos);

		void removeCommonTerms(Xapian::Document &doc, const Xapian::WritableDatabase &db);

		std::string scanDocument(const std::string &suggestedLanguage,
			const char *pData, off_t dataLength);

		void setDocumentData(const DocumentInfo &info, Xapian::Document &doc,
			const std::string &language) const;

		bool deleteDocuments(const std::string &term);

};

#endif // _XAPIAN_INDEX_H
