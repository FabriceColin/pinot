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

#ifndef _ACTION_QUEUE_H
#define _ACTION_QUEUE_H

#include <time.h>
#include <string>

#include "DocumentInfo.h"
#include "SQLiteBase.h"

/// Handles the ActionQueue table.
class ActionQueue : public SQLiteBase
{
	public:
		ActionQueue(const std::string &database, const std::string queueId);
		virtual ~ActionQueue();

		/// Creates the ActionQueue table in the database.
		static bool create(const std::string &database);

		typedef enum { INDEX = 0, UNINDEX } ActionType;

		/// Pushes an item.
		bool pushItem(ActionType type, const DocumentInfo &docInfo);

		/// Pops and deletes the oldest item.
		bool popItem(ActionType &type, DocumentInfo &docInfo);

		/// Returns the number of items of a particular type.
		unsigned int getItemsCount(ActionType type);

		/// Deletes all items under a given URL.
		bool deleteItems(const std::string &url);

		/// Expires items older than the given date.
		bool expireItems(time_t expiryDate);

        protected:
		std::string m_queueId;

		bool getOldestItem(ActionType &type, DocumentInfo &docInfo);

		static std::string typeToText(ActionType type);

		static ActionType textToType(const std::string &text);

        private:
		ActionQueue(const ActionQueue &other);
		ActionQueue &operator=(const ActionQueue &other);

};

#endif // _ACTION_QUEUE_H
