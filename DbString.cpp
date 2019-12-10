/*
* DbString.cpp
*
*  Created on: Nov 2, 2016
*  \copyright 2016 DCBlaha.  Distributed under the Mozilla Public License 2.0.
*/
#include "DbString.h"
#include <ctype.h>

/*
void printQuery(char const *str)
    {
    std::string qs = str;
    StringReplace(qs, "JOIN", "\nJOIN");
    StringReplace(qs, "FROM", "\nFROM");
    StringReplace(qs, "WHERE", "\nWHERE");
    StringReplace(qs, " AND", "\n AND");
    StringReplace(qs, " OR", "\n OR");
    StringReplace(qs, ",", ",\n");
    printf("%s\n", qs.c_str());
    }

void printQueryArgs(char const *str)
    {
    printf("%s\n", str);
    }
*/

/// Finds comma delimited arguments. Spaces are skipped.
/// Usage is typically to continue until length is zero.
/// @param argPos This is incremented through the arguments. It is advanced to
///     after the currently returned argument.
/// @startArgPos This is the returned start position of the currently found argument.
static size_t findNextArg(StringRef args, size_t &argIter, size_t &startArgPos)
    {
    size_t retArgLen = 0;
    if(argIter != String::npos)
        {
        while(args[argIter] == ' ')
            { argIter++; }
        startArgPos = argIter;
        argIter = args.find(',', startArgPos);
        if(argIter != std::string::npos)
            {
            retArgLen = argIter - startArgPos;
            argIter++;      // Add one for comma
            }
        else
            {
            retArgLen = args.length() - startArgPos;
            }
        }
    return retArgLen;
    }

// Appends argument to a string.
// Updates startArgPos to point to next starting argument.
static void appendArg(String &targetString, StringRef args, size_t &argPos, bool insertDelimiter)
    {
    size_t initialArgPos = argPos;
    size_t startArgPos;
    size_t len = findNextArg(args, argPos, startArgPos);
    if(len > 0)
        {
        if(initialArgPos != 0 && insertDelimiter)
            {
            targetString.append(",");
            }
        targetString.append(&args[startArgPos], len);
        }
    }

DbResult DbString::getColumnIndex(std::string const &queryStr,
    const char *fieldName, int &index)
    {
    DbResult result;
    size_t pos = queryStr.find(fieldName);
    if(pos != std::string::npos)
        {
        // Count commas between fieldName and start.
        index = static_cast<int>(std::count(queryStr.begin(),
            queryStr.begin()+pos, ','));
        }
    else
        {
        std::string errStr = "Unable to find field ";
        errStr += fieldName;
        result.setError(errStr.c_str());
        }
    return result;
    }

DbResult DbString::getOptionalColumnIndex(std::string const &queryStr,
    const char *fieldName, int &index)
    {
    DbResult result;
    index = -1;
    size_t pos = queryStr.find(fieldName);
    if(pos != std::string::npos)
        {
        result = getColumnIndex(queryStr, fieldName, index);
        }
    return result;
    }

static inline void appendColumnName(String &targetString, StringRef columns, size_t &startColumnPos)
    {
    appendArg(targetString, columns, startColumnPos, true);
    }

static void appendColumnNames(String &targetString, StringRef colNames)
    {
    size_t startArgPos = 0;
    while(startArgPos != String::npos)
        {
        appendColumnName(targetString, colNames, startArgPos);
        }
    }

static inline void appendValue(String &targetString, StringRef values, size_t &startValPos, 
    bool insertDelimiter)
    {
    appendArg(targetString, values, startValPos, insertDelimiter);
    }

static void appendValues(String &targetString, DbValues const &values)
    {
    size_t startArgPos = 0;
    while(startArgPos != String::npos)
        {
        appendValue(targetString, values, startArgPos, true);
        }
    }

static void appendColumnNamesAndValues(String &targetString, StringRef columnNames,
    DbValues const &values)
    {
    size_t startColPos = 0;
    size_t startValPos = 0;
    while(startColPos != String::npos && startValPos != String::npos)
        {
        appendColumnName(targetString, columnNames, startColPos);
        if(startColPos != String::npos)
            {
            targetString.append("=");
            appendValue(targetString, values, startValPos, false);
            }
        }
    }

DbValues::DbValues(DbValueParamCounts numBindParams)
    {
    for(size_t i=0; i<numBindParams; i++)
        {
        if(i != 0)
            { append(","); }
        append("?");
        }
    }

void DbString::startStatement(StringRef statement, StringRef arg)
    {
    clear();
    append(statement);
    append(arg);
    }

DbString &DbString::FROM(StringRef table)
    {
    append(" FROM ");
    append(table);
    return *this;
    }

DbString &DbString::ORDER_BY(StringRef table, bool ascend)
    {
    append(" ORDER BY ");
    append(table);
    if(!ascend)
        {
        append(" DESC ");
        }
    return *this;
    }

DbString &DbString::JOIN(StringRef table)
    {
    append(" JOIN ");
    append(table);
    return *this;
    }

DbString &DbString::ON(StringRef condition)
    {
    append(" ON ");
    append(condition);
    return *this;
    }

DbString &DbString::SET(StringRef columns, DbValues const &values)
    {
    append(" SET ");
    appendColumnNamesAndValues(*this, columns, values);
    return *this;
    }

DbString &DbString::WHERE(StringRef columnName, StringRef operStr, DbValues const &values)
    {
    append(" WHERE ");
    append(columnName);
    append(operStr);
    append("(");
    appendValues(*this, values);
    append(")");
    return *this;
    }

DbString &DbString::AND(StringRef columnName, StringRef operStr, DbValues const &values)
    {
    append(" AND ");
    append(columnName);
    append(operStr);
    append("(");
    appendValues(*this, values);
    append(")");
    return *this;
    }

DbString &DbString::COLUMNS(StringRef columnNames)
    {
    append("(");
    append(columnNames);
    append(")");
    return *this;
    }

DbString &DbString::VALUES(DbValues const &values)
    {
    append(" VALUES (");
    appendValues(*this, values);
    append(")");
    return *this;
    }

DbString &DbString::COLUMN_DEFS(DbColumnDefinitions const &defs)
    {
    append("(");
    append(defs);
    append(")");
    return *this;
    }

String DbString::getDbStr()
    {
    size_t len = length();
    if(len > 0 && (*this)[len-1] != ';')
        {
        append(";");
        }
    return *this;
    }

