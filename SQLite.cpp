/*
* SQLite.cpp
*
*  Created on: Sept. 28, 2015
*  \copyright 2015 DCBlaha.  Distributed under the Mozilla Public License 2.0.
*/

#include "SQLite.h"

void SQLiteImporter::loadSymbols()
    {
    loadModuleSymbol("sqlite3_open", (ModuleProcPtr*)&sqlite3_open);
    loadModuleSymbol("sqlite3_close", (ModuleProcPtr*)&sqlite3_close);
    loadModuleSymbol("sqlite3_exec", (ModuleProcPtr*)&sqlite3_exec);

	loadModuleSymbol("sqlite3_prepare_v2", (ModuleProcPtr*)&sqlite3_prepare_v2);
	loadModuleSymbol("sqlite3_errmsg", (ModuleProcPtr*)&sqlite3_errmsg);
	loadModuleSymbol("sqlite3_finalize", (ModuleProcPtr*)&sqlite3_finalize);
	loadModuleSymbol("sqlite3_step", (ModuleProcPtr*)&sqlite3_step);
	loadModuleSymbol("sqlite3_reset", (ModuleProcPtr*)&sqlite3_reset);

    loadModuleSymbol("sqlite3_bind_parameter_index", (ModuleProcPtr*)&sqlite3_bind_parameter_index);
    loadModuleSymbol("sqlite3_bind_blob", (ModuleProcPtr*)&sqlite3_bind_blob);
	loadModuleSymbol("sqlite3_bind_null", (ModuleProcPtr*)&sqlite3_bind_null);
	loadModuleSymbol("sqlite3_bind_int", (ModuleProcPtr*)&sqlite3_bind_int);
	loadModuleSymbol("sqlite3_bind_double", (ModuleProcPtr*)&sqlite3_bind_double);
	loadModuleSymbol("sqlite3_bind_text", (ModuleProcPtr*)&sqlite3_bind_text);

	loadModuleSymbol("sqlite3_column_int", (ModuleProcPtr*)&sqlite3_column_int);
	loadModuleSymbol("sqlite3_column_double", (ModuleProcPtr*)&sqlite3_column_double);
	loadModuleSymbol("sqlite3_column_blob", (ModuleProcPtr*)&sqlite3_column_blob);
	loadModuleSymbol("sqlite3_column_bytes", (ModuleProcPtr*)&sqlite3_column_bytes);
    loadModuleSymbol("sqlite3_column_text", (ModuleProcPtr*)&sqlite3_column_text);

    loadModuleSymbol("sqlite3_mutex_alloc", (ModuleProcPtr*)&sqlite3_mutex_alloc);
    loadModuleSymbol("sqlite3_mutex_free", (ModuleProcPtr*)&sqlite3_mutex_free);
    loadModuleSymbol("sqlite3_mutex_enter", (ModuleProcPtr*)&sqlite3_mutex_enter);
    loadModuleSymbol("sqlite3_mutex_try", (ModuleProcPtr*)&sqlite3_mutex_try);
    loadModuleSymbol("sqlite3_mutex_leave", (ModuleProcPtr*)&sqlite3_mutex_leave);

    // This must be called for returned error strings.
    loadModuleSymbol("sqlite3_free", (ModuleProcPtr*)&sqlite3_free);
    }

bool SQLite::loadDbLib(char const *libName)
    {
    close();
    bool success = Module::open(libName);
    if(success)
        {
        loadSymbols();
        }
    return success;
    }

void SQLite::closeDb()
    {
    if(mDb)
        {
        handleRetCode(sqlite3_close(mDb));
        mDb = nullptr;
        }
    }

int SQLite::execDb(const char *sql)
    {
    int retCode = sqlite3_exec(mDb, sql, &resultsCallback, this, nullptr);
    return handleRetCode(retCode);
    }

int SQLite::resultsCallback(void *customData, int numColumns,
    char **colValues, char **colNames)
    {
    SQLite *sqlite = static_cast<SQLite*>(customData);
    if(sqlite->mListener)
        {
        sqlite->mListener->SQLResultCallback(numColumns, colValues, colNames);
        }
    return 0;
    }

int SQLite::handleRetCode(int retCode)
    {
    if(IS_SQLITE_ERROR(retCode) && mListener)
        {
        mListener->SQLError(retCode, sqlite3_errmsg(mDb));
        }
    return(retCode);
    }

SQLiteStatement::~SQLiteStatement()
	{
	if(mStatement)
		{
		mDb.handleRetCode(mDb.sqlite3_finalize(mStatement));
		mStatement = nullptr;
		}
	}

int SQLiteStatement::set(char const *query)
    {
    if(mStatement)
        {
        reset();
        }
	mStatement = nullptr;
	int res = mDb.sqlite3_prepare_v2(mDb.getDb(), query, -1, &mStatement, nullptr);
    return mDb.handleRetCode(res);
    }

int SQLiteStatement::step()
    {
    int res = mDb.sqlite3_step(mStatement);
    return mDb.handleRetCode(res);
    }

void SQLiteTransaction::begin()
	{
	if(!mInTransaction)
		{
		mDb.execDb("BEGIN TRANSACTION");
		mInTransaction = true;
		}
	}

void SQLiteTransaction::end()
	{
	if(mInTransaction)
		{
		mDb.execDb("END TRANSACTION");
		mInTransaction = false;
		}
	}
