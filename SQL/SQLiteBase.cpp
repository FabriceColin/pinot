/*
 *  Copyright 2005-2014 Fabrice Colin
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
#include <sys/stat.h>
#include <stdarg.h>
#include <unistd.h>
#include <algorithm>
#include <iostream>

#include "config.h"
#include "NLS.h"
#include "SQLiteBase.h"

using std::clog;
using std::endl;
using std::string;
using std::vector;
using std::map;
using std::pair;
using std::for_each;

static int busyHandler(void *pData, int lockNum)
{
	// Try again after 100 ms
	usleep(100000);
	return 1;
}

// A function object to finalize statements with for_each()
struct FinalizeStatementsFunc
{
	public:
		void operator()(map<string, sqlite3_stmt*>::value_type &p)
		{
			if (p.second != NULL)
			{
				sqlite3_finalize(p.second);
			}
		}
};

SQLiteRow::SQLiteRow(const vector<string> &rowColumns, unsigned int nColumns) :
	SQLRow(nColumns),
	m_pStatement(NULL)
{
	if (rowColumns.empty() == false)
	{
		m_columns.reserve(rowColumns.size());
#if 0
		// FIXME: why does this segfault in string::assign() ?
		copy(rowColumns.begin(), rowColumns.end(), m_columns.begin());
#else
		for (vector<string>::const_iterator colIter = rowColumns.begin(); colIter != rowColumns.end(); ++colIter)
		{
			m_columns.push_back(*colIter);
		}
#endif
	}
}

SQLiteRow::SQLiteRow(sqlite3_stmt *pStatement, unsigned int nColumns) :
	SQLRow(nColumns),
	m_pStatement(pStatement)
{
}

SQLiteRow::~SQLiteRow()
{
}

string SQLiteRow::getColumn(unsigned int nColumn) const
{
	if (m_pStatement != NULL)
	{
		const unsigned char *pTextColumn = sqlite3_column_text(m_pStatement, nColumn);
		if (pTextColumn != NULL)
		{
			return (const char*)pTextColumn;
		}

		// We may sometime not be able to get a column, eg a sum of 0 records will have type SQLITE_NULL
		int columnType = sqlite3_column_type(m_pStatement, nColumn);
		if (columnType != SQLITE_NULL)
		{
#ifdef DEBUG
			clog << "SQLiteRow::getColumn: couldn't get column " << nColumn << ", type " << columnType << endl;
#endif
		}

		return "";
	}

	if (nColumn < m_nColumns)
	{
		vector<string>::const_iterator colIter = m_columns.begin();
		for (unsigned int i = 0; (i < m_nColumns) && (colIter != m_columns.end()); ++i)
		{
			if (i == nColumn)
			{
				string column(*colIter);

				return column;
			}
			++colIter;
		}
	}

	return "";
}

SQLiteResults::SQLiteResults(char **results, unsigned long nRows, unsigned int nColumns) :
	SQLResults(nRows, nColumns),
	m_results(results),
	m_pStatement(NULL),
	m_done(false),
	m_firstStep(true),
	m_stepCode(SQLITE_BUSY)
{
	// Check we actually have results
	if ((m_results == NULL) ||
		(m_nRows <= 0))
	{
		m_nRows = m_nCurrentRow = 0;
		m_nColumns = 0;
	}
}

SQLiteResults::SQLiteResults(sqlite3_stmt *pStatement) :
	SQLResults(0, sqlite3_column_count(pStatement)),
	m_results(NULL),
	m_pStatement(pStatement),
	m_done(false),
	m_firstStep(true),
	m_stepCode(SQLITE_BUSY)
{
	// If the statement returns rows, this will get the first row
	// If not, this will be evaluate and "complete" the statement
	step();
}

SQLiteResults::~SQLiteResults()
{
	if (m_results != NULL)
	{
		sqlite3_free_table(m_results);
	}
	if (m_pStatement != NULL)
	{
		rewind();
	}
}

void SQLiteResults::step(void)
{
	if (m_pStatement == NULL)
	{
		return;
	}

	m_stepCode = SQLITE_BUSY;
	while ((m_stepCode == SQLITE_BUSY) ||
		(m_stepCode == SQLITE_IOERR_BLOCKED))
	{
		m_stepCode = sqlite3_step(m_pStatement);
		if ((m_stepCode == SQLITE_BUSY) ||
			(m_stepCode == SQLITE_IOERR_BLOCKED))
		{
#if SQLITE_VERSION_NUMBER < 3006024
			// 3.6.23 and older require a call to sqlite3_reset() for any code other
			// than SQLITE_ROW before stepping again
			// http://www.sqlite.org/c3ref/step.html
			rewind();
#endif
			// Sleep roughly a sixth of a second, ie around 10 write operations
			usleep(150000);
		}
		else if ((m_stepCode != SQLITE_ROW) &&
			(m_stepCode != SQLITE_DONE))
		{
#ifdef DEBUG
			clog << "Step returned error code " << m_stepCode << endl;
#endif
		}
	}
}

int SQLiteResults::getStepCode(void) const
{
	return m_stepCode;
}

bool SQLiteResults::hasMoreRows(void) const
{
	if (m_pStatement != NULL)
	{
		return !m_done;
	}

	return SQLResults::hasMoreRows();
}

string SQLiteResults::getColumnName(unsigned int nColumn) const
{
	if (m_pStatement != NULL)
	{
		return sqlite3_column_name(m_pStatement, (int)nColumn);
	}

	if (nColumn < m_nColumns)
	{
		return m_results[nColumn];
	}

	return "";
}

SQLRow *SQLiteResults::nextRow(void)
{
	if (m_pStatement != NULL)
	{
		if (m_done == true)
		{
			return NULL;
		}

		if (m_firstStep == false)
		{
			step();
		}
		else
		{
			m_firstStep = false;
		}
		if (m_stepCode == SQLITE_ROW)
		{
			++m_nCurrentRow;
			return new SQLiteRow(m_pStatement, m_nColumns);
		}
		else if (m_stepCode == SQLITE_DONE)
		{
			m_done = true;
		}
		else
		{
			clog << "Failed to get next result row, error code " << m_stepCode << endl;
		}

		return NULL;
	}

	if ((m_nCurrentRow < 0) ||
		(m_nCurrentRow >= m_nRows))
	{
		return NULL;
	}

	// The very first row holds the column names
	unsigned long firstIndex = (m_nCurrentRow  + 1) * m_nColumns;
	unsigned long lastIndex = firstIndex + m_nColumns - 1;
	vector<string> rowColumns;

	for (unsigned long i = firstIndex; i <= lastIndex; ++i)
	{
		if (m_results[i] == NULL)
		{
			rowColumns.push_back("");
		}
		else
		{
			rowColumns.push_back(m_results[i]);
		}
	}
	++m_nCurrentRow;

	return new SQLiteRow(rowColumns, m_nColumns);
}

bool SQLiteResults::rewind(void)
{
	SQLResults::rewind();
	if (m_pStatement != NULL)
	{
		// The constructor made sure that step() ran at least once
		sqlite3_reset(m_pStatement);
		m_done = false;
	}

	return true;
}

SQLiteBase::SQLiteBase(const string &databaseName,
	bool readOnly, bool onDemand) :
	SQLDB(databaseName, readOnly),
	m_onDemand(onDemand),
	m_inTransaction(false),
	m_pDatabase(NULL)
{
	if (m_onDemand == false)
	{
		open();
	}
}

SQLiteBase::~SQLiteBase()
{
	if (m_onDemand == false)
	{
		close();
	}
}

void SQLiteBase::executeSimpleStatement(const string &sql, int &execError)
{
	char *errMsg = NULL;

	execError = sqlite3_exec(m_pDatabase,
		sql.c_str(), 
		NULL, NULL, // No callback
		&errMsg);
	if (execError != SQLITE_OK)
	{
		if (errMsg != NULL)
		{
			clog << m_databaseName << ": SQL <" << sql << "> failed with error " << execError << ": " << errMsg << endl;

			sqlite3_free(errMsg);
		}
	}
}

void SQLiteBase::open(void)
{
	int openFlags = SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE;

	if (m_readOnly == true)
	{
		openFlags = SQLITE_OPEN_READONLY;
	}

	// Open the new database
	// FIXME: ensure it's in mode SQLITE_CONFIG_SERIALIZED
	if (sqlite3_open_v2(m_databaseName.c_str(), &m_pDatabase,
		openFlags, NULL) != SQLITE_OK)
	{
		// An handle is returned even when an error occurs !
		if (m_pDatabase != NULL)
		{
			clog << m_databaseName << ": " << sqlite3_errmsg(m_pDatabase) << endl;
			close();
		}
	}

	if (m_pDatabase != NULL)
	{
		// Set up a busy handler
		sqlite3_busy_handler(m_pDatabase, busyHandler, NULL);
	}
	else
	{
		clog << "Couldn't open " << m_databaseName << endl;
	}
}

void SQLiteBase::close(void)
{
	if (m_pDatabase != NULL)
	{
		for_each(m_statements.begin(), m_statements.end(),
			FinalizeStatementsFunc());
		m_statements.clear();

		sqlite3_close(m_pDatabase);
		m_pDatabase = NULL;
	}
}

bool SQLiteBase::check(const string &databaseName)
{
	struct stat dbStat;

	// The specified path must be a file
	if ((stat(databaseName.c_str(), &dbStat) != -1) &&
		(!S_ISREG(dbStat.st_mode)))
	{
		// It exists, but it's not a file as expected
		clog << databaseName << " is not a file" << endl;
		return false;
	}

	return true;
}

bool SQLiteBase::backup(const string &destDatabaseName,
	int pagesCount, bool retryOnLock)
{
	sqlite3 *pBackupDatabase = NULL;
	int errorCode = sqlite3_open(destDatabaseName.c_str(), &pBackupDatabase);

	if (errorCode != SQLITE_OK)
	{
		return false;
	}

	// Open the backup object
	sqlite3_backup *pBackup = sqlite3_backup_init(pBackupDatabase, "main", m_pDatabase, "main");
	if (pBackup != NULL)
	{
		// Copy database pages
		errorCode = sqlite3_backup_step(pBackup, pagesCount);
		while ((errorCode == SQLITE_OK) ||
			(errorCode == SQLITE_BUSY) ||
			((errorCode == SQLITE_LOCKED) && (retryOnLock == true)))
		{
			// Sleep roughly a sixth of a second, ie around 10 write operations
			sqlite3_sleep(150);

			int remainingPages = sqlite3_backup_remaining(pBackup);
			int totalPages = sqlite3_backup_pagecount(pBackup);
			int donePages = totalPages - remainingPages;

			clog << m_databaseName << ": backed up " << donePages
				<< " pages out of " << totalPages << endl;

			errorCode = sqlite3_backup_step(pBackup, pagesCount);
		}

		sqlite3_backup_finish(pBackup);
	}

	errorCode = sqlite3_errcode(pBackupDatabase);
	sqlite3_close(pBackupDatabase);

	if (errorCode == SQLITE_OK)
	{
		return true;
	}

	return false;
}

bool SQLiteBase::isOpen(void) const
{
	if (m_pDatabase == NULL)
	{
		return false;
	}

	return true;
}

bool SQLiteBase::reopen(const string &databaseName)
{
	if (m_databaseName == databaseName)
	{
		return false;
	}

	close();

	m_databaseName = databaseName;

	open();
	if (isOpen() == true)
	{
		return true;
	}

	return false;
}

bool SQLiteBase::alterTable(const string &tableName,
	const string &columns, const string &newDefinition)
{
	if ((tableName.empty() == true) ||
		(columns.empty() == true) ||
		(newDefinition.empty() == true))
	{
		return false;
	}

	string sql("BEGIN TRANSACTION; CREATE TEMPORARY TABLE ");
	sql += tableName;
	sql += "_backup (";
	sql += columns;
	sql += "); INSERT INTO ";
	sql += tableName;
	sql += "_backup SELECT ";
	sql += columns;
	sql += " FROM ";
	sql += tableName;
	sql += "; DROP TABLE ";
	sql += tableName;
	sql += "; CREATE TABLE ";
	sql += tableName;
	sql += " (";
	sql += newDefinition;
	sql += "); INSERT INTO ";
	sql += tableName;
	sql += " SELECT ";
	sql += columns;
	sql += " FROM ";
	sql += tableName;
	sql += "_backup; DROP TABLE ";
	sql += tableName;
	sql += "_backup; COMMIT;";
#ifdef DEBUG
	clog << "SQLiteBase::alterTable: " << sql << endl;
#endif

	return executeSimpleStatement(sql);
}

bool SQLiteBase::beginTransaction(void)
{
	if ((m_pDatabase == NULL) ||
		(m_onDemand == true) ||
		(m_inTransaction == true))
	{
		// Not applicable
		return true;
	}

	if (executeSimpleStatement("BEGIN TRANSACTION;") == true)
	{
		m_inTransaction = true;
		return true;
	}

	clog << m_databaseName << ": failed to begin transaction" << endl;

	return false;
}

bool SQLiteBase::rollbackTransaction(void)
{
	if ((m_pDatabase == NULL) ||
		(m_onDemand == true) ||
		(m_inTransaction == false))
	{
		// Not applicable
		return true;
	}

	int execError = SQLITE_OK;

	do
	{
		executeSimpleStatement("ROLLBACK TRANSACTION;", execError);
		if (execError == SQLITE_OK)
		{
			m_inTransaction = false;
			return true;
		}
		if (execError != SQLITE_BUSY)
		{
			clog << m_databaseName << ": failed to rollback transaction with error " << execError << endl;
			return false;
		}

		// This failed because operations are pending
		// Sleep roughly a sixth of a second, ie around 10 write operations
		usleep(150000);
	}
	while (execError != SQLITE_OK);

	clog << m_databaseName << ": failed to rollback transaction" << endl;

	return false;
}

bool SQLiteBase::endTransaction(void)
{
	if ((m_pDatabase == NULL) ||
		(m_onDemand == true) ||
		(m_inTransaction == false))
	{
		// Not applicable
		return true;
	}

	int execError = SQLITE_OK;

	do
	{
		executeSimpleStatement("END TRANSACTION;", execError);
		if (execError == SQLITE_OK)
		{
			m_inTransaction = false;
			return true;
		}
		if (execError != SQLITE_BUSY)
		{
			clog << m_databaseName << ": failed to end transaction with error " << execError << endl;
			return false;
		}

		// This failed because write operations are pending
		// Sleep roughly a sixth of a second, ie around 10 write operations
		usleep(150000);
	}
	while (execError != SQLITE_OK);

	clog << m_databaseName << ": failed to end transaction" << endl;

	return false;
}

bool SQLiteBase::executeSimpleStatement(const string &sql)
{
	bool success = false;

	if (sql.empty() == true)
	{
		return false;
	}

	if (m_onDemand == true)
	{
		open();
	}
	if (m_pDatabase == NULL)
	{
		return false;
	}

	int execError = SQLITE_OK;

	executeSimpleStatement(sql, execError);
	if (execError == SQLITE_OK)
	{
		success = true;
	}
	if (m_onDemand == true)
	{
		close();
	}

	return success;
}

SQLResults *SQLiteBase::executeStatement(const char *sqlFormat, ...)
{
	SQLiteResults *pResults = NULL;
	va_list ap;

	if (sqlFormat == NULL)
	{
		return NULL;
	}

	if (m_onDemand == true)
	{
		open();
	}
	if (m_pDatabase == NULL)
	{
		return NULL;
	}

	va_start(ap, sqlFormat);
	char *stringBuff = sqlite3_vmprintf(sqlFormat, ap);
	if (stringBuff == NULL)
	{
		clog << m_databaseName << ": couldn't format SQL statement" << endl;

		if (m_onDemand == true)
		{
			close();
		}
		return NULL;
	}

	char **results;
	char *errMsg;
	int nRows, nColumns;
	int errorCode = sqlite3_get_table(m_pDatabase,
		stringBuff,
		&results,
		&nRows,
		&nColumns,
		&errMsg);

	if (errorCode != SQLITE_OK)
	{
		if (errMsg != NULL)
		{
			clog << m_databaseName << ": SQL <" << stringBuff << "> failed with error " << errorCode << ": " << errMsg << endl;

			sqlite3_free(errMsg);
		}
	}
	else
	{
		pResults = new SQLiteResults(results, (unsigned long)nRows, (unsigned int)nColumns);
	}
	va_end(ap);

	if (m_onDemand == true)
	{
		close();
	}

	return pResults;
}

bool SQLiteBase::prepareStatement(const string &statementId,
	const string &sqlFormat)
{
	if ((sqlFormat.empty() == true) ||
		(m_onDemand == true) ||
		(m_pDatabase == NULL))
	{
		return false;
	}

	map<string, sqlite3_stmt*>::iterator statIter = m_statements.find(statementId);
	if (statIter != m_statements.end())
	{
		return true;
	}

	sqlite3_stmt *pStatement = NULL;
	if (sqlite3_prepare_v2(m_pDatabase,
		sqlFormat.c_str(),
		(int)sqlFormat.length(),
		&pStatement,
		NULL) == SQLITE_OK)
	{
		if (pStatement != NULL)
		{
			m_statements.insert(pair<string, sqlite3_stmt*>(statementId, pStatement));
			return true;
		}
	}

	clog << m_databaseName << ": failed to compile SQL statement " << statementId << endl;

	return false;
}

SQLResults *SQLiteBase::executePreparedStatement(const string &statementId,
	const vector<string> &values)
{
	unsigned int paramIndex = 1;

	map<string, sqlite3_stmt*>::iterator statIter = m_statements.find(statementId);
	if (statIter == m_statements.end())
	{
#ifdef DEBUG
		clog << "SQLiteBase::executePreparedStatement: invalid SQL statement ID " << statementId << endl;
#endif
		return NULL;
	}

	// Bind values
	// The left-most parameter's index is 1
	for (vector<string>::const_iterator valueIter = values.begin();
		valueIter != values.end(); ++valueIter, ++paramIndex)
	{
		int errorCode = sqlite3_bind_text(statIter->second, paramIndex,
			valueIter->c_str(), -1, SQLITE_TRANSIENT);
		if (errorCode != SQLITE_OK)
		{
			clog << m_databaseName << ": failed to bind parameter to statement "
				<< statementId << " (" << paramIndex << "/" << *valueIter
				<< ") with error " << errorCode << endl;
			return NULL;
		}
	}

	SQLiteResults *pResults = new SQLiteResults(statIter->second);
	int stepCode = pResults->getStepCode();

	if ((stepCode != SQLITE_ROW) &&
		(stepCode != SQLITE_DONE))
	{
#ifdef DEBUG
		clog << m_databaseName << ": step for statement " << statementId
			<< " failed with error " << stepCode
			<< " " << sqlite3_errmsg(m_pDatabase) << endl;
#endif
		delete pResults;

		return NULL;
	}

	return pResults;
}

SQLResults *SQLiteBase::executePreparedStatement(const string &statementId,
	const vector<pair<string, SQLRow::SQLType> > &values)
{
	vector<string> untypedValues;

	// SQLite doesn't care about the type of parameters and results
	for (vector<pair<string, SQLRow::SQLType> >::const_iterator valueIter = values.begin();
		valueIter != values.end(); ++valueIter)
	{
		untypedValues.push_back(valueIter->first);
	}

	return executePreparedStatement(statementId,
		untypedValues);
}

