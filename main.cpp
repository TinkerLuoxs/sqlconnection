#include <iostream>
#include <map>
#include "sql/sqlconnection.hpp"
#include "sql/sqlassign.hpp"


#define DB_SQLITE       1
#define DB_MYSQL        2
#define DB_POSTGRESQL   3

#define TEST_DATABASE   (DB_SQLITE)


struct t_weather
{
    std::string city;
    int temp_lo;    // low  temperature
    int temp_hi;    // high temperature
    double prcp;    // precipitation
    std::string date;
};

struct t_city
{
    long long id;
    std::string name;
};

int main()
{
#if TEST_DATABASE == DB_SQLITE
    /// create sqlite connection
    auto db_conn = sql::create_unique_connection<sql::db_sqlite>("e:\\database\\smarthome.db");
#elif TEST_DATABASE == DB_MYSQL
    /// create mysql connection
    auto db_conn = sql::create_unique_connection<sql::db_mysql>(
        "host=192.168.0.152 port=3306 password=123456 user=root dbname=mysql client_encoding=GBK");
#elif TEST_DATABASE == DB_POSTGRESQL
    /// create pgsql connection
    auto db_conn = sql::create_unique_connection<sql::db_pgsql>(
        "host=192.168.0.123 port=5432 password=123456 user=postgres dbname=postgres");
#endif

    if (db_conn->is_connected())
        std::cout << "connected succeed" << std::endl;
    else return -1;

    /// create table.
#if TEST_DATABASE == DB_SQLITE
    const char *sql_create1 = "CREATE TABLE weather ("
        "city TEXT, temp_lo INTEGER, temp_hi INTEGER, prcp REAL, date date)";
    const char *sql_create2 = "CREATE TABLE city ("
        "id INTEGER PRIMARY KEY autoincrement, name TEXT)";
#elif TEST_DATABASE == DB_MYSQL
    const char *sql_create1 = "CREATE TABLE weather ("
        "city varchar(80), temp_lo int, temp_hi int, prcp DOUBLE, date date)";
    const char *sql_create2 = "CREATE TABLE city ("
        "id BIGINT NOT NULL auto_increment primary key, name varchar(80))";
#elif TEST_DATABASE == DB_POSTGRESQL
    const char *sql_create1 = "CREATE TABLE weather ("
        "city varchar(80), temp_lo int, temp_hi int, prcp real, date date)";
    const char *sql_create2 = "CREATE TABLE city (id BIGSERIAL primary key, name text)";
#endif
    db_conn->execute(sql_create1);
    db_conn->execute(sql_create2);

    /// exec insert.
#if TEST_DATABASE == DB_POSTGRESQL
    const char *sql_prepare1 = "INSERT INTO weather VALUES($1, $2, $3, $4, $5)";
    const char *sql_prepare2 = "INSERT INTO weather (date, city, temp_hi, temp_lo) VALUES($1, $2, $3, $4)";
#else
    const char *sql_prepare1 = "INSERT INTO weather VALUES(?, ?, ?, ?, ?)";
    const char *sql_prepare2 = "INSERT INTO weather (date, city, temp_hi, temp_lo) VALUES(?, ?, ?, ?)";
#endif

    db_conn->prepare(sql_prepare1);
    t_weather weather;
    weather.city = "San Francisco";
    weather.temp_lo = 43;
    weather.temp_hi = 57;
    weather.prcp = 0.0;
    weather.date = "1994-11-29";
    db_conn->execute_prepared(weather.city, weather.temp_lo, weather.temp_hi, weather.prcp, weather.date);
    weather.temp_lo = 46;
    weather.temp_hi = 50;
    weather.prcp = 0.25;
    auto nvl = nullptr;
    db_conn->execute_prepared(weather.city, weather.temp_lo, weather.temp_hi, weather.prcp, nvl);
    db_conn->finalize_prepared();

    db_conn->prepare(sql_prepare2);
    weather.city = "Hayward";
    weather.temp_lo = 37;
    weather.temp_hi = 54;
    weather.date = "1994-11-29";
    db_conn->execute_prepared(weather.date, weather.city, weather.temp_hi, weather.temp_lo);
    db_conn->finalize_prepared();

    /// exec select.
    std::vector<t_weather> weathers;
    db_conn->execute("SELECT city, temp_lo, temp_hi, prcp, date FROM weather",
        [&weathers](auto values, auto lengths)
    {
        t_weather w = {};
        sql::assign(values, lengths, w.city, w.temp_lo, w.temp_hi, w.prcp, w.date);
        weathers.push_back(std::move(w));
    });

    db_conn->execute("INSERT INTO city (name) VALUES('Beijing'), ('Shanghai'), ('Guangzhou')");
    std::map<long long, t_city> citys;
    db_conn->execute("SELECT id, name FROM city",
        [&citys](auto values, auto lengths)
    {
        t_city city = {};
        sql::assign(values, lengths, city.id, city.name);
        citys.emplace(city.id, std::move(city));
    });

    return 0;
}
