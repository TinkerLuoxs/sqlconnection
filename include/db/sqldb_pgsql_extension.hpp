//
// Created by Luoxs on 2017-03-16
//
#pragma once

namespace sql
{
    /// support for composites, arrays and composite arrays
    /// serialization.hpp required to include.
    namespace db_pgsql_extension
    {
        class composite
        {
        public:
            class decoder;
            class encoder;

            template <typename T>
            static void from_string(T&& t, const char *reply)
            {
                decoder dec(reply);
                dec.read(t);
            }

            template <typename T>
            static std::string to_string(T&& t)
            {
                std::string result;
                {
                    encoder enc(result);
                    enc.write(t);
                }
                return std::move(result);
            }
        };

        class composite::encoder
        {
        public:
            explicit encoder(std::string& reply) : _iter(reply)
            {
            }
            ~encoder()
            {
                remove();
            }

            template <typename T>
            std::enable_if_t<std::is_arithmetic<T>::value> write(T const& t)
            {
                _iter += std::to_string(t);
                _iter += ",";
            }

            template <typename T>
            std::enable_if_t<has_series<T>::value> write(T const& t)
            {
                _iter += "\"(";
                serialize::foreach([this](auto &&elem) {this->write(elem); }, t);
                remove();
                _iter += ")\",";
            }

            template <typename T>
            void write(std::vector<T> const& t)
            {
                _iter += "{";

                for (auto& i : t)
                    write(i);

                remove();
                _iter += "},";
            }

            void write(std::string const& v)
            {
#if 0
                bool q = v.find(' ') == std::string::npos;
                if (!q)
                {
                    _iter += "\\\"";
                    _iter += v;
                    _iter += "\\\",";
                }
                else
                {
                    _iter += v;
                    _iter += ",";
                }
#endif
                _iter += v;
                _iter += ",";
            }

        private:
            void remove()
            {
                if (_iter.back() == ',')
                    _iter.pop_back();
            }

            std::string& _iter;
        };

        class composite::decoder
        {
        public:
            explicit decoder(const char *reply) : _iter(reply), _array_count(0),
                _field_head(nullptr), _field_tail(nullptr)
            {
            }

            template <typename T>
            std::enable_if_t<std::is_arithmetic<T>::value> read(T& t)
            {
                if (!read_field())
                    return;

                if (_field_tail == _field_head)
                    memset(&t, 0, sizeof(t));
                else
                    read_field_impl(t);
            }

            void read(std::string& v)
            {
                if (read_field())
                    read_field_impl(v);
            }

            template <typename T>
            std::enable_if_t<has_series<T>::value> read(T& t)
            {
                serialize::foreach([this](auto &&elem) {this->read(elem); }, t);
            }

            template <typename T>
            void read(std::vector<T>& t)
            {
                int count = -1;
                do 
                {
                    if (!next_array(count))
                        break;

                    t.emplace_back();
                    auto& last = t.back();
                    serialize::foreach([this](auto &&elem) { this->read(elem); }, last);
                } while (true);
            }

        private:
            template <typename T>
            std::enable_if_t<std::is_integral<T>::value> read_field_impl(T& val)
            {
                char *end;
                val = static_cast<T>(std::strtoll(_field_head, &end, 10));
            }

            template <typename T>
            std::enable_if_t<std::is_floating_point<T>::value> read_field_impl(T& val)
            {
                char *end;
                val = static_cast<T>(std::strtold(_field_head, &end));
            }

            void read_field_impl(std::string& val)
            {
                val.assign(_field_head, _field_tail);
            }

            bool next_array(int& count)
            {
                while (*_iter)
                {
                    switch (*_iter)
                    {
                    case '{': 
                        if (count < 0) count = _array_count; 
                        ++_array_count;
                        ++_iter;
                        return true;
                    case '}': 
                        --_array_count;
                        ++_iter;
                        break;
                    case ',': 
                        ++_iter;
                        if (_array_count > 0) 
                            return count < _array_count; 
                        return false;
                    case ' ': case '(': case ')': case '\\': case '\"': 
                        ++_iter;
                        break;
                    default:
                        return false;
                    }
                }
                return false;
            }

            bool next_field()
            {
                bool nvl = false;
                while (*_iter)
                {
                    switch (*_iter)
                    {
                    case ' ': case '(': case ')': case '\\': case '\"':
                        ++_iter;
                        break;
                    case ',':
                        if (nvl)
                            return true;
                        ++_iter;
                        nvl = true;
                        break;
                    case '{': 
                        ++_array_count; 
                        ++_iter;
                        break;
                    case '}':
                        --_array_count;
                        ++_iter;
                        break;
                    case '\0':
                        return false;
                    default: 
                        return true;
                    }
                }
                return false;
            }

            bool read_field()
            {
                if (!next_field())
                    return false;

                _field_tail = _field_head = _iter;
                while (consume(*_iter)) 
                    ++_iter;
                return true;
            }

            bool consume(char c)
            {
                switch (c)
                {
                case '\0': case '{': case '}': case ',':
                case '(': case ')': case '\\': case '\"':
                    _field_tail = _iter;
                    return false;
                default:
                    return true;
                }
            }

            int _array_count;
            const char *_iter;
            const char *_field_head;
            const char *_field_tail;
        };

    }
}