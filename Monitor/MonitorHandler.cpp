/*
 *  Copyright 2005,2006 Fabrice Colin
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

#include "MonitorHandler.h"

using namespace std;

MonitorHandler::MonitorHandler()
{
}

MonitorHandler::~MonitorHandler()
{
}

void MonitorHandler::initialize(void)
{
}

void MonitorHandler::flushIndex(void)
{
}

bool MonitorHandler::fileExists(const string &fileName)
{
	return false;
}

bool MonitorHandler::fileCreated(const string &fileName)
{
	return false;
}

bool MonitorHandler::directoryCreated(const string &dirName)
{
	return false;
}

bool MonitorHandler::fileModified(const string &fileName)
{
	return false;
}

bool MonitorHandler::fileMoved(const string &fileName,
	const string &previousFileName)
{
	return false;
}

bool MonitorHandler::directoryMoved(const string &dirName,
	const string &previousDirName)
{
	return false;
}

bool MonitorHandler::fileDeleted(const string &fileName)
{
	return false;
}

bool MonitorHandler::directoryDeleted(const string &dirName)
{
	return false;
}

const set<string> &MonitorHandler::getFileNames(void) const
{
	return m_fileNames;
}

