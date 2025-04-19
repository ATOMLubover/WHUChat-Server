// script.js
document.addEventListener('DOMContentLoaded', () => {
    const messagesContainer = document.querySelector('.messages');
    // const messageInput = document.getElementById('messageInput'); // 已移除与发送消息输入框相关的代码
    // const sendButton = document.getElementById('sendButton'); // 已移除与发送按钮相关的代码
    let websocket; // WebSocket连接对象
    let currentSessionId = null; // 用于存储当前会话ID，browse_messages/history时使用

    // 假设用户ID是已知的或从登录/其他地方获取的
    // 在实际应用中，uuid应该从登录成功后的信息中获取，而不是硬编码
    const userUuid = 1; // 示例用户唯一标识

    // Function to display any system message
    function displaySystemMessage(text) {
        const systemMessage = document.createElement('div');
        systemMessage.className = 'message system-message';
        // 检查文本是否为对象或数组，如果是则进行JSON美化
        try {
            const json = JSON.parse(text);
            systemMessage.innerHTML = '<pre>' + JSON.stringify(json, null, 2) + '</pre>';
        } catch (e) {
            systemMessage.textContent = text;
        }
        messagesContainer.appendChild(systemMessage);
        scrollToBottom();
    }

    // Function to display user message (保留此函数，尽管发送消息功能移除，但历史消息可能需要显示用户消息)
    function displayUserMessage(text) {
        const userMessage = document.createElement('div');
        userMessage.className = 'message user-message';
        userMessage.textContent = text;
        messagesContainer.appendChild(userMessage);
        scrollToBottom();
    }

    // Function to handle receiving messages from WebSocket
    function onWebSocketMessage(event) {
        const message = event.data;
        console.log('Received message from WebSocket:', message);
        // 收到服务器通过WSS推送的消息，通常是模型的回答
        // 根据实际消息格式解析并显示
        displaySystemMessage('服务器消息(WSS): ' + message); // 示例：直接显示原始消息
    }

    // Function to handle WebSocket connection open
    function onWebSocketOpen() {
        console.log('WebSocket connection established.');
        displaySystemMessage('已连接到聊天服务器 (WSS)。');
        // 连接建立后，可能需要发送初始消息或认证信息
        // 如果WSS连接URL中没有包含session_id和uuid，可能需要在连接后通过消息发送
        // 例如：websocket.send(JSON.stringify({ type: 'auth', uuid: userUuid, session_id: currentSessionId }));
    }

    // Function to handle WebSocket connection close
    function onWebSocketClose(event) {
        console.log('WebSocket connection closed:', event.code, event.reason);
        let reason = '';
        if (event.code === 1000) {
            reason = '正常关闭';
        } else {
            reason = `异常关闭 (Code: ${event.code})`;
        }
        displaySystemMessage(`与聊天服务器断开连接 (WSS)。${reason}`);
        // Optionally attempt to reconnect after a delay
        setTimeout(connectWebSocket, 3000); // 3秒后尝试重连
    }

    // Function to handle WebSocket errors
    function onWebSocketError(error) {
        console.error('WebSocket error:', error);
        displaySystemMessage('WebSocket连接发生错误。');
    }

    // Function to connect to the WebSocket server
    function connectWebSocket() {
        if (websocket && (websocket.readyState === WebSocket.OPEN || websocket.readyState === WebSocket.CONNECTING)) {
            console.log('WebSocket is already connecting or open.');
            return; // 避免重复连接
        }
        console.log('Attempting to connect to WebSocket...');
        // WSS 连接URL，包含uuid和session_id（如果已知）
        // 这里暂时使用硬编码的示例参数，实际应使用 userUuid 和动态的 currentSessionId
        // 由于移除了 send_message，currentSessionId 可能不会被自动更新，
        // 如果 WSS 连接依赖于准确的 session_id，你需要找到其他方式获取它（例如，从 history 接口获取最新会话ID）
        const wsUrl = `wss://localhost:8081/api/v1/ws/trans_ans?session_id=${currentSessionId || 1}&uuid=${userUuid}`; // 示例：如果session_id未知，先用个默认值或等待获取
        websocket = new WebSocket(wsUrl);
        websocket.onopen = onWebSocketOpen;
        websocket.onmessage = onWebSocketMessage;
        websocket.onclose = onWebSocketClose;
        websocket.onerror = onWebSocketError;
    }

    // --- 发送消息函数及相关逻辑已移除 ---
    // async function sendMessage() { ... }


    // 异步函数：在页面加载后依次发送HTTPS请求并显示响应
    async function fetchInitialData() {
        // API接口的基础URL，请注意，这里需要是后端服务的地址
        const baseUrl = 'https://localhost:8081';
        const headers = { 'Content-Type': 'application/json' };
        const fetchOptions = {
            headers: headers,
            // === 添加 credentials 选项 ===
            credentials: 'include'
            // ==========================
        };


        // --- 请求 1: 获取模型列表 (/api/v1/chat/models) ---
        displaySystemMessage('正在请求模型列表...');
        try {
            // GET 请求通常可以省略 method: 'GET'
            const response = await fetch(`${baseUrl}/api/v1/chat/models`, fetchOptions);
            if (!response.ok) {
                throw new Error(`HTTP error! status: ${response.status}`);
            }
            const data = await response.json();
            displaySystemMessage('收到模型列表响应体:');
            // 使用 JSON.stringify 的第三个参数实现美化输出
            displaySystemMessage(JSON.stringify(data, null, 2));

        } catch (error) {
            displaySystemMessage(`请求模型列表失败: ${error.message}`);
            console.error('Fetch models error:', error);
        }

        // --- 请求 2: 发送一条示例消息 (/api/v1/chat/send_message) ---
        // *** 此段代码已根据您的要求移除 ***
        // ...


        // --- 请求 3: 获取指定会话的历史消息 (/api/v1/chat/browse_messages) ---
        // 假设我们要获取 userUuid 的 session_id 为 8 的会话历史 (与你原代码一致)
        const browseMessageBody = {
            uuid: userUuid,
            session_id: 8 // 示例会话ID，请根据实际情况修改
        };
        displaySystemMessage(`正在请求会话 ${browseMessageBody.session_id} 的历史消息...`);
        try {
            const response = await fetch(`${baseUrl}/api/v1/chat/browse_messages`, {
                method: 'POST', // API文档说 POST
                body: JSON.stringify(browseMessageBody),
                ...fetchOptions // 合并 fetchOptions，包含 headers 和 credentials
            });
            if (!response.ok) {
                const errorText = await response.text();
                throw new Error(`HTTP error! status: ${response.status}, body: ${errorText}`);
            }
            const data = await response.json(); // 解析 { error: int, messages: [...] } 响应体
            displaySystemMessage(`收到会话 ${browseMessageBody.session_id} 历史消息响应体:`);
            // 使用 JSON.stringify 的第三个参数实现美化输出
            displaySystemMessage(JSON.stringify(data, null, 2));

            // **考虑：** 如果 browse_messages 成功，你可能想将这个 session_id 设置为 currentSessionId
            // currentSessionId = browseMessageBody.session_id;


        } catch (error) {
            displaySystemMessage(`请求会话 ${browseMessageBody.session_id} 历史消息失败: ${error.message}`);
            console.error('Fetch browse_messages error:', error);
        }

        // --- 请求 4: 获取用户的所有会话列表 (/api/v1/chat/history) ---
        const historyBody = { uuid: userUuid };
        displaySystemMessage(`正在请求用户 ${userUuid} 的会话历史列表...`);
        try {
            const response = await fetch(`${baseUrl}/api/v1/chat/history`, {
                method: 'POST', // API文档说 POST
                body: JSON.stringify(historyBody),
                ...fetchOptions // 合并 fetchOptions，包含 headers 和 credentials
            });
            if (!response.ok) {
                const errorText = await response.text();
                throw new Error(`HTTP error! status: ${response.status}, body: ${errorText}`);
            }
            const data = await response.json(); // 解析 { error: int, sessions: [...] } 响应体
            displaySystemMessage(`收到用户 ${userUuid} 会话历史列表响应体:`);
            // 使用 JSON.stringify 的第三个参数实现美化输出
            displaySystemMessage(JSON.stringify(data, null, 2));

            // **考虑：** 如果 history 列表不为空，你可能想将第一个（或最新）会话的 ID
            // 设置为 currentSessionId，以便后续 WSS 连接使用
            if (data.error === 0 && data.sessions && data.sessions.length > 0) {
                // 假设 sessions 数组中的对象有 session_id 属性
                // currentSessionId = data.sessions[0].session_id; // 获取第一个会话ID
                console.log(`用户 ${userUuid} 共有 ${data.sessions.length} 个会话。`);
                // 如果 WSS 连接依赖于获取到的真实 session_id，可以在这里更新并连接/重连 WSS
                // connectWebSocket();
            }


        } catch (error) {
            displaySystemMessage(`请求用户 ${userUuid} 会话历史列表失败: ${error.message}`);
            console.error('Fetch history error:', error);
        }

        displaySystemMessage('所有初始HTTPS请求已发送并处理完毕。');

        // 如果 WebSocket 连接需要等待 HTTPS 请求成功获取 session_id 等信息后才能建立，
        // 可以在上述 fetch 成功的回调中（比如获取到历史会话列表后）调用 connectWebSocket()
        // 如果 WSS 可以独立连接，或者使用默认/硬编码的 session_id，则可以在 DOMContentLoaded 末尾调用
        connectWebSocket(); // 在所有初始 HTTPS 请求完成后尝试连接 WSS

    }


    // 滚动到底部函数
    function scrollToBottom() {
        messagesContainer.scrollTop = messagesContainer.scrollHeight;
    }

    // Event listeners - 发送消息相关的事件监听已移除
    // sendButton.addEventListener('click', sendMessage);
    // messageInput.addEventListener('keypress', ...);

    // Initialize scrollbar position and focus on input - 输入框相关的 focus 已移除
    scrollToBottom();
    // messageInput.focus(); // 已移除

    // --- 页面加载后的初始操作 ---

    // 1. 依次发送初始的 HTTPS 请求 (获取模型列表, 浏览历史, 会话列表)
    fetchInitialData();

    // 2. WebSocket 连接将在 fetchInitialData 完成后（或在适当位置）被调用

});