/*
* DbAccessMySql.cpp
*
*  Created: 2016
*  \copyright 2016 DCBlaha.  Distributed under the Mozilla Public License 2.0.
*/
#include "DbAccess.h"

#if(DATABASE == DB_MYSQL)

#if(0)
#define DEBUG_PRINT(x) printf("%s\n", x);
#else
#define DEBUG_PRINT(x)
#endif

// These comments need to be retested since the code has changed.
//
// GET_LENGTHS
//      0 = Get the lengths for each row using mysql_stmt_fetch_column.
//          - This works if field lengths are filled. In the CalcPredictions
//            program, the first two field lengths (floats) are not filled by
//            mysql, but if filled in code here, it does get the data properly.
//            Changing database to DECIMAL instead of FLOAT returns the length.
//
//      1 = Get the lengths based on the meta data using mysql_stmt_result_metadata.
//
// STORE_RESULTS
//      0 = Fetches results as needed.
//          - This fails with a "commands out of sync" error if a normal statement
//            is run during the prepared statement.
//      1 = Calls mysql_stmt_store_result to get all results to client.
//          - This works so that a prepared statement can be called, then normal
//            statements can be run during the prepared statement.
//
// BIND_PREPARED_PARAMETERS
//      0 = This uses simple string combining to build a query string. THIS
//          DOES NOT ESCAPE STRINGS YET.
//          - This works with all tested programs so far.
//
//      1 = This uses mysql_stmt_bind_param() to bind parameters to a query.
//          - This fails with CalcPredictions in release mode. It works in debug mode.
//          - This works with CalcIntegrals in all modes. (Only a single parameter
//            for prepared statement?)
#define GET_LENGTHS 0
#define STORE_RESULTS 1
#define BIND_PREPARED_PARAMETERS 1

MYSQL_RES *DbStatement::mResult;
MYSQL_RES *DbStatement::mPreparedResult;

DbResult DbAccess::open(char const *dbName)
    {
    DbResult result;
    mConnection = mysql_init(NULL);
    if(mConnection)
        {
        char const *user = "user";
        char const *pw = "pw";
        // dbName is host/dbname
        std::vector<std::string> dbId;
        StringSplit(dbName, dbId, "/");
        if(mysql_real_connect(mConnection, dbId[0].c_str(), user, pw,
            dbId[1].c_str(), 0, NULL, 0) == NULL)
            {
            std::string errStr = "Unable to open database connection ";
			errStr += dbName;
            errStr += ". ";
            errStr += mysql_error(mConnection);
            result.setError(errStr);
            }
        }
    else
        {
        result.setError("Unable to initialize MySQL connection");
        }
    return result;
    }

void DbAccess::close()
    {
    if(mConnection)
        {
        DEBUG_PRINT("mysql_close\n");
        mysql_close(mConnection);
        mConnection = nullptr;
        }
    }

void DbStatement::close()
    {
    if(mResult)
        {
        DEBUG_PRINT("mysql_free_result\n");
        mysql_free_result(mResult);
        mResult = nullptr;
        }
    if(mPreparedResult)
        {
        // According to docs for mysql_stmt_result_metadata(),
        // it says to free using mysql_free_result, and not mysql_stmt_free_result.
        DEBUG_PRINT("mysql_free_result\n");
        mysql_free_result(mPreparedResult);
        mPreparedResult = nullptr;
        }
    if(mPreparedStmt)
        {
        DEBUG_PRINT("mysql_stmt_close\n");
        mysql_stmt_close(mPreparedStmt);
        mPreparedStmt = nullptr;
        }
    }

DbResult DbStatement::bindValues(std::vector<std::string> const &values)
    {
    DbResult result;
    for(size_t i=0; i<values.size() && result.isOk(); i++)
        {
        bindText(static_cast<int>(i+1), values[i].c_str());
        }
    return result;
    }


