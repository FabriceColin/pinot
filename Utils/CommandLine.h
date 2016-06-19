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

#ifndef _COMMAND_LINE_H
#define _COMMAND_LINE_H

#include <string>
#include <vector>

#include "Memory.h"
#include "MIMEScanner.h"
#include "Visibility.h"

/// Utilities for running commands.
class PINOT_EXPORT CommandLine
{
	public:
		~CommandLine();

		/// Quotes a string so that the shell will interpret the quoted string to mean str.
		static std::string quote(const std::string &str);

		/// Runs a command synchronously.
		static bool runSync(const std::string &commandLine,
			std::string &output);

		/// Runs a command synchronously.
		static bool runSync(const std::string &commandLine, ssize_t maxSize,
			const std::string &input, dstring &output);

		/// Runs a command asynchronously.
		static bool runAsync(const MIMEAction &action,
			const std::vector<std::string> &arguments);

	protected:
		CommandLine();

		static bool readFile(int fd, ssize_t maxSize,
			dstring &output, ssize_t &totalSize);

	private:
		CommandLine(const CommandLine &other);
		CommandLine& operator=(const CommandLine& other);

};

#endif // _COMMAND_LINE_H
