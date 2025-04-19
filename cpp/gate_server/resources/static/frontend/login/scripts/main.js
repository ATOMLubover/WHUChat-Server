document.addEventListener('DOMContentLoaded', function () {
    console.log('DOM 内容已完全加载。');

    const form = document.getElementById('loginForm');
    const emailInput = document.getElementById('email');
    const passwordInput = document.getElementById('password'); // 修复了这里的 document('password') 错误
    // Removed captchaInput
    const emailError = document.getElementById('emailError');
    const passwordError = document.getElementById('passwordError');
    // Removed captchaError
    const loginError = document.getElementById('loginError'); // General login error div
    // Removed captchaSent
    // Removed sendCaptchaBtn
    const submitBtn = document.getElementById('submitBtn'); // Get submit button

    if (!form) console.error('错误：未找到 ID 为 loginForm 的表单元素！');
    if (!emailInput) console.error('错误：未找到 ID 为 email 的输入框！');
    if (!passwordInput) console.error('错误：未找到 ID 为 password 的输入框！');
    if (!emailError) console.error('错误：未找到 ID 为 emailError 的错误提示元素！');
    if (!passwordError) console.error('错误：未找到 ID 为 passwordError 的错误提示元素！');
    if (!loginError) console.error('错误：未找到 ID 为 loginError 的错误提示元素！');
    if (!submitBtn) console.error('错误：未找到 ID 为 submitBtn 的按钮元素！');

    // --- Send Captcha Logic (REMOVED) ---
    // The entire block for sendCaptchaBtn.addEventListener was here

    // --- Countdown Function (REMOVED) ---
    // The startCountdown function was here

    // --- Form Submit Logic (MODIFIED - Captcha removed, using HTTPS) ---
    if (form) { // 确保表单元素存在再添加事件监听器
        form.addEventListener('submit', function (e) {
            console.log('捕捉到表单提交事件。');
            e.preventDefault(); // Prevent default form submission
            console.log('已阻止默认表单提交行为。');

            let isValid = true;

            // Clear previous errors
            emailError.style.display = 'none';
            passwordError.style.display = 'none';
            // Removed captchaError clearing
            loginError.style.display = 'none'; // Clear previous login errors
            console.log('已清除之前的错误信息显示。');

            // --- Client-Side Validation ---
            const email = emailInput.value;
            const emailRegex = /^[^\s@]+@[^\s@]+\.[^\s@]+$/;
            console.log(`客户端验证 - 邮箱: "${email}", 密码: "${passwordInput.value}"`);

            if (!emailRegex.test(email)) {
                console.log('客户端验证失败：邮箱格式不正确。');
                emailError.style.display = 'block';
                isValid = false;
            } else {
                console.log('客户端验证：邮箱格式正确。');
            }

            const password = passwordInput.value;
            // You might want to adjust this length check or add other password rules
            if (password.length < 6) {
                console.log('客户端验证失败：密码长度小于6位。');
                passwordError.style.display = 'block';
                isValid = false;
            } else {
                console.log('客户端验证：密码长度符合要求。');
            }

            // Removed Captcha Validation block

            console.log(`客户端验证最终结果: isValid = ${isValid}`);

            // --- If Client-Side Validation Passes, Send Login Request ---
            if (isValid) {
                console.log('客户端验证通过，准备发送登录请求。');
                submitBtn.disabled = true; // Disable button during request
                submitBtn.textContent = '登录中...';
                console.log('已禁用登录按钮，文本更新为 "登录中..."');

                const loginData = {
                    email: email,
                    password: password
                    // Removed captcha field
                };

                const loginUrl = 'https://localhost:8080/api/v1/gate/login'; // 目标登录接口 URL
                console.log(`发送 Fetch POST 请求到: ${loginUrl}`);
                console.log('请求体数据:', JSON.stringify(loginData));

                fetch(loginUrl, {
                    method: 'POST',
                    headers: {
                        'Content-Type': 'application/json',
                    },
                    body: JSON.stringify(loginData)
                })
                    .then(async response => { // *** 注意：这里使用了 async ***
                        console.log('收到服务器响应。');
                        console.log(`响应 HTTP 状态码: ${response.status}, response.ok: ${response.ok}`);

                        if (!response.ok) {
                            console.log('响应状态码非成功 (response.ok 为 false)，尝试读取响应体。');
                            // *** 修改点：先读取为文本 ***
                            const errorBodyText = await response.text(); // 只调用一次读取方法
                            console.log('读取到响应体文本:', errorBodyText);

                            let errorData = { message: `服务器返回非成功状态码 ${response.status}` }; // 默认错误信息

                            try {
                                // 尝试将读取到的文本解析为 JSON
                                const parsed = JSON.parse(errorBodyText);
                                console.log('尝试将响应体文本解析为 JSON 成功:', parsed);
                                // 如果解析成功，并且解析结果是对象，使用其中的 message 字段（如果存在）
                                if (parsed && typeof parsed === 'object') {
                                    errorData = parsed;
                                }
                            } catch (parseError) {
                                console.error('将响应体文本解析为 JSON 失败:', parseError);
                                // 如果解析失败，使用默认的错误信息，并在信息中包含部分原始文本
                                errorData.message += `. 响应体非 JSON 或解析错误。原始文本(部分): "${errorBodyText.substring(0, 200)}..."`;
                            }

                            // 抛出一个新的 Error，它会被后续的 .catch() 捕获
                            // 错误信息优先使用解析出的 JSON 中的 message 字段，否则使用默认信息
                            throw new Error(errorData.message || `未知服务器错误 (${response.status})`);
                        }

                        console.log('响应状态码成功 (response.ok 为 true)，解析响应体为 JSON。');
                        return response.json(); // 正常情况下解析为 JSON
                    })
                    .then(data => {
                        console.log('成功处理响应体 JSON 数据:', data);
                        // Assuming server returns { error: 0, message: '...' } on success
                        // And { error: non-zero, message: '...' } on failure

                        console.log(`检查响应数据中的 error 字段: data.error = ${data.error}`);

                        if (data.error === 0) {
                            // SUCCESS
                            console.log('服务器业务逻辑指示登录成功 (error === 0)。');
                            console.log('准备跳转页面到 /chat');
                            window.location.href = '/chat'; // Redirect to a success page (create this page)
                            // Or update UI to show logged-in state

                        } else {
                            // FAIL (Server indicated an error)
                            console.error('服务器业务逻辑指示登录失败 (error 非 0)。');
                            console.error('登录失败:', data);
                            loginError.textContent = data.message || '登录失败，请检查您的邮箱或密码。'; // Show server message or fallback
                            loginError.style.display = 'block';
                            console.log('已显示登录错误信息。');
                        }
                    })
                    .catch(error => {
                        // Network error or error thrown from .then()
                        console.error('Fetch 请求流程中捕获到错误:', error);
                        loginError.textContent = `登录请求失败: ${error.message}`; // Show network/fetch error
                        loginError.style.display = 'block';
                        console.log('已显示请求失败错误信息。');
                    })
                    .finally(() => {
                        console.log('Fetch 请求流程结束 (无论是成功或失败)。');
                        // Re-enable button whether success or fail
                        submitBtn.disabled = false;
                        submitBtn.textContent = '登录';
                        console.log('已重新启用登录按钮，文本恢复为 "登录"');
                    });
            } else {
                console.log('客户端验证失败，未发送登录请求。');
                // If client-side validation failed, re-enable button immediately
                // (Though it wasn't disabled yet in this flow, good practice)
                submitBtn.disabled = false;
                submitBtn.textContent = '登录';
                console.log('客户端验证失败，按钮状态重置。');
            }
        });
    }


    // --- Real-time Input Validation (MODIFIED - Captcha removed) ---
    if (emailInput && emailError && loginError) { // 确保元素存在再添加监听器
        emailInput.addEventListener('input', function () {
            console.log('邮箱输入框内容变化。');
            // Basic check to hide error on input, could add regex check here too
            emailError.style.display = 'none';
            loginError.style.display = 'none'; // Hide server error on new input
            console.log('已隐藏邮箱格式错误和登录错误信息。');
        });
    }


    if (passwordInput && passwordError && loginError) { // 确保元素存在再添加监听器
        passwordInput.addEventListener('input', function () {
            console.log('密码输入框内容变化。');
            // Hide error once condition is met or simply on any input
            if (this.value.length >= 6) {
                passwordError.style.display = 'none';
                console.log('密码长度 >= 6，已隐藏密码长度错误提示。');
            }
            loginError.style.display = 'none'; // Hide server error on new input
            console.log('已隐藏登录错误信息。');
        });
    }


    // Removed Captcha Input Listener block
});