/// @todo - the current code does not use this.
/// See BIND_PREPARED_PARAMETERS and the fact that it replaces all parameters so
/// that paramCount below is zero.
DbResult DbStatement::bindPreparedParameters(MYSQL *conn, MYSQL_STMT *stmt,
    std::vector<std::string> const &bindValues)
    {
    DbResult result;
    unsigned long paramCount = 0;
    if(stmt)
        {
        paramCount = mysql_stmt_param_count(stmt);
        }
    if(paramCount > 0)
        {
        if(paramCount == bindValues.size())
            {
            my_bool setNull = true;
            my_bool clearNull = false;

            mPreparedBindArray.resize(bindValues.size());
            // Looks like isnull, len is not needed.
            // BUT- must set buffer_length to num chars then.
            mPreparedBindValues = bindValues;
            mPreparedBindLengths.resize(bindValues.size());
            mPreparedBindIsNull.resize(bindValues.size());
            mPreparedBindError.resize(bindValues.size());

            for(size_t i=0; i<mPreparedBindArray.size(); i++)
                 {
                 mPreparedBindIsNull[i] = false;
            //     mPreparedBindValues[i].resize(bindValues[i].length() * 2);
            //     mysql_real_escape_string(conn, const_cast<char*>(mPreparedBindValues[i].data()),
            //         bindValues[i].c_str(), bindValues[i].length());
                 }
            for(size_t i=0; i<mPreparedBindArray.size(); i++)
                {
                mPreparedBindArray[i].length=0;
                if(bindValues[i] == NULL_SQL_INDICATOR)
                    {
                    mPreparedBindArray[i].is_null=&setNull;
                    }
                else
                    {
                    mPreparedBindArray[i].is_null=&mPreparedBindIsNull[i];
                    mPreparedBindArray[i].error = &mPreparedBindError[i];
                    mPreparedBindArray[i].buffer_type=MYSQL_TYPE_STRING;
                    mPreparedBindArray[i].buffer=const_cast<void *>(static_cast<void const *>(
                        mPreparedBindValues[i].data()));
                    mPreparedBindArray[i].buffer_length=static_cast<unsigned long>(
                        mPreparedBindValues[i].length()+1);

                    mPreparedBindLengths[i] = mPreparedBindValues[i].length();
                    mPreparedBindArray[i].length = &mPreparedBindLengths[i];
                    }
                }
            DEBUG_PRINT("mysql_stmt_bind_param\n");
            if(mysql_stmt_bind_param(stmt, mPreparedBindArray.data()) != NULL)
                {
                std::string errStr = "Unable to bind values. ";
                errStr += mysql_error(conn);
                result.setError(errStr.c_str());
                }
            for(int i=0; i<mPreparedBindError.size(); i++)
                {
                if(mPreparedBindError[i])
                    {
                    result.setError("Bind error");
                    }
                }
            }
        else
            {
            std::string errStr = "Incorrect bind count. Query expects ";
            errStr += std::to_string(paramCount);
            errStr += " but the number of values is ";
            errStr += std::to_string(bindValues.size());
            errStr += ".\n";
            result.setError(errStr.c_str());
            }
        }
    return result;
    }

static size_t getNumParameters(std::string const &stmt)
    {
    return std::count(stmt.begin(), stmt.end(), ':');
    }

DbResult DbStatement::set(char const *query)
    {
    DbResult result;
    close();
    mMultiRowParams = getNumParameters(query);
    mDbDataState = DBS_Init;
    mQueryString = query;
    return result;
    }

DbResult DbStatement::reset()
    {
    DbResult result;
    if(mDbDataState >= DBS_Execute)
        {
        mDbDataState = DBS_BindParams;
        }
    // If this is an insert statement, there are no results.
    if(mResult)
        {
        mysql_data_seek(mResult, 0);
        }
    return result;
    }

DbResult DbStatement::getNormalResults(std::vector<std::string> &bindResults)
    {
    DbResult result;

    MYSQL *conn = mDb.getConnection();
    if(mDbDataState == DBS_Get)
        {
        DEBUG_PRINT("mysql_use_result\n");
        mResult = mysql_use_result(conn);
        if(mResult)
            {
            mDbDataState = DBS_Extract;
            }
        else
            {
            mDbDataState = DBS_Init;
            }
        }

    if(mDbDataState == DBS_Extract)
        {
        DEBUG_PRINT("mysql_fetch_row\n");
        if(mResult != nullptr)
            {
            int numFields =  mysql_num_fields(mResult);
            MYSQL_ROW row = mysql_fetch_row(mResult);
            if(row != NULL)
                {
                bindResults.resize(numFields);
                for(size_t i=0; i<numFields; i++)
                    {
                    MYSQL_FIELD *field = mysql_fetch_field_direct(mResult,
                        static_cast<unsigned int>(i));
                    if(field->type == MYSQL_TYPE_BLOB)
                        {
                        unsigned long *lengths = mysql_fetch_lengths(mResult);
                        unsigned long len = lengths[i];
                        bindResults[i].resize(len);
                        memcpy(const_cast<void*>(static_cast<const void*>(
                            bindResults[i].data())), row[i], len);
                        }
                    else
                        {
                        if(row[i] == NULL)
                            {
                            bindResults[i] = "";
                            }
                        else
                            {
                            bindResults[i] = row[i];
                            }
                        }
                    }
                }
            else
                {
                bindResults.resize(0);
                mDbDataState = DBS_Init;
                }
            }
        else
            {
            bindResults.resize(0);
            }
        }
    else
        {
        std::string errStr = "Unable to get results. ";
        errStr += mysql_error(conn);
        result.setError(errStr.c_str());
        }
    return result;
    }


