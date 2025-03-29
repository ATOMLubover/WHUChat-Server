#pragma once

#include <functional>

// 对类似 Go 的 defer 语句的封装
// 在析构时自动执行 callback 回调函数
class Defer
{
public:
    Defer( std::function<void()> func ) : callback( func ) { }
    ~Defer() { callback(); }

private:
    std::function<void()> callback;
};