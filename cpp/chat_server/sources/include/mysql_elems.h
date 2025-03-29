#pragma once

#include <string>

// 用于 conversation 和 model 表中 “status” 列的内容
enum class EnumStatus
{
    Active,
    Archieved
};

// conversation 表的元素
struct MySqlCnvElem
{
    int uid;
    int model_id;
    std::string title;
    EnumStatus status;
};

// model 表的元素
struct MySqlModelsElem
{
    std::string name;
    std::string disc;
    std::string api_key;
};

enum class EnumSender
{
    User,
    Assistant,
    System
};

// messages 表的元素
struct MySqlMessagesElem
{
    int cnv_id;
    EnumSender sender;
    std::string content;
};
