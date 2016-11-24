/*
* SQLite.h
*
*  Created on: Sept. 28, 2015
*  \copyright 2015 DCBlaha.  Distributed under the Mozilla Public License 2.0.
*/

// This file allows using an SQLite database without requiring any
// source files from the SQLite project.  This performs a run time load
// of the sqlite3.dll.

#ifndef SQLITE_H
#define SQLITE_H
#include "Module.h"
#include <string.h>

// This is the C style interface to the run-time library.
extern "C"
{
typedef struct sqlite3 sqlite3;
typedef int (*SQLite_callback)(void*,int,char**,char**);
typedef void *sqlite3_mutex_ptr;
typedef struct sqlite3_stmt sqlite3_stmt;

struct SQLiteInterface
    {
    int (*sqlite3_open)(const char *filename, sqlite3 **ppDb);
    int (*sqlite3_close)(sqlite3 *pDb);
    int (*sqlite3_exec)(sqlite3 *pDb, const char *sql,
        SQLite_callback callback, void *callback_data,
        char **errmsg);

	// Statement related interfaces
	int (*sqlite3_prepare_v2)(sqlite3 *pDb, const char *sql, int nBytes,
		sqlite3_stmt **ppStmt, const char **pzTail);
    // Memory returned must not be freed by application.
    const char *(*sqlite3_errmsg)(sqlite3*);
	int (*sqlite3_finalize)(sqlite3_stmt *pStmt);
	int (*sqlite3_step)(sqlite3_stmt*);
	int (*sqlite3_reset)(sqlite3_stmt*);

    int (*sqlite3_bind_parameter_index)(sqlite3_stmt*, const char *zName);
    // Bind ordinals are base one.
    int (*sqlite3_bind_null)(sqlite3_stmt*,int ordinal);
    int (*sqlite3_bind_int)(sqlite3_stmt*, int ordinal, int val);
    int (*sqlite3_bind_double)(sqlite3_stmt*, int ordinal, double val);
    int (*sqlite3_bind_text)(sqlite3_stmt*,int,const char*,int,void(*)(void*));
	int (*sqlite3_bind_blob)(sqlite3_stmt*, int ordinal, const void *bytes,
		int elSize, void(*)(void*));

	int (*sqlite3_column_int)(sqlite3_stmt*, int iCol);
	double (*sqlite3_column_double)(sqlite3_stmt*, int iCol);
    const void *(*sqlite3_column_blob)(sqlite3_stmt*, int iCol);
	int (*sqlite3_column_bytes)(sqlite3_stmt*, int iCol);
	char const *(*sqlite3_column_text)(sqlite3_stmt*, int iCol);

    // SQLITE_MUTEX_FAST, SQLITE_MUTEX_RECURSIVE
    sqlite3_mutex_ptr (*sqlite3_mutex_alloc)(int);
    void (*sqlite3_mutex_free)(sqlite3_mutex_ptr);
    void (*sqlite3_mutex_enter)(sqlite3_mutex_ptr);
    int (*sqlite3_mutex_try)(sqlite3_mutex_ptr);
    void (*sqlite3_mutex_leave)(sqlite3_mutex_ptr);

    void (*sqlite3_free)(void*);
    };
};

#define SQLITE_MUTEX_FAST 0
#define SQLITE_MUTEX_RECURSIVE 1


// This is normally defined in sqlite3.h, so if more error codes are needed,
// get them from there.
#define SQLITE_OK 0
#define SQLITE_ERROR 1
#define SQLITE_ROW 100
#define SQLITE_DONE 101
inline bool IS_SQLITE_ERROR(int x)  { return x>0 && x<100; }
inline bool IS_SQLITE_OK(int x)  { return !IS_SQLITE_ERROR(x); }
typedef void (*sqlite3_destructor_type)(void*);
#define SQLITE_STATIC      ((sqlite3_destructor_type)0)
#define SQLITE_TRANSIENT   ((sqlite3_destructor_type)-1)

/// This loads the symbols from the DLL into the interface.
class SQLiteImporter:public SQLiteInterface, public Module
    {
    public:
        void loadSymbols();
    };

/// Functions that return errors and results will call functions in this
/// listener.
class SQLiteListener
    {
    public:
        virtual void SQLError(int retCode, char const *errMsg) = 0;
        /// This is called for each row returned from a query, so it can be
        /// called many times after a single exec call.
        virtual void SQLResultCallback(int numColumns, char **colVal,
            char **colName) = 0;
    };

/// This is a wrapper class for the SQLite functions.
class SQLite:public SQLiteImporter
    {
    public:
		friend class SQLiteStatement;
        SQLite():
            mDb(nullptr), mListener(nullptr)
            {}
        // This will release the sqlite3.dll since the SQLiteImporter does so.
        ~SQLite()
            {
            close();
            }
        void setListener(SQLiteListener *listener)
            { mListener = listener; }
        /// Load the SQLite library.
        /// The libName is usually libsqlite3.so.? on linux, and sqlite3.dll on windows.
        bool loadDbLib(char const *libName);
        /// The dbName is the name of the file that will be opened.
        int openDb(char const *dbName)
            { return handleRetCode(sqlite3_open(dbName, &mDb)); }
        /// This is called from the destructor, so does not need an additional
        /// call unless it must be closed early.
        void closeDb();
        /// Execute an SQL query.  If there are results, they will be reported
        /// through the listener.
        int execDb(const char *sql);
        sqlite3 const *const getDb() const
		    { return mDb; }
        sqlite3 *const getDb()
		    { return mDb; }

    private:
        sqlite3 *mDb;
        SQLiteListener *mListener;

        /// This is called from the sqlite3_exec call, and sends the results to
        /// the listener.
        static int resultsCallback(void *customData, int numColumns,
            char **colValues, char **colNames);
        /// If the retCode indicates an error, then the errStr is sent to the
        /// listener.
        int handleRetCode(int retCode);
    };

