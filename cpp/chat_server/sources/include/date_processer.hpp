#pragma once

#include <string>
#include <ctime>

// 辅助类，解析字符串形式的日期
class DateProcesser
{
public:
    // 检查输入时间与当前时间是否差距在 days 天内
    static bool CheckWithinDays( const std::string& timestamp, int days );

private:
    // 转换字符串形式的日期到时间戳，转换失败返回 -1
    static std::time_t ParseTimestamp( const std::string& str_date );
};