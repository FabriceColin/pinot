/*
 *  Copyright 2005-2021 Fabrice Colin
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
#include <stdio.h>
#include <getopt.h>
#include <iostream>
#include <fstream>
#include <string>

#include "config.h"
#include "NLS.h"
#include "Languages.h"
#include "MIMEScanner.h"
#include "Url.h"
#include "DownloaderFactory.h"
#include "ModuleFactory.h"
#include "ResultsExporter.h"
#include "WebEngine.h"
#include "PinotSettings.h"

using namespace std;

static struct option g_longOptions[] = {
	{"datefirst", 0, 0, 'd'},
	{"help", 0, 0, 'h'},
	{"locationonly", 0, 0, 'l'},
	{"max", 1, 0, 'm'},
	{"storedquery", 0, 0, 'r'},
	{"stemming", 1, 0, 's'},
	{"tocsv", 1, 0, 'c'},
	{"toxml", 1, 0, 'x'},
	{"version", 0, 0, 'v'},
	{0, 0, 0, 0}
};

static void printHelp(void)
{
	map<ModuleProperties, bool> engines;

	// Help
	ModuleFactory::loadModules(string(LIBDIR) + string("/pinot/backends"));
	ModuleFactory::getSupportedEngines(engines);
	ModuleFactory::unloadModules();
	clog << "pinot-search - Query search engines from the command-line\n\n"
		<< "Usage: pinot-search [OPTIONS] SEARCHENGINETYPE SEARCHENGINENAME|SEARCHENGINEOPTION QUERYINPUT\n\n"
		<< "Options:\n"
		<< "  -d, --datefirst           sort by date then by relevance\n"
		<< "  -h, --help                display this help and exit\n"
		<< "  -l, --locationonly        only show the location of each result\n"
		<< "  -m, --max                 maximum number of results (default 10)\n"
		<< "  -r, --storedquery         query input is the name of a stored query\n"
		<< "  -s, --stemming            stemming language (in English)\n"
		<< "  -c, --tocsv               file to export results in CSV format to\n"
		<< "  -x, --toxml               file to export results in XML format to\n"
		<< "  -v, --version             output version information and exit\n"
		<< "Supported search engine types are :";
	for (map<ModuleProperties, bool>::const_iterator engineIter = engines.begin(); engineIter != engines.end(); ++engineIter)
	{
		clog << " '" << engineIter->first.m_name << "'";
	}
	clog << "\n\nExamples:\n"
		<< "pinot-search opensearch " << PREFIX << "/share/pinot/engines/KrustyDescription.xml \"clowns\"\n\n"
		<< "pinot-search --max 20 sherlock " << PREFIX << "/share/pinot/engines/Bozo.src \"clowns\"\n\n"
		<< "pinot-search xapian ~/.pinot/index \"label:Clowns\"\n\n"
		<< "pinot-search --stemming english xapian somehostname:12345 \"clowning\"\n\n"
		<< "Report bugs to " << PACKAGE_BUGREPORT << endl;
}

int main(int argc, char **argv)
{
	QueryProperties::QueryType queryType = QueryProperties::XAPIAN_QP;
	string engineType, option, csvExport, xmlExport, stemLanguage;
	unsigned int maxResultsCount = 10; 
	int longOptionIndex = 0;
	bool printResults = true;
	bool sortByDate = false;
	bool locationOnly = false;
	bool isStoredQuery = false;

	// Look at the options
	int optionChar = getopt_long(argc, argv, "c:dhlm:rs:vx:", g_longOptions, &longOptionIndex);
	while (optionChar != -1)
	{
		switch (optionChar)
		{
			case 'c':
				if (optarg != NULL)
				{
					csvExport = optarg;
					printResults = false;
				}
				break;
			case 'd':
				sortByDate = true;
				break;
			case 'h':
				printHelp();
				return EXIT_SUCCESS;
			case 'l':
				locationOnly = true;
				break;
			case 'm':
				if (optarg != NULL)
				{
					maxResultsCount = (unsigned int )atoi(optarg);
				}
				break;
			case 'r':
				isStoredQuery = true;
				break;
			case 's':
				if (optarg != NULL)
				{
					stemLanguage = optarg;
				}
				break;
			case 'v':
				clog << "pinot-search - " << PACKAGE_STRING << "\n\n"
					<< "This is free software.  You may redistribute copies of it under the terms of\n"
					<< "the GNU General Public License <http://www.gnu.org/licenses/old-licenses/gpl-2.0.html>.\n"
					<< "There is NO WARRANTY, to the extent permitted by law." << endl;
				return EXIT_SUCCESS;
			case 'x':
				if (optarg != NULL)
				{
					xmlExport = optarg;
					printResults = false;
				}
				break;
			default:
				return EXIT_FAILURE;
		}

		// Next option
		optionChar = getopt_long(argc, argv, "c:dhlm:rs:vx:", g_longOptions, &longOptionIndex);
	}

#if defined(ENABLE_NLS)
	bindtextdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
	textdomain(GETTEXT_PACKAGE);
#endif //ENABLE_NLS

	if (argc == 1)
	{
		printHelp();
		return EXIT_SUCCESS;
	}

	if ((argc < 4) ||
		(argc - optind != 3))
	{
		clog << "Wrong number of parameters" << endl;
		return EXIT_FAILURE;
	}

	// This will create the necessary directories on the first run
	PinotSettings &settings = PinotSettings::getInstance();
	string confDirectory(PinotSettings::getConfigurationDirectory());

	if (MIMEScanner::initialize(PinotSettings::getHomeDirectory() + "/.local",
		string(SHARED_MIME_INFO_PREFIX)) == false)
	{
		clog << "Couldn't load MIME settings" << endl;
	}
	DownloaderInterface::initialize();
	ModuleFactory::loadModules(string(LIBDIR) + string("/pinot/backends"));
	ModuleFactory::loadModules(confDirectory + "/backends");

	// Localize language names
	Languages::setIntlName(0, _("Unknown"));
	Languages::setIntlName(1, _("Danish"));
	Languages::setIntlName(2, _("Dutch"));
	Languages::setIntlName(3, _("English"));
	Languages::setIntlName(4, _("Finnish"));
	Languages::setIntlName(5, _("French"));
	Languages::setIntlName(6, _("German"));
	Languages::setIntlName(7, _("Hungarian"));
	Languages::setIntlName(8, _("Italian"));
	Languages::setIntlName(9, _("Norwegian"));
	Languages::setIntlName(10, _("Portuguese"));
	Languages::setIntlName(11, _("Romanian"));
	Languages::setIntlName(12, _("Russian"));
	Languages::setIntlName(13, _("Spanish"));
	Languages::setIntlName(14, _("Swedish"));
	Languages::setIntlName(15, _("Turkish"));

	// Load the settings
	settings.load(PinotSettings::LOAD_ALL);

	engineType = argv[optind];
	option = argv[optind + 1];
	char *pQueryInput = argv[optind + 2];

	// Set the query
	QueryProperties queryProps("pinot-search", "", queryType);
	if (queryType == QueryProperties::XAPIAN_QP)
	{
		if (isStoredQuery == true)
		{
			const map<string, QueryProperties> &queries = settings.getQueries();
			map<string, QueryProperties>::const_iterator queryIter = queries.find(pQueryInput);
			if (queryIter != queries.end())
			{
				queryProps = queryIter->second;
			}
			else
			{
				clog << "Couldn't find stored query " << pQueryInput << endl;

				DownloaderInterface::shutdown();
				MIMEScanner::shutdown();

				return EXIT_FAILURE;
			}
		}
		else
		{
			queryProps.setFreeQuery(pQueryInput);
		}
	}
	queryProps.setStemmingLanguage(stemLanguage);
	queryProps.setMaximumResultsCount(maxResultsCount);
	if (sortByDate == true)
	{
		queryProps.setSortOrder(QueryProperties::DATE_DESC);
	}

	// Which SearchEngine ?
	SearchEngineInterface *pEngine = ModuleFactory::getSearchEngine(engineType, option);
	if (pEngine == NULL)
	{
		clog << "Couldn't obtain search engine instance" << endl;

		DownloaderInterface::shutdown();
		MIMEScanner::shutdown();

		return EXIT_FAILURE;
	}

	// Set up the proxy
	WebEngine *pWebEngine = dynamic_cast<WebEngine *>(pEngine);
	if (pWebEngine != NULL)
	{
		DownloaderInterface *pDownloader = pWebEngine->getDownloader();
		if ((pDownloader != NULL) &&
			(settings.m_proxyEnabled == true) &&
			(settings.m_proxyAddress.empty() == false))
		{
			char portStr[64];

			pDownloader->setSetting("proxyaddress", settings.m_proxyAddress);
			snprintf(portStr, 64, "%u", settings.m_proxyPort);
			pDownloader->setSetting("proxyport", portStr);
			pDownloader->setSetting("proxytype", settings.m_proxyType);
		}

		pWebEngine->setEditableValues(settings.m_editablePluginValues);
	}

	pEngine->setDefaultOperator(SearchEngineInterface::DEFAULT_OP_AND);
	if (pEngine->runQuery(queryProps) == true)
	{
		string resultsPage;
		unsigned int estimatedResultsCount = pEngine->getResultsCountEstimate();

		const vector<DocumentInfo> &resultsList = pEngine->getResults();
		if (resultsList.empty() == false)
		{
			if (printResults == true)
			{
				unsigned int count = 0;

				if (locationOnly == false)
				{
					clog << "Showing " << resultsList.size() << " results of about " << estimatedResultsCount << endl;
				}

				vector<DocumentInfo>::const_iterator resultIter = resultsList.begin();
				while (resultIter != resultsList.end())
				{
					string rawUrl(resultIter->getLocation(true));

					if (locationOnly == false)
					{
						clog << count << " Location : '" << rawUrl << "'"<< endl;
						clog << count << " Title    : " << resultIter->getTitle() << endl;
						clog << count << " Type     : " << resultIter->getType() << endl;
						clog << count << " Language : " << resultIter->getLanguage() << endl;
						clog << count << " Date     : " << resultIter->getTimestamp() << endl;
						clog << count << " Size     : " << resultIter->getSize() << endl;
						clog << count << " Extract  : " << resultIter->getExtract() << endl;
						clog << count << " Score    : " << resultIter->getScore() << endl;
					}
					else
					{
						clog << rawUrl << endl;
					}
					++count;

					// Next
					++resultIter;
				}
			}
			else
			{
				string engineName(ModuleFactory::getSearchEngineName(engineType, option));

				if (csvExport.empty() == false)
				{
					CSVExporter exporter(csvExport, queryProps);

					exporter.exportResults(engineName, maxResultsCount, resultsList);
				}

				if (xmlExport.empty() == false)
				{
					OpenSearchExporter exporter(xmlExport, queryProps);

					exporter.exportResults(engineName, maxResultsCount, resultsList);
				}
			}
		}
		else
		{
			clog << "No results" << endl;
		}
	}
	else
	{
		clog << "Couldn't run query on search engine " << engineType << endl;
	}

	delete pEngine;

	ModuleFactory::unloadModules();
	DownloaderInterface::shutdown();
	MIMEScanner::shutdown();

	return EXIT_SUCCESS;
}
