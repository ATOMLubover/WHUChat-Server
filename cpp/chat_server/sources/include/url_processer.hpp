#pragma once

#include <string>
#include <sstream>
#include <map>
#include <cstdint>

// 辅助类，处理 URL 相关操作
class UrlProcesser
{
public:
    static void ParseGet( const std::string& url,
        std::string* uri,
        std::string* raw_params, std::map<std::string, std::string>* params )
    {
        auto query_pos = url.find( '?' );
        if ( query_pos == std::string::npos )
        {
            *uri = url;
            return;
        }

        *uri = url.substr( 0, query_pos );
        std::string query_string = url.substr( query_pos + 1 );
        *raw_params = query_string; // 原本的参数部分
        query_string += "&"; // add "&" to simplify the check conditions

        std::string key, val;
        std::size_t pos_equal = 0;
        std::size_t pos_amper = 0;
        std::size_t pos_last_amper = 0;
        while ( pos_amper < query_string.size() )
        {
            if ( query_string[ pos_equal ] != '=' )
                pos_equal++;

            auto Parse
                = [ params, // 传入 parans 指针进行修饰
                &key, &val, &query_string ](
                    std::size_t begin, std::size_t pos_equal, std::size_t end )
                {
                    key = UrlDecode( query_string.substr( begin, pos_equal - begin ) );
                    val = UrlDecode( query_string.substr( pos_equal + 1, end - pos_equal - 1 ) );
                    ( *params )[ key ] = val;
                };
            if ( query_string[ pos_amper ] == '&' )
            {
                Parse( pos_last_amper, pos_equal, pos_amper );

                pos_equal = pos_amper + 1;
                pos_last_amper = pos_amper + 1;
            }
            pos_amper++;
        }
    }

    static std::string UrlEncode( const std::string& raw )
    {
        std::ostringstream oss_encoded;

        for ( char ch : raw )
        {
            // only directly input alpha, num, and some common chars
            if ( std::isalnum( ch )
                || ch == '-' || ch == '_' || ch == '.' || ch == '~' )
            {
                oss_encoded << ch;
            }
            else if ( ch == ' ' )
            {
                oss_encoded << '+';
            }
            else
            {
                oss_encoded
                    << '%' << std::uppercase << std::hex
                    << static_cast< int >( static_cast< std::uint8_t >( ch ) );
            }
        }

        return oss_encoded.str();
    }
    static std::string UrlDecode( const std::string& url )
    {
        std::ostringstream oss_decoded;

        for ( std::size_t i = 0; i < url.size(); i++ )
        {
            char ch{};

            switch ( url[ i ] )
            {
                case '%':
                {
                    std::string hex = url.substr( i + 1, 2 );
                    ch = static_cast< char >( std::stoul( hex, nullptr, 16 ) );

                    i += 2; // skip two chars after '%'

                    break;
                }

                case '+':
                {
                    ch = ' ';
                    break;
                }

                default:
                {
                    ch = url[ i ];
                    break;
                }
            }

            oss_decoded << ch;
        }

        return oss_decoded.str();
    }
};