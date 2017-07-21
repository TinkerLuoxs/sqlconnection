//
// Created by Luoxs on 2017-03-16
//
#pragma once

namespace sql
{
    template <typename T>
    static std::enable_if_t<std::is_integral<T>::value> assign_t(T& t, const char *value, unsigned long)
    {
        char *end;
        t = static_cast<T>(std::strtoll(value, &end, 10));
    }

    template <typename T>
    static std::enable_if_t<std::is_floating_point<T>::value> assign_t(T& t, const char *value, unsigned long)
    {
        char *end;
        t = static_cast<T>(std::strtold(value, &end));
    }

    static void assign_t(std::string& t, const char *value, unsigned long)
    {
        t = value;
    }

    static void assign_t(blob_t& t, const char *value, unsigned long size)
    {
        t.size = size;
        t.data.reset(new char[size], std::default_delete<char[]>());
        memcpy(t.data.get(), value, size);
    }

    template <size_t I>
    static void assign(char **, unsigned long *)
    {
    }

    template <size_t I, typename Arg, typename... Args>
    static void assign(char **value, unsigned long *size, Arg& arg, Args&... args)
    {
        if (value[I]) assign_t(arg, value[I], size[I]);
        else SQLDEBUG("field value[%zu] is null\n", I);
        assign<I + 1>(value, size, args...);
    }

    template <typename... Args>
    static void assign(char **val_cols, unsigned long *size_cols, Args&... args)
    {
        assign<0>(val_cols, size_cols, args...);
    }
}
