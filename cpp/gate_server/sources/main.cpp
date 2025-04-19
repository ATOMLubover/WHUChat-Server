#include "gate_server.hpp"

#include <openssl/opensslv.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include <iostream>

// 在您的程序入口点（例如 main 函数的开始）调用此函数
void InitializeOpenssl()
{
    // OPENSSL_init_ssl 是 OpenSSL 1.1.0+ 推荐的初始化函数
    // 它会处理大部分必要的初始化步骤。
    // 参数 flags 指定了需要初始化的组件：
    // OPENSSL_INIT_LOAD_SSL_STRINGS: 加载 SSL 相关的错误信息字符串
    // OPENSSL_INIT_LOAD_CRYPTO_STRINGS: 加载加密库相关的错误信息字符串
    // OPENSSL_INIT_ADD_ALL_CIPHERS: （可选，但推荐）添加所有可用的密码套件和摘要算法
    // OPENSSL_INIT_ADD_ALL_DIGESTS: （可选，但推荐）添加所有可用的摘要算法

    SSL_load_error_strings();
    SSL_library_init();
    OpenSSL_add_all_algorithms();

    unsigned long init_flags = OPENSSL_INIT_LOAD_SSL_STRINGS |
        OPENSSL_INIT_LOAD_CRYPTO_STRINGS |
        OPENSSL_INIT_ADD_ALL_CIPHERS |
        OPENSSL_INIT_ADD_ALL_DIGESTS;

    if ( OPENSSL_init_ssl( init_flags, NULL ) == 0 )
    {
        // 初始化失败，通常这是一个严重错误
        fprintf( stderr, "OpenSSL initialization failed!\n" );
        // 打印 OpenSSL 错误堆栈，帮助诊断问题
        ERR_print_errors_fp( stderr );
        // 根据您的应用程序，可以选择退出或抛出异常
        exit( EXIT_FAILURE );
    }

    // 如果您需要在多线程环境中使用 OpenSSL，还需要配置线程回调函数。
    // 但基本的 SSL_CTX_new 崩溃问题通常是由于缺少上述基本初始化引起的。
    // 线程安全配置代码通常在 OPENSSL_init_ssl 调用之后。
    // 具体配置方法请参考 OpenSSL 文档中关于线程安全的部分 (如 OPENSSL_threaded_mode())
}

int main( int argc, char* argv[] )
{
    try
    {
        InitializeOpenssl();
        auto test = ::TLS_method();
        auto i = ::SSL_CTX_new( test );


        printf( "test\n" );
        std::make_shared<GateServer>()->Run();
    }
    catch ( const std::exception& exp )
    {
        std::cout << "main函数调用gate_server处产生异常：" << exp.what() << std::endl;
    }

    return 0;
}