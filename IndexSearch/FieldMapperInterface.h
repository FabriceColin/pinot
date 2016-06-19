/*
 *  Copyright 2012-2013 Fabrice Colin
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
 
#ifndef _FIELD_MAPPER_INTERFACE_H
#define _FIELD_MAPPER_INTERFACE_H

#include <string>
#include <vector>
#include <map>
#include <utility>

#include "DocumentInfo.h"
#include "Visibility.h"

/// Interface implemented by field mappers.
class PINOT_EXPORT FieldMapperInterface
{
	public:
		FieldMapperInterface(const FieldMapperInterface &other) {};
		virtual ~FieldMapperInterface() {};

		/// Gets the host for this document.
		virtual std::string getHost(const DocumentInfo &docInfo) = 0;

		/// Gets the directory for this document.
		virtual std::string getDirectory(const DocumentInfo &docInfo) = 0;

		/// Gets the file for this document.
		virtual std::string getFile(const DocumentInfo &docInfo) = 0;

		/// Gets terms from the document and their prefixes.
		virtual void getTerms(const DocumentInfo &docInfo,
			std::vector<std::pair<std::string, std::string> > &prefixedTerms) = 0;

		/// Gets values.
		virtual void getValues(const DocumentInfo &docInfo,
			std::map<unsigned int, std::string> &values) = 0;

		/// Saves terms as record data.
		virtual void toRecord(const DocumentInfo *pDocInfo,
			std::string &record) = 0;

		/// Retrieves terms from record data.
		virtual void fromRecord(DocumentInfo *pDocInfo,
			const std::string &record) = 0;

		/// Returns whether terms with the prefix this filter corresponds to were escaped.
		virtual bool isEscaped(const std::string &filterName) = 0;

		/// Returns boolean query filters and their prefixes.
		virtual void getBooleanFilters(std::map<std::string, std::string> &filters) = 0;

		/// Returns the valuenumber  to collapse on, if any.
		virtual bool collapseOnValue(unsigned int &valueNumber) = 0;

	protected:
		FieldMapperInterface() { };

};

#endif // _FIELD_MAPPER_INTERFACE_H
