/*
* DbString.cpp
*
*  Created on: Nov 2, 2016
*  \copyright 2016 DCBlaha.  Distributed under the GPL.
*/

#include "DbString.h"
#include <ctype.h>


// Finds comma delimited arguments. Spaces are skipped.
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
            endArgPos++;        // Skip comma
            retArgLen = endArgPos - startArgPos;
            }
        else
            {
            retArgLen = args.length()-startArgPos;
            }
        if(retArgLen == 0)
            {
            startArgPos = String::npos;
            }
        else
            {
            startArgPos += retArgLen;
            }
        }
    return retArgLen;
    }

// Appends comma delimited arguments to a string.
static void appendArg(String &targetString, StringRef args, size_t &startArgPos)
    {
    int len = findNextArg(args, startArgPos);
    if(startArgPos != String::npos)
        {
        targetString.append(&args[startArgPos], len);
        targetString.append(",");
        }
    }

static inline void appendColumnName(String &targetString, StringRef columns, size_t &startColumnPos)
    {
    appendArg(targetString, columns, startColumnPos);
    }

static inline void appendValue(String &targetString, StringRef values, size_t startValuePos)
    {
    targetString.append("\"");
    appendArg(targetString, values, startValuePos);
    targetString.append("\"");
    }

static void appendBoundValues(String &targetString, size_t numBoundValues)
    {
    for(int i=0; i<numBoundValues; i++)
        {
        if(i != 0)
            { targetString.append(","); }
        targetString.append("?");
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

// Appends values with no outer parenthesis
static void appendValues(String &targetString, StringRef values)
    {
    size_t startArgPos = 0;
//    if(values.length() != 0)
        {
        // parse string and separate into values.
        while(startArgPos != String::npos)
            {
            appendValue(targetString,  values, startArgPos);
            }
        }
// Should use bind null
//    else
//        {
//        append("NULL");
//        }
    }

static void appendNamesAndValues(String &targetString, StringRef columnNames,
    StringRef values)
    {
    size_t startColPos = 0;
    size_t startValuePos = 0;
    while(startColPos != String::npos && startValuePos != String::npos)
        {
        appendColumnName(targetString, columnNames, startColPos);
        targetString.append("=");
        appendValue(targetString, values, startValuePos);
        if(startColPos != String::npos && startValuePos != String::npos)
            {
            targetString.append(",");
            }
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

DbString &DbString::SET(StringRef columns, size_t numBoundValues)
    {
    append(" SET ");
//    appendNamesAndValues(*this, columns, numBoundValues);
    return *this;
    }

DbString &DbString::WHERE(StringRef columnName, StringRef operStr, size_t numBoundValues)
    {
    append(" WHERE ");
    append(columnName);
    append(operStr);
    append("(");
    appendBoundValues(*this, numBoundValues);
    append(")");
    return *this;
    }

DbString &DbString::AND(StringRef columnName, StringRef operStr, size_t numBoundValues)
    {
    append(" AND ");
    append(columnName);
    append(operStr);
    append("(");
    appendBoundValues(*this, numBoundValues);
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

DbString &DbString::VALUES(StringRef columnValues)
    {
    append("VALUES (");
    appendValues(*this, columnValues);
    append(")");
    return *this;
    }

DbString &DbString::VALUES(size_t numBoundValues)
    {
    append("VALUES (");
    appendBoundValues(*this, numBoundValues);
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

