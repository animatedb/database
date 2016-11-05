/*
* DbString.cpp
*
*  Created on: Nov 2, 2016
*  \copyright 2016 DCBlaha.  Distributed under the GPL.
*/

#include "DbString.h"
#include <ctype.h>


/// Finds comma delimited arguments. Spaces are skipped.
/// @param startArgPos Start of arg, updated to next start of arg.
static size_t findNextArg(StringRef args, size_t &startArgPos)
    {
    size_t retArgLen = 0;
    if(startArgPos != String::npos)
        {
        while(args[startArgPos] == ' ')
            { startArgPos++; }
        size_t endArgPos = args.find(',', startArgPos);
        if(endArgPos != std::string::npos)
            {
            retArgLen = endArgPos - startArgPos;
            startArgPos = endArgPos+1;       // Skip comma
            }
        else
            {
            retArgLen = args.length()-startArgPos+1;
            startArgPos = endArgPos;
            }
        }
    return retArgLen;
    }

// Appends comma delimited arguments to a string.
static void appendArg(String &targetString, StringRef args, size_t &argPos)
    {
    size_t startArgPos = argPos;
    int len = findNextArg(args, argPos);
    if(len > 0)
        {
        if(startArgPos != 0)
            {
            targetString.append(",");
            }
        targetString.append(&args[startArgPos], len);
        }
    }

static inline void appendColumnName(String &targetString, StringRef columns, size_t &startColumnPos)
    {
    appendArg(targetString, columns, startColumnPos);
    }

static void appendValues(String &targetString, DbValues const &values)
    {
    for(size_t i=0; i<values.size(); i++)
        {
        if(i != 0)
            { targetString.append(","); }
        targetString.append(values.getValue(i));
        }
    }

static void appendColumnNames(String &targetString, StringRef colNames)
    {
    size_t startArgPos = 0;
    while(startArgPos != String::npos)
        {
        appendColumnName(targetString, colNames, startArgPos);
        }
    }

static void appendNamesAndValues(String &targetString, StringRef columnNames,
    DbValues const &values)
    {
    size_t startColPos = 0;
    size_t valueIndex = 0;
    while(startColPos != String::npos && valueIndex<values.size())
        {
        appendColumnName(targetString, columnNames, startColPos);
        targetString.append("=");
        targetString.append(values.getValue(valueIndex++));
        }
    }

String DbValues::getValue(size_t index) const
    {
    return "?";
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
    appendNamesAndValues(*this, columns, values);
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
    append("VALUES (");
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

