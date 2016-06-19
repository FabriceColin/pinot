/*
 *  Copyright 2007 Fabrice Colin
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

#ifndef _RESULTS_EXPORTER_H
#define _RESULTS_EXPORTER_H

#include <string>
#include <vector>
#include <fstream>
#include <libxml++/document.h>
#include <libxml++/nodes/element.h>

#include "DocumentInfo.h"
#include "QueryProperties.h"

/// Exports results to a given format.
class ResultsExporter
{
	public:
		ResultsExporter(const std::string &fileName,
			const QueryProperties &queryProps);
		virtual ~ResultsExporter();

		/// Exports a list of results.
		virtual bool exportResults(const std::string &engineName,
			unsigned int maxResultsCount,
			const std::vector<DocumentInfo> &resultsList) = 0;

		/// Starts export.
		virtual bool exportStart(const std::string &engineName,
			unsigned int maxResultsCount) = 0;

		/// Exports the given result.
		virtual bool exportResult(const std::string &engineName,
			const DocumentInfo &docInfo) = 0;

		/// Ends export.
		virtual void exportEnd(void) = 0;

	protected:
		std::string m_fileName;
		std::string m_queryName;
		std::string m_queryDetails;

	private:
		ResultsExporter(const ResultsExporter &other);
		ResultsExporter& operator=(const ResultsExporter& other);

};

/// Exports results to CSV.
class CSVExporter : public ResultsExporter
{
	public:
		CSVExporter(const std::string &fileName,
			const QueryProperties &queryProps);
		virtual ~CSVExporter();

		/// Exports the results; false if an error occured.
		virtual bool exportResults(const std::string &engineName,
			unsigned int maxResultsCount,
			const std::vector<DocumentInfo> &resultsList);

		/// Starts export.
		virtual bool exportStart(const std::string &engineName,
			unsigned int maxResultsCount);

		/// Exports the given result.
		virtual bool exportResult(const std::string &engineName,
			const DocumentInfo &docInfo);

		/// Ends export.
		virtual void exportEnd(void);

	protected:
		std::ofstream m_outputFile;

	private:
		CSVExporter(const CSVExporter &other);
		CSVExporter& operator=(const CSVExporter& other);

};

/// Exports results to OpenSearch response. 
class OpenSearchExporter : public ResultsExporter
{
	public:
		OpenSearchExporter(const std::string &fileName,
			const QueryProperties &queryProps);
		virtual ~OpenSearchExporter();

		/// Exports the results; false if an error occured.
		virtual bool exportResults(const std::string &engineName,
			unsigned int maxResultsCount,
			const std::vector<DocumentInfo> &resultsList);

		/// Starts export.
		virtual bool exportStart(const std::string &engineName,
			unsigned int maxResultsCount);

		/// Exports the given result.
		virtual bool exportResult(const std::string &engineName,
			const DocumentInfo &docInfo);

		/// Ends export.
		virtual void exportEnd(void);

	protected:
		xmlpp::Document *m_pDoc;
		xmlpp::Element *m_pChannelElem;

	private:
		OpenSearchExporter(const OpenSearchExporter &other);
		OpenSearchExporter& operator=(const OpenSearchExporter& other);

};

#endif // _RESULTS_EXPORTER_H