// This was created for type safety.
struct SQLiteResult
    {
    int code;
    };

class SQLiteStatement
{
public:
    SQLiteStatement(SQLite &db):
        mDb(db), mStatement(nullptr)
        {}
	SQLiteStatement(SQLite &db, char const *query):
		mDb(db)
		{ set(query); }
    // This does a finalize.
	~SQLiteStatement();
    void setDb(SQLite &db)
        { mDb = db; }
    SQLite &getDb()
        { return mDb; }
    int set(char const *query);
	int step();
	int reset()
        { return mDb.handleRetCode(mDb.sqlite3_reset(mStatement)); }

	int getColumnInt(int columnIndex) const
		{ return mDb.sqlite3_column_int(mStatement, columnIndex); }
	double getColumnDouble(int columnIndex) const
		{ return mDb.sqlite3_column_double(mStatement, columnIndex); }
    // Since this must be called in order, this is removed.
//    int getColumnBytes(int columnIndex) const
//		{ return mDb->sqlite3_column_bytes(mStatement, columnIndex); }
	char const *getColumnText(int columnIndex) const
		{ return mDb.sqlite3_column_text(mStatement, columnIndex); }
	void const *getColumnBlob(int columnIndex, int &numBytes) const
		{
        void const *blob = mDb.sqlite3_column_blob(mStatement, columnIndex);
        numBytes = mDb.sqlite3_column_bytes(mStatement, columnIndex);
        return blob;
        }

    // Bind ordinals are base one. The ordinal parameters can be passed a
    // bind string such as ":id". Then this will bind a value to a parameter
    // in the query such as "select from tablex where(id=:id);"
    int bindNull(int ordinal)
		{ return mDb.handleRetCode(mDb.sqlite3_bind_null(mStatement, ordinal)); }
    int bindNull(char const *param)
		{
        return mDb.handleRetCode(mDb.sqlite3_bind_null(mStatement,
            mDb.sqlite3_bind_parameter_index(mStatement, param)));
        }
	int bindInt(int ordinal, int val)
		{ return mDb.handleRetCode(mDb.sqlite3_bind_int(mStatement, ordinal, val)); }
	int bindInt(char const *param, int val)
		{
        return mDb.handleRetCode(mDb.sqlite3_bind_int(mStatement, 
            mDb.sqlite3_bind_parameter_index(mStatement, param), val));
        }
	int bindDouble(int ordinal, double val)
		{ return mDb.handleRetCode(mDb.sqlite3_bind_double(mStatement, ordinal, val)); }
	int bindDouble(char const *param, double val)
		{ 
        return mDb.handleRetCode(mDb.sqlite3_bind_double(mStatement, 
            mDb.sqlite3_bind_parameter_index(mStatement, param), val));
        }
	int bindText(int ordinal, char const *val)
		{
        /// @todo - strlen may not be right for UTF-8
        return mDb.handleRetCode(mDb.sqlite3_bind_text(mStatement, ordinal, val,
            static_cast<int>(strlen(val)), nullptr));
        }
	int bindText(char const *param, char const *val)
		{
        /// @todo - strlen may not be right for UTF-8
        return mDb.handleRetCode(mDb.sqlite3_bind_text(mStatement,
            mDb.sqlite3_bind_parameter_index(mStatement, param), val,
            static_cast<int>(strlen(val)), nullptr));
        }
	int bindBlob(int ordinal, const void *bytes, int elNumBytes)
		{
        return mDb.handleRetCode(mDb.sqlite3_bind_blob(mStatement, ordinal,
            bytes, elNumBytes, SQLITE_STATIC));
        }

private:
	sqlite3_stmt *mStatement;
	SQLite &mDb;
};

class SQLiteTransaction
    {
    public:
	    SQLiteTransaction(SQLite &db):
	        mDb(db), mInTransaction(false)
		    {
		    begin();
		    }

	    ~SQLiteTransaction()
		    { end(); }
	    void transact()
		    {
		    end();
		    begin();
		    }
	    void begin();
	    void end();

    private:
	    SQLite &mDb;
	    bool mInTransaction;
    };

class SQLiteMutex
    {
    public:
        SQLiteMutex(SQLite const &db):
            mDb(db)
            {
            mMutexPtr = db.sqlite3_mutex_alloc(SQLITE_MUTEX_FAST);
            }
        void Enter()
            {
            mDb.sqlite3_mutex_enter(mMutexPtr);
            }
        void Leave()
            {
            mDb.sqlite3_mutex_leave(mMutexPtr);
            }
        ~SQLiteMutex()
            {
            mDb.sqlite3_mutex_free(mMutexPtr);
            }
    private:
        SQLite const &mDb;
        sqlite3_mutex_ptr mMutexPtr;
    };

class SQLiteMutexGuard
    {
    public:
        SQLiteMutexGuard(SQLiteMutex &mutex):
            mMutex(mutex)
            {
            mMutex.Enter();
            }
        ~SQLiteMutexGuard()
            {
            mMutex.Leave();
            }
        SQLiteMutex &mMutex;
    };

#endif
