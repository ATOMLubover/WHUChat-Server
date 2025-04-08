#include "cookie_processer.hpp"

std::map<std::string, std::string> CookieProcesser::Parse( const std::string& str_cookie )
{
    std::map<std::string, std::string> map_cookie;
    std::stringstream ss( str_cookie );
    std::string pair;
    while ( std::getline( ss, pair, ';' ) )
    {
        // 去除前后空格
        Trim( pair );
        if ( pair.empty() )
            continue;

        std::size_t pos_equal = pair.find( '=' );
        if ( pos_equal != std::string::npos )
        {
            std::string key = pair.substr( 0, pos_equal );
            std::string value = pair.substr( pos_equal + 1 );

            // key 和 value 同样去除两侧空格
            Trim( key );
            Trim( value );
            map_cookie[ Decode( key ) ] = Decode( value );
        }
    }
    return std::move( map_cookie );
}

std::string CookieProcesser::Serialize(
    const std::string& name,
    const std::string& value,
    const std::map<std::string, std::string>& attributes
)
{
    // 编码键值对
    std::string encoded_name = Encode( name );
    std::string encoded_value = Encode( value );

    // 构建基础 Cookie 字符串
    std::ostringstream cookie;
    cookie << encoded_name << "=" << encoded_value;

    // 添加属性（如 Path、Secure、HttpOnly 等）
    for ( const auto& attr : attributes )
    {
        std::string key = attr.first;
        std::transform( key.begin(), key.end(), key.begin(), ::toupper ); // 转为大写

        // 注意这里要插入一个分号一个空格
        cookie << "; ";

        if ( attr.second.empty() )
        {
            // 无值属性（如 Secure、HttpOnly）
            cookie << key;
        }
        else
        {
            // 有值属性（如 Path=/, Max-Age=3600）
            cookie << key << "=" << attr.second;
        }
    }

    return cookie.str();
}

std::string CookieProcesser::Encode( const std::string& value )
{
    std::ostringstream escaped;
    escaped.fill( '0' );
    escaped << std::hex;
    // 逐个编码每个字符
    for ( char c : value )
    {
        if ( IsUnreserved( c ) )
            escaped << c;
        else
            escaped << '%' << std::setw( 2 ) << int( static_cast< unsigned char >( c ) );
    }

    return escaped.str();
}

std::string CookieProcesser::Decode( const std::string& value )
{
    std::ostringstream decoded;
    for ( size_t i = 0; i < value.length(); ++i )
    {
        if ( value[ i ] == '%' && i + 2 < value.length() )
        {
            int hex;
            std::istringstream iss_hex( value.substr( i + 1, 2 ) );
            if ( iss_hex >> std::hex >> hex )
            {
                decoded << static_cast< char >( hex );
                i += 2;
            }
            else
            {
                decoded << value[ i ];
            }
        }
        else
        {
            decoded << value[ i ];
        }
    }
    return decoded.str();
}

void CookieProcesser::Trim( std::string& str )
{
    str.erase(
        str.begin(),
        std::find_if(
            str.begin(),
            str.end(),
            [] ( int ch ) { return !std::isspace( ch ); } ) );
    str.erase(
        std::find_if(
            str.rbegin(),
            str.rend(),
            [] ( int ch ) { return !std::isspace( ch ); } ).base(),
        str.end() );
}