DbResult DbStatement::getPreparedResults(std::vector<std::string> &bindResults)
    {
    DbResult result;

    MYSQL *conn = mDb.getConnection();
    if(mDbDataState == DBS_Get)
        {
        unsigned int fieldCount = mysql_stmt_field_count(mPreparedStmt);
        mPreparedBindResultArray.resize(fieldCount);
        bindResults.resize(fieldCount);
        mPreparedResultLengths.resize(fieldCount);
        mPreparedResultIsNull.resize(fieldCount);
        if(fieldCount > 0)
            {
#if(GET_LENGTHS == 0)
#if(STORE_RESULTS)
            // store results gets all results, so should only be done once per execute.
            DEBUG_PRINT("mysql_stmt_store\n");
            int retVal = mysql_stmt_store_result(mPreparedStmt);
            if(retVal != 0)
                {
                std::string errStr = "Unable to store prepared results. ";
                int x = mysql_errno(conn);
                errStr += mysql_stmt_error(mPreparedStmt);
                result.setError(errStr.c_str());
                }
#endif
             mDbDataState = DBS_Extract;
#else
            mPreparedResult = mysql_stmt_result_metadata(mPreparedStmt);
            MYSQL_FIELD *fields = mysql_fetch_fields(mPreparedResult);
            mPreparedResultStrings.resize(fieldCount);
            for(size_t i=0; i<mPreparedBindResultArray.size(); i++)
                {
                int len = fields[i].length + 1; // Add one for end null byte.
                mPreparedBindResultArray[i].length = &mPreparedResultLengths[i];
                mPreparedResultStrings[i].resize(len);
                mPreparedBindResultArray[i].buffer = const_cast<char*>(mPreparedResultStrings[i].data());
                mPreparedBindResultArray[i].buffer_length = len;
                mPreparedBindResultArray[i].buffer_type = MYSQL_TYPE_STRING;
                }
            // Now set the bindings.
            DEBUG_PRINT("mysql_stmt_bind_result\n");
            if(mysql_stmt_bind_result(mPreparedStmt, mPreparedBindResultArray.data()) == 0)
                {
#if(STORE_RESULTS)
                // IF THIS IS NOT DONE, then the normal statement cannot be run
                // while a prepared statement is running.
                // This is used to get all results at once to the client.
                // This takes more memory than needed?
                DEBUG_PRINT("mysql_stmt_store_result\n");
                // Move the results to the client. Length results are still not available.
                if(mysql_stmt_store_result(mPreparedStmt) == 0)
                    {
#endif
                    mDbState = DBS_Extract;
#if(STORE_RESULTS)

                    }
                else
                    {
                    std::string errStr = "Unable to get result local. ";
                    errStr += mysql_stmt_error(mPreparedStmt);
                    result.setError(errStr.c_str());
                    }
#endif
                }
            else
                {
                std::string errStr = "Unable to bind results. ";
                errStr += mysql_stmt_error(mPreparedStmt);
                result.setError(errStr.c_str());
                }
#endif
            }
        else
            {
            mDbDataState = DBS_Init;
            }
        }

    if(mDbDataState == DBS_Extract && result.isOk())
        {
#if(GET_LENGTHS == 0)
            // First get the result lengths.
            // If this is used, I think that this and mysql_stmt_bind_result
            // should be called before every fetch so that the lengths are
            // reset every row. The disadvantage of this method is that the
            // memory has to be resized (std::string) every row.
            for(size_t i=0; i<mPreparedBindResultArray.size(); i++)
                {
                mPreparedBindResultArray[i].buffer = nullptr;
                mPreparedBindResultArray[i].buffer_length = 0;
                mPreparedBindResultArray[i].is_null = &mPreparedResultIsNull[i];
                mPreparedBindResultArray[i].length = &mPreparedResultLengths[i];
                }
        int retValx = mysql_stmt_bind_result(mPreparedStmt, mPreparedBindResultArray.data());
        if(retValx != 0)
            {
            std::string errStr = "Unable to bind results. ";
            errStr += mysql_stmt_error(mPreparedStmt);
            result.setError(errStr.c_str());
            }
#endif
        int retVal = 0;
        if(retVal == 0)
            {
            DEBUG_PRINT("mysql_stmt_fetch\n");
            // This will fill mPreparedResultLengths.
            retVal = mysql_stmt_fetch(mPreparedStmt);
            }
#if(GET_LENGTHS == 0)
        const char *state = mysql_sqlstate(conn);
        const char *stmt_state = mysql_stmt_sqlstate(mPreparedStmt);
        char const *err = mysql_stmt_error(mPreparedStmt);

        // MYSQL_DATA_TRUNCATED is returned when first two fields are floats.
        if(retVal == MYSQL_DATA_TRUNCATED || retVal == 0)
            {
            for(size_t i=0; i<mPreparedResultLengths.size() && result.isOk(); i++)
                {
                // Zero is returned for floats or null.
                if(*mPreparedBindResultArray[i].is_null)
                    {
                    mPreparedResultLengths[i] = 0;
                    }
                else if(mPreparedResultLengths[i] == 0)
                    {
                    // A better way to do this may be to use mysql_stmt_result_metadata/mysql_fetch_fields.
                    mPreparedResultLengths[i] = 12;
                    }
                bindResults[i].resize(mPreparedResultLengths[i]);
                mPreparedBindResultArray[i].buffer = const_cast<char*>(bindResults[i].data());
                mPreparedBindResultArray[i].buffer_length = mPreparedResultLengths[i];
                mPreparedBindResultArray[i].length = &mPreparedResultLengths[i];
                if(mysql_stmt_fetch_column(mPreparedStmt, &mPreparedBindResultArray[i],
                    static_cast<unsigned int>(i), 0) != 0)
                    {
                    std::string errStr = "Unable to get column data. ";
                    errStr += mysql_stmt_error(mPreparedStmt);
                    result.setError(errStr.c_str());
                    }
                }
            }
        else if(retVal == MYSQL_NO_DATA)
            {
            bindResults.resize(0);
            mDbDataState = DBS_Init;
            }
#else
        if(retVal == MYSQL_DATA_TRUNCATED || retVal == 0)
            {
            for(size_t i=0; i<mPreparedResultLengths.size(); i++)
                {
                bindResults[i] = mPreparedResultStrings[i];
                }
            }
        else if(retVal == MYSQL_NO_DATA)
            {
            closePreparedResult();
            closePreparedStatement();
            bindResults.resize(0);
            mDbState = DBS_Init;
            }
#endif
        }
    return result;
    }

