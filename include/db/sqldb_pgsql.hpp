//
// Created by Luoxs on 2017-03-16
//
#pragma once

#include "libpq-fe.h"

namespace sql
{
    class db_pgsql
    {
    public:
        explicit db_pgsql(const char *conninfo)
        {
            _db = PQconnectdb(conninfo);
            if (CONNECTION_OK == PQstatus(_db))
                _connected = true;
        }

        ~db_pgsql()
        {
            PQfinish(_db);
        }

        bool is_connected()
        {
            return _connected;
        }

        bool execute(const char *sql_str)
        {
            auto res = PQexec(_db, sql_str);
            SQLDEBUG("%s\n", PQresultErrorMessage(res));
            bool result = PGRES_COMMAND_OK == PQresultStatus(res);
            PQclear(res);
            return result;
        }

        bool execute(const char *sql_str, size_t)
        {
            return execute(sql_str);
        }

        bool execute(const char *sql_str, execute_handler&& handler)
        {
            auto res = PQexec(_db, sql_str);
            SQLDEBUG("%s\n", PQresultErrorMessage(res));
            bool result = fetch_result(std::move(handler), res);
            PQclear(res);
            return result;
        }

        bool execute(const char *sql_str, size_t, execute_handler&& handler)
        {
            return execute(sql_str, std::move(handler));
        }

        // SELECT * FROM mytable WHERE x = $1::bigint;
        bool prepare(const char *pre_str)
        {
            auto res = PQprepare(_db, "", pre_str, 0, NULL);
            SQLDEBUG("%s\n", PQresultErrorMessage(res));
            bool result = PGRES_COMMAND_OK == PQresultStatus(res);
            PQclear(res);
            return result;
        }

        bool prepare(const char *pre_str, size_t)
        {
            return prepare(pre_str);
        }

        bool execute_prepared()
        {
            auto res = PQexecPrepared(_db, "", 0, NULL, NULL, NULL, 0);
            SQLDEBUG("%s\n", PQresultErrorMessage(res));
            bool result = PGRES_COMMAND_OK == PQresultStatus(res);
            PQclear(res);
            return result;
        }

        template <typename... Arg, typename = std::enable_if_t<sizeof...(Arg) >= 1>>
        bool execute_prepared(Arg&... arg)
        {
            constexpr int num_args = sizeof...(arg);
            BindParam param(num_args);
            param_bind<0>(&param, arg...);
            auto res = PQexecPrepared(_db, "", num_args, &param.values[0], &param.lengths[0], &param.formats[0], 0);
            SQLDEBUG("%s\n", PQresultErrorMessage(res));
            bool result = PGRES_COMMAND_OK == PQresultStatus(res);
            PQclear(res);
            return result;
        }

        bool execute_prepared(execute_handler&& handler)
        {
            auto res = PQexecPrepared(_db, "", 0, NULL, NULL, NULL, 0);
            SQLDEBUG("%s\n", PQresultErrorMessage(res));
            auto result = fetch_result(std::move(handler), res);
            PQclear(res);
            return result;
        }

        template <typename... Arg, typename = std::enable_if_t<sizeof...(Arg) >= 1>>
        bool execute_prepared(execute_handler&& handler, Arg&... arg)
        {
            constexpr int num_args = sizeof...(arg);
            BindParam param(num_args);
            param_bind<0>(&param, arg...);
            auto res = PQexecPrepared(_db, "", num_args, &param.values[0], &param.lengths[0], &param.formats[0], 0);
            SQLDEBUG("%s\n", PQresultErrorMessage(res));
            bool result = fetch_result(std::move(handler), res);
            PQclear(res);
            return result;
        }

        bool finalize_prepared()
        {
//             return execute("DEALLOCATE ALL");
            return true;
        }

    private:
        struct BindParam
        {
            std::vector<const char *> values;
            std::vector<int> lengths;
            std::vector<int> formats;
            std::vector<std::string> cache_values;
            BindParam(int num) : values(num, nullptr), lengths(num, 0), formats(num, 0)
            {
                cache_values.reserve(num);
            }
        };

        template <typename T, typename = std::enable_if_t<std::is_arithmetic<T>::value>>
        int converts(BindParam *param, const T& item, size_t I)
        {
            param->cache_values.emplace_back(std::to_string(item));
            param->values[I] = param->cache_values.back().data();
            return 0;
        }

        int converts(BindParam *param, const nullptr_t& , size_t I)
        {
            param->values[I] = nullptr;
            return 0;
        }

        int converts(BindParam *param, const std::string& item, size_t I)
        {
            param->values[I] = item.data();
            return 0;
        }

        int converts(BindParam *param, const blob_t& item, size_t I)
        {
            param->values[I] = item.data.get();
            param->lengths[I] = (int)item.size;
            param->formats[I] = 1;
            return 0;
        }

        template<size_t I, typename Arg, typename... Args>
        int param_bind(BindParam *param, Arg& arg, Args&... args)
        {
            if (converts(param, arg, I))
                return -1;
            return param_bind<I + 1>(param, args...);
        }

        template<size_t I>
        int param_bind(BindParam *param)
        {
            return 0;
        }

        bool fetch_result(execute_handler&& handler, PGresult *res)
        {
            if (PGRES_TUPLES_OK != PQresultStatus(res))
                return false;

            auto num_rows = PQntuples(res);
            auto num_cols = PQnfields(res);
            std::vector<char *> values(num_cols, nullptr);
            std::vector<unsigned long> lengths(num_cols, 0);

            int i = 0, j = 0;
            for (i = 0; i < num_rows; ++i)
            {
                for (j = 0; j < num_cols; ++j)
                {
                    values[j] = PQgetisnull(res, i, j) ? nullptr : PQgetvalue(res, i, j);
                    lengths[j] = PQgetlength(res, i, j);
                }

                handler(&values[0], &lengths[0]);
            }
            return true;
        }

        PGconn *_db = nullptr;
        bool _connected = false;
    };

}
