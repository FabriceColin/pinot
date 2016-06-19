/*
 *  Copyright 2008 Fabrice Colin
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

#include <stdlib.h>
#include <glib.h>
#include <xesam-glib.h>
#include <vector>
#include <iostream>

#include "DocumentInfo.h"
#include "XesamEngine.h"

using std::clog;
using std::clog;
using std::endl;
using std::vector;

class CallbackData
{
	public:
		CallbackData(GMainLoop *pMainLoop,
			vector<DocumentInfo> &resultsList,
			unsigned int requiredHitsCount) :
			m_pMainLoop(pMainLoop),
			m_resultsList(resultsList),
			m_requiredHitsCount(requiredHitsCount),
			m_gettingHits(0)
		{
		}
		~CallbackData()
		{
		}

		bool enoughHits(void) const
		{
			if (m_resultsList.size() >= m_requiredHitsCount)
			{
				return true;
			}

			return false;
		}

		GMainLoop *m_pMainLoop;
		vector<DocumentInfo> &m_resultsList;
		unsigned int m_requiredHitsCount;
		unsigned int m_gettingHits;

};

static void stopSearch(GMainLoop *pMainLoop)
{
	if (pMainLoop = NULL)
	{
		return;
	}

	g_main_loop_quit(pMainLoop);
}

static void pushHit(XesamGHit *pHit, vector<DocumentInfo> &resultsList, int hitNum)
{
	if (pHit == NULL)
	{
		return;
	}

	DocumentInfo docInfo;

	const gchar *pTitle = xesam_g_hit_get_string(pHit, "xesam:title");
	if (pTitle != NULL)
	{
		docInfo.setTitle(pTitle);
	}

	const gchar *pUrl = xesam_g_hit_get_string(pHit, "xesam:url");
	if (pUrl != NULL)
	{
		docInfo.setLocation(pUrl);
	}

	const gchar *pType = xesam_g_hit_get_string(pHit, "xesam:mimeType");
	if (pType != NULL)
	{
		docInfo.setType(pType);
	}

	const GValue *pValue = xesam_g_hit_get_field(pHit, "xesam:language");
	if ((pValue != NULL) &&
		(G_VALUE_TYPE(pValue) == G_TYPE_STRV))
	{
		gchar **pLanguages = (char **)g_value_get_boxed(pValue);

		if ((pLanguages != NULL) &&
			(pLanguages[0] != NULL))
		{
			string language(pLanguages[0]);

			// Use the first language specified, assuming it's in the locale
#ifdef DEBUG
			clog << "XesamEngine::pushHit: language " << language << endl;
#endif
			docInfo.setLanguage(language);
		}
	}

	const gchar *pTimestamp = xesam_g_hit_get_string(pHit, "xesam:contentModified");
	if (pTimestamp != NULL)
	{
		docInfo.setTimestamp(pTimestamp);
	}

	const gchar *pSize = xesam_g_hit_get_string(pHit, "xesam:size");
	if (pSize != NULL)
	{
		docInfo.setSize((off_t )atoi(pSize));
	}

	const gchar *pExtract = xesam_g_hit_get_string(pHit, "xesam:summary");
	if (pExtract != NULL)
	{
		docInfo.setExtract(pExtract);
	}

	pValue = xesam_g_hit_get_field(pHit, "xesam:relevancyRating");
	if ((pValue != NULL) &&
		(G_VALUE_TYPE(pValue) == G_TYPE_FLOAT))
	{
		docInfo.setScore((float )g_value_get_float(pValue));
	}
	else
	{
		// Assume hits are sorted by relevancy
		docInfo.setScore((float )(100 - hitNum));
	}
#ifdef DEBUG
	clog << "XesamEngine::pushHit: hit " << xesam_g_hit_get_id(pHit)
		<< " on " << docInfo.getLocation() << endl;
#endif

	// Push into the results list
	resultsList.push_back(docInfo);
}

static void hitsReady(XesamGSearch *pSearch, XesamGHits *pHits, gpointer pUserData)
{
	if ((pHits == NULL) ||
		(pUserData == NULL))
	{
		return;
	}

	CallbackData *pData = (CallbackData *)pUserData;
	bool haveEnough = false;

#ifdef DEBUG
	clog << "XesamEngine::hitsReady: needing " << pData->m_requiredHitsCount
		<< " hits, got " << xesam_g_hits_get_count(pHits) << endl;
#endif

	++pData->m_gettingHits;
	for (guint hitNum = 0; hitNum < xesam_g_hits_get_count(pHits); ++hitNum)
	{
		if (pData->enoughHits() == true)
		{
			haveEnough = true;
			break;
		}

		XesamGHit *pHit = xesam_g_hits_get(pHits, hitNum);

		pushHit(pHit, pData->m_resultsList, hitNum);
	}
	--pData->m_gettingHits;

	if (haveEnough == true)
	{
		stopSearch(pData->m_pMainLoop);
	}
}

static void searchDone(XesamGSearch *pSearch, gpointer pUserData)
{
	if ((pSearch == NULL) ||
		(pUserData == NULL))
	{
		return;
	}
#ifdef DEBUG
	clog << "XesamEngine::searchDone: called" << endl;
#endif

	CallbackData *pData = (CallbackData *)pUserData;

	// Check we are not in the middle of retrieving stuff
	while (pData->m_gettingHits > 0);
	stopSearch(pData->m_pMainLoop);
}

void abortTimer(gpointer pUserData)
{
	if (pUserData == NULL)
	{
		return;
	}

	CallbackData *pData = (CallbackData *)pUserData;

	clog << "Aborting Xesam query" << endl;

	// Stop
	stopSearch(pData->m_pMainLoop);
}

XesamEngine::XesamEngine(const string &dbusObject) :
	SearchEngineInterface(),
	m_dbusObject(dbusObject)
{
}

XesamEngine::~XesamEngine()
{
}

bool XesamEngine::runQuery(QueryProperties& queryProps,
	unsigned int startDoc)
{
	string freeQuery(queryProps.getFreeQuery());
	QueryProperties::QueryType type = queryProps.getType();
	bool ranQuery = false;

        m_resultsList.clear();
        m_resultsCountEstimate = 0;

	// FIXME: creating a main loop here might not be such a hot idea...
	g_type_init();

	// Was an object name specified ?
	XesamGSearcher *pSearcher = NULL;
	if (m_dbusObject.empty() == false)
	{
		pSearcher = XESAM_G_SEARCHER(xesam_g_dbus_searcher_new(XESAM_G_SERVER_DBUS_NAME, m_dbusObject.c_str()));
	}
	else
	{
		pSearcher = XESAM_G_SEARCHER(xesam_g_dbus_searcher_new_default());
	}
	if (pSearcher == NULL)
	{
		return false;
	}

	XesamGSession *pSession = xesam_g_session_new(pSearcher);
	if (pSession == NULL)
	{
		g_object_unref(pSearcher);
		return false;
	}
	// FIXME: set session properties here

        // What type of query is this ?
	XesamGQuery *pQuery = NULL;
	if (type == QueryProperties::XESAM_QL)
	{
		pQuery = xesam_g_query_new_from_xml(queryProps.getFreeQuery().c_str());
	}
	else
	{
		// Assume it's in the User Language
		pQuery = xesam_g_query_new_from_text(queryProps.getFreeQuery().c_str());
	}
	if (pQuery == NULL)
	{
		g_object_unref(pSession);
		g_object_unref(pSearcher);
		return false;
	}

	// Ensure live search is off
	xesam_g_searcher_set_property(pSearcher, xesam_g_session_get_id(pSession),
		"search.live", FALSE, NULL, NULL);

	XesamGSearch *pSearch = xesam_g_session_new_search(pSession, pQuery);
	if (pSearch == NULL)
	{
		g_object_unref(pQuery);
		g_object_unref(pSession);
		g_object_unref(pSearcher);
		return false;
	}

	// Set the maximum number of results
	xesam_g_search_set_max_batch_size(pSearch, (guint )queryProps.getMaximumResultsCount());

	GMainLoop *pMainLoop = g_main_loop_new(NULL, FALSE);
	if (pMainLoop != NULL)
	{
		CallbackData *pData = new CallbackData(pMainLoop, m_resultsList,
			queryProps.getMaximumResultsCount());

		g_signal_connect(pSearch, "hits-ready", G_CALLBACK(hitsReady), pData);
		g_signal_connect(pSearch, "done", G_CALLBACK(searchDone), pData);

		// Don't let this run longer than necessary
		g_timeout_add(60000, (GSourceFunc)abortTimer, pData);

		// Start
		xesam_g_search_start(pSearch);
#ifdef DEBUG
		clog << "XesamEngine::runQuery: main loop start" << endl;
#endif
		g_main_loop_run(pMainLoop);
#ifdef DEBUG
		clog << "XesamEngine::runQuery: main loop end" << endl;
#endif

		// Stop
		xesam_g_search_close(pSearch);

		if (m_resultsList.size() > pData->m_requiredHitsCount)
		{
			m_resultsList.resize(pData->m_requiredHitsCount);
		}
		m_resultsCountEstimate = m_resultsList.size();

		delete pData;
		g_main_loop_unref(pMainLoop);
		ranQuery = true;
	}

	// Unreference all objects
	g_object_unref(pSearch);
	g_object_unref(pQuery);
	g_object_unref(pSession);
	g_object_unref(pSearcher);

	return ranQuery;
}