// @param bindResults The returned results. Size is zero if there are no
//      more rows.
DbResult DbStatement::getResults(std::vector<std::string> &bindResults)
    {
    DbResult result;
    if(mUsePreparedStatement)
        {
        result = getPreparedResults(bindResults);
        }
    else
        {
        result = getNormalResults(bindResults);
        }
    return result;
    }

static std::string normalizeMySqlBindParameterStatement(std::string const &stmt)
    {
    std::string mysqlStmt = stmt;
    for(size_t startI=0; startI<mysqlStmt.length(); startI++)
        {
        if(mysqlStmt[startI] == ':')
            {
            size_t endI=startI+1;
            for(; endI<mysqlStmt.length(); endI++)
                {
                if(!isalpha(mysqlStmt[endI]))
                    break;
                }
            mysqlStmt.replace(startI, endI-startI, "?");
            }
        else if(mysqlStmt[startI] == '%')
            {
            mysqlStmt.replace(startI, 2, "?");  // Convert %s
            }
        }
    return mysqlStmt;
    }

static std::string normalizeMySqlBindParameterMultiRowStatement(std::string const &stmt,
    int numRows, int paramsPerRow)
    {
    std::string mysqlStmt = stmt;
    size_t pos = mysqlStmt.find(':');
    if(pos != std::string::npos)
        {
        mysqlStmt.resize(pos-1);
        for(int rowI=0; rowI<numRows; rowI++)
            {
            mysqlStmt.append("(");
            for(int i=0; i<paramsPerRow; i++)
                {
                mysqlStmt.append("?");
                if(i != paramsPerRow-1)
                    { mysqlStmt.append(","); }
                }
            mysqlStmt.append(")");
            if(rowI != numRows-1)
                { mysqlStmt.append(","); }
            }
        mysqlStmt.append(";");
        }
    return mysqlStmt;
    }

