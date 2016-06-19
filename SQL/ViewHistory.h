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

#ifndef _VIEW_HISTORY_H
#define _VIEW_HISTORY_H

#include <time.h>
#include <string>

#include "SQLiteBase.h"

/// Manages view history.
class ViewHistory : public SQLiteBase
{
	public:
		ViewHistory(const std::string &database);
		virtual ~ViewHistory();

		/// Creates the ViewHistory table in the database.
		static bool create(const std::string &database);

		/// Inserts an URL.
		bool insertItem(const std::string &url);

		/// Checks if an URL is in the history.
		bool hasItem(const std::string &url);

		/// Returns the number of items.
		unsigned int getItemsCount(void);

		/// Deletes an URL.
		bool deleteItem(const std::string &url);

		/// Expires items older than the given date.
		bool expireItems(time_t expiryDate);

	private:
		ViewHistory(const ViewHistory &other);
		ViewHistory &operator=(const ViewHistory &other);

};

#endif // _VIEW_HISTORY_H
