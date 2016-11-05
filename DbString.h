/*
* DbString.h
*
*  Created on: Nov 2, 2016
*  \copyright 2016 DCBlaha.  Distributed under the GPL.
*/

#ifndef DB_STRING_H
#define DB_STRING_H

#include <string>
#include <vector>

typedef std::string const &StringRef;
typedef std::string String;

// This uses method chaining or the named parameter idiom.
// Examples:
//      dbStr.SELECT("idModule").FROM("Module").WHERE("name", "=").VALUES(2);
//      dbStr.INSERT("Module").INTO("idModule, name").VALUES(2);
//      dbStr.UPDATE("Module").SET("idModule, name").VALUES(2);
//      dbStr.DELETE("idModule").FROM("Module");
class DbString:public String
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
        DbString &INSERT(StringRef table)
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

        DbString &DELETE(StringRef table)
            {
            startStatement("DELETE FROM ", table);
            return *this;
            }

        // Appends " FROM "
        DbString &FROM(StringRef table);
        // Appends "(columnName1, columnName2)"
        DbString &INTO(StringRef columnNames);
        // Appends "VALUES (columnValue1, columnValue2)"
        DbString &VALUES(StringRef columnValues);
        DbString &VALUES(size_t numBoundValues);

        // Appends " SET column1, column2 = value1, value2"
        // The number of columns and values must match.
        DbString &SET(StringRef columnNames, size_t numBoundValues);

        // Appends " WHERE columnName operStr colVal"
        DbString &WHERE(StringRef columnName, StringRef operStr,
            size_t numBoundValues);
        // Appends " AND columnName operStr colVal"
        DbString &AND(StringRef columnName, StringRef operStr,
            size_t numBoundValues);
        // Appends a semicolon to the returned string.
        String getDbStr();

    private:
        // Prevent usage. Use getDbStr instead. This is undefined.
        char const *c_str();
        void startStatement(StringRef statement, StringRef arg);
    };

#endif

