/*
 *  Copyright 2005-2011 Fabrice Colin
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
#include <signal.h>
#include <unistd.h>
#include <libintl.h>
#include <getopt.h>
#include <strings.h>
#include <iostream>
#include <fstream>
#include <glibmm.h>
#include <glibmm/thread.h>
#include <glibmm/ustring.h>
#include <glibmm/miscutils.h>
#include <glibmm/convert.h>
#include "config.h"
#include <gtkmm/main.h>

#include "NLS.h"
#include "FilterFactory.h"
#include "Languages.h"
#include "MIMEScanner.h"
#include "ModuleFactory.h"
#include "ActionQueue.h"
#include "QueryHistory.h"
#include "ViewHistory.h"
#include "DownloaderInterface.h"
#ifdef HAVE_DBUS
#include "DBusIndex.h"
#endif
#include "PinotSettings.h"
#include "mainWindow.hh"
#include "prefsWindow.hh"

#define EXPIRY_PERIOD	(3600 * 24 * 30 * 6)

using namespace std;

static ofstream g_outputFile;
static streambuf *g_coutBuf = NULL;
static streambuf *g_cerrBuf = NULL;
static streambuf *g_clogBuf = NULL;
static struct option g_longOptions[] = {
	{"help", 0, 0, 'h'},
	{"preferences", 0, 0, 'p'},
	{"version", 0, 0, 'v'},
	{0, 0, 0, 0}
};

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

	DownloaderInterface::shutdown();
	MIMEScanner::shutdown();
}

static void quitAll(int sigNum)
{
	clog << "Quitting..." << endl;

	Gtk::Main::quit();
}

#ifdef HAVE_DBUS
static DBusHandlerResult filterHandler(DBusConnection *pConnection, DBusMessage *pMessage, void *pData)
{
	// Are we about to be disconnected ?
	if (dbus_message_is_signal(pMessage, DBUS_INTERFACE_LOCAL, "Disconnected") == TRUE)
	{
#ifdef DEBUG
		clog << "filterHandler: received Disconnected" << endl;
#endif
	}
	else if (dbus_message_is_signal(pMessage, DBUS_INTERFACE_DBUS, "NameOwnerChanged") == TRUE)
	{
#ifdef DEBUG
		clog << "filterHandler: received NameOwnerChanged" << endl;
#endif
	}
	else if (dbus_message_is_signal(pMessage, PINOT_DBUS_SERVICE_NAME, "IndexFlushed") == TRUE)
	{
		DBusError error;
		unsigned int docsCount = 0;

#ifdef DEBUG
		clog << "filterHandler: received IndexFlushed" << endl;
#endif
		dbus_error_init(&error);
		if ((dbus_message_get_args(pMessage, &error,
			DBUS_TYPE_UINT32, &docsCount,
			DBUS_TYPE_INVALID) == TRUE))
		{
			PinotSettings &settings = PinotSettings::getInstance();

#ifdef DEBUG
			clog << "filterHandler: reopening index, now with " << docsCount << " documents" << endl;
#endif
			IndexInterface *pIndex = settings.getIndex(settings.m_daemonIndexLocation);
			if (pIndex != NULL)
			{
				pIndex->reopen();

				delete pIndex;
			}
#ifdef DEBUG
			clog << "filterHandler: reopened index" << endl;
#endif
		}
		dbus_error_free(&error);

		return DBUS_HANDLER_RESULT_HANDLED;
	}

	return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}
#endif

int main(int argc, char **argv)
{
	string prefixDir(PREFIX);
	Glib::ustring errorMsg;
	int longOptionIndex = 0;
	bool warnAboutVersion = false, prefsMode = false;

	// Look at the options
	int optionChar = getopt_long(argc, argv, "hpv", g_longOptions, &longOptionIndex);
	while (optionChar != -1)
	{
		switch (optionChar)
		{
			case 'h':
				// Help
				clog << "pinot - A metasearch tool for the Free Desktop\n\n"
					<< "Usage: pinot [OPTIONS]\n\n"
					<< "Options:\n"
					<< "  -h, --help		display this help and exit\n"
					<< "  -p, --preferences		show preferences and exit\n"
					<< "  -v, --version		output version information and exit\n"
					<< "\nReport bugs to " << PACKAGE_BUGREPORT << endl;
				return EXIT_SUCCESS;
			case 'p':
				prefsMode = true;
				break;
			case 'v':
				clog << "pinot - " << PACKAGE_STRING << "\n\n" 
					<< "This is free software.  You may redistribute copies of it under the terms of\n"
					<< "the GNU General Public License <http://www.gnu.org/licenses/old-licenses/gpl-2.0.html>.\n"
					<< "There is NO WARRANTY, to the extent permitted by law." << endl;
				return EXIT_SUCCESS;
			default:
				return EXIT_FAILURE;
		}

		// Next option
		optionChar = getopt_long(argc, argv, "hpv", g_longOptions, &longOptionIndex);
	}

	string programName(argv[0]);
	if ((programName.length() >= 11) &&
		(programName.substr(programName.length() - 11) == "pinot-prefs"))
	{
		prefsMode = true;
	}

#if defined(ENABLE_NLS)
	bindtextdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
	textdomain(GETTEXT_PACKAGE);
#endif //ENABLE_NLS

	// Initialize threads support before doing anything else
	Glib::thread_init();
	// Initialize the GType and the D-Bus thread system
	g_type_init();
#ifdef HAVE_DBUS
#if DBUS_NUM_VERSION > 1000000
	dbus_threads_init_default();
#endif
	dbus_g_thread_init();
#endif

	Gtk::Main m(&argc, &argv);
	if (prefsMode == false)
	{
		Glib::set_application_name("Pinot GTK2 UI");
	}
	else
	{
		Glib::set_application_name("Pinot Preferences");
	}

	// This should make Xapian use Chert rather than Flint
	Glib::setenv("XAPIAN_PREFER_CHERT", "1");
	// This will be useful for indexes served by xapian-progsrv+ssh
	Glib::setenv("SSH_ASKPASS", prefixDir + "/libexec/openssh/ssh-askpass");

	// This is a hack to force the locale to UTF-8
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

	// This will create the necessary directories on the first run
	PinotSettings &settings = PinotSettings::getInstance();
#ifdef HAVE_DBUS
	// Talk to the daemon through DBus
	settings.enableClientMode(true);
#endif

	string confDirectory(PinotSettings::getConfigurationDirectory());
	if (chdir(confDirectory.c_str()) == 0)
	{
		// Redirect cout, cerr and clog to a file
		string logFileName = confDirectory;
		if (prefsMode == false)
		{
			logFileName += "/pinot.log";
		}
		else
		{
			logFileName += "/pinot-prefs.log";
		}
		g_outputFile.open(logFileName.c_str());
		g_coutBuf = clog.rdbuf();
		g_cerrBuf = clog.rdbuf();
		g_clogBuf = clog.rdbuf();
		clog.rdbuf(g_outputFile.rdbuf());
		clog.rdbuf(g_outputFile.rdbuf());
		clog.rdbuf(g_outputFile.rdbuf());
	}

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

	// Open this index read-write, unless we are in preferences mode
	bool wasObsoleteFormat = false;
	if (ModuleFactory::openOrCreateIndex(settings.m_defaultBackend, settings.m_docsIndexLocation, wasObsoleteFormat, prefsMode) == false)
	{
		errorMsg = _("Couldn't open index");
		errorMsg += " ";
		errorMsg += settings.m_docsIndexLocation;
	}
	else
	{
		warnAboutVersion = wasObsoleteFormat;
	}
	// ...and the daemon index in read-only mode
	// If it can't be open, it just means the daemon has not yet created it
	ModuleFactory::openOrCreateIndex(settings.m_defaultBackend, settings.m_daemonIndexLocation, wasObsoleteFormat, true);
	// Merge these two, this will be useful later
	ModuleFactory::mergeIndexes(settings.m_defaultBackend, "MERGED", settings.m_docsIndexLocation, settings.m_daemonIndexLocation);

	// Do the same for the history database
	string historyDatabase(settings.getHistoryDatabaseName());
	if ((historyDatabase.empty() == true) ||
		(ActionQueue::create(historyDatabase) == false) ||
		(QueryHistory::create(historyDatabase) == false) ||
		(ViewHistory::create(historyDatabase) == false))
	{
		errorMsg = _("Couldn't create history database");
		errorMsg += " ";
		errorMsg += historyDatabase;
	}
	else
	{
		ActionQueue actionQueue(historyDatabase, Glib::get_prgname());
		QueryHistory queryHistory(historyDatabase);
		ViewHistory viewHistory(historyDatabase);
		time_t timeNow = time(NULL);

		// Expire all actions left from last time
		actionQueue.expireItems(timeNow);
		// Expire items
		queryHistory.expireItems(timeNow - EXPIRY_PERIOD);
		viewHistory.expireItems(timeNow - EXPIRY_PERIOD);
	}

	atexit(closeAll);

	if (prefsMode == false)
	{
#ifdef HAVE_DBUS
		GError *pError = NULL;
		DBusGConnection *pBus = dbus_g_bus_get(DBUS_BUS_SESSION, &pError);
		DBusConnection *pConnection = NULL;

		if (pBus == NULL)
		{
			if (pError != NULL)
			{
				clog << "Couldn't open bus connection: " << pError->message << endl;
				if (pError->message != NULL)
				{
					clog << "Error is " << pError->message << endl;
				}
				g_error_free(pError);
			}
		}
		else
		{
			pConnection = dbus_g_connection_get_connection(pBus);
		}

		if (pConnection != NULL)
		{
			DBusError error;

			dbus_error_init(&error);
			dbus_connection_set_exit_on_disconnect(pConnection, FALSE);
			dbus_connection_setup_with_g_main(pConnection, NULL);

			string rule("type='signal',interface='");
			rule += PINOT_DBUS_SERVICE_NAME;
			rule += "'";

			// See signals coming from the daemon
			dbus_bus_add_match(pConnection, rule.c_str(), &error);

			dbus_connection_add_filter(pConnection,
				(DBusHandleMessageFunction)filterHandler, NULL, NULL);

			dbus_error_free(&error);
		}
#endif

		IndexInterface *pIndex = settings.getIndex(settings.m_docsIndexLocation);
		if (pIndex != NULL)
		{
			string indexVersion(pIndex->getMetadata("version"));

			// What version is the index at ?
			if (indexVersion.empty() == true)
			{
				indexVersion = "0.0";
			}
			// Is an upgrade necessary ?
			if ((indexVersion < PINOT_INDEX_MIN_VERSION) &&
				(pIndex->getDocumentsCount() > 0))
			{
				warnAboutVersion = true;
			}
#ifdef DEBUG
			clog << "My Web Pages was set to version " << indexVersion << endl;
#endif
			pIndex->setMetadata("version", VERSION);

			delete pIndex;
		}
		if (warnAboutVersion == true)
		{
			settings.m_warnAboutVersion = warnAboutVersion;
		}
	}

	try
	{
		// Set an icon for all windows
		Gtk::Window::set_default_icon_from_file(prefixDir + "/share/icons/hicolor/48x48/apps/pinot.png");

		// Create and open the window
		if (prefsMode == false)
		{ 
			mainWindow mainBox;
			if (errorMsg.empty() == false)
			{
				mainBox.set_status(errorMsg);
			}
			m.run(mainBox);
		}
		else
		{
			prefsWindow prefsBox;
			m.run(prefsBox);
		}
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

	return EXIT_SUCCESS;
}
