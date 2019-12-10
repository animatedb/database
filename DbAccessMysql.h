/*
* DbAccessMysql.h
*
*  Created: 2016
*  \copyright 2016 DCBlaha.  Distributed under the Mozilla Public License 2.0.
*/
// Provides access for the MySQL database.
// Provide generic names for DB Access.

/// This file contains wrapper classes that hides which database is used.

#ifndef DB_ACCESS_SQLITE
#define DB_ACCESS_SQLITE

// Include path on Windows may be "C:\Program Files\MySQL\MySQL Connector C 6.1\include"
// Library path may be "C:\Program Files\MySQL\MySQL Connector C 6.1\lib"
// Library is libmysql.lib
#include "mysql.h"
#include "StringUtil.h"
#include <stdint.h>
#include "DbResult.h"
#include <vector>

typedef uint8_t byte;
#define RETURN_DOUBLE_NULL_AS_NAN 1
#define USE_PREPARED_STATEMENT 1

/// Provides the overall access to the database.
//
// Example use (error handling is not shown):
// Select:
//      DbStatement stmt(db);
//      stmt.set("select...");
//      stmt.bindFloat(":p1", val);
//      gotrow = true;
//      while(gotrow)
//          {
//          stmt.testRow(gotrow);   // This does an execute() the first time.
//          }
//  
// Insert, delete, etc.:
//      DbStatement stmt(db);
//      stmt.set("insert...");
//      stmt.execute();
//      while(moredata)
//          {
//          stmt.bindFloat(":p1", val);
//          reset();    // This is only needed if more parameters will be written.
//          }
// 
class DbAccess
    {
    public:
        DbAccess():
            mConnection(nullptr)
            {}

        ~DbAccess()
            { close(); }

        DbResult setCaching(int /*size*/)
            { return DbResult(); }

        /// Open the database.
        /// @param dbName This should be the database name without the path.
        DbResult open(char const *dbName);
        void close();

        MYSQL *getConnection()
            { return mConnection; }

        DbResult getErrorInfo() const
            { return result; }

    private:
        MYSQL *mConnection;
        DbResult result;
    };

