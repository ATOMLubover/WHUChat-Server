const { readFileSync } = require('fs'); // 文件读写库

console.log('curr path: ', __dirname);
const CONFIG = JSON.parse(readFileSync('./resources/config.json', 'utf8'));
console.log('config loaded: ', CONFIG);

// 用于 nodemailer 代理发送的配置
const EMAIL_USR = CONFIG.email.user;
const EMAIL_PWD = CONFIG.email.password;
// redis 配置
const REDIS_HOST = CONFIG.redis.host;
const REDIS_PORT = CONFIG.redis.port;
const REDIS_PWD = CONFIG.redis.password;

module.exports = {
    EMAIL_USR, EMAIL_PWD,
    REDIS_HOST, REDIS_PORT, REDIS_PWD
};