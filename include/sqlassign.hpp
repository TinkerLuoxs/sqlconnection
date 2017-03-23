//
// Created by Luoxs on 2017-03-16
//
#pragma once

namespace sql
{
    void assign_t(const char *value, char& t)
    {
        t = std::stoi(value);
    }

    void assign_t(const char *value, short& t)
    {
        t = std::stoi(value);
    }

    void assign_t(const char *value, int& t)
    {
        t = std::stoi(value);
    }

    void assign_t(const char *value, long long& t)
    {
        t = std::stoll(value);
    }

    void assign_t(const char *value, unsigned char& t)
    {
        t = std::stoi(value);
    }

    void assign_t(const char *value, unsigned short& t)
    {
        t = std::stoi(value);
    }

    void assign_t(const char *value, unsigned int& t)
    {
        t = std::stoul(value);
    }

    void assign_t(const char *value, unsigned long long& t)
    {
        t = std::stoull(value);
    }

    void assign_t(const char *value, float& t)
    {
        t = std::stof(value);
    }

    void assign_t(const char *value, double& t)
    {
        t = std::stod(value);
    }

    void assign_t(const char *value, std::string& t)
    {
        t = value;
    }

    template <size_t I>
    void assign(char **value)
    {
    }

    template <size_t I, typename Arg, typename... Args>
    void assign(char **value, Arg& arg, Args&... args)
    {
        if (value[I]) assign_t(value[I], arg);
        else SQLDEBUG("field value[%zu] is null\n", I);
        assign<I + 1>(value, args...);
    }

    template <typename... Args>
    void assign(int num_cols, char **val_cols, Args&... args)
    {
        constexpr int num_args = sizeof...(args);
        if (num_cols == num_args)
            assign<0>(val_cols, args...);
        else SQLDEBUG("num_cols[%d] != num_args[%d]\n", num_cols, num_args);
    }

}
