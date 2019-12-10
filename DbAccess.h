/*
* DbAccess.h
*
*  Created: 2017
*  \copyright 2017 DCBlaha.  Distributed under the Mozilla Public License 2.0.
*/
/// This file contains wrapper classes that hides which database is used.

#ifndef DB_ACCESS
#define DB_ACCESS

#define DB_SQLITE 1
#define DB_MYSQL 2
// Change this to use a different database
#define DATABASE DB_SQLITE

#if(DATABASE == DB_MYSQL)
#include "DbAccessMySql.h"
#elif(DATABASE == DB_SQLITE)
#include "DbAccessSqlite.h"
#endif

#endif

