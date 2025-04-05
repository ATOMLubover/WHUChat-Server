#include "redis_dao.hpp"

#include "config_mgr.hpp"
#include "defer.hpp"

#include <format>

RedisDao::~RedisDao()
{
    Close();
}

//bool RedisDao::Connect( const std::string& host, const std::string& post )
//{
//	connection = redisConnect( host.c_str(), std::stoi( post ) );
//	// 若 connection 为空，或者其 err 不为 0 则返回错误 false
//	if ( connection == nullptr
//		 || ( connection != nullptr && connection->err ) )
//	{
//		std::cout << "redis connect error: "
//			<< ( connection ? connection->errstr : "connection is nullptr" ) << std::endl;
//		return false;
//	}
//	return true;
//}

void RedisDao::Close()
{
    connection_pool->ClosePool();
}

bool RedisDao::Get( const std::string& input_key, std::string* output_value )
{
    RedisContext::Raw* connection = connection_pool->TakeConn();
    if ( !connection )
        return false;

    Defer defer(
        [ this, &connection ] ()
        {
            connection_pool->ReturnConn( connection );
        } );

    std::string command = std::format( "GET {}", input_key );
    std::string log_failure = std::format( "[ {} ] failed ", command );

    RedisReply reply( SendCommand( connection, command ) );
    if ( !reply )
    {
        std::cout << log_failure << std::endl;
        return false;
    }
    if ( reply->type != REDIS_REPLY_STRING )
    {
        std::cout << log_failure << std::endl;
        return false;
    }
    std::string log_success = std::format( "[ {} ] executed successfully", command );
    std::cout << log_success << std::endl;

    if ( output_value )
        *output_value = reply->str; // 返回 key 对应的 value

    return true;
}

bool RedisDao::Set( const std::string& key, const std::string& value )
{
    RedisContext::Raw* connection = connection_pool->TakeConn();
    if ( !connection )
        return false;

    Defer defer(
        [ this, &connection ] ()
        {
            connection_pool->ReturnConn( connection );
        } );

    std::string command = std::format( "SET {} {}", key, value );
    std::string log_failure = std::format( "[ {} ] failed ", command );

    RedisReply reply( SendCommand( connection, command ) );
    if ( !reply )
    {
        std::cout << log_failure << std::endl;
        return false;
    }
    bool is_ok
        = reply->type == REDIS_REPLY_STATUS
        && ( strcmp( reply->str, "OK" ) == 0 /* || strcmp( reply->str, "ok" ) == 0 */ );
    if ( !is_ok )
    {
        std::cout << log_failure << std::endl;
        return false;
    }
    std::string log_success = std::format( "[ {} ] executed successfully", command );
    std::cout << log_success << std::endl;

    return true;
}

//bool RedisDao::Authorize( const std::string& password )
//{
//	std::string command = std::format( "AUTH {}", password );
//	RedisReply reply( SendCommand( command ) );
//	if ( !reply )
//	{
//		std::cout << "authorize failed" << std::endl;
//		return false;
//	}
//	if ( reply->type == REDIS_REPLY_ERROR )
//	{
//		std::cout << "authorize failed" << std::endl;
//		return false;
//	}
//
//	std::cout << "authorize succeeded" << std::endl;
//	return true;
//}

bool RedisDao::LeftPush( const std::string& key, const std::string& value )
{
    RedisContext::Raw* connection = connection_pool->TakeConn();
    if ( !connection )
        return false;

    Defer defer(
        [ this, &connection ] ()
        {
            connection_pool->ReturnConn( connection );
        } );

    std::string command = std::format( "LPUSH {} {}", key, value );
    std::string log_failure = std::format( "[ {} ] failed", command );

    RedisReply reply( SendCommand( connection, command ) );
    if ( !reply )
    {
        std::cout << log_failure << std::endl;
        return false;
    }
    // 当 LPUSH 成功时，返回值是产生/更新的队列的长度
    if ( reply->type != REDIS_REPLY_INTEGER || reply->integer <= 0 )
    {
        std::cout << log_failure << std::endl;
        return false;
    }
    std::string log_success = std::format( "[ {} ] executed successfully", command );
    std::cout << log_success << std::endl;

    return true;
}

