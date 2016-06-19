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

#ifndef _ABSTRACT_GENERATOR_H
#define _ABSTRACT_GENERATOR_H

#include <string>
#include <vector>

#include <xapian.h>

/// Generates abstracts for indexed documents.
class AbstractGenerator
{
	public:
		AbstractGenerator(const Xapian::Database *pIndex, unsigned int wordsCount);
		virtual ~AbstractGenerator();

		/// Attempts to generate an abstract of wordsCount words.
		std::string generateAbstract(Xapian::docid docId,
			const std::vector<std::string> &seedTerms);

	protected:
		static unsigned int m_maxSeedTerms;
		static unsigned int m_minTermPositions;
		const Xapian::Database *m_pIndex;
		unsigned int m_wordsCount;

		class PositionWindow
		{
			public:
				PositionWindow();
				~PositionWindow();

				unsigned int m_backWeight;
				unsigned int m_forwardWeight;

		};

	private:
		AbstractGenerator(const AbstractGenerator &other);
		AbstractGenerator &operator=(const AbstractGenerator &other);

};

#endif // _ABSTRACT_GENERATOR_H
