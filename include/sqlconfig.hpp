//
// Created by Luoxs on 2017-03-16
//
#pragma once

#if !defined(SQL_ENABLE_SQLITE) && !defined(SQL_DISABLE_SQLITE)
#define SQL_ENABLE_SQLITE
#endif
#if !defined(SQL_ENABLE_MYSQL) && !defined(SQL_DISABLE_MYSQL)
#define SQL_ENABLE_MYSQL 
#endif
#if !defined(SQL_ENABLE_PGSQL) && !defined(SQL_DISABLE_PGSQL)
#define SQL_ENABLE_PGSQL 
#endif


#include <string>
#include <vector>
#include <memory>
#include <functional>

#if defined(SQL_ENABLE_DEBUG)
#define SQLDEBUG(info, ...) printf("[sql:%d] " info, __LINE__, ##__VA_ARGS__)
#else
#define SQLDEBUG(info, ...)
#endif

namespace sql
{
    typedef std::function<void(char **, unsigned long *)> execute_handler;

    typedef std::vector<char> blob_t;
}

#if defined(SQL_ENABLE_SQLITE)
#include "db/sqldb_sqlite.hpp"
#endif

#if defined(SQL_ENABLE_MYSQL)
#include "db/sqldb_mysql.hpp"
#endif

#if defined(SQL_ENABLE_PGSQL)
#include "db/sqldb_pgsql.hpp"
#endif