bool RedisDao::LeftPop( const std::string& input_key, std::string* output_value_popped )
{
    RedisContext::Raw* connection = connection_pool->TakeConn();
    if ( !connection )
        return false;

    Defer defer(
        [ this, &connection ] ()
        {
            connection_pool->ReturnConn( connection );
        } );

    std::string command = std::format( "LPOP {}", input_key );
    std::string log_failure = std::format( "[ {} ] failed", command );

    RedisReply reply( SendCommand( connection, command ) );
    if ( !reply )
    {
        std::cout << log_failure << std::endl;
        return false;
    }
    if ( reply->type == REDIS_REPLY_NIL )
    {
        std::cout << log_failure << std::endl;
        return false;
    }
    std::string log_success = std::format( "[ {} ] executed successfully", command );
    std::cout << log_success << std::endl;

    if ( output_value_popped )
        *output_value_popped = reply->str;

    return true;
}

bool RedisDao::RightPush( const std::string& key, const std::string& value )
{
    RedisContext::Raw* connection = connection_pool->TakeConn();
    if ( !connection )
        return false;

    Defer defer(
        [ this, &connection ] ()
        {
            connection_pool->ReturnConn( connection );
        } );

    std::string command = std::format( "RPUSH {} {}", key, value );
    std::string log_failure = std::format( "[ {} ] failed", command );

    RedisReply reply( SendCommand( connection, command ) );
    if ( !reply )
    {
        std::cout << log_failure << std::endl;
        return false;
    }
    // 当 RPUSH 成功时，返回值同样是产生/更新的队列的长度
    if ( reply->type != REDIS_REPLY_INTEGER || reply->integer <= 0 )
    {
        std::cout << log_failure << std::endl;
        return false;
    }
    std::string log_success = std::format( "[ {} ] executed successfully", command );
    std::cout << log_success << std::endl;

    return true;
}

bool RedisDao::RightPop( const std::string& input_key, std::string* output_value_popped )
{
    RedisContext::Raw* connection = connection_pool->TakeConn();
    if ( !connection )
        return false;

    Defer defer(
        [ this, &connection ] ()
        {
            connection_pool->ReturnConn( connection );
        } );

    std::string command = std::format( "LPOP {}", input_key );
    std::string log_failure = std::format( "[ {} ] failed", command );

    RedisReply reply( SendCommand( connection, command ) );
    if ( !reply )
    {
        std::cout << log_failure << std::endl;
        return false;
    }
    if ( reply->type == REDIS_REPLY_NIL )
    {
        std::cout << log_failure << std::endl;
        return false;
    }
    std::string log_success = std::format( "[ {} ] executed successfully", command );
    std::cout << log_success << std::endl;

    if ( output_value_popped )
        *output_value_popped = reply->str;

    return true;
}

bool RedisDao::HashSet( const std::string& key1, const std::string& key2, const std::string& value )
{
    RedisContext::Raw* connection = connection_pool->TakeConn();
    if ( !connection )
        return false;

    Defer defer(
        [ this, &connection ] ()
        {
            connection_pool->ReturnConn( connection );
        } );

    std::string command = std::format( "HSET {} {} {}", key1, key2, value );
    std::string log_failure = std::format( "[ {} ] failed", command );

    RedisReply reply( SendCommand( connection, command ) );
    if ( !reply )
    {
        std::cout << log_failure << std::endl;
        return false;
    }
    if ( reply->type != REDIS_REPLY_INTEGER || reply->integer <= 0 )
    {
        std::cout << log_failure << std::endl;
        return false;
    }
    std::string log_success = std::format( "[ {} ] executed successfully", command );
    std::cout << log_success << std::endl;

    return true;
}

bool RedisDao::HashSet(
    const std::string& key1, const std::string& key2,
    ByteList value_data, std::size_t value_len )
{
    RedisContext::Raw* connection = connection_pool->TakeConn();
    if ( !connection )
        return false;

    Defer defer(
        [ this, &connection ] ()
        {
            connection_pool->ReturnConn( connection );
        } );

    std::string str_data( value_data, value_len );
    std::string command_log = std::format( "HSET {} {} <{} bytes data>", key1, key2, value_len );
    std::string log_failure = std::format( "[ {} ] failed", command_log );

    RedisReply reply( SendCommandArgv( connection, { "HSET", key1.c_str(), key2.c_str(), str_data.c_str() } ) );
    if ( !reply )
    {
        std::cout << log_failure << std::endl;
        return false;
    }
    if ( reply->type != REDIS_REPLY_INTEGER )
    {
        std::cout << log_failure << std::endl;
        return false;
    }
    std::string log_success = std::format( "[ {} ] executed successfully", command_log );
    std::cout << log_success << std::endl;

    return true;
}

