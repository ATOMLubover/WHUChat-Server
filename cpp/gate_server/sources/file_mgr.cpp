#include "include/file_mgr.hpp"

#include <fmt/core.h>

#include <fstream>
#include <iostream>
#include <stdexcept>
#include <sstream>

///////////////////////////
///// FileRes 实现部分 /////
///////////////////////////

FileRes::FileRes( const std::string& file_path, EnumFileType type )
    : path( file_path )
    , file_type( type )
{
    // 根据文件类型确定打开方式
    std::ifstream ifs_file;
    switch ( file_type )
    {
        case EnumFileType::Text: ifs_file.open( file_path ); break;
        case EnumFileType::Binary: ifs_file.open( file_path, std::ios::binary ); break;
    }
    if ( !ifs_file.is_open() )
    {
        std::cout << "文件打开失败：" << file_path << std::endl;
        throw std::runtime_error( fmt::format( "文件打开失败：{}", file_path ) );
        return;
    }

    file_size = GetFileSizeHelper( ifs_file );
    // 初步根据文件类型来决定如何存储（函数内再根据大小决定具体的存储方式）
    switch ( file_type )
    {
        case EnumFileType::Text: InitTextFile( ifs_file ); break;
        case EnumFileType::Binary: InitBinaryFile( ifs_file );break;
    }

    ifs_file.close();

    std::cout << "文件构建完毕：" << file_path
        << "，大小：" << file_size << std::endl;
}

std::int32_t FileRes::GetFileSizeHelper( std::ifstream& ifs_file )
{
    ifs_file.seekg( 0, std::ios::end );
    std::streamsize sz_file = ifs_file.tellg(); // 字节为单位的文件大小
    ifs_file.seekg( 0, std::ios::beg ); // 必须要将读取位置放回开头，否则接下来无法读取
    return static_cast< std::int64_t >( sz_file );
}

FileRes::FileRes( FileRes&& rhs )
{
    *this = std::move( rhs );
}

FileRes& FileRes::operator=( FileRes&& rhs )
{
    path = std::move( rhs.path );
    file_size = std::move( rhs.file_size );
    file_type = std::move( rhs.file_type );
    str_text = std::move( rhs.str_text );
    vec_binary = std::move( rhs.vec_binary );

    return *this;
}

const std::unique_ptr<std::string>& FileRes::GetText()
{
    if ( file_type != EnumFileType::Text )
    {
        str_text.reset( nullptr );
        return str_text;
    }

    return str_text;
}

const std::unique_ptr<std::vector<char>>& FileRes::GetBinary()
{
    if ( file_type != EnumFileType::Binary )
    {
        vec_binary.reset( nullptr );
        return vec_binary;
    }

    return vec_binary;
}

void FileRes::InitTextFile( std::ifstream& ifs )
{
    str_text = std::make_unique<std::string>();

    // 判断文件大小
    if ( file_size <= NORMAL_SEND_SIZE * BYTES_PER_MB ) // 当文件小于等于 5MB，直接存入 std::string
    {
        // std::string line;
        // while ( std::getline( ifs, line ) )
        //     std::cout << line << std::endl;

        std::stringstream ss_file;
        ss_file << ifs.rdbuf();
        str_text = std::make_unique<std::string>( ss_file.str() );

        return;
    }
    // TODO: <= 500MB 的逻辑暂时搁置
}

void FileRes::InitBinaryFile( std::ifstream& ifs )
{
    vec_binary = std::make_unique<std::vector<char>>();

    // 判断文件大小
    if ( file_size <= NORMAL_SEND_SIZE * BYTES_PER_MB ) // 当文件小于等于 5MB，直接存入 vector
    {
        vec_binary->resize( file_size ); // 预分配内存
        ifs.read( vec_binary->data(), file_size );

        return;
    }
    // TODO: <= 500MB 的逻辑暂时搁置
}

///////////////////////////
///// FileMgr 实现部分 /////
///////////////////////////

FileMgr::~FileMgr()
{
    file_list.clear();

    std::cout << "FileMgr被析构" << std::endl;
}

const std::unique_ptr<std::string>& FileMgr::GetTextFileContent( const std::string& file_path )
{
    std::lock_guard<std::mutex> guard( mtx_filelist );

    for ( auto iter = file_list.begin(); iter != file_list.end(); ++iter )
    {
        // 若能找到，则先将该 FileRes 转移到头部
        if ( ( *iter )->GetPath() == file_path )
        {
            file_list.splice( file_list.begin(), file_list, iter );

            // 最后返回位于头部的 file
            return ( *file_list.front() ).GetText();
        }
    }
    // 如果没有找到，先查看是否缓存文件过多
    if ( file_list.size() >= FILE_ABANDON_CNT )
        file_list.pop_back(); // 弹出最后一个（离上次使用最远的）
    // 然后插入新文件到头部
    std::unique_ptr<FileRes> new_file( new FileRes( file_path, FileRes::EnumFileType::Text ) );
    file_list.push_front( std::move( new_file ) );

    return ( *file_list.front() ).GetText();
}

const std::unique_ptr<std::vector<char>>& FileMgr::GetBinFileContent( const std::string& file_path )
{
    std::lock_guard<std::mutex> guard( mtx_filelist );

    for ( auto iter = file_list.begin(); iter != file_list.end(); ++iter )
    {
        // 若能找到，则先将该 FileRes 转移到头部
        if ( ( *iter )->GetPath() == file_path )
        {
            file_list.splice( file_list.begin(), file_list, iter );

            // 最后返回位于头部的 file
            return ( *file_list.front() ).GetBinary();
        }
    }
    // 如果没有找到，先查看是否缓存文件过多
    if ( file_list.size() >= FILE_ABANDON_CNT )
        file_list.pop_back(); // 弹出最后一个（离上次使用最远的）
    // 然后插入新文件到头部
    std::unique_ptr<FileRes> new_file( new FileRes( file_path, FileRes::EnumFileType::Binary ) );
    file_list.push_front( std::move( new_file ) );

    return ( *file_list.front() ).GetBinary();
}

FileMgr::FileMgr()
{
    std::cout << "FileMgr构造" << std::endl;
}

/* 暂时换用新实现

FileRes::FileRes( const std::string& file_path )
    : path( file_path )
    // , cnt_form_lastuse( 0 )
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
    ifs.close();
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
    deq_files.clear();

    std::cout << "FileMgr被析构" << std::endl;
}

const std::stringstream& FileMgr::GetFileContent( const std::string& file_path )
{
    auto iter_file = deq_files.find( file_path );

    // 文件不存在则加载文件
    if ( iter_file == deq_files.end() )
    {
        std::unique_ptr<FileRes> file_res = std::make_unique<FileRes>( file_path );
        deq_files.emplace( file_path, std::move( file_res ) );

        std::cout << "加载文件：" << file_path << std::endl;
    }
    // 处理完后获得文件资源（其实这里也是为了刷新对应文件资源的未使用计数器，否则接下来会被删除）
    const std::stringstream& file_content = *( deq_files[ file_path ]->GetContent() );

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
    for ( auto& file : deq_files )
        file.second->UpdateCnt();

    // 遍历文件资源，释放未使用计数器超过 FILE_ABANDON_CNT 的文件资源
    for ( auto iter = deq_files.begin(); iter != deq_files.end(); )
    {
        if ( iter->second->GetCnt() > FILE_ABANDON_CNT )
        {
            std::cout << "释放过期文件资源：" << iter->first << std::endl;
            iter = deq_files.erase( iter );
        }
        else
            ++iter;
    }
}

*/