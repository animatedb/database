/*
* DbString.cpp
*
*  Created on: Nov 2, 2016
*  \copyright 2016 DCBlaha.  Distributed under the GPL.
*/

#include "DbString.h"
#include <ctype.h>


/// Finds comma delimited arguments. Spaces are skipped.
/// Usage is typically to continue until length is zero.
/// @param startArgPos Start of arg, updated to start of arg. Start of next arg
///     will be startArgPos+returned length.
static size_t findNextArg(StringRef args, size_t &startArgPos)
    {
    size_t retArgLen;
    if(startArgPos != String::npos)
        {
        while(args[startArgPos] == ' ')
            { startArgPos++; }
        size_t endArgPos = args.find(',', startArgPos);
        if(endArgPos != std::string::npos)
            {
            retArgLen = endArgPos - startArgPos;
            }
        else
            {
            retArgLen = args.length()-startArgPos;
            }
        }
    return retArgLen;
    }

// Appends argument to a string.
// Updates startArgPos to point to next starting argument.
static void appendArg(String &targetString, StringRef args, size_t &startArgPos, bool insertDelimiter)
    {
    int len = findNextArg(args, startArgPos);
    if(len > 0)
        {
        if(startArgPos != 0 && insertDelimiter)
            {
            targetString.append(",");
            }
        targetString.append(&args[startArgPos], len);
        startArgPos += len + 1;     // Skip comma
        }
    else
        {
        startArgPos = String::npos;
        }
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

DbValues::DbValues(DbValueParamCounts numBoundParams)
    {
    for(size_t i=0; i<numBoundParams; i++)
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

DbString &DbString::INTO(StringRef columnNames)
    {
    append("(");
    appendColumnNames(*this, columnNames);
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

String DbString::getDbStr()
    {
    size_t len = length();
    if(len > 0 && (*this)[len-1] != ';')
        {
        append(";");
        }
    return *this;
    }

