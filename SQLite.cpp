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

	loadModuleSymbol("sqlite3_column_blob", (ModuleProcPtr*)&sqlite3_column_blob);
	loadModuleSymbol("sqlite3_column_bytes", (ModuleProcPtr*)&sqlite3_column_bytes);
    loadModuleSymbol("sqlite3_column_text", (ModuleProcPtr*)&sqlite3_column_text);
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
        sqlite3_close(mDb);
        mDb = nullptr;
        }
    }

bool SQLite::execDb(const char *sql)
    {
    char *errMsg = nullptr;
    int retCode = sqlite3_exec(mDb, sql, &resultsCallback, this, &errMsg);
    bool success = handleRetCode(retCode, errMsg);
    if(errMsg)
        {
        sqlite3_free(errMsg);
        }
    return success;
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

bool SQLite::handleRetCode(int retCode, char const *errStr)
    {
    if(retCode != SQLITE_OK && mListener)
        {
        mListener->SQLError(retCode, errStr);
        }
    return(retCode == SQLITE_OK);
    }

SQLiteStatement::~SQLiteStatement()
	{
	if(mStatement)
		{
		mDb.sqlite3_finalize(mStatement);
		mStatement = nullptr;
		}
	}

void SQLiteStatement::set(char const *query)
    {
    if(mStatement)
        {
        reset();
        }
	mStatement = nullptr;
	int res = mDb.sqlite3_prepare_v2(mDb.getDb(), query, -1, &mStatement, nullptr);
    if(res == SQLITE_ERROR)
        {
        ErrorTrace(mDb.sqlite3_errmsg(mDb.getDb()));
        }
    }

int SQLiteStatement::step()
	{
    int res = mDb.sqlite3_step(mStatement);
    if(res != SQLITE_OK && res != SQLITE_DONE && res != SQLITE_ROW)
        {
        ErrorTrace(mDb.sqlite3_errmsg(mDb.getDb()));
        }
    return res;
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