/// Provides the ability to execute statements to the database.
class DbStatement
    {
    public:
        // One way that this can be used is to make a long running prepared
        // statement, and then run small normal statements to get or set specific items.
        explicit DbStatement(DbAccess &db):
            mDb(db), mPreparedStmt(nullptr),
            mUsePreparedStatement(USE_PREPARED_STATEMENT), mDbDataState(DBS_Init),
            mMultiRowSize(0), mMultiRowIndex(0), mMultiRowParams(0)
            {}
	    DbStatement(DbAccess &db, char const *query):
            mDb(db), mPreparedStmt(nullptr),
            mUsePreparedStatement(USE_PREPARED_STATEMENT), mDbDataState(DBS_Init),
            mMultiRowSize(0), mMultiRowIndex(0), mMultiRowParams(0)
		    { set(query); }
        ~DbStatement()
            {
            close();
            }

        void close();

        void usePreparedStatement(bool prep)
            { mUsePreparedStatement = prep; }

        // Set the query string. Bind the values for the query using bindValues().
        // Resets the internal state.
        DbResult set(char const *query);

        // Resets the statement to the beginning. This does not clear bindings.
        // This should be used to redo an insert, and then the bindings do not
        // need to be cleared.
	    DbResult reset();

        // Execute the query string with bound parameters.
        // execute is optional if getRow or testRow is called immediately after
        // bind parameters.
        DbResult execute();

        /// Start multiple row inserts. This is often many times faster than
        /// individual inserts. Make sure to call endMultiRowInsert() in order
        /// to store all data. This binds many values to during a single execute.
        void startMultiRowInsert(int numRows);
        DbResult endMultiRowInsert();
        bool isMultiRow() const
            { return mMultiRowSize > 1; }

        // Attmpts to get a row. Returns true if a row was retrieved.
        // If the query is failing, make sure the bound values are in memory.
        // Search for SQLITE_STATIC in the code for more info.
        DbResult testRow(bool &gotRow);

        // Gets a row and creates an error if the row does not exist.
        // If the query is failing, make sure the bound values are in memory.
        // Search for SQLITE_STATIC in the code for more info.
        DbResult getRow();

        // This has been replaced with StringUtil.h, StringGetDictValues()
//        static std::vector<std::string> getValuesFromDictString(std::string const &str);

        //********* Bind parameter values

        // Bind an array of strings in order.
        DbResult bindValues(std::vector<std::string> const &values);

#define NULL_SQL_INDICATOR "\001"
        void bindNull(int ordinal)
            { setBindValue(ordinal, NULL_SQL_INDICATOR); }
        void bindInt(int ordinal, int val)
            {
            std::string str = std::to_string(val);
            setBindValue(ordinal, str.c_str());
            }
        void bindInt(char const *param, int val)
            {
            std::string str = std::to_string(val);
            setBindValue(param, str.c_str());
            }
        void bindInt64(char const *param, int64_t val)
            {
            std::string str = std::to_string(val);
            setBindValue(param, str.c_str());
            }
        void bindFloat(int ordinal, float val)
            {
            std::string str = std::to_string(val);
            setBindValue(ordinal, str.c_str());
            }
        void bindFloat(char const *param, float val)
            { 
            std::string str = std::to_string(val);
            setBindValue(param, str.c_str());
            }
        void bindDouble(int ordinal, double val)
            {
            std::string str = std::to_string(val);
            setBindValue(ordinal, str.c_str());
            }
        void bindDouble(char const *param, double val)
            { 
            std::string str = std::to_string(val);
            setBindValue(param, str.c_str());
            }
        // WARNING - SQLITE_STATIC means that the val parameter must be around while
        // the statement is executing.
        void bindText(int ordinal, char const *val)
            {
            setBindValue(ordinal, val);
            }
        void bindText(char const *param, char const *val)
            {
            setBindValue(param, val);
            }
//        void bindBlob(int ordinal, const void *bytes, int elNumBytes)
//            {
//            }

        //********* Get values

        int getColumnInt(int columnIndex) const
            { return std::stoi(mBindResults[columnIndex]); }
        int64_t getColumnInt64(int columnIndex) const
            { return std::stoll(mBindResults[columnIndex]); }
        bool getColumnBool(int columnIndex) const
            { return mBindResults[columnIndex] != ""; }
#if(RETURN_DOUBLE_NULL_AS_NAN)
        double getColumnDouble(int columnIndex) const
            {
            if(mBindResults[columnIndex].length() == 0)
                { return std::numeric_limits<double>::quiet_NaN(); }
            else
                { return std::stod(mBindResults[columnIndex]); }
            }
#else
        double getColumnDouble(int columnIndex) const
            { return std::stod(mBindResults[columnIndex]); }
#endif
        // Since this must be called in order, this is removed.
//    int getColumnBytes(int columnIndex) const
//        { return mDb->sqlite3_column_bytes(mStatement, columnIndex); }

        /// This will return nullptr if the database contains NULL.
        char const *getColumnText(int columnIndex) const
            { return mBindResults[columnIndex].c_str(); }
        // columnIndex is base 0.
        /// @todo - this could be made more efficient by not copying data.
        DbResult getColumnBlob(int columnIndex, std::vector<byte> &bytes)
            {
            DbResult result;
            bytes.resize(mBindResults[columnIndex].size());
            memcpy(bytes.data(), mBindResults[columnIndex].data(), bytes.size());
            return result;
            }

        DbResult getLastInsertedRowIndex(int64_t &lastInsertedRowIndex);

        static DbResult getDbResult(int sqliteErr)
            {
            DbResult result;
//            if(IS_SQLITE_ERROR(sqliteErr))
//                { result.setError("Error with db"); }
            return result;
            }
        DbResult getErrorInfo() const
            { return mDb.getErrorInfo(); }

    private:
        std::string mQueryString;
        // https://stackoverflow.com/questions/1176352/pdo-prepared-inserts-multiple-rows-in-single-query
        // This stuff has never been tested yet. Multiple row insert without prepared
        // statements was tested, and was 100 times (row size=200) faster than single row insert.
        int mMultiRowSize;
        int mMultiRowIndex;
        int mMultiRowParams;

        // The functions that get data execute through these states.
        // The reset and set functions also reset to the initial state.
        // These states are very close to enum enum_mysql_stmt_state
        //      MYSQL_STMT_INIT_DONE=1, MYSQL_STMT_PREPARE_DONE,
        //      MYSQL_STMT_EXECUTE_DONE, MYSQL_STMT_FETCH_DONE
        enum DbDataStates
            {
            DBS_Init,
            DBS_BindParams,
            DBS_Execute,    // Runs the execute SQL function. The initial state.
            DBS_Get,        // Gets the bind data results for prepared statements.
            DBS_Extract     // Gets the row data from the bind value results.
            };
        enum DbDataStates mDbDataState;

        // Used for normal or prepared statements.
        std::vector<std::string> mBindValues;
        std::vector<std::string> mBindResults;

        // Prepared statement storage
        MYSQL_STMT *mPreparedStmt;

        std::vector<unsigned long> mPreparedBindLengths;
        std::vector<my_bool> mPreparedBindIsNull;
        std::vector<my_bool> mPreparedBindError;
        std::vector<std::string> mPreparedBindValues;   // buffers for SQL parameters;
        std::vector<MYSQL_BIND> mPreparedBindArray;     // full definitions of SQL parameters

        std::vector<std::string> mPreparedResultStrings;    // buffers for returned field results
        std::vector<my_bool> mPreparedResultIsNull;         // buffers for returned is null results
        std::vector<unsigned long> mPreparedResultLengths;  // lengths of returned field results
        std::vector<MYSQL_BIND> mPreparedBindResultArray;   // definitions of returned field results

        // MySQL requires ending results before starting a new query/execute.
        // This may not be true for multi-threading.
        // Making the result static allows cleaning up previous statements.
        static MYSQL_RES *mResult;
        static MYSQL_RES *mPreparedResult;
        bool mUsePreparedStatement;

        DbAccess &mDb;

        // ordinal is base 1.
        void setBindValue(int ordinal, char const *str);
        void setBindValue(char const *param, char const *str);
        DbResult getResults(std::vector<std::string> &bindResults);
        DbResult getNormalResults(std::vector<std::string> &bindResults);
        DbResult getPreparedResults(std::vector<std::string> &bindResults);
        DbResult bindPreparedParameters(MYSQL *conn, MYSQL_STMT *stmt,
            std::vector<std::string> const &bindValues);
    };

/// Defines a transaction so that the transaction ends on destruction.
class DbTransaction
    {
    public:
        explicit DbTransaction(DbAccess &db):
            dbAccess(db)
            {}
        ~DbTransaction()
            {
            end();
            }
        // By default, mysql runs with autocommit enabled. Not recommended for InnoDB tables.
        void transact()
            {
            DbStatement stmt(dbAccess);
            stmt.usePreparedStatement(false);
            DbResult result = end(stmt);
            if(result.isOk())
                {
                start(stmt);
                }
            }
        void end()
            {
            DbStatement stmt(dbAccess);
            stmt.usePreparedStatement(false);
            end(stmt);
            }
    private:
        DbAccess &dbAccess;

        DbResult start(DbStatement &stmt)
            {
            stmt.set("START TRANSACTION");
            return stmt.execute();
            }
        DbResult end(DbStatement &stmt)
            {
            stmt.set("COMMIT");
            return stmt.execute();
            }
    };

class SQLiteMutex
    {
    public:
        SQLiteMutex(DbAccess const &)
            {}
    };

#endif

