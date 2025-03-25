const CONFIG_MOD = require('./config');
const nodemailer = require('nodemailer'); // nodemailer 代理邮件发送库

// 创建发送邮件的代理
let transporter = nodemailer.createTransport(
    {
        host: 'smtp.163.com',
        port: 465,
        secure: true,
        auth: {
            user: CONFIG_MOD.EMAIL_USR, // 发送方邮箱地址（我自己的邮箱）
            pass: CONFIG_MOD.EMAIL_PWD // 邮箱代理授权码
        }
    }
);

/**
 * 协程异步的发送邮件的函数
 * @param {*} mail_options 发送邮件的参数
 * @returns {Promise} 返回一个 Promise 对象
 */
function AyncSendMail(mail_options) {
    // 封装为同步地处理发送邮件（通过Promise的阻塞）
    return new Promise(
        (resolve, reject) => {
            // transporter 本身是异步 sendMail，使用回调函数处理发送结果
            transporter.sendMail(
                mail_options,
                // 当 transporter 成功异步发送邮件后触发该回调函数
                (error, info) => {
                    // 当 sendMail 出错时调用协程的 reject 回调
                    if (error) {
                        console.log(error);
                        reject(error);
                        return;
                    }

                    // 当 sendMail 正常时调用协程的 resolve 回调
                    console.log('Mail sent successfully：' + info.response);
                    resolve(info.response);
                }
            )
        }
    );
}

module.exports = AyncSendMail;