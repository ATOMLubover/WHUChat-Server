/// verifi_grpc_server main源文件 ///

const CONST_MOD = require('./const');
const CONFIG_MOD = require('./config');
const PROTO_MSG = require('./proto_resolution');
const AsyncSendMail = require('./email_service');
const redis_module = require('./redis_service');

const { v4: uuidv4 } = require('uuid'); // 生成 unique user id 的库
const grpc = require('@grpc/grpc-js');

/**
 * @param req 等于GetVerifyRequest
 * @param rsp 等于GetVerifyResponse
 */
async function GetVerifyCode(req, rsp) {
    console.log("目标email为：", req.request.email);
    try {
        // 以 “code_{email}” 作为 key 值存在 redis，返回值是对应的验证码
        let get_result
            = await redis_module.GetRedis(
                CONST_MOD.CODE_PREFIX + req.request.email);
        console.log("查询该email的结果为: ", get_result);

        let uuid;
        // 如果 get_result 为空，则说明之前没有对该邮箱发送验证码
        // 补充一个 uuid 再进行下面的步骤
        if (get_result == null) {
            // 生成一个新 uuid 到 redis 中
            uuid = uuidv4();
            if (uuid.length > 4) {
                uuid = uuid.substring(0, 4);
            }
            let expire_result
                = await redis_module.SetRedisExpire(
                    CONST_MOD.CODE_PREFIX + req.request.email,
                    uuid,
                    180
                );
            // 当设置时限失败则直接返回
            if (!expire_result) {
                rsp(
                    null,
                    {
                        email: req.request.email,
                        error: CONST_MOD.EnumError.ErrorRedis
                    }
                );
                return;
            }
        }

        console.log("unique_id is: ", uuid);
        let str_text
            = req.request.email + '的验证码为：' + uuid
            + ', 请在三分钟内完成验证';
        //发送邮件
        let mail_options = {
            from: CONFIG_MOD.EMAIL_USR,
            to: req.request.email,
            subject: 'Verification code',
            text: str_text
        };

        let send_result
            = await AsyncSendMail(mail_options);
        console.log("邮件发送结果为：", send_result);

        // 如果发送邮件失败，也直接返回
        if (!send_result) {
            rsp(
                null,
                {
                    email: req.request.email,
                    error: CONST_MOD.EnumError.ErrorSend
                }
            );
            return;
        }
        rsp(
            null,
            {
                email: req.request.email,
                error: CONST_MOD.EnumError.Success
            }
        );

    } catch (exp) {
        console.log("GetVerifyCode函数处异常：", exp);

        rsp(
            null,
            {
                email: req.request.email,
                error: CONST_MOD.EnumError.Exception
            }
        );
    }
}

function main() {
    let server = new grpc.Server();
    server.addService(
        PROTO_MSG.VerifiService.service,
        { GetVerifyCode: GetVerifyCode }
    );
    // 启动异步监听
    const ADDR_GRPC = '0.0.0.0:50051';
    server.bindAsync(
        ADDR_GRPC,
        grpc.ServerCredentials.createInsecure(),
        () => { console.log('grpc服务端已启动在：' + ADDR_GRPC) }
    );
}

// 运行入口
main();