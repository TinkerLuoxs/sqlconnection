//
// Created by Luoxs on 2017-03-16
//
#pragma once

namespace sql
{
namespace my
{
    class resultset
    {
    public:
        explicit resultset(unsigned int size); 
        ~resultset();

        void set_bind(MYSQL_RES *meta_result);
        MYSQL_BIND *get_bind();
        char **get_value();

    private:
        void allocate_buffer(size_t size);

        void setup_one_fetch(MYSQL_BIND *param, MYSQL_FIELD *field);

        const unsigned int _array_size;

        std::vector<MYSQL_BIND> _bindvalue;
        std::vector<my_bool> _nullvalue;
        std::vector<unsigned long> _real_length;

        char *_valid_buffer;
        char *_alloc_buffer;

        std::vector<std::string> _fieldvalue;
        std::vector<char *> _target;
    };

    resultset::resultset(unsigned int size) : _array_size(size), _bindvalue(size), 
        _nullvalue(size, 0), _real_length(size, 0), _valid_buffer(NULL),
        _alloc_buffer(NULL), _fieldvalue(size), _target(size, nullptr)
    {
        memset(&_bindvalue[0], 0, sizeof(MYSQL_BIND) * size);
    }

    resultset::~resultset()
    {
        free(_alloc_buffer);
    }

    void resultset::allocate_buffer(size_t size)
    {
//         SQLDEBUG("allocate memory size : %zu\n", size);
        _alloc_buffer = (char *)malloc(size);
        _valid_buffer = _alloc_buffer;
        memset(_valid_buffer, 0, size);
    }

    void resultset::set_bind(MYSQL_RES *meta_result)
    {
        auto fields = mysql_fetch_fields(meta_result);
        unsigned int i = 0;
        size_t alloc_size = 0;
        for (i = 0; i < _array_size; ++i)
        {
            ++alloc_size;
            alloc_size += fields[i].max_length;
        }

        allocate_buffer(alloc_size);
        for (unsigned int i = 0; i < _array_size; ++i)
        {
            _target[i] = _valid_buffer;
            setup_one_fetch(&_bindvalue[i], &fields[i]);
            _bindvalue[i].length = &_real_length[i];
            _bindvalue[i].is_null = &_nullvalue[i];
        }
    }

    void resultset::setup_one_fetch(MYSQL_BIND *param, MYSQL_FIELD *field)
    {
        param->buffer_type = MYSQL_TYPE_STRING;
        param->buffer = _valid_buffer;
        param->buffer_length = field->max_length + 1;
        _valid_buffer += param->buffer_length;
    }

    MYSQL_BIND * resultset::get_bind()
    {
        return &_bindvalue[0];
    }

    char ** resultset::get_value()
    {
        for (unsigned int i = 0; i < _array_size; ++i)
        {
            if (_nullvalue[i])
                _target[i] = nullptr;
        }
        return &_target[0];
    }

}
}