bool RedisDao::HashGet( const std::string& input_key1, const std::string& input_key2, std::string* output_value )
{
    RedisContext::Raw* connection = connection_pool->TakeConn();
    if ( !connection )
        return false;

    Defer defer(
        [ this, &connection ] ()
        {
            connection_pool->ReturnConn( connection );
        } );

    std::string command = std::format( "HGET {} {}", input_key1, input_key2 );
    std::string log_failure = std::format( "[ {} ] failed", command );

    RedisReply reply( SendCommand( connection, command ) );
    if ( !reply )
    {
        std::cout << log_failure << std::endl;
        return false;
    }
    if ( reply->type == REDIS_REPLY_NIL )
    {
        std::cout << log_failure << std::endl;
        return false;
    }
    std::string log_success = std::format( "[ {} ] executed successfully", command );
    std::cout << log_success << std::endl;

    if ( output_value )
        *output_value = reply->str;

    return true;
}

bool RedisDao::Delete( const std::string& key )
{
    RedisContext::Raw* connection = connection_pool->TakeConn();
    if ( !connection )
        return false;

    Defer defer(
        [ this, &connection ] ()
        {
            connection_pool->ReturnConn( connection );
        } );

    std::string command = std::format( "DEL {}", key );
    std::string log_failure = std::format( "[ {} ] failed", command );

    RedisReply reply( SendCommand( connection, command ) );
    if ( !reply )
    {
        std::cout << log_failure << std::endl;
        return false;
    }
    if ( reply->type != REDIS_REPLY_INTEGER )
    {
        std::cout << log_failure << std::endl;
        return false;
    }
    std::string log_success = std::format( "[ {} ] executed successfully", command );
    std::cout << log_success << std::endl;

    return true;
}

bool RedisDao::IsKeyExisting( const std::string& key )
{
    RedisContext::Raw* connection = connection_pool->TakeConn();
    if ( !connection )
        return false;

    Defer defer(
        [ this, &connection ] ()
        {
            connection_pool->ReturnConn( connection );
        } );

    std::string command = std::format( "exists {}", key );

    RedisReply reply( SendCommand( connection, command ) );
    if ( !reply )
    {
        std::cout << std::format( "Not found [ Key = {} ]", key ) << std::endl;
        return false;
    }
    if ( reply->type != REDIS_REPLY_INTEGER || reply->integer == 0 )
    {
        std::cout << std::format( "Not found [ Key = {} ]", key ) << std::endl;
        return false;
    }
    std::cout << std::format( "Found [ Key = {} ]", key ) << std::endl;

    return true;
}

RedisDao::RedisDao()
{
    std::string host = ConfigMgr::GetInstance()[ "redis" ][ "host" ];
    std::string port = ConfigMgr::GetInstance()[ "redis" ][ "port" ];
    std::string pwd = ConfigMgr::GetInstance()[ "redis" ][ "password" ];
    connection_pool.reset( new RedisConnPool( 5, host, port, pwd ) );
}

// 传送字符串形式的内容
inline RedisReply::Raw* RedisDao::SendCommand(
    RedisContext::Raw* connection, const std::string& command )
{
    return ( RedisReply::Raw* ) redisCommand( connection, command.c_str() );
}

// 传入的所有参数可以是字节存储的二进制数据
RedisReply::Raw* RedisDao::SendCommandArgv(
    RedisContext::Raw* connection, const std::initializer_list<ByteList>& argv )
{

    std::vector<std::size_t> vec_argvlen;
    for ( const auto& arg : argv )
        vec_argvlen.push_back( strlen( arg ) );
    std::vector<ByteList> vec_argv( argv );

    ByteList* array_argv = vec_argv.data();
    const std::size_t* array_argvlen = vec_argvlen.data();

    return ( RedisReply::Raw* ) redisCommandArgv(
        connection,
        ( int ) vec_argv.size(), array_argv, array_argvlen );
}
