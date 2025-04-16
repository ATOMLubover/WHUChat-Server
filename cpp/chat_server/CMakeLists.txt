cmake_minimum_required( VERSION 3.10 )

# 打印当前运行 CMake 源文件的目录
message( STATUS "CMake curr dir: " ${CMAKE_CURRENT_SOURCE_DIR} )

# 项目名称宏定义
set( PROJ_NAME "chat_server" )

# 设置项目基础属性
project( ${PROJ_NAME}
    LANGUAGES C CXX
    VERSION 0.1.0
)

# 设置 C++ 标准，并限制编译器扩展
set( CMAKE_CXX_STANDARD 20 )
set( CMAKE_CXX_STANDARD_REQUIRED ON )
set( CMAKE_CXX_EXTENSIONS OFF )

# 限制 RPATH，防止 miniconda 路径干扰
set(CMAKE_BUILD_RPATH_USE_ORIGIN ON)  # 允许使用 $ORIGIN（相对路径）
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH FALSE)  # 关闭自动添加链接路径到RPATH
set(CMAKE_INSTALL_RPATH "/usr/lib/x86_64-linux-gnu")  # 仅指定系统库路径

# 设置源码集合
file( GLOB SOURCE_CODE "sourcs/**/*.cpp" "sources/*.cpp" )
# 打印源码文件宏
message( STATUS "Source code: " ${SOURCE_CODE} )

# 第三方库包
find_package( Threads REQUIRED )                        # 使用 CMake 的线程库
find_package( fmt REQUIRED )                            # fmt 字符串格式化库
set(OPENSSL_ROOT_DIR "/usr")
find_package( OpenSSL REQUIRED)                         # OpenSSL 库 
# find_package( Protobuf CONFIG REQUIRED )                # protocol buffer 库
# message( STATUS "Using protobuf ${Protobuf_VERSION}" )
# find_package( gRPC CONFIG REQUIRED )                    # gRPC 库（依赖 Protobuf）
# message( STATUS "Using gRPC ${gRPC_VERSION}" )

# 创建目标
add_executable( ${PROJ_NAME} )

set_target_properties( ${PROJ_NAME} 
    PROPERTIES
        BUILD_RPATH "/usr/lib/x86_64-linux-gnu"
        INSTALL_RPATH "/usr/lib/x86_64-linux-gnu"
)

# 设定源码
target_sources( ${PROJ_NAME}
    PRIVATE 
        ${SOURCE_CODE}
)

# 设定 include 目录
target_include_directories( ${PROJ_NAME}
    PRIVATE 
        "./sources/include/"                         # 自己编写的头文件
        "./sources/thirdparty/"                      # 第三方工具生成的代码
        "/home/lubover/code_resources/thirdparty/"   # 下载的第三方 dev 源码
        ${OPENSSL_INCLUDE_DIRS}                      # openssl
)

# 设定所需链接第三方库
target_link_libraries( ${PROJ_NAME}
    PRIVATE 
        fmt::fmt                    # fmt 字符串格式化库
        # gRPC::grpc++                # gRPC C++ 库
        # gRPC::grpc++_reflection     # gRPC C++ 反射库
        # protobuf::libprotobuf       # protobuf 库（必须在 gRPC 链接后链接）
        mysqlcppconn                # MySQL C++ 连接库
        hiredis                     # redis C 连接库
        ${OPENSSL_LIBRARIES}        # openssl 库
)

# 添加不同编译模式下的宏定义
if( CMAKE_BUILD_TYPE STREQUAL "Debug" )  
    add_definitions( -DDEBUG ) # DEBUG 宏
endif()  