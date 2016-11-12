/*
* DbString.h
*
*  Created on: Nov 2, 2016
*  \copyright 2016 DCBlaha.  Distributed under the Mozilla Public License 2.0.
*/
// This provides an efficient SQL string builder where the source code is easy to read.
// This only supports prepared statements at the moment.
// This does not handle all types of queries. The examples below show some of them.


#ifndef DB_STRING_H
#define DB_STRING_H

#include <string>
#include <vector>

typedef std::string const &StringRef;
typedef std::string String;

enum DbValueParamCounts { ONE_PARAM=1, TWO_PARAMS, THREE_PARAMS, FOUR_PARAMS};

/// This supports non-bound parameters, but generally should be used for
/// bound parameters.  This can be used two ways for bound parameters.
/// 1. For example, TWO_PARAMS will insert "?,?". Bind functions use base 1 ordinals.
/// 2. For example, ":id, :name". Bind functions can then use these names.
class DbValues:public String
    {
    public:
        /// This adds the specified number of question marks. Each question
        /// mark corresponds to a bound value.
        DbValues(DbValueParamCounts numBoundParams);

        /// This is for bound parameters.
        /// @params params A list of comma delimited parameters with leading colons
        ///     such as ":id, :name". Then these names can be used with bind
        ///     functions.
        DbValues(StringRef params):
            String(params)
            {}

        DbValues(char const *params):
            String(params)
            {}
    };

// This uses method chaining or the named parameter idiom. These are meant to be used
// with bound parameters.
//
// Examples:
//    dbStr.SELECT("catId").FROM("Cat").WHERE("catName", "=", ONE_PARAM).AND("catId", "=", ONE_PARAM);
//    dbStr.SELECT("catId").FROM("Cat").WHERE("catName", "=", ONE_PARAM).AND("catId", "=", ":name");
//
//    dbStr.INSERT_INTO("Cat").COLUMNS("catId, catName").VALUES(TWO_PARAMS);
//    dbStr.INSERT_INTO("Cat").COLUMNS("catId, catName").VALUES(":id, :name");
//
//    dbStr.UPDATE("Cat").SET("catId, catName", TWO_PARAMS);
//    dbStr.DELETEFROM("Cat");
class DbString:private String
    {
    public:
        DbString()
            {}
        DbString(StringRef str):
            String(str)
            {}
        // Appends "SELECT column"
        DbString &SELECT(StringRef column)
            {
            startStatement("SELECT ", column);
            return *this;
            }

        // Appends "INSERT INTO table"
        DbString &INSERT_INTO(StringRef table)
            {
            startStatement("INSERT INTO ", table);
            return *this;
            }

        // Appends "UPDATE table"
        DbString &UPDATE(StringRef table)
            {
            startStatement("UPDATE ", table);
            return *this;
            }
/*
// This doesn't work for sqlite?
        DbString &SHOWTABLES(StringRef databaseName)
            {
            startStatement("SHOW TABLES FROM ", databaseName);
            return *this;
            }
*/

        DbString &DELETEFROM(StringRef table)
            {
            startStatement("DELETE FROM ", table);
            return *this;
            }

        // Appends " FROM "
        DbString &FROM(StringRef table);
        // Appends "(columnName1, columnName2)"
        DbString &COLUMNS(StringRef columnNames);
        // Appends "VALUES (columnValue1, columnValue2)"
        DbString &VALUES(DbValues const &values);

        // Appends " SET column1, column2 = value1, value2"
        // The number of columns and values must match.
        DbString &SET(StringRef columnNames, DbValues const &values);

        // Appends " WHERE columnName operStr colVal"
        DbString &WHERE(StringRef columnName, StringRef operStr,
            DbValues const &values);
        // Appends " AND columnName operStr colVal"
        DbString &AND(StringRef columnName, StringRef operStr,
            DbValues const &values);
        // Appends a semicolon to the returned string.
        String getDbStr();

    private:
        // Prevent usage. Use getDbStr instead. This is undefined.
        char const *c_str();
        void startStatement(StringRef statement, StringRef arg);
    };

#endif
