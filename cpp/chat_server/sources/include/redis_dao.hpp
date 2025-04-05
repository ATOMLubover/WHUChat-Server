#pragma once

#include "singleton.hpp"
#include "redis_reply.hpp"
#include "redis_context.hpp"
#include "redis_conn_pool.hpp"

#include <hiredis/hiredis.h>

#include <string>
#include <memory>
#include <vector>
#include <initializer_list>

class RedisDao
{
    using ByteList = const char*;

public:
    RedisDao();
    RedisDao( const RedisDao& ) = delete;
    RedisDao& operator=( const RedisDao& ) = delete;
    ~RedisDao();

    void Close();

    bool Get( const std::string& input_key, std::string* output_value );
    bool Set( const std::string& key, const std::string& value );

    // bool Authorize( const std::string& password );

    bool LeftPush( const std::string& key, const std::string& value );
    bool LeftPop( const std::string& input_key, std::string* output_value_popped );
    bool RightPush( const std::string& key, const std::string& value );
    bool RightPop( const std::string& input_key, std::string* output_value_popped );

    bool HashSet( const std::string& key1, const std::string& key2, const std::string& value );
    bool HashSet( const std::string& key1, const std::string& key2, ByteList value_data, std::size_t value_len );
    bool HashGet( const std::string& input_key1, const std::string& input_key2, std::string* output_value );

    bool Delete( const std::string& key );

    bool IsKeyExisting( const std::string& key );

private:
    // 传送字符串形式的内容
    RedisReply::Raw* SendCommand(
        RedisContext::Raw* context, const std::string& command );
    // 传入的所有参数可以是字节存储的二进制数据
    RedisReply::Raw* SendCommandArgv(
        RedisContext::Raw* context, const std::initializer_list<ByteList>& argv );

private:
    std::unique_ptr<RedisConnPool> connection_pool;
};