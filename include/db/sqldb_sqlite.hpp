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
            if (SQLITE_OK == sqlite3_open(conninfo.data(), &_db))
                _connected = true;
        }

        ~db_sqlite()
        {
            sqlite3_close(_db);
        }

        bool is_connected()
        {
            return _connected;
        }

        bool execute(std::string const& sql_str)
        {
            int result = sqlite3_exec(_db, sql_str.data(), nullptr, nullptr, nullptr);
            SQLDEBUG("%s\n", sqlite3_errmsg(_db));
            return SQLITE_OK == result;
        }

        bool execute(std::string const& sql_str, execute_handler&& handler)
        {
            sqlite3_stmt *stmt = nullptr;
            sqlite3_prepare_v2(_db, sql_str.data(), sql_str.size(), &stmt, nullptr);
            bool result = fetch(stmt, std::move(handler));
            SQLDEBUG("%s\n", sqlite3_errmsg(_db));
            sqlite3_reset(stmt);
            sqlite3_finalize(stmt);
            return result;
        }

        bool prepare(std::string const& pre_str)
        {
            int result = sqlite3_prepare_v2(_db, pre_str.data(), pre_str.size(), &_stmt, nullptr);
            SQLDEBUG("%s\n", sqlite3_errmsg(_db));
            return SQLITE_OK == result;
        }

        template <typename... Arg>
        bool execute_prepared(Arg&... arg)
        {
            bind<1>(arg...);
            int result = sqlite3_step(_stmt);
            SQLDEBUG("%s\n", sqlite3_errmsg(_db));
            sqlite3_reset(_stmt);
            return SQLITE_DONE == result;
        }

        template <typename... Arg>
        bool execute_prepared(execute_handler&& handler, Arg&... arg)
        {
            bind<1>(arg...);
            bool result = fetch(_stmt, std::move(handler));
            SQLDEBUG("%s\n", sqlite3_errmsg(_db));
            sqlite3_reset(_stmt);
            return result;
        }

        bool finalize_prepared()
        {
            return SQLITE_OK == sqlite3_finalize(_stmt);
        }

    private:
        bool fetch(sqlite3_stmt *stmt, execute_handler&& handler)
        {
            const int num_cols = sqlite3_column_count(stmt);
            std::vector<char *> val_cols(num_cols, nullptr);
            std::vector<unsigned long> size_cols(num_cols, 0);
            int i = 0, type = 0;
            do
            {
                if (sqlite3_step(stmt) != SQLITE_ROW)
                    return false;

                for (i = 0; i < num_cols; ++i)
                {
                    type = sqlite3_column_type(stmt, i);
                    switch (type)
                    {
                    default:
                        val_cols[i] = (char *)sqlite3_column_text(stmt, i);
                        break;
                    case SQLITE_NULL:
                        val_cols[i] = nullptr;
                        break;
                    case SQLITE_BLOB:
                        val_cols[i] = (char *)sqlite3_column_blob(stmt, i);
                        size_cols[i] = sqlite3_column_bytes(stmt, i);
                        break;
                    }
                }
                handler(&val_cols[0], &size_cols[0]);
            } while (true);
            return true;
        }

        template <typename T, typename = std::enable_if_t<std::is_integral<T>::value &&
            !std::is_same<T, long long>::value && !std::is_same<T, unsigned long long>::value>>
        int bind_t(T const& item, size_t I)
        {
            return sqlite3_bind_int(_stmt, I, item);
        }

        int bind_t(long long const& item, size_t I)
        {
            return sqlite3_bind_int64(_stmt, I, item);
        }

        int bind_t(unsigned long long const& item, size_t I)
        {
            return sqlite3_bind_int64(_stmt, I, item);
        }

        int bind_t(double const& item, size_t I)
        {
            return sqlite3_bind_double(_stmt, I, item);
        }

        int bind_t(float const& item, size_t I)
        {
            return sqlite3_bind_double(_stmt, I, item);
        }

        int bind_t(std::string const& item, size_t I)
        {
            return sqlite3_bind_text(_stmt, I, item.data(), item.size(), SQLITE_STATIC);
        }

        int bind_t(nullptr_t const&, size_t I)
        {
            return sqlite3_bind_null(_stmt, I);
        }

        int bind_t(blob_t const& item, size_t I)
        {
            return sqlite3_bind_blob(_stmt, I, item.data(), item.size(), SQLITE_STATIC);
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

        sqlite3 *_db = nullptr;
        sqlite3_stmt *_stmt = nullptr;
        bool _connected = false;
    };
}
