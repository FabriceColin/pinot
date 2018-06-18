/*
 *  Copyright 2005-2012 Fabrice Colin
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
#include <strings.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_SOCKETPAIR
#ifdef HAVE_FORK
#ifdef HAVE_SETRLIMIT
#include <signal.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/resource.h>
#endif
#endif
#endif
#include <unistd.h>
#include <errno.h>
#include <glibmm/shell.h>
#include <glibmm/spawn.h>
#include <iostream>

#include "CommandLine.h"
#include "Url.h"

using std::clog;
using std::endl;
using std::string;
using std::vector;

CommandLine::CommandLine()
{
}

CommandLine::~CommandLine()
{
}

bool CommandLine::readFile(int fd, ssize_t maxSize,
	dstring &output, ssize_t &totalSize)
{
	struct stat fdStats;
	ssize_t bytesRead = 0;
	bool gotOutput = true;

#ifdef DEBUG
	if (fstat(fd, &fdStats) == 0)
	{
		clog << "CommandLine::readFile: file size " << fdStats.st_size << endl;
	}
#endif

	do
	{
		if ((maxSize > 0) &&
			(totalSize >= maxSize))
		{
#ifdef DEBUG
			clog << "CommandLine::readFile: stopping at " << totalSize << endl;
#endif
			break;
		}

		char readBuffer[4096];
		bytesRead = read(fd, readBuffer, 4096);
		if (bytesRead > 0)
		{
			output.append(readBuffer, bytesRead);
			totalSize += bytesRead;
		}
		else if (bytesRead == -1)
		{
			// An error occurred
			if (errno != EINTR)
			{
				gotOutput = false;
				break;
			}

			// Try again
			bytesRead = 1;
		}
	} while (bytesRead > 0);

	return gotOutput;
}

/// Quotes a string so that the shell will interpret the quoted string to mean str.
string CommandLine::quote(const string &str)
{
	return Glib::shell_quote(str);
}

/// Runs a command synchronously.
bool CommandLine::runSync(const string &commandLine, string &output)
{
	int exitStatus = 0;

	if (commandLine.empty() == true)
	{
		return false;
	}

	Glib::spawn_command_line_sync(commandLine, &output, NULL, &exitStatus);
	if (exitStatus == 0)
	{
		return true;
	}
#ifdef DEBUG
	clog << "CommandLine::runSync: exit status is " << exitStatus
		<< " for \"" << commandLine << "\"" << endl;
#endif

	return false;
}

/**
 * Runs a command synchronously.
 * This function is heavily inspired by Xapian Omega's stdout_to_string()
 */
bool CommandLine::runSync(const string &commandLine, ssize_t maxSize,
	const string &input, dstring &output)
{
	char inTemplate[20] = "/tmp/command_XXXXXX";
	int fds[2];
	int inFd = -1, status = 0;
	bool gotOutput = false;

	// We want to be able to get the exit status of the child process
	signal(SIGCHLD, SIG_DFL);

	if (socketpair(AF_UNIX, SOCK_STREAM, PF_UNSPEC, fds) < 0)
	{
		return false;
	}

	// Any input ?
	if (input.empty() == false)
	{
		inFd = mkstemp(inTemplate);
		if (inFd == -1)
		{
			return false;
		}

		if (write(inFd, (const void*)input.c_str(), input.length()) < (ssize_t)input.length())
		{
			close(inFd);
			return false;
		}
		else
		{
			lseek(inFd, 0, SEEK_SET);
		}
	}

#ifdef DEBUG
	clog << "CommandLine::runSync: running " << commandLine << endl;
#endif
	// Fork and execute the command
	pid_t childPid = fork();
	if (childPid == 0)
	{
		// Child process
		// Close the parent's side of the socket pair
		close(fds[0]);
		if (inFd > -1)
		{
			// Connect stdin
			dup2(inFd, 0);
		}
		// Connect stdout, stderr and stdlog to our side of the socket pair
		dup2(fds[1], 1);
		dup2(fds[1], 2);
		dup2(fds[1], 3);

		// Limit CPU time for external programs to 300 seconds
		struct rlimit cpu_limit = { 300, RLIM_INFINITY } ;
		setrlimit(RLIMIT_CPU, &cpu_limit);

		execl("/bin/sh", "/bin/sh", "-c", commandLine.c_str(), (void*)NULL);
		exit(-1);
	}

	// Parent process
	// Close the child's side of the socket pair
	close(fds[1]);
	if (childPid == -1)
	{
		// The fork failed
		close(fds[0]);
		return false;
	}

	ssize_t totalSize = 0;
	gotOutput = readFile(fds[0], maxSize, output, totalSize);

	// Close our side of the socket pair
	close(fds[0]);

	// Wait until the child terminates
	pid_t actualChildPid = waitpid(childPid, &status, 0);

	if (inFd > -1)
	{
		close(inFd);
		unlink(inTemplate);
	}

	if ((gotOutput == false) ||
		(actualChildPid == -1))
	{
		return false;
	}

	if (status != 0)
	{
		if (WIFEXITED(status) && WEXITSTATUS(status) == 127)
		{
#ifdef DEBUG
			clog << "CommandLine::runSync: couldn't run " << commandLine << endl;
#endif
			return false;
		}
	}
#ifdef SIGXCPU
	if (WIFSIGNALED(status) && WTERMSIG(status) == SIGXCPU)
	{
#ifdef DEBUG
		clog << "CommandLine::runSync: " << commandLine << " consumed too much CPU" << endl;
#endif
		return false;
	}
#endif

	return true;
}

