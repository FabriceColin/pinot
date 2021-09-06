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
 
#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <strings.h>
#include <signal.h>
#include <iostream>
#include <string>
#include <fstream>
#include <glibmm.h>
#include <glibmm/thread.h>
#include <glibmm/miscutils.h>
#include <sigc++/sigc++.h>

#include "config.h"
#include "NLS.h"
#include "Languages.h"
#include "Memory.h"
#include "MIMEScanner.h"
#include "Url.h"
#include "FilterFactory.h"
#include "DownloaderFactory.h"
#include "FilterWrapper.h"
#include "ModuleFactory.h"
#include "ActionQueue.h"
#include "PinotSettings.h"
#include "WorkerThreads.h"

using namespace std;

static struct option g_longOptions[] = {
	{"backend", 1, 0, 'b'},
	{"check", 0, 0, 'c'},
	{"db", 1, 0, 'd'},
	{"help", 0, 0, 'h'},
	{"index", 0, 0, 'i'},
	{"override", 1, 0, 'o'},
	{"showinfo", 0, 0, 's'},
	{"version", 0, 0, 'v'},
	{0, 0, 0, 0}
};
static Glib::RefPtr<Glib::MainLoop> g_refMainLoop;

class IndexingState : public QueueManager
{
	public:
		IndexingState(const string &indexLocation) :
			QueueManager(indexLocation, 60, true),
			m_docId(0)
		{
			// Disable implicit flushing
			WorkerThread::immediateFlush(false);

			m_onThreadEndSignal.connect(sigc::mem_fun(*this, &IndexingState::on_thread_end));
		}

		virtual ~IndexingState()
		{
		}

		virtual Glib::ustring queue_index(const DocumentInfo &docInfo)
		{
			// Index directly
			return index_document(docInfo);
		}

		void on_thread_end(WorkerThread *pThread)
		{
			string indexedUrl;

			if (pThread == NULL)
			{
				return;
			}

			string type(pThread->getType());
			bool isStopped = pThread->isStopped();
#ifdef DEBUG
			clog << "IndexingState::on_thread_end: end of thread " << type << " " << pThread->getId() << endl;
#endif

			// What type of thread was it ?
			if ((isStopped == false) &&
				((type == "IndexingThread") || (type == "DirectoryScannerThread")))
			{
				IndexingThread *pIndexThread = dynamic_cast<IndexingThread *>(pThread);
				if (pIndexThread == NULL)
				{
					delete pThread;
					return;
				}

				// Get the document ID of the URL we have just indexed
				m_docId = pIndexThread->getDocumentID();

				// Explicitely flush the index once a directory has been crawled
				IndexInterface *pIndex = PinotSettings::getInstance().getIndex(m_defaultIndexLocation);
				if (pIndex != NULL)
				{
					pIndex->flush();

					delete pIndex;
				}
			}

			// Delete the thread
			delete pThread;

			Memory::reclaim();

			// Stop there
			g_refMainLoop->quit();
		}

		unsigned int getDocumentID(void) const
		{
			return m_docId;
		}

	protected:
		unsigned int m_docId;

};
static IndexingState *g_pState = NULL;

static void printHelp(void)
{
	map<ModuleProperties, bool> engines;

	// Help
	ModuleFactory::loadModules(string(LIBDIR) + string("/pinot/backends"));
	ModuleFactory::getSupportedEngines(engines);
	clog << "pinot-index - Index documents from the command-line\n\n"
		<< "Usage: pinot-index [OPTIONS] --db DATABASE URLS\n\n"
		<< "Options:\n"
		<< "  -b, --backend             name of back-end to use (default " << PinotSettings::getInstance().m_defaultBackend << ")\n"
		<< "  -c, --check               check whether the given URL is in the index\n"
		<< "  -d, --db                  path to, or name of, index to use (mandatory)\n"
		<< "  -h, --help                display this help and exit\n"
		<< "  -i, --index               index the given URL\n"
		<< "  -o, --override            MIME type detection override, as TYPE:EXT\n"
		<< "  -s, --showinfo            show information about the document\n"
		<< "  -v, --version             output version information and exit\n\n"
		<< "Supported back-ends are :";
	for (map<ModuleProperties, bool>::const_iterator engineIter = engines.begin(); engineIter != engines.end(); ++engineIter)
	{
		if ((engineIter->second == true) &&
			(ModuleFactory::isSupported(engineIter->first.m_name, true) == true))
		{
			clog << " '" << engineIter->first.m_name << "'";
		}
	}
	ModuleFactory::unloadModules();
	clog << "\n\nExamples:\n"
		<< "pinot-index --check --showinfo --backend xapian --db ~/.pinot/daemon ../Bozo.txt\n\n"
		<< "pinot-index --index --db Docs https://github.com/FabriceColin/pinot\n\n"
		<< "pinot-index --index --db Docs --override text/x-rst:rst /usr/share/doc/python-kitchen-1.1.1/docs/index.rst\n\n"
		<< "Indexing documents to My Web Pages or My Documents with pinot-index is not recommended\n\n"
		<< "Report bugs to " << PACKAGE_BUGREPORT << endl;
}

