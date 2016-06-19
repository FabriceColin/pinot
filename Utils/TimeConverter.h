/*
 *  Copyright 2005-2014 Fabrice Colin
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

#ifndef _TIME_CONVERTER_H
#define _TIME_CONVERTER_H

#include <time.h>
#include <string>

#include "Visibility.h"

/// This class handles time conversions.
class PINOT_EXPORT TimeConverter
{
	public:
		/// Inverse of gmtime().
		static time_t timegm(struct tm *tm);

		/// Converts into an RFC 822 timestamp.
		static std::string toTimestamp(time_t aTime, bool inGMTime = false);

		/// Converts from a RFC 822 timestamp.
		static time_t fromTimestamp(const std::string &timestamp);

		/// Converts to a YYYYMMDD-formatted string.
		static std::string toYYYYMMDDString(int year, int month, int day);

		/// Converts from a YYYYMMDD-formatted string.
		static time_t fromYYYYMMDDString(const std::string &yyyymmdd, bool inGMTime = false);

		/// Converts to a HHMMSS-formatted string.
		static std::string toHHMMSSString(int hours, int minutes, int seconds);

		/// Converts from a HHMMSS-formatted string.
		static time_t fromHHMMSSString(const std::string &hhmmss, bool inGMTime = false);

		typedef enum { DATE_EUROPE = 0, DATE_JAPAN } DateFormat;

		/// Converts into a longer human-readable date.
		static std::string toNormalDate(time_t aTime, DateFormat format);

	protected:
		TimeConverter();

	private:
		TimeConverter(const TimeConverter &other);
		TimeConverter& operator=(const TimeConverter& other);

};

#endif // _TIME_CONVERTER_H
