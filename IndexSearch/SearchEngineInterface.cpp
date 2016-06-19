/*
 *  Copyright 2005,2006 Fabrice Colin
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

#include <iostream>

#include "Document.h"
#include "StringManip.h"
#include "Url.h"
#include "SearchEngineInterface.h"

using std::clog;
using std::endl;

SearchEngineInterface::SearchEngineInterface() :
	m_defaultOperator(DEFAULT_OP_AND),
	m_resultsCountEstimate(0)
{
}

SearchEngineInterface::~SearchEngineInterface()
{
}

/// Sets whether AND is the default operator.
void SearchEngineInterface::setDefaultOperator(DefaultOperator op)
{
	m_defaultOperator = op;
}

/// Sets the set of documents to limit to.
bool SearchEngineInterface::setLimitSet(const set<string> &docsSet)
{
	// Not all engines support this
	return false;
}

/// Sets the set of documents to expand from.
bool SearchEngineInterface::setExpandSet(const set<string> &docsSet)
{
	// Not all engines support this
	return false;
}

/// Returns the results for the previous query.
const vector<DocumentInfo> &SearchEngineInterface::getResults(void) const
{
	return m_resultsList;
}

/// Returns an estimate of the total number of results for the previous query.
unsigned int SearchEngineInterface::getResultsCountEstimate(void) const
{
	return m_resultsCountEstimate;
}

/// Returns the charset for the previous query's results.
string SearchEngineInterface::getResultsCharset(void) const
{
	return m_charset;
}

/// Suggests a spelling correction.
string SearchEngineInterface::getSpellingCorrection(void) const
{
	return m_correctedFreeQuery;
}

/// Returns expand terms from the previous query.
const set<string> &SearchEngineInterface::getExpandTerms(void) const
{
	return m_expandTerms;
}