static std::string getMySqlStatement(std::string const &stmt,
    std::vector<std::string> const &bindValues)
    {
    std::string mysqlStmt = stmt;

    // This groups multiple bind value rows for one statement and is much
    // faster than inserting a single row.
    // It puts the burden of grouping on the client. Instead we will try
    // prepared statements to see if that works.
    /*
    size_t numParameters = getNumParameters(stmt);
    int numGroups = bindValues.size() / numParameters;

    /// @todo - This does not bind parameters with ":tag" column binding values.
    /// Is uses order. This was done as a test. It can be 100 times faster than
    /// inserting values one row at a time.
    if(numGroups > 1)
        {
        size_t startArgs = stmt.find('?');
        if(startArgs != std::string::npos)
            {
            mysqlStmt.resize(startArgs-1);
            for(int groupI=0; groupI<numGroups; groupI++)
                {
                mysqlStmt.append("(");
                for(int paramI=0; paramI<numParameters; paramI++)
                    {
                    mysqlStmt.append(bindValues[groupI*numParameters+paramI]);
                    if(paramI != numParameters-1)
                        {
                        mysqlStmt.append(",");
                        }
                    }
                mysqlStmt.append(")");
                if(groupI == numGroups-1)
                    {
                    mysqlStmt.append(";");
                    }
                else
                    {
                    mysqlStmt.append(",");
                    }
                }
            }
        }
    else
*/
        {
        int bindIndex = 0;
        for(size_t startI=0; startI<mysqlStmt.length(); startI++)
            {
            if(mysqlStmt[startI] == '?')
                {
                std::string valStr = "";
                bool allDigits = true;
                for(int i=0; i<bindValues[bindIndex].length(); i++)
                    {
                    if(!isdigit(bindValues[bindIndex][i]))
                        {
                        allDigits = false;
                        }
                    }
                if(allDigits)
                    {
                    valStr = bindValues[bindIndex];
                    }
                else
                    {
                    valStr = '\'' + bindValues[bindIndex] + '\'';
                    }
                mysqlStmt.replace(startI, 1, valStr);
                bindIndex++;
                }
            }
        }
    return mysqlStmt;
    }

void DbStatement::startMultiRowInsert(int numRows)
    {
    mMultiRowSize = numRows;
    }

DbResult DbStatement::endMultiRowInsert()
    {
    mMultiRowSize = 0;
    // Force an execute. Bind param count will be zero if no params were bound.
    mMultiRowIndex = 1;
    DbResult result =  execute();
    mMultiRowIndex = 0;
    return result;
    }

