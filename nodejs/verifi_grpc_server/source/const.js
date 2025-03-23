// 在向 redis 中插入时自动增添的前缀
const CODE_PREFIX = 'code_';

// 错误码
const EnumError =
{
    Success: 0,

    ErrorRedis: 101,    // redis 错误
    ErrorSend: 102,     // 发送 email 错误
    Exception: 103      // 其他异常
};

export default { CODE_PREFIX, EnumError };