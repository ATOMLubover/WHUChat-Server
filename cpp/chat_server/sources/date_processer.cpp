#include "date_processer.hpp"

#include <chrono>
#include <sstream>

std::time_t DateProcesser::ParseTimestamp( const std::string& str_timestamp )
{
    struct tm tm = {};
    if ( sscanf( str_timestamp.c_str(), "%d-%d-%d %d:%d:%d",
        &tm.tm_year, &tm.tm_mon, &tm.tm_mday,
        &tm.tm_hour, &tm.tm_min, &tm.tm_sec ) != 6 )
    {
        return -1; // 格式错误
    }
    tm.tm_year -= 1900;         // tm_year 是自 1900 的偏移
    tm.tm_mon -= 1;             // tm_mon 范围为 0-11
    tm.tm_isdst = -1;           // 自动判断夏令时
    return mktime( &tm );   // 转换为本地时间戳
}

bool DateProcesser::CheckWithinDays( const std::string& str_timestamp, int days )
{
    time_t timestamp = ParseTimestamp( str_timestamp );
    if ( timestamp == -1 )
        return false;

    // 获取当前时间戳（UTC 或本地时间，需与数据库一致）
    auto now = std::chrono::system_clock::now();
    time_t now_time = std::chrono::system_clock::to_time_t( now );

    // 计算差值（秒）
    int limit_diff_sec = days * 24 * 3600;
    double diff_seconds = difftime( now_time, timestamp );

    return ( diff_seconds >= 0 && diff_seconds <= limit_diff_sec );
}
