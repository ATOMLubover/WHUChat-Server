#include "include/mysql_conn.hpp"
#include "mysql_conn.hpp"

MySqlConn::MySqlConn( const MySqlConnInfo& info )
{
    try
    {
        sql::mysql::MySQL_Driver* driver = sql::mysql::get_driver_instance(); // 返回的 driver 单例不需要 delete
        sql::Connection* sqlconn
            = driver->connect( info.addr, info.user, info.password );
        sqlconn->setSchema( info.schema );
        conn.reset( sqlconn );

        std::cout << "MySqlConn构造成功在：" << info.addr << " "
            << info.schema << std::endl;

        // auto curr_time = std::chrono::steady_clock::now().time_since_epoch();
        // sec_last_oper = std::chrono::duration_cast< std::chrono::seconds >( curr_time ).count();
    }
    catch ( std::exception& exp )
    {
        std::cout << "MySqlConn构造失败" << std::endl;
    }
}

void MySqlConn::Rebuild()
{
    try
    {
        sql::mysql::MySQL_Driver* driver = sql::mysql::get_driver_instance();
        sql::Connection* sqlconn
            = driver->connect( info.addr, info.user, info.password );
        sqlconn->setSchema( info.schema );
        conn.reset( sqlconn );

        // auto curr_time = std::chrono::steady_clock::now().time_since_epoch();
        // sec_last_oper = std::chrono::duration_cast< std::chrono::seconds >( curr_time ).count();
    }
    catch ( std::exception& exp )
    {
        std::cout << "MySqlConn重新建立失败" << std::endl;
    }
}

sql::Connection& MySqlConn::GetRawConn()
{
    return *conn;
}
