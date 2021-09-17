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

#include "config.h"
#include <stdlib.h>
#include <libintl.h>
#include <getopt.h>
#ifdef HAVE_LINUX_SCHED_H
#include <linux/sched.h>
#include <sched.h>
#else
#include <sys/time.h>
#include <sys/resource.h>
#endif
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <string.h>
#include <strings.h>
#include <sigc++/sigc++.h>
#include <giomm/init.h>
#include <glibmm.h>

#include "NLS.h"
#include "FilterFactory.h"
#include "Languages.h"
#include "MIMEScanner.h"
#include "ModuleFactory.h"
#include "ActionQueue.h"
#include "CrawlHistory.h"
#include "MetaDataBackup.h"
#include "QueryHistory.h"
#include "ViewHistory.h"
#include "DownloaderInterface.h"
#include "DaemonState.h"
#include "PinotSettings.h"
#include "ServerThreads.h"
#include "UniqueApplication.h"

using namespace std;

static ofstream g_outputFile;
static string g_pidFileName;
static streambuf *g_coutBuf = NULL;
static streambuf *g_cerrBuf = NULL;
static streambuf *g_clogBuf = NULL;
static struct option g_longOptions[] = {
	{"help", 0, 0, 'h'},
	{"ignore-version", 0, 0, 'i'},
	{"priority", 1, 0, 'p'},
	{"reindex", 0, 0, 'r'},
	{"version", 0, 0, 'v'},
	{0, 0, 0, 0}
};
static Glib::RefPtr<Glib::MainLoop> g_refMainLoop;
static DaemonState *g_pState = NULL;

