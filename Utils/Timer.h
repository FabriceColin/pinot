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

#ifndef _TIMER_H
#define _TIMER_H

#include <time.h>
#include <sys/time.h>

#include "Visibility.h"

/// A timer with microseconds precision.
class PINOT_EXPORT Timer
{
	public:
		Timer();
		Timer(const Timer &other);
		virtual ~Timer();

		Timer &operator=(const Timer &other);

		/// Starts the timer.
		void start(void);

		/// Stops the timer and returns the number of milliseconds elapsed.
		long stop(void);

	protected:
		struct timeval m_start;
		struct timeval m_stop;

};

#endif // _TIMER_H
