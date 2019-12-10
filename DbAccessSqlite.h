/*
* DbAccessSqLite.h
*
*  Created on: Sept. 28, 2015
*  \copyright 2015 DCBlaha.  Distributed under the Mozilla Public License 2.0.
*/

// This file allows using an SQLite database without requiring any
// source files from the SQLite project.  This performs a run time load
// of the sqlite3.dll.

#ifndef DB_ACCESS_SQLITE
#define DB_ACCESS_SQLITE

#include "SQLite.h"
#include "DbResult.h"
//#include <cstddef>		// For std::byte
#ifdef __linux__
typedef unsigned char byte;
#endif
#include <vector>

/// Provides the overall access to the database.
class DbAccess:public SQLite, public SQLiteListener
    {
    public:
        DbAccess():
            pragmaSetCaching(false), transactSeconds(5)
            {
            setListener(this);
            }
        /// Open the database.
        /// @param dbName This should be the database name without the path.
        DbResult open(char const *dbName);
        int getTransactSeconds()
            { return transactSeconds; }

        /// This is for optimization. This will only be set if the sizes were
        /// not set in the pragma config file.
        DbResult setCaching(int cacheSize=-1, int pageSize=-1);
        DbResult getErrorInfo() const
            { return SqlErrorResult; }

        /// Define this in the derived class to handle SQL errors.
        virtual void SQLError(int /*retCode*/, char const *errMsg) override
            {
            SqlErrorResult.setWarning(errMsg);
            }
        virtual void SQLResultCallback(int /*numColumns*/, char ** /*colVal*/,
            char ** /*colName*/)
            {}

    private:
        DbResult SqlErrorResult;
        bool pragmaSetCaching;
        int transactSeconds;
    };

/// Provides the ability to execute statements to the database.
class DbStatement:public SQLiteStatement
    {
    public:
        explicit DbStatement(DbAccess &db):
            SQLiteStatement(db), mDb(db)
            {}
        DbStatement(DbAccess &db, char const *query):
	    SQLiteStatement(db, query), mDb(db)
            {}
        // Set the query string. Bind the values for the query using bindValues().
        DbResult set(char const *query);

        // If the query is failing, make sure the bound values are in memory.
        // Search for SQLITE_STATIC in the code for more info.
        DbResult testRow(bool &gotRow);

        // If the query is failing, make sure the bound values are in memory.
        // Search for SQLITE_STATIC in the code for more info.
        DbResult getRow();
        DbResult execute();

        /// These are not implemented, but are not needed since they provide an
        /// interface for compatibility with MySQL optimization.
        void startMultiRowInsert(int /*numRows*/)
            {}
        DbResult endMultiRowInsert()
            { return DbResult(); }

        // columnIndex is base 0.
        DbResult getColumnBlob(int columnIndex, std::vector<byte> &bytes);

        DbResult getLastInsertedRowIndex(int64_t &lastInsertedRowIndex);

        // WARNING - The bind values must be kept around while the statement is executing.
        DbResult bindValues(std::vector<std::string> const &values);
        static DbResult getDbResult(int sqliteErr)
            {
            DbResult result;
            if(IS_SQLITE_ERROR(sqliteErr))
                { result.setError("Error with db"); }
            return result;
            }
        DbResult getErrorInfo() const
            { return mDb.getErrorInfo(); }

    private:
        DbAccess &mDb;
    };

/// Defines a transaction so that the transaction ends on destruction.
class DbTransaction:public SQLiteTransaction
    {
    public:
        explicit DbTransaction(DbAccess &db):
	    SQLiteTransaction(db)
	    {}
    };

#endif

