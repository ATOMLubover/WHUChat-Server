#pragma once

#include "singleton.hpp"

#include <string>
#include <sstream>
#include <memory>
#include <cstdint>
#include <unordered_map>

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