static void closeAll(void)
{
	clog << "Exiting..." << endl;

	// Close everything
	ModuleFactory::unloadModules();

	// Restore the stream buffers
	if (g_coutBuf != NULL)
	{
		clog.rdbuf(g_coutBuf);
	}
	if (g_cerrBuf != NULL)
	{
		clog.rdbuf(g_cerrBuf);
	}
	if (g_clogBuf != NULL)
	{
		clog.rdbuf(g_clogBuf);
	}
	g_outputFile.close();
	if (g_pidFileName.empty() == false)
	{
		unlink(g_pidFileName.c_str());
	}

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
#ifdef HAVE_DBUS
	string programName("pinot-dbus-daemon");
#else
	string programName("pinot-daemon");
#endif
	int longOptionIndex = 0, priority = 15;
	bool resetHistory = false;
	bool resetLabels = false;
	bool reindex = false;
	bool ignoreVersion = false;

	// Look at the options
	int optionChar = getopt_long(argc, argv, "hip:rv", g_longOptions, &longOptionIndex);
	while (optionChar != -1)
	{
		switch (optionChar)
		{
			case 'h':
				// Help
#ifdef HAVE_DBUS
				clog << programName << " - D-Bus search and index daemon\n\n"
#else
				clog << programName << " - Search and index daemon\n\n"
#endif
					<< "Usage: " << programName << " [OPTIONS]\n\n"
					<< "Options:\n"
					<< "  -h, --help		display this help and exit\n"
					<< "  -i, --ignore-version	ignore the index version number\n"
					<< "  -p, --priority	set the daemon's priority (default 15)\n"
					<< "  -r, --reindex		force a reindex\n"
					<< "  -v, --version		output version information and exit\n"
					<< "\nReport bugs to " << PACKAGE_BUGREPORT << endl;
				return EXIT_SUCCESS;
			case 'i':
				ignoreVersion = true;
				break;
			case 'p':
				if (optarg != NULL)
				{
					int newPriority = atoi(optarg);
					if ((newPriority >= -20) &&
						(newPriority < 20))
					{
						priority = newPriority;
					}
				}
				break;
			case 'r':
				reindex = true;
				break;
			case 'v':
				clog << programName << " - " << PACKAGE_STRING << "\n\n" 
					<< "This is free software.  You may redistribute copies of it under the terms of\n"
					<< "the GNU General Public License <http://www.gnu.org/licenses/old-licenses/gpl-2.0.html>.\n"
					<< "There is NO WARRANTY, to the extent permitted by law." << endl;
				return EXIT_SUCCESS;
			default:
				return EXIT_FAILURE;
		}

		// Next option
		optionChar = getopt_long(argc, argv, "hip:rv", g_longOptions, &longOptionIndex);
	}

#if defined(ENABLE_NLS)
	bindtextdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
	textdomain(GETTEXT_PACKAGE);
#endif //ENABLE_NLS

	Glib::init();
	Gio::init();
	// Initialize threads support before doing anything else
	if (Glib::thread_supported() == false)
	{
		Glib::thread_init();
	}
	// Initialize the GType and the D-Bus thread system
#if !GLIB_CHECK_VERSION(2,35,0)
	g_type_init();
#endif
#if DBUS_NUM_VERSION > 1000000
	dbus_threads_init_default();
#endif
#ifdef HAVE_DBUS
	dbus_g_thread_init();
#endif

	g_refMainLoop = Glib::MainLoop::create();
#ifdef HAVE_DBUS
	Glib::set_application_name("Pinot DBus Daemon");
#else
	Glib::set_application_name("Pinot Daemon");
#endif

	// Make the locale follow the environment variables
	setlocale(LC_ALL, "");
	char *pLocale = setlocale(LC_ALL, NULL);
	if (pLocale != NULL)
	{
		string locale(pLocale);

		if (locale != "C")
		{
			bool appendUTF8 = false;

			string::size_type pos = locale.find_last_of(".");
			if ((pos != string::npos) &&
				((strcasecmp(locale.substr(pos).c_str(), ".utf8") != 0) &&
				(strcasecmp(locale.substr(pos).c_str(), ".utf-8") != 0)))
			{
				locale.resize(pos);
				appendUTF8 = true;
			}

			if (appendUTF8 == true)
			{
				locale += ".UTF-8";

				pLocale = setlocale(LC_ALL, locale.c_str());
				if (pLocale != NULL)
				{
#ifdef DEBUG
					clog << "Changed locale to " << pLocale << endl;
#endif
				}
			}
		}
	}

	// Make sure only one instance runs
#ifdef HAVE_DBUS
	UniqueApplication uniqueApp("com.github.fabricecolin.PinotDBusDaemon");
#else
	UniqueApplication uniqueApp("com.github.fabricecolin.PinotDaemon");
#endif
	string confDirectory(PinotSettings::getConfigurationDirectory());
	g_pidFileName = confDirectory + "/" + programName + ".pid";
	if (chdir(confDirectory.c_str()) == 0)
	{
		if (uniqueApp.isRunning(g_pidFileName, programName) == true)
		{
			return EXIT_SUCCESS;
		}

		// Redirect cout, cerr and clog to a file
		string fileName(confDirectory);
		fileName += "/";
		fileName += programName;
		fileName += ".log";
		g_outputFile.open(fileName.c_str());
		g_coutBuf = cout.rdbuf();
		g_cerrBuf = cerr.rdbuf();
		g_clogBuf = clog.rdbuf();
		clog.rdbuf(g_outputFile.rdbuf());
		clog.rdbuf(g_outputFile.rdbuf());
		clog.rdbuf(g_outputFile.rdbuf());
	}
	else
	{
		// We can't rely on the PID file
		if (uniqueApp.isRunning() == true)
		{
			return EXIT_SUCCESS;
		}
	}

	// This will create the necessary directories on the first run
	PinotSettings &settings = PinotSettings::getInstance();
	// This is the daemon so disable client-side code 
	settings.setClientMode(false);

	// Initialize utility classes
	if (MIMEScanner::initialize(PinotSettings::getHomeDirectory() + "/.local",
		string(SHARED_MIME_INFO_PREFIX)) == false)
	{
		clog << "Couldn't load MIME settings" << endl;
	}
	DownloaderInterface::initialize();
	// Load filter libraries, if any
	Dijon::FilterFactory::loadFilters(string(LIBDIR) + "/pinot/filters");
	Dijon::FilterFactory::loadFilters(confDirectory + "/filters");
	// Load backends, if any
	ModuleFactory::loadModules(string(LIBDIR) + "/pinot/backends");
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

	// Open the daemon index in read-write mode 
	bool wasObsoleteFormat = false;
	if (ModuleFactory::openOrCreateIndex(settings.m_defaultBackend, settings.m_daemonIndexLocation, wasObsoleteFormat, false) == false)
	{
		clog << "Couldn't open index " << settings.m_daemonIndexLocation << endl;
		return EXIT_FAILURE;
	}
	if (wasObsoleteFormat == true)
	{
		resetHistory = resetLabels = true;
	}

	// Do the same for the history database
	PinotSettings::checkHistoryDatabase();
	string historyDatabase(settings.getHistoryDatabaseName());
	if ((historyDatabase.empty() == true) ||
		(ActionQueue::create(historyDatabase) == false) ||
		(CrawlHistory::create(historyDatabase) == false) ||
		(MetaDataBackup::create(historyDatabase) == false) ||
		(QueryHistory::create(historyDatabase) == false) ||
		(ViewHistory::create(historyDatabase) == false))
	{
		clog << "Couldn't create history database " << historyDatabase << endl;
		return EXIT_FAILURE;
	}
	else
	{
		ActionQueue actionQueue(historyDatabase, Glib::get_application_name());
		QueryHistory queryHistory(historyDatabase);
		ViewHistory viewHistory(historyDatabase);
		time_t timeNow = time(NULL);
		unsigned int actionsCount = actionQueue.getItemsCount(ActionQueue::INDEX);

		// Don't expire actions left from last time
		actionsCount += actionQueue.getItemsCount(ActionQueue::UNINDEX);
		clog << actionsCount << " actions left" << endl;

		// Expire the rest
		queryHistory.expireItems(timeNow);
		viewHistory.expireItems(timeNow);
	}

	atexit(closeAll);

#ifdef HAVE_LINUX_SCHED_H
	// Set the scheduling policy
	struct sched_param schedParam;
	if (sched_getparam(0, &schedParam) == -1)
	{
		clog << "Couldn't get current scheduling policy" << endl;
	}
	else if (sched_setscheduler(0, SCHED_IDLE, &schedParam) == -1)
	{
		clog << "Couldn't set scheduling policy" << endl;
	}
#else
	// Change the daemon's priority
	if (setpriority(PRIO_PROCESS, 0, priority) == -1)
	{
		clog << "Couldn't set scheduling priority to " << priority << endl;
	}
#endif

	DaemonState server;
	IndexInterface *pIndex = NULL;

	g_pState = &server;

	try
	{
		set<string> labels;
		bool gotLabels = false;

		server.register_session();

		pIndex = settings.getIndex(settings.m_daemonIndexLocation);
		if (pIndex != NULL)
		{
			string indexVersion(pIndex->getMetadata("version"));

			gotLabels = pIndex->getLabels(labels);

			// What version is the index at ?
			if (indexVersion.empty() == true)
			{
				indexVersion = "0.0";
			}
			if (ignoreVersion == true)
			{
				// Better reset labels, they may have been lost too
				resetLabels = true;
			}
			else if (indexVersion < PINOT_INDEX_MIN_VERSION)
			{
				clog << "Upgrading index from version " << indexVersion << " to " << VERSION << endl;

				reindex = true;
			}
			if (reindex == true)
			{
				// Reset the index so that all documents are reindexed
				pIndex->reset();
				pIndex->flush();
				server.emit_IndexFlushed(0);

				clog << "Reset index" << endl;

				resetHistory = resetLabels = true;
			}

			pIndex->setMetadata("version", VERSION);
			pIndex->setMetadata("dbus-status", "Running");
		}

		if (resetHistory == true)
		{
			CrawlHistory crawlHistory(historyDatabase);
			map<unsigned int, string> sources;

			// Reset the history
			crawlHistory.getSources(sources);
			for (std::map<unsigned int, string>::iterator sourceIter = sources.begin();
				sourceIter != sources.end(); ++sourceIter)
			{
				crawlHistory.deleteItems(sourceIter->first);
				crawlHistory.deleteSource(sourceIter->first);
			}

			clog << "Reset crawler history" << endl;
		}

		if ((resetLabels == true) &&
			(pIndex != NULL))
		{
			// Re-apply the labels list
			if (gotLabels == false)
			{
				// If this is an upgrade from a version < 0.80, the labels list
				// needs to be pulled from the configuration file
				pIndex->setLabels(settings.m_labels, true);

				clog << "Set labels as per the configuration file" << endl;
			}
			else
			{
				pIndex->setLabels(labels, true);
			}
		}

		// Connect to the quit signal
		server.getQuitSignal().connect(sigc::ptr_fun(&quitAll));

		// Connect to threads' finished signal
		server.connect();

		server.start(reindex);

		// Run the main loop
		g_refMainLoop->run();

	}
	catch (const Glib::Exception &e)
	{
		clog << e.what() << endl;
		return EXIT_FAILURE;
	}
	catch (const char *pMsg)
	{
		clog << pMsg << endl;
		return EXIT_FAILURE;
	}
	catch (...)
	{
		clog << "Unknown exception" << endl;
		return EXIT_FAILURE;
	}

	if (pIndex != NULL)
	{
		if (server.is_flag_set(DaemonState::DISCONNECTED) == true)
		{
			pIndex->setMetadata("dbus-status", "Disconnected");
		}
		else if (server.is_flag_set(DaemonState::STOPPED) == true)
		{
			pIndex->setMetadata("dbus-status", "Stopped");
		}
		else
		{
			pIndex->setMetadata("dbus-status", "");
		}
		delete pIndex;
	}

	// Stop everything
	server.disconnect();
	server.mustQuit(true);
	g_pState = NULL;

	return EXIT_SUCCESS;
}

