/*
 *  Copyright 2005-2009 Fabrice Colin
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

#ifndef _META_DATA_BACKUP_H
#define _META_DATA_BACKUP_H

#include <time.h>
#include <string>
#include <set>

#include "DocumentInfo.h"
#include "SQLiteBase.h"

/// Handles the MetaDataBackup table.
class MetaDataBackup : public SQLiteBase
{
	public:
		MetaDataBackup(const std::string &database);
		virtual ~MetaDataBackup();

		/// Creates the MetaDataBackup table in the database.
		static bool create(const std::string &database);

		/// Adds an item.
		bool addItem(const DocumentInfo &docInfo, DocumentInfo::SerialExtent extent);

		/// Gets an item.
		bool getItem(DocumentInfo &docInfo, DocumentInfo::SerialExtent extent);

		/// Gets items.
		bool getItems(const std::string &likeUrl,
			std::set<std::string> &urls,
			unsigned long min, unsigned long max);

		/// Deletes an item.
		bool deleteItem(const DocumentInfo &docInfo, DocumentInfo::SerialExtent extent,
			const std::string &value = "");

		/// Deletes a label.
		bool deleteLabel(const std::string &value);

        protected:
		bool setAttribute(const DocumentInfo &docInfo,
			const std::string &name, const std::string &value,
			bool noXAttr = false);

		bool getAttribute(const DocumentInfo &docInfo,
			const std::string &name, std::string &value,
			bool noXAttr = false);

		bool getAttributes(const DocumentInfo &docInfo,
			const std::string &name, std::set<std::string> &values);

		bool removeAttribute(const DocumentInfo &docInfo,
			const std::string &name,
			bool noXAttr = false, bool likeName = false);

        private:
		MetaDataBackup(const MetaDataBackup &other);
		MetaDataBackup &operator=(const MetaDataBackup &other);

};

#endif // _META_DATA_BACKUP_H
