//
// Created by Luoxs on 2017-03-16
//
#pragma once
 
#define SQL_ENABLE_SQLITE
#define SQL_ENABLE_MYSQL 
#define SQL_ENABLE_PGSQL 

// #define SQL_ENABLE_DEBUG


#include <string>
#include <vector>
#include <memory>
#include <functional>

#if defined(SQL_ENABLE_DEBUG)
#define SQLDEBUG(info, ...) printf("[%s]"info, __func__, ##__VA_ARGS__)
#else
#define SQLDEBUG(info, ...)
#endif

namespace sql
{
    typedef std::function<void(int, char **)> execute_handler;

    //struct blob { void *data; size_t size; };
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


