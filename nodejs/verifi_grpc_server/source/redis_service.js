import CONFIG_MOD from './config';
import { Redis as redis } from 'ioredis' // 包含 redis 的客户端

/**
 * 根据 key 值获取 value
 * @param {*} key 键值
 */
async function GetRedis(key) {
    try {
        const result = await REDIS_CLI.get(key);
        // 如果未找到 key
        if (result === null) {
            console.log('结果： ', '<' + result + '>', ' key未找到')
            return null;
        }

        // 找到 key 则返回对应的 value
        console.log('结果： ', '<' + result + '>', ' key GET成功');
        return result;

    } catch (exp) {
        console.log('GetRedis函数处异常：', exp);
        return null;
    }
}

/**
 * 根据 key 查询 redis 中是否存在 key
 * @param {*} key 
 */
async function QueryRedis(key) {
    try {
        const result = await REDIS_CLI.exists(key);
        //  判断该值是否为空 如果为空返回 null
        if (result === 0) {
            console.log('结果： ', '<' + result + '>', ' key不存在');
            return null;
        }

        // 如果找到则返回 result
        console.log('结果： ', '<' + result + '>', ' key查找成功');
        return result;

    } catch (exp) {
        console.log('QueryRedis函数处异常：', exp);
        return null;
    }

}

/**
 * 设置 key 和 value，并设置过期时间
 * @param {*} key 
 * @param {*} value 
 * @param {*} exptime 以秒为单位的过期时间
 */
async function SetRedisExpire(key, value, exptime) {
    try {
        // 设置键和值
        await REDIS_CLI.set(key, value);
        // 设置过期时间（以秒为单位）
        await REDIS_CLI.expire(key, exptime);

        return true;

    } catch (exp) {
        console.log('SetRedisExpire函数处异常：', exp);
        return false;
    }
}

/**
 * 退出函数
 */
function Quit() {
    console.log("redis客户端退出");
    REDIS_CLI.quit();
}

// 创建 redis 客户端
const REDIS_CLI = new redis(
    {
        host: CONFIG_MOD.REDIS_HOST,
        port: CONFIG_MOD.REDIS_PORT,
        password: CONFIG_MOD.REDIS_PWD
    }
);
// 启动 redis 客户端
REDIS_CLI.on(
    "error",
    (err) => {
        console.log("无法连接到redis服务端");
        REDIS_CLI.quit();
    }
);

module.exports = { GetRedis, QueryRedis, Quit, SetRedisExpire, }