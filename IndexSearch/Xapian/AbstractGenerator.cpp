/*
 *  Copyright 2005-2013 Fabrice Colin
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

#include <string.h>
#include <ctype.h>
#include <sys/time.h>
#include <glib.h>
#include <map>
#include <set>
#include <algorithm>
#include <iostream>
#include <utility>

#include "Timer.h"
#include "AbstractGenerator.h"
#include "CJKVTokenizer.h"

using std::clog;
using std::clog;
using std::endl;
using std::string;
using std::vector;
using std::map;
using std::set;
using std::find;
using namespace Dijon;

unsigned int AbstractGenerator::m_maxSeedTerms = 5;
unsigned int AbstractGenerator::m_minTermPositions = 10;

AbstractGenerator::PositionWindow::PositionWindow() :
	m_backWeight(1),
	m_forwardWeight(1)
{
}

AbstractGenerator::PositionWindow::~PositionWindow()
{
}

AbstractGenerator::AbstractGenerator(const Xapian::Database *pIndex,
	unsigned int wordsCount) :
	m_pIndex(pIndex),
	m_wordsCount(wordsCount)
{
}

AbstractGenerator::~AbstractGenerator()
{
}

/// Attempts to generate an abstract of wordsCount words.
string AbstractGenerator::generateAbstract(Xapian::docid docId,
	const vector<string> &seedTerms)
{
	CJKVTokenizer tokenizer;
	map<Xapian::termpos, PositionWindow> abstractWindows;
	set<Xapian::termpos> seedTermsPositions;
	map<Xapian::termpos, string> wordsBuffer;
	string summary;
	Xapian::termpos bestPosition = 0, startPosition = 0;
	unsigned int seedTermsCount = 0, bestWeight = 0;
	bool topTerm = true;
	bool wasCJKV = false;

	if (m_pIndex == NULL)
	{
		return "";
	}

#ifdef DEBUG
	Timer timer;
	timer.start();
#endif
	for (vector<string>::const_iterator termIter = seedTerms.begin();
		termIter != seedTerms.end(); ++termIter)
	{
		string termName(*termIter);

#ifdef DEBUG
		clog << "AbstractGenerator::generateAbstract: term " << termName << endl;
#endif
		try
		{
			set<Xapian::termpos> positions;

			// Go through that term's position list in the document
			for (Xapian::PositionIterator positionIter = m_pIndex->positionlist_begin(docId, termName);
				positionIter != m_pIndex->positionlist_end(docId, termName); ++positionIter)
			{
				Xapian::termpos termPos = *positionIter;

				positions.insert(termPos);
				seedTermsPositions.insert(termPos);

				if (seedTermsCount >= m_maxSeedTerms)
				{
					// Enough windows already 
					continue;
				}

				// Take all the top term's positions into account, and some of 
				// the other terms' too if the minimum number is not reached
				if ((m_minTermPositions > abstractWindows.size()) ||
					(topTerm == true))
				{
					abstractWindows[termPos] = PositionWindow();
				}

				// Look for other terms close to that position
				for (map<Xapian::termpos, PositionWindow>::iterator winIter = abstractWindows.begin();
					winIter != abstractWindows.end(); ++winIter)
				{
					if (positions.find(winIter->first) != positions.end())
					{
						// This is for the same term at another position
						continue;
					}

					// Is this within the number of words we are interested in ?
					if (winIter->first > termPos)
					{
						if (winIter->first - termPos <= m_wordsCount)
						{
							++winIter->second.m_backWeight;
						}
					}
					else if (termPos - winIter->first <= m_wordsCount)
					{
						++winIter->second.m_forwardWeight;
					}
				}
			}

			++seedTermsCount;
			topTerm = false;
		}
		catch (const Xapian::Error &error)
		{
#ifdef DEBUG
			clog << "AbstractGenerator::generateAbstract: " << error.get_msg() << endl;
#endif
			continue;
		}

#ifdef DEBUG
		clog << "AbstractGenerator::generateAbstract: " << abstractWindows.size()
			<< " windows, " << seedTermsCount << " terms" << endl;
#endif
	}

	// Go through positions and find out which one has the largest
	// number of terms nearby
	for (map<Xapian::termpos, PositionWindow>::iterator winIter = abstractWindows.begin();
		winIter != abstractWindows.end(); ++winIter)
	{
#ifdef DEBUG
		clog << "AbstractGenerator::generateAbstract: position " << winIter->first
			<< " weighs " << winIter->second.m_backWeight 
			<< "/" << winIter->second.m_forwardWeight << endl;
#endif
		if (bestWeight < winIter->second.m_forwardWeight)
		{
			bestPosition = startPosition = winIter->first;
			bestWeight = winIter->second.m_forwardWeight;
		}
		if (bestWeight < winIter->second.m_backWeight)
		{
			bestPosition = winIter->first;
			startPosition = 0;
			if (bestPosition > m_wordsCount)
			{
				startPosition = bestPosition - m_wordsCount;
			}
			bestWeight = winIter->second.m_backWeight;
		}
	}
#ifdef DEBUG
	clog << "AbstractGenerator::generateAbstract: best position is "
		<< bestPosition << ":" << startPosition << " with weight " << bestWeight << endl;
#endif

	try
	{
		// Go through the position list of each term
		for (Xapian::TermIterator termIter = m_pIndex->termlist_begin(docId);
			termIter != m_pIndex->termlist_end(docId); ++termIter)
		{
			string termName(*termIter);

			// Skip prefixed terms
			if (isupper((int)termName[0]) != 0)
			{
				continue;
			}
			// Skip multi-character CJKV terms
			if ((tokenizer.has_cjkv(termName) == true) &&
				(termName.length() > 4))
			{
				continue;
			}

			for (Xapian::PositionIterator positionIter = m_pIndex->positionlist_begin(docId, termName);
				positionIter != m_pIndex->positionlist_end(docId, termName); ++positionIter)
			{
				Xapian::termpos termPos = *positionIter;

				// ...and get those that fall in the abstract window
				if ((startPosition <= termPos + 1) &&
					(termPos < startPosition + m_wordsCount))
				{
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
	}
	catch (const Xapian::Error &error)
	{
#ifdef DEBUG
		clog << "AbstractGenerator::generateAbstract: " << error.get_msg() << endl;
#endif
		return "";
	}

	// Generate the abstract
	for (map<Xapian::termpos, string>::iterator wordIter = wordsBuffer.begin();
		wordIter != wordsBuffer.end(); ++wordIter)
	{
		gchar *pEscToken = NULL;

		if (g_utf8_validate(wordIter->second.c_str(), -1, NULL))
		{
			pEscToken = g_markup_escape_text(wordIter->second.c_str(), -1);
		}
		if (pEscToken == NULL)
		{
			continue;
		}

		bool isCJKV = tokenizer.has_cjkv(wordIter->second);
		if ((summary.empty() == false) &&
			((wasCJKV != isCJKV) || (isCJKV == false)))
		{
			summary += " ";
		}

		// Is this a seed term ?
		if (seedTermsPositions.find(wordIter->first) != seedTermsPositions.end())
		{
			summary += "<b>";
			summary += wordIter->second;
			summary += "</b>";
		}
		else
		{
			summary += wordIter->second;
		}

		wasCJKV = isCJKV;
	}
#ifdef DEBUG
	clog << "AbstractGenerator::generateAbstract: summarized document "
		<< docId << " in " << timer.stop() << " ms" << endl;
#endif

	return summary;
}
