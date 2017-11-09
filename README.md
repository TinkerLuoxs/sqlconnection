# sqlconnection
sqlite、mysql、postgresql common connection library, header-only

## 说明

封装数据库的底层接口，提供统一的c++调用方式，更为方便、灵活并且跨平台。调用例子见test.cpp.

## 编译事项：
1. 编译器需支持c++14
2. 数据库需要的底层库如下（当只使用其中一种数据库时，不需要全部包含）:

   sqlite      依赖 sqlite3

   mysql       依赖 libmysql

   postgresql  依赖 libpq
   

