/*
 *  Copyright 2005-2009 Fabrice Colin
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
 
#ifndef _ONDISKHANDLER_HH
#define _ONDISKHANDLER_HH

#include <time.h>
#include <pthread.h>
#include <string>
#include <map>
#include <sigc++/sigc++.h>

#include "CrawlHistory.h"
#include "MetaDataBackup.h"
#include "IndexInterface.h"
#include "MonitorHandler.h"
#include "PinotSettings.h"

class OnDiskHandler : public MonitorHandler
{
	public:
		OnDiskHandler();
		virtual ~OnDiskHandler();

		/// Initializes things before starting monitoring.
		virtual void initialize(void);

		/// Handles file existence events.
		virtual bool fileExists(const std::string &fileName);

		/// Handles file creation events.
		virtual bool fileCreated(const std::string &fileName);

		/// Handles directory creation events.
		virtual bool directoryCreated(const std::string &dirName);

		/// Handles file modified events.
		virtual bool fileModified(const std::string &fileName);

		/// Handles file moved events.
		virtual bool fileMoved(const std::string &fileName,
			const std::string &previousFileName);

		/// Handles directory moved events.
		virtual bool directoryMoved(const std::string &dirName,
			const std::string &previousDirName);

		/// Handles file deleted events.
		virtual bool fileDeleted(const std::string &fileName);

		/// Handles directory deleted events.
		virtual bool directoryDeleted(const std::string &dirName);

		sigc::signal2<void, DocumentInfo, bool>& getFileFoundSignal(void);

	protected:
		pthread_mutex_t m_mutex;
		sigc::signal2<void, DocumentInfo, bool> m_signalFileFound;
		std::map<unsigned int, std::string> m_fileSources;
		CrawlHistory m_history;
		MetaDataBackup m_metaData;
		IndexInterface *m_pIndex;

		bool fileMoved(const std::string &fileName,
			const std::string &previousFileName,
			IndexInterface::NameType type);

		bool fileDeleted(const std::string &fileName, IndexInterface::NameType type);

		bool indexFile(const std::string &fileName, bool isDirectory, unsigned int &sourceId);

		bool replaceFile(unsigned int docId, DocumentInfo &docInfo);

	private:
		OnDiskHandler(const OnDiskHandler &other);
		OnDiskHandler &operator=(const OnDiskHandler &other);

};

#endif // _ONDISKHANDLER_HH
