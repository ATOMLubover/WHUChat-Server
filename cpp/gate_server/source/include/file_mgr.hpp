#pragma once

#include "singleton.hpp"

#include <string>
#include <memory>
#include <cstdint>
#include <vector>
#include <cstddef>
#include <list>
#include <mutex>

// 封装文件资源类
// 支持文本或者二进制文件
class FileRes
{
public:
    enum class EnumFileType
    {
        Text,    // 文本文件
        Binary   // 二进制文件
    };

public:
    FileRes( const std::string& file_path, EnumFileType type = EnumFileType::Text );
    // 去除拷贝构造函数
    FileRes( const FileRes& ) = delete;
    FileRes& operator=( const FileRes& ) = delete;
    // 实现移动构造函数
    FileRes( FileRes&& rhs );
    FileRes& operator=( FileRes&& rhs );
    ~FileRes() = default;

    // 返回被构造的时候的文件路径
    const std::string GetPath() const { return path; }
    // 返回文件大小（字节为单位）
    std::int32_t GetFileSize() const { return file_size; }
    // 返回文件类型
    EnumFileType GetFileType() const { return file_type; }

    // 获取文本类型文件内容（type 不匹配时会刷新对应的文件内容指针）
    const std::unique_ptr<std::string>& GetText();
    // 获取二进制类型文件内容（type 不匹配时会刷新对应的文件内容指针）
    const std::unique_ptr<std::vector<char>>& GetBinary();

private:
    std::int32_t GetFileSizeHelper( std::ifstream& ifs_file );

    // 如果是文本文件，调用此函数初始化
    void InitTextFile( std::ifstream& ifs );
    // 如果是二进制文件，调用此函数初始化
    void InitBinaryFile( std::ifstream& ifs );

private:
    std::string path;                                    // 地址（绝对 or 相对）

    const std::int32_t BYTES_PER_MB = 1024 * 1024;
    const int NORMAL_SEND_SIZE = 5;                      // 能够直接发送的最大大小
    const int CHUNKED_SEND_SIZE = 200;                   // 能够分块传输的最大大小
    std::int32_t file_size;                              // 以字节作为单位的文件大小
    EnumFileType file_type;                              // 文件类型

    std::unique_ptr<std::string> str_text;               // 文本文件以这个形式存储
    std::unique_ptr<std::vector<char>> vec_binary;       // 二进制文件已这个形式存储
};

// 用于管理文件资源的加载和释放（内置锁）
// 获取文件资源内容的唯一方式
class FileMgr
    : public Singleton<FileMgr>
{
    friend class Singleton<FileMgr>;

public:
    ~FileMgr();
    FileMgr( const FileMgr& ) = delete;
    FileMgr& operator=( const FileMgr& ) = delete;

    // 返回含有文本内容的 std::string
    const std::unique_ptr<std::string>& GetTextFileContent( const std::string& file_path );
    // 返回含有二进制内容的 std::vecor<char>>
    const std::unique_ptr<std::vector<char>>& GetBinFileContent( const std::string& file_path );

private:
    FileMgr();

private:
    const int FILE_ABANDON_CNT = 5;                     // 保证持有的文件资源不超过 FILE_ABANDON_CNT 个
    std::list<std::unique_ptr<FileRes>> file_list;
    std::mutex mtx_filelist;
};

/* 暂时换用新实现

// 封装文件资源类
class FileRes
{
public:
    FileRes( const std::string& file_path );
    // 去除拷贝构造函数
    FileRes( const FileRes& ) = delete;
    FileRes& operator=( const FileRes& ) = delete;
    // 实现移动构造函数
    FileRes( FileRes&& rhs ) noexcept;
    FileRes& operator=( FileRes&& rhs ) noexcept;
    ~FileRes() = default;

    const std::string GetPath() const { return path; }
    // 会隐式地刷新未使用计数器为 0
    const std::unique_ptr<std::stringstream>& GetContent();

    // 更新一次文件计数器，以便Mgr检查是否需要删除他
    void UpdateCnt() { cnt_form_lastuse++; }
    // 获取文件计数器，以便 Mgr 检查是否需要删除他
    std::int16_t GetCnt() const { return cnt_form_lastuse; }

private:
    std::string path;
    std::unique_ptr<std::stringstream> strs_file;

    std::int16_t cnt_form_lastuse; // 距离上次使用的计数
};

// 用于管理文件资源的加载和释放
// 获取文件资源内容的唯一方式是通过 GetFileContent
class FileMgr
    : public Singleton<FileMgr>
{
    friend class Singleton<FileMgr>;

public:
    ~FileMgr();
    FileMgr( const FileMgr& ) = delete;
    FileMgr& operator=( const FileMgr& ) = delete;

    // 返回含有文件内容的 stringstream
    const std::stringstream& GetFileContent( const std::string& file_path );

private:
    FileMgr();

    // 会隐式地刷新文件资源的未使用计数器，然后再进行清理
    void AbandonExpiredFilesHelper();

private:
    const int FILE_ABANDON_CNT = 5; // 文件未使用计数器达到该值则释放文件资源，可以保证持有的文件资源不超过 FILE_ABANDON_CNT 个
    std::unordered_map<std::string, std::unique_ptr<FileRes>> files;
};

*/