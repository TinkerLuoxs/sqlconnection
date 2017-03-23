//
// Created by Luoxs on 2017-03-16
//
#pragma once

#include "sqlconfig.hpp"

namespace sql
{
    class db_sqlite;
    class db_mysql;
    class db_pgsql;

    // @ģ�����Database ���� db_sqlite��db_mysql��db_pgsql
    // ������sqlconfig.hpp�ļ����Ժ����ʽ���û�ر�ĳ��ʵ��(���ٿ�����)
    //
    // @��������conninfo �����¼���:
    // 1����ʹ��db_sqliteʱ, ��ʾ���ݿ��ļ�����, �� "/home/mydb.db";
    // 2����ʹ��db_mysql����db_pgsqlʱ, ��ʾһ�� �ؼ���=ֵ ���ַ���, ��:
    //    "host=localhost port=5432 dbname=mydb connect_timeout=10"
    //    �ؼ����� host��port��user��password��dbname��connect_timeout client_encoding
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

    // db connection�ĳ�Ա����˵������:
    // 1)���� bool is_connected();
    // 1)˵�� ���ӳɹ�����true, ����false

    // 2)���� bool execute(std::string const& sql_str);
    // 2)˵�� ִ��û�з��ؽ����SQL�����INSERT,UPDATE.�ɹ�����true,����false

    // 3)���� bool execute(std::string const& sql_str, execute_handler&& handler);
    // 3)˵�� ִ���з��ؽ����SQL�����SELECT.�ɹ�����true,ÿһ�н������һ��handler,����false
    // 3)    �ص�����handler��ǰ���������ֱ�Ϊ����(�ֶ�����),�ֶ�ֵ

    // 4)���� bool prepare(std::string const& pre_str);
    // 4)˵�� ����һ��SQLԤ�����.�ɹ�����true,����false
    // 4)    ������sqlite/mysqlʱ, pre_str��ʽ�� "INSERT INTO user(id,name) VALUES(?, ?)"
    // 4)    ������pgsqlsʱ, pre_str��ʽ�� "INSERT INTO foo VALUES($1, $2, $3, $4);"

    // 5)���� template <typename... Arg> bool execute_prepared(Arg&... arg);
    // 5)˵�� ִ��һ���Ѵ����õ�SQLԤ�����.�ɹ�����true,����false
    // 5)    ������������Ϊc++��������,��������Ϊprepare()����ʱ�ı�Ÿ���

    // 6)���� template <typename... Arg> bool execute_prepared(execute_handler&& handler, Arg&... arg);
    // 6)˵�� ִ��һ���Ѵ����õ�SQLԤ������ҷ��ؽ����.�ɹ�����true,����false
    // 6)    �ص�����handler��ǰ���������ֱ�Ϊ����(�ֶ�����),�ֶ�ֵ
    // 6)    ������������Ϊc++��������,��������Ϊprepare()����ʱ��ͨ�������

    // 7)���� bool finalize_prepared();
    // 7)˵�� ɾ��SQLԤ�����, �ɹ�����true,����false
}