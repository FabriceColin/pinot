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
 
#include <getopt.h>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <set>
#include <giomm/init.h>
#include <glibmm/init.h>

#include "config.h"
#include "StringManip.h"
#include "MIMEScanner.h"
#include "Url.h"
#include "DBusIndex.h"

using namespace std;

static struct option g_longOptions[] = {
	{"get", 0, 0, 'g'},
	{"help", 0, 0, 'h'},
	{"list", 0, 0, 'l'},
	{"reload", 0, 0, 'r'},
	{"set", 1, 0, 's'},
	{"version", 0, 0, 'v'},
	{0, 0, 0, 0}
};


static void printLabels(const set<string> &labels, const string &fileName)
{
	if (fileName.empty() == false)
	{
		clog << fileName << endl;
	}
	clog << "Labels: ";

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

static void printHelp(void)
{
	// Help
	clog << "pinot-label - Label files from the command-line\n\n"
		<< "Usage: pinot-label [OPTIONS] [FILES]\n\n"
		<< "Options:\n"
		<< "  -g, --get                 get the labels list for the given file\n"
		<< "  -h, --help                display this help and exit\n"
		<< "  -l, --list                list known labels\n"
		<< "  -r, --reload              get the daemon to reload the configuration\n"
		<< "  -s, --set                 set labels on the given file\n"
		<< "  -v, --version             output version information and exit\n\n";
	clog << "Examples:\n"
		<< "pinot-label --get /home/fabrice/Documents/Bozo.txt\n\n"
		<< "pinot-label --list\n\n"
		<< "pinot-label --set \"[Clowns][Fun][My Hero]\" /home/fabrice/Documents/Bozo.txt\n\n"
		<< "Report bugs to " << PACKAGE_BUGREPORT << endl;
}

int main(int argc, char **argv)
{
	set<string> labels;
	string labelsString;
	int longOptionIndex = 0;
	unsigned int docId = 0;
	int minArgNum = 1;
	bool getLabels = false, getDocumentLabels = false, reloadIndex = false, setDocumentLabels = false, success = false;

	// Look at the options
	int optionChar = getopt_long(argc, argv, "ghlrs:v", g_longOptions, &longOptionIndex);
	while (optionChar != -1)
	{
		set<string> engines;

		switch (optionChar)
		{
			case 'g':
				getDocumentLabels = true;
				break;
			case 'h':
				printHelp();
				return EXIT_SUCCESS;
			case 'l':
				minArgNum = 0;
				getLabels = true;
				break;
			case 'r':
				minArgNum = 0;
				reloadIndex = true;
				break;
			case 's':
				setDocumentLabels = true;
				if (optarg != NULL)
				{
					labelsString = optarg;
				}
				break;
			case 'v':
				clog << "pinot-label - " << PACKAGE_STRING << "\n\n"
					<< "This is free software.  You may redistribute copies of it under the terms of\n"
					<< "the GNU General Public License <http://www.gnu.org/licenses/old-licenses/gpl-2.0.html>.\n"
					<< "There is NO WARRANTY, to the extent permitted by law." << endl;
				return EXIT_SUCCESS;
			default:
				return EXIT_FAILURE;
		}

		// Next option
		optionChar = getopt_long(argc, argv, "ghls:v", g_longOptions, &longOptionIndex);
	}

	if (argc == 1)
	{
		printHelp();
		return EXIT_SUCCESS;
	}

	if ((argc < 2) ||
		(argc - optind < minArgNum))
	{
		clog << "Not enough parameters" << endl;
		return EXIT_FAILURE;
	}

	if ((setDocumentLabels == true) &&
		(labelsString.empty() == true))
	{
		clog << "Incorrect parameters" << endl;
		return EXIT_FAILURE;
	}

	Glib::init();
	Gio::init();
	// Initialize GType
#if !GLIB_CHECK_VERSION(2,35,0)
	g_type_init();
#endif

	MIMEScanner::initialize("", "");

	// We need a pure DBusIndex object
	DBusIndex index(NULL);

	if (getLabels == true)
	{
		if (index.getLabels(labels) == true)
		{
			printLabels(labels, "");

			success = true;
		}
	}

	while (optind < argc)
	{
		string fileParam(argv[optind]);
		Url thisUrl(fileParam, "");

		// Rewrite it as a local URL
		string urlParam(thisUrl.getProtocol());
		urlParam += "://";
		urlParam += thisUrl.getLocation();
		if (thisUrl.getFile().empty() == false)
		{
			urlParam += "/";
			urlParam += thisUrl.getFile();
		}
#ifdef DEBUG
		clog << "URL rewritten to " << urlParam << endl;
#endif

		if ((getDocumentLabels == true) ||
			(setDocumentLabels == true))
		{
			docId = index.hasDocument(urlParam);
			if (docId == 0)
			{
				clog << fileParam << " is not indexed" << endl;
				success = false;

				// Next
				++optind;
				continue;
			}
		}

		if (getDocumentLabels == true)
		{
			labels.clear();

			if (index.getDocumentLabels(docId, labels) == true)
			{
				printLabels(labels, fileParam);

				success = true;
			}
		}

		if (setDocumentLabels == true)
		{
			string::size_type endPos = 0;
			string label(StringManip::extractField(labelsString, "[", "]", endPos));

			labels.clear();

			// Parse labels
			while (label.empty() == false)
			{
				labels.insert(Url::unescapeUrl(label));

				if (endPos == string::npos)
				{
					break;
				}
				label = StringManip::extractField(labelsString, "[", "]", endPos);
			}

#ifdef DEBUG
			printLabels(labels, fileParam);
#endif
			success = index.setDocumentLabels(docId, labels);
		}

		// Next
		++optind;
	}

	if (reloadIndex == true)
	{
		index.reload();
	}

	MIMEScanner::shutdown();

	// Did whatever operation we carried out succeed ?
	if (success == true)
	{
		return EXIT_SUCCESS;
	}

	return EXIT_FAILURE;
}
