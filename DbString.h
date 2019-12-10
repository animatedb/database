/*
* DbString.h
*
*  Created on: Nov 2, 2016
*  \copyright 2016 DCBlaha.  Distributed under the Mozilla Public License 2.0.
*/
// Allows defining SQL query strings. The main advantage for using this module
// is that concatenation of strings and dynamic variables is specific to SQL.
// See the examples in the class DbString.

#ifndef DB_STRING_H
#define DB_STRING_H
#include "DbAccess.h"       // For DATABASE

#include <string>
#include <vector>
#include <algorithm>
#include "DbResult.h"

typedef std::string const &StringRef;
typedef std::string String;

enum DbValueParamCounts { ONE_PARAM=1, TWO_PARAMS, THREE_PARAMS, FOUR_PARAMS};

void printQuery(char const *str);
void printQueryArgs(char const *str);

/// This supports non-bind parameters, but generally should be used for
/// bind parameters.  This can be used two ways for bind parameters.
/// 1. For example, TWO_PARAMS will insert "?,?". Bind functions use base 1 ordinals.
/// 2. For example, ":id, :name". Bind functions can then use these names.
class DbValues:public String
    {
    public:
        /// This adds the specified number of question marks. Each question
        /// mark corresponds to a bind parameter.
        DbValues(DbValueParamCounts numBindParams);

        /// This is for bind parameters.
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

class DbColumnDefinitions:public String
    {
    public:
        DbColumnDefinitions(StringRef params):
            String(params)
            {}

        DbColumnDefinitions(char const *params):
            String(params)
            {}
    };

class DbJoinString:public String
    {
    public:
        /// @todo - fix - should not be duplicate with DbString
        // Appends " JOIN "
        DbJoinString &JOIN(StringRef table)
            { append(" JOIN "); append(table); return *this; }
        /// @todo - fix - should not be duplicate with DbString
        // Appends " ON "
        DbJoinString &ON(StringRef condition)
            { append(" ON "); append(condition); return *this; }
    };

// This uses method chaining or the named parameter idiom. These are meant to be used
// with bind parameters.
//
// Examples:
//    dbStr.SELECT("catId").FROM("Cat").WHERE("catName", "=", ONE_PARAM).AND("catId", "=", ONE_PARAM);
//    dbStr.SELECT("catId").FROM("Cat").WHERE("catName", "=", ONE_PARAM).AND("catId", "=", ":name");
//
//    dbStr.INSERT_INTO("Cat").COLUMNS("catId, catName").VALUES(TWO_PARAMS);
//    dbStr.INSERT_INTO("Cat").COLUMNS("catId, catName").VALUES(":id, :name");
//
//    dbStr.UPDATE("Cat").SET("catId, catName", TWO_PARAMS);
//    dbStr.DELETE_FROM("Cat");
class DbString:private String
    {
    public:
        DbString()
            {}
        DbString(StringRef str):
            String(str)
            {}
        // Appends "CREATE table"
        DbString &CREATE_TABLE(StringRef table)
            {
            startStatement("CREATE TABLE ", table);
            return *this;
            }

        // Appends "SELECT column"
        DbString &SELECT(StringRef column)
            {
            startStatement("SELECT ", column);
            return *this;
            }

        /// This is not an UPSERT for sqlite, but will replace all values.
        /// INSERT_OR_REPLACE_INTO
        /// http://stackoverflow.com/questions/3634984/insert-if-not-exists-else-update
        /// Appends "INSERT INTO table"
        DbString &INSERT_INTO(StringRef table)
            {
            startStatement("INSERT INTO ", table);
            return *this;
            }

        /// This is not an UPSERT for sqlite, but will replace all values
        /// including the primary id.
        /// http://stackoverflow.com/questions/3634984/insert-if-not-exists-else-update
        /// Appends "INSERT OR REPLACE INTO table"
        /// This might work with CREATE TABLE UNIQUE ON CONFLICT IGNORE, but if this is not
        /// used, if anything is referencing a record, the record will be replaced with a
        /// new primary ID (unless supplied), and the depdendent data will not refer to
        /// the newly inserted record.
        ///
        /// Using CREATE TABLE UNIQUE ON CONFLICT IGNORE is faster than other solutions since
        /// it does not write to the database if the record already exists.
        /// https://stackoverflow.com/questions/15523442/what-is-the-reason-for-sqlite-upsert-performance-improvment-with-unique-on-confl
        DbString &INSERT_OR_REPLACE_INTO(StringRef table)
            {
#if(DATABASE == DB_SQLITE)
            startStatement("INSERT OR REPLACE INTO ", table);
#else
            startStatement("REPLACE ", table);
#endif
            return *this;
            }

        /// This is only for SQLITE since it doesn't have upsert.
        /// I am not sure if this will ignore other failures, so this might not
        /// be the best solution.
        /// This is probably faster than other solutions since it does not write to
        /// the database if the record already exists.
        /// https://sqlite.org/lang_conflict.html
        DbString &INSERT_OR_IGNORE(StringRef table)
            {
#if(DATABASE == DB_SQLITE)
            startStatement("INSERT OR IGNORE INTO ", table);
#else
            startStatement("INSERT IGNORE INTO ", table);
#endif
            return *this;
            }

        /// Appends "UPDATE table"
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

        /// Appends "DELETE FROM "
        DbString &DELETE_FROM(StringRef table)
            {
            startStatement("DELETE FROM ", table);
            return *this;
            }

        // This only drops if exists. This is because if the software crashes after a
        // drop, the index may not exist the next time the drop is attempted.
        DbString &DROP_INDEX(StringRef index)
            {
            std::string stmt = "DROP INDEX IF EXISTS " + index;
            append(stmt);
            return *this;
            }

        // This only creates if it doesn't exist. This is because if the software
        // crashes after a create, the index may exist the next time the create is attempted.
        DbString &CREATE_INDEX(StringRef index, StringRef table, StringRef column)
            {
            std::string stmt = "CREATE INDEX IF NOT EXISTS " + index + " ON " + table + "(" + column + ")";
            append(stmt);
            return *this;
            }

        /// Appends " FROM "
        DbString &FROM(StringRef table);
        /// Appends " ORDER BY "
        DbString &ORDER_BY(StringRef table, bool ascend=true);
        /// Appends " JOIN "
        DbString &JOIN(StringRef table);
        /// Appends " ON "
        DbString &ON(StringRef condition);
        /// Appends "(columnName1, columnName2)"
        DbString &COLUMNS(StringRef columnNames);
        /// Appends "VALUES (columnValue1, columnValue2)"
        DbString &VALUES(DbValues const &values);
        /// Appends "(columnDef1, columnDef2);"
        DbString &COLUMN_DEFS(DbColumnDefinitions const &defs);

        /// Appends " SET column1, column2 = value1, value2"
        /// The number of columns and values must match.
        DbString &SET(StringRef columnNames, DbValues const &values);

        /// Appends " WHERE columnName operStr colVal"
        DbString &WHERE(StringRef columnName, StringRef operStr,
            DbValues const &values);
        /// Appends " AND columnName operStr colVal"
        DbString &AND(StringRef columnName, StringRef operStr,
            DbValues const &values);

        /// Special to allow adding a bunch of joins.
        DbString &addJoins(std::string const &joinStr)
            { append(joinStr.c_str()); return *this; }

        /// Appends a semicolon to the returned string.
        String getDbStr();

        /// Find the index of the field from the select query statement.
        /// Go through the query string and find the field from the select part of the query,
        /// then count back for the number of commas to get the index of the field.
        static DbResult getColumnIndex(std::string const &queryStr,
            const char *fieldName, int &index);
        static DbResult getOptionalColumnIndex(std::string const &queryStr,
            const char *fieldName, int &index);

        // This module expects that bind parameters start with a colon and have a
        // name, or use a question mark.
        // It is up to the DB interface to alter if its interface is different.
        size_t getNumBindParameters() const
            {
            return std::count(begin(), end(), ':') +
                std::count(begin(), end(), '?') + std::count(begin(), end(), '@');
            }

        void clear()
            { String::clear(); }


    private:
        // Prevent usage. Use getDbStr instead. This is undefined.
        char const *c_str();
        void startStatement(StringRef statement, StringRef arg);
    };

#endif

