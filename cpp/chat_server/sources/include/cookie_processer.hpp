#pragma once

#include <map>
#include <string>
#include <sstream>
#include <iomanip>
#include <algorithm>

class CookieProcesser
{
public:
    // 解析 cookie 字符串为键值对（自动解码）
    static std::map<std::string, std::string> Parse( const std::string& str_cookie );

    // 序列化键值对为Cookie字符串（自动编码）
    static std::string Serialize( 
        const std::string& name,
        const std::string& value,
        const std::map<std::string, std::string>& attributes = {} );

private:
    // URL 编码
    static std::string Encode( const std::string& value );

    // URL解码
    static std::string Decode( const std::string& value );

    // 检查保留字符（根据RFC 3986）
    static bool IsUnreserved( char c ) { return isalnum( c ) || c == '-' || c == '_' || c == '.' || c == '~'; }

    // 去除字符串两端空格
    static void Trim( std::string& str );
};