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

#ifndef _SQLITE_BASE_H
#define _SQLITE_BASE_H

#include <string>
#include <map>
#include <vector>
#include <utility>

#include <sqlite3.h>

#include "SQLDB.h"

/// A row of results.
class SQLiteRow : public SQLRow
{
	public:
		SQLiteRow(const std::vector<std::string> &rowColumns, unsigned int nColumns);
		SQLiteRow(sqlite3_stmt *pStatement, unsigned int nColumns);
		virtual ~SQLiteRow();

		virtual std::string getColumn(unsigned int nColumn) const;

	protected:
		std::vector<std::string> m_columns;
		sqlite3_stmt *m_pStatement;

	private:
		SQLiteRow(const SQLiteRow &other);
		SQLiteRow &operator=(const SQLiteRow &other);

};

/// Results extracted from a SQLite database.
class SQLiteResults : public SQLResults
{
	public:
		SQLiteResults(char **results, unsigned long nRows, unsigned int nColumns);
		SQLiteResults(sqlite3_stmt *pStatement);
		virtual ~SQLiteResults();

		int getStepCode(void) const;

		virtual bool hasMoreRows(void) const;

		virtual std::string getColumnName(unsigned int nColumn) const;

		virtual SQLRow *nextRow(void);

		virtual bool rewind(void);

	protected:
		char **m_results;
		sqlite3_stmt *m_pStatement;
		bool m_done;
		bool m_firstStep;
		int m_stepCode;

		void step(void);

	private:
		SQLiteResults(const SQLiteResults &other);
		SQLiteResults &operator=(const SQLiteResults &other);

};

/// Simple C++ wrapper around the SQLite API.
class SQLiteBase : public SQLDB
{
	public:
		SQLiteBase(const std::string &databaseName,
			bool readOnly = false, bool onDemand = true);
		virtual ~SQLiteBase();

		static bool check(const std::string &databaseName);

		bool backup(const std::string &destDatabaseName,
			int pagesCount = 5, bool retryOnLock = true);

		virtual bool isOpen(void) const;

		virtual bool reopen(const std::string &databaseName);

		virtual bool alterTable(const std::string &tableName,
			const std::string &columns,
			const std::string &newDefinition);

		virtual bool beginTransaction(void);

		virtual bool rollbackTransaction(void);

		virtual bool endTransaction(void);

		virtual bool executeSimpleStatement(const std::string &sql);

		virtual SQLResults *executeStatement(const char *sqlFormat, ...);

		virtual bool prepareStatement(const std::string &statementId,
			const std::string &sqlFormat);

		virtual SQLResults *executePreparedStatement(const std::string &statementId,
			const std::vector<std::string> &values);

		virtual SQLResults *executePreparedStatement(const std::string &statementId,
			const std::vector<std::pair<std::string, SQLRow::SQLType> > &values);

	protected:
		bool m_onDemand;
		bool m_inTransaction;
		sqlite3 *m_pDatabase;
		std::map<std::string, sqlite3_stmt*> m_statements;

		void executeSimpleStatement(const std::string &sql, int &execError);

		void open(void);

		void close(void);

	private:
		SQLiteBase(const SQLiteBase &other);
		SQLiteBase &operator=(const SQLiteBase &other);

};

#endif // _SQLITE_BASE_H
