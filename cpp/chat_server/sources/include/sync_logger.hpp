#pragma once

#include "singleton.hpp"

#include <iostream>
#include <fstream>
#include <string>
#include <format>
#include <chrono>
#include <mutex>
#include <thread>
#include <sstream>
#include <vector>
#include <ctime>
#include <string>
#include <iomanip>
#include <memory>

// 日志记录等级
enum class LogLevel
{
    Trace = 0,
    Debug = 1,
    Info = 2,
    Warning = 3,
    Error = 4,
    Critical = 5,
};

// 保证线程安全的日志记录类
// 但是不能保证一定在调用 Log 函数时立刻打印日志
class SyncLogger
    : public Singleton<SyncLogger>
{
    friend class Singleton<SyncLogger>;

public:
    ~SyncLogger()
    {
        if ( m_ofs.is_open() )
            m_ofs.close();
    }

    // 设置记录的最低等级
    void SetMinLevel( LogLevel level )
    {
        std::lock_guard<std::mutex> lock( m_mutex );
        m_min_level = level;
    }

    // 设置输出文件
    void SetOutputFile( const std::string& filename )
    {
        std::lock_guard<std::mutex> lock( m_mutex );

        try
        {
            // 以追加方式打开文件
            m_ofs.open( filename, std::ios::app );
            if ( !m_ofs.is_open() )
                std::cerr << "Error opening log file: " << filename << std::endl;

            m_is_ofs_usable = true;
        }
        catch ( const std::exception& exp )
        {
            std::cerr
                << "SyncLogger SetOutputFile failed: " << filename
                << ", exception: " << exp.what() << std::endl;

            // 出现异常时，禁止文件输出，且关闭文件输出流
            m_is_ofs_usable = false;
            m_ofs.close();
        }
    }

    // 关闭文件输出
    void DisableFileOutput()
    {
        std::lock_guard<std::mutex> lock( m_mutex );

        if ( m_ofs.is_open() )
            m_ofs.close();
        m_is_ofs_usable = false;
    }

    template <typename... Args>
    void Log( LogLevel level, const std::string& fmt, Args&&... args )
    {
        // 当输出等级小于最小记录等级时，不输出
        if ( level < m_min_level )
            return;

        // 组合获得日志的主体内容
        std::format_args fmt_args = std::make_format_args( args... );
        std::string msg = std::vformat( fmt, fmt_args );
        // 获得当前时间
        std::string timestamp = GetCurrentTimestamp();

        // 组合生成完整日志内容
        std::string log_entry = std::format( "{} {} {}\n",
            timestamp, FormatLevel( level ), msg );
        {
            std::lock_guard<std::mutex> lock( m_mutex );

            std::cout << log_entry;
            // 如果要向文件输出，则将日志内容也写入文件
            if ( m_is_ofs_usable && m_ofs.is_open() )
                m_ofs << log_entry;
        }
    }

private:
    template <typename... Args>
    void Trace( const std::string& fmt, Args&&... args )
    {
        Log( LogLevel::Trace, fmt, std::forward<Args>( args )... );
    }

    template <typename... Args>
    void Debug( const std::string& fmt, Args&&... args )
    {
        Log( LogLevel::Debug, fmt, std::forward<Args>( args )... );
    }

    template <typename... Args>
    void Info( const std::string& fmt, Args&&... args )
    {
        Log( LogLevel::Info, fmt, std::forward<Args>( args )... );
    }

    template <typename... Args>
    void Warning( const std::string& fmt, Args&&... args )
    {
        Log( LogLevel::Warning, fmt, std::forward<Args>( args )... );
    }

    template <typename... Args>
    void Error( const std::string& fmt, Args&&... args )
    {
        Log( LogLevel::Error, fmt, std::forward<Args>( args )... );
    }

    template <typename... Args>
    void Critical( const std::string& fmt, Args&&... args )
    {
        Log( LogLevel::Critical, fmt, std::forward<Args>( args )... );
    }

    // 获得日志等级的字符串形式
    std::string FormatLevel( LogLevel Level )
    {
        switch ( Level )
        {
            case LogLevel::Trace:    return "[Trace]";
            case LogLevel::Debug:    return "[Debug]";
            case LogLevel::Info:     return "[INFO]";
            case LogLevel::Warning:  return "[Warning]";
            case LogLevel::Error:    return "[Error]";
            case LogLevel::Critical: return "[Critical]";
            default:                 return "[UNKNOWN]";
        }
    }

    // 获取当前时间的字符串形式
    std::string GetCurrentTimestamp()
    {
        // 获取当前时间
        auto now = std::chrono::system_clock::now();
        auto time_t_now = std::chrono::system_clock::to_time_t( now );

        // 转化当前时间为 tm 类型，方便格式化输出
        std::tm tm_now;
#ifdef _WIN32
        localtime_s( &tm_now, &time_t_now );
#else
        localtime_r( &time_t_now, &tm_now );
#endif

        // 最后输出时间的字符串形式
        std::stringstream ss;
        ss << "[" << std::put_time( &tm_now, "%Y-%m-%d %H:%M:%S." ) << "]";
        return ss.str();
    }

private:
    SyncLogger()
        : m_is_ofs_usable( false )
    {
#ifdef DEBUG
        m_min_level = LogLevel::Trace;
#else
        m_min_level = LogLevel::Warning;
#endif
    }
    // 禁用拷贝构造和拷贝赋值
    SyncLogger( const SyncLogger& ) = delete;
    SyncLogger& operator=( const SyncLogger& ) = delete;

private:
    LogLevel m_min_level;

    bool m_is_std_usable;

    std::ofstream m_ofs;
    bool m_is_ofs_usable;

    std::mutex m_mutex;
};