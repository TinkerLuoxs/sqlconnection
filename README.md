# sqlconnection
sqlite、mysql、postgresql connection library, header-only

## 说明

封装数据库的底层接口，提供c++调用方式的接口，调用见test.cpp.

## 编译事项：
1. 编译器需支持c++14
2. 数据库需要支持:

   sqlite             sqlite3.h/c

   mysql              MySQL Connector/C

   postgresql         libpq