/// Runs a command asynchronously.
bool CommandLine::runAsync(const MIMEAction &action, const vector<string> &arguments)
{
#ifdef USE_GIO
	if (action.m_pAppInfo == NULL)
	{
		return false;
	}

	GList *pFilesList = NULL;

	vector<string>::const_iterator firstArg = arguments.begin();
	while (firstArg != arguments.end())
	{
		Url firstUrl(*firstArg);
		string protocol(firstUrl.getProtocol());

		if (action.m_localOnly == false)
		{
			pFilesList = g_list_prepend(pFilesList, g_strdup((*firstArg).c_str()));
		}
		else if (protocol == "file")
		{
			pFilesList = g_list_prepend(pFilesList, g_file_new_for_uri((*firstArg).c_str()));
		}
#ifdef DEBUG
		else clog << "CommandLine::runAsync: can't open URL " << *firstArg << endl;
#endif

		// Next
		++firstArg;
	}

	GError *pError = NULL;
	gboolean launched = FALSE;

	if (action.m_localOnly == false)
	{
		launched = g_app_info_launch_uris(action.m_pAppInfo, pFilesList, NULL, &pError);
	}
	else
	{
		launched = g_app_info_launch(action.m_pAppInfo, pFilesList, NULL, &pError);
	}

	if (action.m_localOnly == false)
	{
		g_list_foreach(pFilesList, (GFunc)g_free, NULL);
	}
	else
	{
		g_list_foreach(pFilesList, (GFunc)g_object_unref, NULL);
	}
	g_list_free(pFilesList);

	if (launched == FALSE)
	{
		return false;
	}
#else
	string commandLine(action.m_exec);

	if (action.m_exec.empty() == true)
	{
		return false;
	}

	string::size_type equalPos = action.m_exec.find("=");
	if (equalPos != string::npos)
	{
		// The .desktop spec says that the program name or path in Exec may not contain it
		// but that's ignored by some .desktop files. So it could be either of :
		//  FOO=bar my program				- not valid
		//  sh -c "FOO=bar exec myprogram"		- valid
		//  myprogram --option FOO=bar			- valid
		// There's no easy way to find out which, so be lenient and use system()
		commandLine = "sh -c \"";
		commandLine += action.m_exec;
		commandLine += "\"";
	}

	if (commandLine.empty() == true)
	{
		return false;
	}
#ifdef DEBUG
	clog << "CommandLine::runAsync: " << arguments.size() << " arguments for application '"
		<< action.m_exec << "'" << endl;
#endif

	// We may have to spawn several copies of the same program if it doesn't support multiple arguments
	vector<string>::const_iterator firstArg = arguments.begin();
	while (firstArg != arguments.end())
	{
		Url firstUrl(*firstArg);
		bool foundParam = false;
		bool noArgument = true;
		bool oneArgumentOnly = true;

		// Expand parameters
		// See http://standards.freedesktop.org/desktop-entry-spec/latest/
		// We assume that all arguments are full-blown URLs
		string::size_type paramPos = commandLine.find('%');
		while ((paramPos != string::npos) &&
				(paramPos + 1 < commandLine.length()))
		{
			string shellReplacement, replacement;

			foundParam = true;
			switch (commandLine[paramPos + 1])
			{
				// Single parameter arguments
				case 'u':
					replacement = *firstArg;
					noArgument = false;
					break;
				case 'f':
					if (firstUrl.getProtocol() == "file")
					{
						replacement = firstUrl.getLocation();
						replacement += "/";
						replacement += firstUrl.getFile();
					}
					noArgument = false;
					break;
				case 'd':
					if (firstUrl.getProtocol() == "file")
					{
						replacement = firstUrl.getLocation();
					}
					noArgument = false;
					break;
				case 'n':
					if (firstUrl.getProtocol() == "file")
					{
						replacement = firstUrl.getFile();
					}
					noArgument = false;
					break;
				// Multiple parameters arguments
				case 'U':
					for (vector<string>::const_iterator argIter = firstArg; argIter != arguments.end(); ++argIter)
					{
						if (replacement.empty() == false)
						{
							replacement += " ";
						}
						replacement += *argIter;
					}
					noArgument = oneArgumentOnly = false;
					break;
				case 'F':
					for (vector<string>::const_iterator argIter = firstArg; argIter != arguments.end(); ++argIter)
					{
						Url argUrl(*argIter);

						if (argUrl.isLocal() == true)
						{
							if (replacement.empty() == false)
							{
								replacement += " ";
							}
							replacement += argUrl.getLocation();
							replacement += "/";
							replacement += argUrl.getFile();
						}
					}
					noArgument = oneArgumentOnly = false;
					break;
				case 'D':
					for (vector<string>::const_iterator argIter = firstArg; argIter != arguments.end(); ++argIter)
					{
						Url argUrl(*argIter);

						if (argUrl.isLocal() == true)
						{
							if (replacement.empty() == false)
							{
								replacement += " ";
							}
							replacement += argUrl.getLocation();
						}
					}
					noArgument = oneArgumentOnly = false;
					break;
				case 'N':
					for (vector<string>::const_iterator argIter = firstArg; argIter != arguments.end(); ++argIter)
					{
						Url argUrl(*argIter);

						if (argUrl.isLocal() == true)
						{
							if (replacement.empty() == false)
							{
								replacement += " ";
							}
							replacement += argUrl.getFile();
						}
					}
					noArgument = oneArgumentOnly = false;
					break;
				// Other parameters
				case 'i':
					replacement = action.m_icon;
					break;
				case 'c':
					// FIXME: this should be the "translated name"
					replacement = action.m_name;
					break;
				case 'k':
					replacement = action.m_location;
					break;
				case 'v':
					replacement = action.m_device;
					break;
				default:
					break;
			}

			if (replacement.empty() == false)
			{
				shellReplacement = quote(replacement);
			}
			commandLine.replace(paramPos, 2, shellReplacement);

			// Next
			paramPos = commandLine.find('%', paramPos + shellReplacement.length());
		}

		if (foundParam == false)
		{
			// If no parameter was found, assume %f
			if (firstUrl.getProtocol() == "file")
			{
				commandLine += " ";
				commandLine += firstUrl.getLocation();
				commandLine += "/";
				commandLine += firstUrl.getFile();
			}
		}

#ifdef DEBUG
		clog << "CommandLine::runAsync: spawning '" << commandLine << "'" << endl;
#endif
		Glib::spawn_command_line_async(commandLine);

		if ((noArgument == true) ||
			(oneArgumentOnly == false))
		{
			// No or all arguments were processed
			break;
		}
		else
		{
			// Only the first argument was processed
			commandLine = action.m_exec;
			++firstArg;
		}

	}
#endif

	return true;
}
