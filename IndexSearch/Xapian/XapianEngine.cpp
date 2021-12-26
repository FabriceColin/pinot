/*
 *  Copyright 2005-2021 Fabrice Colin
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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <string>
#include <cstring>
#include <map>
#include <vector>
#include <iostream>
#include <fstream>
#include <algorithm>

#include "config.h"
#include "Languages.h"
#include "StringManip.h"
#include "TimeConverter.h"
#include "Timer.h"
#include "Url.h"
#include "CJKVTokenizer.h"
#include "FieldMapperInterface.h"
#include "XapianDatabaseFactory.h"
#include "XapianEngine.h"

using std::string;
using std::map;
using std::multimap;
using std::vector;
using std::clog;
using std::clog;
using std::endl;
using std::inserter;
using std::getline;
using std::ifstream;
using namespace Dijon;

extern FieldMapperInterface *g_pMapper;

static void checkFilter(const string &freeQuery, string::size_type filterValueStart,
	bool &escapeValue, bool &hashValue)
{
	string filterName;
	string::size_type filterNameStart = freeQuery.rfind(' ', filterValueStart);

	escapeValue = hashValue = false;

	if (filterNameStart == string::npos)
	{
		filterName = freeQuery.substr(0, filterValueStart);
	}
	else
	{
		filterName = freeQuery.substr(filterNameStart + 1, filterValueStart - filterNameStart - 1);
	}
#ifdef DEBUG
	clog << "checkFilter: filter " << filterName << endl;
#endif

	// In XapianIndex, these are escaped and hashed
	if ((filterName == "file") ||
		(filterName =="dir") ||
		(filterName == "url") ||
		(filterName == "path"))
	{
		escapeValue = hashValue = true;
	}
	// except label which is only escaped
	else if (filterName == "label")
	{
		escapeValue = true;
	}
	else if (g_pMapper != NULL)
	{
		escapeValue = g_pMapper->isEscaped(filterName);
	}
}

class TimeValueRangeProcessor : public Xapian::RangeProcessor
{
	public:
		TimeValueRangeProcessor(Xapian::valueno valueNumber) :
			Xapian::RangeProcessor(),
			m_valueNumber(valueNumber)
		{
		}
		virtual ~TimeValueRangeProcessor()
		{
		}

		virtual Xapian::Query operator()(const std::string &begin, const std::string &end)
		{
			if ((begin.size() == 6) &&
				(end.size() == 6))
			{
				// HHMMSS
#ifdef DEBUG
				clog << "TimeValueRangeProcessor::operator: accepting " << begin << ".." << end << endl;
#endif

				return Xapian::Query(Xapian::Query::OP_VALUE_RANGE,
					m_valueNumber,
					begin,end);
			}
			if ((begin.size() == 8) && (end.size() == 8) &&
				(begin[2] == begin[5]) && (end[2] == end[5]) && (begin[2] == end[2]) &&
				(end[4] == ':'))
			{
				std::string lower(begin), upper(end);

				// HH:MM:SS
				lower.erase(2, 1);
				lower.erase(5, 1);
				upper.erase(2, 1);
				upper.erase(5, 1);
#ifdef DEBUG
				clog << "TimeValueRangeProcessor::operator: accepting " << lower << ".." << upper << endl;
#endif

				return Xapian::Query(Xapian::Query::OP_VALUE_RANGE,
					m_valueNumber,
					lower, upper);
			}
#ifdef DEBUG
			clog << "TimeValueRangeProcessor::operator: rejecting " << begin << ".." << end << endl;
#endif

			return Xapian::Query(Xapian::Query::OP_INVALID);
		}

	protected:
		Xapian::valueno m_valueNumber;

};

class TermDecider : public Xapian::ExpandDecider
{
	public:
		TermDecider(Xapian::Database *pIndex,
			Xapian::Stem *pStemmer,
			Xapian::Stopper *pStopper,
			const string &allowedPrefixes,
			Xapian::Query &query) :
			Xapian::ExpandDecider(),
			m_pIndex(pIndex),
			m_pStemmer(pStemmer),
			m_pStopper(pStopper),
			m_allowedPrefixes(allowedPrefixes),
			m_pTermsToAvoid(NULL)
		{
			m_pTermsToAvoid = new set<string>();

			for (Xapian::TermIterator termIter = query.get_terms_begin();
				termIter != query.get_terms_end(); ++termIter)
			{
				string term(*termIter);

				if (isupper((int)(term[0])) == 0)
				{
					m_pTermsToAvoid->insert(term);
					if (m_pStemmer != NULL)
					{
						string stem((*m_pStemmer)(term));
						m_pTermsToAvoid->insert(stem);
					}
				}
				else if (term[0] == 'Z')
				{
					m_pTermsToAvoid->insert(term.substr(1));
				}
			}
#ifdef DEBUG
			clog << "TermDecider: avoiding " << m_pTermsToAvoid->size() << " terms" << endl;
#endif
		}
		virtual ~TermDecider()
		{
			if (m_pTermsToAvoid != NULL)
			{
				delete m_pTermsToAvoid;
			}
		}

		virtual bool operator()(const std::string &term) const
		{
			CJKVTokenizer tokenizer;
			bool isPrefixed = false;

			// Reject short terms
			if ((tokenizer.has_cjkv(term) == false) &&
				(term.length() < 3))
			{
				return false;
			}

			// Reject terms with prefixes we don't want
			if (isupper((int)(term[0])) != 0)
			{
				isPrefixed = true;

				if (m_allowedPrefixes.find(term[0]) == string::npos)
				{
					return false;
				}
			}

			// Reject terms with spaces
			if (term.find_first_of(" \t\r\n") != string::npos)
			{
				return false;
			}

			// Reject terms that occur only once
			if ((m_pIndex != NULL) &&
				(m_pIndex->get_termfreq(term) <= 1))
			{
				return false;
			}

			// Reject stop words
			if ((m_pStopper != NULL) &&
				((*m_pStopper)(term) == true))
			{
				return false;
			}

			// Stop here if there's no specific terms to avoid
			if (m_pTermsToAvoid->empty() == true)
			{
				return true;
			}

			// Reject query terms
			if (m_pTermsToAvoid->find(term) != m_pTermsToAvoid->end())
			{
				return false;
			}

			// Stop here is there's no stemmer
			if (m_pStemmer == NULL)
			{
				return true;
			}

			// Reject terms that stem to the same as query terms
			// or previously validated terms
			string stem;
			if (isPrefixed == true)
			{
				stem = (*m_pStemmer)(term.substr(1));
			}
			else
			{
				stem = (*m_pStemmer)(term);
			}
			if (m_pTermsToAvoid->find(stem) != m_pTermsToAvoid->end())
			{
				return false;
			}
			m_pTermsToAvoid->insert(stem);

			return true;
		}

	protected:
		Xapian::Database *m_pIndex;
		Xapian::Stem *m_pStemmer;
		Xapian::Stopper *m_pStopper;
		string m_allowedPrefixes;
		set<string> *m_pTermsToAvoid;

};

class FileStopper : public Xapian::SimpleStopper
{
	public:
		FileStopper(const string &languageCode) :
			Xapian::SimpleStopper(),
			m_languageCode(languageCode),
			m_stopwordsCount(0)
		{
			if (languageCode.empty() == false)
			{
				ifstream inputFile;
				string fileName(PREFIX);

				fileName += "/share/pinot/stopwords/stopwords.";
				fileName += languageCode;
				inputFile.open(fileName.c_str());
				if (inputFile.good() == true)
				{
					string line;

					// Each line is a stopword
					while (getline(inputFile, line).eof() == false)
					{
						add(line);
						++m_stopwordsCount;
					}
				}
				inputFile.close();

#ifdef DEBUG
				clog << "FileStopper: " << m_stopwordsCount << " stopwords for language code " << languageCode << endl;
#endif
			}
		}
		virtual ~FileStopper()
		{
		}

		unsigned int get_stopwords_count(void) const
		{
			return m_stopwordsCount;
		}

		static FileStopper *get_stopper(const string &languageCode)
		{
			if (m_pStopper == NULL)
			{
				m_pStopper = new FileStopper(languageCode);
			}
			else if (m_pStopper->m_languageCode != languageCode)
			{
				delete m_pStopper;

				m_pStopper = new FileStopper(languageCode);
			}

			return m_pStopper;
		}

		static void free_stopper(void)
		{
			if (m_pStopper != NULL)
			{
				delete m_pStopper;
				m_pStopper = NULL;
			}
		}

	protected:
		string m_languageCode;
		unsigned int m_stopwordsCount;
		static FileStopper *m_pStopper;

};

FileStopper *FileStopper::m_pStopper = NULL;

class QueryModifier : public Dijon::CJKVTokenizer::TokensHandler
{
	public:
		typedef enum { NONE = 0, BRACKETS } CJKVWrap;

		QueryModifier(const string &query,
			bool diacriticSensitive, unsigned int nGramSize) :
			m_query(query),
			m_diacriticSensitive(diacriticSensitive),
			m_pos(0),
			m_wrap(BRACKETS),
			m_wrapped(false),
			m_nGramCount(0),
			m_nGramSize(nGramSize),
			m_tokensCount(0),
			m_hasCJKV(false),
			m_hasNonCJKV(false)
		{
		}
		virtual ~QueryModifier()
		{
		}

		virtual bool handle_token(const string &tok, bool is_cjkv)
		{
			if (tok.empty() == true)
			{
				return false;
			}
#ifdef DEBUG
			clog << "QueryModifier::handle_token: " << tok << endl;
#endif

			// Where is this token in the original query ?
			string::size_type tokPos = m_query.find(tok, m_pos);
			++m_tokensCount;

			// Is this CJKV ?
			if (is_cjkv == false)
			{
				char lastChar = tok[tok.length() - 1];

				if (tokPos == string::npos)
				{
					// This should have been found
					return false;
				}

				if (m_nGramCount > 0)
				{
					wrapClose();

					m_nGramCount = 0;
					m_pos = tokPos;
				}

				m_currentFilter.clear();
				if (lastChar == '"')
				{
					// It's a quoted string
					m_wrap = NONE;
				}
				else if (lastChar == ':')
				{
					// It's a filter
					m_wrap = NONE;
					m_currentFilter = tok;
				}
				else
				{
					m_wrap = BRACKETS;
				}

				if (m_currentFilter.empty() == true)
				{
					m_hasNonCJKV = true;
				}

				if (m_diacriticSensitive == false)
				{
					// Strip accents and other diacritics from terms
					string unaccentedTok(Dijon::CJKVTokenizer::strip_marks(tok));
					if (tok != unaccentedTok)
					{
#ifdef DEBUG
						clog << "QueryModifier::handle_token: " << tok << " stripped to " << unaccentedTok << endl;
#endif
						m_query.replace(tokPos, tok.length(), unaccentedTok);
					}
				}

				// Return right away
				return true;
			}

			// First n-gram ?
			if (m_nGramCount == 0)
			{
				if (tokPos == string::npos)
				{
					// That's definitely not right
					return false;
				}

				// Append non-CJKV text that precedes and start wrapping CJKV tokens
				if (tokPos > m_pos)
				{
					m_modifiedQuery += " " + m_query.substr(m_pos, tokPos - m_pos);
				}
				m_pos += tok.length();

				wrapOpen();
			}
			else
			{
				m_modifiedQuery += " ";
				if (m_currentFilter.empty() == false)
				{
					m_modifiedQuery += m_currentFilter;
				}
			}
			m_modifiedQuery += tok;
#ifdef DEBUG
			clog << "QueryModifier::handle_token: " << m_modifiedQuery << endl;
#endif

			if (tokPos != string::npos)
			{
				m_pos = tokPos + tok.length();
			}
			++m_nGramCount;
			m_hasCJKV = true;

			return true;
		}

		unsigned int get_tokens_count(void) const
		{
			return m_tokensCount;
		}

		string get_modified_query(bool &pureCJKV)
		{
#ifdef DEBUG
			clog << "QueryModifier::get_modified_query: " << m_pos << "/" << m_query.length() << endl;
#endif

			// Anything left ?
			if (m_pos < m_query.length() - 1)
			{
				m_modifiedQuery += " " + m_query.substr(m_pos);
			}
			wrapClose();
#ifdef DEBUG
			clog << "QueryModifier::get_modified_query: " << m_modifiedQuery << endl;
#endif

			if ((m_hasCJKV == true) &&
				(m_hasNonCJKV == false))
			{
				pureCJKV = true;
			}
			else
			{
				pureCJKV = false;
			}

			return m_modifiedQuery;
		}

	protected:
		string m_query;
		bool m_diacriticSensitive;
		string m_modifiedQuery;
		string::size_type m_pos;
		CJKVWrap m_wrap;
		bool m_wrapped;
		string m_currentFilter;
		unsigned int m_nGramCount;
		unsigned int m_nGramSize;
		unsigned int m_tokensCount;
		bool m_hasCJKV;
		bool m_hasNonCJKV;

		void wrapOpen(void)
		{
			switch (m_wrap)
			{
				case BRACKETS:
					m_modifiedQuery += " (";
					break;
				case NONE:
				default:
					break;
			}
			m_wrapped = true;
		}

		void wrapClose(void)
		{
			if (m_wrapped == false)
			{
				return;
			}

			// Finish wrapping CJKV tokens
			switch (m_wrap)
			{
				case BRACKETS:
					m_modifiedQuery += ')';
					break;
				case NONE:
				default:
					break;
			}
			m_wrapped = false;
		}

};

XapianEngine::XapianEngine(const string &database) :
	SearchEngineInterface()
{
	// We expect documents to have been converted to UTF-8 at indexing time
	m_charset = "UTF-8";

	// If the database name ends with a slash, remove it
	if (database[database.length() - 1] == '/')
	{
		m_databaseName = database.substr(0, database.length() - 1);
	}
	else
	{
		m_databaseName = database;
	}
}

XapianEngine::~XapianEngine()
{
}

Xapian::Query XapianEngine::parseQuery(Xapian::Database *pIndex, const QueryProperties &queryProps,
	const string &stemLanguage, DefaultOperator defaultOperator,
	string &correctedFreeQuery, bool minimal)
{
	Xapian::QueryParser parser;
	CJKVTokenizer tokenizer;
	string freeQuery(queryProps.getFreeQuery());
	unsigned int tokensCount = 1;
	bool diacriticSensitive = queryProps.getDiacriticSensitive();

	// Modifying the query is necessary if it's CJKV or diacritics are off
	if ((tokenizer.has_cjkv(freeQuery) == true) ||
		(diacriticSensitive == false))
	{
		QueryModifier handler(freeQuery,
			diacriticSensitive,
			tokenizer.get_ngram_size());

		tokenizer.tokenize(freeQuery, handler, true);

		tokensCount = handler.get_tokens_count();

		// We can disable stemming and spelling correction for pure CJKV queries
		string cjkvQuery(handler.get_modified_query(minimal));
#ifdef DEBUG
		clog << "XapianEngine::parseQuery: CJKV query is " << cjkvQuery << endl;
#endif

		// Do as if the user had given this as input
		freeQuery = cjkvQuery;
	}
	else
	{
		string::size_type spacePos = freeQuery.find(' ');
		while (spacePos != string::npos)
		{
			++tokensCount;

			if (spacePos + 1 >= freeQuery.length())
			{
				break;
			}

			// Next
			spacePos = freeQuery.find(' ', spacePos + 1);
		}
	}
#ifdef DEBUG
	clog << "XapianEngine::parseQuery: " << tokensCount << " tokens" << endl;
#endif

	if (pIndex != NULL)
	{
		// The database is required for wildcards and spelling
		parser.set_database(*pIndex);
	}

	// Set things up
	if ((minimal == false) &&
		(stemLanguage.empty() == false))
	{
		parser.set_stemmer(m_stemmer);
		parser.set_stemming_strategy(Xapian::QueryParser::STEM_SOME);

		// Don't bother loading the stopwords list if there's only one token
		if (tokensCount > 1)
		{
			FileStopper *pStopper = FileStopper::get_stopper(Languages::toCode(stemLanguage));
			if ((pStopper != NULL) &&
				(pStopper->get_stopwords_count() > 0))
			{
				parser.set_stopper(pStopper);
			}
		}
	}
	else
	{
#ifdef DEBUG
		clog << "XapianEngine::parseQuery: no stemming" << endl;
#endif
		parser.set_stemming_strategy(Xapian::QueryParser::STEM_NONE);
	}
	// What's the default operator ?
	if (defaultOperator == DEFAULT_OP_AND)
	{
		parser.set_default_op(Xapian::Query::OP_AND);
	}
	else
	{
		parser.set_default_op(Xapian::Query::OP_OR);
	}
	// Search across text body and title
	parser.add_prefix("", "");
	parser.add_prefix("", "S");
	// X prefixes should always include a colon
	parser.add_boolean_prefix("site", "H");
	parser.add_boolean_prefix("file", "P");
	parser.add_boolean_prefix("ext", "E");
	parser.add_prefix("title", "S");
	parser.add_boolean_prefix("url", "U");
	parser.add_boolean_prefix("dir", "XDIR:");
	parser.add_boolean_prefix("inurl", "XFILE:");
	parser.add_prefix("path", "XPATH:");
	parser.add_boolean_prefix("lang", "L");
	parser.add_boolean_prefix("type", "T");
	parser.add_boolean_prefix("class", "XCLASS:");
	parser.add_boolean_prefix("label", "XLABEL:");
	parser.add_boolean_prefix("tokens", "XTOK:");
	if (g_pMapper != NULL)
	{
		map<string, string> filters;

		g_pMapper->getBooleanFilters(filters);

		for (map<string, string>::const_iterator filterIter = filters.begin();
			filterIter != filters.end(); ++filterIter)
		{
			parser.add_boolean_prefix(filterIter->first, filterIter->second);
		}
	}

	// Date range
	Xapian::DateRangeProcessor dateProcessor(0);
	parser.add_rangeprocessor(&dateProcessor);

	// Size with a "b" suffix, ie 1024..10240b
	Xapian::NumberRangeProcessor sizeProcessor(2, "b", Xapian::RP_SUFFIX);
	parser.add_rangeprocessor(&sizeProcessor);

	// Time range
	TimeValueRangeProcessor timeProcessor(3);
	parser.add_rangeprocessor(&timeProcessor);

	// What type of query is this ?
	QueryProperties::QueryType type = queryProps.getType();
	if (type != QueryProperties::XAPIAN_QP)
	{
		// This isn't supported
		return Xapian::Query();
	}

	// Do some pre-processing : look for filters with quoted values
	string::size_type escapedFilterEnd = 0;
	string::size_type escapedFilterStart = freeQuery.find(":\"");
	while ((escapedFilterStart != string::npos) &&
		(escapedFilterStart < freeQuery.length() - 2))
	{
		escapedFilterEnd = freeQuery.find("\"", escapedFilterStart + 2);
		if (escapedFilterEnd == string::npos)
		{
			break;
		}

		string filterValue = freeQuery.substr(escapedFilterStart + 2, escapedFilterEnd - escapedFilterStart - 2);
		if (filterValue.empty() == false)
		{
			string escapedValue(Url::escapeUrl(filterValue));
			bool escapeValue = false, hashValue = false;

			// The value should be escaped and length-limited as done at indexing time
			checkFilter(freeQuery, escapedFilterStart, escapeValue, hashValue);

			if (escapeValue == false)
			{
				// No escaping
				escapedValue = filterValue;
			}
			if (hashValue == true)
			{
				// Partially hash if necessary
				escapedValue = XapianDatabase::limitTermLength(escapedValue, true);
			}
			else
			{
				escapedValue = XapianDatabase::limitTermLength(escapedValue);
			}

#ifdef DEBUG
			clog << "XapianEngine::parseQuery: escaping to " << escapedValue << endl;
#endif
			freeQuery.replace(escapedFilterStart + 1, escapedFilterEnd - escapedFilterStart,
				escapedValue);
			escapedFilterEnd = escapedFilterEnd + escapedValue.length() - filterValue.length();
		}
		else
		{
			// No value !
			freeQuery.replace(escapedFilterStart, escapedFilterEnd - escapedFilterStart + 1, ":");
			escapedFilterEnd -= 2;
		}
#ifdef DEBUG
		clog << "XapianEngine::parseQuery: replaced filter: " << freeQuery << endl;
#endif

		// Next
		escapedFilterStart = freeQuery.find(":\"", escapedFilterEnd);
	}

	// Parse the query string with all necessary options
	unsigned int flags = Xapian::QueryParser::FLAG_BOOLEAN|Xapian::QueryParser::FLAG_PHRASE|
		Xapian::QueryParser::FLAG_LOVEHATE|Xapian::QueryParser::FLAG_PURE_NOT;
	if (minimal == false)
	{
		flags |= Xapian::QueryParser::FLAG_WILDCARD;
		flags |= Xapian::QueryParser::FLAG_SPELLING_CORRECTION;
	}
	Xapian::Query parsedQuery = parser.parse_query(freeQuery, flags);
#ifdef DEBUG
	clog << "XapianEngine::parseQuery: query is " << parsedQuery.get_description() << endl;
#endif

	// Any limit on what documents should be searched ?
	if (m_limitDocuments.empty() == false)
	{
		Xapian::Query filterQuery(Xapian::Query::OP_OR,
			m_limitDocuments.begin(), m_limitDocuments.end());

		parsedQuery = Xapian::Query(Xapian::Query::OP_FILTER,
			parsedQuery, filterQuery);
#ifdef DEBUG
		clog << "XapianEngine::parseQuery: limited query is " << parsedQuery.get_description() << endl;
#endif
	}

	if (minimal == false)
	{
		// Any correction ?
		correctedFreeQuery = parser.get_corrected_query_string();
#ifdef DEBUG
		if (correctedFreeQuery.empty() == false)
		{
			clog << "XapianEngine::parseQuery: corrected spelling to: " << correctedFreeQuery << endl;
		}
#endif
	}

	return parsedQuery;
}

bool XapianEngine::queryDatabase(Xapian::Database *pIndex, Xapian::Query &query,
	const string &stemLanguage, unsigned int startDoc, const QueryProperties &queryProps)
{
	Timer timer;
	unsigned int maxResultsCount = queryProps.getMaximumResultsCount();
	bool completedQuery = false;

	if (pIndex == NULL)
	{
		return false;
	}

	// Start an enquire session on the database
	Xapian::Enquire enquire(*pIndex);

	timer.start();
	try
	{
		// Give the query object to the enquire session
		enquire.set_query(query);
		// How should results be sorted ?
		if (queryProps.getSortOrder() == QueryProperties::RELEVANCE)
		{
			// By relevance, then date
			enquire.set_sort_by_relevance_then_value(4, true);
#ifdef DEBUG
			clog << "XapianEngine::queryDatabase: sorting by relevance first" << endl;
#endif
		}
		else if (queryProps.getSortOrder() == QueryProperties::DATE_DESC)
		{
			// By date, and then by relevance
			enquire.set_docid_order(Xapian::Enquire::DONT_CARE);
			enquire.set_sort_by_value_then_relevance(4, true);
#ifdef DEBUG
			clog << "XapianEngine::queryDatabase: sorting by date and time desc" << endl;
#endif
		}
		else if (queryProps.getSortOrder() == QueryProperties::DATE_ASC)
		{
			// By date, and then by relevance
			enquire.set_docid_order(Xapian::Enquire::DONT_CARE);
			enquire.set_sort_by_value_then_relevance(5, true);
#ifdef DEBUG
			clog << "XapianEngine::queryDatabase: sorting by date and time asc" << endl;
#endif
		}
		else if (queryProps.getSortOrder() == QueryProperties::SIZE_DESC)
		{
			// By date, and then by relevance
			enquire.set_docid_order(Xapian::Enquire::DONT_CARE);
			enquire.set_sort_by_value_then_relevance(2, true);
#ifdef DEBUG
			clog << "XapianEngine::queryDatabase: sorting by size asc" << endl;
#endif
		}

		// Collapse results ?
		if (g_pMapper != NULL)
		{
			unsigned int valueNumber;

			if (g_pMapper->collapseOnValue(valueNumber) == true)
			{
				enquire.set_collapse_key(valueNumber, 1);
			}
		}

		// Get the top results of the query
		Xapian::MSet matches = enquire.get_mset(startDoc, maxResultsCount, (2 * maxResultsCount) + 1);
		m_resultsCountEstimate = matches.get_matches_estimated();
		if (matches.empty() == false)
		{
#ifdef DEBUG
			clog << "XapianEngine::queryDatabase: found " << matches.size() << "/" << maxResultsCount
				<< " results found from position " << startDoc << endl;
			clog << "XapianEngine::queryDatabase: estimated " << matches.get_matches_lower_bound()
				<< "/" << m_resultsCountEstimate << "/" << matches.get_matches_upper_bound()
				<< ", " << matches.get_description() << endl;
#endif

			// Get the results
			for (Xapian::MSetIterator mIter = matches.begin(); mIter != matches.end(); ++mIter)
			{
				Xapian::docid docId = *mIter;
				Xapian::Document doc(mIter.get_document());
				bool hasCJKV = false;

				if (docId <= 0)
				{
#ifdef DEBUG
					clog << "XapianEngine::queryDatabase: bogus document ID " << docId << endl;
#endif
					continue;
				}

				DocumentInfo thisResult;
				string docText(getDocumentText(pIndex, docId, hasCJKV));
				unsigned int flags = Xapian::MSet::SNIPPET_BACKGROUND_MODEL|Xapian::MSet::SNIPPET_EXHAUSTIVE;

				if (hasCJKV == true)
				{
					flags |= Xapian::MSet::SNIPPET_CJK_NGRAM;
				}
				if (stemLanguage.empty() == true)
				{
					thisResult.setExtract(matches.snippet(docText, 300, Xapian::Stem(), flags));
				}
				else
				{
					thisResult.setExtract(matches.snippet(docText, 300, m_stemmer, flags));
				}
				thisResult.setScore((float)mIter.get_percent());

#ifdef DEBUG
				clog << "XapianEngine::queryDatabase: found document ID " << docId << endl;
#endif
				XapianDatabase::recordToProps(doc.get_data(), &thisResult);
				// XapianDatabase stored the language in English
				thisResult.setLanguage(Languages::toLocale(thisResult.getLanguage()));

				string url(thisResult.getLocation());
				if (url.empty() == true)
				{
					// Hmmm this shouldn't be empty...
					// Use this instead, even though the document isn't cached in the index
					thisResult.setLocation(XapianDatabase::buildUrl(m_databaseName, docId));
				}

				// We don't know the index ID, just the document ID
				thisResult.setIsIndexed(0, docId);

				// Add this result
				m_resultsList.push_back(thisResult);
			}
		}

		completedQuery = true;
	}
	catch (const Xapian::Error &error)
	{
		clog << "Couldn't run query: " << error.get_type() << ": " << error.get_msg() << endl;
	}
	clog << "Ran query \"" << queryProps.getFreeQuery() << "\" in " << timer.stop() << " ms" << endl;

	try
	{
		m_expandTerms.clear();

		// Expand the query ?
		if (m_expandDocuments.empty() == false)
		{
			Xapian::RSet expandDocs;

			for (set<string>::const_iterator docIter = m_expandDocuments.begin();
				docIter != m_expandDocuments.end(); ++docIter)
			{
				string uniqueTerm(string("U") + XapianDatabase::limitTermLength(Url::escapeUrl(Url::canonicalizeUrl(*docIter)), true));

				// Only one document may have this term
				Xapian::PostingIterator postingIter = pIndex->postlist_begin(uniqueTerm);
				if (postingIter != pIndex->postlist_end(uniqueTerm))
				{
					expandDocs.add_document(*postingIter);
				}
			}
#ifdef DEBUG
			clog << "XapianEngine::queryDatabase: expand from " << expandDocs.size() << " documents" << endl;
#endif

			// Get 10 non-prefixed terms
			string allowedPrefixes("RS");
			TermDecider expandDecider(pIndex, ((stemLanguage.empty() == true) ? NULL : &m_stemmer),
				FileStopper::get_stopper(Languages::toCode(stemLanguage)),
				allowedPrefixes, query);
			Xapian::ESet expandTerms = enquire.get_eset(10, expandDocs, &expandDecider);
#ifdef DEBUG
			clog << "XapianEngine::queryDatabase: " << expandTerms.size() << " expand terms" << endl;
#endif
			for (Xapian::ESetIterator termIter = expandTerms.begin();
				termIter != expandTerms.end(); ++termIter)
			{
				string expandTerm(*termIter);
				char firstChar = expandTerm[0];

				// Is this prefixed ?
				if (allowedPrefixes.find(firstChar) != string::npos)
				{
					expandTerm.erase(0, 1);
				}

				m_expandTerms.insert(expandTerm);
			}
		}
	}
	catch (const Xapian::Error &error)
	{
		clog << "Couldn't run query: " << error.get_type() << ": " << error.get_msg() << endl;
	}

	// Be tolerant of errors as long as we got some results
	if ((completedQuery == true) ||
		(m_resultsList.empty() == false))
	{
		return true;
	}

	return false;
}

/// Frees all objects.
void XapianEngine::freeAll(void)
{
	FileStopper::free_stopper();
}

string XapianEngine::getDocumentText(Xapian::Database *pIndex,
	Xapian::docid docId, bool &hasCJKV)
{
	map<Xapian::termpos, string> wordsBuffer;
	CJKVTokenizer tokenizer;

	try
	{
		// Go through the position list of each term
		for (Xapian::TermIterator termIter = pIndex->termlist_begin(docId);
			termIter != pIndex->termlist_end(docId); ++termIter)
		{
			string termName(*termIter);

			// Skip prefixed terms
			if (isupper((int)termName[0]) != 0)
			{
				if (termName == "XTOK:CJKV")
				{
					hasCJKV = true;
				}
				continue;
			}
			// Skip multi-character CJKV terms
			if ((tokenizer.has_cjkv(termName) == true) &&
				(termName.length() > 4))
			{
				continue;
			}

			for (Xapian::PositionIterator positionIter = pIndex->positionlist_begin(docId, termName);
				positionIter != pIndex->positionlist_end(docId, termName); ++positionIter)
			{
				Xapian::termpos termPos = *positionIter;

				// If several terms exist at this position, prefer the shortest one
				map<Xapian::termpos, string>::const_iterator wordIter = wordsBuffer.find(termPos);
				if ((wordIter == wordsBuffer.end()) ||
					(wordIter->second.length() > termName.length()))
				{
					wordsBuffer[termPos] = termName;
				}
			}
		}
	}
	catch (const Xapian::Error &error)
	{
#ifdef DEBUG
		clog << "XapianEngine::getDocumentText: " << error.get_msg() << endl;
#endif
	}

	string docText;

	for (map<Xapian::termpos, string>::const_iterator wordIter = wordsBuffer.begin();
		wordIter != wordsBuffer.end(); ++wordIter)
	{
		docText += " ";
		docText += wordIter->second;
	}

	return docText;
}

//
// Implementation of SearchEngineInterface
//

/// Sets the set of documents to limit to.
bool XapianEngine::setLimitSet(const set<string> &docsSet)
{
	for (set<string>::const_iterator docIter = docsSet.begin();
		docIter != docsSet.end(); ++docIter)
	{
		string urlFilter("U");

		// Escape and hash
		urlFilter += XapianDatabase::limitTermLength(Url::escapeUrl(*docIter), true);
		m_limitDocuments.insert(urlFilter);
	}
#ifdef DEBUG
	clog << "XapianEngine::setLimitSet: " << m_limitDocuments.size() << " documents" << endl;
#endif

	return true;
}

/// Sets the set of documents to expand from.
bool XapianEngine::setExpandSet(const set<string> &docsSet)
{
	copy(docsSet.begin(), docsSet.end(),
		inserter(m_expandDocuments, m_expandDocuments.begin()));
#ifdef DEBUG
	clog << "XapianEngine::setExpandSet: " << m_expandDocuments.size() << " documents" << endl;
#endif

	return true;
}

/// Runs a query; true if success.
bool XapianEngine::runQuery(QueryProperties& queryProps,
	unsigned int startDoc)
{
	string stemLanguage(Languages::toEnglish(queryProps.getStemmingLanguage()));

	// Clear the results list
	m_resultsList.clear();
	m_resultsCountEstimate = 0;
	m_correctedFreeQuery.clear();

	if (queryProps.isEmpty() == true)
	{
#ifdef DEBUG
		clog << "XapianEngine::runQuery: query is empty" << endl;
#endif
		return false;
	}

	XapianDatabase *pDatabase = XapianDatabaseFactory::getDatabase(m_databaseName, true);
	if (pDatabase == NULL)
	{
		clog << "Couldn't get index " << m_databaseName << endl;
		return false;
	}

	if ((stemLanguage.empty() == false) &&
		(stemLanguage != "unknown"))
	{
#ifdef DEBUG
		clog << "XapianEngine::runQuery: " << stemLanguage << " stemming" << endl;
#endif
		try
		{
			m_stemmer = Xapian::Stem(StringManip::toLowerCase(stemLanguage));
		}
		catch (const Xapian::Error &error)
		{
			clog << "Couldn't create stemmer: " << error.get_type() << ": " << error.get_msg() << endl;
		}
	}

	// Get the latest revision...
	pDatabase->reopen();
	Xapian::Database *pIndex = pDatabase->readLock();
	try
	{
		unsigned int searchStep = 1;

		// Searches are run in this order :
		// 1. no stemming, exact matches only
		// 2. stem terms if a language is defined for the query
		Xapian::Query fullQuery = parseQuery(pIndex, queryProps, "",
			m_defaultOperator, m_correctedFreeQuery);
		while (fullQuery.empty() == false)
		{
			// Query the database
			if (queryDatabase(pIndex, fullQuery, stemLanguage, startDoc, queryProps) == false)
			{
				break;
			}

			if (m_resultsList.empty() == true)
			{
				// The search did succeed but didn't return anything
				if ((searchStep == 1) &&
					(stemLanguage.empty() == false))
				{
#ifdef DEBUG
					clog << "XapianEngine::runQuery: trying again with stemming" << endl;
#endif
					fullQuery = parseQuery(pIndex, queryProps, stemLanguage,
						m_defaultOperator, m_correctedFreeQuery);
					++searchStep;
					continue;
				}
			}
			else
			{
				// We have results, don't bother about correcting the query
				m_correctedFreeQuery.clear();
			}

			pDatabase->unlock();
			return true;
		}
	}
	catch (const Xapian::Error &error)
	{
		clog << "Couldn't run query: " << error.get_type() << ": " << error.get_msg() << endl;
	}
	pDatabase->unlock();

	return false;
}
