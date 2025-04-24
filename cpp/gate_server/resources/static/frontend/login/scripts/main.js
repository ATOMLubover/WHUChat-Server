document.addEventListener('DOMContentLoaded', function () {
    console.log('DOM 内容已完全加载。');

    const form = document.getElementById('loginForm');
    const emailInput = document.getElementById('email');
    const passwordInput = document.getElementById('password');
    const emailError = document.getElementById('emailError');
    const passwordError = document.getElementById('passwordError');
    const loginError = document.getElementById('loginError'); // General login error div
    const submitBtn = document.getElementById('submitBtn'); // Get submit button

    // 元素存在性检查，有助于调试
    if (!form) console.error('错误：未找到 ID 为 loginForm 的表单元素！');
    if (!emailInput) console.error('错误：未找到 ID 为 email 的输入框！');
    if (!passwordInput) console.error('错误：未找到 ID 为 password 的输入框！');
    if (!emailError) console.error('错误：未找到 ID 为 emailError 的错误提示元素！');
    if (!passwordError) console.error('错误：未找到 ID 为 passwordError 的错误提示元素！');
    if (!loginError) console.error('错误：未找到 ID 为 loginError 的错误提示元素！');
    if (!submitBtn) console.error('错误：未找到 ID 为 submitBtn 的按钮元素！');

    // --- Form Submit Logic (MODIFIED - Captcha removed, using HTTPS & Cookies) ---
    if (form) { // 确保表单元素存在再添加事件监听器
        form.addEventListener('submit', function (e) {
            console.log('捕捉到表单提交事件。');
            e.preventDefault(); // Prevent default form submission
            console.log('已阻止默认表单提交行为。');

            let isValid = true;

            // Clear previous errors
            // Use optional chaining ?. style.display if you aren't sure elements exist based on console errors
            if (emailError) emailError.style.display = 'none';
            if (passwordError) passwordError.style.display = 'none';
            if (loginError) loginError.style.display = 'none'; // Clear previous login errors
            console.log('已清除之前的错误信息显示。');

            // --- Client-Side Validation ---
            const email = emailInput ? emailInput.value : ''; // Safe access
            const emailRegex = /^[^\s@]+@[^\s@]+\.[^\s@]+$/;
            const password = passwordInput ? passwordInput.value : ''; // Safe access

            console.log(`客户端验证 - 邮箱: "${email}", 密码: "${password}"`);

            // Validate email
            if (emailInput && emailError) { // Only validate if elements exist
                if (!emailRegex.test(email)) {
                    console.log('客户端验证失败：邮箱格式不正确。');
                    emailError.style.display = 'block';
                    isValid = false;
                } else {
                    console.log('客户端验证：邮箱格式正确。');
                }
            } else if (!emailInput) {
                console.warn('邮箱输入框元素未找到，跳过邮箱验证。');
                isValid = false; // Or handle appropriately if element is critical
            }


            // Validate password
            if (passwordInput && passwordError) { // Only validate if elements exist
                // You might want to adjust this length check or add other password rules
                if (password.length < 6) {
                    console.log('客户端验证失败：密码长度小于6位。');
                    passwordError.style.display = 'block';
                    isValid = false;
                } else {
                    console.log('客户端验证：密码长度符合要求。');
                }
            } else if (!passwordInput) {
                console.warn('密码输入框元素未找到，跳过密码验证。');
                isValid = false; // Or handle appropriately
            }

            console.log(`客户端验证最终结果: isValid = ${isValid}`);

            // --- If Client-Side Validation Passes, Send Login Request ---
            if (isValid && submitBtn && loginError) { // Ensure critical elements exist before fetch
                console.log('客户端验证通过，准备发送登录请求。');
                submitBtn.disabled = true; // Disable button during request
                submitBtn.textContent = '登录中...';
                console.log('已禁用登录按钮，文本更新为 "登录中..."');

                const loginData = {
                    email: email,
                    password: password
                };

                const loginUrl = 'https://127.0.0.1:8091/api/v1/gate/login'; // 目标登录接口 URL (已是 HTTPS)
                console.log(`发送 Fetch POST 请求到: ${loginUrl}`);
                console.log('请求体数据:', JSON.stringify(loginData));

                fetch(loginUrl, {
                    method: 'POST',
                    headers: {
                        'Content-Type': 'application/json',
                    },
                    body: JSON.stringify(loginData),
                    // --- 修改点：添加 credentials 选项以携带 Cookie ---
                    credentials: 'include' // 'include' 表示在跨域和同域请求中都携带 Cookie
                    // 注意：如果目标是跨域且需要携带 Cookie，服务器必须在响应头中包含 Access-Control-Allow-Credentials: true
                    // 并且 Access-Control-Allow-Origin 不能是 '*'，必须是具体的源
                    // 在 localhost 开发环境下通常不是问题，但在生产环境中需要注意 CORS 配置
                    // ------------------------------------------------------
                })
                    .then(async response => {
                        console.log('收到服务器响应。');
                        console.log(`响应 HTTP 状态码: ${response.status}, response.ok: ${response.ok}`);

                        if (!response.ok) {
                            console.log('响应状态码非成功 (response.ok 为 false)，尝试读取响应体。');
                            const errorBodyText = await response.text();
                            console.log('读取到响应体文本:', errorBodyText);

                            let errorData = { message: `服务器返回非成功状态码 ${response.status}` };

                            try {
                                const parsed = JSON.parse(errorBodyText);
                                console.log('尝试将响应体文本解析为 JSON 成功:', parsed);
                                if (parsed && typeof parsed === 'object') {
                                    // 如果服务器返回 { error: 1, message: '...' } 这样的格式，使用其 message
                                    errorData.message = parsed.message || errorData.message;
                                    // 如果服务器有特定的错误代码（如 error 字段），也可以在这里处理
                                    if (parsed.error !== undefined) {
                                        errorData.error = parsed.error;
                                    }
                                }
                            } catch (parseError) {
                                console.error('将响应体文本解析为 JSON 失败:', parseError);
                                errorData.message += `. 响应体非 JSON 或解析错误。原始文本(部分): "${errorBodyText.substring(0, 200)}..."`;
                            }

                            // 抛出错误，由 .catch() 统一处理
                            throw new Error(errorData.message);
                        }

                        console.log('响应状态码成功 (response.ok 为 true)，解析响应体为 JSON。');
                        return response.json();
                    })
                    .then(data => {
                        console.log('成功处理响应体 JSON 数据:', data);

                        // Assuming server returns { error: 0, message: '...', data: {...} } on success
                        // And { error: non-zero, message: '...' } on failure

                        console.log(`检查响应数据中的 error 字段: data.error = ${data.error}`);

                        // Check for successful login based on server response structure
                        if (data && data.error === 0) { // Check if data exists and error is 0
                            // SUCCESS
                            console.log('服务器业务逻辑指示登录成功 (error === 0)。');
                            console.log('准备跳转页面到 /chat');
                            // 登录成功后，浏览器通常会自动保存响应中的 Set-Cookie 头设置的 Cookie
                            window.location.href = '/chat'; // Redirect to a success page
                        } else {
                            // FAIL (Server indicated an error or unexpected structure)
                            console.error('服务器业务逻辑指示登录失败 (error 非 0) 或响应结构异常。');
                            console.error('登录失败响应数据:', data);
                            // Use server message if available, otherwise use a fallback
                            loginError.textContent = (data && data.message) ? data.message : '登录失败，请检查您的邮箱或密码或服务器响应异常。';
                            loginError.style.display = 'block';
                            console.log('已显示登录错误信息。');
                        }
                    })
                    .catch(error => {
                        // Network error, JSON parsing error, or error thrown from .then()
                        console.error('Fetch 请求流程中捕获到错误:', error);
                        if (loginError) {
                            loginError.textContent = `登录请求失败: ${error.message}`; // Show network/fetch error
                            loginError.style.display = 'block';
                        } else {
                            console.error('错误提示元素 loginError 未找到，无法显示错误信息。');
                        }
                        console.log('已显示请求失败错误信息。');
                    })
                    .finally(() => {
                        console.log('Fetch 请求流程结束 (无论是成功或失败)。');
                        // Re-enable button whether success or fail
                        if (submitBtn) {
                            submitBtn.disabled = false;
                            submitBtn.textContent = '登录';
                        }
                        console.log('已重新启用登录按钮，文本恢复为 "登录"');
                    });
            } else {
                console.log('客户端验证失败或必要元素缺失，未发送登录请求。');
                // If client-side validation failed, re-enable button immediately
                if (submitBtn) {
                    submitBtn.disabled = false;
                    submitBtn.textContent = '登录';
                }
                console.log('客户端验证失败，按钮状态重置。');
            }
        });
    }


    // --- Real-time Input Validation (MODIFIED - Captcha removed) ---
    // Add checks for element existence before adding listeners
    if (emailInput && emailError && loginError) {
        emailInput.addEventListener('input', function () {
            console.log('邮箱输入框内容变化。');
            emailError.style.display = 'none';
            loginError.style.display = 'none'; // Hide server error on new input
            console.log('已隐藏邮箱格式错误和登录错误信息。');
        });
    } else {
        console.warn('邮箱相关的输入或错误元素未完全找到，跳过实时邮箱验证监听。');
    }


    if (passwordInput && passwordError && loginError) {
        passwordInput.addEventListener('input', function () {
            console.log('密码输入框内容变化。');
            if (this.value.length >= 6) { // Basic length check for hiding error
                passwordError.style.display = 'none';
                console.log('密码长度 >= 6，已隐藏密码长度错误提示。');
            }
            loginError.style.display = 'none'; // Hide server error on new input
            console.log('已隐藏登录错误信息。');
        });
    } else {
        console.warn('密码相关的输入或错误元素未完全找到，跳过实时密码验证监听。');
    }
});