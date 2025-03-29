#include "include/mysql_conn_pool.hpp"

#include "include/defer.hpp"

MySqlConnPool::MySqlConnPool( const MySqlConnInfo& info, int poolsize )
    : login_info( info ), is_stopped( false ), size( poolsize )
{
    for ( std::size_t i = 0; i < poolsize; i++ )
    {
        que_conn.push( std::move( std::make_unique<MySqlConn>( info ) ) );
    }

    // check_thread = std::thread(
    //     [ this ] ()
    //     {
    //         while ( !this->is_stopped )
    //         {
    //             this->CheckConnections();
    //             std::this_thread::sleep_for( std::chrono::seconds( 60 ) );
    //         }
    //     } );
    // check_thread.detach();
}

MySqlConnPool::~MySqlConnPool()
{
    std::unique_lock<std::mutex> lock( mtx_queconn );

    ClosePool();
    while ( !que_conn.empty() )
        que_conn.pop();
}

// void MySqlConnPool::CheckConnections()
// {
//     std::lock_guard<std::mutex> guard( mtx_queconn );

//     auto curr_time = std::chrono::steady_clock::now().time_since_epoch();
//     std::int64_t sec_curr = std::chrono::duration_cast< std::chrono::seconds >( curr_time ).count();
//     std::size_t size_pool = que_conn.size(); // 不能使用成员函数在 for 循环中获得 size，否则会死循环
//     for ( std::size_t i = 0; i < size_pool; i++ )
//     {
//         std::unique_ptr<MySqlConnPool> conn = std::move( que_conn.front() );
//         que_conn.pop();
//         // 在每次循环结束后自动回收（压回）取出的 conn
//         Defer defer( [ this, &conn ] { this->que_conn.push( std::move( conn ) ); } );

//         // 如果上次查询时间在最近 5 秒之内，直接跳过
//         if ( sec_curr - conn->sec_last_oper < 5 )
//             continue;

//         try
//         {
//             std::unique_ptr<sql::Statement> stmt( conn->conn->createStatement() );
//             stmt->executeQuery( "SELECT 1" );
//             conn->sec_last_oper = sec_curr;

//             std::cout << "execute time alive, curr time is" << sec_curr << std::endl;
//         }
//         catch ( sql::SQLException& exp )
//         {
//             std::cout << "error at keeping MySQL conn alive: " << exp.what() << std::endl;
//             // 创建新链接代替旧链接
//             conn->Rebuild();
//         }
//     }
// }

std::unique_ptr<MySqlConn> MySqlConnPool::TakeConn()
{
    std::unique_lock<std::mutex> lock( mtx_queconn );

    cv_queconn.wait(
        lock,
        [ this ] ()
        {
            if ( this->is_stopped )
                return true;

            return !this->que_conn.empty();
        } );

    if ( is_stopped )
        return nullptr;

    std::unique_ptr<MySqlConn> conn( std::move( que_conn.front() ) );
    que_conn.pop();
    return conn;
}

void MySqlConnPool::ReturnConn( std::unique_ptr<MySqlConn>&& conn )
{
    std::unique_lock<std::mutex> lock( mtx_queconn );

    if ( is_stopped )
        return;

    que_conn.push( std::move( conn ) );
    cv_queconn.notify_one();
}

void MySqlConnPool::ClosePool()
{
    is_stopped = true;
    cv_queconn.notify_all();
}
