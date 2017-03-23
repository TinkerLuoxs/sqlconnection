//
// Created by Luoxs on 2017-03-16
//
#pragma once

#include "sqlite3.h"

namespace sql
{
    class db_sqlite
    {
    public:
        explicit db_sqlite(std::string const& conninfo)
        {
            if (SQLITE_OK == sqlite3_open(conninfo.data(), &_handler))
                _connected = true;
        }

        ~db_sqlite()
        {
            sqlite3_close(_handler);
        }

        bool is_connected()
        {
            return _connected;
        }

        bool execute(std::string const& sql_str)
        {
            int result = sqlite3_exec(_handler, sql_str.data(), nullptr, nullptr, nullptr);
            SQLDEBUG("%s\n", sqlite3_errmsg(_handler));
            return SQLITE_OK == result;
        }

        bool execute(std::string const& sql_str, execute_handler&& handler)
        {
            int result = sqlite3_exec(_handler, sql_str.data(), sqlite_callback, &handler, nullptr);
            SQLDEBUG("%s\n", sqlite3_errmsg(_handler));
            return SQLITE_OK == result;
        }

        bool prepare(std::string const& pre_str)
        {
            int result = sqlite3_prepare_v2(_handler, pre_str.data(), pre_str.size(), &_stmt, nullptr);
            SQLDEBUG("%s\n", sqlite3_errmsg(_handler));
            return SQLITE_OK == result;
        }

        template <typename... Arg>
        bool execute_prepared(Arg&... arg)
        {
            bind<1>(arg...);
            int result = sqlite3_step(_stmt);
            SQLDEBUG("%s\n", sqlite3_errmsg(_handler));
            sqlite3_reset(_stmt);
            return SQLITE_DONE == result;
        }

        template <typename... Arg>
        bool execute_prepared(execute_handler&& handler, Arg&... arg)
        {
            bind<1>(arg...);
            int result = SQLITE_OK;
            const int num_cols = sqlite3_column_count(_stmt);
            std::vector<char *> val_cols(num_cols, nullptr);
            int i = 0, type = 0;
            do
            {
                result = sqlite3_step(_stmt);
                if (result != SQLITE_ROW)
                    break;

                for (i = 0; i < num_cols; ++i)
                {
                    type = sqlite3_column_type(_stmt, i);
                    switch (type)
                    {
                    default:
                        val_cols[i] = (char *)sqlite3_column_text(_stmt, i);
                        break;
                    case SQLITE_NULL:
                        val_cols[i] = nullptr;
                        break;
                    case SQLITE_BLOB:
                        val_cols[i] = (char *)sqlite3_column_blob(_stmt, i);
                        break;
                    }
                }
                handler(num_cols, &val_cols[0]);
            } while (true);
            SQLDEBUG("%s\n", sqlite3_errmsg(_handler));
            sqlite3_reset(_stmt);
            return SQLITE_DONE == result;
        }

        bool finalize_prepared()
        {
            return SQLITE_OK == sqlite3_finalize(_stmt);
        }

    private:
        int bind_t(int const& item, size_t I)
        {
            return sqlite3_bind_int(_stmt, I, item);
        }

        int bind_t(long long const& item, size_t I)
        {
            return sqlite3_bind_int64(_stmt, I, item);
        }

        int bind_t(double const& item, size_t I)
        {
            return sqlite3_bind_double(_stmt, I, item);
        }

        int bind_t(std::string const& item, size_t I)
        {
            return sqlite3_bind_text(_stmt, I, item.data(), item.size(), nullptr);
        }

        int bind_t(nullptr_t const&, size_t I)
        {
            return sqlite3_bind_null(_stmt, I);
        }

        template<size_t I, typename Arg, typename... Args>
        int bind(Arg& arg, Args&... args)
        {
            if (SQLITE_OK != bind_t(arg, I))
                return SQLITE_ERROR;
            return bind<I + 1>(args...);
        }

        template<size_t I>
        int bind()
        {
            return 0;
        }

        static int sqlite_callback(void *para, int column, char **column_value, char **column_name)
        {
            auto handler = static_cast<execute_handler *>(para);
            handler->operator()(column, column_value);
            return 0;
        }

        sqlite3 *_handler = nullptr;
        sqlite3_stmt *_stmt = nullptr;
        bool _connected = false;
    };
}
