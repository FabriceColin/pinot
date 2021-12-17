/*
 *  Copyright 2007-2021 Fabrice Colin
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
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
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
#include <string.h>
#include <sstream>
#include <algorithm>
#include <iostream>
#include <libxml/xmlreader.h>

#include "ExternalFilter.h"

using std::clog;
using std::endl;
using std::min;
using std::string;
using std::set;
using std::map;

using namespace Dijon;

#ifdef _DYNAMIC_DIJON_FILTERS
DIJON_FILTER_EXPORT bool get_filter_types(MIMETypes &mime_types)
{
#ifdef _DIJON_EXTERNALFILTER_CONFFILE
	ExternalFilter::initialize(_DIJON_EXTERNALFILTER_CONFFILE, mime_types);
#else
	ExternalFilter::initialize("/etc/dijon/external-filters.xml", mime_types);
#endif

	return true;
}

DIJON_FILTER_EXPORT bool check_filter_data_input(int data_input)
{
	Filter::DataInput input = (Filter::DataInput)data_input;

	if (input == Filter::DOCUMENT_FILE_NAME)
	{
		return true;
	}

	return false;
}

DIJON_FILTER_EXPORT Filter *get_filter(void)
{
	return new ExternalFilter();
}
#endif

// This function is heavily inspired by Xapian Omega's shell_protect()
static string shell_protect(const string &file_name)
{
	string safefile(file_name);
	string::size_type p = 0;

	if ((safefile.empty() == false) &&
		(safefile[0] == '-'))
	{
		// If the filename starts with a '-', protect it from being treated as
		// an option by prepending "./".
		safefile.insert(0, "./");
		p = 2;
	}

	while (p < safefile.size())
	{
		// Don't escape some safe characters which are common in filenames.
		unsigned char ch = safefile[p];

		if ((isalnum(ch) == 0) &&
			(strchr("/._-", ch) == NULL))
		{
			safefile.insert(p, "\\");
			++p;
		}
		++p;
	}

	return safefile;
}

map<string, string> ExternalFilter::m_commandsByType;
map<string, string> ExternalFilter::m_outputsByType;
map<string, string> ExternalFilter::m_charsetsByType;

ExternalFilter::ExternalFilter() :
	FileOutputFilter(),
	m_maxSize(0),
	m_doneWithDocument(false)
{
}

ExternalFilter::~ExternalFilter()
{
	rewind();
}

bool ExternalFilter::is_data_input_ok(DataInput input) const
{
	if (input == DOCUMENT_FILE_NAME)
	{
		return true;
	}

	return false;
}

bool ExternalFilter::set_property(Properties prop_name, const string &prop_value)
{
	if ((prop_name == MAXIMUM_NESTED_SIZE) &&
		(prop_value.empty() == false))
	{
		m_maxSize = (off_t)atoll(prop_value.c_str());
	}

	return true;
}

bool ExternalFilter::set_document_data(const char *data_ptr, off_t data_length)
{
	return false;
}

bool ExternalFilter::set_document_string(const string &data_str)
{
	return false;
}

bool ExternalFilter::set_document_uri(const string &uri)
{
	return false;
}

bool ExternalFilter::has_documents(void) const
{
	if ((m_doneWithDocument == false) &&
		(m_filePath.empty() == false))
	{
		return true;
	}

	return false;
}

bool ExternalFilter::next_document(void)
{
	if ((m_doneWithDocument == false) &&
		(m_mimeType.empty() == false) &&
		(m_filePath.empty() == false) &&
		(m_commandsByType.empty() == false))
	{
		string outputType("text/plain");
		ssize_t maxSize = 0;

		m_doneWithDocument = true;

		// Is this type supported ? Assume text/plain if not specified
		map<string, string>::const_iterator commandIter = m_commandsByType.find(m_mimeType);
		if ((commandIter == m_commandsByType.end()) ||
			(commandIter->second.empty() == true))
		{
			return false;
		}

		// What's the output type ?
		map<string, string>::const_iterator outputIter = m_outputsByType.find(m_mimeType);
		if (outputIter != m_outputsByType.end())
		{
			outputType = outputIter->second;
		}

		if (outputType != "text/plain")
		{
			maxSize = m_maxSize;
		}

		if (run_command(commandIter->second, maxSize) == true)
		{
			// Fill in general details
			m_metaData["uri"] = "file://" + m_filePath;
			m_metaData["mimetype"] = outputType;
			// Is it in a known charset ?
			map<string, string>::const_iterator charsetIter = m_charsetsByType.find(m_mimeType);
			if (charsetIter != m_charsetsByType.end())
			{
				m_metaData["charset"] = charsetIter->second;
			}

			return true;
		}

		return false;
	}

	rewind();

	return false;
}

bool ExternalFilter::skip_to_document(const string &ipath)
{
	if (ipath.empty() == true)
	{
		return next_document();
	}

	return false;
}

string ExternalFilter::get_error(void) const
{
	return "";
}

void ExternalFilter::initialize(const string &config_file, MIMETypes &types)
{
	xmlDoc *pDoc = NULL;
	xmlNode *pRootElement = NULL;

	types.m_mimeTypes.clear();

	// Parse the file and get the document
#if LIBXML_VERSION < 20600
	pDoc = xmlParseFile(config_file.c_str());
#else
	pDoc = xmlReadFile(config_file.c_str(), NULL, XML_PARSE_NOCDATA);
#endif
	if (pDoc == NULL)
	{
		return;
	}

	// Iterate through the root element's nodes
	pRootElement = xmlDocGetRootElement(pDoc);
	for (xmlNode *pCurrentNode = pRootElement->children; pCurrentNode != NULL;
		pCurrentNode = pCurrentNode->next)
	{
		// What type of tag is it ?
		if (pCurrentNode->type != XML_ELEMENT_NODE)
		{
			continue;
		}

		// Get all filter elements
		if (xmlStrncmp(pCurrentNode->name, BAD_CAST"filter", 6) == 0)
		{
			string mimeType, charset, command, arguments, output;

			for (xmlNode *pCurrentCodecNode = pCurrentNode->children;
				pCurrentCodecNode != NULL; pCurrentCodecNode = pCurrentCodecNode->next)
			{
				if (pCurrentCodecNode->type != XML_ELEMENT_NODE)
				{
					continue;
				}

				char *pChildContent = (char*)xmlNodeGetContent(pCurrentCodecNode);
				if (pChildContent == NULL)
				{
					continue;
				}

				// Filters are keyed by their MIME type, "extension" is ignored
				if (xmlStrncmp(pCurrentCodecNode->name, BAD_CAST"mimetype", 8) == 0)
				{
					mimeType = pChildContent;
				}
				else if (xmlStrncmp(pCurrentCodecNode->name, BAD_CAST"charset", 7) == 0)
				{
					charset = pChildContent;
				}
				else if (xmlStrncmp(pCurrentCodecNode->name, BAD_CAST"command", 7) == 0)
				{
					command = pChildContent;
				}
				if (xmlStrncmp(pCurrentCodecNode->name, BAD_CAST"arguments", 9) == 0)
				{
					arguments = pChildContent;
				}
				else if (xmlStrncmp(pCurrentCodecNode->name, BAD_CAST"output", 6) == 0)
				{
					output = pChildContent;
				}

				// Free
				xmlFree(pChildContent);
			}

			if ((mimeType.empty() == false) &&
				(command.empty() == false) &&
				(arguments.empty() == false))
			{
#ifdef DEBUG
				clog << "ExternalFilter::initialize: " << mimeType << "="
					<< command << " " << arguments << endl;
#endif

				// Command to run
				m_commandsByType[mimeType] = command + " " + arguments;
				// Output
				if (output.empty() == false)
				{
					m_outputsByType[mimeType] = output;
				}
				// Charset
				if (charset.empty() == false)
				{
					m_charsetsByType[mimeType] = charset;
				}

				types.m_mimeTypes.insert(mimeType);
			}
		}
	}

	// Free the document
	xmlFreeDoc(pDoc);
}

void ExternalFilter::rewind(void)
{
	Filter::rewind();
	m_doneWithDocument = false;
}

// This function is heavily inspired by Xapian Omega's stdout_to_string()
bool ExternalFilter::run_command(const string &command, ssize_t maxSize)
{
	string commandLine(command);
	int fds[2];
	int status = 0;
	bool replacedParam = false, gotOutput = false;

	string::size_type argPos = commandLine.find("%s");
	while (argPos != string::npos)
	{
		string quotedFilePath(shell_protect(m_filePath));

		commandLine.replace(argPos, 2, quotedFilePath);
		replacedParam = true;

		// Next
		argPos = commandLine.find("%s", argPos + 1);
	}

	if (replacedParam == false)
	{
		// Append
		commandLine += " ";
		commandLine += shell_protect(m_filePath);
	}

	// We want to be able to get the exit status of the child process
	signal(SIGCHLD, SIG_DFL);

	if (socketpair(AF_UNIX, SOCK_STREAM, PF_UNSPEC, fds) < 0)
	{
		return false;
	}

#ifdef DEBUG
	clog << "ExternalFilter::run_command: running " << commandLine << endl;
#endif

	// Fork and execute the command
	pid_t childPid = fork();
	if (childPid == 0)
	{
		// Child process
		// Close the parent's side of the socket pair
		close(fds[0]);
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
	gotOutput = read_file(fds[0], maxSize, totalSize);

	// Close our side of the socket pair
	close(fds[0]);

	// Wait until the child terminates
	pid_t actualChildPid = waitpid(childPid, &status, 0);

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
			clog << "ExternalFilter::run_command: couldn't run " << command << endl;
#endif
			return false;
		}
	}
#ifdef SIGXCPU
	if (WIFSIGNALED(status) && WTERMSIG(status) == SIGXCPU)
	{
#ifdef DEBUG
		clog << "ExternalFilter::run_command: " << command << " consumed too much CPU" << endl;
#endif
		return false;
	}
#endif

	return true;
}

