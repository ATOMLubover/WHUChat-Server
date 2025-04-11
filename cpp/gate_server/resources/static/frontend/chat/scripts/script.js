// script.js
document.addEventListener('DOMContentLoaded', () => {
    const messagesContainer = document.querySelector('.messages');
    const messageInput = document.getElementById('messageInput');
    const sendButton = document.getElementById('sendButton');
    let websocket;

    // Function to display any system message
    function displaySystemMessage(text) {
        const systemMessage = document.createElement('div');
        systemMessage.className = 'message system-message';
        systemMessage.textContent = text;
        messagesContainer.appendChild(systemMessage);
        scrollToBottom();
    }

    // Function to display user message
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
        displaySystemMessage('服务器消息: ' + message);
    }

    // Function to handle WebSocket connection open
    function onWebSocketOpen() {
        console.log('WebSocket connection established.');
        displaySystemMessage('已连接到聊天服务器。');
    }

    // Function to handle WebSocket connection close
    function onWebSocketClose() {
        console.log('WebSocket connection closed.');
        displaySystemMessage('与聊天服务器断开连接。');
        // Optionally attempt to reconnect after a delay
        setTimeout(connectWebSocket, 3000);
    }

    // Function to handle WebSocket errors
    function onWebSocketError(error) {
        console.error('WebSocket error:', error);
        displaySystemMessage('WebSocket连接发生错误。');
    }

    // Function to connect to the WebSocket server
    function connectWebSocket() {
        console.log('Attempting to connect to WebSocket...');
        websocket = new WebSocket('ws://localhost:8081/api/v1/ws/trans_ans?session_id=1');
        websocket.onopen = onWebSocketOpen;
        websocket.onmessage = onWebSocketMessage;
        websocket.onclose = onWebSocketClose;
        websocket.onerror = onWebSocketError;
    }

    // 发送消息函数 (currently only displays locally)
    function sendMessage() {
        const messageText = messageInput.value.trim();
        if (messageText === '') return;

        // Display user message immediately
        displayUserMessage(messageText);

        // Clear input field
        messageInput.value = '';

        // In a real application, you would send this message via WebSocket
        // if (websocket && websocket.readyState === WebSocket.OPEN) {
        //     websocket.send(messageText);
        // } else {
        //     displaySystemMessage('无法发送消息，WebSocket连接未打开。');
        // }

        // Ensure input is focused after sending
        messageInput.focus();
    }

    // 滚动到底部函数
    function scrollToBottom() {
        messagesContainer.scrollTop = messagesContainer.scrollHeight;
    }

    // Event listeners
    sendButton.addEventListener('click', sendMessage);
    messageInput.addEventListener('keypress', (e) => {
        if (e.key === 'Enter' && !e.shiftKey) {
            e.preventDefault();
            sendMessage();
        }
    });

    // Initialize scrollbar position and focus on input
    scrollToBottom();
    messageInput.focus();

    // Connect to WebSocket after the page has loaded
    connectWebSocket();
});