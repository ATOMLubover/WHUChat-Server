document.addEventListener('DOMContentLoaded', function () {
    const form = document.getElementById('loginForm');
    const emailInput = document.getElementById('email');
    const passwordInput = document.getElementById('password');
    // Removed captchaInput
    const emailError = document.getElementById('emailError');
    const passwordError = document.getElementById('passwordError');
    // Removed captchaError
    const loginError = document.getElementById('loginError'); // General login error div
    // Removed captchaSent
    // Removed sendCaptchaBtn
    const submitBtn = document.getElementById('submitBtn'); // Get submit button

    // --- Send Captcha Logic (REMOVED) ---
    // The entire block for sendCaptchaBtn.addEventListener was here

    // --- Countdown Function (REMOVED) ---
    // The startCountdown function was here

    // --- Form Submit Logic (MODIFIED - Captcha removed) ---
    form.addEventListener('submit', function (e) {
        e.preventDefault(); // Prevent default form submission
        let isValid = true;

        // Clear previous errors
        emailError.style.display = 'none';
        passwordError.style.display = 'none';
        // Removed captchaError clearing
        loginError.style.display = 'none'; // Clear previous login errors

        // --- Client-Side Validation ---
        const email = emailInput.value;
        const emailRegex = /^[^\s@]+@[^\s@]+\.[^\s@]+$/;
        if (!emailRegex.test(email)) {
            emailError.style.display = 'block';
            isValid = false;
        }

        const password = passwordInput.value;
        // You might want to adjust this length check or add other password rules
        if (password.length < 6) {
            passwordError.style.display = 'block';
            isValid = false;
        }

        // Removed Captcha Validation block

        // --- If Client-Side Validation Passes, Send Login Request ---
        if (isValid) {
            submitBtn.disabled = true; // Disable button during request
            submitBtn.textContent = '登录中...';

            const loginData = {
                email: email,
                password: password
                // Removed captcha field
            };

            fetch('http://localhost:8080/api/v1/login', { // Target API endpoint remains
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify(loginData)
            })
                .then(response => {
                    if (!response.ok) {
                        // Try to parse error json, otherwise throw status text
                        return response.json().catch(() => {
                            throw new Error(`服务器错误: ${response.statusText} (${response.status})`);
                        }).then(errData => {
                            // Throw an error with the message from server if available
                            throw new Error(errData.message || `服务器返回错误 ${response.status}`);
                        });
                    }
                    return response.json(); // Parse successful response
                })
                .then(data => {
                    // Assuming server returns { error: 0, message: '...' } on success
                    // And { error: non-zero, message: '...' } on failure
                    if (data.error === 0) {
                        // SUCCESS
                        console.log('登录成功', data);
                        // Redirect to a success page (create this page)
                        window.location.href = '/chat';
                        // Or update UI to show logged-in state
                    } else {
                        // FAIL (Server indicated an error)
                        console.error('登录失败:', data);
                        loginError.textContent = data.message || '登录失败，请检查您的邮箱或密码。'; // Show server message or fallback
                        loginError.style.display = 'block';
                    }
                })
                .catch(error => {
                    // Network error or error thrown from .then()
                    console.error('登录请求错误:', error);
                    loginError.textContent = `登录请求失败: ${error.message}`; // Show network/fetch error
                    loginError.style.display = 'block';
                })
                .finally(() => {
                    // Re-enable button whether success or fail
                    submitBtn.disabled = false;
                    submitBtn.textContent = '登录';
                });
        } else {
            // If client-side validation failed, re-enable button immediately
            // (Though it wasn't disabled yet in this flow, good practice)
            submitBtn.disabled = false;
            submitBtn.textContent = '登录';
        }
    });

    // --- Real-time Input Validation (MODIFIED - Captcha removed) ---
    emailInput.addEventListener('input', function () {
        // Basic check to hide error on input, could add regex check here too
        emailError.style.display = 'none';
        loginError.style.display = 'none'; // Hide server error on new input
    });

    passwordInput.addEventListener('input', function () {
        // Hide error once condition is met or simply on any input
        if (this.value.length >= 6) {
            passwordError.style.display = 'none';
        }
        loginError.style.display = 'none'; // Hide server error on new input
    });

    // Removed Captcha Input Listener block
});