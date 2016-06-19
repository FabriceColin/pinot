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

#include <iostream>
#include <fstream>
#include <libxml++/parsers/domparser.h>
#include <libxml++/nodes/node.h>
#include <libxml++/nodes/textnode.h>

#include "StringManip.h"
#include "FilterUtils.h"
#include "ResultsExporter.h"

using std::string;
using std::vector;
using std::ofstream;
using std::endl;
using xmlpp::Element;

static Element *addChildElement(Element *pElem, const string &nodeName, const string &nodeContent)
{
	if (pElem == NULL)
	{
		return NULL;
	}

	Element *pSubElem = pElem->add_child(nodeName);
	if (pSubElem != NULL)
	{
#ifdef HAS_LIBXMLPP026
		pSubElem->set_child_content(nodeContent);
#else
		pSubElem->set_child_text(nodeContent);
#endif
	}

	return pSubElem;
}

ResultsExporter::ResultsExporter(const string &fileName,
	const QueryProperties &queryProps) :
	m_fileName(fileName),
	m_queryName(queryProps.getName()),
	m_queryDetails(queryProps.getFreeQuery())
{
}

ResultsExporter::~ResultsExporter()
{
}

CSVExporter::CSVExporter(const string &fileName,
	const QueryProperties &queryProps) :
	ResultsExporter(fileName, queryProps)
{
}

CSVExporter::~CSVExporter()
{
}

bool CSVExporter::exportResults(const string &engineName, unsigned int maxResultsCount,
	const vector<DocumentInfo> &resultsList)
{
	if ((resultsList.empty() == true) ||
		(exportStart(engineName, maxResultsCount) == false))
	{
		return false;
	}

	for (vector<DocumentInfo>::const_iterator iter = resultsList.begin();
		iter != resultsList.end(); ++iter)
	{
		exportResult(engineName, *iter);
	}
	exportEnd();

	return true;
}

bool CSVExporter::exportStart(const string &engineName, unsigned int maxResultsCount)
{
	if (m_fileName.empty() == true)
	{
		return false;
	}

	m_outputFile.open(m_fileName.c_str());
	if (m_outputFile.good() == false)
	{
		m_outputFile.close();

		return false;
	}

	m_outputFile << "\"query\";\"engine\";\"caption\";\"url\";\"type\";\"language\";\"modtime\";\"size\";\"abstract\"" << endl;

	return true;
}

bool CSVExporter::exportResult(const string &engineName, const DocumentInfo &docInfo)
{
	string title(FilterUtils::stripMarkup(docInfo.getTitle()));
	string extract(FilterUtils::stripMarkup(docInfo.getExtract()));

	if (m_outputFile.good() == false)
	{
		return false;
	}

	// Double double-quotes
	m_outputFile << "\"" << StringManip::replaceSubString(m_queryName, "\"", "\"\"")
		<< "\";\"" << StringManip::replaceSubString(engineName, "\"", "\"\"")
		<< "\";\"" << StringManip::replaceSubString(title, "\"", "\"\"")
		<< "\";\"" << StringManip::replaceSubString(docInfo.getLocation(), "\"", "\"\"")
		<< "\";\"" << StringManip::replaceSubString(docInfo.getType(), "\"", "\"\"")
		<< "\";\"" << StringManip::replaceSubString(docInfo.getLanguage(), "\"", "\"\"")
		<< "\";\"" << StringManip::replaceSubString(docInfo.getTimestamp(), "\"", "\"\"")
		<< "\";\"" << docInfo.getSize()
		<< "\";\"" << StringManip::replaceSubString(extract, "\"", "\"\"")
		<< "\"" << endl;

	return true;
}

void CSVExporter::exportEnd(void)
{
	m_outputFile.close();
}

OpenSearchExporter::OpenSearchExporter(const string &fileName,
	const QueryProperties &queryProps) :
	ResultsExporter(fileName, queryProps),
	m_pDoc(NULL),
	m_pChannelElem(NULL)
{
}

OpenSearchExporter::~OpenSearchExporter()
{
}

bool OpenSearchExporter::exportResults(const string &engineName, unsigned int maxResultsCount,
	const vector<DocumentInfo> &resultsList)
{
	if ((resultsList.empty() == true) ||
		(exportStart(engineName, maxResultsCount) == false))
	{
		return false;
	}

	for (vector<DocumentInfo>::const_iterator iter = resultsList.begin();
		iter != resultsList.end(); ++iter)
	{
		exportResult(engineName, *iter);
	}
	exportEnd();

	return true;
}

bool OpenSearchExporter::exportStart(const string &engineName, unsigned int maxResultsCount)
{
	if (m_fileName.empty() == true)
	{
		return false;
	}

	if (m_pDoc != NULL)
	{
		delete m_pDoc;
		m_pDoc = NULL;
		m_pChannelElem = NULL;
	}

	Element *pRootElem = NULL;
	string description("Search");
	char numStr[64];

	m_pDoc = new xmlpp::Document("1.0");

	// Create a new node
	pRootElem = m_pDoc->create_root_node("rss");
	if (pRootElem == NULL)
	{
		return false;
	}
	pRootElem->set_attribute("version", "2.0");
	pRootElem->set_attribute("xmlns:opensearch", "http://a9.com/-/spec/opensearch/1.1/");
	pRootElem->set_attribute("xmlns:atom", "http://www.w3.org/2005/Atom");

	// User interface position and size
	m_pChannelElem = pRootElem->add_child("channel");
	if (m_pChannelElem == NULL)
	{
		return false;
	}
	if (m_queryName.empty() == false)
	{
		addChildElement(m_pChannelElem, "title", m_queryName);
	}
	if (m_queryName.empty() == false)
	{
		description += " for \"";
		description += m_queryName;
		description += "\"";
	}
	if (engineName.empty() == false)
	{
		description += " on ";
		description += engineName;
	}
	addChildElement(m_pChannelElem, "description", description);
	snprintf(numStr, 64, "%d", maxResultsCount);
	addChildElement(m_pChannelElem, "opensearch:totalResults", numStr);
	addChildElement(m_pChannelElem, "opensearch:itemsPerPage", numStr);
	if (m_queryDetails.empty() == false)
	{
		Element *pQueryElem = addChildElement(m_pChannelElem, "opensearch:Query", "");
		if (pQueryElem != NULL)
		{
			pQueryElem->set_attribute("role", "request");
			pQueryElem->set_attribute("searchTerms", m_queryDetails);
			pQueryElem->set_attribute("startPage", "1");
		}
	}

	return true;
}

bool OpenSearchExporter::exportResult(const string &engineName, const DocumentInfo &docInfo)
{
	if (m_pChannelElem == NULL)
	{
		return false;
	}

	Element *pElem = m_pChannelElem->add_child("item");
	addChildElement(pElem, "title", docInfo.getTitle());
	addChildElement(pElem, "link", docInfo.getLocation());
	addChildElement(pElem, "description", FilterUtils::stripMarkup(docInfo.getExtract()));

	return true;
}

void OpenSearchExporter::exportEnd(void)
{
	if (m_pDoc == NULL)
	{
		return;
	}

	// Save to file
	m_pDoc->write_to_file_formatted(m_fileName);

	m_pChannelElem = NULL;
	delete m_pDoc;
	m_pDoc = NULL;
}

