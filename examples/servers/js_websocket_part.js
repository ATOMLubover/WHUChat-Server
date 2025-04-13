const WebSocket = require('ws');

// 配置参数
const uuid = '1';
const token = '2518253c-0f38-4c68-8266-a8e0018ed4a6';
const sessionId = '8';

// 创建 WebSocket 连接时添加 Cookie 头
const ws = new WebSocket(
    `ws://127.0.0.1:8081/api/v1/ws/trans_ans?session_id=${sessionId}&model_id=1&uuid=1`, // session_id 保留在 URL
    {
        headers: {
            // 将 uuid 和 token 写入 Cookie
            Cookie: `uuid=${uuid}; token=${token}`
        }
    }
);

// 连接成功时触发
ws.on('open', function open() {
    console.log('Connected to server');

    // 发送一条消息到服务器
    ws.send('Hello, WebSocket Server!');
});

// 收到消息时触发
ws.on('message', function incoming(message) {
    // 判断是否为 Buffer
    if (Buffer.isBuffer(message)) {
        // 转换为 UTF-8 字符串
        const text = message.toString('utf8');
        console.log('Received as text:', text);
    } else {
        console.log('Received:', message);
    }
});

// 连接关闭时触发
ws.on('close', function close() {
    console.log('Disconnected from server');
});

// 发生错误时触发
ws.on('error', function error(err) {
    console.error('WebSocket encountered an error:', err);
});

const readline = require('readline');
const rl = readline.createInterface({
    input: process.stdin,
    output: process.stdout
});

rl.on('line', (input) => {
    const message = input.trim();
    if (message) {
        ws.send(message);
        // console.log(`Sent to server: ${message}`);
    }
});