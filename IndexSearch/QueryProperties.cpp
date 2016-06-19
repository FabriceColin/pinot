/*
 *  Copyright 2005-2010 Fabrice Colin
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

#include <ctype.h>
#include <set>
#include <iostream>
#include <algorithm>
#include <utility>

#include "Document.h"
#include "StringManip.h"
#include "CJKVTokenizer.h"
#include "QueryProperties.h"

class FilterRemover : public Dijon::CJKVTokenizer::TokensHandler
{
        public:
                FilterRemover(const string &freeQuery) :
			Dijon::CJKVTokenizer::TokensHandler(),
			m_freeQuery(freeQuery)
		{
		}

		virtual ~FilterRemover()
		{
		}

		virtual bool handle_token(const string &tok, bool is_cjkv)
		{
			if (tok.empty() == true)
			{
				return false;
			}

			// Is this CJKV ?
			if (is_cjkv == false)
			{
				if ((tok.find(':') != string::npos) ||
					(tok.find("..") != string::npos))
				{
					string::size_type tokPosStart = m_freeQuery.find(tok);

					// It's a filter or a range, remove it
					if (tokPosStart != string::npos)
					{
						string::size_type tokPosEnd = m_freeQuery.find(" ", tokPosStart);

						if (tokPosEnd != string::npos)
						{
							m_freeQuery.erase(tokPosStart, tokPosEnd - tokPosStart);
						}
						else
						{
							m_freeQuery.erase(tokPosStart);
						}
					}

					return true;
				}
			}

			return true;
		}

		string m_freeQuery;

};

class SetTokensHandler : public Dijon::CJKVTokenizer::TokensHandler
{
	public:
		SetTokensHandler(set<string> &tokens) :
			Dijon::CJKVTokenizer::TokensHandler(),
			m_tokens(tokens)
		{
		}

		virtual ~SetTokensHandler()
		{
		}

		virtual bool handle_token(const string &tok, bool is_cjkv)
		{
			m_tokens.insert(tok);
			return true;
		}

	protected:
		set<string> &m_tokens;

};

QueryProperties::QueryProperties() :
	m_type(XAPIAN_QP),
	m_order(RELEVANCE),
	m_diacriticSensitive(false),
	m_resultsCount(10),
	m_indexResults(NOTHING),
	m_modified(false)
{
}

QueryProperties::QueryProperties(const string &name, const string &freeQuery,
	QueryType type) :
	m_name(name),
	m_type(type),
	m_order(RELEVANCE),
	m_diacriticSensitive(false),
	m_freeQuery(freeQuery),
	m_resultsCount(10),
	m_indexResults(NOTHING),
	m_modified(false)
{
	removeFilters();
}

QueryProperties::QueryProperties(const QueryProperties &other) :
	m_name(other.m_name),
	m_type(other.m_type),
	m_order(other.m_order),
	m_language(other.m_language),
	m_diacriticSensitive(other.m_diacriticSensitive),
	m_freeQuery(other.m_freeQuery),
	m_freeQueryWithoutFilters(other.m_freeQueryWithoutFilters),
	m_resultsCount(other.m_resultsCount),
	m_indexResults(other.m_indexResults),
	m_labelName(other.m_labelName),
	m_modified(other.m_modified)
{
}

QueryProperties::~QueryProperties()
{
}

QueryProperties &QueryProperties::operator=(const QueryProperties &other)
{
	if (this != &other)
	{
		m_name = other.m_name;
		m_type = other.m_type;
		m_order = other.m_order;
		m_language = other.m_language;
		m_diacriticSensitive = other.m_diacriticSensitive;
		m_freeQuery = other.m_freeQuery;
		m_freeQueryWithoutFilters = other.m_freeQueryWithoutFilters;
		m_resultsCount = other.m_resultsCount;
		m_indexResults = other.m_indexResults;
		m_labelName = other.m_labelName;
		m_modified = other.m_modified;
	}

	return *this;
}

bool QueryProperties::operator==(const QueryProperties &other) const
{
	if (m_name == other.m_name)
	{
		return true;
	}

	return false;
}

bool QueryProperties::operator<(const QueryProperties &other) const
{
	if (m_name < other.m_name)
	{
		return true;
	}

	return false;
}

void QueryProperties::removeFilters(void)
{
	m_freeQueryWithoutFilters.clear();

	StringManip::trimSpaces(m_freeQuery);
	if (m_freeQuery.empty() == true)
	{
		return;
	}

	if (m_type != XAPIAN_QP)
	{
		m_freeQueryWithoutFilters = m_freeQuery.substr(0, min(20, (int)m_freeQuery.length()));
		return;
	}

	// Remove carriage returns
	string noCR(StringManip::replaceSubString(m_freeQuery, "\r\n", "\n"));
	// If there's a line break right after an hyphen, remove both
	string dehyphenedOnNL(StringManip::replaceSubString(noCR, "-\n", ""));
	// ... and replace line breaks with spaces
	m_freeQuery = StringManip::replaceSubString(dehyphenedOnNL, "\n", " ");

	Dijon::CJKVTokenizer tokenizer;
	FilterRemover handler(m_freeQuery);

	tokenizer.tokenize(m_freeQuery, handler, true);
	m_freeQueryWithoutFilters = handler.m_freeQuery;
	StringManip::trimSpaces(m_freeQueryWithoutFilters);
}

/// Sets the name.
void QueryProperties::setName(const string &name)
{
	m_name = name;
}

/// Gets the name.
string QueryProperties::getName(void) const
{
	return m_name;
}

/// Sets the type.
void QueryProperties::setType(QueryType type)
{
	m_type = type;
}

/// Gets the type.
QueryProperties::QueryType QueryProperties::getType(void) const
{
	return m_type;
}

/// Sets the sort order.
void QueryProperties::setSortOrder(SortOrder order)
{
	m_order = order;
}

/// Gets the sort order.
QueryProperties::SortOrder QueryProperties::getSortOrder(void) const
{
	return m_order;
}

/// Sets the language to use for stemming.
void QueryProperties::setStemmingLanguage(const string &language)
{
	m_language = language;
}

/// Gets the language to use for stemming.
string QueryProperties::getStemmingLanguage(void) const
{
	return m_language;
}

/// Sets whether the query is sensitive to diacritics.
void QueryProperties::setDiacriticSensitive(bool sensitive)
{
	m_diacriticSensitive = sensitive;
}

/// Gets whether the query is sensitive to diacritics.
bool QueryProperties::getDiacriticSensitive(void) const
{
	return m_diacriticSensitive;
}

/// Sets the query string.
void QueryProperties::setFreeQuery(const string &freeQuery)
{
	m_freeQuery = freeQuery;
	removeFilters();
}

/// Gets the query string.
string QueryProperties::getFreeQuery(bool withoutFilters) const
{
	if (withoutFilters == false)
	{
		return m_freeQuery;
	}

#ifdef DEBUG
	clog << "QueryProperties::getFreeQuery: " << m_freeQueryWithoutFilters << endl;
#endif
	return m_freeQueryWithoutFilters;
}

/// Sets the maximum number of results.
void QueryProperties::setMaximumResultsCount(unsigned int count)
{
	m_resultsCount = count;
}

/// Gets the maximum number of results.
unsigned int QueryProperties::getMaximumResultsCount(void) const
{
	return m_resultsCount;
}

/// Sets whether results should be indexed.
void QueryProperties::setIndexResults(IndexWhat indexResults)
{
	m_indexResults = indexResults;
}

/// Gets whether results should be indexed
QueryProperties::IndexWhat QueryProperties::getIndexResults(void) const
{
	return m_indexResults;
}

/// Sets the name of the label to use for indexed documents.
void QueryProperties::setLabelName(const string &labelName)
{
	m_labelName = labelName;
}

/// Gets the name of the label to use for indexed documents.
string QueryProperties::getLabelName(void) const
{
	return m_labelName;
}

/// Sets whether the query was modified in some way.
void QueryProperties::setModified(bool isModified)
{
	m_modified = isModified;
}

/// Gets whether the query was modified in some way.
bool QueryProperties::getModified(void) const
{
	return m_modified;
}

/// Returns the query's terms.
void QueryProperties::getTerms(set<string> &terms) const
{
	Dijon::CJKVTokenizer tokenizer;
	SetTokensHandler handler(terms);

	terms.clear();
	tokenizer.tokenize(m_freeQueryWithoutFilters, handler);
}

/// Returns whether the query is empty.
bool QueryProperties::isEmpty() const
{
	if (m_freeQuery.empty() == true)
	{
		return true;
	}

	return false;
}
