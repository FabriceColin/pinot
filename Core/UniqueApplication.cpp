/*
 *  Copyright 2008-2009 Fabrice Colin
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

#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <iostream>
#include <fstream>
#include <sstream>

#include "config.h"
#include "UniqueApplication.h"

using std::clog;
using std::clog;
using std::endl;
using std::string;
using std::fstream;
using std::stringstream;

UniqueApplication::UniqueApplication(const string &name)
{
#ifdef HAVE_UNIQUE
	m_pApp = unique_app_new(name.c_str(), NULL);
#ifdef DEBUG
	if (m_pApp != NULL)
	{
		clog << "UniqueApplication: registered" << endl;
	}
	else
	{
		clog << "UniqueApplication: failed to register" << endl;
	}
#endif
#endif
}

UniqueApplication::~UniqueApplication()
{
#ifdef HAVE_UNIQUE
	if (m_pApp != NULL)
	{
		g_object_unref(m_pApp);
	}
#endif
}

bool UniqueApplication::isRunning(void)
{
#ifdef HAVE_UNIQUE
	if ((m_pApp != NULL) &&
		(unique_app_is_running(m_pApp) == TRUE))
	{
		return true;
	}
#endif

	return false;
}

bool UniqueApplication::isRunning(const string &pidFileName, const string &processName)
{
	fstream pidFile;

	// Open the PID file
	pidFile.open(pidFileName.c_str(), std::ios::in);
	if (pidFile.is_open() == false)
	{
		// The application may still be running even though the PID file doesn't exist
		if (isRunning() == true)
		{
			return true;
		}
		// Keep going
	}
	else
	{
		pid_t processID = 0;
		bool stillRunning = false, processDied = false;

		pidFile >> processID;
		pidFile.close();

		// Is another process running ?
		if (processID > 0)
		{
#ifdef HAVE_UNIQUE
			if (isRunning() == true)
			{
				// It's still running
				stillRunning = true;
			}
			else
			{
				// It most likely died
				processDied = true;
			}
#else
			fstream cmdLineFile;
			stringstream cmdLineFileName;
			bool checkProcess = true;

			// FIXME: check for existence of /proc
			cmdLineFileName << "/proc/" << processID << "/cmdline";
			cmdLineFile.open(cmdLineFileName.str().c_str(), std::ios::in);
			if (cmdLineFile.is_open() == true)
			{
				string cmdLine;

				cmdLineFile >> cmdLine;
				cmdLineFile.close();

				if (cmdLine.find(processName) == string::npos)
				{
					// It's another process
					checkProcess = false;
					processDied = true;
				}
			}

#ifdef HAVE_KILL
			if (checkProcess == true)
			{
				if (kill(processID, 0) == 0)
				{
					// It's still running
					stillRunning = true;
				}
				else if (errno == ESRCH)
				{
					// This PID doesn't exist
					processDied = true;
				}
			}
#endif
#endif

			if (stillRunning == true)
			{
				clog << "Process " << processName << " (" << processID << ") is still running" << endl;
				return true;
			}

			if (processDied == true)
			{
				clog << "Previous instance " << processID << " died prematurely" << endl;
			}
		}
	}

	// Now save our PID
	pidFile.open(pidFileName.c_str(), std::ios::out);
	if (pidFile.is_open() == true)
	{
		pidFile << getpid() << endl;
		pidFile.close();
	}

	return false;
}