static void closeAll(void)
{
	if (g_pState != NULL)
	{
		delete g_pState;
		g_pState = NULL;
	}

	// Close everything
	ModuleFactory::unloadModules();
	DownloaderInterface::shutdown();
	MIMEScanner::shutdown();
}

static void quitAll(int sigNum)
{
	if (g_refMainLoop->is_running() == true)
	{
		clog << "Quitting..." << endl;

		if (g_pState != NULL)
		{
			g_pState->mustQuit(true);
		}
		g_refMainLoop->quit();
	}
}

int main(int argc, char **argv)
{
	string type, option;
	string backendType, databaseName;
	int longOptionIndex = 0;
	bool checkDocument = false, indexDocument = false, showInfo = false, success = false;

	// Look at the options
	int optionChar = getopt_long(argc, argv, "b:cd:hio:sv", g_longOptions, &longOptionIndex);
	while (optionChar != -1)
	{
		set<string> engines;

		switch (optionChar)
		{
			case 'b':
				if (optarg != NULL)
				{
					backendType = optarg;
				}
				break;
			case 'c':
				checkDocument = true;
				break;
			case 'd':
				if (optarg != NULL)
				{
					databaseName = optarg;
				}
				break;
			case 'h':
				printHelp();
				return EXIT_SUCCESS;
			case 'i':
				indexDocument = true;
				break;
			case 'o':
				if (optarg != NULL)
				{
					string override(optarg);
					string::size_type pos = override.find(':');

					if ((pos != string::npos) &&
						(pos + 1 < override.length()))
					{
						MIMEScanner::addOverride(override.substr(0, pos), override.substr(pos + 1));
					}
				}
				break;
			case 's':
				showInfo = true;
				break;
			case 'v':
				clog << "pinot-index - " << PACKAGE_STRING << "\n\n"
					<< "This is free software.  You may redistribute copies of it under the terms of\n"
					<< "the GNU General Public License <http://www.gnu.org/licenses/old-licenses/gpl-2.0.html>.\n"
					<< "There is NO WARRANTY, to the extent permitted by law." << endl;
				return EXIT_SUCCESS;
			default:
				return EXIT_FAILURE;
		}

		// Next option
		optionChar = getopt_long(argc, argv, "b:cd:hio:sv", g_longOptions, &longOptionIndex);
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

	if ((argc < 2) ||
		(argc - optind == 0))
	{
		clog << "Not enough parameters" << endl;
		return EXIT_FAILURE;
	}

	if (((indexDocument == false) &&
		(checkDocument == false)) ||
		(databaseName.empty() == true))
	{
		clog << "Incorrect parameters" << endl;
		return EXIT_FAILURE;
	}

	// Initialize threads support before doing anything else
	if (Glib::thread_supported() == false)
	{
		Glib::thread_init();
	}

	g_refMainLoop = Glib::MainLoop::create();
	Glib::set_application_name("Pinot Indexer");

	// This will create the necessary directories on the first run
	PinotSettings &settings = PinotSettings::getInstance();
	string confDirectory(PinotSettings::getConfigurationDirectory());

	if (MIMEScanner::initialize(PinotSettings::getHomeDirectory() + "/.local",
		string(SHARED_MIME_INFO_PREFIX)) == false)
	{
		clog << "Couldn't load MIME settings" << endl;
	}
	DownloaderInterface::initialize();
	// Load filter libraries, if any
	Dijon::FilterFactory::loadFilters(string(LIBDIR) + string("/pinot/filters"));
	Dijon::FilterFactory::loadFilters(confDirectory + "/filters");
	// Load backends, if any
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

	// Catch interrupts
#ifdef HAVE_SIGACTION
	struct sigaction newAction;
	sigemptyset(&newAction.sa_mask);
	newAction.sa_flags = 0;
	newAction.sa_handler = quitAll;
	sigaction(SIGINT, &newAction, NULL);
	sigaction(SIGQUIT, &newAction, NULL);
	sigaction(SIGTERM, &newAction, NULL);
#else
	signal(SIGINT, quitAll);
#ifdef SIGQUIT
	signal(SIGQUIT, quitAll);
#endif
	signal(SIGTERM, quitAll);
#endif

	// Is this a known index name ?
	PinotSettings::IndexProperties indexProps = settings.getIndexPropertiesByName(databaseName);
	if (indexProps.m_name.empty() == true)
	{
		// No, it's not
		indexProps.m_location = databaseName;
	}

	if (backendType.empty() == true)
	{
		backendType = settings.m_defaultBackend;
	}

	// Make sure the index is open in the correct mode
	bool wasObsoleteFormat = false;
	if (ModuleFactory::openOrCreateIndex(backendType, indexProps.m_location, wasObsoleteFormat, (indexDocument ? false : true)) == false)
	{
		clog << "Couldn't open index " << indexProps.m_location << endl;

		return EXIT_FAILURE;
	}

	// Do the same for the history database
	string historyDatabase(settings.getHistoryDatabaseName());
	if ((historyDatabase.empty() == true) ||
		(ActionQueue::create(historyDatabase) == false))
	{
		clog << "Couldn't create history database " << historyDatabase << endl;
		return EXIT_FAILURE;
	}
	else
	{
		ActionQueue actionQueue(historyDatabase, Glib::get_application_name());
		time_t timeNow = time(NULL);

		// Expire
		actionQueue.expireItems(timeNow);
	}

	atexit(closeAll);

	// Get a read-write index of the given type
	IndexInterface *pIndex = ModuleFactory::getIndex(backendType, indexProps.m_location);
	if (pIndex == NULL)
	{
		clog << "Couldn't obtain index for " << indexProps.m_location << endl;

		return EXIT_FAILURE;
	}

	while (optind < argc)
	{
		string urlParam(argv[optind]);
		Url thisUrl(urlParam, "");

		// Rewrite the URL, dropping user name and password which we don't support
		urlParam = thisUrl.getProtocol();
		urlParam += "://";
		if (thisUrl.isLocal() == false)
		{
			urlParam += thisUrl.getHost();
			urlParam += "/";
		}
		urlParam += thisUrl.getLocation();
		if (thisUrl.getFile().empty() == false)
		{
			urlParam += "/";
			urlParam += thisUrl.getFile();
		}
#ifdef DEBUG
		clog << "URL rewritten to " << urlParam << endl;
#endif

		DocumentInfo docInfo("", urlParam, MIMEScanner::scanUrl(thisUrl), "");
		unsigned int docId = 0;

		if (checkDocument == true)
		{
			if (pIndex->isGood() == true)
			{
				set<unsigned int> docIds;

				docId = pIndex->hasDocument(urlParam);
				if (docId > 0)
				{
					clog << urlParam << ": document ID " << docId << endl;
					success = true;
				}
				else if ((pIndex->listDocuments(urlParam, docIds, IndexInterface::BY_FILE, 100, 0) == true) &&
					(docIds.empty() == false))
				{
					docId = *(docIds.begin());

					clog << urlParam << ": document ID " << docId
						<< " and at least " << docIds.size() - 1 << " others" << endl;
					success = true;
				}
				else
				{
					clog << urlParam << ": not found" << endl;
				}
			}
		}
		if (indexDocument == true)
		{
			if (g_pState == NULL)
			{
				g_pState = new IndexingState(indexProps.m_location);
			}

			// Connect to threads' finished signal
			g_pState->connect();

			Glib::ustring status(g_pState->queue_index(docInfo));
			if (status.empty() == true)
			{
				// Run the main loop
				g_refMainLoop->run();
			}
			else
			{
				clog << status << endl;
			}

			// Stop everything
			g_pState->disconnect();

			docId = g_pState->getDocumentID();
			if (g_pState->mustQuit() == true)
			{
				break;
			}
		}
		if ((showInfo == true) &&
			(docId > 0))
		{
			set<string> labels;

			clog << "Index version       : " << pIndex->getMetadata("version") << endl;

			if (pIndex->getDocumentInfo(docId, docInfo) == true)
			{
				clog << "Location : '" << docInfo.getLocation(true) << "'" << endl;
				clog << "Title    : " << docInfo.getTitle() << endl;
				clog << "Type     : " << docInfo.getType() << endl;
				clog << "Language : " << docInfo.getLanguage() << endl;
				clog << "Date     : " << docInfo.getTimestamp() << endl;
				clog << "Size     : " << docInfo.getSize() << endl;
			}
			if (pIndex->getDocumentLabels(docId, labels) == true)
			{
				clog << "Labels   : ";
				for (set<string>::const_iterator labelIter = labels.begin();
					labelIter != labels.end(); ++labelIter)
				{
					if (labelIter->substr(0, 2) == "X-")
					{
						continue;
					}
					clog << "[" << Url::escapeUrl(*labelIter) << "]";
				}
				clog << endl;
			}

			vector<MIMEAction> typeActions;
			MIMEScanner::getDefaultActions(docInfo.getType(), thisUrl.isLocal(), typeActions);
			for (vector<MIMEAction>::const_iterator actionIter = typeActions.begin();
				actionIter != typeActions.end(); ++actionIter)
			{
				clog << "Action   : '" << actionIter->m_name << "' " << actionIter->m_exec << endl;
			}
		}

		// Next
		++optind;
	}
	delete pIndex;

	// Did whatever operation we carried out succeed ?
	if (success == true)
	{
		return EXIT_SUCCESS;
	}

	return EXIT_FAILURE;
}
