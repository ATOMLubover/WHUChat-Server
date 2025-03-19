#include "FileMgr.hpp"

#include <fmt/core.h>

#include <fstream>
#include <iostream>
#include <stdexcept>

FileRes::FileRes( const std::string& file_path )
    : path( file_path )
    , cnt_form_lastuse( 0 )
{
    strs_file = std::make_unique<std::stringstream>(); // 为 strs_file 分配内存
    std::ifstream ifs( file_path );
    if ( !ifs.is_open() )
    {
        std::cout << "文件打开失败：" << file_path << std::endl;
        throw std::runtime_error( fmt::format( "文件打开失败：{}", file_path ) );
        return;
    }
    *strs_file << ifs.rdbuf(); // 将文件内容读入 strs_file
}

FileRes::FileRes( FileRes&& rhs ) noexcept
{
    path = std::move( rhs.path );
    strs_file = std::move( rhs.strs_file );
}

const std::unique_ptr<std::stringstream>& FileRes::GetContent()
{
    cnt_form_lastuse = 0; // 刷新未使用计数器
    return strs_file;
}

FileMgr::~FileMgr()
{
    files.clear();

    std::cout << "FileMgr被析构" << std::endl;
}

const std::stringstream& FileMgr::GetFileContent( const std::string& file_path )
{
    auto iter_file = files.find( file_path );

    // 文件不存在则加载文件
    if ( iter_file == files.end() )
    {
        std::unique_ptr<FileRes> file_res = std::make_unique<FileRes>( file_path );
        files.emplace( file_path, std::move( file_res ) );

        std::cout << "加载文件：" << file_path << std::endl;
    }
    // 处理完后获得文件资源（其实这里也是为了刷新对应文件资源的未使用计数器，否则接下来会被删除）
    const std::stringstream& file_content = *( files[ file_path ]->GetContent() );

    // 更新 FileRes 的未使用计数器，清理过期的文件资源
    AbandonExpiredFilesHelper();

    return file_content;
}

FileMgr::FileMgr()
{
    std::cout << "FileMgr构造" << std::endl;
}

void FileMgr::AbandonExpiredFilesHelper()
{
    // 先更新文件资源的未使用计数器
    for ( auto& file : files )
        file.second->UpdateCnt();

    // 遍历文件资源，释放未使用计数器超过 FILE_ABANDON_CNT 的文件资源
    for ( auto iter = files.begin(); iter != files.end(); )
    {
        if ( iter->second->GetCnt() > FILE_ABANDON_CNT )
        {
            std::cout << "释放过期文件资源：" << iter->first << std::endl;
            iter = files.erase( iter );
        }
        else
            ++iter;
    }
}
