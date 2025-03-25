const { join } = require('path'); // 文件路径库
const { loadPackageDefinition } = require('@grpc/grpc-js'); // grpc-js 通讯库
const { loadSync } = require('@grpc/proto-loader'); // grpc 预处理 proto 库

// message.proto 所在路径
const PROTO_PATH = join(__dirname, '../resources/message.proto');

// 首先同步地加载 proto 成为一个 js 对象中
const PACKAGE_DEF = loadSync(
    PROTO_PATH,
    {
        keepCase: true,     // 保持原变量命名的形式
        longs: String,
        enums: String,
        defaults: true,     // 保持原变量的默认值
        oneofs: true
    }
);

// 然后再加载成 grpc 能够使用的 js 对象
const PROTO_DESC = loadPackageDefinition(PACKAGE_DEF);
// 最后生成供收发使用的协议对象
module.exports = PROTO_DESC.message;