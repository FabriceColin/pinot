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

#include <iostream>
#include <algorithm>

#include "MIMEScanner.h"
#include "Url.h"
#include "FilterUtils.h"
#include "FileCollector.h"

using namespace std;

class LastIPathAction : public ReducedAction
{
	public:
		LastIPathAction(const string &ipath) :
			ReducedAction(),
			m_ipath(ipath),
			m_pDocument(NULL)
		{
		}
		virtual ~LastIPathAction()
		{
		}

		virtual bool positionFilter(const Document &doc, Dijon::Filter *pFilter)
		{
			string::size_type nextPos = m_ipath.find("&next&");
			string thisIPath(m_ipath);

			if (nextPos != string::npos)
			{
				thisIPath = m_ipath.substr(0, nextPos);
			}

			if (pFilter != NULL)
			{
#ifdef DEBUG
				clog << "LastIPathAction::positionFilter: moving filter for "
					<< pFilter->get_mime_type() << " to ipath " << thisIPath << endl;
#endif
				pFilter->set_property(Dijon::Filter::OPERATING_MODE, "view");

				// Skip forward
				pFilter->skip_to_document(thisIPath);
			}

			if (nextPos != string::npos)
			{
				 m_ipath.erase(0, nextPos + 6);
			}
			else
			{
				m_ipath.clear();
			}

			return true;
		}

		virtual bool isReduced(const Document &doc)
		{
			if (m_ipath.empty() == false)
			{
				// Go one level deeper
#ifdef DEBUG
				clog << "LastIPathAction::isReduced: not final" << endl;
#endif
				return false;
			}

			return true;
		}

		virtual bool takeAction(Document &doc, bool isNested)
		{
			if (m_ipath.empty() == true)
			{
				m_pDocument = new Document(doc);
#ifdef DEBUG
				clog << "LastIPathAction::takeAction: ipath " << doc.getInternalPath() << " is final" << endl;
#endif
			}

			return true;
		}

		Document *getDocument(void)
		{
			return m_pDocument;
		}

	protected:
		string m_ipath;
		Document *m_pDocument;

	private:
		LastIPathAction(const LastIPathAction &other);
		LastIPathAction &operator=(const LastIPathAction &other);

};

FileCollector::FileCollector() :
	DownloaderInterface()
{
}

FileCollector::~FileCollector()
{
}

//
// Implementation of DownloaderInterface
//

/// Retrieves the specified document; NULL if error.
Document *FileCollector::retrieveUrl(const DocumentInfo &docInfo)
{
	map<string, string> headers;

	return retrieveUrl(docInfo, headers);
}

/// Retrieves the specified document; NULL if error.
Document *FileCollector::retrieveUrl(const DocumentInfo &docInfo,
	const map<string, string> &headers)
{
	Url thisUrl(docInfo.getLocation());
	string protocol(thisUrl.getProtocol());
	string ipath(docInfo.getInternalPath());

	if (protocol != "file")
	{
		// We can't handle that type of protocol...
		return NULL;
	}

	string fileLocation(thisUrl.getLocation());
	fileLocation += "/";
	fileLocation += thisUrl.getFile();

	Document *pDocument = new Document(docInfo);

	if (pDocument->setDataFromFile(fileLocation) == false)
	{
		delete pDocument;
		return NULL;
	}

	// Determine the file type
	string type(MIMEScanner::scanFile(fileLocation));

	// Stop here ?
	if (ipath.empty() == true)
	{
		if (pDocument->getType().empty() == true)
		{
			pDocument->setType(type);
		}

		return pDocument;
	}

	// Reset these to avoid confusing the filters
	pDocument->setInternalPath("");
	pDocument->setType(type);

	LastIPathAction action(ipath);
	bool reduced = FilterUtils::reduceDocument(*pDocument, action);
	if (reduced == true)
	{
		Document *pBottomMostDocument = action.getDocument();

		if (pBottomMostDocument != NULL)
		{
			delete pDocument;

			return pBottomMostDocument;
		}
	}

	return pDocument;
}

