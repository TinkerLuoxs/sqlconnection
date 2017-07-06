//
// Created by Luoxs on 2017-03-16
//
#pragma once

#include "sqlconfig.hpp"

namespace sql
{
    // @模板参数Database 包括 db_sqlite、db_mysql、db_pgsql
    // 可以在sqlconfig.hpp文件中以宏的形式启用或关闭某个实例(减少库依赖)
    //
    // @函数参数conninfo 有如下几种:
    // 1、当使用db_sqlite时, 表示数据库文件名称, 如 "/home/mydb.db";
    // 2、当使用db_mysql或者db_pgsql时, 表示一组 关键字=值 的字符串, 如:
    //    "host=localhost port=5432 dbname=mydb connect_timeout=10"
    //    关键字有 host、port、user、password、dbname、connect_timeout client_encoding
    template <typename Database>
    auto create_shared_connection(std::string const& conninfo)
    {
        return std::make_shared<Database>(conninfo);
    }

    template <typename Database>
    auto create_unique_connection(std::string const& conninfo)
    {
        return std::make_unique<Database>(conninfo);
    }

    // db connection的成员函数说明如下:
    // 1)函数 bool is_connected();
    // 1)说明 连接成功返回true, 否则false

    // 2)函数 bool execute(std::string const& sql_str);
    // 2)说明 执行没有返回结果的SQL语句如INSERT,UPDATE.成功返回true,否则false

    // 3)函数 bool execute(std::string const& sql_str, execute_handler&& handler);
    // 3)说明 执行有返回结果的SQL语句如SELECT.成功返回true,每一行结果调用一次handler,否则false
    // 3)    回调函数handler的前两个参数分别为字段值,字段大小

    // 4)函数 bool prepare(std::string const& pre_str);
    // 4)说明 创建一个SQL预备语句.成功返回true,否则false
    // 4)    当操作sqlite/mysql时, pre_str格式如 "INSERT INTO user(id,name) VALUES(?, ?)"
    // 4)    当操作pgsqls时, pre_str格式如 "INSERT INTO foo VALUES($1, $2, $3, $4);"

    // 5)函数 template <typename... Arg> bool execute_prepared(Arg&... arg);
    // 5)说明 执行一次已创建好的SQL预备语句.成功返回true,否则false
    // 5)    函数参数类型为c++基本类型,参数个数为prepare()创建时的编号个数

    // 6)函数 template <typename... Arg> bool execute_prepared(execute_handler&& handler, Arg&... arg);
    // 6)说明 执行一次已创建好的SQL预备语句且返回结果集.成功返回true,否则false
    // 6)    回调函数handler的前两个参数分别为字段值,字段大小
    // 6)    函数参数类型为c++基本类型,参数个数为prepare()创建时的通配符个数

    // 7)函数 bool finalize_prepared();
    // 7)说明 删除SQL预备语句, 成功返回true,否则false
}