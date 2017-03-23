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
        explicit db_pgsql(std::string const& conninfo)
        {
            _handler = PQconnectdb(conninfo.c_str());
            if (CONNECTION_OK == PQstatus(_handler))
                _connected = true;
        }

        ~db_pgsql()
        {
            PQfinish(_handler);
        }

        bool is_connected()
        {
            return _connected;
        }

        bool execute(std::string const& sql_str)
        {
            auto res = PQexec(_handler, sql_str.data());
            SQLDEBUG("%s\n", PQresultErrorMessage(res));
            bool result = PGRES_COMMAND_OK == PQresultStatus(res);
            PQclear(res);
            return result;
        }

        bool execute(std::string const& sql_str, execute_handler&& handler)
        {
            auto res = PQexec(_handler, sql_str.data());
            SQLDEBUG("%s\n", PQresultErrorMessage(res));
            bool result = fetch_result(std::move(handler), res);
            PQclear(res);
            return result;
        }

        bool prepare(std::string const& pre_str)
        {
            auto res = PQprepare(_handler, "", pre_str.data(), 0, NULL);
            SQLDEBUG("%s\n", PQresultErrorMessage(res));
            bool result = PGRES_COMMAND_OK == PQresultStatus(res);
            PQclear(res);
            return result;
        }

        bool execute_prepared()
        {
            auto res = PQexecPrepared(_handler, "", 0, NULL, NULL, NULL, 0);
            SQLDEBUG("%s\n", PQresultErrorMessage(res));
            bool result = PGRES_COMMAND_OK == PQresultStatus(res);
            PQclear(res);
            return result;
        }

        template <typename... Arg, typename = std::enable_if_t<sizeof...(Arg) >= 1>>
        bool execute_prepared(Arg&... arg)
        {
            constexpr int num_args = sizeof...(arg);
            param_bind param(num_args);
            converts_value<0>(&param, arg...);
            auto res = PQexecPrepared(_handler, "", num_args, &param.value[0], NULL, NULL, 0);
            SQLDEBUG("%s\n", PQresultErrorMessage(res));
            bool result = PGRES_COMMAND_OK == PQresultStatus(res);
            PQclear(res);
            return result;
        }

        bool execute_prepared(execute_handler&& handler)
        {
            auto res = PQexecPrepared(_handler, "", 0, NULL, NULL, NULL, 0);
            SQLDEBUG("%s\n", PQresultErrorMessage(res));
            auto result = fetch_result(std::move(handler), res);
            PQclear(res);
            return result;
        }

        template <typename... Arg, typename = std::enable_if_t<sizeof...(Arg) >= 1>>
        bool execute_prepared(execute_handler&& handler, Arg&... arg)
        {
            constexpr int num_args = sizeof...(arg);
            param_bind param(num_args);
            converts_value<0>(&param, arg...);
            auto res = PQexecPrepared(_handler, "", num_args, &param.value[0], NULL, NULL, 0);
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
        struct param_bind
        {
            std::vector<const char *> value;
            std::vector<std::string> type;
            param_bind(size_t num) : value(num, nullptr), type(num) {}
        };

        template <typename T, typename = std::enable_if_t<std::is_arithmetic<T>::value>>
        int converts(param_bind *param, T& item, size_t I)
        {
            auto& val = param->type[I];
            val = std::to_string(item);
            param->value[I] = val.data();
            return 0;
        }

        int converts(param_bind *param, nullptr_t& , size_t I)
        {
            param->value[I] = nullptr;
            return 0;
        }

        int converts(param_bind *param, std::string& arg, size_t I)
        {
            param->value[I] = arg.data();
            return 0;
        }

        template<size_t I, typename Arg, typename... Args>
        int converts_value(param_bind *param, Arg& arg, Args&... args)
        {
            if (converts(param, arg, I))
                return -1;
            return converts_value<I + 1>(param, args...);
        }

        template<size_t I>
        int converts_value(param_bind *param)
        {
            return 0;
        }

        bool fetch_result(execute_handler&& handler, PGresult *res)
        {
            if (PGRES_TUPLES_OK != PQresultStatus(res))
                return false;

            auto num_rows = PQntuples(res);
            auto num_cols = PQnfields(res);
            std::vector<char *> v(num_cols, nullptr);

            int i = 0, j = 0;
            for (i = 0; i < num_rows; ++i)
            {
                for (j = 0; j < num_cols; ++j)
                    v[j] = PQgetisnull(res, i, j) ? nullptr : PQgetvalue(res, i, j);
                        
                handler(num_cols, &v[0]);
            }
            return true;
        }


        PGconn *_handler = nullptr;
        bool _connected = false;
    };

}
