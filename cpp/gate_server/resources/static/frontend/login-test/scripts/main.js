document.addEventListener('DOMContentLoaded', function () {
    const form = document.getElementById('loginForm');
    const emailError = document.getElementById('emailError');
    const passwordError = document.getElementById('passwordError');
    const captchaError = document.getElementById('captchaError');
    const captchaSent = document.getElementById('captchaSent');

    // 发送验证码按钮点击事件
    document.getElementById('sendCaptcha').addEventListener('click', function () {
        const email = document.getElementById('email').value;
        const emailRegex = /^[^\s@]+@[^\s@]+\.[^\s@]+$/;

        if (emailRegex.test(email)) {
            // 这里可以添加实际发送验证码的逻辑
            captchaSent.style.display = 'block';
            setTimeout(() => {
                captchaSent.style.display = 'none';
            }, 3000);
        } else {
            emailError.style.display = 'block';
        }
    });

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