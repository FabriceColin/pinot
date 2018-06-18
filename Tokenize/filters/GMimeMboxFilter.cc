/*
 *  Copyright 2007-2016 Fabrice Colin
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#ifdef HAVE_MMAP
#include <sys/mman.h>
#endif
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <iostream>
#include <algorithm>

#include <gmime/gmime.h>

#include "GMimeMboxFilter.h"

using std::clog;
using std::endl;
using std::string;
using std::max;
using std::map;
using std::set;
using std::pair;
using namespace Dijon;

#ifdef _DYNAMIC_DIJON_FILTERS
DIJON_FILTER_EXPORT bool get_filter_types(MIMETypes &mime_types)
{
	mime_types.m_mimeTypes.clear();
	mime_types.m_mimeTypes.insert("application/mbox");
	mime_types.m_mimeTypes.insert("text/x-mail");
	mime_types.m_mimeTypes.insert("text/x-news");

	return true;
}

DIJON_FILTER_EXPORT bool check_filter_data_input(int data_input)
{
	Filter::DataInput input = (Filter::DataInput)data_input;

	if ((input == Filter::DOCUMENT_DATA) ||
		(input == Filter::DOCUMENT_FILE_NAME))
	{
		return true;
	}

	return false;
}

DIJON_FILTER_EXPORT Filter *get_filter(void)
{
	return new GMimeMboxFilter();
}

DIJON_FILTER_INITIALIZE void initialize_gmime(void)
{
	// Initialize gmime
#ifdef GMIME_ENABLE_RFC2047_WORKAROUNDS
	g_mime_init(GMIME_ENABLE_RFC2047_WORKAROUNDS);
#else
	g_mime_init(GMIME_INIT_FLAG_UTF8);
#endif
}

DIJON_FILTER_SHUTDOWN void shutdown_gmime(void)
{
	// Shutdown gmime
	g_mime_shutdown();
}
#endif

static string extractField(const string &str,
	const string &start, const string &end,
	string::size_type &endPos, bool anyCharacterOfEnd = false)
{
	string fieldValue;
	string::size_type startPos = string::npos;

	if (start.empty() == true)
	{
		startPos = 0;
	}
	else
	{
		startPos = str.find(start, endPos);
	}

	if (startPos != string::npos)
	{
		startPos += start.length();

		if (end.empty() == true)
		{
			fieldValue = str.substr(startPos);
		}
		else
		{
			if (anyCharacterOfEnd == false)
			{
				endPos = str.find(end, startPos);
			}
			else
			{
				endPos = str.find_first_of(end, startPos);
			}
			if (endPos != string::npos)
			{
				fieldValue = str.substr(startPos, endPos - startPos);
			}
		}
	}

	return fieldValue;
}

GMimeMboxFilter::GMimeMboxPart::GMimeMboxPart(const string &subject,
	dstring &buffer) :
	m_subject(subject),
	m_buffer(buffer)
{
}

GMimeMboxFilter::GMimeMboxPart::~GMimeMboxPart()
{
}

GMimeMboxFilter::GMimeMboxFilter() :
	Filter(),
	m_returnHeaders(false),
	m_maxSize(0),
	m_pData(NULL),
	m_dataLength(0),
	m_fd(-1),
	m_pGMimeMboxStream(NULL),
	m_pParser(NULL),
	m_pMimeMessage(NULL),
	m_partsCount(-1),
	m_partNum(-1),
	m_partLevel(-1),
	m_currentLevel(0),
	m_messageStart(0),
	m_foundDocument(false)
{
}

GMimeMboxFilter::~GMimeMboxFilter()
{
	finalize(true);
}

bool GMimeMboxFilter::is_data_input_ok(DataInput input) const
{
	if ((input == DOCUMENT_DATA) ||
		(input == DOCUMENT_FILE_NAME))
	{
		return true;
	}

	return false;
}

bool GMimeMboxFilter::set_property(Properties prop_name, const string &prop_value)
{
	if (prop_name == PREFERRED_CHARSET)
	{
		m_defaultCharset = prop_value;

		return true;
	}
	else if (prop_name == OPERATING_MODE)
	{
		if (prop_value == "view")
		{
			m_returnHeaders = true;
		}
		else
		{
			m_returnHeaders = false;
		}

		return true;
	}
	else if ((prop_name == MAXIMUM_NESTED_SIZE) &&
		(prop_value.empty() == false))
	{
		m_maxSize = (off_t)atoll(prop_value.c_str());
	}

	return false;
}

bool GMimeMboxFilter::set_document_data(const char *data_ptr, off_t data_length)
{
	// Close/free whatever was opened/allocated on a previous call to set_document()
	finalize(true);
	m_partsCount = m_partNum = m_partLevel = -1;
	m_levels.clear();
	m_messageStart = 0;
	m_messageDate.clear();
	m_partCharset.clear();
	m_foundDocument = false;

	m_pData = data_ptr;
	m_dataLength = data_length;

	// Assume there are documents if initialization is successful
	// but don't actually retrieve anything, until next or skip is called
	if (initializeData() == true)
	{
		m_foundDocument = initialize();
	}

	return m_foundDocument;
}

bool GMimeMboxFilter::set_document_string(const string &data_str)
{
	return false;
}

bool GMimeMboxFilter::set_document_file(const string &file_path, bool unlink_when_done)
{
	// Close/free whatever was opened/allocated on a previous call to set_document()
	finalize(true);
	m_partsCount = m_partNum = m_partLevel = -1;
	m_levels.clear();
	m_messageStart = 0;
	m_messageDate.clear();
	m_partCharset.clear();
	m_foundDocument = false;

	Filter::set_document_file(file_path, unlink_when_done);

	// Assume there are documents if initialization is successful
	// but don't actually retrieve anything, until next or skip is called
	if (initializeFile() == true)
	{
		m_foundDocument = initialize();
	}

	return m_foundDocument;
}

bool GMimeMboxFilter::set_document_uri(const string &uri)
{
	return false;
}

bool GMimeMboxFilter::has_documents(void) const
{
	// As long as a document was found, chances are another one is available
	return m_foundDocument;
}

bool GMimeMboxFilter::next_document(void)
{
	string subject;

	map<string, string>::const_iterator titleIter = m_metaData.find("title");
	if (titleIter != m_metaData.end())
	{
		subject = titleIter->second;
	}

	return extractMessage(subject);
}

bool GMimeMboxFilter::skip_to_document(const string &ipath)
{
	if (ipath.empty() == true)
	{
		if (m_messageStart > 0)
		{
			// Reset
			return set_document_file(m_filePath);
		}

		return true;
	}

	// ipath's format is "o=offset&l=part_levels"
	if (sscanf(ipath.c_str(), "o=" GMIME_OFFSET_MODIFIER "&l=[", &m_messageStart) != 1)
	{
		return false;
	}

	finalize(false);
	m_partsCount = -1;
	m_levels.clear();
	string::size_type levelsPos = ipath.find("l=[");
	if (levelsPos != string::npos)
	{
		string::size_type endPos = 0;
		string levels(ipath.substr(levelsPos + 2));
		string levelInfo(extractField(levels, "[", "]", endPos));

		// Parse levels
		while (levelInfo.empty() == false)
		{
			int partLevel = 0, partsCount = 0, partNum = 0;

#ifdef DEBUG
			clog << "GMimeMboxFilter::skip_to_document: level " << levelInfo << endl;
#endif
			if (sscanf(levelInfo.c_str(), "%d,%d,%d", &partLevel, &partsCount, &partNum) == 3)
			{
				m_levels[partLevel] = pair<int, int>(partsCount, partNum);
			}

			if (endPos == string::npos)
			{
				break;
			}
			levelInfo = extractField(levels, "[", "]", endPos);
		}
	}
	m_messageDate.clear();
	m_partCharset.clear();
	m_foundDocument = false;

	if (((m_filePath.empty() == false) && (initializeFile() == true)) ||
		(initializeData() == true))
	{
		if (initialize() == true)
		{
			// Extract the first message at the given offset
			m_foundDocument = extractMessage("");
		}
	}

	return m_foundDocument;
}

string GMimeMboxFilter::get_error(void) const
{
	return "";
}

int GMimeMboxFilter::openFile(const string &filePath)
{
	int openFlags = O_RDONLY;

#ifdef O_CLOEXEC
	openFlags |= O_CLOEXEC;
#endif

	// Open the mbox file
#ifdef O_NOATIME
	int fd = open(filePath.c_str(), openFlags|O_NOATIME);
#else
	int fd = open(filePath.c_str(), openFlags);
#endif
#ifdef O_NOATIME
	if ((fd < 0) &&
		(errno == EPERM))
	{
		// Try again
		fd = open(filePath.c_str(), openFlags);
	}
#endif
	if (fd < 0)
	{
#ifdef DEBUG
		clog << "GMimeMboxFilter::openFile: couldn't open " << filePath << endl;
#endif
		return false;
	}
#ifndef O_CLOEXEC
	int fdFlags = fcntl(fd, F_GETFD);
	fcntl(fd, F_SETFD, fdFlags|FD_CLOEXEC);
#endif

	return fd;
}

bool GMimeMboxFilter::initializeData(void)
{
	// Create a stream
	m_pGMimeMboxStream = g_mime_stream_mem_new_with_buffer(m_pData, m_dataLength);
	if (m_pGMimeMboxStream == NULL)
	{
		return false;
	}

	ssize_t streamLength = g_mime_stream_length(m_pGMimeMboxStream);
	if (m_messageStart > 0)
	{
		if (m_messageStart > (GMIME_OFFSET_TYPE)streamLength)
		{
			// This offset doesn't make sense !
			m_messageStart = 0;
		}
#ifdef DEBUG
		clog << "GMimeMboxFilter::initializeData: from offset " << m_messageStart
			<< " to " << streamLength << endl;
#endif

		g_mime_stream_set_bounds(m_pGMimeMboxStream, m_messageStart, (GMIME_OFFSET_TYPE)streamLength);
	}

	return true;
}

bool GMimeMboxFilter::initializeFile(void)
{
	m_fd = openFile(m_filePath);
	if (m_fd < 0)
	{
		return false;
	}

	// Create a stream
	if (m_messageStart > 0)
	{
		ssize_t streamLength = g_mime_stream_length(m_pGMimeMboxStream);

		if (m_messageStart > (GMIME_OFFSET_TYPE)streamLength)
		{
			// This offset doesn't make sense !
			m_messageStart = 0;
		}
#ifdef DEBUG
		clog << "GMimeMboxFilter::initializeFile: from offset " << m_messageStart
			<< " to " << streamLength << endl;
#endif

#ifdef HAVE_MMAP
		m_pGMimeMboxStream = g_mime_stream_mmap_new_with_bounds(m_fd, PROT_READ, MAP_PRIVATE, m_messageStart, (GMIME_OFFSET_TYPE)streamLength);
#else
		m_pGMimeMboxStream = g_mime_stream_fs_new_with_bounds(m_fd, m_messageStart, (GMIME_OFFSET_TYPE)streamLength);
#endif
	}
	else
	{
#ifdef HAVE_MMAP
		m_pGMimeMboxStream = g_mime_stream_mmap_new(m_fd, PROT_READ, MAP_PRIVATE);
#else
		m_pGMimeMboxStream = g_mime_stream_fs_new(m_fd);
#endif
	}

	return true;
}

bool GMimeMboxFilter::initialize(void)
{
	if (m_pGMimeMboxStream == NULL)
	{
		return false;
	}

	// And a parser
	m_pParser = g_mime_parser_new();
	if (m_pParser != NULL)
	{
		g_mime_parser_init_with_stream(m_pParser, m_pGMimeMboxStream);
		g_mime_parser_set_respect_content_length(m_pParser, TRUE);
		// Scan for mbox From-lines
		g_mime_parser_set_scan_from(m_pParser, TRUE);

		return true;
	}
#ifdef DEBUG
	clog << "GMimeMboxFilter::initialize: couldn't create new parser" << endl;
#endif

	return false;
}

void GMimeMboxFilter::finalize(bool fullReset)
{
	if (m_pMimeMessage != NULL)
	{
#ifdef GMIME_ENABLE_RFC2047_WORKAROUNDS
		if (G_IS_OBJECT(m_pMimeMessage))
		{
			g_object_unref(m_pMimeMessage);
		}
#else
		g_mime_object_unref(GMIME_OBJECT(m_pMimeMessage));
#endif
		m_pMimeMessage = NULL;
	}
	if (m_pParser != NULL)
	{
		// FIXME: does the parser close the stream ?
		if (G_IS_OBJECT(m_pParser))
		{
			g_object_unref(m_pParser);
		}
		m_pParser = NULL;
	}
	if (m_pGMimeMboxStream != NULL)
	{
		if (G_IS_OBJECT(m_pGMimeMboxStream))
		{
			g_object_unref(m_pGMimeMboxStream);
		}
		m_pGMimeMboxStream = NULL;
	}

	// initializeFile() will always reopen the file
	if (m_fd >= 0)
	{
		close(m_fd);
		m_fd = -1;
	}
	if (fullReset == true)
	{
		// ...but those data fields will only be reinit'ed on a full reset
		m_pData = NULL;
		m_dataLength = 0;

		rewind();
	}
}

bool GMimeMboxFilter::readStream(GMimeStream *pStream, dstring &fileBuffer)
{
	char readBuffer[4096];
	ssize_t streamLen = g_mime_stream_length(pStream);
	ssize_t totalSize = 0, bytesRead = 0;
	bool gotOutput = true;

#ifdef DEBUG
	clog << "GMimeMboxFilter::readStream: stream is " << streamLen
		<< " bytes long" << endl;
#endif

	do
	{
		if ((m_maxSize > 0) &&
			(totalSize >= m_maxSize))
		{
#ifdef DEBUG
			clog << "GMimeMboxFilter::readStream: stopping at "
				<< totalSize << endl;
#endif
			break;
		}

		bytesRead = g_mime_stream_read(pStream, readBuffer, 4096);
		if (bytesRead > 0)
		{
			fileBuffer.append(readBuffer, bytesRead);
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
#ifdef DEBUG
	clog << "GMimeMboxFilter::readStream: read " << totalSize
		<< "/" << fileBuffer.size() << " bytes" << endl;
#endif

	return gotOutput;
}

bool GMimeMboxFilter::nextPart(const string &subject)
{
	if (m_pMimeMessage != NULL)
	{
		// Get the top-level MIME part in the message
		GMimeObject *pMimePart = g_mime_message_get_mime_part(m_pMimeMessage);
		if (pMimePart != NULL)
		{
			GMimeMboxPart mboxPart(subject, m_content);

			// Extract the part's text
			m_content.clear();
			if (extractPart(pMimePart, mboxPart) == true)
			{
				extractMetaData(mboxPart);

#ifndef GMIME_ENABLE_RFC2047_WORKAROUNDS
				if (pMimePart != NULL)
				{
					g_mime_object_unref(pMimePart);
				}
#endif

				return true;
			}

#ifndef GMIME_ENABLE_RFC2047_WORKAROUNDS
			if (pMimePart != NULL)
			{
				g_mime_object_unref(pMimePart);
			}
#endif
		}

#ifdef GMIME_ENABLE_RFC2047_WORKAROUNDS
		if (G_IS_OBJECT(m_pMimeMessage))
		{
			g_object_unref(m_pMimeMessage);
		}
#else
		g_mime_object_unref(GMIME_OBJECT(m_pMimeMessage));
#endif
		m_pMimeMessage = NULL;
	}

	// If we get there, no suitable parts were found
	m_partsCount = m_partNum = m_partLevel = -1;

	return false;
}

bool GMimeMboxFilter::extractPart(GMimeObject *part, GMimeMboxPart &mboxPart)
{
	if (part == NULL)
	{
		return false;
	}

	// Message parts may be nested
	while (GMIME_IS_MESSAGE_PART(part))
	{
#ifdef DEBUG
		clog << "GMimeMboxFilter::extractPart: nested message part" << endl;
#endif
		GMimeMessage *partMessage = g_mime_message_part_get_message(GMIME_MESSAGE_PART(part));
		part = g_mime_message_get_mime_part(partMessage);
#ifndef GMIME_ENABLE_RFC2047_WORKAROUNDS
		g_mime_object_unref(GMIME_OBJECT(partMessage));
#endif
	}

	// Is this a multipart ?
	if (GMIME_IS_MULTIPART(part))
	{
		int partsCount = 0, partNum = 0;
		bool gotPart = false;

#ifdef GMIME_ENABLE_RFC2047_WORKAROUNDS
		m_partsCount = partsCount = g_mime_multipart_get_count(GMIME_MULTIPART(part));
#else
		m_partsCount = partsCount = g_mime_multipart_get_number(GMIME_MULTIPART(part));
#endif
		++m_currentLevel;
#ifdef DEBUG
		clog << "GMimeMboxFilter::extractPart: message has " << m_partsCount << " parts at level " << m_currentLevel << endl;
#endif

		map<int, pair<int, int> >::iterator levelIter = m_levels.find(m_currentLevel);
		if (levelIter != m_levels.end())
 		{
			pair<int, int> partPair = levelIter->second;
#ifdef DEBUG
			clog << "GMimeMboxFilter::extractPart: level " << m_currentLevel << " had " << partPair.first << " parts" << endl;
#endif
			if (partPair.first == m_partsCount)
			{
				partNum = partPair.second;
#ifdef DEBUG
				clog << "GMimeMboxFilter::extractPart: restarting level " << m_currentLevel << " at part " << partNum << endl;
#endif
			}
		}
		else
		{
			partNum = 0;
		}

		for (; partNum < m_partsCount; ++partNum)
		{
#ifdef DEBUG
			clog << "GMimeMboxFilter::extractPart: extracting part " << partNum << endl;
#endif
			m_partNum = partNum;

			GMimeObject *multiMimePart = g_mime_multipart_get_part(GMIME_MULTIPART(part), partNum);
			if (multiMimePart == NULL)
			{
				continue;
			}

			gotPart = extractPart(multiMimePart, mboxPart);
#ifndef GMIME_ENABLE_RFC2047_WORKAROUNDS
			g_mime_object_unref(multiMimePart);
#endif

			if (gotPart == true)
			{
				break;
			}
		}

		// Were all parts in the next level parsed ?
		levelIter = m_levels.find(m_currentLevel + 1);
		if ((levelIter == m_levels.end()) ||
			(levelIter->second.second + 1 > levelIter->second.first))
		{
			// Move to the next part at this level
			++partNum;
		}

		levelIter = m_levels.find(m_currentLevel);
		if (levelIter != m_levels.end())
		{
			if (partNum > levelIter->second.second)
			{
				levelIter->second.second = partNum;
#ifdef DEBUG
				clog << "GMimeMboxFilter::extractPart: remembering to restart level "
					<< m_currentLevel << " at part " << partNum << endl;
#endif
			}
		}
		else
		{
			m_levels[m_currentLevel] = pair<int, int>(partsCount, partNum);
#ifdef DEBUG
			clog << "GMimeMboxFilter::extractPart: remembering to restart level "
				<< m_currentLevel << " at part " << partNum << endl;
#endif
		}
		--m_currentLevel;

		if (gotPart == true)
		{

			return true;
		}

		// None of the parts were suitable
		m_partsCount = m_partNum = m_partLevel = -1;
	}

	if (!GMIME_IS_PART(part))
	{
#ifdef DEBUG
		clog << "GMimeMboxFilter::extractPart: not a part" << endl;
#endif
		return false;
	}
	GMimePart *mimePart = GMIME_PART(part);

	// Check the content type
#ifdef GMIME_ENABLE_RFC2047_WORKAROUNDS
	GMimeContentType *mimeType = g_mime_object_get_content_type(GMIME_OBJECT(mimePart));
#else
	const GMimeContentType *mimeType = g_mime_object_get_content_type(GMIME_OBJECT(mimePart));
#endif
	// Set this for caller
	char *partType = g_mime_content_type_to_string(mimeType);
	if (partType != NULL)
	{
#ifdef DEBUG
		clog << "GMimeMboxFilter::extractPart: type is " << partType << endl;
#endif
		mboxPart.m_contentType = partType;

		// Is the body in a local file ?
		if (mboxPart.m_contentType == "message/external-body")
		{
			const char *partAccessType = g_mime_content_type_get_parameter(mimeType, "access-type");
			if (partAccessType != NULL)
			{
				string contentAccessType(partAccessType);

#ifdef DEBUG
				clog << "GMimeMboxFilter::extractPart: part access type is " << contentAccessType << endl;
#endif
				if (contentAccessType == "local-file")
				{
					const char *partLocalFile = g_mime_content_type_get_parameter(mimeType, "name");

					if (partLocalFile != NULL)
					{
						mboxPart.m_contentType = "SCAN";
						mboxPart.m_subject = partLocalFile;
						mboxPart.m_buffer.clear();
#ifdef DEBUG
						clog << "GMimeMboxFilter::extractPart: local file at " << partLocalFile << endl;
#endif

						// Load the part from file
						int fd = openFile(partLocalFile);
						if (fd >= 0)
						{
							GMimeStream *fileStream = g_mime_stream_mmap_new(fd, PROT_READ, MAP_PRIVATE);
							if (fileStream != NULL)
							{
								readStream(fileStream, mboxPart.m_buffer);
								if (G_IS_OBJECT(fileStream))
								{
									g_object_unref(fileStream);
								}
							}
						}
					}
				}
				else
				{
					mboxPart.m_contentType = "application/octet-stream";
#ifdef DEBUG
					clog << "GMimeMboxFilter::extractPart: unknown part access type" << endl;
#endif
				}
			}
		}
		g_free(partType);
	}

	// Was the part already loaded ?
	if (mboxPart.m_buffer.empty() == false)
	{
		return true;
	}

#ifdef GMIME_ENABLE_RFC2047_WORKAROUNDS
	GMimeContentEncoding encodingType = g_mime_part_get_content_encoding(mimePart);
#else
	GMimePartEncodingType encodingType = g_mime_part_get_encoding(mimePart);
#endif
#ifdef DEBUG
	clog << "GMimeMboxFilter::extractPart: encoding is " << encodingType << endl;
#endif
#ifdef GMIME_ENABLE_RFC2047_WORKAROUNDS
	g_mime_part_set_content_encoding(mimePart, GMIME_CONTENT_ENCODING_QUOTEDPRINTABLE);
#else
	g_mime_part_set_encoding(mimePart, GMIME_PART_ENCODING_QUOTEDPRINTABLE);
#endif
	const char *fileName = g_mime_part_get_filename(mimePart);
	if (fileName != NULL)
	{
#ifdef DEBUG
		clog << "GMimeMboxFilter::extractPart: file name is " << fileName << endl;
#endif
		mboxPart.m_subject = fileName;
	}

	// Create a in-memory output stream
	GMimeStream *memStream = g_mime_stream_mem_new();
	if (memStream == NULL)
	{
		return false;
	}

	const char *charset = g_mime_content_type_get_parameter(mimeType, "charset");
	if (charset != NULL)
	{
		m_partCharset = charset;
#if 0
		// Install a charset filter
		if (strncasecmp(charset, "UTF-8", 5) != 0)
		{
			GMimeFilter *charsetFilter = g_mime_filter_charset_new(charset, "UTF-8");
			if (charsetFilter != NULL)
			{
#ifdef DEBUG
				clog << "GMimeMboxFilter::extractPart: converting from charset " << charset << endl;
#endif
				g_mime_stream_filter_add(GMIME_STREAM_FILTER(memStream), charsetFilter);
				g_object_unref(charsetFilter);
			}
		}
#endif
	}

	// Write the part to the stream
	GMimeDataWrapper *dataWrapper = g_mime_part_get_content_object(mimePart);
	if (dataWrapper != NULL)
	{
		ssize_t writeLen = g_mime_data_wrapper_write_to_stream(dataWrapper, memStream);
#ifdef DEBUG
		clog << "GMimeMboxFilter::extractPart: wrote " << writeLen << " bytes" << endl;
#endif
		if (G_IS_OBJECT(dataWrapper))
		{
			g_object_unref(dataWrapper);
		}
	}
	g_mime_stream_flush(memStream);

	if ((m_returnHeaders == true) &&
		(mboxPart.m_contentType.length() >= 10) &&
		(strncasecmp(mboxPart.m_contentType.c_str(), "text/plain", 10) == 0))
	{
		char *pHeaders = g_mime_object_get_headers(GMIME_OBJECT(m_pMimeMessage));

		if (pHeaders != NULL)
		{
			mboxPart.m_buffer = pHeaders;
			mboxPart.m_buffer += "\n";
			free(pHeaders);
		}
	}

	g_mime_stream_reset(memStream);
	readStream(memStream, mboxPart.m_buffer);
	if (G_IS_OBJECT(memStream))
	{
		g_object_unref(memStream);
	}
	m_partLevel = m_currentLevel;

	return true;
}

bool GMimeMboxFilter::extractDate(const string &header)
{
	const char *pDate = g_mime_object_get_header(GMIME_OBJECT(m_pMimeMessage), header.c_str());

	if (pDate == NULL)
	{
		return false;
	}

	string date(pDate);
	struct tm timeTm;

	timeTm.tm_sec = timeTm.tm_min = timeTm.tm_hour = timeTm.tm_mday = 0;
	timeTm.tm_mon = timeTm.tm_year = timeTm.tm_wday = timeTm.tm_yday = timeTm.tm_isdst = 0;

	if (date.find(',') != string::npos)
	{
		strptime(pDate, "%a, %d %b %Y %H:%M:%S %z", &timeTm);
		if (timeTm.tm_year <= 0)
		{
			strptime(pDate, "%a, %d %b %y %H:%M:%S %z", &timeTm);
		}
	}
	else
	{
		strptime(pDate, "%d %b %Y %H:%M:%S %z", &timeTm);
		if (timeTm.tm_year <= 0)
		{
			strptime(pDate, "%d %b %y %H:%M:%S %z", &timeTm);
		}
	}

	// Sanity check
	if (timeTm.tm_year <= 0)
	{
#ifdef DEBUG
		clog << "GMimeMboxFilter::extractDate: ignoring bogus year " << timeTm.tm_year << endl;
#endif
		return false;
	}

	m_messageDate = mktime(&timeTm);
#ifdef DEBUG
	clog << "GMimeMboxFilter::extractDate: message date is " << pDate
		<< ": " << m_messageDate << endl;
#endif

	return true;
}

bool GMimeMboxFilter::extractMessage(const string &subject)
{
	string msgSubject(subject);

	m_currentLevel = 0;
	while (g_mime_stream_eos(m_pGMimeMboxStream) == FALSE)
	{
		// Does the previous message have parts left to parse ?
		if (m_partsCount == -1)
		{
			// No, it doesn't
			if (m_pMimeMessage != NULL)
			{
#ifdef GMIME_ENABLE_RFC2047_WORKAROUNDS
				if (G_IS_OBJECT(m_pMimeMessage))
				{
					g_object_unref(m_pMimeMessage);
				}
#else
				g_mime_object_unref(GMIME_OBJECT(m_pMimeMessage));
#endif
				m_pMimeMessage = NULL;
			}

			// Get the next message
			m_pMimeMessage = g_mime_parser_construct_message(m_pParser);
			if (m_pMimeMessage == NULL)
			{
				clog << "Couldn't construct new MIME message" << endl;
				break;
			}

			m_messageStart = g_mime_parser_get_from_offset(m_pParser);
#ifdef GMIME_ENABLE_RFC2047_WORKAROUNDS
			gint64 messageEnd = g_mime_parser_tell(m_pParser);
#else
			off_t messageEnd = g_mime_parser_tell(m_pParser);
#endif
#ifdef DEBUG
			clog << "GMimeMboxFilter::extractMessage: message between offsets " << m_messageStart
				<< " and " << messageEnd << endl;
#endif
			if (messageEnd > m_messageStart)
			{
				// This only applies to Mozilla
				const char *pMozStatus = g_mime_object_get_header(GMIME_OBJECT(m_pMimeMessage), "X-Mozilla-Status");
				if (pMozStatus != NULL)
				{
					long int mozFlags = strtol(pMozStatus, NULL, 16);

					// Watch out for Mozilla specific flags :
					// MSG_FLAG_EXPUNGED, MSG_FLAG_EXPIRED
					// They are defined in mailnews/MailNewsTypes.h and msgbase/nsMsgMessageFlags.h
					if ((mozFlags & 0x0008) ||
						(mozFlags & 0x0040))
					{
#ifdef DEBUG
						clog << "GMimeMboxFilter::extractMessage: flagged by Mozilla" << endl;
#endif
						continue;
					}
				}
				// This only applies to Evolution
				const char *pEvoStatus = g_mime_object_get_header(GMIME_OBJECT(m_pMimeMessage), "X-Evolution");
				if (pEvoStatus != NULL)
				{
					string evoStatus(pEvoStatus);
					string::size_type flagsPos = evoStatus.find('-');

					if (flagsPos != string::npos)
					{
						long int evoFlags = strtol(evoStatus.substr(flagsPos + 1).c_str(), NULL, 16);

						// Watch out for Evolution specific flags :
						// CAMEL_MESSAGE_DELETED
						// It's defined in camel/camel-folder-summary.h
						if (evoFlags & 0x0002)
						{
#ifdef DEBUG
							clog << "GMimeMboxFilter::extractMessage: flagged by Evolution" << endl;
#endif
							continue;
						}
					}
				}

				// How old is this message ?
				if ((extractDate("Date") == false) &&
					(extractDate("Delivery-Date") == false) &&
					(extractDate("Resent-Date") == false))
				{
					m_messageDate = time(NULL);
#ifdef DEBUG
					clog << "GMimeMboxFilter::extractMessage: message date is today's " << m_messageDate << endl;
#endif
				}

				// Extract the subject
				const char *pSubject = g_mime_message_get_subject(m_pMimeMessage);
				if (pSubject != NULL)
				{
					msgSubject = pSubject;
				}
			}
		}
#ifdef DEBUG
		clog << "GMimeMboxFilter::extractMessage: message subject is " << msgSubject << endl;
#endif

		if (nextPart(msgSubject) == true)
		{
			return true;
		}
		// Try the next message
	}

	// The last message may have parts left
	if (m_partsCount != -1)
	{
		return nextPart(msgSubject);
	}

	return false;
}

void GMimeMboxFilter::extractMetaData(GMimeMboxPart &mboxPart)
{
	string ipath;
	char posStr[128];

	// New document
	m_metaData.clear();
	m_metaData["title"] = mboxPart.m_subject;
	m_metaData["mimetype"] = mboxPart.m_contentType;
	if (m_messageDate.empty() == false)
	{
		m_metaData["date"] = m_messageDate;
	}
	m_metaData["charset"] = m_partCharset;
	snprintf(posStr, 128, "%lu", m_content.length());
	m_metaData["size"] = posStr;
	snprintf(posStr, 128, "o=%ld&l=", m_messageStart);
	ipath = posStr;
	for (map<int, pair<int, int> >::const_iterator levelIter = m_levels.begin();
		levelIter != m_levels.end(); ++levelIter)
	{
		int partNum = max(levelIter->second.second - 1, 0);

		if (levelIter->first == m_partLevel)
		{
			partNum = m_partNum;
		}
		snprintf(posStr, 128, "[%d,%d,%d]", levelIter->first, levelIter->second.first, partNum);
		ipath += posStr;
	}
	m_metaData["ipath"] = ipath;
#ifdef DEBUG
	clog << "GMimeMboxFilter::extractMetaData: message location is " << ipath << endl; 
#endif
}