DbResult DbStatement::execute()
    {
    DbResult result;

    if(isMultiRow())
        {
        mMultiRowIndex++;
        }
    MYSQL *conn = mDb.getConnection();
//    printf("%d %d %d\n", mDbDataState, mMultiRowIndex, mMultiRowSize);
    if(mDbDataState == DBS_Init && mMultiRowIndex >= mMultiRowSize)
        {
        /// @todo - if multirow, then clear params and generate ? from mBindValues.size()
        std::string query;
        if(isMultiRow())
            {
            int numRows = mBindValues.size() / mMultiRowParams;
            query = normalizeMySqlBindParameterMultiRowStatement(mQueryString,
                numRows, mMultiRowParams);
            }
        else
            {
            query = normalizeMySqlBindParameterStatement(mQueryString);
            }
        if(mUsePreparedStatement)
            {
            if(!mPreparedStmt)
                {
                mPreparedStmt = mysql_stmt_init(conn);
                }
            DEBUG_PRINT(query.c_str());
            DEBUG_PRINT("mysql_stmt_prepare\n");
    #if(BIND_PREPARED_PARAMETERS == 0)
            std::string noParamQuery = getMySqlStatement(query.c_str(), mBindValues);
            query = noParamQuery;
            mBindValues.resize(0);
    #endif
            if(mysql_stmt_prepare(mPreparedStmt, query.c_str(),
                static_cast<unsigned long>(query.length())) == NULL)
                {
                mDbDataState = DBS_BindParams;
                }
            else
                {
                std::string errStr = "Unable to prepare SQL statement. ";
                errStr += mysql_stmt_error(mPreparedStmt);
                result.setError(errStr.c_str());
                }
            }
        else
            {
            std::string noParamQuery = getMySqlStatement(query.c_str(), mBindValues);
            DEBUG_PRINT(noParamQuery.c_str());
            DEBUG_PRINT("mysql_query\n");
            if(mysql_query(conn, noParamQuery.c_str()) == NULL)
                {
                mDbDataState = DBS_Get;
                }
            else
                {
                std::string errStr = "Unable to query. ";
                errStr += mysql_error(conn);
                result.setError(errStr.c_str());
                }
            }
        }
    if(result.isOk() && (mDbDataState == DBS_BindParams))
        {
        // This binds a whole set of bind values, therefore it should only be
        // done once for one prepare and execute.
        result = bindPreparedParameters(conn, mPreparedStmt, mBindValues);
        if(result.isOk())
            {
            mDbDataState = DBS_Execute;
            }
        }
    if(result.isOk() && mDbDataState == DBS_Execute && mMultiRowIndex >= mMultiRowSize)
        {
        DEBUG_PRINT("mysql_stmt_execute\n");
        if(mysql_stmt_execute(mPreparedStmt) == NULL)
            {
            mDbDataState = DBS_Get;
            }
        else
            {
            std::string errStr = "Unable to execute. ";
            errStr += mysql_stmt_error(mPreparedStmt);
            result.setError(errStr.c_str());
            }
        mMultiRowIndex = 0;
        }
    return result;
    }

DbResult DbStatement::testRow(bool &gotRow)
    {
    DbResult result = execute();
    if(result.isOk())
        {
        result = getResults(mBindResults);
        gotRow = (mBindResults.size() > 0);
        }
    return result;
    }

DbResult DbStatement::getRow()
    {
    DbResult result = execute();
    if(result.isOk())
        {
        result = getResults(mBindResults);
        if(result.isOk())
            {
            if(mBindResults.size() == 0)
                {
                result.setError("Unable to get row");
                }
            }
        }
    return result;
    }

void DbStatement::setBindValue(int ordinal, char const *str)
    {
    size_t index = (mMultiRowIndex * mMultiRowParams) + ordinal -1;
    if(mBindValues.size() <= index)
        {
        mBindValues.resize(index + 1);
        }
    mBindValues[index] = str;
    }

void DbStatement::setBindValue(char const *param, char const *str)
    {
    size_t paramEndI = mQueryString.find(param);
    int ordinal = static_cast<int>(std::count(mQueryString.begin(),
        mQueryString.begin()+paramEndI, ':')) + 1;
    setBindValue(ordinal, str);
    }

/*
std::vector<std::string> DbStatement::getValuesFromDictString(std::string const &str)
    {
    size_t pos = 0;
    std::vector<std::string> values;
    while(pos != std::string::npos)
        {
        size_t endTuplePos = str.find(')', pos);
        if(endTuplePos == std::string::npos)
            {
            endTuplePos = str.find(']', pos);
            }
        if(endTuplePos != std::string::npos)
            {
            size_t endValPos = endTuplePos-2;
            size_t startValQuotePos = str.rfind('\'', endValPos);
            if(startValQuotePos == std::string::npos)
                startValQuotePos = str.rfind('\"', endValPos);
            // Parsing json with ]], will end up with a zero length string that
            // must not be added.
            if(startValQuotePos != std::string::npos && endValPos > startValQuotePos)
                {
                values.push_back(str.substr(startValQuotePos+1,
                    endValPos-startValQuotePos));
                }
            pos = endTuplePos+1;
            }
        else
            {
            pos = endTuplePos;
            }
        }
    return values;
    }
*/

DbResult DbStatement::getLastInsertedRowIndex(int64_t &lastInsertedRowIndex)
    {
    DbResult result;
    lastInsertedRowIndex = mysql_insert_id(mDb.getConnection());
    return result;
    }

#endif

