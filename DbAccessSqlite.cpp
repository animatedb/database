/*
* DbAccessSqLite.cpp
*
*  Created on: Sept. 28, 2015
*  \copyright 2015 DCBlaha.  Distributed under the Mozilla Public License 2.0.
*/
#define _CRT_SECURE_NO_WARNINGS 1
#include <algorithm>    // for std:find()
#include <stdlib.h>     // For getenv()

#include "DbAccess.h"
#include "DbString.h"

#if(DATABASE == DB_SQLITE)

static std::string getSqliteLibraryPath()
    {
    return "libsqlite3.so.0";
    }

DbResult DbAccess::open(char const *dbName)
    {
    DbResult result;
    bool gotDll = false;
    std::string libPath = getSqliteLibraryPath();
    if(loadDbLib(libPath.c_str()))
        {
        gotDll = true;
        }
    if(!gotDll)
        {
        result.setError("Unable to open sqlite3.dll");
        result.insertContext(getDbResultString(getErrorInfo()));
        }
    if(result.isOk())
		{
        if(IS_SQLITE_ERROR(SQLite::openDb(dbName)))
            {
			std::string errStr = "Unable to open database file ";
			errStr += dbName;
            result.setError(errStr);
            result.insertContext(getDbResultString(getErrorInfo()));
            }
        }
    return result;
    }

DbResult DbAccess::setCaching(int cacheSize, int pageSize)
    {
    DbResult result;
    if(!pragmaSetCaching)
        {
        DbStatement stmt(*this);
        std::string pragma;
        if(cacheSize != -1)
            {
            pragma = "PRAGMA cache_size=" + std::to_string(cacheSize);
            stmt.set(pragma.c_str());
            result = stmt.execute();
            }
        if(result.isOk())
            {
            if(pageSize != -1)
                {
                pragma = "PRAGMA page_size=" + std::to_string(pageSize);
                stmt.set(pragma.c_str());
                result = stmt.execute();
                }
            }
        }
    return result;
    }

static std::string osPathJoin(std::string const &dir, std::string const &fn)
    {
    std::string joinedStr = dir;
    size_t lastCharIndex = dir.length() - 1;
    if(dir[lastCharIndex] != '\\' && dir[lastCharIndex] != '/')
        {
        joinedStr += '\\';
        }
    return (joinedStr + fn);
    }

DbResult DbStatement::set(char const *query)
    {
    DbResult result;
    int retCode = SQLiteStatement::set(query);
    if(!IS_SQLITE_OK(retCode))
        {
        result.setError("Unable to set statement");
        result.insertContext(getDbResultString(getErrorInfo()));
        }
    return result;
    }

DbResult DbStatement::testRow(bool &gotRow)
    {
    int retCode = SQLiteStatement::step();
    gotRow = (retCode == SQLITE_ROW);
    DbResult result;
    if(!IS_SQLITE_OK(retCode))
        {
        result.setError("Unable to test row");
        result.insertContext(getDbResultString(getErrorInfo()));
        }
    return result;
    }

DbResult DbStatement::getRow()
    {
    bool gotRow;
    DbResult result = testRow(gotRow);
    if(result.isOk() && !gotRow)
        {
        result.setError("Unable to get row");
        result.insertContext(getDbResultString(getErrorInfo()));
        }
    return result;
    }

DbResult DbStatement::execute()
    { 
    int retCode = SQLiteStatement::step();
    // SQLITE_ROW is returned from pragma executing.
    bool executed = (retCode == SQLITE_DONE || retCode == SQLITE_ROW);
    DbResult result;
    if(!IS_SQLITE_OK(retCode) || !executed)
        {
        result.setError("Unable to execute");
        result.insertContext(getDbResultString(getErrorInfo()));
        }
    return result;
    }

DbResult DbStatement::getColumnBlob(int columnIndex, std::vector<byte> &bytes)
    {
    DbResult result;
    int numBytes;
    void const *blob = SQLiteStatement::getColumnBlob(columnIndex, numBytes);
    if(blob)
        {
        /// @todo - there is an extra memory copy here.
        bytes.resize(numBytes);
        for(size_t i=0; i<numBytes; i++)
            {
            bytes[i] = static_cast<byte const *>(blob)[i];
            }
        }
    else
        {
        bytes.clear();
        }
    return result;
    }

DbResult DbStatement::bindValues(std::vector<std::string> const &values)
    {
    DbResult result;
    if(values.size() == getBindCount())
        {
        for(size_t i=0; i<values.size() && result.isOk(); i++)
            {
            result = getDbResult(bindText(static_cast<int>(i+1), values[i].c_str()));
            }
        }
    else
        {
        std::string errStr = "Incorrect bind count. Query expects ";
        errStr += std::to_string(getBindCount());
        errStr += " but the number of values is ";
        errStr += std::to_string(values.size());
        errStr += ".\n";
        for(auto const &value : values)
            {
            errStr += value;
            errStr += ", ";
            }
        result.setError(errStr.c_str());
        result.insertContext(getDbResultString(getErrorInfo()));
        }
    return result;
    }

DbResult DbStatement::getLastInsertedRowIndex(int64_t &lastInsertedRowId)
    {
    DbString selectStr;
    selectStr.SELECT("last_insert_rowid()");
    DbResult result = set(selectStr.getDbStr().c_str());
    if(result.isOk())
        {
        result = getRow();
        }
    if(result.isOk())
        {
        lastInsertedRowId = getColumnInt64(0);
        }
    return result;
    }

#endif

