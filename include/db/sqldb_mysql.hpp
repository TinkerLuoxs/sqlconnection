//
// Created by Luoxs on 2017-03-16
//
#pragma once

#include <atomic>
#include "mysql.h"
#include "sqldb_mysql_resultset.hpp"

namespace sql
{
    template <typename T>
    struct unique_impl
    {
        static T value;
    };

    template <typename T>
    T unique_impl<T>::value;

    static std::atomic_uint& sql_use_count = unique_impl<std::atomic_uint>::value;

    class db_mysql
    {
    public:
        explicit db_mysql(std::string const& conninfo)
        {
            if (++sql_use_count <= 1)
                mysql_library_init(0, nullptr, nullptr);

            _handler = mysql_init(nullptr);
            set_options();
            launch_connect(conninfo);
        }

        ~db_mysql()
        {
            mysql_close(_handler);

            if (--sql_use_count <= 0)
                mysql_library_end();
        }

        bool is_connected()
        {
            return _connected;
        }

        bool execute(std::string const& sql_str)
        {
            bool result = !mysql_real_query(_handler, sql_str.data(), sql_str.size());
            SQLDEBUG("%s\n", mysql_error(_handler));
            return result;
        }

        bool execute(std::string const& sql_str, execute_handler&& handler)
        {
            if (!execute(sql_str))
                return false;
            auto result = mysql_store_result(_handler);
            SQLDEBUG("%s\n", mysql_error(_handler));
            if (!result) return false;
            auto num_fields = mysql_num_fields(result);
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(result)))
                handler(num_fields, row);
            mysql_free_result(result);
            return true;
        }

        bool prepare(std::string const& pre_str)
        {
            _stmt = mysql_stmt_init(_handler);
            return !mysql_stmt_prepare(_stmt, pre_str.data(), pre_str.size());
        }

        bool execute_prepared()
        {
            auto result = !mysql_stmt_execute(_stmt);
            SQLDEBUG("%s\n", mysql_stmt_error(_stmt));
            mysql_stmt_reset(_stmt);
            return result;
        }

        template <typename... Arg, typename = std::enable_if_t<sizeof...(Arg) >= 1> >
        bool execute_prepared(Arg&... arg)
        {
            constexpr unsigned long num_args = sizeof...(arg);
            if (!equal_params(num_args))
                return false;

            MYSQL_BIND param_bind[num_args] = {};
            in_bind<0>(param_bind, arg...);
            if (mysql_stmt_bind_param(_stmt, param_bind))
                return false;
            return execute_prepared();
        }

        bool execute_prepared(execute_handler&& handler)
        {
            bool result = false;
            do
            {
                if (mysql_stmt_execute(_stmt))
                    break;

                my_bool	bool_tmp = 1;
                mysql_stmt_attr_set(_stmt, STMT_ATTR_UPDATE_MAX_LENGTH, &bool_tmp);

                if (mysql_stmt_store_result(_stmt))
                    break;

                auto num_cols = mysql_stmt_field_count(_stmt);
                if (!num_cols)
                    break;

                auto meta_result = mysql_stmt_result_metadata(_stmt);
                if (!meta_result)
                    break;

                my::resultset resl(num_cols);
                resl.set_bind(meta_result);

                if (!mysql_stmt_bind_result(_stmt, resl.get_bind()))
                {
                    while (!mysql_stmt_fetch(_stmt))
                        handler(num_cols, resl.get_value());
                    result = true;
                }
                mysql_free_result(meta_result);
            } while (0);
            SQLDEBUG("%s\n", mysql_stmt_error(_stmt));
            mysql_stmt_reset(_stmt);
            return result;
        }

        template <typename... Arg, typename = std::enable_if_t<sizeof...(Arg) >= 1> >
        bool execute_prepared(execute_handler&& handler, Arg&... arg)
        {
            constexpr size_t num_args = sizeof...(arg);
            if (!equal_params(num_args))
                return false;

            MYSQL_BIND param_bind[num_args] = {};
            in_bind<0>(param_bind, arg...);
            if (mysql_stmt_bind_param(_stmt, param_bind))
                return false;
            return execute_prepared(std::move(handler));
        }

        bool finalize_prepared()
        {
            return !mysql_stmt_close(_stmt);
        }

    private:
        int in_bind(MYSQL_BIND *bnd, char const& item, size_t I)
        {
            bnd[I].buffer_type = MYSQL_TYPE_TINY;
            bnd[I].buffer = const_cast<char *>(&item);
            return 0;
        }

        int in_bind(MYSQL_BIND *bnd, short const& item, size_t I)
        {
            bnd[I].buffer_type = MYSQL_TYPE_SHORT;
            bnd[I].buffer = const_cast<short *>(&item);
            return 0;
        }

        int in_bind(MYSQL_BIND *bnd, int const& item, size_t I)
        {
            bnd[I].buffer_type = MYSQL_TYPE_LONG;
            bnd[I].buffer = const_cast<int *>(&item);
            return 0;
        }

        int in_bind(MYSQL_BIND *bnd, long long const& item, size_t I)
        {
            bnd[I].buffer_type = MYSQL_TYPE_LONGLONG;
            bnd[I].buffer = const_cast<long long *>(&item);
            return 0;
        }

        int in_bind(MYSQL_BIND *bnd, float const& item, size_t I)
        {
            bnd[I].buffer_type = MYSQL_TYPE_FLOAT;
            bnd[I].buffer = const_cast<float *>(&item);
            return 0;
        }

        int in_bind(MYSQL_BIND *bnd, double const& item, size_t I)
        {
            bnd[I].buffer_type = MYSQL_TYPE_DOUBLE;
            bnd[I].buffer = const_cast<double *>(&item);
            return 0;
        }

        int in_bind(MYSQL_BIND *bnd, std::string const& item, size_t I)
        {
            bnd[I].buffer_type = MYSQL_TYPE_STRING;
            bnd[I].buffer = const_cast<char *>(item.data());
            bnd[I].buffer_length = item.size();
            return 0;
        }

        int in_bind(MYSQL_BIND *bnd, nullptr_t const& item, size_t I)
        {
            bnd[I].buffer_type = MYSQL_TYPE_NULL;
            return 0;
        }

        template<size_t I, typename Arg, typename... Args>
        int in_bind(MYSQL_BIND *bnd, Arg& arg, Args&... args)
        {
            if (in_bind(bnd, arg, I))
                return -1;
            return in_bind<I + 1>(bnd, args...);
        }

        template<size_t I>
        int in_bind(MYSQL_BIND *bnd)
        {
            return 0;
        }

        bool equal_params(unsigned long num)
        {
            return num == mysql_stmt_param_count(_stmt);
        }

        void set_options()
        {
            my_bool value = 1;
            mysql_options(_handler, MYSQL_OPT_RECONNECT, &value);
            mysql_options(_handler, MYSQL_INIT_COMMAND, "SET autocommit=1");
        }

        void launch_connect(std::string const& conninfo)
        {
            char value[NUM_CONNECT_PARAM][LEN_CONNECT_VALUE] = {};
            int i = 0, j = 0;

            const char *info = conninfo.c_str();
            while (*info)
            {
                switch (*info)
                {
                case '=': case ' ': ++i; j = 0; break;
                default: value[i][j++] = *info; break;
                }
                ++info;
            }

            const char *host = nullptr;
            const char *user = nullptr;
            const char *passwd = nullptr;
            const char *dbname = nullptr;
            const char *coding = nullptr;
            unsigned int timeout = 0;
            unsigned int port = 0;

            for (i = 0; i < NUM_CONNECT_PARAM; i += 2)
            {
                if (!strcmp("host", value[i]))
                    host = value[i + 1];
                else if (!strcmp("port", value[i]))
                    port = std::stoi(value[i + 1]);
                else if (!strcmp("user", value[i]))
                    user = value[i + 1];
                else if (!strcmp("password", value[i]))
                    passwd = value[i + 1];
                else if (!strcmp("dbname", value[i]))
                    dbname = value[i + 1];
                else if (!strcmp("connect_timeout", value[i]))
                    timeout = std::stoi(value[i + 1]);
                else if (!strcmp("client_encoding", value[i]))
                    coding = value[i + 1];
            }

            if (timeout > 0)
                mysql_options(_handler, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);

            if (coding)
                mysql_options(_handler, MYSQL_SET_CHARSET_NAME, coding);

            if (mysql_real_connect(_handler, host, user, passwd, dbname, port, nullptr, 0))
                _connected = true;
        }

        MYSQL *_handler = nullptr;
        MYSQL_STMT *_stmt = nullptr;
        bool _connected = false;
        enum { NUM_CONNECT_PARAM = 14, LEN_CONNECT_VALUE = 32 };
    };

}
