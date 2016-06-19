/*
 *  Copyright 2005-2008 Fabrice Colin
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

#ifdef WIN32
#include <windows.h>
#endif
#include <stdlib.h>
#include <pthread.h>
#include <iostream>

#ifdef USE_SSL
#include <openssl/crypto.h>
#endif

#include "DownloaderInterface.h"

using namespace std;

#ifdef USE_SSL
// OpenSSL multi-thread support, required by Curl
static pthread_mutex_t locksTable[CRYPTO_NUM_LOCKS];

// OpenSSL locking functiom
static void lockingCallback(int mode, int n, const char *file, int line)
{
	int status = 0;

	if (mode & CRYPTO_LOCK)
	{
		status = pthread_mutex_lock(&(locksTable[n]));
#ifdef DEBUG
		if (status != 0)
		{
			clog << "lockingCallback: failed to lock mutex " << n << endl;
		}
#endif
	}
	else
	{
		status = pthread_mutex_unlock(&(locksTable[n]));
#ifdef DEBUG
		if (status != 0)
		{
			clog << "lockingCallback: failed to unlock mutex " << n << endl;
		}
#endif
	}
}

static unsigned long idCallback(void)
{
#ifdef WIN32
	return (unsigned long)GetCurrentThreadId();
#else
	return (unsigned long)pthread_self();
#endif
}
#endif

/// Initialize downloaders.
void DownloaderInterface::initialize(void)
{
#ifdef USE_SSL
	pthread_mutexattr_t mutexAttr;

	pthread_mutexattr_init(&mutexAttr);
	pthread_mutexattr_settype(&mutexAttr, PTHREAD_MUTEX_ERRORCHECK);

	// Initialize the OpenSSL mutexes
	for (unsigned int lockNum = 0; lockNum < CRYPTO_NUM_LOCKS; ++lockNum)
	{
		pthread_mutex_init(&(locksTable[lockNum]), &mutexAttr);
	}
	// Set the callbacks
	CRYPTO_set_locking_callback(lockingCallback);
	CRYPTO_set_id_callback(idCallback);

	pthread_mutexattr_destroy(&mutexAttr);
#endif
}

/// Shutdown downloaders.
void DownloaderInterface::shutdown(void)
{
#ifdef USE_SSL
	// Reset the OpenSSL callbacks
	CRYPTO_set_id_callback(NULL);
	CRYPTO_set_locking_callback(NULL);

	// Free the mutexes
	for (unsigned int lockNum = 0; lockNum < CRYPTO_NUM_LOCKS; ++lockNum)
	{
		pthread_mutex_destroy(&(locksTable[lockNum]));
	}
#endif
}

DownloaderInterface::DownloaderInterface() :
	m_userAgent("Mozilla/5.0 (X11; U; Linux i686; en-US; rv:1.7.3) Gecko/20041020"),
	m_proxyPort(0),
	m_timeout(60),
	m_method("GET")
{
}

DownloaderInterface::~DownloaderInterface()
{
}

/// Sets a (name, value) setting; true if success.
bool DownloaderInterface::setSetting(const string &name, const string &value)
{
	bool goodSetting = true;

	if (name == "useragent")
	{
		m_userAgent = value;
	}
	else if (name == "proxyaddress")
	{
		m_proxyAddress = value;
	}
	else if ((name == "proxyport") &&
		(value.empty() == false))
	{
		m_proxyPort = (unsigned int )atoi(value.c_str());
	}
	else if (name == "proxytype")
	{
		m_proxyType = value;
	}
	else if (name == "timeout")
	{
		m_timeout = (unsigned int)atoi(value.c_str());
	}
	else if (name == "method")
	{
		if ((value == "GET") ||
			(value == "POST"))
		{
			m_method = value;
		}
	}
	else if (name == "postfields")
	{
		m_postFields = value;
	}
	else
	{
		goodSetting = false;
	}

	return goodSetting;
}

