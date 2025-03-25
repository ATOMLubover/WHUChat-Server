document.addEventListener('DOMContentLoaded', function () {
    const form = document.getElementById('loginForm');
    const emailError = document.getElementById('emailError');
    const passwordError = document.getElementById('passwordError');
    const captchaError = document.getElementById('captchaError');
    const captchaSent = document.getElementById('captchaSent');
    const sendCaptchaBtn = document.getElementById('sendCaptcha');

    // 发送验证码按钮点击事件
    sendCaptchaBtn.addEventListener('click', function () {
        const email = document.getElementById('email').value;
        const emailRegex = /^[^\s@]+@[^\s@]+\.[^\s@]+$/;

        if (!emailRegex.test(email)) {
            emailError.style.display = 'block';
            return;
        }

        // 禁用按钮防止重复点击
        sendCaptchaBtn.disabled = true;
        sendCaptchaBtn.textContent = '发送中...';

        // 准备请求数据
        const requestData = {
            email: email
        };

        // 发送POST请求到后端
        fetch('/login-test/post_verification', {  // 替换为你的实际API端点
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
            },
            body: JSON.stringify(requestData)
        })
            .then(response => {
                if (!response.ok) {
                    throw new Error('网络响应不正常');
                }
                return response.json();
            })
            .then(data => {
                // 显示发送成功消息
                captchaSent.style.display = 'block';
                setTimeout(() => {
                    captchaSent.style.display = 'none';
                }, 3000);

                // 开始倒计时
                startCountdown(60);
            })
            .catch(error => {
                console.error('发送验证码错误:', error);
                alert('发送验证码失败，请稍后重试');
            })
            .finally(() => {
                // 重新启用按钮
                sendCaptchaBtn.disabled = false;
                sendCaptchaBtn.textContent = '发送验证码';
            });
    });

    // 倒计时函数
    function startCountdown(seconds) {
        let remaining = seconds;
        sendCaptchaBtn.disabled = true;

        const timer = setInterval(() => {
            sendCaptchaBtn.textContent = `重新发送(${remaining}s)`;
            remaining--;

            if (remaining < 0) {
                clearInterval(timer);
                sendCaptchaBtn.disabled = false;
                sendCaptchaBtn.textContent = '发送验证码';
            }
        }, 1000);
    }

    // 表单提交事件
    form.addEventListener('submit', function (e) {
        e.preventDefault();
        let isValid = true;

        // 验证邮箱
        const email = document.getElementById('email').value;
        const emailRegex = /^[^\s@]+@[^\s@]+\.[^\s@]+$/;
        if (!emailRegex.test(email)) {
            emailError.style.display = 'block';
            isValid = false;
        } else {
            emailError.style.display = 'none';
        }

        // 验证密码
        const password = document.getElementById('password').value;
        if (password.length < 6) {
            passwordError.style.display = 'block';
            isValid = false;
        } else {
            passwordError.style.display = 'none';
        }

        // 验证验证码
        const captcha = document.getElementById('captchaInput').value;
        if (!/^\d{4}$/.test(captcha)) {
            captchaError.style.display = 'block';
            isValid = false;
        } else {
            captchaError.style.display = 'none';
        }

        // 如果全部验证通过
        if (isValid) {
            // 跳转到成功页面
            // TODO: 暂时删除了跳转逻辑
            // window.location.href = 'login-success.html';
            console.log('登录成功');
        }
    });

    // 实时验证输入
    document.getElementById('email').addEventListener('input', function () {
        emailError.style.display = 'none';
    });

    document.getElementById('password').addEventListener('input', function () {
        if (this.value.length >= 6) {
            passwordError.style.display = 'none';
        }
    });

    document.getElementById('captchaInput').addEventListener('input', function () {
        if (/^\d{4}$/.test(this.value)) {
            captchaError.style.display = 'none';
        }
    });
});