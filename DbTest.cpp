#include "DbAccess.h"
#include "DbString.h"

int main()
    {
    DbAccess db;
    // This creates a database file in the current directory if it does not exist.
    // SQLite3 must be installed.
    printf("Open a database\n");
    DbResult result = db.open("DbTest.db");
    if(result.isOk())
        {
        printf("Create a table\n");
        DbString command;
        command.CREATE_TABLE("Person").COLUMN_DEFS("id INTEGER PRIMARY KEY, name TEXT NOT NULL");

        DbStatement statement(db, command.getDbStr().c_str());
        result = statement.execute();
        }
    if(result.isOk())
        {
        printf("Insert a row\n");
        DbStatement statement(db);
        DbString insertStr;
        insertStr.INSERT_INTO("Person").COLUMNS("name").VALUES(":name");
        result = statement.set(insertStr.getDbStr().c_str());
        if(result.isOk())
            {
            statement.bindText(":name", "Fred");
            result = statement.execute();
            }
        }
    if(result.isOk())
        {
        printf("Iterate through all Person rows\n");
        DbStatement statement(db);
        DbString selectStr;
        selectStr.SELECT("id, name").FROM("Person");
        result = statement.set(selectStr.getDbStr().c_str());
        bool gotRow = true;
        while(result.isOk() && gotRow)
            {
            result = statement.testRow(gotRow);
            if(result.isOk() && gotRow)
                {
                int id = statement.getColumnInt(0);
                std::string name = statement.getColumnText(1);
                printf("  %d %s\n", id, name.c_str());
                }
            }
        }
    return 0;
    }

